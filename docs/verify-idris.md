# Verificação formal com Idris

Este projeto inclui um verificador externo escrito em Idris para checar a AST do DataLang após a análise semântica do compilador C. O fluxo garante invariantes como:
- Estrutura da AST (raiz `Program`, campos obrigatórios).
- Tipagem das expressões/comandos, incluindo pipelines (`filter`, `map`, `reduce`, `|> fn(...)`).
- Unicidade de nomes (funções, parâmetros, campos de `data`).
- Construtores de `data` e chamadas de função com tipos compatíveis.
- Funções não-`Void` retornam em todos os caminhos.
- Rejeição de tipos indefinidos (`?`/`Unknown`) em assinaturas, campos e globais.

## Comandos principais

1) Compilar verificador Idris:
```bash
make verify-idris
```

2) Rodar compilador com verificação Idris (exemplo completo):
```bash
./bin/datalang examples/exemplo_completo.datalang --verify --verify-cmd "verify/run_verifier.sh"
```

3) Alvo automatizado para compilar tudo e verificar o exemplo completo:
```bash
make verify-full
```

## Saídas esperadas
- `[Idris verify] AST OK (Program válido e invariantes satisfeitas)` → verificação passou.
- Em caso de erro, o verificador lista cada violação (tipos incompatíveis, falta de retorno, nomes duplicados, tipo indefinido etc.) e retorna erro para o compilador.

## Dicas de uso
- Sempre mantenha `idris2` instalado e no `PATH`.
- Se precisar usar outro verificador/versão, troque `--verify-cmd` ao chamar `datalang`.
- Para depurar, gere a AST em JSON (`ast.json`) e rode diretamente:
  ```bash
  verify/run_verifier.sh ast.json
  ```
