#ifndef DATALANG_LEXER_H
#define DATALANG_LEXER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>
// Definições de tipos de token
typedef enum {
    TOKEN_KEYWORD,
    TOKEN_TYPE,
    TOKEN_IDENTIFIER,
    TOKEN_INTEGER,
    TOKEN_FLOAT,
    TOKEN_STRING,
    TOKEN_BOOLEAN,
    TOKEN_OPERATOR,
    TOKEN_DELIMITER,
    TOKEN_WHITESPACE,
    TOKEN_COMMENT,
    TOKEN_UNKNOWN,
    TOKEN_EOF
} TokenType;

// Estrutura para representar um token
typedef struct {
    TokenType type;
    char* value;
    int line;
    int column;
} Token;

// Estados dos AFDs
typedef enum {
    STATE_START = 0,
    STATE_ACCEPT = -1,
    STATE_REJECT = -2
} AFDState;

// Estrutura para AFD
typedef struct {
    int num_states;
    int alphabet_size;
    int** transition_table;
    bool* final_states;
    int start_state;
} AFD;

// Protótipos de funções principais
AFD* create_afd(int num_states, int alphabet_size);
void free_afd(AFD* afd);
bool run_afd(AFD* afd, const char* input, int* chars_consumed);

// Protótipos para criação de AFDs específicos
AFD* create_identifier_afd(void);
AFD* create_number_afd(void);
AFD* create_string_afd(void);
AFD* create_whitespace_afd(void);

// Protótipos para análise léxica
Token* tokenize(const char* input);
TokenType recognize_operator(char c);
TokenType recognize_delimiter(char c);
const char* token_type_name(TokenType type);

// Protótipos para algoritmos de otimização
AFD* minimize_afd(AFD* afd);

// Protótipos para funções auxiliares
bool is_in_list(const char* str, const char** list);

// Protótipos para testes
void test_identifier_afd(void);
void test_number_afd(void);
void test_string_afd(void);
void test_whitespace_afd(void);
void test_line_comment_afd(void);
void test_block_comment_afd(void);
void test_operator_afd(void);
void test_delimiter_afd(void);
void test_lexer_integration(void);
void test_keywords_and_types(void);
void test_minimization_algorithm(void);
void test_error_handling(void);
void test_lexer(void);
void run_comprehensive_tests(void);


// Constantes externas
extern const char* keywords[];
extern const char* types[];
extern const char* booleans[];

#endif // DATALANG_LEXER_H