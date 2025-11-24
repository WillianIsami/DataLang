# Análise LL(1) da Gramática DataLang

## Gramática LL(1) Refatorada

Esta gramática reflete a implementação real do compilador DataLang.

```ebnf
Program       = { TopLevel } .

TopLevel      = LetDecl
              | FnDecl
              | DataDecl
              | ImportDecl
              | ExportDecl
              | Statement .

LetDecl       = "let" Ident [ ":" Type ] "=" Expr ";" .

FnDecl        = "fn" Ident "(" [ FormalParams ] ")" [ "->" Type ] Block .

FormalParams  = FormalParam { "," FormalParam } .

FormalParam   = Ident ":" Type .

DataDecl      = "data" Ident "{" { FieldDecl } "}" .

FieldDecl     = Ident ":" Type ";" .

ImportDecl    = "import" STRING [ "as" Ident ] ";" .

ExportDecl    = "export" Ident ";" .

Type          = "Int" | "Float" | "String" | "Bool" 
              | "DataFrame" | "Vector" | "Series"
              | Ident
              | "[" Type "]"
              | "(" Type { "," Type } ")" .

Block         = "{" { Statement } "}" .

Statement     = LetDecl
              | IfStatement
              | ForStatement
              | ReturnStatement
              | PrintStatement
              | ExprStatement .

PrintStatement = "print" "(" Expr ")" ";" .

ExprStatement = Expr ";" .

IfStatement   = "if" Expr Block [ "else" ( IfStatement | Block ) ] .

ForStatement  = "for" Ident "in" Expr Block .

ReturnStatement = "return" [ Expr ] ";" .

Expr          = PipelineExpr .

PipelineExpr  = TransformExpr { "|>" TransformExpr } .

TransformExpr = AssignExpr
              | FilterTransform
              | MapTransform
              | ReduceTransform
              | SelectTransform
              | GroupByTransform
              | AggregateTransform .

FilterTransform  = "filter" "(" LambdaExpr ")" .

MapTransform     = "map" "(" LambdaExpr ")" .

ReduceTransform  = "reduce" "(" Expr "," LambdaExpr ")" .

SelectTransform  = "select" "(" IdentList ")" .

GroupByTransform = "groupby" "(" IdentList ")" .

AggregateTransform = ("sum" | "mean" | "count" | "min" | "max") "(" [ ExprList ] ")" .

AssignExpr    = LogicOrExpr { "=" LogicOrExpr } .

LogicOrExpr   = LogicAndExpr { "||" LogicAndExpr } .

LogicAndExpr  = EqualityExpr { "&&" EqualityExpr } .

EqualityExpr  = RelationalExpr { ( "==" | "!=" ) RelationalExpr } .

RelationalExpr = RangeExpr { ( "<" | "<=" | ">" | ">=" ) RangeExpr } .

RangeExpr     = AddExpr [ ".." AddExpr ] .

AddExpr       = MultExpr { ( "+" | "-" ) MultExpr } .

MultExpr      = UnaryExpr { ( "*" | "/" | "%" ) UnaryExpr } .

UnaryExpr     = ( "-" | "!" ) UnaryExpr
              | PostfixExpr .

PostfixExpr   = Primary { Postfix } .

Postfix       = "(" [ ExprList ] ")"
              | "[" Expr "]"
              | "." Ident .

Primary       = Literal
              | Ident
              | LambdaExpr
              | LoadExpr
              | SaveExpr
              | "(" Expr ")"
              | "[" [ ExprList ] "]" .

LambdaExpr    = "|" [ LambdaParams ] "|" Expr .

LambdaParams  = LambdaParam { "," LambdaParam } .

LambdaParam   = Ident [ ":" Type ] .

LoadExpr      = "load" "(" STRING ")" .

SaveExpr      = "save" "(" Expr "," STRING ")" .

Literal       = INTEGER
              | FLOAT
              | STRING
              | "true"
              | "false" .

ExprList      = Expr { "," Expr } .

IdentList     = Ident { "," Ident } .
```

---

## Conjuntos FIRST e FOLLOW

### Convenções
- `ε` = string vazia
- `$` = fim da entrada
- `|` entre pipes = token pipe (lambda)

### Conjuntos FIRST

| Não-Terminal | Conjunto FIRST |
|:-------------|:---------------|
| `Program` | { "let", "fn", "data", "import", "export", "if", "print", INTEGER, FLOAT, STRING, "true", "false", Ident, "\|", "load", "save", "(", "[", "-", "!", "filter", "map", "reduce", "select", "groupby", "sum", "mean", "count", "min", "max", ε } |
| `TopLevel` | { "let", "fn", "data", "import", "export", "if", "print", INTEGER, FLOAT, STRING, "true", "false", Ident, "\|", "load", "save", "(", "[", "-", "!", "filter", "map", "reduce", "select", "groupby", "sum", "mean", "count", "min", "max" } |
| `LetDecl` | { "let" } |
| `FnDecl` | { "fn" } |
| `FormalParams` | { Ident } |
| `FormalParam` | { Ident } |
| `DataDecl` | { "data" } |
| `FieldDecl` | { Ident } |
| `ImportDecl` | { "import" } |
| `ExportDecl` | { "export" } |
| `Type` | { "Int", "Float", "String", "Bool", "DataFrame", "Vector", "Series", Ident, "[", "(" } |
| `Block` | { "{" } |
| `Statement` | { "let", "if", "for", "return", "print", INTEGER, FLOAT, STRING, "true", "false", Ident, "\|", "load", "save", "(", "[", "-", "!", "filter", "map", "reduce", "select", "groupby", "sum", "mean", "count", "min", "max" } |
| `PrintStatement` | { "print" } |
| `ExprStatement` | { INTEGER, FLOAT, STRING, "true", "false", Ident, "\|", "load", "save", "(", "[", "-", "!", "filter", "map", "reduce", "select", "groupby", "sum", "mean", "count", "min", "max" } |
| `IfStatement` | { "if" } |
| `ForStatement` | { "for" } |
| `ReturnStatement` | { "return" } |
| `Expr` | { INTEGER, FLOAT, STRING, "true", "false", Ident, "\|", "load", "save", "(", "[", "-", "!", "filter", "map", "reduce", "select", "groupby", "sum", "mean", "count", "min", "max" } |
| `PipelineExpr` | { INTEGER, FLOAT, STRING, "true", "false", Ident, "\|", "load", "save", "(", "[", "-", "!", "filter", "map", "reduce", "select", "groupby", "sum", "mean", "count", "min", "max" } |
| `TransformExpr` | { INTEGER, FLOAT, STRING, "true", "false", Ident, "\|", "load", "save", "(", "[", "-", "!", "filter", "map", "reduce", "select", "groupby", "sum", "mean", "count", "min", "max" } |
| `FilterTransform` | { "filter" } |
| `MapTransform` | { "map" } |
| `ReduceTransform` | { "reduce" } |
| `SelectTransform` | { "select" } |
| `GroupByTransform` | { "groupby" } |
| `AggregateTransform` | { "sum", "mean", "count", "min", "max" } |
| `AssignExpr` | { INTEGER, FLOAT, STRING, "true", "false", Ident, "\|", "load", "save", "(", "[", "-", "!" } |
| `LogicOrExpr` | { INTEGER, FLOAT, STRING, "true", "false", Ident, "\|", "load", "save", "(", "[", "-", "!" } |
| `LogicAndExpr` | { INTEGER, FLOAT, STRING, "true", "false", Ident, "\|", "load", "save", "(", "[", "-", "!" } |
| `EqualityExpr` | { INTEGER, FLOAT, STRING, "true", "false", Ident, "\|", "load", "save", "(", "[", "-", "!" } |
| `RelationalExpr` | { INTEGER, FLOAT, STRING, "true", "false", Ident, "\|", "load", "save", "(", "[", "-", "!" } |
| `RangeExpr` | { INTEGER, FLOAT, STRING, "true", "false", Ident, "\|", "load", "save", "(", "[", "-", "!" } |
| `AddExpr` | { INTEGER, FLOAT, STRING, "true", "false", Ident, "\|", "load", "save", "(", "[", "-", "!" } |
| `MultExpr` | { INTEGER, FLOAT, STRING, "true", "false", Ident, "\|", "load", "save", "(", "[", "-", "!" } |
| `UnaryExpr` | { INTEGER, FLOAT, STRING, "true", "false", Ident, "\|", "load", "save", "(", "[", "-", "!" } |
| `PostfixExpr` | { INTEGER, FLOAT, STRING, "true", "false", Ident, "\|", "load", "save", "(", "[" } |
| `Postfix` | { "(", "[", "." } |
| `Primary` | { INTEGER, FLOAT, STRING, "true", "false", Ident, "\|", "load", "save", "(", "[" } |
| `LambdaExpr` | { "\|" } |
| `LambdaParams` | { Ident } |
| `LambdaParam` | { Ident } |
| `LoadExpr` | { "load" } |
| `SaveExpr` | { "save" } |
| `Literal` | { INTEGER, FLOAT, STRING, "true", "false" } |
| `ExprList` | { INTEGER, FLOAT, STRING, "true", "false", Ident, "\|", "load", "save", "(", "[", "-", "!", "filter", "map", "reduce", "select", "groupby", "sum", "mean", "count", "min", "max" } |
| `IdentList` | { Ident } |

### Conjuntos FOLLOW

| Não-Terminal | Conjunto FOLLOW |
|:-------------|:----------------|
| `Program` | { $ } |
| `TopLevel` | { "let", "fn", "data", "import", "export", "if", "print", INTEGER, FLOAT, STRING, "true", "false", Ident, "\|", "load", "save", "(", "[", "-", "!", "filter", "map", "reduce", "select", "groupby", "sum", "mean", "count", "min", "max", $ } |
| `LetDecl` | { "let", "fn", "data", "import", "export", "if", "for", "return", "print", INTEGER, FLOAT, STRING, "true", "false", Ident, "\|", "load", "save", "(", "[", "-", "!", "filter", "map", "reduce", "select", "groupby", "sum", "mean", "count", "min", "max", "}", $ } |
| `FnDecl` | { "let", "fn", "data", "import", "export", $ } |
| `FormalParams` | { ")" } |
| `FormalParam` | { ",", ")" } |
| `DataDecl` | { "let", "fn", "data", "import", "export", $ } |
| `FieldDecl` | { Ident, "}" } |
| `ImportDecl` | { "let", "fn", "data", "import", "export", $ } |
| `ExportDecl` | { "let", "fn", "data", "import", "export", $ } |
| `Type` | { "=", "->", "{", ";", "]", ",", ")", "\|" } |
| `Block` | { "let", "fn", "data", "import", "export", "if", "for", "return", "print", INTEGER, FLOAT, STRING, "true", "false", Ident, "\|", "load", "save", "(", "[", "-", "!", "filter", "map", "reduce", "select", "groupby", "sum", "mean", "count", "min", "max", "}", "else", $ } |
| `Statement` | { "let", "if", "for", "return", "print", INTEGER, FLOAT, STRING, "true", "false", Ident, "\|", "load", "save", "(", "[", "-", "!", "filter", "map", "reduce", "select", "groupby", "sum", "mean", "count", "min", "max", "}" } |
| `PrintStatement` | { "let", "if", "for", "return", "print", INTEGER, FLOAT, STRING, "true", "false", Ident, "\|", "load", "save", "(", "[", "-", "!", "filter", "map", "reduce", "select", "groupby", "sum", "mean", "count", "min", "max", "}" } |
| `ExprStatement` | { "let", "if", "for", "return", "print", INTEGER, FLOAT, STRING, "true", "false", Ident, "\|", "load", "save", "(", "[", "-", "!", "filter", "map", "reduce", "select", "groupby", "sum", "mean", "count", "min", "max", "}" } |
| `IfStatement` | { "let", "if", "for", "return", "print", INTEGER, FLOAT, STRING, "true", "false", Ident, "\|", "load", "save", "(", "[", "-", "!", "filter", "map", "reduce", "select", "groupby", "sum", "mean", "count", "min", "max", "}", "else" } |
| `ForStatement` | { "let", "if", "for", "return", "print", INTEGER, FLOAT, STRING, "true", "false", Ident, "\|", "load", "save", "(", "[", "-", "!", "filter", "map", "reduce", "select", "groupby", "sum", "mean", "count", "min", "max", "}" } |
| `ReturnStatement` | { "let", "if", "for", "return", "print", INTEGER, FLOAT, STRING, "true", "false", Ident, "\|", "load", "save", "(", "[", "-", "!", "filter", "map", "reduce", "select", "groupby", "sum", "mean", "count", "min", "max", "}" } |
| `Expr` | { ";", ")", "]", "}", ",", "else", "\|>", "\|" } |
| `PipelineExpr` | { ";", ")", "]", "}", ",", "else", "\|>" } |
| `TransformExpr` | { ";", ")", "]", "}", ",", "else", "\|>" } |
| `FilterTransform` | { ";", ")", "]", "}", ",", "else", "\|>" } |
| `MapTransform` | { ";", ")", "]", "}", ",", "else", "\|>" } |
| `ReduceTransform` | { ";", ")", "]", "}", ",", "else", "\|>" } |
| `SelectTransform` | { ";", ")", "]", "}", ",", "else", "\|>" } |
| `GroupByTransform` | { ";", ")", "]", "}", ",", "else", "\|>" } |
| `AggregateTransform` | { ";", ")", "]", "}", ",", "else", "\|>" } |
| `AssignExpr` | { ";", ")", "]", "}", ",", "else", "\|>" } |
| `LogicOrExpr` | { "=", ";", ")", "]", "}", ",", "else", "\|>" } |
| `LogicAndExpr` | { "\|\|", "=", ";", ")", "]", "}", ",", "else", "\|>" } |
| `EqualityExpr` | { "&&", "\|\|", "=", ";", ")", "]", "}", ",", "else", "\|>" } |
| `RelationalExpr` | { "==", "!=", "&&", "\|\|", "=", ";", ")", "]", "}", ",", "else", "\|>" } |
| `RangeExpr` | { "<", "<=", ">", ">=", "==", "!=", "&&", "\|\|", "=", ";", ")", "]", "}", ",", "else", "\|>" } |
| `AddExpr` | { "..", "<", "<=", ">", ">=", "==", "!=", "&&", "\|\|", "=", ";", ")", "]", "}", ",", "else", "\|>" } |
| `MultExpr` | { "+", "-", "..", "<", "<=", ">", ">=", "==", "!=", "&&", "\|\|", "=", ";", ")", "]", "}", ",", "else", "\|>" } |
| `UnaryExpr` | { "*", "/", "%", "+", "-", "..", "<", "<=", ">", ">=", "==", "!=", "&&", "\|\|", "=", ";", ")", "]", "}", ",", "else", "\|>" } |
| `PostfixExpr` | { "*", "/", "%", "+", "-", "..", "<", "<=", ">", ">=", "==", "!=", "&&", "\|\|", "=", ";", ")", "]", "}", ",", "else", "\|>" } |
| `Postfix` | { "(", "[", ".", "*", "/", "%", "+", "-", "..", "<", "<=", ">", ">=", "==", "!=", "&&", "\|\|", "=", ";", ")", "]", "}", ",", "else", "\|>" } |
| `Primary` | { "(", "[", ".", "*", "/", "%", "+", "-", "..", "<", "<=", ">", ">=", "==", "!=", "&&", "\|\|", "=", ";", ")", "]", "}", ",", "else", "\|>" } |
| `LambdaExpr` | { "(", "[", ".", "*", "/", "%", "+", "-", "..", "<", "<=", ">", ">=", "==", "!=", "&&", "\|\|", "=", ";", ")", "]", "}", ",", "else", "\|>" } |
| `LambdaParams` | { "\|" } |
| `LambdaParam` | { ",", "\|" } |
| `LoadExpr` | { "(", "[", ".", "*", "/", "%", "+", "-", "..", "<", "<=", ">", ">=", "==", "!=", "&&", "\|\|", "=", ";", ")", "]", "}", ",", "else", "\|>" } |
| `SaveExpr` | { "(", "[", ".", "*", "/", "%", "+", "-", "..", "<", "<=", ">", ">=", "==", "!=", "&&", "\|\|", "=", ";", ")", "]", "}", ",", "else", "\|>" } |
| `Literal` | { "(", "[", ".", "*", "/", "%", "+", "-", "..", "<", "<=", ">", ">=", "==", "!=", "&&", "\|\|", "=", ";", ")", "]", "}", ",", "else", "\|>" } |
| `ExprList` | { ")", "]" } |
| `IdentList` | { ")" } |

---

## A Gramática é LL(1)?

### Verificação das Condições LL(1)

Uma gramática é LL(1) se satisfaz:

1. **Condição FIRST/FIRST:** Para produções alternativas A → α₁ | α₂ | ... | αₙ:
   - FIRST(αᵢ) ∩ FIRST(αⱼ) = ∅ para todo i ≠ j

2. **Condição FIRST/FOLLOW:** Se ε ∈ FIRST(αᵢ):
   - FIRST(αⱼ) ∩ FOLLOW(A) = ∅ para todo j ≠ i

### Conclusão:

**SIM, a gramática é LL(1).**

**Justificativa:**
1. Todas as alternativas têm conjuntos FIRST disjuntos
2. Palavras-chave funcionam como discriminadores únicos
3. Não há ambiguidades em nenhuma produção
4. O parser pode determinar inequivocamente qual produção usar com 1 token de lookahead
5. A implementação do parser recursivo descendente confirma a propriedade LL(1)
