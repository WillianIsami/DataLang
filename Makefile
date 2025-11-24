# Makefile para DataLang - Compilador Completo com LLVM IR
CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -g
INCLUDES = -I. -Isrc/lexer -Isrc/parser -Isrc/semantic -Isrc/codegen

# Diretórios
LEXER_DIR = src/lexer
PARSER_DIR = src/parser
SEMANTIC_DIR = src/semantic
CODEGEN_DIR = src/codegen
BUILD_DIR = build
BIN_DIR = bin

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

# Objetos
LEXER_OBJECTS = $(patsubst $(LEXER_DIR)/%.c,$(BUILD_DIR)/%.o,$(LEXER_SOURCES))
PARSER_OBJECTS = $(patsubst $(PARSER_DIR)/%.c,$(BUILD_DIR)/%.o,$(PARSER_SOURCES))
SEMANTIC_OBJECTS = $(patsubst $(SEMANTIC_DIR)/%.c,$(BUILD_DIR)/%.o,$(SEMANTIC_SOURCES))
CODEGEN_OBJECTS = $(patsubst $(CODEGEN_DIR)/%.c,$(BUILD_DIR)/%.o,$(CODEGEN_SOURCES))
MAIN_OBJECT = $(BUILD_DIR)/main.o

ALL_OBJECTS = $(MAIN_OBJECT) $(LEXER_OBJECTS) $(PARSER_OBJECTS) $(SEMANTIC_OBJECTS) $(CODEGEN_OBJECTS)

# Executável
COMPILER = $(BIN_DIR)/datalang

.PHONY: all clean directories test help run compile-example

all: directories $(COMPILER)

# Compilador completo
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
	@rm -f AST.json *.ll *.s *.o programa
	@echo "✓ Limpeza concluída"

# ==================== TESTES ====================

# Compila e executa exemplo
run: $(COMPILER)
	@if [ -f "examples/exemplo_01.datalang" ]; then \
		echo "\n╔════════════════════════════════════════════════════════════╗"; \
		echo "║           COMPILANDO EXEMPLO                              ║"; \
		echo "╚════════════════════════════════════════════════════════════╝\n"; \
		$(COMPILER) examples/exemplo_01.datalang -o output.ll; \
		if [ -f "output.ll" ]; then \
			echo "\n╔════════════════════════════════════════════════════════════╗"; \
			echo "║           COMPILANDO LLVM IR → EXECUTÁVEL                 ║"; \
			echo "╚════════════════════════════════════════════════════════════╝\n"; \
			clang output.ll -o programa; \
			echo "\n╔════════════════════════════════════════════════════════════╗"; \
			echo "║           EXECUTANDO PROGRAMA                             ║"; \
			echo "╚════════════════════════════════════════════════════════════╝\n"; \
			./programa; \
		fi \
	else \
		echo "❌ Arquivo de exemplo não encontrado: examples/exemplo_01.datalang"; \
	fi

# Compila exemplo sem executar
compile-example: $(COMPILER)
	@if [ -f "examples/exemplo_01.datalang" ]; then \
		$(COMPILER) examples/exemplo_01.datalang -o output.ll; \
		echo "\n✓ LLVM IR gerado em: output.ll"; \
		echo "\nPara compilar e executar:"; \
		echo "  clang output.ll -o programa && ./programa"; \
	else \
		echo "❌ Arquivo não encontrado: examples/exemplo_01.datalang"; \
	fi

# Teste com arquivo específico
test-file: $(COMPILER)
	@if [ -z "$(FILE)" ]; then \
		echo "❌ Uso: make test-file FILE=examples/exemplo_01.datalang"; \
	else \
		echo "\n╔════════════════════════════════════════════════════════════╗"; \
		echo "║           COMPILANDO: $(FILE)"; \
		echo "╚════════════════════════════════════════════════════════════╝\n"; \
		$(COMPILER) $(FILE) -o output.ll; \
		if [ -f "output.ll" ]; then \
			echo "\nCompilando para executável..."; \
			clang output.ll -o programa; \
			echo "\nExecutando..."; \
			./programa; \
		fi \
	fi

# ==================== DESENVOLVIMENTO ====================

rebuild: clean all

check:
	@echo "🔍 Verificando sintaxe..."
	$(CC) $(CFLAGS) $(INCLUDES) -fsyntax-only $(LEXER_SOURCES) $(PARSER_SOURCES) $(SEMANTIC_SOURCES) $(CODEGEN_SOURCES)
	@echo "✓ Sintaxe verificada"

# ==================== INFORMAÇÕES ====================

help:
	@echo "╔════════════════════════════════════════════════════════════╗"
	@echo "║              MAKEFILE DATALANG - AJUDA                    ║"
	@echo "╚════════════════════════════════════════════════════════════╝"
	@echo ""
	@echo "Alvos disponíveis:"
	@echo ""
	@echo "  all              - Compila o compilador completo (padrão)"
	@echo "  clean            - Remove arquivos compilados"
	@echo "  run              - Compila e executa exemplo padrão"
	@echo "  compile-example  - Apenas compila exemplo (sem executar)"
	@echo "  test-file        - Testa arquivo: make test-file FILE=<arquivo>"
	@echo "  rebuild          - Recompila tudo do zero"
	@echo "  check            - Verifica sintaxe sem compilar"
	@echo "  help             - Mostra esta ajuda"
	@echo ""
	@echo "Estrutura do projeto:"
	@echo "  src/lexer/       - Analisador léxico (AFN/AFD)"
	@echo "  src/parser/      - Analisador sintático (LL1)"
	@echo "  src/semantic/    - Analisador semântico e inferência"
	@echo "  src/codegen/     - Gerador de código LLVM IR"
	@echo "  examples/        - Exemplos de código DataLang"
	@echo ""
	@echo "Pipeline completo:"
	@echo "  Código → Léxico → Sintático → Semântico → LLVM IR → Executável"
	@echo ""

version:
	@echo "DataLang Compiler v1.0.0"
	@echo "Componentes:"
	@echo "  ✓ Analisador Léxico (AFN → AFD)"
	@echo "  ✓ Analisador Sintático (LL1 Recursivo Descendente)"
	@echo "  ✓ Analisador Semântico (Tabela de Símbolos + Inferência)"
	@echo "  ✓ Gerador de Código (LLVM IR)"
	@echo ""
	@echo "Compilador: $(CC)"
	@echo "Flags: $(CFLAGS)"

.PHONY: version