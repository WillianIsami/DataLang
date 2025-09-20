#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>

// Token types
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
    TOKEN_COMMENT_LINE,
    TOKEN_COMMENT_BLOCK,
    TOKEN_UNKNOWN,
    TOKEN_EOF
} TokenType;

// Token structure
typedef struct {
    TokenType type;
    char* value;
    int line;
    int column;
} Token;

// AFD states
typedef enum {
    STATE_START = 0,
    STATE_ACCEPT = -1,
    STATE_REJECT = -2
} AFDState;

// AFD structure
typedef struct {
    int num_states;
    int alphabet_size;
    int** transition_table;
    bool* final_states;
    int start_state;
} AFD;

// Keywords, types, and booleans
static const char* keywords[] = {
    "let", "fn", "data", "filter", "map", "reduce", "import", "export",
    "if", "else", "for", "in", "return", "load", "save", "select",
    "groupby", "sum", "mean", "count", "min", "max", "as", NULL
};

static const char* types[] = {
    "Int", "Float", "String", "Bool", "DataFrame", "Vector", "Series", NULL
};

static const char* booleans[] = {
    "true", "false", NULL
};

// Safe string duplication function
char* safe_strndup(const char* s, size_t n) {
    if (!s) return NULL;
    
    size_t len = 0;
    while (len < n && s[len]) len++; // Find actual length up to n
    
    char* result = malloc(len + 1);
    if (!result) return NULL;
    
    memcpy(result, s, len);
    result[len] = '\0';
    return result;
}

// Check if string is in list
bool is_in_list(const char* str, const char** list) {
    if (!str || !list) return false;
    for (int i = 0; list[i] != NULL; i++) {
        if (strcmp(str, list[i]) == 0) {
            return true;
        }
    }
    return false;
}

// Create AFD
AFD* create_afd(int num_states, int alphabet_size) {
    AFD* afd = malloc(sizeof(AFD));
    if (!afd) return NULL;
    
    afd->num_states = num_states;
    afd->alphabet_size = alphabet_size;
    afd->start_state = 0;
    
    // Allocate transition table
    afd->transition_table = malloc(num_states * sizeof(int*));
    if (!afd->transition_table) {
        free(afd);
        return NULL;
    }
    
    for (int i = 0; i < num_states; i++) {
        afd->transition_table[i] = malloc(alphabet_size * sizeof(int));
        if (!afd->transition_table[i]) {
            // Clean up on failure
            for (int j = 0; j < i; j++) {
                free(afd->transition_table[j]);
            }
            free(afd->transition_table);
            free(afd);
            return NULL;
        }
        for (int j = 0; j < alphabet_size; j++) {
            afd->transition_table[i][j] = STATE_REJECT;
        }
    }
    
    // Allocate final states array
    afd->final_states = malloc(num_states * sizeof(bool));
    if (!afd->final_states) {
        for (int i = 0; i < num_states; i++) {
            free(afd->transition_table[i]);
        }
        free(afd->transition_table);
        free(afd);
        return NULL;
    }
    
    for (int i = 0; i < num_states; i++) {
        afd->final_states[i] = false;
    }
    
    return afd;
}

// Free AFD
void free_afd(AFD* afd) {
    if (afd) {
        if (afd->transition_table) {
            for (int i = 0; i < afd->num_states; i++) {
                free(afd->transition_table[i]);
            }
            free(afd->transition_table);
        }
        free(afd->final_states);
        free(afd);
    }
}

// AFD for identifiers
AFD* create_identifier_afd() {
    AFD* afd = create_afd(2, 256);
    if (!afd) return NULL;
    
    // State 0 -> State 1 with letters and underscore
    for (int c = 'a'; c <= 'z'; c++) afd->transition_table[0][c] = 1;
    for (int c = 'A'; c <= 'Z'; c++) afd->transition_table[0][c] = 1;
    afd->transition_table[0]['_'] = 1;
    
    // State 1 -> State 1 with letters, digits and underscore
    for (int c = 'a'; c <= 'z'; c++) afd->transition_table[1][c] = 1;
    for (int c = 'A'; c <= 'Z'; c++) afd->transition_table[1][c] = 1;
    for (int c = '0'; c <= '9'; c++) afd->transition_table[1][c] = 1;
    afd->transition_table[1]['_'] = 1;
    
    afd->final_states[1] = true;
    return afd;
}

// AFD for numbers
AFD* create_number_afd() {
    AFD* afd = create_afd(8, 256);
    if (!afd) return NULL;
    
    // State 0: start
    afd->transition_table[0]['+'] = 1;
    afd->transition_table[0]['-'] = 1;
    for (int c = '0'; c <= '9'; c++) afd->transition_table[0][c] = 2;
    afd->transition_table[0]['.'] = 3;
    
    // State 1: after optional sign
    for (int c = '0'; c <= '9'; c++) afd->transition_table[1][c] = 2;
    afd->transition_table[1]['.'] = 3;
    
    // State 2: integer digits
    for (int c = '0'; c <= '9'; c++) afd->transition_table[2][c] = 2;
    afd->transition_table[2]['.'] = 4;
    afd->transition_table[2]['e'] = afd->transition_table[2]['E'] = 5;
    
    // State 3: after initial dot
    for (int c = '0'; c <= '9'; c++) afd->transition_table[3][c] = 4;
    
    // State 4: decimal part
    for (int c = '0'; c <= '9'; c++) afd->transition_table[4][c] = 4;
    afd->transition_table[4]['e'] = afd->transition_table[4]['E'] = 5;
    
    // State 5: after 'e' or 'E'
    afd->transition_table[5]['+'] = afd->transition_table[5]['-'] = 6;
    for (int c = '0'; c <= '9'; c++) afd->transition_table[5][c] = 7;
    
    // State 6: after exponent sign
    for (int c = '0'; c <= '9'; c++) afd->transition_table[6][c] = 7;
    
    // State 7: exponent digits
    for (int c = '0'; c <= '9'; c++) afd->transition_table[7][c] = 7;
    
    // Final states
    afd->final_states[2] = true; // integer
    afd->final_states[4] = true; // decimal
    afd->final_states[7] = true; // scientific
    
    return afd;
}

// AFD for strings
AFD* create_string_afd() {
    AFD* afd = create_afd(4, 256);
    if (!afd) return NULL;
    
    // State 0 -> State 1 with opening quote
    afd->transition_table[0]['"'] = 1;
    
    // State 1: inside string
    for (int c = 0; c < 256; c++) {
        if (c != '"' && c != '\\' && c != '\n') {
            afd->transition_table[1][c] = 1;
        }
    }
    afd->transition_table[1]['\\'] = 2; // escape
    afd->transition_table[1]['"'] = 3; // closing
    
    // State 2: after backslash
    afd->transition_table[2]['"'] = 1;
    afd->transition_table[2]['\\'] = 1;
    afd->transition_table[2]['n'] = 1;
    afd->transition_table[2]['t'] = 1;
    afd->transition_table[2]['r'] = 1;
    
    afd->final_states[3] = true;
    return afd;
}

// AFD for whitespace
AFD* create_whitespace_afd() {
    AFD* afd = create_afd(2, 256);
    if (!afd) return NULL;
    
    // State 0 -> State 1 with whitespace
    afd->transition_table[0][' '] = 1;
    afd->transition_table[0]['\t'] = 1;
    afd->transition_table[0]['\n'] = 1;
    afd->transition_table[0]['\r'] = 1;
    
    // State 1 -> State 1 with more whitespace
    afd->transition_table[1][' '] = 1;
    afd->transition_table[1]['\t'] = 1;
    afd->transition_table[1]['\n'] = 1;
    afd->transition_table[1]['\r'] = 1;
    
    afd->final_states[1] = true;
    return afd;
}

// AFD for line comments
AFD* create_line_comment_afd() {
    AFD* afd = create_afd(4, 256);
    if (!afd) return NULL;
    
    // State 0 -> State 1 with first '/'
    afd->transition_table[0]['/'] = 1;
    
    // State 1 -> State 2 with second '/'
    afd->transition_table[1]['/'] = 2;
    
    // State 2: any character except newline
    for (int c = 0; c < 256; c++) {
        if (c != '\n' && c != '\r') {
            afd->transition_table[2][c] = 2;
        }
    }
    
    // State 2 -> State 3 with newline (final)
    afd->transition_table[2]['\n'] = 3;
    afd->transition_table[2]['\r'] = 3;
    
    afd->final_states[2] = true; // Accept even without newline (EOF)
    afd->final_states[3] = true; // Accept with newline
    return afd;
}

// AFD for block comments
AFD* create_block_comment_afd() {
    AFD* afd = create_afd(5, 256);
    if (!afd) return NULL;
    
    // State 0 -> State 1 with '/'
    afd->transition_table[0]['/'] = 1;
    
    // State 1 -> State 2 with '*'
    afd->transition_table[1]['*'] = 2;
    
    // State 2: any character except '*'
    for (int c = 0; c < 256; c++) {
        if (c != '*') {
            afd->transition_table[2][c] = 2;
        }
    }
    
    // State 2 -> State 3 with '*'
    afd->transition_table[2]['*'] = 3;
    
    // State 3: if not '/', back to state 2; if '*', stay in 3
    for (int c = 0; c < 256; c++) {
        if (c != '*' && c != '/') {
            afd->transition_table[3][c] = 2;
        }
    }
    afd->transition_table[3]['*'] = 3;
    afd->transition_table[3]['/'] = 4; // Final
    
    afd->final_states[4] = true;
    return afd;
}

// AFD for operators
AFD* create_operator_afd() {
    AFD* afd = create_afd(22, 256);
    if (!afd) return NULL;
    
    // Error state (not final state)
    int error_state = 20;
    
    // Simple operators (immediate final states)
    afd->transition_table[0]['+'] = 1;  // +
    afd->transition_table[0]['-'] = 2;  // -
    afd->transition_table[0]['*'] = 3;  // *
    afd->transition_table[0]['/'] = 4;  // /
    afd->transition_table[0]['%'] = 5;  // %
    
    // Operator '=' and compounds
    afd->transition_table[0]['='] = 6;  // =
    afd->transition_table[6]['='] = 7;  // ==
    afd->transition_table[6]['>'] = 8;  // =>
    
    // // Operator '!' and compounds
    afd->transition_table[0]['!'] = 9;  // !
    afd->transition_table[9]['='] = 10; // !=
    
    // Operator '<' and compounds
    afd->transition_table[0]['<'] = 11; // <
    afd->transition_table[11]['='] = 12; // <=
    
    // Operator '>' and compounds
    afd->transition_table[0]['>'] = 13; // >
    afd->transition_table[13]['='] = 14; // >=
    
    // Operator '&' and compounds
    afd->transition_table[0]['&'] = 15; // &
    afd->transition_table[15]['&'] = 16; // &&
    
    // Operator '|' and compounds
    afd->transition_table[0]['|'] = 17; // |
    afd->transition_table[17]['|'] = 18; // ||
    afd->transition_table[17]['>'] = 19; // |>
    
    // --- Errors  ---

    // After '+', X '++', '+='
    afd->transition_table[1]['+'] = error_state;
    afd->transition_table[1]['='] = error_state;

    // After '-', X '--', '-=', '->'
    afd->transition_table[2]['-'] = error_state;
    afd->transition_table[2]['='] = error_state;
    afd->transition_table[2]['>'] = error_state;

    // After '*', X '**', '*='
    afd->transition_table[3]['*'] = error_state;
    afd->transition_table[3]['='] = error_state;

    // After '/', X '//', '/='
    afd->transition_table[4]['/'] = error_state;
    afd->transition_table[4]['='] = error_state;

    // After '%', X '%%', '%='
    afd->transition_table[5]['%'] = error_state;
    afd->transition_table[5]['='] = error_state;
    
    // After '==', X '==='
    afd->transition_table[7]['='] = error_state;

    // After '=>', X extensão óbvia é um erro comum, mas podemos proibir '='
    afd->transition_table[8]['='] = error_state;

    // After '!', X '!!' (se não for um operador válido)
    afd->transition_table[9]['!'] = error_state;
    
    // After '!=', X '!=='
    afd->transition_table[10]['='] = error_state;

    // After '<', X '<<' (shift), '<-' (atribuição)
    afd->transition_table[11]['<'] = error_state;
    afd->transition_table[11]['-'] = error_state;

    // After '<=', X '<=='
    afd->transition_table[12]['='] = error_state;

    // After '>', X '>>' (shift)
    afd->transition_table[13]['>'] = error_state;

    // After '>=', X '>=='
    afd->transition_table[14]['='] = error_state;

    // After '&', X '&=' (bitwise AND assign)
    afd->transition_table[15]['='] = error_state;
    
    // After '&&', X '&&&', '&&='
    afd->transition_table[16]['&'] = error_state;
    afd->transition_table[16]['='] = error_state;

    // After '|', X '|='
    afd->transition_table[17]['='] = error_state;

    // After '||', X '|||', '||='
    afd->transition_table[18]['|'] = error_state;
    afd->transition_table[18]['='] = error_state;

    // After '|>', X extensão óbvia é um erro comum, mas podemos proibir '='
    afd->transition_table[19]['='] = error_state;
    
    // --- Marcar Estados Finais ---
    // O estado de erro (20) não é incluído aqui, permanecendo como não final.
    for (int i = 1; i <= 19; i++) {
        afd->final_states[i] = true;
    }
    
    return afd;
}

// AFD for delimiters
AFD* create_delimiter_afd() {
    AFD* afd = create_afd(12, 256);
    if (!afd) return NULL;
    
    // Simple delimiters
    afd->transition_table[0]['('] = 1;  // LPAREN
    afd->transition_table[0][')'] = 2;  // RPAREN
    afd->transition_table[0]['['] = 3;  // LBRACKET
    afd->transition_table[0][']'] = 4;  // RBRACKET
    afd->transition_table[0]['{'] = 5;  // LBRACE
    afd->transition_table[0]['}'] = 6;  // RBRACE
    afd->transition_table[0][';'] = 7;  // SEMICOLON
    afd->transition_table[0][','] = 8;  // COMMA
    afd->transition_table[0][':'] = 9;  // COLON
    
    // Dot and range (..)
    afd->transition_table[0]['.'] = 10; // DOT
    afd->transition_table[10]['.'] = 11; // RANGE (..)
    
    // Mark final states
    for (int i = 1; i <= 11; i++) {
        afd->final_states[i] = true;
    }
    
    return afd;
}

// Run AFD
bool run_afd(AFD* afd, const char* input, int* chars_consumed) {
    if (!afd || !input) {
        if (chars_consumed) *chars_consumed = 0;
        return false;
    }
    
    int current_state = afd->start_state;
    int i = 0;
    int last_accept = -1;
    
    while (input[i] != '\0') {
        int c = (unsigned char)input[i];
        if (c >= afd->alphabet_size) {
            break;
        }
        
        int next_state = afd->transition_table[current_state][c];
        
        if (next_state == STATE_REJECT) {
            break;
        }
        
        current_state = next_state;
        i++;
        
        if (afd->final_states[current_state]) {
            last_accept = i;
        }
    }
    
    if (chars_consumed) {
        *chars_consumed = last_accept;
    }
    
    return last_accept > 0;
}

// Minimization algorithm (simplified)
AFD* minimize_afd(AFD* afd) {
    if (!afd) return NULL;
    
    // Basic partitioning algorithm implementation
    bool* partition = malloc(afd->num_states * sizeof(bool));
    bool* new_partition = malloc(afd->num_states * sizeof(bool));
    
    if (!partition || !new_partition) {
        free(partition);
        free(new_partition);
        return afd; // Return original on failure
    }
    
    // Initial partition: final vs non-final states
    for (int i = 0; i < afd->num_states; i++) {
        partition[i] = afd->final_states[i];
        new_partition[i] = afd->final_states[i];
    }
    
    bool changed = true;
    while (changed) {
        changed = false;
        
        // Check if states in same partition should be separated
        for (int i = 0; i < afd->num_states; i++) {
            for (int j = i + 1; j < afd->num_states; j++) {
                if (partition[i] == partition[j]) {
                    // Check if transitions lead to different partitions
                    for (int c = 0; c < afd->alphabet_size; c++) {
                        int next_i = afd->transition_table[i][c];
                        int next_j = afd->transition_table[j][c];
                        
                        if ((next_i >= 0 && next_j >= 0 && partition[next_i] != partition[next_j]) ||
                            (next_i < 0 && next_j >= 0) || (next_i >= 0 && next_j < 0)) {
                            new_partition[j] = !partition[j];
                            changed = true;
                            break;
                        }
                    }
                }
            }
        }
        
        // Update partitions
        memcpy(partition, new_partition, afd->num_states * sizeof(bool));
    }
    
    free(partition);
    free(new_partition);
    
    // For simplicity, return original AFD
    // Complete implementation would construct new minimized AFD
    return afd;
}

// Main lexical analyzer
Token* tokenize(const char* input) {
    if (!input) return NULL;
    
    Token* tokens = calloc(1000, sizeof(Token));
    if (!tokens) return NULL;
    
    int token_count = 0;
    int pos = 0;
    int line = 1, column = 1;
    
    // Create all AFDs
    AFD* id_afd = create_identifier_afd();
    AFD* num_afd = create_number_afd();
    AFD* str_afd = create_string_afd();
    AFD* ws_afd = create_whitespace_afd();
    AFD* line_comment_afd = create_line_comment_afd();
    AFD* block_comment_afd = create_block_comment_afd();
    AFD* op_afd = create_operator_afd();
    AFD* del_afd = create_delimiter_afd();
    
    // Check if AFD creation failed
    if (!id_afd || !num_afd || !str_afd || !ws_afd || 
        !line_comment_afd || !block_comment_afd || !op_afd || !del_afd) {
        free(tokens);
        free_afd(id_afd); free_afd(num_afd); free_afd(str_afd); free_afd(ws_afd);
        free_afd(line_comment_afd); free_afd(block_comment_afd); free_afd(op_afd); free_afd(del_afd);
        return NULL;
    }
    
    while (input[pos] != '\0' && token_count < 999) {
        int chars_consumed = 0;
        Token token = {0};
        token.line = line;
        token.column = column;
        
        // block comment first
        if (run_afd(block_comment_afd, &input[pos], &chars_consumed) && chars_consumed > 0) {
            token.type = TOKEN_COMMENT_BLOCK;
            token.value = safe_strndup(&input[pos], chars_consumed);
            pos += chars_consumed;
            
            // Count newlines in comment
            for (int i = 0; i < chars_consumed; i++) {
                if (input[pos - chars_consumed + i] == '\n') {
                    line++;
                    column = 1;
                } else {
                    column++;
                }
            }
            
            tokens[token_count++] = token;
            continue;
        }
        
        // line comment
        if (run_afd(line_comment_afd, &input[pos], &chars_consumed) && chars_consumed > 0) {
            token.type = TOKEN_COMMENT_LINE;
            token.value = safe_strndup(&input[pos], chars_consumed);
            pos += chars_consumed;
            
            // Adjust line and column
            if (chars_consumed > 0 && input[pos-1] == '\n') {
                line++;
                column = 1;
            } else {
                column += chars_consumed;
            }
            
            tokens[token_count++] = token;
            continue;
        }
        
        // whitespace
        if (run_afd(ws_afd, &input[pos], &chars_consumed) && chars_consumed > 0) {
            token.type = TOKEN_WHITESPACE;
            token.value = safe_strndup(&input[pos], chars_consumed);
            pos += chars_consumed;
            
            // Count newlines
            for (int i = 0; i < chars_consumed; i++) {
                if (token.value[i] == '\n') {
                    line++;
                    column = 1;
                } else {
                    column++;
                }
            }
            
            tokens[token_count++] = token;
            continue;
        }
        
        // string
        if (run_afd(str_afd, &input[pos], &chars_consumed) && chars_consumed > 0) {
            token.type = TOKEN_STRING;
            token.value = safe_strndup(&input[pos], chars_consumed);
            pos += chars_consumed;
            column += chars_consumed;
            tokens[token_count++] = token;
            continue;
        }
        
        // number
        if (run_afd(num_afd, &input[pos], &chars_consumed) && chars_consumed > 0) {
            char* temp_str = safe_strndup(&input[pos], chars_consumed);
            if (temp_str && (strchr(temp_str, '.') || strchr(temp_str, 'e') || strchr(temp_str, 'E'))) {
                token.type = TOKEN_FLOAT;
            } else {
                token.type = TOKEN_INTEGER;
            }
            token.value = temp_str;
            pos += chars_consumed;
            column += chars_consumed;
            tokens[token_count++] = token;
            continue;
        }
        
        // operator
        if (run_afd(op_afd, &input[pos], &chars_consumed) && chars_consumed > 0) {
            token.type = TOKEN_OPERATOR;
            token.value = safe_strndup(&input[pos], chars_consumed);
            pos += chars_consumed;
            column += chars_consumed;
            tokens[token_count++] = token;
            continue;
        }
        
        // delimiter
        if (run_afd(del_afd, &input[pos], &chars_consumed) && chars_consumed > 0) {
            token.type = TOKEN_DELIMITER;
            token.value = safe_strndup(&input[pos], chars_consumed);
            pos += chars_consumed;
            column += chars_consumed;
            tokens[token_count++] = token;
            continue;
        }
        
        // identifier/keyword (last to give precedence to operators)
        if (run_afd(id_afd, &input[pos], &chars_consumed) && chars_consumed > 0) {
            token.value = safe_strndup(&input[pos], chars_consumed);
            
            if (is_in_list(token.value, keywords)) {
                token.type = TOKEN_KEYWORD;
            } else if (is_in_list(token.value, types)) {
                token.type = TOKEN_TYPE;
            } else if (is_in_list(token.value, booleans)) {
                token.type = TOKEN_BOOLEAN;
            } else {
                token.type = TOKEN_IDENTIFIER;
            }
            
            pos += chars_consumed;
            column += chars_consumed;
            tokens[token_count++] = token;
            continue;
        }
        
        // Unrecognized character
        token.type = TOKEN_UNKNOWN;
        token.value = safe_strndup(&input[pos], 1);
        pos++;
        column++;
        tokens[token_count++] = token;
    }
    
    // EOF token
    tokens[token_count].type = TOKEN_EOF;
    tokens[token_count].value = NULL;
    tokens[token_count].line = line;
    tokens[token_count].column = column;
    token_count++;
    
    // Free AFDs
    free_afd(id_afd); free_afd(num_afd); free_afd(str_afd); free_afd(ws_afd);
    free_afd(line_comment_afd); free_afd(block_comment_afd); free_afd(op_afd); free_afd(del_afd);
    
    return tokens;
}

// Get token type name
const char* token_type_name(TokenType type) {
    switch (type) {
        case TOKEN_KEYWORD: return "KEYWORD";
        case TOKEN_TYPE: return "TYPE";
        case TOKEN_IDENTIFIER: return "IDENTIFIER";
        case TOKEN_INTEGER: return "INTEGER";
        case TOKEN_FLOAT: return "FLOAT";
        case TOKEN_STRING: return "STRING";
        case TOKEN_BOOLEAN: return "BOOLEAN";
        case TOKEN_OPERATOR: return "OPERATOR";
        case TOKEN_DELIMITER: return "DELIMITER";
        case TOKEN_WHITESPACE: return "WHITESPACE";
        case TOKEN_COMMENT_LINE: return "COMMENT_LINE";
        case TOKEN_COMMENT_BLOCK: return "COMMENT_BLOCK";
        case TOKEN_UNKNOWN: return "UNKNOWN";
        case TOKEN_EOF: return "EOF";
        default: return "INVALID";
    }
}

// Test functions implementation
void test_identifier_afd() {
    printf("=== Teste AFD Identificadores ===\n");
    AFD* afd = create_identifier_afd();
    if (!afd) {
        printf("Erro: falha ao criar AFD\n");
        return;
    }
    
    struct {
        const char* input;
        bool expected;
        const char* description;
    } test_cases[] = {
        {"variable", true, "Identificador simples"},
        {"_private", true, "Começando com underscore"},
        {"var1", true, "Com número"},
        {"_var_name_123", true, "Complexo válido"},
        {"CamelCase", true, "CamelCase"},
        {"UPPER_CASE", true, "Upper case"},
        {"a", true, "Letra única"},
        {"_", true, "Underscore único"},
        {"123invalid", false, "Começando com número"},
        {"", false, "String vazia"},
        {"var-name", true, "Para até o hífen (aceita 'var')"},
        {NULL, false, NULL}
    };
    
    int passed = 0, total = 0;
    for (int i = 0; test_cases[i].input != NULL; i++) {
        int chars_consumed;
        bool result = run_afd(afd, test_cases[i].input, &chars_consumed);
        bool test_passed = (result == test_cases[i].expected);
        
        printf("'%s': %s (esperado: %s) - %s - %s\n", 
               test_cases[i].input,
               result ? "ACEITO" : "REJEITADO",
               test_cases[i].expected ? "ACEITO" : "REJEITADO",
               test_passed ? "PASS" : "FAIL",
               test_cases[i].description);
        
        if (test_passed) passed++;
        total++;
    }
    
    printf("Resultado: %d/%d testes passaram\n\n", passed, total);
    free_afd(afd);
}

void test_number_afd() {
    printf("=== Teste AFD Números ===\n");
    AFD* afd = create_number_afd();
    if (!afd) {
        printf("Erro: falha ao criar AFD\n");
        return;
    }
    
    struct {
        const char* input;
        bool expected;
        const char* description;
    } test_cases[] = {
        {"123", true, "Inteiro simples"},
        {"0", true, "Zero"},
        {"+42", true, "Inteiro positivo"},
        {"-17", true, "Inteiro negativo"},
        {"3.14", true, "Decimal simples"},
        {".5", true, "Decimal começando com ponto"},
        {"42.", true, "Decimal terminando com ponto"},
        {"2.5e10", true, "Notação científica positiva"},
        {"1.23e-5", true, "Notação científica negativa"},
        {"1e5", true, "Científica sem decimal"},
        {"+1.5E+10", true, "Completa com sinais"},
        {"e10", false, "Começando com e"},
        {"+", false, "Apenas sinal"},
        {".", false, "Apenas ponto"},
        {NULL, false, NULL}
    };
    
    int passed = 0, total = 0;
    for (int i = 0; test_cases[i].input != NULL; i++) {
        int chars_consumed;
        bool result = run_afd(afd, test_cases[i].input, &chars_consumed);
        bool test_passed = (result == test_cases[i].expected);
        
        printf("'%s': %s (esperado: %s) - %s - %s\n", 
               test_cases[i].input,
               result ? "ACEITO" : "REJEITADO",
               test_cases[i].expected ? "ACEITO" : "REJEITADO",
               test_passed ? "PASS" : "FAIL",
               test_cases[i].description);
        
        if (test_passed) passed++;
        total++;
    }
    
    printf("Resultado: %d/%d testes passaram\n\n", passed, total);
    free_afd(afd);
}

void test_string_afd() {
    printf("=== Teste AFD Strings ===\n");
    AFD* afd = create_string_afd();
    if (!afd) {
        printf("Erro: falha ao criar AFD\n");
        return;
    }
    
    const char* test_cases[] = {
        "\"hello\"", 
        "\"hello\\nworld\"", 
        "\"unterminated", 
        "\"\"", 
        "\"say \\\"hi\\\"\"",
        "\"path\\\\file\"",
        "\"tab\\there\"",
        "\"special!@#$%\"",
        "hello\"",
        NULL
    };
    bool expected[] = {true, true, false, true, true, true, true, true, false};
    
    for (int i = 0; test_cases[i] != NULL; i++) {
        int chars_consumed;
        bool result = run_afd(afd, test_cases[i], &chars_consumed);
        printf("'%s': %s (esperado: %s) - %s\n", 
               test_cases[i], 
               result ? "ACEITO" : "REJEITADO",
               expected[i] ? "ACEITO" : "REJEITADO",
               (result == expected[i]) ? "PASS" : "FAIL");
    }
    
    free_afd(afd);
    printf("\n");
}

void test_whitespace_afd() {
    printf("=== Teste AFD Whitespace ===\n");
    AFD* afd = create_whitespace_afd();
    if (!afd) {
        printf("Erro: falha ao criar AFD\n");
        return;
    }
    
    struct {
        const char* input;
        bool expected;
        const char* description;
    } test_cases[] = {
        {" ", true, "Espaço simples"},
        {"   ", true, "Múltiplos espaços"},
        {"\t", true, "Tab"},
        {"\n", true, "Newline"},
        {"\r", true, "Carriage return"},
        {" \t\n\r", true, "Combinação de whitespace"},
        {"", false, "String vazia"},
        {"a", false, "Não é whitespace"},
        {"\t\t  \n", true, "Múltiplos tipos"},
        {NULL, false, NULL}
    };
    
    int passed = 0, total = 0;
    for (int i = 0; test_cases[i].input != NULL; i++) {
        int chars_consumed;
        bool result = run_afd(afd, test_cases[i].input, &chars_consumed);
        bool test_passed = (result == test_cases[i].expected);
        
        printf("'%s': %s (esperado: %s) - %s - %s\n", 
               test_cases[i].input[0] == '\0' ? "(vazio)" : test_cases[i].input,
               result ? "ACEITO" : "REJEITADO",
               test_cases[i].expected ? "ACEITO" : "REJEITADO",
               test_passed ? "PASS" : "FAIL",
               test_cases[i].description);
        
        if (test_passed) passed++;
        total++;
    }
    
    printf("Resultado: %d/%d testes passaram\n\n", passed, total);
    free_afd(afd);
}

void test_line_comment_afd() {
    printf("=== Teste AFD Comentários de Linha ===\n");
    AFD* afd = create_line_comment_afd();
    if (!afd) {
        printf("Erro: falha ao criar AFD\n");
        return;
    }
    
    struct {
        const char* input;
        bool expected;
        const char* description;
    } test_cases[] = {
        {"// comentário", true, "Comentário simples"},
        {"//", true, "Comentário vazio"},
        {"// comentário\n", true, "Com newline"},
        {"// múltiplas // barras", true, "Múltiplas barras internas"},
        {"/", false, "Apenas uma barra"},
        {"/* não é linha */", false, "Comentário de bloco"},
        {"// com símbolos !@#$%", true, "Com símbolos especiais"},
        {NULL, false, NULL}
    };
    
    int passed = 0, total = 0;
    for (int i = 0; test_cases[i].input != NULL; i++) {
        int chars_consumed;
        bool result = run_afd(afd, test_cases[i].input, &chars_consumed);
        bool test_passed = (result == test_cases[i].expected);
        
        printf("'%s': %s (esperado: %s) - %s - %s\n", 
               test_cases[i].input,
               result ? "ACEITO" : "REJEITADO",
               test_cases[i].expected ? "ACEITO" : "REJEITADO",
               test_passed ? "PASS" : "FAIL",
               test_cases[i].description);
        
        if (test_passed) passed++;
        total++;
    }
    
    printf("Resultado: %d/%d testes passaram\n\n", passed, total);
    free_afd(afd);
}

void test_block_comment_afd() {
    printf("=== Teste AFD Comentários de Bloco ===\n");
    AFD* afd = create_block_comment_afd();
    if (!afd) {
        printf("Erro: falha ao criar AFD\n");
        return;
    }
    
    struct {
        const char* input;
        bool expected;
        const char* description;
    } test_cases[] = {
        {"/* comentário */", true, "Comentário simples"},
        {"/**/", true, "Comentário vazio"},
        {"/* múltiplas\nlinhas */", true, "Múltiplas linhas"},
        {"/* com * interno */", true, "Com asterisco interno"},
        {"/* aninhado /* não */ funciona */", true, "Com asterisco e barra"},
        {"/*", false, "Não fechado"},
        {"/* sem fechamento", false, "Sem fechamento"},
        {"não é comentário", false, "Texto normal"},
        {NULL, false, NULL}
    };
    
    int passed = 0, total = 0;
    for (int i = 0; test_cases[i].input != NULL; i++) {
        int chars_consumed;
        bool result = run_afd(afd, test_cases[i].input, &chars_consumed);
        bool test_passed = (result == test_cases[i].expected);
        
        printf("'%s': %s (esperado: %s) - %s - %s\n", 
               test_cases[i].input,
               result ? "ACEITO" : "REJEITADO",
               test_cases[i].expected ? "ACEITO" : "REJEITADO",
               test_passed ? "PASS" : "FAIL",
               test_cases[i].description);
        
        if (test_passed) passed++;
        total++;
    }
    
    printf("Resultado: %d/%d testes passaram\n\n", passed, total);
    free_afd(afd);
}

void test_operator_afd() {
    printf("=== Teste AFD Operadores ===\n");
    AFD* afd = create_operator_afd();
    if (!afd) {
        printf("Erro: falha ao criar AFD\n");
        return;
    }
    
    struct {
        const char* input;
        bool expected;
        const char* description;
    } test_cases[] = {
        {"+", true, "Adição"},
        {"-", true, "Subtração"},
        {"*", true, "Multiplicação"},
        {"/", true, "Divisão"},
        {"%", true, "Módulo"},
        {"%%", false, "Módulo"},
        {"=", true, "Atribuição"},
        {"==", true, "Igualdade"},
        {"!=", true, "Diferença"},
        {"<", true, "Menor que"},
        {"<=", true, "Menor igual"},
        {">", true, "Maior que"},
        {">=", true, "Maior igual"},
        {"&&", true, "E lógico"},
        {"&&&", false, "E lógico"},
        {"||", true, "Ou lógico"},
        {"|||", false, "Ou lógico"},
        {"|>", true, "Pipe"},
        {"=>", true, "Arrow"},
        {"!", true, "Negação"},
        {"&", true, "E bitwise"},
        {"|", true, "Ou bitwise"},
        {"===", false, "Tripla igualdade (inválido)"},
        {"", false, "String vazia"},
        {"a", false, "Não é operador"},
        {NULL, false, NULL}
    };
    
    int passed = 0, total = 0;
    for (int i = 0; test_cases[i].input != NULL; i++) {
        int chars_consumed = 0;
        bool accepted_by_afd = run_afd(afd, test_cases[i].input, &chars_consumed);
        
        size_t input_len = strlen(test_cases[i].input);
        bool result = accepted_by_afd && (chars_consumed == input_len);
        
        bool test_passed = (result == test_cases[i].expected);
        
        printf("'%s': %s (esperado: %s) - %s - %s\n", 
               test_cases[i].input,
               result ? "ACEITO" : "REJEITADO",
               test_cases[i].expected ? "ACEITO" : "REJEITADO",
               test_passed ? "PASS" : "FAIL",
               test_cases[i].description);
        
        if (test_passed) passed++;
        total++;
    }
    
    printf("Resultado: %d/%d testes passaram\n\n", passed, total);
    free_afd(afd);
}

void test_delimiter_afd() {
    printf("=== Teste AFD Delimitadores ===\n");
    AFD* afd = create_delimiter_afd();
    if (!afd) {
        printf("Erro: falha ao criar AFD\n");
        return;
    }
    
    struct {
        const char* input;
        bool expected;
        const char* description;
    } test_cases[] = {
        {"(", true, "Parêntese esquerdo"},
        {")", true, "Parêntese direito"},
        {"[", true, "Colchete esquerdo"},
        {"]", true, "Colchete direito"},
        {"{", true, "Chave esquerda"},
        {"}", true, "Chave direita"},
        {";", true, "Ponto e vírgula"},
        {",", true, "Vírgula"},
        {":", true, "Dois pontos"},
        {".", true, "Ponto"},
        {"..", true, "Range"},
        {"...", true, "Aceita apenas os dois primeiros pontos"},
        {"", false, "String vazia"},
        {"a", false, "Não é delimitador"},
        {NULL, false, NULL}
    };
    
    int passed = 0, total = 0;
    for (int i = 0; test_cases[i].input != NULL; i++) {
        int chars_consumed;
        bool result = run_afd(afd, test_cases[i].input, &chars_consumed);
        bool test_passed = (result == test_cases[i].expected);
        
        printf("'%s': %s (esperado: %s) - %s - %s\n", 
               test_cases[i].input,
               result ? "ACEITO" : "REJEITADO",
               test_cases[i].expected ? "ACEITO" : "REJEITADO",
               test_passed ? "PASS" : "FAIL",
               test_cases[i].description);
        
        if (test_passed) passed++;
        total++;
    }
    
    printf("Resultado: %d/%d testes passaram\n\n", passed, total);
    free_afd(afd);
}

void test_lexer_integration() {
    printf("=== Teste de Integração - Analisador Léxico Completo ===\n");
    
    // Test 1: Simple program
    printf("Teste 1: Programa simples\n");
    const char* code1 = "let x = 42";
    Token* tokens1 = tokenize(code1);
    
    if (!tokens1) {
        printf("Erro: falha ao tokenizar código\n");
        return;
    }
    
    printf("Código: %s\n", code1);
    printf("Tokens gerados:\n");
    for (int i = 0; tokens1[i].type != TOKEN_EOF; i++) {
        if (tokens1[i].type != TOKEN_WHITESPACE) {
            printf("  %s: '%s'\n", token_type_name(tokens1[i].type), 
                   tokens1[i].value ? tokens1[i].value : "(null)");
        }
    }
    
    // Free tokens1
    for (int i = 0; tokens1[i].type != TOKEN_EOF; i++) {
        free(tokens1[i].value);
    }
    free(tokens1);
    
    // Test 2: Function with string
    printf("\nTeste 2: Função com string e comentário\n");
    const char* code2 = "fn hello() { // comentário\n  return \"world\" }";
    Token* tokens2 = tokenize(code2);
    
    if (!tokens2) {
        printf("Erro: falha ao tokenizar código\n");
        return;
    }
    
    printf("Código: %s\n", code2);
    printf("Tokens gerados:\n");
    for (int i = 0; tokens2[i].type != TOKEN_EOF; i++) {
        if (tokens2[i].type != TOKEN_WHITESPACE) {
            printf("  %s: '%s' (linha: %d)\n", 
                   token_type_name(tokens2[i].type), 
                   tokens2[i].value ? tokens2[i].value : "(null)",
                   tokens2[i].line);
        }
    }
    
    // Free tokens2
    for (int i = 0; tokens2[i].type != TOKEN_EOF; i++) {
        free(tokens2[i].value);
    }
    free(tokens2);
    
    // Test 3: Complex expression
    printf("\nTeste 3: Expressão matemática com operadores compostos\n");
    const char* code3 = "data |> filter(|x| x >= 3.14) |> map(|y| y * 2)";
    Token* tokens3 = tokenize(code3);
    
    if (!tokens3) {
        printf("Erro: falha ao tokenizar código\n");
        return;
    }
    
    printf("Código: %s\n", code3);
    printf("Tokens gerados:\n");
    for (int i = 0; tokens3[i].type != TOKEN_EOF; i++) {
        if (tokens3[i].type != TOKEN_WHITESPACE) {
            printf("  %s: '%s'\n", token_type_name(tokens3[i].type), 
                   tokens3[i].value ? tokens3[i].value : "(null)");
        }
    }
    
    // Free tokens3
    for (int i = 0; tokens3[i].type != TOKEN_EOF; i++) {
        free(tokens3[i].value);
    }
    free(tokens3);
    
    // Test 4: Block comments
    printf("\nTeste 4: Comentários de bloco\n");
    const char* code4 = "/* Comentário\n   multilinhas */ let x = /* inline */ 42";
    Token* tokens4 = tokenize(code4);
    
    if (!tokens4) {
        printf("Erro: falha ao tokenizar código\n");
        return;
    }
    
    printf("Código: %s\n", code4);
    printf("Tokens gerados:\n");
    for (int i = 0; tokens4[i].type != TOKEN_EOF; i++) {
        if (tokens4[i].type != TOKEN_WHITESPACE) {
            printf("  %s: '%s'\n", token_type_name(tokens4[i].type), 
                   tokens4[i].value ? tokens4[i].value : "(null)");
        }
    }
    
    // Free tokens4
    for (int i = 0; tokens4[i].type != TOKEN_EOF; i++) {
        free(tokens4[i].value);
    }
    free(tokens4);
    
    printf("\n");
}

void test_keywords_and_types() {
    printf("=== Teste - Palavras-chave, Tipos e Booleanos ===\n");
    
    const char* test_code = "let data: DataFrame = true\n"
                           "fn process(x: Int) -> Float {\n"
                           "  if x > 0 {\n"
                           "    return 3.14\n"
                           "  } else {\n"
                           "    return false\n"
                           "  }\n"
                           "}";
    
    printf("Código de teste:\n%s\n\n", test_code);
    
    Token* tokens = tokenize(test_code);
    if (!tokens) {
        printf("Erro: falha ao tokenizar código\n");
        return;
    }
    
    int keyword_count = 0, type_count = 0, bool_count = 0;
    
    printf("Análise de tokens:\n");
    for (int i = 0; tokens[i].type != TOKEN_EOF; i++) {
        if (tokens[i].type != TOKEN_WHITESPACE) {
            printf("  %s: '%s'", token_type_name(tokens[i].type), 
                   tokens[i].value ? tokens[i].value : "(null)");
            
            if (tokens[i].type == TOKEN_KEYWORD) {
                keyword_count++;
                printf(" ← PALAVRA-CHAVE");
            } else if (tokens[i].type == TOKEN_TYPE) {
                type_count++;
                printf(" ← TIPO");
            } else if (tokens[i].type == TOKEN_BOOLEAN) {
                bool_count++;
                printf(" ← BOOLEANO");
            }
            printf("\n");
        }
    }
    
    printf("\nEstatísticas:\n");
    printf("  Palavras-chave encontradas: %d\n", keyword_count);
    printf("  Tipos encontrados: %d\n", type_count);
    printf("  Booleanos encontrados: %d\n", bool_count);
    
    // Free tokens
    for (int i = 0; tokens[i].type != TOKEN_EOF; i++) {
        free(tokens[i].value);
    }
    free(tokens);
    
    printf("\n");
}

void test_minimization_algorithm() {
    printf("=== Teste - Algoritmo de Minimização ===\n");
    
    AFD* original = create_identifier_afd();
    if (!original) {
        printf("Erro: falha ao criar AFD original\n");
        return;
    }
    
    printf("AFD original - Estados: %d\n", original->num_states);
    
    AFD* minimized = minimize_afd(original);
    if (!minimized) {
        printf("Erro: falha na minimização\n");
        free_afd(original);
        return;
    }
    
    printf("AFD minimizado - Estados: %d\n", minimized->num_states);
    
    const char* test_inputs[] = {
        "variable", "_private", "test123", "CamelCase", "123invalid", NULL
    };
    
    bool all_equivalent = true;
    printf("\nTeste de equivalência funcional:\n");
    
    for (int i = 0; test_inputs[i] != NULL; i++) {
        int chars1, chars2;
        bool result1 = run_afd(original, test_inputs[i], &chars1);
        bool result2 = run_afd(minimized, test_inputs[i], &chars2);
        
        bool equivalent = (result1 == result2 && chars1 == chars2);
        if (!equivalent) all_equivalent = false;
        
        printf("  '%s': Original=%s, Minimized=%s - %s\n",
               test_inputs[i],
               result1 ? "ACEITO" : "REJEITADO",
               result2 ? "ACEITO" : "REJEITADO",
               equivalent ? "EQUIVALENTE" : "DIFERENTE");
    }
    
    printf("\nResultado da minimização: %s\n", 
           all_equivalent ? "SUCESSO (funcionalidade preservada)" : 
                          "FALHA (funcionalidade alterada)");
    
    free_afd(original);
    if (minimized != original) {
        free_afd(minimized);
    }
    
    printf("\n");
}

void test_error_handling() {
    printf("=== Teste - Tratamento de Erros ===\n");
    
    const char* error_cases[] = {
        "@#$%^&",
        "\"string não terminada",
        "123.45.67",
        "/* comentário não fechado",
        "variável_com_çentão",
        NULL
    };
    
    for (int i = 0; error_cases[i] != NULL; i++) {
        printf("Testando código com erro: '%s'\n", error_cases[i]);
        Token* tokens = tokenize(error_cases[i]);
        
        if (!tokens) {
            printf("  Erro: falha ao tokenizar\n");
            continue;
        }
        
        int unknown_count = 0;
        printf("  Tokens gerados:\n");
        for (int j = 0; tokens[j].type != TOKEN_EOF; j++) {
            if (tokens[j].type != TOKEN_WHITESPACE) {
                printf("    %s: '%s'\n", token_type_name(tokens[j].type), 
                       tokens[j].value ? tokens[j].value : "(null)");
                if (tokens[j].type == TOKEN_UNKNOWN) {
                    unknown_count++;
                }
            }
        }
        
        printf("  Tokens desconhecidos: %d\n", unknown_count);
        
        // Free tokens
        for (int j = 0; tokens[j].type != TOKEN_EOF; j++) {
            free(tokens[j].value);
        }
        free(tokens);
        
        printf("\n");
    }
}

void test_lexer() {
    printf("=== Teste Analisador Léxico Completo ===\n");
    const char* code = "let x = 42\nlet name = \"DataLang\"\nif (x > 0) { return true }";
    
    Token* tokens = tokenize(code);
    if (!tokens) {
        printf("Erro: falha ao tokenizar código\n");
        return;
    }
    
    for (int i = 0; tokens[i].type != TOKEN_EOF; i++) {
        if (tokens[i].type != TOKEN_WHITESPACE) {
            printf("Token: %-12s | Valor: %-15s | Linha: %d | Coluna: %d\n",
                   token_type_name(tokens[i].type),
                   tokens[i].value ? tokens[i].value : "(null)",
                   tokens[i].line,
                   tokens[i].column);
        }
    }
    
    // Free tokens
    for (int i = 0; tokens[i].type != TOKEN_EOF; i++) {
        free(tokens[i].value);
    }
    free(tokens);
    
    printf("\n");
}

void run_comprehensive_tests() {
    printf("DataLang AFDs - Suite Completa de Testes\n");
    printf("========================================\n\n");
    
    // Run all individual AFD tests
    test_identifier_afd();
    test_number_afd();
    test_string_afd();
    test_whitespace_afd();
    test_line_comment_afd();
    test_block_comment_afd();
    test_operator_afd();
    test_delimiter_afd();
    
    // Run integration tests
    test_lexer_integration();
    test_keywords_and_types();
    test_minimization_algorithm();
    test_error_handling();
    
    printf("========================================\n");
    printf("Todos os testes concluídos!\n");
    printf("========================================\n");
}

#ifndef LIB_BUILD

int main() {
    run_comprehensive_tests();
    return 0;
}
#endif