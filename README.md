# DataLang

Linguagem de programação especializada em processamento de dados, desenvolvida como parte do projeto de Compiladores e Linguagens Formais.

## Acesso Rápido (IMPORTANTE)
- [Manual de utilização](docs/manual_uso.md)
- [Manual de instalação](docs/manual_instalacao.md)
- [Analisador léxico](src/lexer/)
- [Analisador sintático (AST)](src/parser/)
- [Analisador semântico](src/semantic/)
- [Gerador de código (LLVM IR)](src/codegen/)
- [Gramática Refatorada](docs/gramatica_refatorada.md)

## Visão Geral

- DataLang é uma linguagem de programação projetada para facilitar a manipulação e transformação de dados

- Este repositório contém o compilador completo: análise léxica (AFN→AFD), sintática (LL1), semântica e geração de LLVM IR.

---

## Estrutura do Projeto

```bash
DataLang/
├── Makefile              # Build system principal
├── README.md             # Este arquivo
├── bin/                  # Executáveis compilados (gerado)
├── build/                # Arquivos objeto (gerado)
├── docs/                 # Documentação técnica
│   ├── gramatica_formal.md
│   ├── gramatica_refatorada.md
│   └── diagramas/        # AFDs dos tokens
├── examples/             # Programas de exemplo
│   ├── exemplo_01.datalang
│   ├── exemplo_02.datalang
│   └── *.datalang
└── src/
    ├── main.c            # Ponto de entrada
    ├── lexer/            # Analisador léxico
    │   ├── datalang_afn.c
    │   ├── afn_to_afd.c
    │   └── lexer.c
    ├── parser/           # Analisador sintático
    │   ├── parser.c
    │   ├── parser_expr.c
    │   └── parser_main.c
    ├── semantic/         # Análise semântica
    │   ├── semantic_analyzer.c
    │   ├── symbol_table.c
    │   ├── type_system.c
    │   └── type_inference.c
    └── codegen/          # Geração de código
    │   ├── runtime.c
        └── codegen.c

```

---

## Manual de Instalação Completo

### Pré-requisitos

Para compilar e executar programas DataLang, você precisa:

1. **Sistema Operacional**: Linux (nativo ou WSL no Windows)
2. **Compilador C**: GCC
3. **Build Tool**: Make
4. **LLVM/Clang**: Para compilar o IR gerado

---

### Instalação no Windows (via WSL)

#### Passo 1: Instalar o WSL com Ubuntu

1. Abra o PowerShell como Administrador
2. Execute:
   ```powershell
   wsl --install
   ```
3. Reinicie o computador
4. O Ubuntu será instalado automaticamente
5. Crie um usuário e senha quando solicitado

**Nota**: Se o WSL já estiver instalado, instale o Ubuntu:
```powershell
wsl --install -d Ubuntu
```

#### Passo 2: Atualizar o Sistema

Abra o terminal do Ubuntu (WSL) e execute:

```bash
sudo apt update
sudo apt upgrade -y
```

#### Passo 3: Instalar Ferramentas de Desenvolvimento

```bash
# Instala GCC, Make e ferramentas essenciais
sudo apt install -y build-essential make

# Instala o Clang/LLVM (necessário para compilar LLVM IR)
sudo apt install -y clang llvm

# Verifica as instalações
gcc --version
make --version
clang --version
```

Saída esperada:
```
gcc (Ubuntu ...) 11.x.x ou superior
GNU Make 4.3 ou superior
Ubuntu clang version 14.x.x ou superior
```

#### Passo 4: Clonar ou Transferir o Projeto

```bash
cd ~
git clone https://github.com/WillianIsami/DataLang.git
cd DataLang
```

---

### Compilação do Compilador

#### Passo 1: Navegar até o Diretório

```bash
cd ~/DataLang
```

#### Passo 2: Compilar

```bash
make
```

Saída esperada:
```
Compilando src/lexer/datalang_afn.c...
Compilando src/lexer/afn_to_afd.c...
...
Linkando compilador completo...
Compilador criado: bin/datalang
```

Se houver erros, verifique:
- Você está no diretório correto? (`pwd` mostra o caminho atual)
- Os arquivos fonte estão presentes? (`ls src/`)
- O GCC está instalado? (`gcc --version`)

#### Passo 3: Verificar a Compilação

```bash
ls -lh bin/datalang
```

Deve mostrar o executável criado.

---

### Compilando seu Primeiro Programa

#### Passo 1: Ver Exemplos Disponíveis

```bash
ls examples/
```

#### Passo 2: Compilar um Exemplo

O projeto inclui um target `make run` que:
1. Compila um exemplo para LLVM IR
2. Converte o IR em executável nativo
3. Executa o programa

```bash
make run
```

Isso compila `examples/exemplo_01.datalang`.

#### Passo 3: Compilar um Arquivo Específico

```bash
# Compilar para LLVM IR
./bin/datalang examples/exemplo_01.datalang -o output.ll

# Compilar LLVM IR para executável
clang -Wno-override-module output.ll src/codegen/runtime.c -o programa -lm

# Executar
./programa
```

---

### Testando com Seu Próprio Código

#### Passo 1: Criar um Arquivo DataLang

```bash
nano meu_programa.datalang
```

Exemplo de código:
```datalang
let x = 20;
let y = 21;
let texto = "TESTE";
print(x+y);
print(texto);
```

Salve com `Ctrl+O`, `Enter`, `Ctrl+X`.

#### Passo 2: Compilar

```bash
./bin/datalang meu_programa.datalang -o meu_programa.ll
```

Se houver erros de sintaxe ou tipos, serão exibidos aqui.

#### Passo 3: Compilar e Executar

```bash
clang -Wno-override-module meu_programa.ll src/codegen/runtime.c -o meu_programa -lm
./meu_programa
```

---

## Comandos do Makefile

O Makefile oferece vários alvos úteis:

```bash
# Compilar tudo
make

# Limpar arquivos gerados
make clean

# Compilar e executar exemplo padrão
make run

# Compilar exemplo sem executar
make compile-example

# Testar arquivo específico
make test-file FILE=examples/exemplo_completo.datalang

# Recompilar do zero
make rebuild

# Verificar sintaxe sem compilar
make check

# Ver ajuda completa
make help

# Ver versão e componentes
make version
```

---

## Exemplos de Uso

### Exemplo 1: Operações Básicas

```datalang
let x = 42;
let soma = x + 10;
print(soma);
```

### Exemplo 2: Funções

```datalang
fn fatorial(n: Int) -> Int {
    if n <= 1 {
        return 1;
    } else {
        return n * fatorial(n - 1);
    }
}

let resultado = fatorial(5);
print(resultado);
```

### Comandos para testar os exemplos
Execute cada um conforme desejado:
```bash
make clean && make test-file FILE=examples/exemplo_01.datalang
make clean && make test-file FILE=examples/exemplo_02.datalang
make clean && make test-file FILE=examples/exemplo_03.datalang
make clean && make test-file FILE=examples/exemplo_04.datalang
make clean && make test-file FILE=examples/exemplo_05.datalang
make clean && make test-file FILE=examples/exemplo_06.datalang
make clean && make test-file FILE=examples/exemplo_avancado.datalang
make clean && make test-file FILE=examples/exemplo_completo.datalang
make clean && make test-file FILE=examples/exemplo_completo_2.datalang
make clean && make test-file FILE=examples/test_fixes.datalang
make clean && make test-file FILE=examples/teste_erro_global.datalang
make clean && make test-file FILE=examples/teste_erro_local.datalang
make clean && make test-file FILE=examples/teste_erro_retorno.datalang
make clean && make test-file FILE=examples/teste_erro_tipo.datalang
make clean && make test-file FILE=examples/teste_livre.datalang
make clean && make test-file FILE=examples/teste_pipeline.datalang
make clean && make test-file FILE=examples/teste_shadowing.datalang
```

---

## Solução de Problemas

### Erro: "command not found: make"
```bash
sudo apt install build-essential
```

### Erro: "gcc: command not found"
```bash
sudo apt install gcc
```

### Erro: "clang: command not found"
```bash
sudo apt install clang llvm
```

### Erro ao executar ./programa
Verifique se você compilou o LLVM IR:
```bash
clang -Wno-override-module output.ll src/codegen/runtime.c -o programa -lm
```

### Programa compila mas não executa
Verifique permissões:
```bash
chmod +x programa
./programa
```

### Erro "No such file or directory" ao compilar
Verifique se você está no diretório correto:
```bash
pwd  # Deve mostrar o caminho do projeto
ls   # Deve listar Makefile, src/, examples/, etc.
```

---

## Verificação externa com Idris

- Execute o compilador com `--verify` para exportar a AST em JSON (padrão: `ast.json`) e chamar um verificador externo.
- Comandos úteis:
  - `--verify` ou `-V`: habilita a etapa.
  - `--verify-json <arquivo>`: caminho do JSON gerado.
  - `--verify-cmd "<comando>"`: comando de verificação (padrão: `verify/run_verifier.sh`).
- Estrutura sugerida do verificador Idris: veja `verify/README.md`. Compile seu verificador para `verify/datalang_verify` ou passe outro comando via `--verify-cmd`.

Exemplo:
```bash
./bin/datalang examples/exemplo_01.datalang --verify \
  --verify-json /tmp/ast.json \
  --verify-cmd "verify/run_verifier.sh"

# Para compilar o verificador padrão em Idris:
make verify-idris
```

Se o comando do verificador retornar código != 0, a compilação é interrompida antes da geração de LLVM.

---

## Documentação Adicional

- [Gramática Refatorada](docs/gramatica_refatorada.md)
- [Definição da Linguagem](docs/2_definicao_formal_linguagem.md)
- [Expressões Regulares](docs/4_expressoes_regulares_.md)
- [Diagramas AFDs](docs/diagramas/)
- [Manual de utilização](docs/manual_uso.md)
- [Manual de instalação](docs/manual_instalacao.md)
