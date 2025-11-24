#ifndef DATALANG_AFN_H
#define DATALANG_AFN_H

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#define EPSILON 256

// Tipos de Token
typedef enum {
    // Palavras-chave
    TOKEN_LET, TOKEN_FN, TOKEN_DATA, TOKEN_FILTER, TOKEN_MAP, TOKEN_REDUCE,
    TOKEN_IMPORT, TOKEN_EXPORT, TOKEN_IF, TOKEN_ELSE, TOKEN_FOR, TOKEN_IN,
    TOKEN_RETURN, TOKEN_LOAD, TOKEN_SAVE, TOKEN_SELECT, TOKEN_GROUPBY,
    TOKEN_SUM, TOKEN_MEAN, TOKEN_COUNT, TOKEN_MIN, TOKEN_MAX, TOKEN_AS,
    TOKEN_TRUE, TOKEN_FALSE,
    
    // Tipos
    TOKEN_INT_TYPE, TOKEN_FLOAT_TYPE, TOKEN_STRING_TYPE, TOKEN_BOOL_TYPE,
    TOKEN_DATAFRAME_TYPE, TOKEN_VECTOR_TYPE, TOKEN_SERIES_TYPE,
    
    // Identificadores e literais
    TOKEN_IDENTIFIER, TOKEN_INTEGER, TOKEN_FLOAT, TOKEN_STRING,
    
    // Operadores
    TOKEN_PLUS, TOKEN_MINUS, TOKEN_MULT, TOKEN_DIV, TOKEN_MOD,
    TOKEN_ASSIGN, TOKEN_EQUAL, TOKEN_NOT_EQUAL, TOKEN_LESS, TOKEN_LESS_EQUAL,
    TOKEN_GREATER, TOKEN_GREATER_EQUAL, TOKEN_AND, TOKEN_OR, TOKEN_NOT,
    TOKEN_ARROW, TOKEN_PIPE, TOKEN_OPERATOR,
    
    // Delimitadores
    TOKEN_LPAREN, TOKEN_RPAREN, TOKEN_LBRACKET, TOKEN_RBRACKET,
    TOKEN_LBRACE, TOKEN_RBRACE, TOKEN_SEMICOLON, TOKEN_COMMA, TOKEN_DOT,
    TOKEN_COLON, TOKEN_RANGE, TOKEN_DELIMITER,
    
    // Outros
    TOKEN_WHITESPACE, TOKEN_COMMENT, TOKEN_ERROR, TOKEN_EOF, TOKEN_PRINT
} TokenType;

// Estrutura do Token
typedef struct {
    TokenType type;
    char* lexema;
    int length;
    int line;
    int column;
} Token;

// Conjunto de estados
typedef struct {
    int* states;
    int size;
    int capacity;
} StateSet;

// Transição do AFN
typedef struct {
    int input;
    StateSet* to_states;
} AFNTransition;

// AFN
typedef struct {
    int num_states;
    int alphabet_size;
    int start_state;
    AFNTransition** transitions;
    int* transition_counts;
    bool* final_states;
    TokenType* token_types;
} AFN;

// AFD
typedef struct {
    int num_states;
    int alphabet_size;
    int start_state;
    int** transition_table;
    bool* final_states;
    TokenType* token_types;
} AFD;

// Funções para conjuntos de estados
StateSet* create_state_set();
void free_state_set(StateSet* set);
void add_state(StateSet* set, int state);
bool contains_state(StateSet* set, int state);
StateSet* copy_state_set(StateSet* set);
bool sets_equal(StateSet* a, StateSet* b);

// Funções do AFN
AFN* create_afn(int num_states, int alphabet_size);
void free_afn(AFN* afn);
void add_afn_transition(AFN* afn, int from, int input, int to);

// Funções do algoritmo
StateSet* epsilon_closure(AFN* afn, StateSet* states);
StateSet* move(AFN* afn, StateSet* states, int input);

// Conversão AFN -> AFD
AFD* afn_to_afd(AFN* afn);
void free_afd(AFD* afd);

// AFN unificado para DataLang
AFN* create_unified_datalang_afn();

#endif