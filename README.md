# DataLang

Linguagem de programação especializada em processamento de dados, desenvolvida como parte do projeto integrador do curso de Compiladores e Linguagens Formais.

## Visão Geral

DataLang é uma linguagem de programação projetada para facilitar a manipulação e transformação de dados. Ela combina uma sintaxe limpa e expressiva com operações de alto nível para trabalhar com conjuntos de dados, inspirada em linguagens como Rust, Elixir e Python.

## Estrutura do Projeto

```bash
DataLang/
├── README.md                 # Este arquivo
├── docs/                     # Documentação do projeto
│   ├── 1_proposta_inicial.md
│   ├── 2_definicao_formal_linguagem.md
│   ├── 4_expressoes_regulares_.md
│   ├── gramatica_formal.md
│   └── diagramas/            # Diagramas de autômatos
├── examples/                 # Exemplos de código DataLang
│   ├── exemplo.datalang
│   ├── exemplo_01.datalang
│   └── test_fix.datalang
└── src/
    ├── Makefile              # Novo Makefile unificado
    ├── lexer/                # Implementação do analisador léxico
    │   ├── datalang_afn.c    # Implementação de AFNs
    │   ├── datalang_afn.h
    │   ├── afn_to_afd.c      # Conversão AFN para AFD
    │   ├── afn_to_afd.h
    │   ├── lexer.c           # Analisador léxico principal
    │   └── lexer.h
    └── parser/               # NOVO: Implementação do analisador sintático
        ├── parser.c
        ├── parser.h
        ├── parser_expr.c
        └── parser_main.c
```

## Começando

### Pré-requisitos

- GCC (GNU Compiler Collection) ou compilador C compatível
- **Windows**: MinGW ou WSL (Windows Subsystem for Linux)

### Compilação

#### Método 1: Usando Make (Linux/macOS)
```bash
cd src
make
```

#### Método 2: Compilação manual (Linux/macOS/Windows)
```bash
cd src
gcc -Wall -Wextra -std=c99 -g -I. -o datalang lexer/datalang_afn.c lexer/afn_to_afd.c lexer/lexer.c parser/parser.c parser/parser_expr.c parser/parser_main.c
```

#### Método 3: Windows com MinGW
```cmd
cd src
gcc -Wall -Wextra -std=c99 -g -I. -o datalang.exe lexer/datalang_afn.c lexer/afn_to_afd.c lexer/lexer.c parser/parser.c parser/parser_expr.c parser/parser_main.c
```

#### Método 4: Windows com WSL
```bash
# Dentro do WSL
cd src
gcc -Wall -Wextra -std=c99 -g -I. -o datalang lexer/datalang_afn.c lexer/afn_to_afd.c lexer/lexer.c parser/parser.c parser/parser_expr.c parser/parser_main.c
```

### Execução

#### Modo de teste (executa testes internos)

**Linux/macOS:**
```bash
./datalang
```

**Windows:**
```cmd
datalang.exe
```

#### Modo de arquivo (analisa um arquivo .datalang)

**Linux/macOS:**
```bash
./datalang ../examples/exemplo.datalang
./datalang ../examples/test_fix.datalang
```

**Windows:**
```cmd
datalang.exe ..\examples\exemplo.datalang
datalang.exe ..\examples\test_fix.datalang
```

### Comandos Make Úteis

```bash
cd src
make          # Compila o compilador
make clean    # Remove arquivos compilados
make test     # Executa teste com código embutido
make test-file # Executa teste com arquivo exemplo
make exemplo  # Cria arquivo de exemplo se não existir
make help     # Mostra ajuda completa
```

## Novas Funcionalidades

### Analisador Sintático (Parser)
- Análise de expressões aritméticas e lógicas
- Reconhecimento de estruturas de controle
- Validação sintática completa
- Integração com o analisador léxico

### Sistema de Build Unificado
- Makefile único para todo o projeto
- Compilação integrada lexer + parser
- Comandos de teste simplificados
- Suporte multiplataforma

---

Para mais detalhes sobre a implementação do analisador léxico e sintático, consulte [src/README.md](src/README.md).
