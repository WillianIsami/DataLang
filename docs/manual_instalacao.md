# Manual de Instalação e Build

## Pré‑requisitos
- Linux (nativo ou WSL/Ubuntu no Windows) ou macOS.
- Compilador C (`gcc`), ferramenta de build (`make`).
- LLVM/Clang para linkar o IR gerado: `clang -Wno-override-module output.ll src/codegen/runtime.c -o programa -lm`.

## Estrutura do projeto
- Código‐fonte do compilador: `src/lexer`, `src/parser`, `src/semantic`, `src/codegen`.
- Exemplos da linguagem: `examples/`.
- Gramática formal: `docs/gramatica_refatorada.md`.
- Runtime em C usado na linkedição: `src/codegen/runtime.c`.

## Instalação detalhada (baseado no README)

### Windows (WSL Ubuntu)
1) Instalar WSL/Ubuntu:
```
wsl --install -d Ubuntu
```
2) Atualizar pacotes:
```
sudo apt update
sudo apt upgrade -y
```
3) Instalar ferramentas:
```
sudo apt install -y build-essential make clang llvm
```
4) Verificar versões:
```
gcc --version
make --version
clang --version
```
5) Clonar o repositório:
```
cd ~
git clone https://github.com/WillianIsami/DataLang.git
cd DataLang
```

## Passo a passo
1. Abra um terminal e entre no diretório do projeto `DataLang/`.
2. Compile o compilador:
   - `make`
   - O binário é criado em `bin/datalang`.
3. Compile um programa DataLang para LLVM IR:
   - `./bin/datalang examples/exemplo_completo_2.datalang -o output.ll`
   - A saída LLVM fica em `output.ll`.
4. Gere o executável final com o runtime:
   - `clang -Wno-override-module output.ll src/codegen/runtime.c -o programa -lm`
5. Execute:
   - `./programa`

## Rodando todos os exemplos principais
- `examples/exemplo_completo.datalang`
- `examples/exemplo_completo_2.datalang`
- `examples/exemplo_06.datalang`
- `examples/exemplo_avancado.datalang`

Comando típico:
```
make && ./bin/datalang examples/exemplo_avancado.datalang -o output.ll && \
clang -Wno-override-module output.ll src/codegen/runtime.c -o programa -lm && ./programa
```

## Dicas de solução de problemas
- Erro de link/clang ausente: instale o `clang` do seu sistema.
- Erro de permissão: garanta que está no diretório do projeto e tem permissão de escrita.
- Arquivo CSV não encontrado: coloque o CSV no diretório raiz do projeto ou ajuste o caminho no código.
