# DataLang - Analisador Léxico

## 🎯 Sobre

Este módulo implementa um analisador léxico completo para a linguagem DataLang, utilizando:
- **AFDs individuais e conversão para AFN unificado**
- **Conversão AFN→AFD** via construção de subconjuntos
- **AFD unificado** para reconhecimento eficiente de tokens

## 🚀 Uso Rápido

### Compilação com Make (Linux/macOS)

```bash
make
```

### Compilação Manual (Todas as Plataformas)

#### Linux/macOS:
```bash
gcc -Wall -Wextra -std=c99 -g -o datalang_lexer datalang_afn.c afn_to_afd.c lexer.c
```

#### Windows (MinGW):
```cmd
gcc -Wall -Wextra -std=c99 -g -o datalang_lexer.exe datalang_afn.c afn_to_afd.c lexer.c
```

#### Windows (WSL):
```bash
gcc -Wall -Wextra -std=c99 -g -o datalang_lexer datalang_afn.c afn_to_afd.c lexer.c
```

### Execução

#### Testes Internos

**Linux/macOS:**
```bash
./datalang_lexer
```

**Windows:**
```cmd
datalang_lexer.exe
```

#### Analisar Arquivo

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

## 🏗️ Arquitetura

### Componentes Principais

1. **datalang_afn.c** - Implementação dos AFDs individuais e transformando em um único AFN
2. **afn_to_afd.c** - Algoritmo de conversão AFN→AFD
3. **lexer.c** - Analisador léxico principal usando AFD

### Estrutura de Desenvolvimento

```bash
src/lexer/
├── datalang_afn.h/c    # Implementações dos AFDs e conversão para AFN
├── afn_to_afd.h/c      # Conversão AFN→AFD  
├── lexer.c             # Analisador léxico principal
├── Makefile            # Sistema de build (Linux/macOS)
└── README.md           # Este arquivo
```

## 🔧 Limpeza de Arquivos Compilados

### Linux/macOS:
```bash
rm -f datalang_lexer
```

### Windows:
```cmd
del datalang_lexer.exe
```

## 📚 Documentação Relacionada

- [Documentação Principal](../../README.md)
- [Definição Formal da Linguagem](../../docs/2_definicao_formal_linguagem.md)
- [Expressões Regulares](../../docs/4_expressoes_regulares_.md)
- [Diagramas dos AFDs](../../docs/diagramas/)

---

**Nota:** Para desenvolvimento no Windows, recomenda-se o uso do WSL (Windows Subsystem for Linux) ou MinGW para melhor compatibilidade.