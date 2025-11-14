# DataLang

Linguagem de programação especializada em processamento de dados, desenvolvida como parte do projeto de Compiladores e Linguagens Formais.

## Visão Geral

DataLang é uma linguagem de programação projetada para facilitar a manipulação e transformação de dados. Este repositório contém a implementação do compilador, agora incluindo análise léxica, sintática e semântica.

-----

## Estrutura do Projeto

A estrutura foi atualizada para incluir o analisador semântico e centralizar o `Makefile` na raiz.

```
DataLang/
├── Makefile           # Makefile principal
├── README.md          # Este arquivo
├── bin/               # Executáveis compilados
├── build/             # Arquivos-objeto intermediários
├── docs/              # Documentação (AFDs, gramática, etc.)
├── examples/          # Exemplos de código DataLang
├── src/
│   ├── main.c         # Ponto de entrada do compilador
│   ├── lexer/         # Código do Analisador Léxico
│   ├── parser/        # Código do Analisador Sintático
│   └── semantic/      # Código do Analisador Semântico
└── tests/
    └── test_semantic.c  # Testes para o Analisador Semântico
```

-----

## 🛠️ Compilação e Execução

### Pré-requisitos

  * `gcc` (GNU Compiler Collection) ou compilador C compatível
  * `make` (opcional, recomendado)
  * **Windows**: Recomenda-se o uso de MinGW ou WSL (Windows Subsystem for Linux)

-----

### Método 1: Usando Make (Recomendado)

O `Makefile` na raiz do projeto gerencia toda a compilação.

1.  **Compilar tudo (compilador e testes):**

    ```bash
    make
    ```

    (ou `make all`)

2.  **Executar o compilador em um arquivo de exemplo:**

    ```bash
    make run
    ```

    (Isso executa `./bin/datalang examples/exemplo.datalang`)

3.  **Testar um arquivo específico:**

    ```bash
    make test-file FILE=examples/exemplo.datalang
    ```

4.  **Limpar arquivos compilados:**

    ```bash
    make clean
    ```

5.  **Ver ajuda:**

    ```bash
    make help
    ```

-----

### Método 2: Compilação Manual (Sem Make)

Siga estas instruções caso não tenha o `make` instalado. Os comandos devem ser executados a partir do diretório **raiz** do projeto.

#### 1\. Criar Diretórios de Saída

```bash
# Linux / macOS / WSL
mkdir -p bin

# Windows (CMD)
if not exist bin ( mkdir bin )
```

#### 2\. Compilar o Compilador `datalang`

**Linux / macOS / WSL:**

```bash
gcc -Wall -Wextra -std=c11 -g -I. -Isrc/lexer -Isrc/parser -Isrc/semantic -o bin/datalang \
    src/main.c \
    src/lexer/datalang_afn.c src/lexer/afn_to_afd.c src/lexer/lexer.c \
    src/parser/parser.c src/parser/parser_expr.c src/parser/parser_main.c \
    src/semantic/symbol_table.c src/semantic/type_system.c src/semantic/type_inference.c src/semantic/semantic_analyzer.c
```

**Windows (MinGW):**

```cmd
gcc -Wall -Wextra -std=c11 -g -I. -Isrc/lexer -Isrc/parser -Isrc/semantic -o bin\datalang.exe ^
    src/main.c ^
    src/lexer/datalang_afn.c src/lexer/afn_to_afd.c src/lexer/lexer.c ^
    src/parser/parser.c src/parser/parser_expr.c src/parser/parser_main.c ^
    src/semantic/symbol_table.c src/semantic/type_system.c src/semantic/type_inference.c src/semantic/semantic_analyzer.c
```

-----

### Execução (Após compilar manualmente)

#### Analisar um Arquivo

**Linux / macOS / WSL:**

```bash
./bin/datalang examples/exemplo.datalang
```

**Windows (CMD):**

```cmd
bin\datalang.exe examples\exemplo.datalang
```

-----

## Documentação Adicional

Para mais detalhes sobre a gramática, autômatos e definições formais da linguagem, consulte os arquivos no diretório `/docs`:

  * [Definição Formal da Linguagem](docs/2_definicao_formal_linguagem.md)
  * [Expressões Regulares](docs/4_expressoes_regulares_.md)
  * [Gramática Formal](docs/gramatica_formal.md)
  * [Diagramas dos AFDs](docs/diagramas/)