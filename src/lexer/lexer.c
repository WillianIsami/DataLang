/*
 * DataLang - Analisador Léxico Final
 * Implementação do analisador léxico usando o AFD gerado pela conversão AFN->AFD.
 */
#define _GNU_SOURCE 

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>
#include "datalang_afn.h"
#include "afn_to_afd.h"

// ==================== TABELA DE PALAVRAS-CHAVE ====================

typedef struct {
    const char* keyword;
    TokenType token_type;
} KeywordEntry;

static const KeywordEntry keywords[] = {
    {"let", TOKEN_LET},
    {"fn", TOKEN_FN},
    {"data", TOKEN_DATA},
    {"filter", TOKEN_FILTER},
    {"map", TOKEN_MAP},
    {"reduce", TOKEN_REDUCE},
    {"import", TOKEN_IMPORT},
    {"export", TOKEN_EXPORT},
    {"if", TOKEN_IF},
    {"else", TOKEN_ELSE},
    {"for", TOKEN_FOR},
    {"in", TOKEN_IN},
    {"return", TOKEN_RETURN},
    {"load", TOKEN_LOAD},
    {"save", TOKEN_SAVE},
    {"select", TOKEN_SELECT},
    {"groupby", TOKEN_GROUPBY},
    {"sum", TOKEN_SUM},
    {"mean", TOKEN_MEAN},
    {"count", TOKEN_COUNT},
    {"min", TOKEN_MIN},
    {"max", TOKEN_MAX},
    {"as", TOKEN_AS},
    {"true", TOKEN_TRUE},
    {"false", TOKEN_FALSE},
    {"Int", TOKEN_INT_TYPE},
    {"Float", TOKEN_FLOAT_TYPE},
    {"String", TOKEN_STRING_TYPE},
    {"Bool", TOKEN_BOOL_TYPE},
    {"DataFrame", TOKEN_DATAFRAME_TYPE},
    {"Vector", TOKEN_VECTOR_TYPE},
    {"Series", TOKEN_SERIES_TYPE},
    {NULL, TOKEN_ERROR}
};

// Busca palavra-chave na tabela
TokenType lookup_keyword(const char* str) {
    for (int i = 0; keywords[i].keyword != NULL; i++) {
        if (strcmp(str, keywords[i].keyword) == 0) {
            return keywords[i].token_type;
        }
    }
    return TOKEN_IDENTIFIER;
}

// ==================== NOMES DOS TOKENS ====================

const char* token_type_name(TokenType type) {
    switch (type) {
        case TOKEN_LET: return "LET";
        case TOKEN_FN: return "FN";
        case TOKEN_DATA: return "DATA";
        case TOKEN_FILTER: return "FILTER";
        case TOKEN_MAP: return "MAP";
        case TOKEN_REDUCE: return "REDUCE";
        case TOKEN_IMPORT: return "IMPORT";
        case TOKEN_EXPORT: return "EXPORT";
        case TOKEN_IF: return "IF";
        case TOKEN_ELSE: return "ELSE";
        case TOKEN_FOR: return "FOR";
        case TOKEN_IN: return "IN";
        case TOKEN_RETURN: return "RETURN";
        case TOKEN_LOAD: return "LOAD";
        case TOKEN_SAVE: return "SAVE";
        case TOKEN_SELECT: return "SELECT";
        case TOKEN_GROUPBY: return "GROUPBY";
        case TOKEN_SUM: return "SUM";
        case TOKEN_MEAN: return "MEAN";
        case TOKEN_COUNT: return "COUNT";
        case TOKEN_MIN: return "MIN";
        case TOKEN_MAX: return "MAX";
        case TOKEN_AS: return "AS";
        case TOKEN_INT_TYPE: return "INT_TYPE";
        case TOKEN_FLOAT_TYPE: return "FLOAT_TYPE";
        case TOKEN_STRING_TYPE: return "STRING_TYPE";
        case TOKEN_BOOL_TYPE: return "BOOL_TYPE";
        case TOKEN_DATAFRAME_TYPE: return "DATAFRAME_TYPE";
        case TOKEN_VECTOR_TYPE: return "VECTOR_TYPE";
        case TOKEN_SERIES_TYPE: return "SERIES_TYPE";
        case TOKEN_IDENTIFIER: return "IDENTIFIER";
        case TOKEN_INTEGER: return "INTEGER";
        case TOKEN_FLOAT: return "FLOAT";
        case TOKEN_STRING: return "STRING";
        case TOKEN_TRUE: return "TRUE";
        case TOKEN_FALSE: return "FALSE";
        case TOKEN_PLUS: return "PLUS";
        case TOKEN_MINUS: return "MINUS";
        case TOKEN_MULT: return "MULT";
        case TOKEN_DIV: return "DIV";
        case TOKEN_MOD: return "MOD";
        case TOKEN_ASSIGN: return "ASSIGN";
        case TOKEN_EQUAL: return "EQUAL";
        case TOKEN_NOT_EQUAL: return "NOT_EQUAL";
        case TOKEN_LESS: return "LESS";
        case TOKEN_LESS_EQUAL: return "LESS_EQUAL";
        case TOKEN_GREATER: return "GREATER";
        case TOKEN_GREATER_EQUAL: return "GREATER_EQUAL";
        case TOKEN_AND: return "AND";
        case TOKEN_OR: return "OR";
        case TOKEN_NOT: return "NOT";
        case TOKEN_ARROW: return "ARROW";
        case TOKEN_PIPE: return "PIPE";
        case TOKEN_LPAREN: return "LPAREN";
        case TOKEN_RPAREN: return "RPAREN";
        case TOKEN_LBRACKET: return "LBRACKET";
        case TOKEN_RBRACKET: return "RBRACKET";
        case TOKEN_LBRACE: return "LBRACE";
        case TOKEN_RBRACE: return "RBRACE";
        case TOKEN_SEMICOLON: return "SEMICOLON";
        case TOKEN_COMMA: return "COMMA";
        case TOKEN_DOT: return "DOT";
        case TOKEN_COLON: return "COLON";
        case TOKEN_RANGE: return "RANGE";
        case TOKEN_WHITESPACE: return "WHITESPACE";
        case TOKEN_COMMENT: return "COMMENT";
        case TOKEN_ERROR: return "ERROR";
        case TOKEN_EOF: return "EOF";
        default: return "UNKNOWN";
    }
}

// ==================== ANALISADOR LÉXICO ====================

typedef struct {
    const char* input;
    int position;
    int line;
    int column;
    int length;
    AFD* afd;
} Lexer;

Lexer* create_lexer(const char* input, AFD* afd) {
    Lexer* lexer = (Lexer*)malloc(sizeof(Lexer));
    if (!lexer) return NULL;
    
    lexer->input = input;
    lexer->position = 0;
    lexer->line = 1;
    lexer->column = 1;
    lexer->length = strlen(input);
    lexer->afd = afd;
    
    return lexer;
}

void free_lexer(Lexer* lexer) {
    free(lexer);
}

// ==================== RECONHECIMENTO DE TOKENS COM AFD ====================

/*
 * Reconhece o próximo token usando o AFD.
 * Implementa a estratégia do "match mais longo" (maximal munch).
 */
Token recognize_token(Lexer* lexer) {
    Token token = {0};
    token.line = lexer->line;
    token.column = lexer->column;
    
    // Se chegou ao fim da entrada
    if (lexer->position >= lexer->length) {
        token.type = TOKEN_EOF;
        token.value = NULL;
        token.length = 0;
        return token;
    }
    
    int current_state = lexer->afd->start_state;
    int last_final_state = -1;
    int last_final_position = -1;
    int start_position = lexer->position;
    
    while (lexer->position < lexer->length) {
        unsigned char c = (unsigned char)lexer->input[lexer->position];
        
        // Verifica se há transição válida
        if (c >= lexer->afd->alphabet_size) {
            break;
        }
        
        int next_state = lexer->afd->transition_table[current_state][c];
        
        // Se não há transição válida, para
        if (next_state < 0) {
            break;
        }
        
        current_state = next_state;
        lexer->position++;
        
        // Se chegou a um estado final, registra
        if (lexer->afd->final_states[current_state]) {
            last_final_state = current_state;
            last_final_position = lexer->position;
        }
    }
    
    // Se encontrou um estado final, aceita o token
    if (last_final_state >= 0) {
        lexer->position = last_final_position;
        token.length = last_final_position - start_position;
        token.value = strndup(&lexer->input[start_position], token.length);
        token.type = lexer->afd->token_types[last_final_state];
        
        // Atualiza posição (linha/coluna)
        for (int i = start_position; i < last_final_position; i++) {
            if (lexer->input[i] == '\n') {
                lexer->line++;
                lexer->column = 1;
            } else {
                lexer->column++;
            }
        }
        
        // Classifica identificadores (palavras-chave vs identificadores)
        if (token.type == TOKEN_IDENTIFIER) {
            TokenType keyword_type = lookup_keyword(token.value);
            token.type = keyword_type;
        }
        
        return token;
    }
    
    // Erro léxico: caractere inválido
    token.type = TOKEN_ERROR;
    token.value = strndup(&lexer->input[start_position], 1);
    token.length = 1;
    lexer->position = start_position + 1;
    lexer->column++;
    
    return token;
}

// ==================== TOKENIZAÇÃO COMPLETA ====================

typedef struct {
    Token* tokens;
    int count;
    int capacity;
} TokenStream;

TokenStream* create_token_stream() {
    TokenStream* stream = (TokenStream*)malloc(sizeof(TokenStream));
    stream->capacity = 100;
    stream->count = 0;
    stream->tokens = (Token*)malloc(stream->capacity * sizeof(Token));
    return stream;
}

void add_token(TokenStream* stream, Token token) {
    if (stream->count >= stream->capacity) {
        stream->capacity *= 2;
        stream->tokens = (Token*)realloc(stream->tokens, stream->capacity * sizeof(Token));
    }
    stream->tokens[stream->count++] = token;
}

void free_token_stream(TokenStream* stream) {
    if (stream) {
        for (int i = 0; i < stream->count; i++) {
            free(stream->tokens[i].value);
        }
        free(stream->tokens);
        free(stream);
    }
}

TokenStream* tokenize(const char* input, AFD* afd) {
    Lexer* lexer = create_lexer(input, afd);
    TokenStream* stream = create_token_stream();
    
    while (true) {
        Token token = recognize_token(lexer);
        
        // Filtra whitespace e comentários
        if (token.type != TOKEN_WHITESPACE && token.type != TOKEN_COMMENT) {
            add_token(stream, token);
        } else {
            free(token.value);
        }
        
        if (token.type == TOKEN_EOF || token.type == TOKEN_ERROR) break;
    }
    
    free_lexer(lexer);
    return stream;
}

// ==================== INTEGRAÇÃO COM AFN->AFD ====================

// A função afn_to_afd está declarada em afn_to_afd.h

AFD* create_datalang_afd_from_afn() {
    printf("Criando AFD a partir do AFN unificado...\n");
    
    // Cria o AFN unificado
    AFN* afn = create_unified_datalang_afn();
    if (!afn) {
        printf("Erro ao criar AFN unificado\n");
        return NULL;
    }
    
    // Converte para AFD
    AFD* afd = afn_to_afd(afn);
    
    // Libera o AFN (não é mais necessário)
    free_afn(afn);
    
    if (!afd) {
        printf("Erro na conversão AFN -> AFD\n");
        return NULL;
    }
    
    printf("AFD criado com sucesso: %d estados\n", afd->num_states);
    return afd;
}

// ==================== FUNÇÕES DE VISUALIZAÇÃO ====================

void print_tokens(TokenStream* stream) {
    printf("\n╔════════════════════════════════════════════════════════════╗\n");
    printf("║           TOKENS RECONHECIDOS PELO AFD                   ║\n");
    printf("╚════════════════════════════════════════════════════════════╝\n\n");
    
    for (int i = 0; i < stream->count; i++) {
        Token* t = &stream->tokens[i];
        
        printf("[%3d] L%03d:C%03d  %-15s", 
               i, t->line, t->column, token_type_name(t->type));
        
        if (t->value) {
            if (t->type == TOKEN_STRING) {
                printf(" %s", t->value);
            } else {
                printf(" '%s'", t->value);
            }
        }
        printf("\n");
    }
    
    printf("\n━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n");
    printf("Total: %d tokens (excluindo whitespace)\n", stream->count - 1);
    printf("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n\n");
}

void print_afd_info(AFD* afd) {
    printf("\n   Informações do AFD:\n");
    printf("   - Estados: %d\n", afd->num_states);
    printf("   - Alfabeto: %d símbolos\n", afd->alphabet_size);
    printf("   - Estado inicial: %d\n", afd->start_state);
    
    int final_count = 0;
    for (int i = 0; i < afd->num_states; i++) {
        if (afd->final_states[i]) final_count++;
    }
    printf("   - Estados finais: %d\n", final_count);
}

// ==================== TESTES ====================

void test_simple_code() {
    printf("\nTESTE 1: Código Simples\n");
    printf("════════════════════════════════════════════════════════════\n");
    
    const char* test_code = 
        "let x = 42\n"
        "let name = \"DataLang\"\n"
        "if (x > 0) { return true }";
    
    printf("Código:\n%s\n", test_code);
    
    AFD* afd = create_datalang_afd_from_afn();
    if (!afd) return;
    
    print_afd_info(afd);
    
    TokenStream* stream = tokenize(test_code, afd);
    print_tokens(stream);
    
    free_token_stream(stream);
    free_afd(afd);
}

void test_complex_code() {
    printf("\nTESTE 2: Código Complexo\n");
    printf("════════════════════════════════════════════════════════════\n");
    
    const char* test_code = 
        "let data = load(\"file.csv\")\n"
        "let result = data \n"
        "    |> filter(|row| row.age >= 18) \n"
        "    |> map(|x| x.salary * 1.1)\n"
        "    |> reduce(0, |acc, val| acc + val)\n"
        "\n"
        "fn calculate(a: Int, b: Float) -> Float {\n"
        "    if a > b {\n"
        "        return a * 3.14\n"
        "    } else {\n"
        "        return b + 2.5\n"
        "    }\n"
        "}";
    
    printf("Código:\n%s\n", test_code);
    
    AFD* afd = create_datalang_afd_from_afn();
    if (!afd) return;
    
    TokenStream* stream = tokenize(test_code, afd);
    print_tokens(stream);
    
    free_token_stream(stream);
    free_afd(afd);
}

void test_edge_cases() {
    printf("\nTESTE 3: Casos Especiais\n");
    printf("════════════════════════════════════════════════════════════\n");
    
    const char* test_code = 
        "let _private = 123\n"
        "let var123 = 45.67\n"
        "let text = \"Hello\\nWorld\\t!\"\n"
        "let range = 1..10\n"
        "let result = (a + b) * c / d % e";
    
    printf("Código:\n%s\n", test_code);
    
    AFD* afd = create_datalang_afd_from_afn();
    if (!afd) return;
    
    TokenStream* stream = tokenize(test_code, afd);
    print_tokens(stream);
    
    free_token_stream(stream);
    free_afd(afd);
}

void test_error_handling() {
    printf("\nTESTE 4: Tratamento de Erros\n");
    printf("════════════════════════════════════════════════════════════\n");
    
    const char* test_code = 
        "let x = 42$\n"           // Caractere inválido
        "let y = 3.14.15\n"       // Número mal formado
        "let z = \"unterminated\n" // String não fechada
        "let w = @invalid";       // Símbolo inválido
    
    printf("Código com erros:\n%s\n", test_code);
    
    AFD* afd = create_datalang_afd_from_afn();
    if (!afd) return;
    
    TokenStream* stream = tokenize(test_code, afd);
    
    printf("Tokens (incluindo erros):\n");
    for (int i = 0; i < stream->count; i++) {
        Token* t = &stream->tokens[i];
        printf("  %-15s", token_type_name(t->type));
        if (t->value) printf(" '%s'", t->value);
        if (t->type == TOKEN_ERROR) printf(" ← ERRO LÉXICO");
        printf("\n");
    }
    
    free_token_stream(stream);
    free_afd(afd);
}

// ==================== FUNÇÃO PRINCIPAL ====================

#ifndef LIB_BUILD
int main() {
    printf("\n╔════════════════════════════════════════════════════════════╗\n");
    printf("║          ANALISADOR LÉXICO DATALANG - AFD UNIFICADO       ║\n");
    printf("╚════════════════════════════════════════════════════════════╝\n\n");
    
    printf("Este analisador léxico usa um AFD unificado gerado a partir\n");
    printf("de um AFN através do algoritmo de construção de subconjuntos.\n\n");
    
    // Executa todos os testes
    test_simple_code();
    test_complex_code();
    test_edge_cases();
    test_error_handling();
    
    printf("\nTodos os testes concluídos!\n");
    printf("O analisador léxico está funcionando com o AFD unificado.\n\n");
    
    return 0;
}
#endif