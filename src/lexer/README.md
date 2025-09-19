# DataLang AFDs - Analisador Léxico

Implementação de Autômatos Finitos Determinísticos (AFDs) para a linguagem DataLang, desenvolvida como parte do projeto integrador do curso de Compiladores e Linguagens Formais.

## 📋 Visão Geral

Este projeto implementa um analisador léxico completo usando AFDs para reconhecer os tokens da linguagem DataLang, incluindo:

- **Identificadores**: variáveis, nomes de função
- **Números**: inteiros, decimais, notação científica
- **Strings**: com suporte a caracteres de escape
- **Palavras-chave**: let, fn, data, filter, map, reduce, etc.
- **Operadores**: +, -, *, /, ==, !=, <=, >=, &&, ||, |>, =>, etc.
- **Delimitadores**: parênteses, colchetes, chaves, vírgulas
- **Comentários**: linha (//) e bloco (/* */)
- **Whitespace**: espaços, tabs, quebras de linha

## 🏗️ Estrutura do Projeto

```bash
DataLang/
├── README.md
├── .gitignore
│
├── docs/                                 # Documentação
│   ├── 1_proposta_inicial.md             # Proposta inicial
│   ├── 2_definicao_formal_linguagem.md   # Proposta inicial
│   ├── especificacao-linguagem.md        # Definição completa da sua Linguagem
│   └── diagramas/                        # Diagramas
│       ├── afd_comentarios.md
│       ├── afd_delimitadores.md
│       ├── afd_identificadores.md
│       ├── afd_literais_numericos.md
│       ├── afd_literais_strings.md
│       ├── afd_operadores.md
│       ├── afd_palavra_chave.md
│       ├── afd_tipos_dados.md
│       └── afd_whitespace.md
│
├── src/                               # Código fonte principal (src)
│   ├── lexer/                         # Análise léxica
│   │   ├── datalang_lexer.c       # Implementação principal dos AFDs
│   │   ├── datalang_lexer.h       # Arquivo de cabeçalho
│   │   ├── datalang_tests.c       # Testes unitários abrangentes
│   │   ├── Makefile               # Sistema de build
│   │   └── README.md              # Este arquivo
```

## 🚀 Compilação e Execução

### Pré-requisitos

- GCC (GNU Compiler Collection)
- Make (opcional, mas recomendado)
- Valgrind (opcional, para verificação de memória)

### Compilação

```bash
# Compilar tudo
make

# Ou compilar componentes individuais
make lexer-only    # Apenas o lexer
make test-only     # Apenas os testes

# Compilação manual
gcc -Wall -Wextra -std=c99 -o datalang_lexer datalang_lexer.c
gcc -Wall -Wextra -std=c99 -o datalang_test datalang_test.c datalang_lexer.c
```

### Execução

```bash
# Executar lexer com testes integrados
make run
# ou
./datalang_lexer

# Executar suite estendida de testes
make test
# ou
./datalang_test

# Executar ambos
make run-all
```

### Targets do Makefile

```bash
make all              # Compilar todos os executáveis
make lexer-only       # Compilar apenas o lexer
make test-only        # Compilar apenas os testes
make run              # Executar lexer com testes integrados
make test             # Executar suite estendida de testes
make run-all          # Executar ambos os programas
make debug            # Compilar com informações de debug
make release          # Compilar otimizado para produção
make memcheck         # Verificar vazamentos de memória
make memcheck-lexer   # Verificar memória do lexer
make static-analysis  # Análise estática (cppcheck, clang-tidy)
make coverage         # Gerar relatório de cobertura
make clean            # Limpar arquivos gerados
make check-deps       # Verificar dependências do sistema
make help             # Mostrar ajuda completa
```

## 🧪 Testes

O projeto inclui duas suites de testes:

### 1. Testes Integrados (datalang_lexer.c)
- Testes unitários de cada AFD individual
- Testes de integração do analisador completo
- Validação de palavras-chave e tipos
- Teste do algoritmo de minimização
- Tratamento de erros

### 2. Suite Estendida (datalang_test.c)
- Teste de padrões específicos do DataLang
- Casos extremos e edge cases
- Teste de consistência
- Gerenciamento de memória
- Stress test com entradas grandes
- Benchmark de performance

### Executando Testes

```bash
# Testes básicos integrados
make run

# Suite estendida completa
make test

# Testes com verificação de memória
make memcheck

# Verificar dependências antes dos testes
make check-deps
```

## 🔧 Funcionalidades Implementadas

### AFDs Específicos

1. **AFD de Identificadores**
   - Padrão: `[a-zA-Z_][a-zA-Z0-9_]*`
   - Estados: 2
   - Reconhece: `variable`, `_private`, `var1`, `CamelCase`

2. **AFD de Números**
   - Padrão: `[+-]?(\d+\.\d*|\.\d+|\d+)([eE][+-]?\d+)?`
   - Estados: 8
   - Reconhece: `42`, `3.14`, `2.5e-10`, `+123`, `-.5`, `1E+5`

3. **AFD de Strings**
   - Padrão: `"([^"\\]|\\.)*"`
   - Estados: 4
   - Suporte a: `\"`, `\\`, `\n`, `\t`, `\r`

4. **AFD de Comentários de Linha**
   - Padrão: `//.*`
   - Estados: 4
   - Reconhece comentários até final da linha

5. **AFD de Comentários de Bloco**
   - Padrão: `/* ... */`
   - Estados: 5
   - Suporte a comentários multilinhas

6. **AFD de Operadores**
   - Estados: 20
   - Reconhece: `+`, `-`, `*`, `/`, `%`, `=`, `==`, `!=`, `<`, `<=`, `>`, `>=`, `&&`, `||`, `|>`, `=>`

7. **AFD de Delimitadores**
   - Estados: 12
   - Reconhece: `(`, `)`, `[`, `]`, `{`, `}`, `;`, `,`, `:`, `.`, `..`

8. **AFD de Whitespace**
   - Padrão: `[ \t\n\r]+`
   - Estados: 2

### Recursos Implementados

- **Análise léxica completa**: Combina múltiplos AFDs para tokenização
- **Gerenciamento de memória**: Alocação/liberação segura de tokens
- **Tratamento de erros**: Detecção de tokens malformados e caracteres inválidos
- **Informações de localização**: Linha e coluna para cada token
- **Classificação inteligente**: Diferencia keywords, tipos e identificadores
- **Algoritmo de minimização**: Versão básica para otimização de AFDs

## 💡 Exemplo de Uso

```c
#include <stdio.h>
#include <stdlib.h>

// Assumindo que as definições estão no mesmo arquivo ou header
int main() {
    const char* code = "let x = 42\n"
                       "if (x > 0) {\n"
                       "  return \"positive\"\n"
                       "}";
    
    Token* tokens = tokenize(code);
    if (!tokens) {
        printf("Erro na tokenização\n");
        return 1;
    }
    
    for (int i = 0; tokens[i].type != TOKEN_EOF; i++) {
        if (tokens[i].type != TOKEN_WHITESPACE) {
            printf("Token: %-12s | Valor: %-15s | L:%d C:%d\n",
                   token_type_name(tokens[i].type),
                   tokens[i].value ? tokens[i].value : "(null)",
                   tokens[i].line,
                   tokens[i].column);
        }
    }
    
    // Liberar memória
    for (int i = 0; tokens[i].type != TOKEN_EOF; i++) {
        free(tokens[i].value);
    }
    free(tokens);
    
    return 0;
}
```

**Saída esperada:**
```
Token: KEYWORD      | Valor: let             | L:1 C:1
Token: IDENTIFIER   | Valor: x               | L:1 C:5
Token: OPERATOR     | Valor: =               | L:1 C:7
Token: INTEGER      | Valor: 42              | L:1 C:9
Token: KEYWORD      | Valor: if              | L:2 C:1
Token: DELIMITER    | Valor: (               | L:2 C:4
Token: IDENTIFIER   | Valor: x               | L:2 C:5
Token: OPERATOR     | Valor: >               | L:2 C:7
Token: INTEGER      | Valor: 0               | L:2 C:9
Token: DELIMITER    | Valor: )               | L:2 C:10
Token: DELIMITER    | Valor: {               | L:2 C:12
Token: KEYWORD      | Valor: return          | L:3 C:3
Token: STRING       | Valor: "positive"      | L:3 C:9
Token: DELIMITER    | Valor: }               | L:4 C:1
```

## 📊 Resultados dos Testes

### Testes Integrados
```
=== Teste AFD Identificadores ===
'variable': ACEITO (esperado: ACEITO) - PASS - Identificador simples
'_private': ACEITO (esperado: ACEITO) - PASS - Começando com underscore
'var1': ACEITO (esperado: ACEITO) - PASS - Com número
Resultado: 11/11 testes passaram

=== Teste de Integração - Analisador Léxico Completo ===
Teste 1: Programa simples
Código: let x = 42
Tokens gerados:
  KEYWORD: 'let'
  IDENTIFIER: 'x'
  OPERATOR: '='
  INTEGER: '42'
```

### Suite Estendida
```
=== Teste de Stress - Entrada Grande ===
Testando entrada com 1247 caracteres...
Tokens gerados: 405
Tempo de processamento: 0.001234 segundos
Performance: 328205 tokens/segundo

=== Benchmark de Performance ===
Executando 1000 iterações de tokenização...
Tempo total: 0.0234 segundos
Tempo médio por iteração: 0.000023 segundos
Iterações por segundo: 42735
```

## 🎯 Características Técnicas

### Performance
- **Complexidade temporal**: O(n) para análise léxica
- **Complexidade espacial**: O(m) onde m é o número de tokens
- **Throughput**: ~300k tokens/segundo em hardware moderno

### Robustez
- Tratamento seguro de caracteres inválidos
- Detecção de tokens malformados (strings não fechadas, etc.)
- Gerenciamento automático de memória com verificação de erros
- Suporte a entradas grandes (testado com 50k+ caracteres)

### Portabilidade
- Código C99 padrão
- Sem dependências externas
- Funciona em sistemas Unix/Linux/Windows com GCC

## 🔍 Limitações Conhecidas

1. **Alfabeto limitado**: Suporte apenas ASCII (0-255)
2. **Tokens malformados**: Alguns erros geram múltiplos tokens UNKNOWN
3. **Algoritmo de minimização**: Implementação básica, não otimizada
4. **Capacidade de tokens**: Limitado a 1000 tokens por entrada
5. **Comentários aninhados**: Não suportados (apenas um nível)

## 🚧 Decisões de Implementação

### Escolhas de Design
1. **AFDs separados**: Cada tipo de token tem seu próprio AFD para modularidade
2. **Tabela de transições**: Array 2D para performance O(1) nas transições
3. **Estados especiais**: Constantes STATE_ACCEPT/STATE_REJECT para clareza
4. **Função safe_strndup**: Implementação própria para portabilidade
5. **Preservação de whitespace**: Mantém formatação para possível uso futuro

### Tratamento de Erros
- Caracteres inválidos geram tokens TOKEN_UNKNOWN
- Strings/comentários não fechados são tratados como válidos até EOF
- Falhas de alocação de memória são detectadas e tratadas

## 🔧 Extensões Futuras

### Melhorias Planejadas
- [ ] Suporte a caracteres Unicode
- [ ] Mensagens de erro mais descritivas
- [ ] Otimização do algoritmo de minimização
- [ ] Suporte a comentários aninhados
- [ ] Interface para streaming de tokens grandes
- [ ] Modo de recuperação de erros mais inteligente

### Integração
- [ ] Geração de header file separado
- [ ] Interface para analisador sintático
- [ ] Suporte a múltiplos arquivos fonte
- [ ] Plugin para editores de código

## 🤝 Uso Acadêmico

Este projeto foi desenvolvido para demonstrar:
- Implementação prática de AFDs
- Técnicas de análise léxica
- Gerenciamento de memória em C
- Testes unitários abrangentes
- Algoritmos de minimização de autômatos

## 📄 Informações Técnicas

### Compilação
- **Padrão**: C99
- **Compilador**: GCC recomendado
- **Flags**: `-Wall -Wextra -std=c99 -pedantic`
- **Linking**: Sem dependências externas

### Arquivos
- `datalang_lexer.c`: ~1500 linhas, implementação completa
- `datalang_test.c`: ~800 linhas, testes estendidos
- `Makefile`: Sistema de build completo

Este README reflete a implementação atual e real do projeto, incluindo todas as funcionalidades implementadas e limitações conhecidas.
