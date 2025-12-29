# Makefile para DataLang - Compilador Completo com LLVM IR + Runtime
CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -g
INCLUDES = -I. -Isrc/lexer -Isrc/parser -Isrc/semantic -Isrc/codegen

# Ferramentas LLVM
LLVM_AS = llvm-as
LLVM_LINK = llvm-link
LLC = llc
CLANG = clang
CLANG_FLAGS = -Wno-override-module

# Diretórios
LEXER_DIR = src/lexer
PARSER_DIR = src/parser
SEMANTIC_DIR = src/semantic
CODEGEN_DIR = src/codegen
BUILD_DIR = build
BIN_DIR = bin

# Exemplo padrão para os comandos de atalho
DEFAULT_EXAMPLE = examples/exemplo_avancado.datalang

# Arquivos fonte
LEXER_SOURCES = $(LEXER_DIR)/datalang_afn.c \
                $(LEXER_DIR)/afn_to_afd.c \
                $(LEXER_DIR)/lexer.c

PARSER_SOURCES = $(PARSER_DIR)/parser.c \
                 $(PARSER_DIR)/parser_expr.c \
                 $(PARSER_DIR)/parser_main.c

SEMANTIC_SOURCES = $(SEMANTIC_DIR)/symbol_table.c \
                   $(SEMANTIC_DIR)/type_system.c \
                   $(SEMANTIC_DIR)/type_inference.c \
                   $(SEMANTIC_DIR)/semantic_analyzer.c

CODEGEN_SOURCES = $(CODEGEN_DIR)/codegen.c

MAIN_SOURCE = src/main.c

# Runtime (agora compilamos o source diretamente com clang)
RUNTIME_SOURCE = $(CODEGEN_DIR)/runtime.c

# Objetos
LEXER_OBJECTS = $(patsubst $(LEXER_DIR)/%.c,$(BUILD_DIR)/%.o,$(LEXER_SOURCES))
PARSER_OBJECTS = $(patsubst $(PARSER_DIR)/%.c,$(BUILD_DIR)/%.o,$(PARSER_SOURCES))
SEMANTIC_OBJECTS = $(patsubst $(SEMANTIC_DIR)/%.c,$(BUILD_DIR)/%.o,$(SEMANTIC_SOURCES))
CODEGEN_OBJECTS = $(patsubst $(CODEGEN_DIR)/%.c,$(BUILD_DIR)/%.o,$(CODEGEN_SOURCES))
MAIN_OBJECT = $(BUILD_DIR)/main.o

ALL_OBJECTS = $(MAIN_OBJECT) $(LEXER_OBJECTS) $(PARSER_OBJECTS) $(SEMANTIC_OBJECTS) $(CODEGEN_OBJECTS)

# Executável
COMPILER = $(BIN_DIR)/datalang

.PHONY: all clean directories test help run compile-example test-file

all: directories $(COMPILER)
	@echo ""
	@echo "╔════════════════════════════════════════════════════════════╗"
	@echo "║  ✓ COMPILADOR DATALANG CRIADO COM SUCESSO                ║"
	@echo "╚════════════════════════════════════════════════════════════╝"
	@echo ""
	@echo "Componentes compilados:"
	@echo "  ✓ Compilador: $(COMPILER)"
	@echo "  ✓ Runtime:    $(RUNTIME_SOURCE)"
	@echo ""

# ==================== COMPILADOR ====================

$(COMPILER): $(ALL_OBJECTS)
	@echo "🔗 Linkando compilador completo..."
	@mkdir -p $(BIN_DIR)
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ $^
	@echo "✓ Compilador criado: $(COMPILER)"

# ==================== COMPILAÇÃO DE OBJETOS ====================

# Lexer
$(BUILD_DIR)/%.o: $(LEXER_DIR)/%.c
	@echo "📦 Compilando $<..."
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# Parser
$(BUILD_DIR)/%.o: $(PARSER_DIR)/%.c
	@echo "📦 Compilando $<..."
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# Semantic
$(BUILD_DIR)/%.o: $(SEMANTIC_DIR)/%.c
	@echo "📦 Compilando $<..."
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# CodeGen
$(BUILD_DIR)/%.o: $(CODEGEN_DIR)/%.c
	@echo "📦 Compilando $<..."
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# Main
$(BUILD_DIR)/main.o: $(MAIN_SOURCE)
	@echo "📦 Compilando $<..."
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# ==================== DIRETÓRIOS ====================

directories:
	@mkdir -p $(BUILD_DIR) $(BIN_DIR) $(CODEGEN_DIR)

# ==================== LIMPEZA ====================

clean:
	@echo "🧹 Limpando arquivos compilados..."
	@rm -rf $(BUILD_DIR) $(BIN_DIR)
	@rm -f AST.json *.ll *.s *.o *.bc programa output test_output.csv
	@echo "✓ Limpeza concluída"

# ==================== COMPILAÇÃO E EXECUÇÃO ====================

# Compila DataLang → LLVM IR → Executável com Runtime
run: $(COMPILER)
	@if [ -f "$(DEFAULT_EXAMPLE)" ]; then \
		echo ""; \
		echo "╔════════════════════════════════════════════════════════════╗"; \
		echo "║         ETAPA 1: COMPILANDO DATALANG → LLVM IR           ║"; \
		echo "╚════════════════════════════════════════════════════════════╝"; \
		echo ""; \
		$(COMPILER) $(DEFAULT_EXAMPLE) -o output.ll; \
		if [ -f "output.ll" ]; then \
			echo ""; \
			echo "╔════════════════════════════════════════════════════════════╗"; \
			echo "║         ETAPA 2: LINKANDO COM RUNTIME                    ║"; \
			echo "╚════════════════════════════════════════════════════════════╝"; \
			echo ""; \
			$(CLANG) $(CLANG_FLAGS) output.ll $(RUNTIME_SOURCE) -o programa -lm; \
			echo "✓ Executável criado: ./programa"; \
			echo ""; \
			echo "╔════════════════════════════════════════════════════════════╗"; \
			echo "║         ETAPA 3: EXECUTANDO PROGRAMA                     ║"; \
			echo "╚════════════════════════════════════════════════════════════╝"; \
			echo ""; \
			./programa; \
			RETCODE=$$?; \
			echo ""; \
			echo "╔════════════════════════════════════════════════════════════╗"; \
			echo "║         EXECUÇÃO FINALIZADA                              ║"; \
			echo "╚════════════════════════════════════════════════════════════╝"; \
			echo "Código de retorno: $$RETCODE"; \
			echo ""; \
		fi \
	else \
		echo "❌ Arquivo de exemplo não encontrado: $(DEFAULT_EXAMPLE)"; \
	fi

# Compila exemplo sem executar
compile-example: $(COMPILER)
	@if [ -f "$(DEFAULT_EXAMPLE)" ]; then \
		echo "Compilando DataLang → LLVM IR..."; \
		$(COMPILER) $(DEFAULT_EXAMPLE) -o output.ll; \
		echo ""; \
		echo "✓ LLVM IR gerado em: output.ll"; \
		echo "✓ Runtime disponível em: $(RUNTIME_SOURCE)"; \
		echo ""; \
		echo "Para compilar e executar:"; \
		echo "  $(CLANG) $(CLANG_FLAGS) output.ll $(RUNTIME_SOURCE) -o programa -lm"; \
		echo "  ./programa"; \
		echo ""; \
	else \
		echo "❌ Arquivo não encontrado: $(DEFAULT_EXAMPLE)"; \
	fi

# Teste com arquivo específico
test-file: $(COMPILER)
	@if [ -z "$(FILE)" ]; then \
		echo "❌ Uso: make test-file FILE=$(DEFAULT_EXAMPLE)"; \
	else \
		echo ""; \
		echo "╔════════════════════════════════════════════════════════════╗"; \
		echo "║  COMPILANDO: $(FILE)"; \
		echo "╚════════════════════════════════════════════════════════════╝"; \
		echo ""; \
		$(COMPILER) $(FILE) -o output.ll; \
		if [ -f "output.ll" ]; then \
			echo "Linkando com runtime..."; \
			$(CLANG) $(CLANG_FLAGS) output.ll $(RUNTIME_SOURCE) -o programa -lm; \
			echo ""; \
			echo "Executando..."; \
			echo ""; \
			./programa; \
		fi \
	fi

# ==================== TESTES ESPECÍFICOS ====================

# Teste rápido sem runtime (apenas gera IR)
test-ir: $(COMPILER)
	@if [ -z "$(FILE)" ]; then \
		echo "❌ Uso: make test-ir FILE=$(DEFAULT_EXAMPLE)"; \
	else \
		$(COMPILER) $(FILE) -o output.ll; \
		echo "✓ LLVM IR gerado em output.ll"; \
		echo ""; \
		echo "Para visualizar:"; \
		echo "  cat output.ll"; \
	fi

# Teste completo com validação
test-validate: $(COMPILER)
	@if [ -z "$(FILE)" ]; then \
		echo "❌ Uso: make test-validate FILE=$(DEFAULT_EXAMPLE)"; \
	else \
		echo "1. Compilando DataLang..."; \
		$(COMPILER) $(FILE) -o output.ll || exit 1; \
		echo "✓ LLVM IR gerado"; \
		echo ""; \
		echo "2. Validando LLVM IR..."; \
		$(LLVM_AS) output.ll -o output.bc || exit 1; \
		echo "✓ LLVM IR válido"; \
		echo ""; \
		echo "3. Linkando com runtime..."; \
		$(CLANG) $(CLANG_FLAGS) output.ll $(RUNTIME_SOURCE) -o programa -lm || exit 1; \
		echo "✓ Executável criado"; \
		echo ""; \
		echo "4. Executando programa..."; \
		./programa; \
		echo ""; \
		echo "✓ Teste completo finalizado"; \
	fi

# Teste com CSV
test-csv: $(COMPILER)
	@echo "Preparando teste CSV..."
	@echo "name,age,city" > test_input.csv
	@echo "Alice,30,New York" >> test_input.csv
	@echo "Bob,25,Los Angeles" >> test_input.csv
	@echo "Charlie,35,Chicago" >> test_input.csv
	@echo "✓ test_input.csv criado"
	@echo ""
	@echo "Compilando código de teste..."
	@echo 'let df = load("test_input.csv");' > test_csv.datalang
	@echo 'print("DataFrame carregado!");' >> test_csv.datalang
	@echo 'save(df, "test_output.csv");' >> test_csv.datalang
	@echo 'print("DataFrame salvo!");' >> test_csv.datalang
	@$(COMPILER) test_csv.datalang -o output.ll
	@echo ""
	@echo "Linkando e executando..."
	@echo "$(CLANG) $(CLANG_FLAGS) output.ll $(RUNTIME_SOURCE) -o programa -lm"
	@$(CLANG) $(CLANG_FLAGS) output.ll $(RUNTIME_SOURCE) -o programa -lm
	@./programa
	@echo ""
	@echo "Verificando arquivo de saída..."
	@if [ -f "test_output.csv" ]; then \
		echo "✓ test_output.csv gerado com sucesso:"; \
		cat test_output.csv; \
	else \
		echo "❌ test_output.csv não foi gerado"; \
	fi

# ==================== DESENVOLVIMENTO ====================

rebuild: clean all

check:
	@echo "🔍 Verificando sintaxe..."
	$(CC) $(CFLAGS) $(INCLUDES) -fsyntax-only $(LEXER_SOURCES) $(PARSER_SOURCES) $(SEMANTIC_SOURCES) $(CODEGEN_SOURCES) $(RUNTIME_SOURCE)
	@echo "✓ Sintaxe verificada"

# Compila apenas o compilador
compiler-only: directories $(COMPILER)
	@echo "✓ Compilador criado"

# ==================== DEBUG ====================

debug: CFLAGS += -DDEBUG -O0
debug: clean all
	@echo "✓ Build de debug criado"

# Executa com valgrind para detectar leaks
valgrind: $(COMPILER)
	@if [ -f "$(DEFAULT_EXAMPLE)" ]; then \
		$(COMPILER) $(DEFAULT_EXAMPLE) -o output.ll; \
		$(CLANG) $(CLANG_FLAGS) output.ll $(RUNTIME_SOURCE) -o programa -lm; \
		valgrind --leak-check=full --show-leak-kinds=all ./programa; \
	fi

# Mostra IR gerado
show-ir: compile-example
	@if [ -f "output.ll" ]; then \
		echo ""; \
		echo "╔════════════════════════════════════════════════════════════╗"; \
		echo "║              LLVM IR GERADO                               ║"; \
		echo "╚════════════════════════════════════════════════════════════╝"; \
		echo ""; \
		cat output.ll; \
	fi

# ==================== INFORMAÇÕES ====================

help:
	@echo "╔════════════════════════════════════════════════════════════╗"
	@echo "║              MAKEFILE DATALANG - AJUDA                    ║"
	@echo "╚════════════════════════════════════════════════════════════╝"
	@echo ""
	@echo "COMPILAÇÃO:"
	@echo "  all              - Compila compilador completo (padrão)"
	@echo "  compiler-only    - Compila apenas o compilador"
	@echo "  rebuild          - Recompila tudo do zero"
	@echo "  clean            - Remove arquivos compilados"
	@echo ""
	@echo "EXECUÇÃO:"
	@echo "  run              - Compila e executa exemplo padrão"
	@echo "  compile-example  - Apenas compila exemplo (sem executar)"
	@echo "  test-file        - Testa arquivo: make test-file FILE=<arquivo>"
	@echo "  test-ir          - Gera apenas IR: make test-ir FILE=<arquivo>"
	@echo "  test-validate    - Teste completo com validação"
	@echo "  test-csv         - Teste completo de load/save CSV"
	@echo ""
	@echo "DEBUG:"
	@echo "  debug            - Build com símbolos de debug"
	@echo "  valgrind         - Executa com valgrind"
	@echo "  show-ir          - Mostra LLVM IR gerado"
	@echo "  check            - Verifica sintaxe sem compilar"
	@echo ""
	@echo "INFORMAÇÕES:"
	@echo "  help             - Mostra esta ajuda"
	@echo "  version          - Mostra versão e componentes"
	@echo ""
	@echo "ESTRUTURA:"
	@echo "  src/lexer/       - Analisador léxico (AFN/AFD)"
	@echo "  src/parser/      - Analisador sintático (LL1)"
	@echo "  src/semantic/    - Analisador semântico e inferência"
	@echo "  src/codegen/     - Gerador de código LLVM IR + Runtime"
	@echo "  examples/        - Exemplos de código DataLang"
	@echo ""
	@echo "PIPELINE COMPLETO:"
	@echo "  DataLang → Léxico → Sintático → Semântico → LLVM IR"
	@echo "           ↓"
	@echo "  LLVM IR + Runtime.c → Clang → Executável"
	@echo ""
	@echo "EXEMPLOS:"
	@echo "  make run                                      # Compila e roda exemplo padrão"
	@echo "  make test-file FILE=examples/teste.datalang   # Testa arquivo específico"
	@echo "  make test-csv                                 # Teste completo CSV"
	@echo "  make test-validate FILE=examples/teste.datalang # Teste com validação"
	@echo ""

version:
	@echo "╔════════════════════════════════════════════════════════════╗"
	@echo "║              DATALANG COMPILER v1.0.0                     ║"
	@echo "╚════════════════════════════════════════════════════════════╝"
	@echo ""
	@echo "Componentes:"
	@echo "  ✓ Analisador Léxico (AFN → AFD)"
	@echo "  ✓ Analisador Sintático (LL1 Recursivo Descendente)"
	@echo "  ✓ Analisador Semântico (Tabela de Símbolos + Inferência)"
	@echo "  ✓ Gerador de Código (LLVM IR)"
	@echo "  ✓ Runtime (Strings, DataFrames, CSV I/O)"
	@echo ""
	@echo "Ferramentas:"
	@echo "  Compilador C: $(CC) $(CFLAGS)"
	@echo "  LLVM:         $(CLANG) $(CLANG_FLAGS)"
	@echo ""
	@echo "Estrutura de Build:"
	@echo "  Compilador:   $(COMPILER)"
	@echo "  Runtime:      $(RUNTIME_SOURCE)"
	@echo "  Build Dir:    $(BUILD_DIR)/"
	@echo "  Binários:     $(BIN_DIR)/"
	@echo ""

# ==================== ATALHOS ÚTEIS ====================

# Compila e roda em um comando
cr: clean run

# Teste rápido
quick: compile-example
	@$(CLANG) $(CLANG_FLAGS) output.ll $(LINK_OBJECTS) -o programa -lm && ./programa

verify-idris:
	@echo "🔨 Compilando verificador Idris (verify/datalang_verify)..."
	@if command -v idris2 >/dev/null 2>&1; then \
		cd verify && idris2 --build datalang_verify.ipkg; \
	else \
		echo "❌ idris2 não encontrado. Instale ou aponte outro comando com --verify-cmd no compilador."; \
	fi

.PHONY: version cr quick debug valgrind show-ir compiler-only test-ir test-validate test-csv verify-idris
