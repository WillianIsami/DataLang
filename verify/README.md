# Verificação Externa com Idris

Objetivo: usar Idris como verificador de propriedades/typing para programas DataLang, sem incorporar código Idris ao runtime da linguagem.

Fluxo recomendado:
1. Gere o JSON da AST com o compilador DataLang usando `--verify` (opcionalmente `--verify-json <arquivo>`).
2. Compile seu verificador Idris para um binário que consuma esse JSON.
3. Execute o binário a partir do `run_verifier.sh` (padrão) ou passe outro comando via `--verify-cmd`.

Como estruturar o verificador Idris:
- Crie um pacote Idris em `verify/` (ex.: `datalang_verify.ipkg`).
- Modele os tipos da AST que precisa (Program, Decl, Expr, Type...).
- Parseie o JSON (use libs de JSON de Idris2) para seus tipos.
- Escreva provas/verificações (totalidade, invariantes de tipos, etc.).
- Exporte um `main : IO ()` que lê o arquivo JSON (arg1) e retorna código 0 em sucesso; diferente de 0 em erro.
- Compile com backend C: `idris2 --build datalang_verify.ipkg` (gera `verify/datalang_verify`).

Stub incluído:
- `verify/Main.idr`: lê o caminho do JSON e imprime; substitua pela sua lógica.
- `verify/datalang_verify.ipkg`: pacote Idris pronto para compilar com `idris2 --build`.

Arquivo de apoio:
- `run_verifier.sh`: executa `verify/datalang_verify` (substitua se quiser).

Exemplo de uso do compilador:
```
./bin/datalang exemplos/ffi_idris.datalang --verify \
    --verify-json ast.json \
    --verify-cmd "verify/run_verifier.sh"
```

Personalize:
- Use `--verify-cmd "idris2 --exec 'Main.main' --verbose verify/Main.idr"` se preferir rodar direto do fonte (mais lento).
- Gere JSON em outro local com `--verify-json /tmp/ast.json`.

Tratamento de erro:
- Se o verificador terminar com código diferente de zero, o compilador interrompe antes de gerar LLVM.
