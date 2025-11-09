# DataLang - Compilador Completo

## Sobre

Este projeto implementa um compilador completo para a linguagem DataLang, contendo:

### Analisador Léxico
- **AFDs individuais e conversão para AFN unificado**
- **Conversão AFN→AFD** via construção de subconjuntos
- **AFD unificado** para reconhecimento eficiente de tokens

### Analisador Sintático (PARSER)
- Análise de expressões aritméticas e lógicas
- Reconhecimento de estruturas de controle
- Validação sintática completa
- Integração com o analisador léxico

## Uso Rápido

### Compilação com Make (Linux/macOS)

```bash
make
```

### Compilação Manual (Todas as Plataformas)

#### Linux/macOS:
```bash
gcc -Wall -Wextra -std=c99 -g -I. -o datalang \
    lexer/datalang_afn.c lexer/afn_to_afd.c lexer/lexer.c \
    parser/parser.c parser/parser_expr.c parser/parser_main.c
```

#### Windows (MinGW):
```cmd
gcc -Wall -Wextra -std=c99 -g -I. -o datalang.exe ^
    lexer/datalang_afn.c lexer/afn_to_afd.c lexer/lexer.c ^
    parser/parser.c parser/parser_expr.c parser/parser_main.c
```

#### Windows (WSL):
```bash
gcc -Wall -Wextra -std=c99 -g -I. -o datalang \
    lexer/datalang_afn.c lexer/afn_to_afd.c lexer/lexer.c \
    parser/parser.c parser/parser_expr.c parser/parser_main.c
```

### Execução

#### Testes Internos

**Linux/macOS:**
```bash
./datalang
```

**Windows:**
```cmd
datalang.exe
```

#### Analisar Arquivo

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

### Comandos Make

```bash
make          # Compila o compilador completo
make clean    # Remove todos os arquivos compilados
make test     # Executa teste com código embutido
make test-file # Executa teste com arquivo exemplo
make exemplo  # Cria arquivo de exemplo se não existir
make help     # Mostra ajuda completa
```

## Arquitetura

### Componentes Principais

#### Analisador Léxico:
1. **datalang_afn.c** - Implementação dos AFDs individuais e transformando em um único AFN
2. **afn_to_afd.c** - Algoritmo de conversão AFN→AFD
3. **lexer.c** - Analisador léxico principal usando AFD

#### Analisador Sintático (NOVO):
1. **parser.c** - Analisador sintático principal
2. **parser_expr.c** - Análise de expressões
3. **parser_main.c** - Ponto de entrada do parser

### Estrutura de Desenvolvimento

```bash
src/
├── Makefile                 # Sistema de build unificado
├── lexer/                   # Analisador léxico
│   ├── datalang_afn.h/c    # Implementações dos AFDs e conversão para AFN
│   ├── afn_to_afd.h/c      # Conversão AFN→AFD  
│   ├── lexer.c/h           # Analisador léxico principal
│   └── logs/               # Logs de desenvolvimento
└── parser/                  # NOVO: Analisador sintático
    ├── parser.c/h          # Parser principal
    ├── parser_expr.c       # Análise de expressões
    └── parser_main.c       # Ponto de entrada
```

## Limpeza de Arquivos Compilados

### Linux/macOS:
```bash
make clean
# ou
rm -f datalang lexer/*.o parser/*.o
```

### Windows:
```cmd
del datalang.exe
```

## Documentação Relacionada

- [Documentação Principal](../README.md)
- [Definição Formal da Linguagem](../docs/2_definicao_formal_linguagem.md)
- [Expressões Regulares](../docs/4_expressoes_regulares_.md)
- [Diagramas dos AFDs](../docs/diagramas/)
- [Gramática Formal](../docs/gramatica_formal.md)

---

**Nota:** Para desenvolvimento no Windows, recomenda-se o uso do WSL (Windows Subsystem for Linux) ou MinGW para melhor compatibilidade.

### Status do Projeto
- ✅ **Analisador Léxico** - Completo
- ✅ **Analisador Sintático** - Em funcionamento
- 🔄 **Analisador Semântico** - Em desenvolvimento
- 🔄 **Gerador de Código** - Planejado

As principais mudanças refletem:
1. **Adição do parser** ao compilador
2. **Makefile unificado** na raiz do src/
3. **Novos comandos** de compilação e teste
4. **Estrutura atualizada** do projeto
5. **Caminhos corrigidos** para os arquivos de exemplo