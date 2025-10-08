#ifndef DATALANG_AFN_H
#define DATALANG_AFN_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#define MAX_TRANSITIONS 256
#define EPSILON -1

// Enumeração COMPLETA dos tokens da DataLang
typedef enum {
    TOKEN_ERROR = 0,
    
    // Palavras-chave
    TOKEN_LET,
    TOKEN_FN,
    TOKEN_DATA,
    TOKEN_FILTER,
    TOKEN_MAP,
    TOKEN_REDUCE,
    TOKEN_IMPORT,
    TOKEN_EXPORT,
    TOKEN_IF,
    TOKEN_ELSE,
    TOKEN_FOR,
    TOKEN_IN,
    TOKEN_RETURN,
    TOKEN_LOAD,
    TOKEN_SAVE,
    TOKEN_SELECT,
    TOKEN_GROUPBY,
    TOKEN_SUM,
    TOKEN_MEAN,
    TOKEN_COUNT,
    TOKEN_MIN,
    TOKEN_MAX,
    TOKEN_AS,
    TOKEN_TRUE,
    TOKEN_FALSE,
    
    // Tipos
    TOKEN_INT_TYPE,
    TOKEN_FLOAT_TYPE,
    TOKEN_STRING_TYPE,
    TOKEN_BOOL_TYPE,
    TOKEN_DATAFRAME_TYPE,
    TOKEN_VECTOR_TYPE,
    TOKEN_SERIES_TYPE,
    
    // Identificadores e Literais
    TOKEN_IDENTIFIER,
    TOKEN_INTEGER,
    TOKEN_FLOAT,
    TOKEN_STRING,
    TOKEN_BOOLEAN,
    
    // Operadores
    TOKEN_OPERATOR,
    TOKEN_PLUS,
    TOKEN_MINUS,
    TOKEN_MULT,
    TOKEN_DIV,
    TOKEN_MOD,
    TOKEN_ASSIGN,
    TOKEN_EQUAL,
    TOKEN_NOT_EQUAL,
    TOKEN_LESS,
    TOKEN_LESS_EQUAL,
    TOKEN_GREATER,
    TOKEN_GREATER_EQUAL,
    TOKEN_AND,
    TOKEN_OR,
    TOKEN_NOT,
    TOKEN_ARROW,
    TOKEN_PIPE,
    
    // Delimitadores
    TOKEN_DELIMITER,
    TOKEN_LPAREN,
    TOKEN_RPAREN,
    TOKEN_LBRACKET,
    TOKEN_RBRACKET,
    TOKEN_LBRACE,
    TOKEN_RBRACE,
    TOKEN_SEMICOLON,
    TOKEN_COMMA,
    TOKEN_DOT,
    TOKEN_COLON,
    TOKEN_RANGE,
    
    // Outros
    TOKEN_WHITESPACE,
    TOKEN_COMMENT,
    TOKEN_COMMENT_LINE,
    TOKEN_EOF
} TokenType;

typedef struct {
    TokenType type;
    char* value;
    int length;
    int line;
    int column;
} Token;

typedef struct {
    int* states;
    int size;
    int capacity;
} StateSet;

typedef struct {
    int input;
    StateSet* to_states;
} AFNTransition;

typedef struct {
    int num_states;
    int alphabet_size;
    AFNTransition** transitions;
    int* transition_counts;
    bool* final_states;
    TokenType* token_types;
    int start_state;
} AFN;

typedef struct {
    int num_states;
    int alphabet_size;
    int** transition_table;
    bool* final_states;
    TokenType* token_types;
    int start_state;
} AFD;

// Protótipos das funções
StateSet* create_state_set();
void free_state_set(StateSet* set);
void add_state(StateSet* set, int state);
bool contains_state(StateSet* set, int state);
StateSet* copy_state_set(StateSet* set);
bool sets_equal(StateSet* a, StateSet* b);

AFN* create_afn(int num_states, int alphabet_size);
void free_afn(AFN* afn);
void add_afn_transition(AFN* afn, int from, int input, int to);
AFN* create_unified_datalang_afn();

StateSet* epsilon_closure(AFN* afn, StateSet* states);
StateSet* move(AFN* afn, StateSet* states, int input);

void free_afd(AFD* afd);

#endif