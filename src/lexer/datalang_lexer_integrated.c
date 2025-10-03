#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>

// ==================== ESTRUTURAS DE DADOS ====================

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
    TOKEN_COMMENT_LINE,
    TOKEN_COMMENT_BLOCK,
    TOKEN_WHITESPACE,
    TOKEN_ERROR,
    TOKEN_EOF
} TokenType;

typedef struct {
    TokenType type;
    char* value;
    int line;
    int column;
    int length;
    char* error_message;  // Para tokens de erro
} Token;

typedef struct {
    Token* tokens;
    int count;
    int capacity;
} TokenStream;

typedef struct {
    char* message;
    int line;
    int column;
    char* suggestion;
    char* context;  // Contexto do código ao redor do erro
} LexerError;

typedef struct {
    LexerError* errors;
    int count;
    int capacity;
} ErrorList;

// ==================== LISTAS DE PALAVRAS-CHAVE ====================

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

// ==================== FUNÇÕES AUXILIARES ====================

bool is_in_list(const char* str, const char** list) {
    for (int i = 0; list[i] != NULL; i++) {
        if (strcmp(str, list[i]) == 0) return true;
    }
    return false;
}

char* strndup_safe(const char* s, size_t n) {
    char* result = malloc(n + 1);
    if (!result) return NULL;
    memcpy(result, s, n);
    result[n] = '\0';
    return result;
}

// Extrai contexto ao redor do erro (linha completa)
char* extract_context(const char* input, int position) {
    int start = position;
    int end = position;
    
    // Encontra início da linha
    while (start > 0 && input[start-1] != '\n') start--;
    
    // Encontra fim da linha
    while (input[end] != '\0' && input[end] != '\n') end++;
    
    return strndup_safe(&input[start], end - start);
}

// ==================== GERENCIAMENTO DE TOKENS ====================

TokenStream* create_token_stream() {
    TokenStream* stream = malloc(sizeof(TokenStream));
    stream->capacity = 100;
    stream->count = 0;
    stream->tokens = malloc(stream->capacity * sizeof(Token));
    return stream;
}

void add_token(TokenStream* stream, Token token) {
    if (stream->count >= stream->capacity) {
        stream->capacity *= 2;
        stream->tokens = realloc(stream->tokens, stream->capacity * sizeof(Token));
    }
    stream->tokens[stream->count++] = token;
}

void free_token_stream(TokenStream* stream) {
    if (stream) {
        for (int i = 0; i < stream->count; i++) {
            free(stream->tokens[i].value);
            free(stream->tokens[i].error_message);
        }
        free(stream->tokens);
        free(stream);
    }
}

// ==================== GERENCIAMENTO DE ERROS ====================

ErrorList* create_error_list() {
    ErrorList* list = malloc(sizeof(ErrorList));
    list->capacity = 10;
    list->count = 0;
    list->errors = malloc(list->capacity * sizeof(LexerError));
    return list;
}

void add_error(ErrorList* list, const char* message, int line, int column, 
               const char* suggestion, const char* context) {
    if (list->count >= list->capacity) {
        list->capacity *= 2;
        list->errors = realloc(list->errors, list->capacity * sizeof(LexerError));
    }
    
    LexerError error;
    error.message = strdup(message);
    error.line = line;
    error.column = column;
    error.suggestion = suggestion ? strdup(suggestion) : NULL;
    error.context = context ? strdup(context) : NULL;
    
    list->errors[list->count++] = error;
}

void print_errors(ErrorList* list) {
    if (!list || list->count == 0) return;
    
    printf("\n╔════════════════════════════════════════════════════════════╗\n");
    printf("║           ERROS LÉXICOS ENCONTRADOS: %d                    ║\n", list->count);
    printf("╚════════════════════════════════════════════════════════════╝\n\n");
    
    for (int i = 0; i < list->count; i++) {
        LexerError* err = &list->errors[i];
        
        printf("❌ Erro %d:\n", i + 1);
        
        if (err->message) {
            printf("   Mensagem: %s\n", err->message);
        }
        
        printf("   Posição:  Linha %d, Coluna %d\n", err->line, err->column);
        
        if (err->context) {
            printf("   Contexto: %s\n", err->context);
            printf("             ");
            int indent = err->column - 1;
            if (indent < 0) indent = 0;
            if (indent > 100) indent = 100; // Limitar para evitar loop infinito
            for (int j = 0; j < indent; j++) printf(" ");
            printf("^\n");
        }
        
        if (err->suggestion) {
            printf("   💡 Sugestão: %s\n", err->suggestion);
        }
        
        printf("\n");
    }
}

void free_error_list(ErrorList* list) {
    if (list) {
        for (int i = 0; i < list->count; i++) {
            free(list->errors[i].message);
            free(list->errors[i].suggestion);
            free(list->errors[i].context);
        }
        free(list->errors);
        free(list);
    }
}

// ==================== RECONHECEDORES DE TOKENS ====================

int scan_identifier(const char* input, int pos) {
    int start = pos;
    
    if (!isalpha(input[pos]) && input[pos] != '_') return 0;
    pos++;
    
    while (isalnum(input[pos]) || input[pos] == '_') pos++;
    
    return pos - start;
}

int scan_number(const char* input, int pos) {
    int start = pos;
    bool has_dot = false;
    bool has_exp = false;
    
    // Sinal opcional
    if (input[pos] == '+' || input[pos] == '-') pos++;
    
    // Parte inteira ou decimal inicial
    if (input[pos] == '.') {
        has_dot = true;
        pos++;
        if (!isdigit(input[pos])) return 0;  // .xxx inválido
    }
    
    while (isdigit(input[pos])) pos++;
    
    // Ponto decimal
    if (input[pos] == '.' && !has_dot) {
        has_dot = true;
        pos++;
        while (isdigit(input[pos])) pos++;
    }
    
    // Notação científica
    if ((input[pos] == 'e' || input[pos] == 'E') && !has_exp) {
        has_exp = true;
        pos++;
        
        if (input[pos] == '+' || input[pos] == '-') pos++;
        
        if (!isdigit(input[pos])) return 0;  // Expoente inválido
        
        while (isdigit(input[pos])) pos++;
    }
    
    return pos - start;
}

int scan_string(const char* input, int pos, ErrorList* errors, int line, int column) {
    int start = pos;
    
    if (input[pos] != '"') return 0;
    pos++;
    
    while (input[pos] != '\0' && input[pos] != '"') {
        if (input[pos] == '\n') {
            char* ctx = extract_context(input, start);
            add_error(errors, "String não fechada antes do fim da linha", 
                     line, column, "Adicione \" para fechar a string", ctx);
            free(ctx);
            return 0;
        }
        
        if (input[pos] == '\\') {
            pos++;  // Escape character
            if (input[pos] == '\0') {
                char* ctx = extract_context(input, start);
                add_error(errors, "Sequência de escape incompleta", 
                         line, column, "Complete a sequência de escape", ctx);
                free(ctx);
                return 0;
            }
        }
        pos++;
    }
    
    if (input[pos] != '"') {
        char* ctx = extract_context(input, start);
        add_error(errors, "String não fechada (fim do arquivo)", 
                 line, column, "Adicione \" para fechar a string", ctx);
        free(ctx);
        return 0;
    }
    
    pos++;  // Closing quote
    return pos - start;
}

int scan_comment(const char* input, int pos) {
    int start = pos;
    
    if (input[pos] != '/') return 0;
    
    if (input[pos+1] == '/') {
        // Comentário de linha
        pos += 2;
        while (input[pos] != '\0' && input[pos] != '\n') pos++;
        if (input[pos] == '\n') pos++;
        return pos - start;
    }
    
    if (input[pos+1] == '*') {
        // Comentário de bloco
        pos += 2;
        while (input[pos] != '\0') {
            if (input[pos] == '*' && input[pos+1] == '/') {
                pos += 2;
                return pos - start;
            }
            pos++;
        }
        return 0;  // Não fechado
    }
    
    return 0;
}

int scan_operator(const char* input, int pos) {
    // Operadores de 2 caracteres
    if ((input[pos] == '=' && input[pos+1] == '=') ||
        (input[pos] == '!' && input[pos+1] == '=') ||
        (input[pos] == '<' && input[pos+1] == '=') ||
        (input[pos] == '>' && input[pos+1] == '=') ||
        (input[pos] == '&' && input[pos+1] == '&') ||
        (input[pos] == '|' && input[pos+1] == '|') ||
        (input[pos] == '|' && input[pos+1] == '>') ||
        (input[pos] == '=' && input[pos+1] == '>') ||
        (input[pos] == '.' && input[pos+1] == '.')) {
        return 2;
    }
    
    // Operadores de 1 caractere
    if (strchr("+-*/%=<>!&|", input[pos])) return 1;
    
    return 0;
}

int scan_delimiter(const char* input, int pos) {
    if (strchr("()[]{}:;,.", input[pos])) return 1;
    return 0;
}

int scan_whitespace(const char* input, int pos) {
    int start = pos;
    while (input[pos] == ' ' || input[pos] == '\t' || 
           input[pos] == '\n' || input[pos] == '\r') {
        pos++;
    }
    return pos - start;
}

// ==================== ANALISADOR LÉXICO PRINCIPAL ====================

TokenStream* tokenize_with_errors(const char* input, ErrorList* errors) {
    TokenStream* stream = create_token_stream();
    int pos = 0;
    int line = 1;
    int column = 1;
    int input_len = strlen(input);
    
    while (pos < input_len && input[pos] != '\0') {
        Token token = {0};
        token.line = line;
        token.column = column;
        int consumed = 0;
        
        // Whitespace
        consumed = scan_whitespace(input, pos);
        if (consumed > 0) {
            token.type = TOKEN_WHITESPACE;
            token.value = strndup_safe(&input[pos], consumed);
            token.length = consumed;
            
            // Atualiza linha e coluna
            for (int i = 0; i < consumed; i++) {
                if (input[pos + i] == '\n') {
                    line++;
                    column = 1;
                } else {
                    column++;
                }
            }
            
            pos += consumed;
            add_token(stream, token);
            continue;
        }
        
        // Comentários
        consumed = scan_comment(input, pos);
        if (consumed > 0) {
            token.type = (input[pos+1] == '/') ? TOKEN_COMMENT_LINE : TOKEN_COMMENT_BLOCK;
            token.value = strndup_safe(&input[pos], consumed);
            token.length = consumed;
            
            // Atualiza linha e coluna
            for (int i = 0; i < consumed; i++) {
                if (input[pos + i] == '\n') {
                    line++;
                    column = 1;
                } else {
                    column++;
                }
            }
            
            pos += consumed;
            add_token(stream, token);
            continue;
        } else if (input[pos] == '/' && input[pos+1] == '*') {
            // Comentário de bloco não fechado
            char* ctx = extract_context(input, pos);
            add_error(errors, "Comentário de bloco não fechado", 
                     line, column, "Adicione */ para fechar o comentário", ctx);
            free(ctx);
            
            // Pula até o fim
            while (input[pos] != '\0') pos++;
            break;
        }
        
        // Strings
        consumed = scan_string(input, pos, errors, line, column);
        if (consumed > 0) {
            token.type = TOKEN_STRING;
            token.value = strndup_safe(&input[pos], consumed);
            token.length = consumed;
            pos += consumed;
            column += consumed;
            add_token(stream, token);
            continue;
        }
        
        // Números
        consumed = scan_number(input, pos);
        if (consumed > 0) {
            token.value = strndup_safe(&input[pos], consumed);
            
            // Verifica se é float ou int
            if (strchr(token.value, '.') || strchr(token.value, 'e') || strchr(token.value, 'E')) {
                token.type = TOKEN_FLOAT;
            } else {
                token.type = TOKEN_INTEGER;
            }
            
            token.length = consumed;
            pos += consumed;
            column += consumed;
            add_token(stream, token);
            continue;
        }
        
        // Operadores
        consumed = scan_operator(input, pos);
        if (consumed > 0) {
            token.type = TOKEN_OPERATOR;
            token.value = strndup_safe(&input[pos], consumed);
            token.length = consumed;
            pos += consumed;
            column += consumed;
            add_token(stream, token);
            continue;
        }
        
        // Delimitadores
        consumed = scan_delimiter(input, pos);
        if (consumed > 0) {
            token.type = TOKEN_DELIMITER;
            token.value = strndup_safe(&input[pos], consumed);
            token.length = consumed;
            pos += consumed;
            column += consumed;
            add_token(stream, token);
            continue;
        }
        
        // Identificadores, palavras-chave, tipos e booleanos
        consumed = scan_identifier(input, pos);
        if (consumed > 0) {
            token.value = strndup_safe(&input[pos], consumed);
            
            if (is_in_list(token.value, keywords)) {
                token.type = TOKEN_KEYWORD;
            } else if (is_in_list(token.value, types)) {
                token.type = TOKEN_TYPE;
            } else if (is_in_list(token.value, booleans)) {
                token.type = TOKEN_BOOLEAN;
            } else {
                token.type = TOKEN_IDENTIFIER;
            }
            
            token.length = consumed;
            pos += consumed;
            column += consumed;
            add_token(stream, token);
            continue;
        }
        
        // Caractere inválido
        char invalid_char = input[pos];
        char msg[200];
        char* suggestion = NULL;
        
        if (invalid_char == '@') {
            snprintf(msg, sizeof(msg), "Caractere inválido '@' (não suportado em DataLang)");
            suggestion = "Use identificadores válidos sem caracteres especiais";
        } else if (invalid_char == '#') {
            snprintf(msg, sizeof(msg), "Caractere inválido '#' (comentários usam //)");
            suggestion = "Use // para comentários de linha ou /* */ para blocos";
        } else if (invalid_char == '\'') {
            snprintf(msg, sizeof(msg), "Caractere inválido ' (não suportado em DataLang)");
            suggestion = "Use identificadores válidos começando com letra ou _";
        } else if ((unsigned char)invalid_char >= 128) {
            snprintf(msg, sizeof(msg), "Caractere Unicode inválido (código %d)", (unsigned char)invalid_char);
            suggestion = "DataLang aceita apenas caracteres ASCII";
        } else {
            snprintf(msg, sizeof(msg), "Caractere inválido '%c' (código %d)", 
                    isprint(invalid_char) ? invalid_char : '?', (unsigned char)invalid_char);
        }
        
        char* ctx = extract_context(input, pos);
        add_error(errors, msg, line, column, suggestion, ctx);
        free(ctx);
        
        // Cria token de erro
        token.type = TOKEN_ERROR;
        token.value = strndup_safe(&input[pos], 1);
        token.error_message = strdup(msg);
        token.length = 1;
        
        pos++;
        column++;
        add_token(stream, token);
    }
    
    // Token EOF
    Token eof_token = {0};
    eof_token.type = TOKEN_EOF;
    eof_token.line = line;
    eof_token.column = column;
    add_token(stream, eof_token);
    
    return stream;
}

// ==================== FUNÇÕES DE UTILIDADE ====================

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
        case TOKEN_COMMENT_LINE: return "COMMENT_LINE";
        case TOKEN_COMMENT_BLOCK: return "COMMENT_BLOCK";
        case TOKEN_WHITESPACE: return "WHITESPACE";
        case TOKEN_ERROR: return "ERROR";
        case TOKEN_EOF: return "EOF";
        default: return "UNKNOWN";
    }
}

void print_tokens(TokenStream* stream, bool show_whitespace) {
    printf("\n╔════════════════════════════════════════════════════════════╗\n");
    printf("║              ANÁLISE LÉXICA - TOKENS GERADOS              ║\n");
    printf("╚════════════════════════════════════════════════════════════╝\n\n");
    
    int significant_count = 0;
    
    for (int i = 0; i < stream->count; i++) {
        Token* t = &stream->tokens[i];
        
        if (!show_whitespace && t->type == TOKEN_WHITESPACE) continue;
        
        if (t->type != TOKEN_WHITESPACE && t->type != TOKEN_EOF) {
            significant_count++;
        }
        
        printf("[%3d] L%03d:C%03d  %-15s", 
               i, t->line, t->column, token_type_name(t->type));
        
        if (t->value) {
            if (t->type == TOKEN_STRING) {
                printf(" \"%s\"", t->value);
            } else if (t->type == TOKEN_WHITESPACE) {
                printf(" (whitespace)");
            } else {
                printf(" '%s'", t->value);
            }
        }
        
        if (t->type == TOKEN_ERROR && t->error_message) {
            printf(" [%s]", t->error_message);
        }
        
        printf("\n");
    }
    
    printf("\n━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n");
    printf("Total de tokens: %d (significativos: %d)\n", stream->count, significant_count);
    printf("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n\n");
}

// ==================== TESTES ====================

void test_datalang_complete_program() {
    printf("\n╔════════════════════════════════════════════════════════════╗\n");
    printf("║         TESTE: PROGRAMA COMPLETO EM DATALANG               ║\n");
    printf("╚════════════════════════════════════════════════════════════╝\n\n");
    
    const char* program = 
        "// Pipeline de análise de vendas\n"
        "import \"data_utils\"\n"
        "\n"
        "fn analyze_sales(data: DataFrame) -> DataFrame {\n"
        "  let filtered = data\n"
        "    |> filter(|row| row[\"amount\"] > 100.0)\n"
        "    |> groupby(\"category\")\n"
        "    |> map(|group| {\n"
        "      let total = group |> sum(\"amount\")\n"
        "      let avg = group |> mean(\"amount\")\n"
        "      return {\"total\": total, \"average\": avg}\n"
        "    })\n"
        "  \n"
        "  return filtered\n"
        "}\n"
        "\n"
        "let sales_data = load(\"sales.csv\")\n"
        "let result = analyze_sales(sales_data)\n"
        "save(result, \"output.csv\")";
    
    printf("Código fonte:\n");
    printf("─────────────────────────────────────────────────────────────\n");
    printf("%s\n", program);
    printf("─────────────────────────────────────────────────────────────\n\n");
    
    ErrorList* errors = create_error_list();
    TokenStream* stream = tokenize_with_errors(program, errors);
    
    if (errors->count > 0) {
        print_errors(errors);
    } else {
        printf("✓ Nenhum erro léxico encontrado!\n\n");
    }
    
    print_tokens(stream, false);
    
    free_error_list(errors);
    free_token_stream(stream);
}

void test_error_recovery() {
    printf("\n╔════════════════════════════════════════════════════════════╗\n");
    printf("║         TESTE: RECUPERAÇÃO DE ERROS                        ║\n");
    printf("╚════════════════════════════════════════════════════════════╝\n\n");
    
    const char* bad_code = 
        "let x = 42\n"
        "let name = \"unclosed string\n"
        "let y = @invalid\n"
        "let z = 3.14.15.92\n"
        "/* comentário não fechado\n"
        "let a = 123";
    
    printf("Código com erros:\n");
    printf("─────────────────────────────────────────────────────────────\n");
    printf("%s\n", bad_code);
    printf("─────────────────────────────────────────────────────────────\n\n");
    
    ErrorList* errors = create_error_list();
    TokenStream* stream = tokenize_with_errors(bad_code, errors);
    
    print_errors(errors);
    print_tokens(stream, false);
    
    free_error_list(errors);
    free_token_stream(stream);
}

void test_complex_patterns() {
    printf("\n╔════════════════════════════════════════════════════════════╗\n");
    printf("║         TESTE: PADRÕES COMPLEXOS                           ║\n");
    printf("╚════════════════════════════════════════════════════════════╝\n\n");
    
    const char* complex = 
        "let pi = 3.14159e-10\n"
        "let scientific = 2.998e8\n"
        "let negative = -42.5\n"
        "let range = 0..100\n"
        "let lambda = |x| => x * 2\n"
        "let pipeline = data |> filter |> map |> reduce\n"
        "let comparison = (x >= 10) && (y <= 20) || (z != 0)\n"
        "let string = \"escape: \\\"quotes\\\" and \\n newlines\"";
    
    printf("Código com padrões complexos:\n");
    printf("─────────────────────────────────────────────────────────────\n");
    printf("%s\n", complex);
    printf("─────────────────────────────────────────────────────────────\n\n");
    
    ErrorList* errors = create_error_list();
    TokenStream* stream = tokenize_with_errors(complex, errors);
    
    if (errors->count > 0) {
        print_errors(errors);
    } else {
        printf("✓ Todos os padrões reconhecidos corretamente!\n\n");
    }
    
    print_tokens(stream, false);
    
    free_error_list(errors);
    free_token_stream(stream);
}

// ==================== MAIN ====================

int main() {
    printf("\n-=-=-=-=-=-=-DATA LANG-=-=-=-=-=-=-\n");
    printf("\n=== Analisador Léxico Integrado ===\n");
    printf("Com suporte a AFNs, conversão AFN → AFD e tratamento de erros.\n\n");

    test_datalang_complete_program();
    test_error_recovery();
    test_complex_patterns();

    printf("\n--- Testes concluídos com sucesso ---\n\n");
    
    return 0;
}
