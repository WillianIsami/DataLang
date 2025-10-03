# DataLang

Linguagem de programação DataLang

## 📋 Visão Geral

## 🏗️ Estrutura do Projeto (Em construção...)

```
├── README.md
├── .gitignore
│
├── docs/                                 # Documentação
│   ├── 1_proposta_inicial.md
│   ├── 2_definicao_formal_linguagem.md
│   ├── 4_expressoes_regulares_.md
│   ├── gramatica_formal.md
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
├── src/lexer/
│   ├── datalang_afn.h                    # Implementação de AFNs
│   ├── datalang_lexer_integrated.c       # Analisador léxico completo
│   ├── test_afn_conversion.c             # Testes de conversão AFN→AFD
│   ├── datalang_lexer.h                  # Header legado
│   ├── datalang_lexer.c                  # Implementação legada
│   ├── datalang_tests.c                  # Testes legados
│   ├── Makefile                          # Sistema de build
│   └── README.md                         # Lexer README
```

## Análise Léxica

Leia o [README.md](src/lexer/README.md) dentro da pasta src/lexer

## 🤝 Contribuições

Este é um projeto acadêmico desenvolvido como parte do curso de Compiladores. Sugestões e melhorias são bem-vindas através de issues e pull requests.

## 📄 Licença

Projeto desenvolvido para fins educacionais. Código disponível sob licença MIT.