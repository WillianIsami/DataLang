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
  | TVoid
  | TUnknown
  | TCustom String
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

mapM_ : Monad m => (a -> m ()) -> List a -> m ()
mapM_ f [] = pure ()
mapM_ f (x :: xs) = do f x; mapM_ f xs

mapMaybe' : (a -> Maybe b) -> List a -> List b
mapMaybe' f [] = []
mapMaybe' f (x :: xs) =
  case f x of
    Just y => y :: mapMaybe' f xs
    Nothing => mapMaybe' f xs

eqType : TypeNode -> TypeNode -> Bool
eqType TInt TInt = True
eqType TFloat TFloat = True
eqType TBool TBool = True
eqType TString TString = True
eqType TVector TVector = True
eqType TSeries TSeries = True
eqType TDataFrame TDataFrame = True
eqType TVoid TVoid = True
eqType TUnknown TUnknown = True
eqType (TCustom a) (TCustom b) = a == b
eqType TUnknown _ = False
eqType _ TUnknown = False
eqType (TArray a) (TArray b) = eqType a b
eqType (TTuple xs) (TTuple ys) = length xs == length ys && all (\(x,y) => eqType x y) (zip xs ys)
eqType _ _ = False

Env : Type
Env = List (String, TypeNode)

FunEnv : Type
FunEnv = List (String, (List TypeNode, TypeNode))

DataEnv : Type
DataEnv = List (String, List FieldDecl)

compatType : TypeNode -> TypeNode -> Bool
compatType TUnknown _ = True
compatType _ TUnknown = True
compatType TInt TInt = True
compatType TFloat TFloat = True
compatType TBool TBool = True
compatType TString TString = True
compatType TVector TVector = True
compatType TSeries TSeries = True
compatType TDataFrame TDataFrame = True
compatType TVoid TVoid = True
compatType (TCustom a) (TCustom b) = a == b
compatType (TArray a) (TArray b) = compatType a b
compatType (TTuple xs) (TTuple ys) = length xs == length ys && all (\(x,y) => compatType x y) (zip xs ys)
compatType _ _ = False

lookupEnv : String -> Env -> Maybe TypeNode
lookupEnv _ [] = Nothing
lookupEnv n ((k,v) :: xs) = if n == k then Just v else lookupEnv n xs

lookupFun : String -> FunEnv -> Maybe (List TypeNode, TypeNode)
lookupFun _ [] = Nothing
lookupFun n ((k,v) :: xs) = if n == k then Just v else lookupFun n xs

lookupData : String -> DataEnv -> Maybe (List FieldDecl)
lookupData _ [] = Nothing
lookupData n ((k,v) :: xs) = if n == k then Just v else lookupData n xs

builtinFuns : FunEnv
builtinFuns =
  [ ("sum", ([TArray TUnknown], TUnknown))
  , ("mean", ([TArray TUnknown], TFloat))
  , ("count", ([TArray TUnknown], TInt))
  , ("min", ([TArray TUnknown], TUnknown))
  , ("max", ([TArray TUnknown], TUnknown))
  , ("print", ([TUnknown], TVoid))
  ]

data BinOp = BAdd | BSub | BMul | BDiv | BMod | BEq | BNeq | BLt | BLte | BGt | BGte | BAnd | BOr
data UnOp = UNeg | UNot

mutual
  data ExprNode
    = ELit TypeNode String -- store literal as string rep
    | EVar String
    | EBin BinOp ExprNode ExprNode
    | EUn UnOp ExprNode
    | ECall String (List ExprNode)
    | EArray (List ExprNode)
    | EAssign String ExprNode
    | EIndex ExprNode ExprNode
    | EMember ExprNode String
    | ELambda (List Param) ExprNode
    | EPipeline ExprNode (List Transform)
    | ERange ExprNode ExprNode
    | ELoad String
    | ESave ExprNode String

  data Transform
    = TFilter Param ExprNode
    | TMap Param ExprNode
    | TReduce ExprNode Param Param ExprNode
    | TCall String (List ExprNode)
    | TSelect (List String)
    | TGroupBy (List String)

data StmtNode
  = SLet String TypeNode ExprNode
  | SAssign String ExprNode
  | SExpr ExprNode
  | SReturn (Maybe ExprNode)
  | SIf ExprNode (List StmtNode) (List StmtNode)
  | SFor String ExprNode (List StmtNode)

data Decl
  = ImportDecl String (Maybe String)
  | ExportDecl String
  | DataDecl String (List FieldDecl)
  | LetDecl String TypeNode JSON -- keep initializer json to check
  | FnDecl String (List Param) (Maybe TypeNode) JSON -- body as JSON for full checking
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
decodeType JNull = Right TUnknown
decodeType (JObject fields) =
  case lookupKey "kind" fields of
    Just (JString "Int") => Right TInt
    Just (JString "Float") => Right TFloat
    Just (JString "Bool") => Right TBool
    Just (JString "String") => Right TString
    Just (JString "Vector") => Right TVector
    Just (JString "Series") => Right TSeries
    Just (JString "DataFrame") => Right TDataFrame
    Just (JString "Void") => Right TVoid
    Just (JString "?") =>
      case lookupKey "arrayOf" fields of
        Just t => do inner <- decodeType t; Right (TArray inner)
        Nothing =>
          case lookupKey "tuple" fields of
            Just (JArray ts) => do inners <- traverse decodeType ts; Right (TTuple inners)
            _ => Right TUnknown
    Just (JString other) => Right (TCustom other)
    _ => Right TUnknown
decodeType _ = Left "Tipo inválido"

decodeField : JSON -> Either String FieldDecl
decodeField (JObject fields) = do
  n <- case lookupKey "name" fields of
         Just v => expectString "field name" v
         Nothing => Left "Campo sem nome"
  t <- case lookupKey "fieldType" fields of
         Just JNull => Right TUnknown
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
         Just JNull => Right TUnknown
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
             Just JNull => Right TUnknown
             Just v => decodeType v
             Nothing => Right TUnknown
      initVal <- case lookupKey "initializer" fields of
                   Just v => Right v
                   Nothing => Left "Let sem inicializador"
      Right $ LetDecl n t initVal
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
      bodyJson <- case lookupKey "body" fields of
                    Just b => Right b
                    Nothing => Left "Fn sem corpo"
      Right $ FnDecl n ps rt bodyJson
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


-- Expressões e comandos

binOpFromString : String -> Maybe BinOp
binOpFromString "+" = Just BAdd
binOpFromString "-" = Just BSub
binOpFromString "*" = Just BMul
binOpFromString "/" = Just BDiv
binOpFromString "%" = Just BMod
binOpFromString "==" = Just BEq
binOpFromString "!=" = Just BNeq
binOpFromString "<" = Just BLt
binOpFromString "<=" = Just BLte
binOpFromString ">" = Just BGt
binOpFromString ">=" = Just BGte
binOpFromString "&&" = Just BAnd
binOpFromString "||" = Just BOr
binOpFromString _ = Nothing

unOpFromString : String -> Maybe UnOp
unOpFromString "-" = Just UNeg
unOpFromString "!" = Just UNot
unOpFromString _ = Nothing

decodeLiteralType : String -> Either String TypeNode
decodeLiteralType "Int" = Right TInt
decodeLiteralType "Float" = Right TFloat
decodeLiteralType "Bool" = Right TBool
decodeLiteralType "String" = Right TString
decodeLiteralType _ = Left "Literal de tipo desconhecido"

mutual
  decodeExpr : JSON -> Either String ExprNode
  decodeExpr (JObject fields) =
    case lookupKey "type" fields of
      Just (JString "Literal") => do
        lt <- case lookupKey "literalType" fields of
                Just (JString s) => decodeLiteralType s
                _ => Left "Literal sem tipo"
        val <- case lookupKey "value" fields of
                 Just (JNumber n) => Right n
                 Just (JString s) => Right s
                 Just (JBool b) => Right (if b then "true" else "false")
                 Just JNull => Right "null"
                 _ => Left "Literal sem valor"
        Right $ ELit lt val
      Just (JString "Identifier") => do
        n <- case lookupKey "name" fields of
               Just (JString s) => Right s
               _ => Left "Identifier sem nome"
        Right $ EVar n
      Just (JString "BinaryExpr") => do
        l <- case lookupKey "left" fields of
               Just v => decodeExpr v
               _ => Left "Binary sem left"
        r <- case lookupKey "right" fields of
               Just v => decodeExpr v
               _ => Left "Binary sem right"
        opStr <- case lookupKey "op" fields of
                   Just (JString s) => Right s
                   _ => Left "Binary sem op"
        case binOpFromString opStr of
          Just op => Right $ EBin op l r
          Nothing => Left "Operador binário desconhecido"
      Just (JString "UnaryExpr") => do
        opStr <- case lookupKey "op" fields of
                   Just (JString s) => Right s
                   _ => Left "Unary sem op"
        operand <- case lookupKey "operand" fields of
                     Just v => decodeExpr v
                     _ => Left "Unary sem operand"
        case unOpFromString opStr of
          Just op => Right $ EUn op operand
          Nothing => Left "Operador unário desconhecido"
      Just (JString "CallExpr") => do
        cal <- case lookupKey "callee" fields of
                 Just (JObject cfields) =>
                   case lookupKey "name" cfields of
                     Just (JString s) => Right s
                     _ => Left "Call sem callee name"
                 _ => Left "Call sem callee"
        args <- case lookupKey "args" fields of
                  Just (JArray xs) => traverse decodeExpr xs
                  _ => Right []
        Right $ ECall cal args
      Just (JString "ArrayLiteral") => do
        elems <- case lookupKey "elements" fields of
                   Just (JArray xs) => traverse decodeExpr xs
                   _ => Right []
        Right $ EArray elems
      Just (JString "AssignExpr") => do
        tgt <- case lookupKey "target" fields of
                 Just (JObject tf) =>
                   case lookupKey "name" tf of
                     Just (JString s) => Right s
                     _ => Left "Assign target inválido"
                 _ => Left "Assign sem target"
        v <- case lookupKey "value" fields of
               Just val => decodeExpr val
               _ => Left "Assign sem value"
        Right $ EAssign tgt v
      Just (JString "MemberExpr") => do
        obj <- case lookupKey "object" fields of
                 Just v => decodeExpr v
                 _ => Left "Member sem objeto"
        prop <- case lookupKey "property" fields of
                  Just (JString s) => Right s
                  _ =>
                    case lookupKey "member" fields of
                      Just (JString s) => Right s
                      _ => Left "Member sem property"
        Right $ EMember obj prop
      Just (JString "IndexExpr") => do
        arr <- case lookupKey "array" fields of
                 Just v => decodeExpr v
                 _ =>
                   case lookupKey "object" fields of
                     Just v => decodeExpr v
                     _ => Left "Index sem array"
        idx <- case lookupKey "index" fields of
                 Just v => decodeExpr v
                 _ => Left "Index sem index"
        Right $ EIndex arr idx
      Just (JString "LambdaExpr") => do
        ps <- case lookupKey "params" fields of
                Just (JArray xs) => traverse decodeParam xs
                _ => Right []
        b <- case lookupKey "body" fields of
               Just v => decodeExpr v
               _ => Left "Lambda sem corpo"
        Right $ ELambda ps b
      Just (JString "RangeExpr") => do
        s <- case lookupKey "start" fields of
               Just v => decodeExpr v
               _ => Left "Range sem start"
        e <- case lookupKey "end" fields of
               Just v => decodeExpr v
               _ => Left "Range sem end"
        Right $ ERange s e
      Just (JString "PipelineExpr") => do
        stages <- case lookupKey "stages" fields of
                    Just (JArray xs) => Right xs
                    _ => Left "Pipeline sem stages"
        case stages of
          [] => Left "Pipeline vazio"
          (first :: rest) => do
            input <- decodeExpr first
            transforms <- traverse decodeTransform rest
            Right $ EPipeline input transforms
      Just (JString "LoadExpr") => do
        p <- case lookupKey "path" fields of
               Just (JString s) => Right s
               _ => Left "Load sem path"
        Right $ ELoad p
      Just (JString "SaveExpr") => do
        d <- case lookupKey "data" fields of
               Just v => decodeExpr v
               _ => Left "Save sem data"
        p <- case lookupKey "path" fields of
               Just (JString s) => Right s
               _ => Left "Save sem path"
        Right $ ESave d p
      _ => Left "Expressão desconhecida"
  decodeExpr _ = Left "Expressão inválida"

  decodeTransform : JSON -> Either String Transform
  decodeTransform (JObject fields) =
    case lookupKey "type" fields of
      Just (JString "FilterTransform") => do
        pred <- case lookupKey "predicate" fields of
                  Just (JObject pfields) => do
                    ps <- case lookupKey "params" pfields of
                            Just (JArray xs) => traverse decodeParam xs
                            _ => Right []
                    b <- case lookupKey "body" pfields of
                           Just v => decodeExpr v
                           _ => Left "Predicate sem corpo"
                    case ps of
                      (p :: _) => Right (p, b)
                      _ => Left "Predicate sem parâmetro"
                  _ => Left "Predicate inválido"
        let (p,b) = pred
        Right $ TFilter p b
      Just (JString "MapTransform") => do
        mapper <- case lookupKey "mapper" fields of
                    Just (JObject mfields) => do
                      ps <- case lookupKey "params" mfields of
                              Just (JArray xs) => traverse decodeParam xs
                              _ => Right []
                      b <- case lookupKey "body" mfields of
                             Just v => decodeExpr v
                             _ => Left "Mapper sem corpo"
                      case ps of
                        (p :: _) => Right (p, b)
                        _ => Left "Mapper sem parâmetro"
                    _ => Left "Mapper inválido"
        let (p,b) = mapper
        Right $ TMap p b
      Just (JString "ReduceTransform") => do
        initExpr <- case lookupKey "initial" fields of
                      Just v => decodeExpr v
                      _ => Left "Reduce sem initial"
        reducer <- case lookupKey "reducer" fields of
                     Just (JObject rfields) => do
                       ps <- case lookupKey "params" rfields of
                               Just (JArray xs) => traverse decodeParam xs
                               _ => Right []
                       b <- case lookupKey "body" rfields of
                              Just v => decodeExpr v
                              _ => Left "Reducer sem corpo"
                       case ps of
                         (p1 :: p2 :: _) => Right (p1, p2, b)
                         _ => Left "Reducer requer dois parâmetros"
                     _ => Left "Reducer inválido"
        let (pAcc, pVal, body) = reducer
        Right $ TReduce initExpr pAcc pVal body
      Just (JString "CallExpr") => do
        cal <- case lookupKey "callee" fields of
                 Just (JObject cfields) =>
                   case lookupKey "name" cfields of
                     Just (JString s) => Right s
                     _ => Left "Call sem callee name"
                 _ => Left "Call sem callee"
        args <- case lookupKey "args" fields of
                  Just (JArray xs) => traverse decodeExpr xs
                  _ => Right []
        Right $ TCall cal args
      Just (JString "SelectTransform") => do
        cols <- case lookupKey "columns" fields of
                  Just (JArray xs) =>
                    traverse (\c => case c of
                                      JString s => Right s
                                      _ => Left "Coluna select inválida") xs
                  _ => Right []
        Right $ TSelect cols
      Just (JString "GroupByTransform") => do
        cols <- case lookupKey "columns" fields of
                  Just (JArray xs) =>
                    traverse (\c => case c of
                                      JString s => Right s
                                      _ => Left "Coluna groupBy inválida") xs
                  _ => Right []
        Right $ TGroupBy cols
      _ => Left "Transform desconhecido"
  decodeTransform _ = Left "Transform inválido"

mutual
  decodeBlock : JSON -> Either String (List StmtNode)
  decodeBlock (JObject fields) =
    case lookupKey "statements" fields of
      Just (JArray xs) => traverse decodeStmt xs
      _ => Right []
  decodeBlock _ = Right []

  decodeStmt : JSON -> Either String StmtNode
  decodeStmt (JObject fields) =
    case lookupKey "type" fields of
      Just (JString "LetDecl") => do
        n <- case lookupKey "name" fields of
               Just v => expectString "let name" v
               _ => Left "Let sem nome"
        t <- case lookupKey "typeAnnotation" fields of
               Just v => decodeType v
               _ => Right TUnknown
        initVal <- case lookupKey "initializer" fields of
                     Just v => decodeExpr v
                     _ => Left "Let sem inicializador"
        Right $ SLet n t initVal
      Just (JString "AssignExpr") => do
        tgt <- case lookupKey "target" fields of
                 Just (JObject tf) =>
                   case lookupKey "name" tf of
                     Just (JString s) => Right s
                     _ => Left "Assign target inválido"
                 _ => Left "Assign sem target"
        v <- case lookupKey "value" fields of
               Just val => decodeExpr val
               _ => Left "Assign sem value"
        Right $ SAssign tgt v
      Just (JString "ExprStmt") => do
        e <- case lookupKey "expression" fields of
               Just v => decodeExpr v
               _ => Left "ExprStmt sem expression"
        Right $ SExpr e
      Just (JString "ReturnStmt") => do
        case lookupKey "value" fields of
          Just JNull => Right $ SReturn Nothing
          Just v => do e <- decodeExpr v; Right $ SReturn (Just e)
          Nothing => Right $ SReturn Nothing
      Just (JString "IfStmt") => do
        cond <- case lookupKey "condition" fields of
                  Just v => decodeExpr v
                  _ => Left "If sem cond"
        let thenVal = case lookupKey "thenBranch" fields of
                        Just v => Just v
                        Nothing => lookupKey "then" fields
        let elseVal = case lookupKey "elseBranch" fields of
                        Just v => Just v
                        Nothing => lookupKey "else" fields
        tbr <- case thenVal of
                 Just (JObject b) =>
                   case lookupKey "type" b of
                     Just (JString "Block") => decodeBlock (JObject b)
                     _ => do s <- decodeStmt (JObject b); Right [s]
                 Just JNull => Right []
                 Nothing => Right []
                 _ => Left "If then inválido"
        ebr <- case elseVal of
                 Just (JObject b) =>
                   case lookupKey "type" b of
                     Just (JString "Block") => decodeBlock (JObject b)
                     _ => do s <- decodeStmt (JObject b); Right [s]
                 Just JNull => Right []
                 Nothing => Right []
                 _ => Left "If else inválido"
        Right $ SIf cond tbr ebr
      Just (JString "ForStmt") => do
        it <- case lookupKey "iterator" fields of
                Just (JString s) => Right s
                _ => Left "For sem iterator"
        iter <- case lookupKey "iterable" fields of
                  Just v => decodeExpr v
                  _ => Left "For sem iterable"
        body <- case lookupKey "body" fields of
                  Just (JObject b) => decodeBlock (JObject b)
                  _ => Right []
        Right $ SFor it iter body
      Just (JString "PrintStmt") => do
        args <- case lookupKey "args" fields of
                  Just (JArray xs) => traverse decodeExpr xs
                  _ => Right []
        Right $ SExpr (ECall "print" args)
      _ => Left "Stmt desconhecido"
  decodeStmt _ = Left "Stmt inválido"

-- Type checking

typeCheckExpr : Env -> FunEnv -> DataEnv -> ExprNode -> Either String TypeNode
typeCheckExpr env fenv denv expr =
  case expr of
    ELit t _ => Right t
    EVar n =>
      case lookupEnv n env of
        Just t => Right t
        Nothing => Left $ "Variável não declarada: " ++ n
    EBin op l r => do
      tl <- typeCheckExpr env fenv denv l
      tr <- typeCheckExpr env fenv denv r
      case op of
        BAdd => arithType "Soma" tl tr
        BSub => arithType "Subtração" tl tr
        BMul => arithType "Multiplicação" tl tr
        BDiv => arithType "Divisão" tl tr
        BMod => if compatType tl TInt && compatType tr TInt then Right TInt else Left "Módulo requer Int"
        BEq => if compatType tl tr then Right TBool else Left "== requer tipos compatíveis"
        BNeq => if compatType tl tr then Right TBool else Left "!= requer tipos compatíveis"
        BLt => if compatType tl TInt || compatType tl TFloat then Right TBool else Left "< requer Int/Float"
        BLte => if compatType tl TInt || compatType tl TFloat then Right TBool else Left "<= requer Int/Float"
        BGt => if compatType tl TInt || compatType tl TFloat then Right TBool else Left "> requer Int/Float"
        BGte => if compatType tl TInt || compatType tl TFloat then Right TBool else Left ">= requer Int/Float"
        BAnd => if eqType tl TBool && eqType tr TBool then Right TBool else Left "&& requer Bool"
        BOr => if eqType tl TBool && eqType tr TBool then Right TBool else Left "|| requer Bool"
    EUn op e => do
      t <- typeCheckExpr env fenv denv e
      case op of
        UNeg => if eqType t TInt then Right TInt else Left "Negação unária requer Int"
        UNot => if eqType t TBool then Right TBool else Left "! requer Bool"
    ECall name args => do
      targs <- traverse (typeCheckExpr env fenv denv) args
      case name of
        "print" => Right TVoid
        "count" =>
          case targs of
            [TArray _] => Right TInt
            [TDataFrame] => Right TInt
            _ => Left "count requer um array"
        "sum" =>
          case targs of
            [TArray TInt] => Right TInt
            [TArray TFloat] => Right TFloat
            [TArray _] => Right TUnknown
            [TUnknown] => Right TUnknown
            _ => Left "sum requer array de Int ou Float"
        "mean" =>
          case targs of
            [TArray TInt] => Right TFloat
            [TArray TFloat] => Right TFloat
            [TArray _] => Right TFloat
            [TUnknown] => Right TFloat
            _ => Left "mean requer array numérico"
        "min" =>
          case targs of
            [TArray TInt] => Right TInt
            [TArray TFloat] => Right TFloat
            [TArray _] => Right TUnknown
            [TUnknown] => Right TUnknown
            _ => Left "min requer array numérico"
        "max" =>
          case targs of
            [TArray TInt] => Right TInt
            [TArray TFloat] => Right TFloat
            [TArray _] => Right TUnknown
            [TUnknown] => Right TUnknown
            _ => Left "max requer array numérico"
        _ =>
          case lookupData name denv of
            Just fields =>
              let ftypes = map (\f => f.ftype) fields in
              if length ftypes == length targs && all (\(a,b) => compatType a b) (zip ftypes targs)
                 then Right (TCustom name)
                 else Left $ "Chamada de construtor " ++ name ++ " com tipos inválidos"
            Nothing =>
              case lookupFun name fenv of
                Just (params, ret) =>
                  if length params == length targs && all (\(a,b) => compatType a b) (zip params targs)
                     then Right ret
                     else Left $ "Chamada de função " ++ name ++ " com tipos incompatíveis"
                Nothing => Left $ "Função não encontrada: " ++ name
    EArray elems => do
      ts <- traverse (typeCheckExpr env fenv denv) elems
      case ts of
        [] => Right (TArray TUnknown)
        (t :: rest) =>
          if all (\x => compatType x t && compatType t x) rest
             then Right (TArray t)
             else Left "Array com elementos de tipos diferentes"
    EAssign n e => do
      te <- typeCheckExpr env fenv denv e
      case lookupEnv n env of
        Just tv => if compatType tv te then Right te else Left $ "Assign expr tipo incompatível em " ++ n
        Nothing => Left $ "Variável não declarada em assign expr: " ++ n
    EIndex arr idx => do
      ta <- typeCheckExpr env fenv denv arr
      ti <- typeCheckExpr env fenv denv idx
      case ta of
        TArray inner =>
          if eqType ti TInt then Right inner else Left "Index requer índice Int"
        TUnknown => Right TUnknown
        TDataFrame => Right TUnknown
        _ => Left "Index aplicado a não-array"
    EMember obj prop => do
      to <- typeCheckExpr env fenv denv obj
      case to of
        TCustom name =>
          case lookupData name denv of
            Just fields =>
              case [ f | f <- fields, f.fname == prop ] of
                (f :: _) => Right f.ftype
                _ => Left $ "Campo " ++ prop ++ " não encontrado em " ++ name
            Nothing =>
              if name == "Row" then Right TUnknown
              else Left $ "Tipo desconhecido para acesso a campo: " ++ name
        _ => Left "Acesso a campo em tipo não-suportado"
    ELambda ps body => do
      let paramEnv = map (\p => (p.pname, p.ptype)) ps
      tb <- typeCheckExpr (paramEnv ++ env) fenv denv body
      Right TUnknown -- lambdas só são verificadas em contexto de transform
    EPipeline input transforms => do
      tin <- typeCheckExpr env fenv denv input
      applyTransforms tin transforms
    ERange s e => do
      ts <- typeCheckExpr env fenv denv s
      te <- typeCheckExpr env fenv denv e
      if eqType ts TInt && eqType te TInt then Right (TArray TInt) else Left "Range requer limites Int"
    ELoad _ => Right TDataFrame
    ESave d _ => do
      td <- typeCheckExpr env fenv denv d
      case td of
        TDataFrame => Right TVoid
        TArray _ => Right TVoid
        _ => Left "save requer DataFrame ou array"
  where
    arithType : String -> TypeNode -> TypeNode -> Either String TypeNode
    arithType _ TInt TInt = Right TInt
    arithType _ TFloat TFloat = Right TFloat
    arithType _ TInt TFloat = Right TFloat
    arithType _ TFloat TInt = Right TFloat
    arithType _ TUnknown TInt = Right TInt
    arithType _ TInt TUnknown = Right TInt
    arithType _ TUnknown TFloat = Right TFloat
    arithType _ TFloat TUnknown = Right TFloat
    arithType _ TUnknown TUnknown = Right TUnknown
    arithType msg _ _ = Left (msg ++ " requer Int ou Float")
    applyTransforms : TypeNode -> List Transform -> Either String TypeNode
    applyTransforms t [] = Right t
    applyTransforms t (tr :: rest) =
      let base = if eqType t TUnknown then TArray TUnknown else t in
      case tr of
        TFilter p pred =>
          case base of
            TArray elemT =>
              if compatType p.ptype elemT && compatType elemT p.ptype then do
                tp <- typeCheckExpr ((p.pname, p.ptype) :: env) fenv denv pred
                if eqType tp TBool then applyTransforms (TArray elemT) rest else Left "Predicate de filter deve ser Bool"
              else Left "Parâmetro de filter não compatível com elemento do array"
            TDataFrame =>
              if compatType p.ptype (TCustom "Row") then do
                tp <- typeCheckExpr ((p.pname, p.ptype) :: env) fenv denv pred
                if eqType tp TBool then applyTransforms TDataFrame rest else Left "Predicate de filter deve ser Bool"
              else Left "Parâmetro de filter não compatível com Row"
            _ => Left "filter requer array ou DataFrame"
        TMap p mapper =>
          case base of
            TArray elemT =>
              if compatType p.ptype elemT && compatType elemT p.ptype then do
                tm <- typeCheckExpr ((p.pname, p.ptype) :: env) fenv denv mapper
                applyTransforms (TArray tm) rest
              else Left "Parâmetro de map não compatível com elemento"
            TDataFrame =>
              if compatType p.ptype (TCustom "Row") then do
                tm <- typeCheckExpr ((p.pname, p.ptype) :: env) fenv denv mapper
                applyTransforms (TArray tm) rest
              else Left "Parâmetro de map não compatível com Row"
            _ => Left "map requer array ou DataFrame"
        TReduce init pAcc pVal reducer =>
          case base of
            TArray elemT =>
              if not (compatType pVal.ptype elemT) then Left "Parâmetro val do reduce incompatível"
              else do
                tInit <- typeCheckExpr env fenv denv init
                if not (compatType pAcc.ptype tInit) then Left "init do reduce não bate com acumulador" else do
                  tRed <- typeCheckExpr ((pAcc.pname, pAcc.ptype) :: (pVal.pname, pVal.ptype) :: env) fenv denv reducer
                  if compatType tRed tInit then applyTransforms tInit rest else Left "Reducer deve retornar tipo do acumulador"
            TDataFrame => do
              let elemT = TCustom "Row"
              if not (compatType pVal.ptype elemT) then Left "Parâmetro val do reduce incompatível" else do
                tInit <- typeCheckExpr env fenv denv init
                if not (compatType pAcc.ptype tInit) then Left "init do reduce não bate com acumulador" else do
                  tRed <- typeCheckExpr ((pAcc.pname, pAcc.ptype) :: (pVal.pname, pVal.ptype) :: env) fenv denv reducer
                  if compatType tRed tInit then applyTransforms tInit rest else Left "Reducer deve retornar tipo do acumulador"
            _ => Left "reduce requer array ou DataFrame"
        TCall name args => do
          tArgs <- traverse (typeCheckExpr env fenv denv) args
          case lookupFun name fenv of
            Just (params, ret) =>
              let argTypes = t :: tArgs in
              if length params == length argTypes && all (\(a,b) => compatType a b) (zip params argTypes)
                then applyTransforms ret rest
                else Left $ "Chamada de função " ++ name ++ " no pipeline com tipos incompatíveis"
            Nothing => Left $ "Função não encontrada em pipeline: " ++ name
        TSelect _ =>
          case base of
            TDataFrame => applyTransforms TDataFrame rest
            TArray (TCustom _) => applyTransforms TDataFrame rest
            _ => Left "select requer DataFrame"
        TGroupBy _ =>
          case base of
            TDataFrame => applyTransforms TDataFrame rest
            TArray (TCustom _) => applyTransforms TDataFrame rest
            _ => Left "groupBy requer DataFrame"

mutual
  typeCheckStmt : Env -> FunEnv -> DataEnv -> TypeNode -> StmtNode -> Either String (Env, Bool)
  typeCheckStmt env fenv denv retTy stmt =
    case stmt of
      SLet n t e => do
        te <- typeCheckExpr env fenv denv e
        if compatType te t then Right ((n, if eqType t TUnknown then te else t) :: env, False) else
          Left $ "Let " ++ n ++ " tipo declarado não bate com inicializador"
      SAssign n e => do
        te <- typeCheckExpr env fenv denv e
        case lookupEnv n env of
          Just tv => if compatType tv te then Right (env, False) else Left $ "Assign tipo incompatível em " ++ n
          Nothing => Left $ "Variável não declarada em assign: " ++ n
      SExpr e => do
        _ <- typeCheckExpr env fenv denv e
        Right (env, False)
      SReturn Nothing =>
        if compatType retTy TVoid then Right (env, True) else Left "return; em função com tipo de retorno"
      SReturn (Just e) => do
        te <- typeCheckExpr env fenv denv e
        if compatType te retTy then Right (env, True) else
          Left "Tipo de retorno incompatível"
      SIf cond tbr ebr => do
        tc <- typeCheckExpr env fenv denv cond
        if not (eqType tc TBool) then Left "If condicional não-bool" else do
          (_, rt) <- typeCheckBlock env fenv denv retTy tbr
          (_, rf) <- typeCheckBlock env fenv denv retTy ebr
          Right (env, rt && rf)
      SFor it iterable body => do
        titer <- typeCheckExpr env fenv denv iterable
        elemT <- case titer of
                   TArray et => Right et
                   TDataFrame => Right (TCustom "Row")
                   TUnknown => Right TUnknown
                   _ => Left "for requer iterable array"
        (env', rbody) <- typeCheckBlock ((it, elemT) :: env) fenv denv retTy body
        Right (env', rbody)

  typeCheckBlock : Env -> FunEnv -> DataEnv -> TypeNode -> List StmtNode -> Either String (Env, Bool)
  typeCheckBlock env fenv denv retTy [] = Right (env, False)
  typeCheckBlock env fenv denv retTy (s :: ss) = do
    (env', ret) <- typeCheckStmt env fenv denv retTy s
    if ret then Right (env', True) else typeCheckBlock env' fenv denv retTy ss



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
  fnDupErrors ++ paramDupErrors ++ fieldDupErrors ++ exportErrors ++ emptyNameErrors ++ typeErrors
  where
    fnDecls : List (String, List Param, Maybe TypeNode, JSON)
    fnDecls = mapMaybe' (\d => case d of
                                FnDecl n ps rt body => Just (n, ps, rt, body)
                                _ => Nothing) decls
    fnNames = map (\(n, _, _, _) => n) fnDecls
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
    paramDupErrors = concat $ map (\(n, ps, _, _) =>
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
    dataEnv : DataEnv
    dataEnv = dataDecls
    funEnv : FunEnv
    funEnv = builtinFuns ++ map (\(n, ps, rt, _) => (n, (map (\p => p.ptype) ps, maybe TVoid id rt))) fnDecls
    globalLets : List (String, TypeNode, JSON)
    globalLets = mapMaybe' (\d => case d of
                                    LetDecl n t initJson => Just (n, t, initJson)
                                    _ => Nothing) decls
    checkGlobals : Either String (Env, List String)
    checkGlobals = go [] [] globalLets
      where
        go : Env -> List String -> List (String, TypeNode, JSON) -> Either String (Env, List String)
        go env acc [] = Right (env, acc)
        go env acc ((n,t,j) :: rest) = do
          e <- decodeExpr j
          te <- typeCheckExpr env funEnv dataEnv e
          let actual = if eqType t TUnknown then te else t
          let unkErr = if eqType te TUnknown then ["Global " ++ n ++ " permanece com tipo desconhecido"] else []
          if compatType te t then go ((n, actual) :: env) (acc ++ unkErr) rest else
            Left ("Global " ++ n ++ " tipo declarado não bate com inicializador")
    fnUnknownErrors : List String
    fnUnknownErrors = concat $ map (\(n, ps, rt, _) =>
      let paramErrs = concat $ map (\p => if eqType p.ptype TUnknown then ["Parâmetro sem tipo em função " ++ n ++ ": " ++ p.pname] else []) ps
          retErrs = case rt of
                      Just t => if eqType t TUnknown then ["Tipo de retorno indefinido em função " ++ n] else []
                      Nothing => []
      in paramErrs ++ retErrs) fnDecls
    dataUnknownErrors : List String
    dataUnknownErrors = concat $ map (\(n, fs) =>
      concat $ map (\f => if eqType f.ftype TUnknown then ["Campo " ++ f.fname ++ " em data " ++ n ++ " sem tipo definido"] else []) fs) dataDecls
    checkFunctions : Env -> List String
    checkFunctions genv = concat $ map (checkOne genv) fnDecls
      where
        checkOne : Env -> (String, List Param, Maybe TypeNode, JSON) -> List String
        checkOne genv (n, ps, rt, bodyJson) =
          case decodeBlock bodyJson of
            Left err => ["Erro ao decodificar corpo de " ++ n ++ ": " ++ err]
            Right stmts =>
              let env0 = map (\p => (p.pname, p.ptype)) ps ++ genv
                  retTy = maybe TVoid id rt in
              case typeCheckBlock env0 funEnv dataEnv retTy stmts of
                Left err => ["Erro de tipo em função " ++ n ++ ": " ++ err]
                Right (_, returnsAll) =>
                  if not (eqType retTy TVoid) && not returnsAll then ["Função " ++ n ++ " pode não retornar em todos os caminhos"] else []
    typeErrors : List String
    typeErrors = case checkGlobals of
                   Left err => [err]
                   Right (genv, gUnknowns) => gUnknowns ++ fnUnknownErrors ++ dataUnknownErrors ++ checkFunctions genv


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
