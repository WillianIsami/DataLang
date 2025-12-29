module Main

import System
import System.File
import Data.List
import Data.String
import Data.Maybe

%default partial

data JSON
  = JNull
  | JBool Bool
  | JString String
  | JNumber String
  | JObject (List (String, JSON))
  | JArray (List JSON)

lookupKey : String -> List (String, JSON) -> Maybe JSON
lookupKey _ [] = Nothing
lookupKey k ((k', v) :: xs) =
  if k == k' then Just v else lookupKey k xs

stripPrefix : List Char -> List Char -> Maybe (List Char)
stripPrefix [] ys = Just ys
stripPrefix (x :: xs) (y :: ys) =
  if x == y then stripPrefix xs ys else Nothing
stripPrefix _ _ = Nothing

mapM_ : Monad m => (a -> m ()) -> List a -> m ()
mapM_ f [] = pure ()
mapM_ f (x :: xs) = do f x; mapM_ f xs

mapMaybe' : (a -> Maybe b) -> List a -> List b
mapMaybe' f [] = []
mapMaybe' f (x :: xs) =
  case f x of
    Just y => y :: mapMaybe' f xs
    Nothing => mapMaybe' f xs

trimStart : List Char -> List Char
trimStart [] = []
trimStart (c :: cs) =
  if isSpace c then trimStart cs else c :: cs

parseString : List Char -> Either String (String, List Char)
parseString ('"' :: cs) = go "" cs
  where
    go : String -> List Char -> Either String (String, List Char)
    go acc [] = Left "EOF while parsing string"
    go acc ('"' :: rest) = Right (acc, rest)
    go acc ('\\' :: 'n' :: rest) = go (acc ++ "\n") rest
    go acc ('\\' :: 't' :: rest) = go (acc ++ "\t") rest
    go acc ('\\' :: '"' :: rest) = go (acc ++ "\"") rest
    go acc ('\\' :: '\\' :: rest) = go (acc ++ "\\") rest
    go acc (c :: rest) = go (acc ++ singleton c) rest
parseString _ = Left "Expected string"

parseNumber : List Char -> Either String (String, List Char)
parseNumber cs =
  let (digits, rest) = span (\c => isDigit c || c == '.' || c == '-' || c == '+') cs in
  if digits == [] then Left "Expected number" else Right (pack digits, rest)

parseLiteral : List Char -> Either String (JSON, List Char)
parseLiteral cs =
  case stripPrefix (unpack "true") cs of
    Just rest => Right (JBool True, rest)
    Nothing =>
      case stripPrefix (unpack "false") cs of
        Just rest2 => Right (JBool False, rest2)
        Nothing =>
          case stripPrefix (unpack "null") cs of
            Just rest3 => Right (JNull, rest3)
            Nothing => Left "Unknown literal"

mutual
  parseArray : List Char -> Either String (List JSON, List Char)
  parseArray cs =
    case trimStart cs of
      ']' :: rest => Right ([], rest)
      other => do
        (v, rest1) <- parseValue other
        let rest1' = trimStart rest1
        case rest1' of
          ',' :: rest2 => do
            (more, rest3) <- parseArray rest2
            Right (v :: more, rest3)
          ']' :: rest2 => Right ([v], rest2)
          _ => Left "Expected ',' or ']' in array"

  parseObjectEntries : List Char -> Either String (List (String, JSON), List Char)
  parseObjectEntries cs =
    case trimStart cs of
      '}' :: rest => Right ([], rest)
      _ => do
        (k, rest1) <- parseString (trimStart cs)
        case trimStart rest1 of
          ':' :: rest2 => do
            (v, rest3) <- parseValue (trimStart rest2)
            case trimStart rest3 of
              ',' :: rest4 => do
                (more, rest5) <- parseObjectEntries rest4
                Right ((k, v) :: more, rest5)
              '}' :: rest4 => Right ([(k, v)], rest4)
              _ => Left "Expected ',' or '}' in object"
          _ => Left "Expected ':' after object key"

  parseValue : List Char -> Either String (JSON, List Char)
  parseValue cs =
    case trimStart cs of
      '"' :: _ => do (s, rest) <- parseString (trimStart cs); Right (JString s, rest)
      '[' :: rest => do (arr, rest') <- parseArray rest; Right (JArray arr, rest')
      '{' :: rest => do (entries, rest') <- parseObjectEntries rest; Right (JObject entries, rest')
      c :: _ =>
        if isDigit c || c == '-' then do
          (n, rest) <- parseNumber (trimStart cs)
          Right (JNumber n, rest)
        else do
          (lit, rest) <- parseLiteral (trimStart cs)
          Right (lit, rest)
      [] => Left "Unexpected end of input"

parseJSON : String -> Either String JSON
parseJSON s = do
  (v, rest) <- parseValue (unpack s)
  case trimStart rest of
    [] => Right v
    _  => Left "Trailing characters after JSON"


-- AST / tipos da linguagem
data TypeNode
  = TInt
  | TFloat
  | TBool
  | TString
  | TVector
  | TSeries
  | TDataFrame
  | TUnknown
  | TArray TypeNode
  | TTuple (List TypeNode)

record FieldDecl where
  constructor MkField
  fname : String
  ftype : TypeNode

record Param where
  constructor MkParam
  pname : String
  ptype : TypeNode

data Decl
  = ImportDecl String (Maybe String)
  | ExportDecl String
  | DataDecl String (List FieldDecl)
  | LetDecl String TypeNode
  | FnDecl String (List Param) (Maybe TypeNode) Bool -- hasBody
  | OtherDecl String

record Program where
  constructor MkProgram
  decls : List Decl


-- Decoders JSON -> AST
expectString : String -> JSON -> Either String String
expectString what (JString s) = Right s
expectString what _ = Left $ "Esperava string em " ++ what

expectArray : String -> JSON -> Either String (List JSON)
expectArray what (JArray xs) = Right xs
expectArray what _ = Left $ "Esperava array em " ++ what

expectObj : String -> JSON -> Either String (List (String, JSON))
expectObj what (JObject xs) = Right xs
expectObj what _ = Left $ "Esperava objeto em " ++ what

decodeType : JSON -> Either String TypeNode
decodeType (JObject fields) =
  case lookupKey "kind" fields of
    Just (JString "Int") => Right TInt
    Just (JString "Float") => Right TFloat
    Just (JString "Bool") => Right TBool
    Just (JString "String") => Right TString
    Just (JString "Vector") => Right TVector
    Just (JString "Series") => Right TSeries
    Just (JString "DataFrame") => Right TDataFrame
    Just (JString "?") =>
      case lookupKey "arrayOf" fields of
        Just t => do inner <- decodeType t; Right (TArray inner)
        Nothing =>
          case lookupKey "tuple" fields of
            Just (JArray ts) => do inners <- traverse decodeType ts; Right (TTuple inners)
            _ => Right TUnknown
    _ => Right TUnknown
decodeType _ = Left "Tipo inválido"

decodeField : JSON -> Either String FieldDecl
decodeField (JObject fields) = do
  n <- case lookupKey "name" fields of
         Just v => expectString "field name" v
         Nothing => Left "Campo sem nome"
  t <- case lookupKey "fieldType" fields of
         Just v => decodeType v
         Nothing => Right TUnknown
  Right $ MkField n t
decodeField _ = Left "Field inválido"

decodeParam : JSON -> Either String Param
decodeParam (JObject fields) = do
  n <- case lookupKey "name" fields of
         Just v => expectString "param name" v
         Nothing => Left "Param sem nome"
  t <- case lookupKey "paramType" fields of
         Just v => decodeType v
         Nothing => Right TUnknown
  Right $ MkParam n t
decodeParam _ = Left "Param inválido"

hasNonEmptyBody : JSON -> Bool
hasNonEmptyBody (JObject fields) =
  case lookupKey "statements" fields of
    Just (JArray stmts) => length stmts > 0
    _ => False
hasNonEmptyBody _ = False

decodeDecl : JSON -> Either String Decl
decodeDecl (JObject fields) =
  case lookupKey "type" fields of
    Just (JString "ImportDecl") => do
      m <- case lookupKey "module" fields of
             Just v => expectString "module" v
             Nothing => Left "Import sem módulo"
      let alias = case lookupKey "alias" fields of
                    Just JNull => Nothing
                    Just (JString s) => Just s
                    _ => Nothing
      Right $ ImportDecl m alias
    Just (JString "ExportDecl") => do
      n <- case lookupKey "name" fields of
             Just v => expectString "export name" v
             Nothing => Left "Export sem nome"
      Right $ ExportDecl n
    Just (JString "DataDecl") => do
      n <- case lookupKey "name" fields of
             Just v => expectString "data name" v
             Nothing => Left "DataDecl sem nome"
      fsJson <- case lookupKey "fields" fields of
                  Just v => expectArray "fields" v
                  Nothing => Right []
      fs <- traverse decodeField fsJson
      Right $ DataDecl n fs
    Just (JString "LetDecl") => do
      n <- case lookupKey "name" fields of
             Just v => expectString "let name" v
             Nothing => Left "Let sem nome"
      t <- case lookupKey "typeAnnotation" fields of
             Just v => decodeType v
             Nothing => Right TUnknown
      Right $ LetDecl n t
    Just (JString "FnDecl") => do
      n <- case lookupKey "name" fields of
             Just v => expectString "fn name" v
             Nothing => Left "Fn sem nome"
      psJson <- case lookupKey "params" fields of
                  Just v => expectArray "params" v
                  Nothing => Right []
      ps <- traverse decodeParam psJson
      rt <- case lookupKey "returnType" fields of
              Just JNull => Right Nothing
              Just v => do t <- decodeType v; Right (Just t)
              Nothing => Right Nothing
      let bodyOk = case lookupKey "body" fields of
                     Just b => hasNonEmptyBody b
                     Nothing => False
      Right $ FnDecl n ps rt bodyOk
    Just (JString other) =>
      Right $ OtherDecl other
    _ => Left "Decl sem campo 'type'"
decodeDecl _ = Left "Decl inválido"

decodeProgram : JSON -> Either String Program
decodeProgram (JObject fields) =
  case lookupKey "type" fields of
    Just (JString "Program") =>
      case lookupKey "declarations" fields of
        Just declsJson => do
          arr <- expectArray "declarations" declsJson
          decls <- traverse decodeDecl arr
          Right $ MkProgram decls
        Nothing => Left "Program sem declarações"
    _ => Left "Root não é Program"
decodeProgram _ = Left "Program inválido"


-- Verificações / invariantes

uniqueNames : List String -> List String
uniqueNames xs = go xs [] []
  where
    go : List String -> List String -> List String -> List String
    go [] _ dups = reverse dups
    go (n :: rest) seen dups =
      if elem n seen then go rest seen (n :: dups)
                     else go rest (n :: seen) dups

verifyProgram : Program -> List String
verifyProgram (MkProgram decls) =
  let fnDecls : List (String, List Param, Bool)
      fnDecls = mapMaybe' (\d => case d of
                                  FnDecl n ps _ hasBody => Just (n, ps, hasBody)
                                  _ => Nothing) decls
      fnNames = map (\(n, _, _) => n) fnDecls
      dataDecls : List (String, List FieldDecl)
      dataDecls = mapMaybe' (\d => case d of
                                    DataDecl n fs => Just (n, fs)
                                    _ => Nothing) decls
      dataNames = map fst dataDecls
      exports : List String
      exports = mapMaybe' (\d => case d of
                                  ExportDecl n => Just n
                                  _ => Nothing) decls
      paramDupErrors : List String
      paramDupErrors = concat $ map (\(n, ps, _) =>
        let names = map (\p => p.pname) ps
            dups = uniqueNames names
        in if length dups > 0
              then ["Parâmetros duplicados em função " ++ n ++ ": " ++ show dups]
              else []) fnDecls
      fieldDupErrors : List String
      fieldDupErrors = concat $ map (\(n, fs) =>
        let names = map (\f => f.fname) fs
            dups = uniqueNames names
        in if length dups > 0
              then ["Campos duplicados em data " ++ n ++ ": " ++ show dups]
              else []) dataDecls
      exportErrors : List String
      exportErrors = filter (\msg => msg /= "") $
        map (\n => if elem n fnNames || elem n dataNames
                    then "" else "Export " ++ n ++ " não encontrado em declarações") exports
      fnDupErrors : List String
      fnDupErrors = let dups = uniqueNames fnNames in
                    if length dups > 0 then ["Funções duplicadas: " ++ show dups] else []
      emptyNameErrors : List String
      emptyNameErrors = filter (\s => s /= "") $
        (map (\n => if n == "" then "Função com nome vazio" else "") fnNames) ++
        (map (\n => if n == "" then "Data com nome vazio" else "") dataNames)
      bodyErrors : List String
      bodyErrors = concat $ map (\(n, _, hasBody) =>
        if hasBody then [] else ["Função " ++ n ++ " com corpo vazio/ausente"]) fnDecls
  in fnDupErrors ++ paramDupErrors ++ fieldDupErrors ++ exportErrors ++ emptyNameErrors ++ bodyErrors


-- Entrada/Saída
verifyFile : String -> IO (Either String (List String))
verifyFile path = do
  exists <- exists path
  if not exists then pure (Left "Arquivo não encontrado") else do
    mContents <- readFile path
    case mContents of
      Left _ => pure (Left "Não foi possível ler o arquivo")
      Right contents =>
        case parseJSON contents of
          Left err => pure (Left ("Erro de parsing JSON: " ++ err))
          Right json =>
            case decodeProgram json of
              Left err => pure (Left ("Erro de parsing da AST: " ++ err))
              Right prog => pure (Right (verifyProgram prog))

main : IO ()
main = do
  args <- getArgs
  let jsonPath = case args of
                   (_ :: path :: _) => path
                   (path :: _) => path
                   _ => ""
  case jsonPath == "" of
    True => do
      putStrLn "Uso: datalang_verify <ast.json>"
      exitWith (ExitFailure 1)
    False => do
      res <- verifyFile jsonPath
      case res of
        Left err => do
          putStrLn $ "[Idris verify] Erro: " ++ err
          exitWith (ExitFailure 1)
        Right [] => do
          putStrLn "[Idris verify] AST OK (Program válido e invariantes satisfeitas)"
          pure ()
        Right errs => do
          putStrLn "[Idris verify] Erros:"
          mapM_ putStrLn (map (\e => "  - " ++ e) errs)
          exitWith (ExitFailure 1)
