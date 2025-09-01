# ENTREGA DA SEMANA 3: GRAMÁTICA FORMAL DA DATALANG

## 1. DEFINIÇÃO FORMAL DA GRAMÁTICA

### Gramática G = (V, T, P, S)

#### Conjunto de Variáveis (V):
```
V = {
    Programa, BlocoDeclaracoes, Declaracao, DeclaracaoVariavel,
    DeclaracaoFuncao, DeclaracaoDataFrame, BlocoComandos, Comando,
    ComandoCondicional, ComandoLaco, ComandoAtribuicao, ComandoIO,
    Expressao, ExpressaoLogica, ExpressaoRelacional, ExpressaoAritmetica,
    ExpressaoDataFrame, Termo, Fator, ListaParametros, ListaArgumentos,
    Tipo, TransformacaoDados, FiltroDados, AgregacaoDados,
    ProjecaoDados, LambdaExpr, ParametrosLambda
}
```

#### Conjunto de Terminais (T):
```
T = {
    // Palavras-chave
    "let", "fn", "data", "filter", "map", "reduce", "import", "export",
    "if", "else", "for", "in", "return", "load", "save", "select", "groupby",
    "sum", "mean", "count", "min", "max", "as", "true", "false",
    
    // Tipos de dados
    "Int", "Float", "String", "Bool", "DataFrame", "Vector", "Series",
    
    // Operadores
    "+", "-", "*", "/", "%", "=", "==", "!=", "<", ">", "<=", ">=", "&&", "||",
    
    // Delimitadores e símbolos
    "(", ")", "[", "]", "{", "}", ";", ",", ".", ":", "|>", "=>", "|", "..",
    
    // Literais e identificadores
    identificador, literal_inteiro, literal_float, literal_string, literal_bool
}
```

#### Símbolo Inicial: S = Programa

### 2. PRINCIPAIS REGRAS DE PRODUÇÃO

#### Estrutura Básica do Programa:
```bnf
Programa → BlocoDeclaracoes BlocoComandos
BlocoDeclaracoes → Declaracao BlocoDeclaracoes | ε
Declaracao → DeclaracaoVariavel | DeclaracaoFuncao | DeclaracaoDataFrame | ImportDecl
ImportDecl → "import" literal_string ("as" identificador)?
```

#### Declarações de Variáveis e Funções:
```bnf
DeclaracaoVariavel → "let" identificador (":" Tipo)? "=" Expressao
DeclaracaoFuncao → "fn" identificador "(" ListaParametros ")" ("->" Tipo)? BlocoComandos
ListaParametros → (Parametro ("," Parametro)*)?
Parametro → identificador ":" Tipo
DeclaracaoDataFrame → identificador "=" "data" "." "load" "(" literal_string ")"
```

#### Expressões Específicas para Manipulação de Dados:
```bnf
ExpressaoDataFrame → ExpressaoDataFrame ("|>" TransformacaoDados)+
    | identificador
    | "data" "." "load" "(" literal_string ")"
    | ChamadaFuncao
TransformacaoDados → FiltroDados | AgregacaoDados | ProjecaoDados | MapDados
FiltroDados → "filter" "(" LambdaExpr ")"
MapDados → "map" "(" LambdaExpr ")"
AgregacaoDados → "groupby" "(" Expressao ")" "." OperacaoAgregacao "(" ")"
OperacaoAgregacao → "sum" | "mean" | "count" | "min" | "max"
ProjecaoDados → "select" "(" ListaArgumentos ")"
LambdaExpr → "|" ParametrosLambda "|" "=>" Expressao
ParametrosLambda → identificador ("," identificador)*
```

#### Expressões Gerais:
```bnf
Expressao → ExpressaoLogica
ExpressaoLogica → ExpressaoRelacional
    | ExpressaoLogica "&&" ExpressaoRelacional
    | ExpressaoLogica "||" ExpressaoRelacional
    | "!" ExpressaoLogica
ExpressaoRelacional → ExpressaoAritmetica
    | ExpressaoAritmetica OperadorRelacional ExpressaoAritmetica
ExpressaoAritmetica → Termo
    | ExpressaoAritmetica "+" Termo
    | ExpressaoAritmetica "-" Termo
Termo → Fator
    | Termo "*" Fator
    | Termo "/" Fator
    | Termo "%" Fator
Fator → identificador | literal | "(" Expressao ")" | ChamadaFuncao | LambdaExpr
ChamadaFuncao → identificador "(" ListaArgumentos ")"
ListaArgumentos → (Expressao ("," Expressao)*)?
```

#### Comandos de Controle de Fluxo:
```bnf
ComandoCondicional → "if" "(" Expressao ")" BlocoComandos ("else" BlocoComandos)?
ComandoLaco → "for" "(" identificador "in" Expressao ".." Expressao ")" BlocoComandos
Comando → ComandoAtribuicao | ComandoCondicional | ComandoLaco | ComandoIO | Expressao
ComandoAtribuicao → identificador "=" Expressao
ComandoIO → "print" "(" Expressao ")" | "read" "(" literal_string ")"
BlocoComandos → "{" (Comando ";")* "}"
```

#### Tipos de Dados:
```bnf
Tipo → "Int" | "Float" | "String" | "Bool" | "DataFrame" | "Vector"
```

---

## 3. CLASSIFICAÇÃO NA HIERARQUIA DE CHOMSKY

### Classificação: Tipo 2 - Livre de Contexto

**Justificativa:**

- Todas as produções seguem a forma A → α, onde A é um único não-terminal (V) e α ∈ (V ∪ T)*
- A gramática gera estruturas aninhadas e recursivas típicas de linguagens de programação
- Reconhecível por autômatos de pilha
- Não é regular (Tipo 3) devido às estruturas recursivas e aninhamento de blocos
- Não requer dependências contextuais complexas que exigiriam gramática sensível ao contexto (Tipo 1)
- É bem estruturada e decidível, não sendo necessário recorrer a gramáticas irrestritas (Tipo 0)

**Verificação Formal:**

- ❌ **Não é Tipo 3 (Regular)**: A linguagem inclui construções com aninhamento arbitrário (parênteses, blocos if-else), o que viola as restrições das gramáticas regulares.

- ✅ **É Tipo 2 (Livre de Contexto)**: Todas as produções seguem estritamente o formato A → α, onde A é um único não-terminal.

- ❌ **Não precisa ser Tipo 1 (Sensível ao Contexto)**: Verificações contextuais serão tratadas na fase de análise semântica.

- ❌ **Não é Tipo 0 (Irrestrita)**: A gramática é bem comportada e pode ser eficientemente analisada.

---

## 4. RESOLUÇÃO DE AMBIGUIDADES

### Problemas Identificados e Soluções:

#### 1. Precedência de Operadores:
**Problema:** Expressões como `a + b * c` podem ter interpretações ambíguas.

**Solução:** Hierarquia de precedência definida:
1. Parênteses `( )`
2. Operadores unários (`!`, `-`)
3. Multiplicação, divisão, módulo (`*`, `/`, `%`) - associativos à esquerda
4. Adição, subtração (`+`, `-`) - associativos à esquerda  
5. Operadores relacionais - não associativos
6. AND lógico (`&&`) - associativo à esquerda
7. OR lógico (`||`) - associativo à esquerda
8. Atribuição (`=`) - associativo à direita

#### 2. Ambiguidade do "else" pendurado:
**Solução:** O `else` sempre se associa ao `if` mais próximo não pareado.

#### 3. Encadeamento de Transformações de DataFrame:
**Solução:** Associatividade à esquerda para o operador `|>`, garantindo avaliação sequencial.

#### 4. Expressões Lambda:
**Solução:** Contexto de uso desambigua naturalmente, com parênteses obrigatórios quando usadas como argumentos.

---

## 5. EXEMPLOS DE DERIVAÇÃO

### Exemplo 1: Carga e Filtro de DataFrame
```
Programa
⇒ BlocoDeclaracoes BlocoComandos
⇒ DeclaracaoDataFrame BlocoComandos
⇒ df = data . load ( "dados.csv" ) Comando ;
⇒ df = data . load ( "dados.csv" ) resultado = df |> filter ( | x | => x > 10 ) ;
```

### Exemplo 2: Função com Agregação
```
DeclaracaoFuncao
⇒ fn calcular_media ( dados : DataFrame ) -> DataFrame { 
    return dados |> groupby ( categoria ) . mean ( ) ; 
}
```

### Exemplo 3: Expressão com Precedência
```
ExpressaoAritmetica
⇒ a + b * c
// Deriva como: a + (b * c)
```
