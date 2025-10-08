# ============================================================================
# DataLang Compiler - Makefile
# Sistema de build para analisador léxico com conversão AFN→AFD
# ============================================================================

# Compilador e flags
CC = gcc
CFLAGS = -std=c99 -Wall -Wextra -pedantic -O2
CFLAGS_DEBUG = -std=c99 -Wall -Wextra -pedantic -g -O0 -DDEBUG
LDFLAGS = 

# Diretórios
SRC_DIR = .
OBJ_DIR = obj
BIN_DIR = bin

# Criar diretórios se não existirem
$(shell mkdir -p $(OBJ_DIR) $(BIN_DIR))

# ============================================================================
# ALVOS PRINCIPAIS
# ============================================================================

.PHONY: all clean test help

# Compila tudo
all: afn_to_afd lexer test_conversion

# Remove arquivos compilados
clean:
	@echo "Limpando arquivos compilados..."
	rm -rf $(OBJ_DIR) $(BIN_DIR)
	rm -f *.o *.out test_conversion lexer afn_to_afd
	@echo "Limpeza concluída!"

# Executa todos os testes
test: test_conversion lexer
	@echo ""
	@echo "╔══════════════════════════════════════════════════════════════╗"
	@echo "║              EXECUTANDO TESTES DO ANALISADOR                 ║"
	@echo "╚══════════════════════════════════════════════════════════════╝"
	@echo ""
	@echo "Teste 1: Conversão AFN→AFD"
	@./test_conversion
	@echo ""
	@echo "Teste 2: Analisador Léxico"
	@./lexer
	@echo ""
	@echo "Todos os testes concluídos!"

# Mostra ajuda
help:
	@echo "╔══════════════════════════════════════════════════════════════╗"
	@echo "║              DataLang Compiler - Makefile                    ║"
	@echo "╚══════════════════════════════════════════════════════════════╝"
	@echo ""
	@echo "Alvos disponíveis:"
	@echo ""
	@echo "  make all              - Compila todos os programas"
	@echo "  make lexer            - Compila analisador léxico"
	@echo "  make afn_to_afd       - Compila conversor AFN→AFD"
	@echo "  make test_conversion  - Compila programa de teste"
	@echo "  make test             - Executa todos os testes"
	@echo "  make clean            - Remove arquivos compilados"
	@echo "  make debug            - Compila com símbolos de debug"
	@echo "  make help             - Mostra esta mensagem"
	@echo ""
	@echo "Exemplos:"
	@echo "  make && make test     - Compila e testa tudo"
	@echo "  make clean all        - Recompila do zero"
	@echo ""

# ============================================================================
# COMPILAÇÃO DOS PROGRAMAS
# ============================================================================

# Analisador Léxico Final
lexer: lexer.c afn_to_afd.c datalang_afn.c
	@echo "Compilando analisador léxico final..."
	$(CC) $(CFLAGS) -o lexer lexer.c afn_to_afd.c datalang_afn.c
	@echo "lexer compilado com sucesso!"

# Conversor AFN->AFD (apenas biblioteca)
afn_to_afd.o: afn_to_afd.c datalang_afn.h
	@echo "Compilando biblioteca afn_to_afd..."
	$(CC) $(CFLAGS) -c afn_to_afd.c -o afn_to_afd.o

# Teste do conversor AFN->AFD
test_afn_to_afd: afn_to_afd.o datalang_afn.o
	@echo "Compilando teste do conversor AFN→AFD..."
	$(CC) $(CFLAGS) -DTEST_AFN_TO_AFD -o test_afn_to_afd afn_to_afd.o datalang_afn.o
	@echo "test_afn_to_afd compilado com sucesso!"

# Programa de teste de conversão
test_conversion: test_afn_to_afd.c
	@echo "Compilando programa de teste..."
	$(CC) $(CFLAGS) -o test_conversion test_afn_to_afd.c
	@echo "test_conversion compilado com sucesso!"

lexer.o: lexer.c datalang_afn.h afn_to_afd.h
	$(CC) $(CFLAGS) -c lexer.c

datalang_afn.o: datalang_afn.c datalang_afn.h
	$(CC) $(CFLAGS) -c datalang_afn.c

# ============================================================================
# COMPILAÇÃO COM DEBUG
# ============================================================================

.PHONY: debug debug_lexer debug_test

debug: debug_lexer debug_test
	@echo "Versões debug compiladas!"

debug_lexer: lexer.c
	@echo "Compilando lexer (DEBUG)..."
	$(CC) $(CFLAGS_DEBUG) -o lexer_debug lexer.c
	@echo "lexer_debug compilado!"

debug_test: test_afn_to_afd.c
	@echo "Compilando test_conversion (DEBUG)..."
	$(CC) $(CFLAGS_DEBUG) -o test_conversion_debug test_afn_to_afd.c
	@echo "test_conversion_debug compilado!"

# ============================================================================
# ALVOS DE TESTE INDIVIDUAIS
# ============================================================================

.PHONY: test_lexer test_conversion_run test_examples

test_lexer: lexer
	@echo "Testando analisador léxico..."
	./lexer

test_conversion_run: test_conversion
	@echo "Testando conversão AFN→AFD..."
	./test_conversion

test_examples: lexer
	@echo "Testando com exemplos..."
	@if [ -d "../../examples" ]; then \
		for file in ../../examples/*.dl; do \
			echo "  Processando $$file..."; \
			./lexer < $$file; \
		done; \
	else \
		echo "  Diretório examples/ não encontrado"; \
	fi

# ============================================================================
# ANÁLISE DE CÓDIGO
# ============================================================================

.PHONY: lint check valgrind

# Análise estática com cppcheck (se disponível)
lint:
	@echo "Executando análise estática..."
	@if command -v cppcheck > /dev/null 2>&1; then \
		cppcheck --enable=all --suppress=missingIncludeSystem *.c; \
	else \
		echo "  cppcheck não instalado - pulando"; \
	fi

# Verificação de warnings
check: CFLAGS += -Werror
check: clean all
	@echo "Compilação com -Werror concluída!"

# Análise de memória com Valgrind (se disponível)
valgrind: test_conversion lexer
	@echo "Executando Valgrind..."
	@if command -v valgrind > /dev/null 2>&1; then \
		echo "  Testando test_conversion..."; \
		valgrind --leak-check=full --show-leak-kinds=all ./test_conversion; \
		echo "  Testando lexer..."; \
		valgrind --leak-check=full --show-leak-kinds=all ./lexer; \
	else \
		echo "  Valgrind não instalado - pulando"; \
	fi

# ============================================================================
# DOCUMENTAÇÃO
# ============================================================================

.PHONY: docs docs_view

# Gera documentação
docs:
	@echo "Gerando documentação..."
	@if command -v doxygen > /dev/null 2>&1; then \
		doxygen Doxyfile 2>/dev/null || echo "  Doxyfile não encontrado"; \
	else \
		echo "  Doxygen não instalado"; \
	fi

# Visualiza diagrama do AFD
docs_view:
	@echo "Abrindo diagrama do AFD final..."
	@if command -v xdg-open > /dev/null 2>&1; then \
		xdg-open ../../docs/diagramas/afd_final.md; \
	elif command -v open > /dev/null 2>&1; then \
		open ../../docs/diagramas/afd_final.md; \
	else \
		cat ../../docs/diagramas/afd_final.md; \
	fi

# ============================================================================
# INFORMAÇÕES DO SISTEMA
# ============================================================================

.PHONY: info

info:
	@echo "╔══════════════════════════════════════════════════════════════╗"
	@echo "║              INFORMAÇÕES DO SISTEMA                          ║"
	@echo "╚══════════════════════════════════════════════════════════════╝"
	@echo ""
	@echo "Compilador:"
	@$(CC) --version | head -n 1
	@echo ""
	@echo "Flags de compilação:"
	@echo "  $(CFLAGS)"
	@echo ""
	@echo "Diretórios:"
	@echo "  SRC: $(SRC_DIR)"
	@echo "  OBJ: $(OBJ_DIR)"
	@echo "  BIN: $(BIN_DIR)"
	@echo ""
	@echo "Ferramentas disponíveis:"
	@command -v cppcheck > /dev/null 2>&1 && echo "  cppcheck" || echo "  ✗ cppcheck"
	@command -v valgrind > /dev/null 2>&1 && echo "  valgrind" || echo "  ✗ valgrind"
	@command -v doxygen > /dev/null 2>&1 && echo "  doxygen" || echo "  ✗ doxygen"
	@echo ""

# ============================================================================
# INSTALAÇÃO
# ============================================================================

.PHONY: install uninstall

PREFIX ?= /usr/local

install: all
	@echo "Instalando DataLang Compiler..."
	install -d $(PREFIX)/bin
	install -m 755 lexer $(PREFIX)/bin/datalang-lexer
	@echo "Instalado em $(PREFIX)/bin/datalang-lexer"

uninstall:
	@echo "Desinstalando DataLang Compiler..."
	rm -f $(PREFIX)/bin/datalang-lexer
	@echo "Desinstalado!"

# ============================================================================
# BENCHMARK
# ============================================================================

.PHONY: benchmark

benchmark: lexer
	@echo "Executando benchmark..."
	@echo "Testando com arquivo grande..."
	@if [ -f "../../examples/large.dl" ]; then \
		time ./lexer < ../../examples/large.dl > /dev/null; \
	else \
		echo "  Arquivo de teste grande não encontrado"; \
		echo "  Criando arquivo de teste..."; \
		for i in {1..1000}; do \
			echo "let x$$i = $$i" >> /tmp/test_large.dl; \
		done; \
		time ./lexer < /tmp/test_large.dl > /dev/null; \
		rm /tmp/test_large.dl; \
	fi

# ============================================================================
# DISTRIBUIÇÃO
# ============================================================================

.PHONY: dist

VERSION = 1.0.0

dist: clean
	@echo "Criando pacote de distribuição..."
	@tar czf datalang-compiler-$(VERSION).tar.gz \
		*.c *.h Makefile README.md \
		../../docs/ \
		--transform 's,^\.,datalang-compiler-$(VERSION),'
	@echo "Pacote criado: datalang-compiler-$(VERSION).tar.gz"

# ============================================================================
# GIT SHORTCUTS
# ============================================================================

.PHONY: commit push

commit:
	@echo "Criando commit..."
	git add *.c *.h Makefile ../../docs/diagramas/afd_final.md ../../README.md
	@read -p "Mensagem do commit: " msg; \
	git commit -m "$$msg"
	@echo "Commit criado!"
	@echo "Número do commit: $$(git log -1 --format='%H')"

push: commit
	@echo "Enviando para repositório remoto..."
	git push origin main
	@echo "Push concluído!"

# ============================================================================
# DEPENDÊNCIAS
# ============================================================================

# Dependências automáticas (para builds incrementais)
-include $(OBJ_DIR)/*.d

# Regra para gerar arquivos de dependência
$(OBJ_DIR)/%.d: %.c
	@$(CC) -MM $(CFLAGS) $< | sed 's|^|$(OBJ_DIR)/|' > $@

.PHONY: deps

deps:
	@echo "Atualizando dependências..."
	@$(CC) -MM $(CFLAGS) *.c > .depend
	@echo "Dependências atualizadas!"

# ============================================================================
# CONFIGURAÇÕES
# ============================================================================

# Não remover arquivos intermediários
.PRECIOUS: $(OBJ_DIR)/%.o

# Desabilitar regras implícitas
.SUFFIXES:

# Força recompilação se Makefile mudar
$(OBJ_DIR)/%.o: Makefile