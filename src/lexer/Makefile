# Makefile para DataLang AFDs
CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -pedantic -g
LDFLAGS = 

# Arquivos fonte
LEXER_SRC = datalang_lexer.c
TEST_SRC = datalang_test.c

# Arquivos objeto
LEXER_OBJ = $(LEXER_SRC:.c=.o)
LEXER_LIB_OBJ = datalang_lexer_lib.o
TEST_OBJ = $(TEST_SRC:.c=.o)

# Executáveis
LEXER_EXEC = datalang_lexer
TEST_EXEC = datalang_test

.PHONY: all clean test run help static-analysis coverage install uninstall test-only lexer-only

# Target padrão - compila ambos
all: $(LEXER_EXEC) $(TEST_EXEC)

# Compilar apenas o lexer
lexer-only: $(LEXER_EXEC)

# Compilar apenas os testes
test-only: $(TEST_EXEC)

# Compilar executável do lexer (com main integrado)
$(LEXER_EXEC): $(LEXER_OBJ)
	$(CC) $(LDFLAGS) -o $@ $^

# Compilar versão biblioteca do lexer (sem main)
$(LEXER_LIB_OBJ): $(LEXER_SRC)
	$(CC) $(CFLAGS) -DLIB_BUILD -c $< -o $@

# Compilar executável de testes (usando versão biblioteca)
$(TEST_EXEC): $(TEST_OBJ) $(LEXER_LIB_OBJ)
	$(CC) $(LDFLAGS) -o $@ $^

# Compilar arquivos objeto
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Executar programa principal (lexer com testes integrados)
run: $(LEXER_EXEC)
	./$(LEXER_EXEC)

# Executar testes estendidos
test: $(TEST_EXEC)
	./$(TEST_EXEC)

# Executar ambos os programas
run-all: $(LEXER_EXEC) $(TEST_EXEC)
	@echo "=== Executando lexer com testes integrados ==="
	./$(LEXER_EXEC)
	@echo ""
	@echo "=== Executando suite estendida de testes ==="
	./$(TEST_EXEC)

# Executar testes com valgrind (se disponível)
memcheck: $(TEST_EXEC)
	@if command -v valgrind > /dev/null 2>&1; then \
		echo "Executando verificação de memória com valgrind..."; \
		valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes ./$(TEST_EXEC); \
	else \
		echo "Valgrind não encontrado. Executando testes normalmente..."; \
		./$(TEST_EXEC); \
	fi

# Executar verificação de memória no lexer também
memcheck-lexer: $(LEXER_EXEC)
	@if command -v valgrind > /dev/null 2>&1; then \
		echo "Executando verificação de memória no lexer..."; \
		valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes ./$(LEXER_EXEC); \
	else \
		echo "Valgrind não encontrado. Executando lexer normalmente..."; \
		./$(LEXER_EXEC); \
	fi

# Compilar com otimizações para produção
release: CFLAGS += -O2 -DNDEBUG
release: clean all

# Compilar com informações de debug
debug: CFLAGS += -O0 -g3 -DDEBUG
debug: clean all

# Executar análise estática (se disponível)
static-analysis:
	@echo "Executando análise estática..."
	@if command -v cppcheck > /dev/null 2>&1; then \
		echo "Usando cppcheck:"; \
		cppcheck --enable=all --std=c99 --suppress=missingIncludeSystem $(LEXER_SRC) $(TEST_SRC); \
	else \
		echo "cppcheck não encontrado."; \
	fi
	@if command -v clang-tidy > /dev/null 2>&1; then \
		echo "Usando clang-tidy:"; \
		clang-tidy $(LEXER_SRC) $(TEST_SRC) -- $(CFLAGS); \
	else \
		echo "clang-tidy não encontrado."; \
	fi

# Gerar relatório de cobertura (se gcov disponível)
coverage: CFLAGS += --coverage -fprofile-arcs -ftest-coverage
coverage: LDFLAGS += --coverage
coverage: clean $(TEST_EXEC)
	@echo "Executando testes para cobertura..."
	./$(TEST_EXEC)
	@if command -v gcov > /dev/null 2>&1; then \
		echo "Gerando relatório de cobertura..."; \
		gcov $(LEXER_SRC) $(TEST_SRC); \
		echo "Arquivos de cobertura gerados:"; \
		ls -la *.gcov; \
	else \
		echo "gcov não encontrado."; \
	fi
	@if command -v lcov > /dev/null 2>&1; then \
		echo "Gerando relatório HTML com lcov..."; \
		lcov --capture --directory . --output-file coverage.info; \
		genhtml coverage.info --output-directory coverage_html; \
		echo "Relatório HTML gerado em coverage_html/index.html"; \
	fi

# Verificar sintaxe sem compilar
check-syntax:
	@echo "Verificando sintaxe..."
	$(CC) $(CFLAGS) -fsyntax-only $(LEXER_SRC)
	$(CC) $(CFLAGS) -fsyntax-only $(TEST_SRC)
	@echo "Sintaxe OK!"

# Limpar arquivos gerados
clean:
	@echo "Limpando arquivos gerados..."
	rm -f $(LEXER_OBJ) $(TEST_OBJ) $(LEXER_LIB_OBJ)
	rm -f $(LEXER_EXEC) $(TEST_EXEC)
	rm -f *.gcov *.gcda *.gcno coverage.info
	rm -rf coverage_html
	rm -f core vgcore.* *.core
	@echo "Limpeza concluída!"

# Limpar tudo incluindo backups
distclean: clean
	rm -f *~ *.bak *.swp
	rm -f .*.swp

# Instalar (copia para /usr/local/bin)
install: $(LEXER_EXEC) $(TEST_EXEC)
	@echo "Instalando executáveis em /usr/local/bin/"
	@if [ ! -d "/usr/local/bin" ]; then \
		echo "Criando diretório /usr/local/bin/"; \
		sudo mkdir -p /usr/local/bin; \
	fi
	@sudo cp $(LEXER_EXEC) /usr/local/bin/
	@sudo cp $(TEST_EXEC) /usr/local/bin/
	@sudo chmod +x /usr/local/bin/$(LEXER_EXEC)
	@sudo chmod +x /usr/local/bin/$(TEST_EXEC)
	@echo "Instalação concluída!"

# Desinstalar
uninstall:
	@echo "Removendo executáveis de /usr/local/bin/"
	@sudo rm -f /usr/local/bin/$(LEXER_EXEC)
	@sudo rm -f /usr/local/bin/$(TEST_EXEC)
	@echo "Desinstalação concluída!"

# Verificar dependências do sistema
check-deps:
	@echo "Verificando dependências do sistema..."
	@command -v $(CC) > /dev/null 2>&1 || (echo "ERRO: $(CC) não encontrado!" && exit 1)
	@echo "✓ $(CC) encontrado: $$($(CC) --version | head -n1)"
	@command -v make > /dev/null 2>&1 && echo "✓ make encontrado: $$(make --version | head -n1)" || echo "⚠ make não encontrado"
	@command -v valgrind > /dev/null 2>&1 && echo "✓ valgrind encontrado" || echo "⚠ valgrind não encontrado (opcional)"
	@command -v cppcheck > /dev/null 2>&1 && echo "✓ cppcheck encontrado" || echo "⚠ cppcheck não encontrado (opcional)"
	@command -v gcov > /dev/null 2>&1 && echo "✓ gcov encontrado" || echo "⚠ gcov não encontrado (opcional)"

# Mostrar informações do projeto
info:
	@echo "Informações do Projeto DataLang AFDs"
	@echo "===================================="
	@echo "Arquivos fonte: $(LEXER_SRC) $(TEST_SRC)"
	@echo "Executáveis: $(LEXER_EXEC) $(TEST_EXEC)"
	@echo "Compilador: $(CC)"
	@echo "Flags: $(CFLAGS)"
	@echo "Tamanho dos arquivos fonte:"
	@wc -l $(LEXER_SRC) $(TEST_SRC) 2>/dev/null || echo "Arquivos não encontrados"

# Ajuda
help:
	@echo "DataLang AFDs - Makefile"
	@echo "========================"
	@echo ""
	@echo "Targets principais:"
	@echo "  all           - Compilar todos os executáveis (padrão)"
	@echo "  lexer-only    - Compilar apenas o lexer"
	@echo "  test-only     - Compilar apenas os testes"
	@echo "  run           - Executar lexer com testes integrados"
	@echo "  test          - Executar suite estendida de testes"
	@echo "  run-all       - Executar ambos os programas"
	@echo ""
	@echo "Targets de qualidade:"
	@echo "  check-syntax  - Verificar sintaxe sem compilar"
	@echo "  memcheck      - Executar com verificação de memória (valgrind)"
	@echo "  memcheck-lexer- Verificar memória do lexer"
	@echo "  static-analysis- Executar análise estática (cppcheck, clang-tidy)"
	@echo "  coverage      - Gerar relatório de cobertura de código"
	@echo ""
	@echo "Targets de build:"
	@echo "  debug         - Compilar com informações de debug"
	@echo "  release       - Compilar otimizado para produção"
	@echo ""
	@echo "Targets de manutenção:"
	@echo "  clean         - Limpar arquivos gerados"
	@echo "  distclean     - Limpeza completa (inclui backups)"
	@echo "  install       - Instalar em /usr/local/bin"
	@echo "  uninstall     - Remover de /usr/local/bin"
	@echo ""
	@echo "Targets informativos:"
	@echo "  check-deps    - Verificar dependências do sistema"
	@echo "  info          - Mostrar informações do projeto"
	@echo "  help          - Mostrar esta ajuda"
	@echo ""
	@echo "Exemplos de uso:"
	@echo "  make              # Compilar tudo"
	@echo "  make test         # Executar testes estendidos"
	@echo "  make memcheck     # Verificar vazamentos de memória"
	@echo "  make coverage     # Gerar relatório de cobertura"
	@echo "  make clean all    # Limpar e recompilar"