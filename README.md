# DataLang

Linguagem de programação especializada em processamento de dados, desenvolvida como parte do projeto integrador do curso de Compiladores e Linguagens Formais.

## 📋 Visão Geral

DataLang é uma linguagem de programação projetada para facilitar a manipulação e transformação de dados. Ela combina uma sintaxe limpa e expressiva com operações de alto nível para trabalhar com conjuntos de dados, inspirada em linguagens como Rust, Elixir e Python.

## 🏗️ Estrutura do Projeto

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
│   └── test_fix.datalang
└── src/
    └── lexer/                # Implementação do analisador léxico
        ├── Makefile
        ├── README.md
        ├── datalang_afn.c    # Implementação de AFNs
        ├── datalang_afn.h
        ├── afn_to_afd.c      # Conversão AFN para AFD
        ├── afn_to_afd.h
        ├── lexer.c           # Analisador léxico principal
```

## 🚀 Começando

### Pré-requisitos

- GCC (GNU Compiler Collection) ou compilador C compatível
- **Windows**: MinGW ou WSL (Windows Subsystem for Linux)

### Compilação

#### Método 1: Usando Make (Linux/macOS)
```bash
cd src/lexer
make
```

#### Método 2: Compilação manual (Linux/macOS/Windows)
```bash
cd src/lexer
gcc -Wall -Wextra -std=c99 -g -o datalang_lexer datalang_afn.c afn_to_afd.c lexer.c
```

#### Método 3: Windows com MinGW
```cmd
cd src\lexer
gcc -Wall -Wextra -std=c99 -g -o datalang_lexer.exe datalang_afn.c afn_to_afd.c lexer.c
```

#### Método 4: Windows com WSL
```bash
# Dentro do WSL
cd src/lexer
gcc -Wall -Wextra -std=c99 -g -o datalang_lexer datalang_afn.c afn_to_afd.c lexer.c
```

### Execução

#### Modo de teste (executa testes internos)

**Linux/macOS:**
```bash
./datalang_lexer
```

**Windows:**
```cmd
datalang_lexer.exe
```

#### Modo de arquivo (analisa um arquivo .datalang)

**Linux/macOS:**
```bash
./datalang_lexer ../../examples/exemplo.datalang
./datalang_lexer ../../examples/test_fix.datalang
```

**Windows:**
```cmd
datalang_lexer.exe ..\..\examples\exemplo.datalang
datalang_lexer.exe ..\..\examples\test_fix.datalang
```

---

Para mais detalhes sobre a implementação do analisador léxico, consulte [src/lexer/README.md](src/lexer/README.md).