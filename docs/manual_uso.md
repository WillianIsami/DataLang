# Manual de Utilização da Linguagem DataLang

## Sintaxe suportada (ver gramática completa em `docs/gramatica_refatorada.md`)
- Declarações: `let x: Int = 5;`
- Funções: `fn f(a: Int) -> Int { ... }`
- Tipos customizados: `data Pessoa { nome: String; idade: Int; }`
- Arrays: `[1, 2, 3]`, `[Pessoa]`
- Controle: `if/else`, `for x in arr { ... }`, `return`
- Expressões: aritmética, lógicas, comparações, intervalos (`1..5`)
- Pipelines: `dados |> filter(...) |> map(...) |> reduce(...)`
- Agregados: `sum`, `mean`, `min`, `max`, `count` (Int e Float)
- DataFrame (simplificado): `load("file.csv")`, `save(df, "out.csv")`, `select(colunas)`, `groupby(colunas)`, `filter` numérico por coluna, `map` para coluna numérica, `reduce` em array numérico de pipeline.

## Limitações atuais vs. gramática
- `import`/`export`, `Vector`/`Series`: não implementados.
- Operações de DataFrame são simplificadas (groupby = distinct, select = projeção, filter numérico simples); não há agregação real por grupos.
- Inferência completa de lambdas não tipadas ainda é parcial (melhor usar anotações).

## Como compilar e executar um programa
1. Gere LLVM IR:
   ```
   ./bin/datalang caminho/arquivo.datalang -o output.ll
   ```
2. Linke com runtime e rode:
   ```
   clang -Wno-override-module output.ll src/codegen/runtime.c -o programa -lm
   ./programa
   ```

## Como usar DataFrames
```datalang
let df: DataFrame = load("dados.csv");
let filtrado: DataFrame = df |> filter(|row: Row| row.idade >= 18);
let proj: DataFrame = filtrado |> select(nome, idade, salario);
let grupos: DataFrame = proj |> groupby(nome);
save(proj, "saida.csv");
```
Notas:
- `filter` numérico aceita comparações simples com literais (>, >=, <, <=, ==, !=).
- `map` sobre DataFrame extrai coluna numérica opcionalmente com escala/offset (ex.: `row.salario * 1.1`).
- `reduce` em DataFrame hoje reduz o array numérico resultante de `map`; agregação por grupos não é suportada.

## Organização dos arquivos
- Fonte do compilador: `src/lexer`, `src/parser`, `src/semantic`, `src/codegen`.
- Runtime C: `src/codegen/runtime.c`.
- Gramática: `docs/gramatica_refatorada.md`.
- Manuais: `docs/manual_instalacao.md`, `docs/manual_uso.md`.
- Exemplos: `examples/`.

## Comandos úteis
- Rebuild do compilador: `make`
- Rodar exemplo avançado:  
  `make && ./bin/datalang examples/exemplo_avancado.datalang -o output.ll && clang -Wno-override-module output.ll src/codegen/runtime.c -o programa -lm && ./programa`
- Rodar exemplo completo 2:  
  `make && ./bin/datalang examples/exemplo_completo_2.datalang -o output.ll && clang -Wno-override-module output.ll src/codegen/runtime.c -o programa -lm && ./programa`

## Mapeamento de tipos para LLVM
- `Int` → `i64`
- `Float` → `double`
- `Bool` → `i1`
- `String` → `i8*`
- Arrays → `{i64, <elem>*}`
- Custom types → `%struct.Nome*`
- DataFrame → `i8*` (manipulado pelo runtime)

## Suíte de testes de exemplo
Execute os arquivos em `examples/` para validar:  
`exemplo_completo.datalang`, `exemplo_completo_2.datalang`, `exemplo_06.datalang`, `exemplo_avancado.datalang`.
