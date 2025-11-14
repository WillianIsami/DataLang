/*
 * DataLang - Parser Principal
 * Implementação do parser LL(1) recursivo descendente
 */

#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include "parser.h"

// ==================== DECLARAÇÕES FORWARD (FUNÇÕES INTERNAS) ====================

// Funções auxiliares (privadas - static)
Token* peek(Parser* p);
Token* previous(Parser* p);
Token* advance(Parser* p);
bool is_at_end(Parser* p);
bool check(Parser* p, TokenType type);
bool match(Parser* p, int count, ...);
void error_at(Parser* p, Token* token, const char* message);
void error(Parser* p, const char* message);
Token* consume(Parser* p, TokenType type, const char* message);
void synchronize(Parser* p);
ASTNode* create_node(ASTNodeType type, int line, int column);

// Funções de parsing (privadas - static)
ASTNode* parse_program(Parser* p);
ASTNode* parse_let_decl(Parser* p);
ASTNode* parse_fn_decl(Parser* p);
ASTNode* parse_data_decl(Parser* p);
ASTNode* parse_import_decl(Parser* p);
ASTNode* parse_export_decl(Parser* p);
ASTNode* parse_type(Parser* p);
ASTNode* parse_block(Parser* p);
ASTNode* parse_statement(Parser* p);
ASTNode* parse_if_statement(Parser* p);
ASTNode* parse_for_statement(Parser* p);
ASTNode* parse_return_statement(Parser* p);
ASTNode* parse_expr_statement(Parser* p);

// Expressões (declaradas em parser_expr.c)
extern ASTNode* parse_expression(Parser* p);

// ==================== UTILITÁRIOS DO PARSER ====================

Token* peek(Parser* p) {
    if (p->current >= p->token_count) {
        return &p->tokens[p->token_count - 1];
    }
    return &p->tokens[p->current];
}

Token* previous(Parser* p) {
    return &p->tokens[p->current - 1];
}

Token* advance(Parser* p) {
    if (!is_at_end(p)) {
        p->current++;
    }
    return previous(p);
}

bool is_at_end(Parser* p) {
    return peek(p)->type == TOKEN_EOF;
}

bool check(Parser* p, TokenType type) {
    if (is_at_end(p)) return false;
    return peek(p)->type == type;
}

bool match(Parser* p, int count, ...) {
    va_list args;
    va_start(args, count);
    
    for (int i = 0; i < count; i++) {
        TokenType type = va_arg(args, TokenType);
        if (check(p, type)) {
            advance(p);
            va_end(args);
            return true;
        }
    }
    
    va_end(args);
    return false;
}

// ==================== TRATAMENTO DE ERROS ====================

void error_at(Parser* p, Token* token, const char* message) {
    if (p->panic_mode) return;
    
    p->had_error = true;
    p->panic_mode = true;
    
    if (p->error_count >= p->error_capacity) {
        p->error_capacity *= 2;
        p->error_messages = realloc(p->error_messages, 
            p->error_capacity * sizeof(char*));
    }
    
    char buffer[512];
    snprintf(buffer, sizeof(buffer), 
        "Erro [linha %d, coluna %d]: %s próximo a '%s'",
        token->line, token->column, message, token->lexema);
    
    p->error_messages[p->error_count++] = strdup(buffer);
    fprintf(stderr, "%s\n", buffer);
}

void error(Parser* p, const char* message) {
    error_at(p, peek(p), message);
}

Token* consume(Parser* p, TokenType type, const char* message) {
    if (check(p, type)) {
        return advance(p);
    }
    
    error(p, message);
    return NULL;
}

void synchronize(Parser* p) {
    p->panic_mode = false;
    advance(p);
    
    while (!is_at_end(p)) {
        if (previous(p)->type == TOKEN_SEMICOLON) return;
        
        switch (peek(p)->type) {
            case TOKEN_LET:
            case TOKEN_FN:
            case TOKEN_DATA:
            case TOKEN_IF:
            case TOKEN_FOR:
            case TOKEN_RETURN:
            case TOKEN_IMPORT:
            case TOKEN_EXPORT:
                return;
            default:
                break;
        }
        
        advance(p);
    }
}

// ==================== CRIAÇÃO DE NODOS AST ====================

ASTNode* create_node(ASTNodeType type, int line, int column) {
    ASTNode* node = calloc(1, sizeof(ASTNode));
    if (!node) {
        fprintf(stderr, "Erro: Falha ao alocar memória para nodo AST\n");
        exit(1);
    }
    node->type = type;
    node->line = line;
    node->column = column;
    return node;
}

// ==================== PARSING PRINCIPAL ====================

// Program = { TopLevel }
ASTNode* parse_program(Parser* p) {
    ASTNode* program = create_node(AST_PROGRAM, 1, 1);
    
    int capacity = 10;
    program->program.declarations = malloc(capacity * sizeof(ASTNode*));
    program->program.decl_count = 0;
    
    while (!is_at_end(p)) {
        ASTNode* decl = NULL;
        
        if (check(p, TOKEN_LET)) {
            decl = parse_let_decl(p);
        } else if (check(p, TOKEN_FN)) {
            decl = parse_fn_decl(p);
        } else if (check(p, TOKEN_DATA)) {
            decl = parse_data_decl(p);
        } else if (check(p, TOKEN_IMPORT)) {
            decl = parse_import_decl(p);
        } else if (check(p, TOKEN_EXPORT)) {
            decl = parse_export_decl(p);
        } else if (check(p, TOKEN_IF)) {
            decl = parse_if_statement(p);
        } else {
            ASTNode* stmt = parse_statement(p);
            if (stmt && stmt->type == AST_EXPR_STMT) {
                decl = stmt;
            } else {
                error(p, "Esperado declaração ou statement de nível superior");
                if (stmt) free_ast(stmt);
                synchronize(p);
                continue;
            }
        }
        
        if (decl != NULL) {
            if (program->program.decl_count >= capacity) {
                capacity *= 2;
                program->program.declarations = realloc(
                    program->program.declarations, 
                    capacity * sizeof(ASTNode*));
            }
            program->program.declarations[program->program.decl_count++] = decl;
        }
    }
    
    return program;
}

// LetDecl = "let" Ident [ ":" Type ] "=" Expr ";"
ASTNode* parse_let_decl(Parser* p) {
    Token* let_token = consume(p, TOKEN_LET, "Esperado 'let'");
    if (!let_token) return NULL;
    
    ASTNode* node = create_node(AST_LET_DECL, let_token->line, let_token->column);
    
    Token* name = consume(p, TOKEN_IDENTIFIER, "Esperado nome da variável");
    if (!name) {
        free_ast(node);
        return NULL;
    }
    node->let_decl.name = strdup(name->lexema);
    
    // Tipo opcional
    if (match(p, 1, TOKEN_COLON)) {
        node->let_decl.type_annotation = parse_type(p);
    } else {
        node->let_decl.type_annotation = NULL;
    }
    
    consume(p, TOKEN_ASSIGN, "Esperado '=' após declaração de variável");
    node->let_decl.initializer = parse_expression(p);
    consume(p, TOKEN_SEMICOLON, "Esperado ';' após declaração de variável");
    
    return node;
}

// FnDecl = "fn" Ident "(" [ FormalParams ] ")" [ "->" Type ] Block
ASTNode* parse_fn_decl(Parser* p) {
    Token* fn_token = consume(p, TOKEN_FN, "Esperado 'fn'");
    if (!fn_token) return NULL;
    
    ASTNode* node = create_node(AST_FN_DECL, fn_token->line, fn_token->column);
    
    Token* name = consume(p, TOKEN_IDENTIFIER, "Esperado nome da função");
    if (!name) {
        free_ast(node);
        return NULL;
    }
    node->fn_decl.name = strdup(name->lexema);
    
    consume(p, TOKEN_LPAREN, "Esperado '(' após nome da função");
    
    // Parâmetros
    int param_capacity = 5;
    node->fn_decl.params = malloc(param_capacity * sizeof(ASTNode*));
    node->fn_decl.param_count = 0;
    
    if (!check(p, TOKEN_RPAREN)) {
        do {
            Token* param_name = consume(p, TOKEN_IDENTIFIER, "Esperado nome do parâmetro");
            if (!param_name) break;
            
            consume(p, TOKEN_COLON, "Esperado ':' após nome do parâmetro");
            ASTNode* param_type = parse_type(p);
            
            ASTNode* param = create_node(AST_PARAM, param_name->line, param_name->column);
            param->param.param_name = strdup(param_name->lexema);
            param->param.param_type = param_type;
            
            if (node->fn_decl.param_count >= param_capacity) {
                param_capacity *= 2;
                node->fn_decl.params = realloc(node->fn_decl.params, 
                    param_capacity * sizeof(ASTNode*));
            }
            node->fn_decl.params[node->fn_decl.param_count++] = param;
            
        } while (match(p, 1, TOKEN_COMMA));
    }
    
    consume(p, TOKEN_RPAREN, "Esperado ')' após parâmetros");
    
    // Tipo de retorno opcional
    if (match(p, 1, TOKEN_ARROW)) {
        node->fn_decl.return_type = parse_type(p);
    } else {
        node->fn_decl.return_type = NULL;
    }
    
    node->fn_decl.body = parse_block(p);
    
    return node;
}

// DataDecl = "data" Ident "{" { FieldDecl } "}"
ASTNode* parse_data_decl(Parser* p) {
    Token* data_token = consume(p, TOKEN_DATA, "Esperado 'data'");
    if (!data_token) return NULL;
    
    ASTNode* node = create_node(AST_DATA_DECL, data_token->line, data_token->column);
    
    Token* name = consume(p, TOKEN_IDENTIFIER, "Esperado nome do tipo de dado");
    if (!name) {
        free_ast(node);
        return NULL;
    }
    node->data_decl.name = strdup(name->lexema);
    
    consume(p, TOKEN_LBRACE, "Esperado '{' antes dos campos");
    
    int field_capacity = 5;
    node->data_decl.fields = malloc(field_capacity * sizeof(ASTNode*));
    node->data_decl.field_count = 0;
    
    while (!check(p, TOKEN_RBRACE) && !is_at_end(p)) {
        Token* field_name = consume(p, TOKEN_IDENTIFIER, "Esperado nome do campo");
        if (!field_name) break;
        
        consume(p, TOKEN_COLON, "Esperado ':' após nome do campo");
        ASTNode* field_type = parse_type(p);
        consume(p, TOKEN_SEMICOLON, "Esperado ';' após campo");
        
        ASTNode* field = create_node(AST_FIELD_DECL, field_name->line, field_name->column);
        field->field_decl.field_name = strdup(field_name->lexema);
        field->field_decl.field_type = field_type;
        
        if (node->data_decl.field_count >= field_capacity) {
            field_capacity *= 2;
            node->data_decl.fields = realloc(node->data_decl.fields,
                field_capacity * sizeof(ASTNode*));
        }
        node->data_decl.fields[node->data_decl.field_count++] = field;
    }
    
    consume(p, TOKEN_RBRACE, "Esperado '}' após campos");
    
    return node;
}

// ImportDecl = "import" STRING [ "as" Ident ] ";"
ASTNode* parse_import_decl(Parser* p) {
    Token* import_token = consume(p, TOKEN_IMPORT, "Esperado 'import'");
    if (!import_token) return NULL;
    
    ASTNode* node = create_node(AST_IMPORT_DECL, import_token->line, import_token->column);
    
    Token* path = consume(p, TOKEN_STRING, "Esperado caminho do módulo");
    if (!path) {
        free_ast(node);
        return NULL;
    }
    node->import_decl.module_path = strdup(path->lexema);
    
    // Alias opcional
    if (match(p, 1, TOKEN_AS)) {
        Token* alias = consume(p, TOKEN_IDENTIFIER, "Esperado identificador após 'as'");
        if (alias) {
            node->import_decl.alias = strdup(alias->lexema);
        }
    } else {
        node->import_decl.alias = NULL;
    }
    
    consume(p, TOKEN_SEMICOLON, "Esperado ';' após import");
    
    return node;
}

// ExportDecl = "export" Ident ";"
ASTNode* parse_export_decl(Parser* p) {
    Token* export_token = consume(p, TOKEN_EXPORT, "Esperado 'export'");
    if (!export_token) return NULL;
    
    ASTNode* node = create_node(AST_EXPORT_DECL, export_token->line, export_token->column);
    
    Token* name = consume(p, TOKEN_IDENTIFIER, "Esperado nome para exportar");
    if (!name) {
        free_ast(node);
        return NULL;
    }
    node->export_decl.export_name = strdup(name->lexema);
    
    consume(p, TOKEN_SEMICOLON, "Esperado ';' após export");
    
    return node;
}

// Type = "Int" | "Float" | "String" | "Bool" | "DataFrame" | ...
ASTNode* parse_type(Parser* p) {
    Token* current = peek(p);
    ASTNode* node = create_node(AST_TYPE, current->line, current->column);
    
    if (match(p, 7, TOKEN_INT_TYPE, TOKEN_FLOAT_TYPE, TOKEN_STRING_TYPE,
               TOKEN_BOOL_TYPE, TOKEN_DATAFRAME_TYPE, TOKEN_VECTOR_TYPE, 
               TOKEN_SERIES_TYPE)) {
        node->type_node.type_kind = previous(p)->type;
        node->type_node.type_name = strdup(previous(p)->lexema);
        node->type_node.inner_type = NULL;
        return node;
    }
    
    if (match(p, 1, TOKEN_IDENTIFIER)) {
        node->type_node.type_kind = TOKEN_IDENTIFIER;
        node->type_node.type_name = strdup(previous(p)->lexema);
        node->type_node.inner_type = NULL;
        return node;
    }
    
    // Array type: [Type]
    if (match(p, 1, TOKEN_LBRACKET)) {
        node->type_node.type_kind = TOKEN_LBRACKET;
        node->type_node.inner_type = parse_type(p);
        consume(p, TOKEN_RBRACKET, "Esperado ']' após tipo de array");
        return node;
    }
    
    // Tuple type: (Type, Type, ...)
    if (match(p, 1, TOKEN_LPAREN)) {
        node->type_node.type_kind = TOKEN_LPAREN;
        int capacity = 5;
        node->type_node.tuple_types = malloc(capacity * sizeof(ASTNode*));
        node->type_node.tuple_type_count = 0;
        
        do {
            if (node->type_node.tuple_type_count >= capacity) {
                capacity *= 2;
                node->type_node.tuple_types = realloc(node->type_node.tuple_types,
                    capacity * sizeof(ASTNode*));
            }
            node->type_node.tuple_types[node->type_node.tuple_type_count++] = 
                parse_type(p);
        } while (match(p, 1, TOKEN_COMMA));
        
        consume(p, TOKEN_RPAREN, "Esperado ')' após tipos da tupla");
        return node;
    }
    
    error(p, "Esperado tipo");
    free_ast(node);
    return NULL;
}

// Block = "{" { Statement } "}"
ASTNode* parse_block(Parser* p) {
    Token* brace = consume(p, TOKEN_LBRACE, "Esperado '{'");
    if (!brace) return NULL;
    
    ASTNode* node = create_node(AST_BLOCK, brace->line, brace->column);
    
    int capacity = 10;
    node->block.statements = malloc(capacity * sizeof(ASTNode*));
    node->block.stmt_count = 0;
    
    while (!check(p, TOKEN_RBRACE) && !is_at_end(p)) {
        ASTNode* stmt = parse_statement(p);
        if (stmt != NULL) {
            if (node->block.stmt_count >= capacity) {
                capacity *= 2;
                node->block.statements = realloc(node->block.statements,
                    capacity * sizeof(ASTNode*));
            }
            node->block.statements[node->block.stmt_count++] = stmt;
        }
    }
    
    consume(p, TOKEN_RBRACE, "Esperado '}' após bloco");
    
    return node;
}

// Statement = LetDecl | IfStatement | ForStatement | ReturnStatement | ExprStatement
ASTNode* parse_statement(Parser* p) {
    if (check(p, TOKEN_LET)) {
        return parse_let_decl(p);
    }
    
    if (check(p, TOKEN_IF)) {
        return parse_if_statement(p);
    }
    
    if (check(p, TOKEN_FOR)) {
        return parse_for_statement(p);
    }
    
    if (check(p, TOKEN_RETURN)) {
        return parse_return_statement(p);
    }
    
    return parse_expr_statement(p);
}

// IfStatement = "if" Expr Block [ "else" ( IfStatement | Block ) ]
ASTNode* parse_if_statement(Parser* p) {
    Token* if_token = consume(p, TOKEN_IF, "Esperado 'if'");
    if (!if_token) return NULL;
    
    ASTNode* node = create_node(AST_IF_STMT, if_token->line, if_token->column);
    
    node->if_stmt.condition = parse_expression(p);
    node->if_stmt.then_block = parse_block(p);
    
    // Else opcional
    if (match(p, 1, TOKEN_ELSE)) {
        if (check(p, TOKEN_IF)) {
            node->if_stmt.else_block = parse_if_statement(p);
        } else {
            node->if_stmt.else_block = parse_block(p);
        }
    } else {
        node->if_stmt.else_block = NULL;
    }
    
    return node;
}

// ForStatement = "for" Ident "in" Expr Block
ASTNode* parse_for_statement(Parser* p) {
    Token* for_token = consume(p, TOKEN_FOR, "Esperado 'for'");
    if (!for_token) return NULL;
    
    ASTNode* node = create_node(AST_FOR_STMT, for_token->line, for_token->column);
    
    Token* iterator = consume(p, TOKEN_IDENTIFIER, "Esperado nome do iterador");
    if (!iterator) {
        free_ast(node);
        return NULL;
    }
    node->for_stmt.iterator = strdup(iterator->lexema);
    
    consume(p, TOKEN_IN, "Esperado 'in' após iterador");
    node->for_stmt.iterable = parse_expression(p);
    node->for_stmt.body = parse_block(p);
    
    return node;
}

// ReturnStatement = "return" [ Expr ] ";"
ASTNode* parse_return_statement(Parser* p) {
    Token* return_token = consume(p, TOKEN_RETURN, "Esperado 'return'");
    if (!return_token) return NULL;
    
    ASTNode* node = create_node(AST_RETURN_STMT, return_token->line, return_token->column);
    
    if (!check(p, TOKEN_SEMICOLON)) {
        node->return_stmt.value = parse_expression(p);
    } else {
        node->return_stmt.value = NULL;
    }
    
    consume(p, TOKEN_SEMICOLON, "Esperado ';' após return");
    
    return node;
}

// ExprStatement = Expr ";"
ASTNode* parse_expr_statement(Parser* p) {
    Token* current = peek(p);
    ASTNode* node = create_node(AST_EXPR_STMT, current->line, current->column);
    
    node->expr_stmt.expression = parse_expression(p);
    consume(p, TOKEN_SEMICOLON, "Esperado ';' após expressão");
    
    return node;
}

// ==================== FUNÇÕES PÚBLICAS ====================

Parser* create_parser(TokenStream* stream) {
    if (!stream || !stream->tokens) return NULL;
    
    Parser* parser = calloc(1, sizeof(Parser));
    if (!parser) return NULL;
    
    parser->tokens = stream->tokens;
    parser->token_count = stream->count;
    parser->current = 0;
    parser->had_error = false;
    parser->panic_mode = false;
    
    parser->error_capacity = 10;
    parser->error_messages = malloc(parser->error_capacity * sizeof(char*));
    parser->error_count = 0;
    
    return parser;
}

void free_parser(Parser* parser) {
    if (!parser) return;
    
    for (int i = 0; i < parser->error_count; i++) {
        free(parser->error_messages[i]);
    }
    free(parser->error_messages);
    free(parser);
}

ASTNode* parse(Parser* parser) {
    if (!parser) return NULL;
    
    printf("\n╔════════════════════════════════════════════════════════════╗\n");
    printf("║              INICIANDO ANÁLISE SINTÁTICA LL(1)            ║\n");
    printf("╚════════════════════════════════════════════════════════════╝\n\n");
    
    ASTNode* ast = parse_program(parser);
    
    if (parser->had_error) {
        printf("\n❌ Parsing concluído com %d erro(s)\n", parser->error_count);
        printf("\nErros encontrados:\n");
        for (int i = 0; i < parser->error_count; i++) {
            printf("  %d. %s\n", i + 1, parser->error_messages[i]);
        }
    } else {
        printf("\n✓ Parsing concluído com sucesso!\n");
    }
    
    return ast;
}