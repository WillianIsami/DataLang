CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -g
TARGET = datalang_lexer
SOURCES = datalang_afn.c afn_to_afd.c lexer.c

all: $(TARGET)

$(TARGET): $(SOURCES)
	$(CC) $(CFLAGS) -o $(TARGET) $(SOURCES)

clean:
	rm -f $(TARGET)

test: $(TARGET)
	./$(TARGET)

test-file: $(TARGET)
	./$(TARGET) exemplo.datalang

.PHONY: all clean test test-file