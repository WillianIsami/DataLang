# Makefile para o Compilador DataLang
# Compila lexer + parser

CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -g -I.
TARGET = datalang

# Fontes do lexer
LEXER_SOURCES = lexer/datalang_afn.c lexer/afn_to_afd.c lexer/lexer.c

# Fontes do parser
PARSER_SOURCES = parser/parser.c parser/parser_expr.c parser/parser_main.c

# Todos os arquivos fonte
SOURCES = $(LEXER_SOURCES) $(PARSER_SOURCES)

# Arquivos objeto
OBJECTS = $(SOURCES:.c=.o)

# Regra principal
all: $(TARGET)

# Compila o executável
$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJECTS)
	@echo ""
	@echo "✓ Compilação concluída com sucesso!"
	@echo "✓ Executável: ./$(TARGET)"
	@echo ""

# Compila arquivos .c em .o
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Limpeza
clean:
	rm -f $(TARGET) $(OBJECTS)
	@echo "✓ Arquivos limpos"

# Testa com código inline
test: $(TARGET)
	@echo ""
	@echo "Executando teste com código embutido..."
	@echo ""
	./$(TARGET)

# Testa com arquivo exemplo
test-file: $(TARGET)
	@if [ -f ../examples/exemplo.datalang ]; then \
		echo ""; \
		echo "Executando teste com arquivo ../examples/exemplo.datalang..."; \
		echo ""; \
		./$(TARGET) ../examples/exemplo.datalang; \
	else \
		echo ""; \
		echo "Erro: arquivo ../examples/exemplo.datalang não encontrado"; \
		echo "Crie um arquivo ../examples/exemplo.datalang para testar"; \
		echo ""; \
	fi

# Mostra ajuda
help:
	@echo ""
	@echo "Makefile do Compilador DataLang"
	@echo "================================"
	@echo ""
	@echo "Targets disponíveis:"
	@echo "  make           - Compila o compilador"
	@echo "  make clean     - Remove arquivos compilados"
	@echo "  make test      - Executa teste com código embutido"
	@echo "  make test-file - Executa teste com ../examples/exemplo.datalang"
	@echo "  make help      - Mostra esta ajuda"
	@echo ""

# Cria arquivo de exemplo se não existir
exemplo:
	@if [ ! -f ../examples/exemplo.datalang ]; then \
		echo "let x = 42;" > ../examples/exemplo.datalang; \
		echo "let name = \"DataLang\";" >> ../examples/exemplo.datalang; \
		echo "" >> ../examples/exemplo.datalang; \
		echo "fn soma(a: Int, b: Int) -> Int {" >> ../examples/exemplo.datalang; \
		echo "    return a + b;" >> ../examples/exemplo.datalang; \
		echo "}" >> ../examples/exemplo.datalang; \
		echo "" >> ../examples/exemplo.datalang; \
		echo "let resultado = soma(5, 3);" >> ../examples/exemplo.datalang; \
		echo ""; \
		echo "✓ Arquivo ../examples/exemplo.datalang criado"; \
		echo ""; \
	else \
		echo ""; \
		echo "Arquivo ../examples/exemplo.datalang já existe"; \
		echo ""; \
	fi

.PHONY: all clean test test-file help exemplo