# Makefile para DataLang
# Analisador Léxico com AFNs e Tratamento de Erros

# Compilador e flags
CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -O2 -g
LDFLAGS = -lm

# Diretórios
SRC_DIR = src/lexer
BUILD_DIR = build
BIN_DIR = bin
TEST_DIR = tests

# Arquivos
DATALANG_AFN = datalang_afn.c
LEXER_SRC = datalang_lexer_integrated.c
TEST_AFN = test_afn_conversion.c
LEGACY_LEXER = datalang_lexer.c
LEGACY_TEST = datalang_test.c

# Executáveis
LEXER_BIN = $(BIN_DIR)/datalang_lexer
TEST_AFN_BIN = $(BIN_DIR)/test_afn
LEGACY_BIN = $(BIN_DIR)/datalang_legacy

# Cores para output
RED = \033[0;31m
GREEN = \033[0;32m
YELLOW = \033[1;33m
BLUE = \033[0;34m
NC = \033[0m # No Color

# Regras principais
.PHONY: all clean test run help setup legacy

all: setup $(LEXER_BIN) $(TEST_AFN_BIN)
	@echo "$(GREEN)✓ Compilação concluída com sucesso!$(NC)"

# Cria diretórios necessários
setup:
	@echo "$(BLUE)Criando estrutura de diretórios...$(NC)"
	@mkdir -p $(BUILD_DIR)
	@mkdir -p $(BIN_DIR)
	@mkdir -p $(TEST_DIR)

# Compila analisador léxico integrado
$(LEXER_BIN): $(LEXER_SRC) $(DATALANG_AFN)
	@echo "$(YELLOW)Compilando analisador léxico integrado...$(NC)"
	$(CC) $(CFLAGS) -o $@ $(LEXER_SRC) $(LDFLAGS)
	@echo "$(GREEN)✓ Analisador léxico compilado$(NC)"

# Compila testes de conversão AFN→AFD
$(TEST_AFN_BIN): $(TEST_AFN)
	@echo "$(YELLOW)Compilando testes de AFN...$(NC)"
	$(CC) $(CFLAGS) -o $@ $(TEST_AFN) $(LDFLAGS)
	@echo "$(GREEN)✓ Testes de AFN compilados$(NC)"

# Compila versão legada (compatibilidade)
legacy: setup $(LEGACY_LEXER) $(LEGACY_TEST)
	@echo "$(YELLOW)Compilando versão legada...$(NC)"
	$(CC) $(CFLAGS) -o $(LEGACY_BIN) $(LEGACY_LEXER) $(LDFLAGS)
	@echo "$(GREEN)✓ Versão legada compilada$(NC)"

# Executa o analisador léxico
run: $(LEXER_BIN)
	@echo "$(BLUE)════════════════════════════════════════════════$(NC)"
	@echo "$(BLUE)    DATALANG - ANALISADOR LÉXICO INTEGRADO    $(NC)"
	@echo "$(BLUE)════════════════════════════════════════════════$(NC)"
	@./$(LEXER_BIN)

# Executa testes de AFN
test-afn: $(TEST_AFN_BIN)
	@echo "$(BLUE)════════════════════════════════════════════════$(NC)"
	@echo "$(BLUE)       TESTES DE CONVERSÃO AFN → AFD          $(NC)"
	@echo "$(BLUE)════════════════════════════════════════════════$(NC)"
	@./$(TEST_AFN_BIN)

# Executa todos os testes
test: $(LEXER_BIN) $(TEST_AFN_BIN)
	@echo "$(BLUE)════════════════════════════════════════════════$(NC)"
	@echo "$(BLUE)         EXECUTANDO SUITE COMPLETA             $(NC)"
	@echo "$(BLUE)════════════════════════════════════════════════$(NC)"
	@echo ""
	@echo "$(YELLOW)1/2 - Testes do Analisador Léxico$(NC)"
	@./$(LEXER_BIN)
	@echo ""
	@echo "$(YELLOW)2/2 - Testes de Conversão AFN$(NC)"
	@./$(TEST_AFN_BIN)
	@echo ""
	@echo "$(GREEN)✓ Todos os testes concluídos!$(NC)"

# Testa com arquivo de entrada customizado
test-file: $(LEXER_BIN)
	@if [ -z "$(FILE)" ]; then \
		echo "$(RED)Erro: Especifique um arquivo com FILE=caminho$(NC)"; \
		exit 1; \
	fi
	@echo "$(BLUE)Analisando arquivo: $(FILE)$(NC)"
	@./$(LEXER_BIN) $(FILE)

# Benchmark de performance
benchmark: $(LEXER_BIN)
	@echo "$(BLUE)════════════════════════════════════════════════$(NC)"
	@echo "$(BLUE)           BENCHMARK DE PERFORMANCE            $(NC)"
	@echo "$(BLUE)════════════════════════════════════════════════$(NC)"
	@time ./$(LEXER_BIN)
	@echo "$(GREEN)✓ Benchmark concluído$(NC)"

# Análise de memória com Valgrind (se disponível)
memcheck: $(LEXER_BIN)
	@echo "$(BLUE)Executando análise de memória com Valgrind...$(NC)"
	@if command -v valgrind >/dev/null 2>&1; then \
		valgrind --leak-check=full --show-leak-kinds=all \
		--track-origins=yes --verbose \
		./$(LEXER_BIN); \
	else \
		echo "$(RED)Valgrind não encontrado$(NC)"; \
	fi

# Análise estática com cppcheck (se disponível)
static-analysis:
	@echo "$(BLUE)Executando análise estática com cppcheck...$(NC)"
	@if command -v cppcheck >/dev/null 2>&1; then \
		cppcheck --enable=all --suppress=missingIncludeSystem \
		--std=c11 $(LEXER_SRC); \
	else \
		echo "$(RED)cppcheck não encontrado$(NC)"; \
	fi

# Gera documentação
docs:
	@echo "$(BLUE)Gerando documentação...$(NC)"
	@if command -v doxygen >/dev/null 2>&1; then \
		doxygen Doxyfile; \
		echo "$(GREEN)✓ Documentação gerada em docs/$(NC)"; \
	else \
		echo "$(RED)doxygen não encontrado$(NC)"; \
	fi

# Limpa arquivos compilados
clean:
	@echo "$(YELLOW)Limpando arquivos compilados...$(NC)"
	@rm -rf $(BUILD_DIR)
	@rm -rf $(BIN_DIR)
	@echo "$(GREEN)✓ Limpeza concluída$(NC)"

# Limpa tudo incluindo documentação
distclean: clean
	@echo "$(YELLOW)Limpeza completa...$(NC)"
	@rm -rf docs/html docs/latex
	@echo "$(GREEN)✓ Limpeza completa concluída$(NC)"

# Instala dependências (Ubuntu/Debian)
install-deps:
	@echo "$(BLUE)Instalando dependências...$(NC)"
	@sudo apt-get update
	@sudo apt-get install -y gcc make valgrind cppcheck doxygen graphviz
	@echo "$(GREEN)✓ Dependências instaladas$(NC)"

# Mostra informações do sistema
info:
	@echo "$(BLUE)════════════════════════════════════════════════$(NC)"
	@echo "$(BLUE)           INFORMAÇÕES DO SISTEMA              $(NC)"
	@echo "$(BLUE)════════════════════════════════════════════════$(NC)"
	@echo "Compilador: $(CC)"
	@$(CC) --version | head -n 1
	@echo ""
	@echo "Flags de compilação: $(CFLAGS)"
	@echo ""
	@echo "Diretórios:"
	@echo "  - Código fonte: $(SRC_DIR)"
	@echo "  - Build: $(BUILD_DIR)"
	@echo "  - Binários: $(BIN_DIR)"
	@echo ""
	@echo "Executáveis:"
	@echo "  - Lexer integrado: $(LEXER_BIN)"
	@echo "  - Testes AFN: $(TEST_AFN_BIN)"

# Ajuda
help:
	@echo "$(BLUE)════════════════════════════════════════════════$(NC)"
	@echo "$(BLUE)         DATALANG - MAKEFILE HELP              $(NC)"
	@echo "$(BLUE)════════════════════════════════════════════════$(NC)"
	@echo ""
	@echo "$(YELLOW)Comandos de Compilação:$(NC)"
	@echo "  make              - Compila todos os componentes"
	@echo "  make legacy       - Compila versão legada"
	@echo "  make clean        - Remove arquivos compilados"
	@echo "  make distclean    - Limpeza completa"
	@echo ""
	@echo "$(YELLOW)Comandos de Teste:$(NC)"
	@echo "  make run          - Executa analisador léxico"
	@echo "  make test         - Executa todos os testes"
	@echo "  make test-afn     - Testa conversão AFN→AFD"
	@echo "  make test-file FILE=<arquivo> - Testa arquivo específico"
	@echo ""
	@echo "$(YELLOW)Comandos de Análise:$(NC)"
	@echo "  make benchmark    - Benchmark de performance"
	@echo "  make memcheck     - Análise de memória (Valgrind)"
	@echo "  make static-analysis - Análise estática (cppcheck)"
	@echo ""
	@echo "$(YELLOW)Outros Comandos:$(NC)"
	@echo "  make docs         - Gera documentação"
	@echo "  make info         - Mostra informações do sistema"
	@echo "  make install-deps - Instala dependências (Ubuntu/Debian)"
	@echo "  make help         - Mostra esta ajuda"
	@echo ""
	@echo "$(GREEN)Exemplo de uso:$(NC)"
	@echo "  make && make test"
	@echo ""

# Regra padrão
.DEFAULT_GOAL := help