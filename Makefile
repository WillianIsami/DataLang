# Makefile para DataLang - Compilador Completo
# Inclui: Léxico, Sintático e Semântico

CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -g
INCLUDES = -I. -Isrc/lexer -Isrc/parser -Isrc/semantic

# Diretórios
LEXER_DIR = src/lexer
PARSER_DIR = src/parser
SEMANTIC_DIR = src/semantic
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

MAIN_SOURCE = src/main.c

# Objetos
LEXER_OBJECTS = $(patsubst $(LEXER_DIR)/%.c,$(BUILD_DIR)/%.o,$(LEXER_SOURCES))
PARSER_OBJECTS = $(patsubst $(PARSER_DIR)/%.c,$(BUILD_DIR)/%.o,$(PARSER_SOURCES))
SEMANTIC_OBJECTS = $(patsubst $(SEMANTIC_DIR)/%.c,$(BUILD_DIR)/%.o,$(SEMANTIC_SOURCES))
MAIN_OBJECT = $(BUILD_DIR)/main.o

ALL_OBJECTS = $(MAIN_OBJECT) $(LEXER_OBJECTS) $(PARSER_OBJECTS) $(SEMANTIC_OBJECTS)

# Executáveis
COMPILER = $(BIN_DIR)/datalang
TEST_SEMANTIC = $(BIN_DIR)/test_semantic

# ==================== ALVOS PRINCIPAIS ====================

.PHONY: all clean directories test help

all: directories $(COMPILER) $(TEST_SEMANTIC)

# Compilador completo
$(COMPILER): $(ALL_OBJECTS)
	@echo "🔗 Linkando compilador completo..."
	@mkdir -p $(BIN_DIR)
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ $^
	@echo "✓ Compilador criado: $(COMPILER)"

# Teste do analisador semântico
$(TEST_SEMANTIC): $(BUILD_DIR)/test_semantic.o $(ALL_OBJECTS)
	@echo "🔗 Linkando teste semântico..."
	@mkdir -p $(BIN_DIR)
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ $^
	@echo "✓ Teste criado: $(TEST_SEMANTIC)"

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

# Testes
$(BUILD_DIR)/test_semantic.o: tests/test_semantic.c
	@echo "📦 Compilando teste semântico..."
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# Main
$(BUILD_DIR)/main.o: $(MAIN_SOURCE)
	@echo "📦 Compilando $<..."
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# ==================== DIRETÓRIOS ====================

directories:
	@mkdir -p $(BUILD_DIR) $(BIN_DIR)

# ==================== LIMPEZA ====================

clean:
	@echo "🧹 Limpando arquivos compilados..."
	@rm -rf $(BUILD_DIR) $(BIN_DIR)
	@rm -f AST.json
	@echo "✓ Limpeza concluída"

# ==================== TESTES ====================

test: $(TEST_SEMANTIC)
	@echo "\n╔════════════════════════════════════════════════════════════╗"
	@echo "║           EXECUTANDO TESTES SEMÂNTICOS                    ║"
	@echo "╚════════════════════════════════════════════════════════════╝\n"
	@$(TEST_SEMANTIC)

# Teste com arquivo específico
test-file: $(COMPILER)
	@if [ -z "$(FILE)" ]; then \
		echo "❌ Uso: make test-file FILE=examples/exemplo.datalang"; \
	else \
		echo "\n╔════════════════════════════════════════════════════════════╗"; \
		echo "║           COMPILANDO: $(FILE)"; \
		echo "╚════════════════════════════════════════════════════════════╝\n"; \
		$(COMPILER) $(FILE); \
	fi

# ==================== EXECUÇÃO ====================

run: $(COMPILER)
	@if [ -f "examples/exemplo.datalang" ]; then \
		$(COMPILER) examples/exemplo.datalang; \
	else \
		echo "❌ Arquivo de exemplo não encontrado: examples/exemplo.datalang"; \
	fi

# ==================== DESENVOLVIMENTO ====================

# Recompila tudo do zero
rebuild: clean all

# Compila apenas o analisador semântico
semantic: $(SEMANTIC_OBJECTS)
	@echo "✓ Módulos semânticos compilados"

# Verifica sintaxe sem compilar
check:
	@echo "🔍 Verificando sintaxe..."
	$(CC) $(CFLAGS) $(INCLUDES) -fsyntax-only $(LEXER_SOURCES) $(PARSER_SOURCES) $(SEMANTIC_SOURCES)
	@echo "✓ Sintaxe verificada"

# ==================== INFORMAÇÕES ====================

help:
	@echo "╔════════════════════════════════════════════════════════════╗"
	@echo "║              MAKEFILE DATALANG - AJUDA                    ║"
	@echo "╚════════════════════════════════════════════════════════════╝"
	@echo ""
	@echo "Alvos disponíveis:"
	@echo ""
	@echo "  all          - Compila o compilador completo (padrão)"
	@echo "  clean        - Remove arquivos compilados"
	@echo "  test         - Executa testes do analisador semântico"
	@echo "  test-file    - Testa com arquivo: make test-file FILE=<arquivo>"
	@echo "  run          - Executa com arquivo de exemplo padrão"
	@echo "  rebuild      - Recompila tudo do zero"
	@echo "  check        - Verifica sintaxe sem compilar"
	@echo "  help         - Mostra esta ajuda"
	@echo ""
	@echo "Estrutura do projeto:"
	@echo "  src/lexer/       - Analisador léxico (AFN/AFD)"
	@echo "  src/parser/      - Analisador sintático (LL1)"
	@echo "  src/semantic/    - Analisador semântico e inferência"
	@echo "  examples/        - Exemplos de código DataLang"
	@echo "  tests/           - Testes unitários"
	@echo ""
	@echo "Exemplo de uso:"
	@echo "  make                              # Compila tudo"
	@echo "  make test                         # Executa testes"
	@echo "  make test-file FILE=exemplo.dl    # Testa arquivo específico"
	@echo ""

# ==================== INFORMAÇÕES DE VERSÃO ====================

version:
	@echo "DataLang Compiler v0.3.0"
	@echo "Componentes:"
	@echo "  ✓ Analisador Léxico (AFN → AFD)"
	@echo "  ✓ Analisador Sintático (LL1 Recursivo Descendente)"
	@echo "  ✓ Analisador Semântico (Tabela de Símbolos + Inferência de Tipos)"
	@echo ""
	@echo "Compilador: $(CC)"
	@echo "Flags: $(CFLAGS)"

.PHONY: version semantic check rebuild help