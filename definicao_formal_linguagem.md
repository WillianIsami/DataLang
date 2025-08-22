# ENTREGA DA SEMANA 2: DEFINIÇÃO FORMAL DOS ELEMENTOS BÁSICOS DA LINGUAGEM

---

### 1. ESPECIFICAÇÃO DO ALFABETO DA LINGUAGEM

O alfabeto da linguagem de programação para análise e manipulação de dados foi definido com base no conjunto **ASCII estendido**, priorizando simplicidade, legibilidade e compatibilidade com ferramentas existentes. O alfabeto Σ é composto pelos seguintes subconjuntos:

#### Σ = Letras ∪ Dígitos ∪ Operadores ∪ Delimitadores ∪ Símbolos Especiais

- **Letras** = {a, b, ..., z, A, B, ..., Z}
- **Dígitos** = {0, 1, 2, ..., 9}
- **Operadores** = {+, -, \*, /, %, =, <, >, !, &, |, ^, ~}
- **Delimitadores** = {(, ), [, ], {, }, ;, ,, ., :, @}
- **Símbolos Especiais** = {\_, ", ', `, #, $, ?, \\}

> **Justificativa**: Optou-se por excluir caracteres raros ou ambíguos (como `ç`, `á`, `é`) para evitar problemas de parsing e garantir portabilidade. O uso de `@` e `$` foi reservado para futuras expansões (ex.: decoradores, variáveis especiais).

---

### 2. DEFINIÇÃO FORMAL DOS TOKENS

Cada categoria de token foi especificada usando operações formais sobre o alfabeto Σ.

#### a) Identificadores

```
letra = {a, b, ..., z, A, B, ..., Z}
dígito = {0, 1, ..., 9}
underscore = {_}
primeiro_char = letra ∪ underscore
chars_seguintes = primeiro_char ∪ dígito
identificador = primeiro_char · (chars_seguintes)*
```

> Exemplos: `x`, `data_frame`, `_temp`, `var1`

#### b) Literais Numéricos

```
inteiro = dígito+
decimal = dígito+ · {.} · dígito+
notação_científica = (dígito⁺(.dígito⁺)?)(e|E)(+|-)?dígito⁺
```

> Exemplos: `42`, `3.14`, `2.5e-10`

#### c) Strings Literais

```
aspas = {"}
normal = Σ - {", \, nova_linha}
escape = {\", \\, \n, \t, \r}
conteúdo = (normal ∪ escape)*
string = aspas conteúdo aspas
```

> Exemplo: `"Hello, World!"`

#### d) Palavras-Chave

```rust
palavras_chave = {let, fn, data, filter, map, reduce, import, export, if, else, for, in}
```

> Case-sensitive: `if` != `IF`

#### e) Operadores e Delimitadores

```rust
operadores = {+, -, *, /, %, =, ==, !=, <, >, <=, >=, &&, ||}
delimitadores = {(, ), [, ], {, }, ;, ,, ., :, @}
```

---

### 3. EXEMPLOS DE PROGRAMAS VÁLIDOS

#### Exemplo 1: Definição de Função

```rust
fn square(x) {
    return x * x
}
```

#### Exemplo 2: Manipulação de DataFrame

```rust
let df = data.load_csv("file.csv")
let filtered = df.filter(|row| row["age"] > 30)
let result = filtered.select(["name", "age"])
```

#### Exemplo 3: Expressão com Notação Científica

```rust
let velocity = 2.5e8
let time = 1.2e-3
```
