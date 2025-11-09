/*
 * DataLang - Parser de Expressões
 * Implementação da hierarquia de precedência seguindo a gramática LL(1)
 */

#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "parser.h"

// Declarações forward das funções de parsing de expressões
static ASTNode* parse_pipeline_expr(Parser* p);
static ASTNode* parse_transform_expr(Parser* p);
static ASTNode* parse_assign_expr(Parser* p);
static ASTNode* parse_logic_or_expr(Parser* p);
static ASTNode* parse_logic_and_expr(Parser* p);
static ASTNode* parse_equality_expr(Parser* p);
static ASTNode* parse_relational_expr(Parser* p);
static ASTNode* parse_range_expr(Parser* p);
static ASTNode* parse_add_expr(Parser* p);
static ASTNode* parse_mult_expr(Parser* p);
static ASTNode* parse_unary_expr(Parser* p);
static ASTNode* parse_postfix_expr(Parser* p);
static ASTNode* parse_primary(Parser* p);
static ASTNode* parse_lambda_expr(Parser* p);

// Transformações
static ASTNode* parse_filter_transform(Parser* p);
static ASTNode* parse_map_transform(Parser* p);
static ASTNode* parse_reduce_transform(Parser* p);
static ASTNode* parse_select_transform(Parser* p);
static ASTNode* parse_groupby_transform(Parser* p);
static ASTNode* parse_aggregate_transform(Parser* p);

/**
 * Processa o conteúdo de um literal de string vindo do lexer.
 * Remove as aspas externas e trata sequências de escape.
 */
static char* process_string_literal(const char* lexema) {
    int length = strlen(lexema);
    if (length < 2) return strdup(""); 

    // Aloca 'length' em vez de 'length - 1'.
    // Isso fornece espaço mais que suficiente para qualquer sequência de escape,
    // prevenindo qualquer estouro de buffer, mesmo que a lógica de 'j' esteja errada.
    char* buffer = (char*)malloc(length); 
    if (!buffer) return NULL;

    int j = 0; // Índice do buffer
    
    // Itera pelo conteúdo da string, pulando as aspas (i=1 to length-2)
    for (int i = 1; i < length - 1; i++) {
        if (lexema[i] == '\\' && i + 1 < length - 1) {
            // Trata caractere de escape
            i++; // Consome a barra
            switch (lexema[i]) {
                case 'n': buffer[j++] = '\n'; break;
                case 't': buffer[j++] = '\t'; break;
                case 'r': buffer[j++] = '\r'; break;
                case '"': buffer[j++] = '"'; break;
                case '\\': buffer[j++] = '\\'; break;
                default: 
                    // Se não for um escape conhecido, mantém a barra e o caractere.
                    buffer[j++] = '\\';
                    buffer[j++] = lexema[i];
                    break;
            }
        } else {
            buffer[j++] = lexema[i];
        }
    }
    buffer[j] = '\0'; // Adiciona null terminator

    // Reduz o buffer para o tamanho exato usado (j + 1 bytes)
    // Esta chamada agora é 100% segura, pois 'j+1' é garantido
    // ser menor ou igual a 'length'.
    char* final_buffer = (char*)realloc(buffer, j + 1);
    
    // realloc pode falhar e retornar NULL
    if (final_buffer == NULL) {
        free(buffer); // Libera o buffer original se realloc falhar
        return strdup(""); // Retorna uma string vazia segura
    }
    
    return final_buffer;
}

// ==================== PONTO DE ENTRADA PARA EXPRESSÕES ====================

// Expr = PipelineExpr
ASTNode* parse_expression(Parser* p) {
    return parse_pipeline_expr(p);
}

// ==================== PIPELINE E TRANSFORMAÇÕES ====================

// PipelineExpr = TransformExpr { "|>" TransformExpr }
static ASTNode* parse_pipeline_expr(Parser* p) {
    ASTNode* left = parse_transform_expr(p);
    
    if (match(p, 1, TOKEN_PIPE)) {
        // Pipeline detected
        ASTNode* pipeline = create_node(AST_PIPELINE_EXPR, 
            previous(p)->line, previous(p)->column);
        
        int capacity = 5;
        pipeline->pipeline_expr.stages = malloc(capacity * sizeof(ASTNode*));
        pipeline->pipeline_expr.stage_count = 0;
        
        // Adiciona primeiro estágio
        pipeline->pipeline_expr.stages[pipeline->pipeline_expr.stage_count++] = left;
        
        // Adiciona estágios subsequentes
        do {
            ASTNode* stage = parse_transform_expr(p);
            if (pipeline->pipeline_expr.stage_count >= capacity) {
                capacity *= 2;
                pipeline->pipeline_expr.stages = realloc(
                    pipeline->pipeline_expr.stages,
                    capacity * sizeof(ASTNode*));
            }
            pipeline->pipeline_expr.stages[pipeline->pipeline_expr.stage_count++] = stage;
        } while (match(p, 1, TOKEN_PIPE));
        
        return pipeline;
    }
    
    return left;
}

// TransformExpr = AssignExpr | FilterTransform | MapTransform | ...
static ASTNode* parse_transform_expr(Parser* p) {
    // Verifica se é uma transformação
    if (check(p, TOKEN_FILTER)) {
        return parse_filter_transform(p);
    }
    if (check(p, TOKEN_MAP)) {
        return parse_map_transform(p);
    }
    if (check(p, TOKEN_REDUCE)) {
        return parse_reduce_transform(p);
    }
    if (check(p, TOKEN_SELECT)) {
        return parse_select_transform(p);
    }
    if (check(p, TOKEN_GROUPBY)) {
        return parse_groupby_transform(p);
    }
    if (check(p, TOKEN_SUM) || check(p, TOKEN_MEAN) || check(p, TOKEN_COUNT) ||
        check(p, TOKEN_MIN) || check(p, TOKEN_MAX)) {
        return parse_aggregate_transform(p);
    }
    
    // Caso contrário, é uma expressão de atribuição
    return parse_assign_expr(p);
}

// FilterTransform = "filter" "(" LambdaExpr ")"
static ASTNode* parse_filter_transform(Parser* p) {
    Token* filter_token = consume(p, TOKEN_FILTER, "Esperado 'filter'");
    if (!filter_token) return NULL;
    
    ASTNode* node = create_node(AST_FILTER_TRANSFORM, 
        filter_token->line, filter_token->column);
    
    consume(p, TOKEN_LPAREN, "Esperado '(' após 'filter'");
    node->filter_transform.filter_predicate = parse_lambda_expr(p);
    consume(p, TOKEN_RPAREN, "Esperado ')' após lambda");
    
    return node;
}

// MapTransform = "map" "(" LambdaExpr ")"
static ASTNode* parse_map_transform(Parser* p) {
    Token* map_token = consume(p, TOKEN_MAP, "Esperado 'map'");
    if (!map_token) return NULL;
    
    ASTNode* node = create_node(AST_MAP_TRANSFORM,
        map_token->line, map_token->column);
    
    consume(p, TOKEN_LPAREN, "Esperado '(' após 'map'");
    node->map_transform.map_function = parse_lambda_expr(p);
    consume(p, TOKEN_RPAREN, "Esperado ')' após lambda");
    
    return node;
}

// ReduceTransform = "reduce" "(" Expr "," LambdaExpr ")"
static ASTNode* parse_reduce_transform(Parser* p) {
    Token* reduce_token = consume(p, TOKEN_REDUCE, "Esperado 'reduce'");
    if (!reduce_token) return NULL;
    
    ASTNode* node = create_node(AST_REDUCE_TRANSFORM,
        reduce_token->line, reduce_token->column);
    
    consume(p, TOKEN_LPAREN, "Esperado '(' após 'reduce'");
    node->reduce_transform.initial_value = parse_expression(p);
    consume(p, TOKEN_COMMA, "Esperado ',' após valor inicial");
    node->reduce_transform.reducer = parse_lambda_expr(p);
    consume(p, TOKEN_RPAREN, "Esperado ')' após lambda");
    
    return node;
}

// SelectTransform = "select" "(" IdentList ")"
static ASTNode* parse_select_transform(Parser* p) {
    Token* select_token = consume(p, TOKEN_SELECT, "Esperado 'select'");
    if (!select_token) return NULL;
    
    ASTNode* node = create_node(AST_SELECT_TRANSFORM,
        select_token->line, select_token->column);
    
    consume(p, TOKEN_LPAREN, "Esperado '(' após 'select'");
    
    int capacity = 5;
    node->select_transform.columns = malloc(capacity * sizeof(char*));
    node->select_transform.column_count = 0;
    
    do {
        Token* col = consume(p, TOKEN_IDENTIFIER, "Esperado nome de coluna");
        if (!col) break;
        
        if (node->select_transform.column_count >= capacity) {
            capacity *= 2;
            node->select_transform.columns = realloc(
                node->select_transform.columns, capacity * sizeof(char*));
        }
        node->select_transform.columns[node->select_transform.column_count++] = 
            strdup(col->lexema);
    } while (match(p, 1, TOKEN_COMMA));
    
    consume(p, TOKEN_RPAREN, "Esperado ')' após lista de colunas");
    
    return node;
}

// GroupByTransform = "groupby" "(" IdentList ")"
static ASTNode* parse_groupby_transform(Parser* p) {
    Token* groupby_token = consume(p, TOKEN_GROUPBY, "Esperado 'groupby'");
    if (!groupby_token) return NULL;
    
    ASTNode* node = create_node(AST_GROUPBY_TRANSFORM,
        groupby_token->line, groupby_token->column);
    
    consume(p, TOKEN_LPAREN, "Esperado '(' após 'groupby'");
    
    int capacity = 5;
    node->groupby_transform.group_columns = malloc(capacity * sizeof(char*));
    node->groupby_transform.group_column_count = 0;
    
    do {
        Token* col = consume(p, TOKEN_IDENTIFIER, "Esperado nome de coluna");
        if (!col) break;
        
        if (node->groupby_transform.group_column_count >= capacity) {
            capacity *= 2;
            node->groupby_transform.group_columns = realloc(
                node->groupby_transform.group_columns, capacity * sizeof(char*));
        }
        node->groupby_transform.group_columns[node->groupby_transform.group_column_count++] = 
            strdup(col->lexema);
    } while (match(p, 1, TOKEN_COMMA));
    
    consume(p, TOKEN_RPAREN, "Esperado ')' após lista de colunas");
    
    return node;
}

// AggregateTransform = ("sum" | "mean" | "count" | "min" | "max") "(" [ ExprList ] ")"
static ASTNode* parse_aggregate_transform(Parser* p) {
    Token* agg_token = advance(p);
    
    ASTNode* node = create_node(AST_AGGREGATE_TRANSFORM,
        agg_token->line, agg_token->column);
    
    // Determina o tipo de agregação
    switch (agg_token->type) {
        case TOKEN_SUM: node->aggregate_transform.agg_type = AGG_SUM; break;
        case TOKEN_MEAN: node->aggregate_transform.agg_type = AGG_MEAN; break;
        case TOKEN_COUNT: node->aggregate_transform.agg_type = AGG_COUNT; break;
        case TOKEN_MIN: node->aggregate_transform.agg_type = AGG_MIN; break;
        case TOKEN_MAX: node->aggregate_transform.agg_type = AGG_MAX; break;
        default: break;
    }
    
    consume(p, TOKEN_LPAREN, "Esperado '(' após função de agregação");
    
    int capacity = 5;
    node->aggregate_transform.agg_args = malloc(capacity * sizeof(ASTNode*));
    node->aggregate_transform.agg_arg_count = 0;
    
    if (!check(p, TOKEN_RPAREN)) {
        do {
            if (node->aggregate_transform.agg_arg_count >= capacity) {
                capacity *= 2;
                node->aggregate_transform.agg_args = realloc(
                    node->aggregate_transform.agg_args, capacity * sizeof(ASTNode*));
            }
            node->aggregate_transform.agg_args[node->aggregate_transform.agg_arg_count++] = 
                parse_expression(p);
        } while (match(p, 1, TOKEN_COMMA));
    }
    
    consume(p, TOKEN_RPAREN, "Esperado ')' após argumentos");
    
    return node;
}

// ==================== HIERARQUIA DE PRECEDÊNCIA ====================

// AssignExpr = LogicOrExpr { "=" LogicOrExpr }
static ASTNode* parse_assign_expr(Parser* p) {
    ASTNode* expr = parse_logic_or_expr(p);
    
    if (match(p, 1, TOKEN_ASSIGN)) {
        Token* op = previous(p);
        ASTNode* value = parse_assign_expr(p); // Associatividade à direita
        
        ASTNode* assign = create_node(AST_ASSIGN_EXPR, op->line, op->column);
        assign->assign_expr.target = expr;
        assign->assign_expr.value = value;
        return assign;
    }
    
    return expr;
}

// LogicOrExpr = LogicAndExpr { "||" LogicAndExpr }
static ASTNode* parse_logic_or_expr(Parser* p) {
    ASTNode* left = parse_logic_and_expr(p);
    
    while (match(p, 1, TOKEN_OR)) {
        Token* op = previous(p);
        ASTNode* right = parse_logic_and_expr(p);
        
        ASTNode* binary = create_node(AST_BINARY_EXPR, op->line, op->column);
        binary->binary_expr.op = BINOP_OR;
        binary->binary_expr.left = left;
        binary->binary_expr.right = right;
        left = binary;
    }
    
    return left;
}

// LogicAndExpr = EqualityExpr { "&&" EqualityExpr }
static ASTNode* parse_logic_and_expr(Parser* p) {
    ASTNode* left = parse_equality_expr(p);
    
    while (match(p, 1, TOKEN_AND)) {
        Token* op = previous(p);
        ASTNode* right = parse_equality_expr(p);
        
        ASTNode* binary = create_node(AST_BINARY_EXPR, op->line, op->column);
        binary->binary_expr.op = BINOP_AND;
        binary->binary_expr.left = left;
        binary->binary_expr.right = right;
        left = binary;
    }
    
    return left;
}

// EqualityExpr = RelationalExpr { ("==" | "!=") RelationalExpr }
static ASTNode* parse_equality_expr(Parser* p) {
    ASTNode* left = parse_relational_expr(p);
    
    while (match(p, 2, TOKEN_EQUAL, TOKEN_NOT_EQUAL)) {
        Token* op = previous(p);
        ASTNode* right = parse_relational_expr(p);
        
        ASTNode* binary = create_node(AST_BINARY_EXPR, op->line, op->column);
        binary->binary_expr.op = (op->type == TOKEN_EQUAL) ? BINOP_EQ : BINOP_NEQ;
        binary->binary_expr.left = left;
        binary->binary_expr.right = right;
        left = binary;
    }
    
    return left;
}

// RelationalExpr = RangeExpr { ("<" | "<=" | ">" | ">=") RangeExpr }
static ASTNode* parse_relational_expr(Parser* p) {
    ASTNode* left = parse_range_expr(p);
    
    while (match(p, 4, TOKEN_LESS, TOKEN_LESS_EQUAL, 
                     TOKEN_GREATER, TOKEN_GREATER_EQUAL)) {
        Token* op = previous(p);
        ASTNode* right = parse_range_expr(p);
        
        ASTNode* binary = create_node(AST_BINARY_EXPR, op->line, op->column);
        switch (op->type) {
            case TOKEN_LESS: binary->binary_expr.op = BINOP_LT; break;
            case TOKEN_LESS_EQUAL: binary->binary_expr.op = BINOP_LTE; break;
            case TOKEN_GREATER: binary->binary_expr.op = BINOP_GT; break;
            case TOKEN_GREATER_EQUAL: binary->binary_expr.op = BINOP_GTE; break;
            default: break;
        }
        binary->binary_expr.left = left;
        binary->binary_expr.right = right;
        left = binary;
    }
    
    return left;
}

// RangeExpr = AddExpr [ ".." AddExpr ]
static ASTNode* parse_range_expr(Parser* p) {
    ASTNode* left = parse_add_expr(p);
    
    if (match(p, 1, TOKEN_RANGE)) {
        Token* op = previous(p);
        ASTNode* right = parse_add_expr(p);
        
        ASTNode* range = create_node(AST_RANGE_EXPR, op->line, op->column);
        range->range_expr.range_start = left;
        range->range_expr.range_end = right;
        return range;
    }
    
    return left;
}

// AddExpr = MultExpr { ("+" | "-") MultExpr }
static ASTNode* parse_add_expr(Parser* p) {
    ASTNode* left = parse_mult_expr(p);
    
    while (match(p, 2, TOKEN_PLUS, TOKEN_MINUS)) {
        Token* op = previous(p);
        ASTNode* right = parse_mult_expr(p);
        
        ASTNode* binary = create_node(AST_BINARY_EXPR, op->line, op->column);
        binary->binary_expr.op = (op->type == TOKEN_PLUS) ? BINOP_ADD : BINOP_SUB;
        binary->binary_expr.left = left;
        binary->binary_expr.right = right;
        left = binary;
    }
    
    return left;
}

// MultExpr = UnaryExpr { ("*" | "/" | "%") UnaryExpr }
static ASTNode* parse_mult_expr(Parser* p) {
    ASTNode* left = parse_unary_expr(p);
    
    while (match(p, 3, TOKEN_MULT, TOKEN_DIV, TOKEN_MOD)) {
        Token* op = previous(p);
        ASTNode* right = parse_unary_expr(p);
        
        ASTNode* binary = create_node(AST_BINARY_EXPR, op->line, op->column);
        switch (op->type) {
            case TOKEN_MULT: binary->binary_expr.op = BINOP_MUL; break;
            case TOKEN_DIV: binary->binary_expr.op = BINOP_DIV; break;
            case TOKEN_MOD: binary->binary_expr.op = BINOP_MOD; break;
            default: break;
        }
        binary->binary_expr.left = left;
        binary->binary_expr.right = right;
        left = binary;
    }
    
    return left;
}

// UnaryExpr = ("-" | "!") UnaryExpr | PostfixExpr
static ASTNode* parse_unary_expr(Parser* p) {
    if (match(p, 2, TOKEN_MINUS, TOKEN_NOT)) {
        Token* op = previous(p);
        ASTNode* operand = parse_unary_expr(p); // Recursão à direita
        
        ASTNode* unary = create_node(AST_UNARY_EXPR, op->line, op->column);
        unary->unary_expr.op = (op->type == TOKEN_MINUS) ? UNOP_NEG : UNOP_NOT;
        unary->unary_expr.operand = operand;
        return unary;
    }
    
    return parse_postfix_expr(p);
}

// PostfixExpr = Primary { Postfix }
// Postfix = "(" [ ExprList ] ")" | "[" Expr "]" | "." Ident
static ASTNode* parse_postfix_expr(Parser* p) {
    ASTNode* expr = parse_primary(p);
    
    while (true) {
        if (match(p, 1, TOKEN_LPAREN)) {
            // Chamada de função
            Token* paren = previous(p);
            ASTNode* call = create_node(AST_CALL_EXPR, paren->line, paren->column);
            call->call_expr.callee = expr;
            
            int capacity = 5;
            call->call_expr.arguments = malloc(capacity * sizeof(ASTNode*));
            call->call_expr.arg_count = 0;
            
            if (!check(p, TOKEN_RPAREN)) {
                do {
                    if (call->call_expr.arg_count >= capacity) {
                        capacity *= 2;
                        call->call_expr.arguments = realloc(
                            call->call_expr.arguments, capacity * sizeof(ASTNode*));
                    }
                    call->call_expr.arguments[call->call_expr.arg_count++] = 
                        parse_expression(p);
                } while (match(p, 1, TOKEN_COMMA));
            }
            
            consume(p, TOKEN_RPAREN, "Esperado ')' após argumentos");
            expr = call;
            
        } else if (match(p, 1, TOKEN_LBRACKET)) {
            // Indexação
            Token* bracket = previous(p);
            ASTNode* index_node = create_node(AST_INDEX_EXPR, 
                bracket->line, bracket->column);
            index_node->index_expr.object = expr;
            index_node->index_expr.index = parse_expression(p);
            consume(p, TOKEN_RBRACKET, "Esperado ']' após índice");
            expr = index_node;
            
        } else if (match(p, 1, TOKEN_DOT)) {
            // Acesso a membro
            Token* dot = previous(p);
            Token* member = consume(p, TOKEN_IDENTIFIER, 
                "Esperado nome de propriedade após '.'");
            if (!member) break;
            
            ASTNode* member_node = create_node(AST_MEMBER_EXPR, 
                dot->line, dot->column);
            member_node->member_expr.object = expr;
            member_node->member_expr.member = strdup(member->lexema);
            expr = member_node;
            
        } else {
            break;
        }
    }
    
    return expr;
}

// Helper para verificar se o token é '|' (pipe para lambda)
static bool check_pipe_lambda(Parser* p) {
    Token* t = peek(p);
    // Pipe para lambda: | (diferente de |> para pipeline)
    if (t->type == TOKEN_DELIMITER && t->lexema && strcmp(t->lexema, "|") == 0) {
        return true;
    }
    return false;
}

// LambdaExpr = "|" [ LambdaParams ] "|" Expr
static ASTNode* parse_lambda_expr(Parser* p) {
    // Consome o primeiro '|'
    if (!check_pipe_lambda(p)) {
        error(p, "Esperado '|' para iniciar lambda");
        return NULL;
    }
    Token* pipe = advance(p);
    
    ASTNode* node = create_node(AST_LAMBDA_EXPR, pipe->line, pipe->column);
    
    int capacity = 5;
    node->lambda_expr.lambda_params = malloc(capacity * sizeof(ASTNode*));
    node->lambda_expr.lambda_param_count = 0;
    
    // Parâmetros opcionais (se não for logo '|')
    if (!check_pipe_lambda(p)) {
        do {
            Token* param_name = consume(p, TOKEN_IDENTIFIER, 
                "Esperado nome do parâmetro");
            if (!param_name) break;
            
            ASTNode* param = create_node(AST_PARAM, 
                param_name->line, param_name->column);
            param->param.param_name = strdup(param_name->lexema);
            
            // Tipo opcional
            if (match(p, 1, TOKEN_COLON)) {
                param->param.param_type = parse_type(p);
            } else {
                param->param.param_type = NULL;
            }
            
            if (node->lambda_expr.lambda_param_count >= capacity) {
                capacity *= 2;
                node->lambda_expr.lambda_params = realloc(
                    node->lambda_expr.lambda_params, capacity * sizeof(ASTNode*));
            }
            node->lambda_expr.lambda_params[node->lambda_expr.lambda_param_count++] = param;
            
        } while (match(p, 1, TOKEN_COMMA));
    }
    
    // Consome o segundo '|'
    if (!check_pipe_lambda(p)) {
        error(p, "Esperado '|' após parâmetros do lambda");
        // free_ast(node);  // Comentado para evitar problemas
        return NULL;
    }
    advance(p);
    
    node->lambda_expr.lambda_body = parse_expression(p);
    
    return node;
}

// Primary = Literal | Ident | LambdaExpr | LoadExpr | SaveExpr | "(" Expr ")" | "[" [ ExprList ] "]"
static ASTNode* parse_primary(Parser* p) {
    Token* current = peek(p);
    
    // ========== LITERAIS ==========
    if (match(p, 3, TOKEN_INTEGER, TOKEN_FLOAT, TOKEN_STRING)) {
        Token* lit = previous(p);
        ASTNode* node = create_node(AST_LITERAL, lit->line, lit->column);
        node->literal.literal_type = lit->type;
        
        switch (lit->type) {
            case TOKEN_INTEGER:
                if (lit->lexema && lit->lexema[0] != '\0') {
                    node->literal.int_value = atoll(lit->lexema);
                } else {
                    node->literal.int_value = 0;
                }
                break;
                
            case TOKEN_FLOAT:
                if (lit->lexema && lit->lexema[0] != '\0') {
                    node->literal.float_value = atof(lit->lexema);
                } else {
                    node->literal.float_value = 0.0;
                }
                break;
                
            case TOKEN_STRING:
                node->literal.string_value = process_string_literal(lit->lexema);
                if (!node->literal.string_value) {
                    // Tratar falha de alocação
                    error(p, "Falha ao alocar memória para string literal");
                    free_ast(node);
                    return NULL;
                }
                break;
                
            default:
                break;
        }
        return node;
    }
    
    // Booleanos
    if (match(p, 2, TOKEN_TRUE, TOKEN_FALSE)) {
        Token* lit = previous(p);
        ASTNode* node = create_node(AST_LITERAL, lit->line, lit->column);
        node->literal.literal_type = TOKEN_BOOL_TYPE;  // Usar TOKEN_BOOL_TYPE
        node->literal.bool_value = (lit->type == TOKEN_TRUE);
        return node;
    }
    
    // Identificador
    if (match(p, 1, TOKEN_IDENTIFIER)) {
        Token* id = previous(p);
        ASTNode* node = create_node(AST_IDENTIFIER, id->line, id->column);
        
        if (id->lexema && id->lexema[0] != '\0') {
            node->identifier.id_name = strdup(id->lexema);
        } else {
            node->identifier.id_name = strdup("unknown");
        }
        
        if (!node->identifier.id_name) {
            error(p, "Falha de memória ao duplicar identificador");
            free_ast(node);
            return NULL;
        }
        
        return node;
    }
    
    // Lambda - verifica se é '|'
    if (check_pipe_lambda(p)) {
        return parse_lambda_expr(p);
    }
    
    // Load
    if (match(p, 1, TOKEN_LOAD)) {
        Token* load = previous(p);
        consume(p, TOKEN_LPAREN, "Esperado '(' após 'load'");
        Token* path = consume(p, TOKEN_STRING, "Esperado caminho do arquivo");
        consume(p, TOKEN_RPAREN, "Esperado ')' após caminho");
        
        ASTNode* node = create_node(AST_LOAD_EXPR, load->line, load->column);
        if (path && path->lexema) {
            node->load_expr.file_path = strdup(path->lexema);
        } else {
            node->load_expr.file_path = strdup("");
        }
        return node;
    }
    
    // Save
    if (match(p, 1, TOKEN_SAVE)) {
        Token* save = previous(p);
        consume(p, TOKEN_LPAREN, "Esperado '(' após 'save'");
        ASTNode* data = parse_expression(p);
        consume(p, TOKEN_COMMA, "Esperado ',' após expressão");
        Token* path = consume(p, TOKEN_STRING, "Esperado caminho do arquivo");
        consume(p, TOKEN_RPAREN, "Esperado ')' após caminho");
        
        ASTNode* node = create_node(AST_SAVE_EXPR, save->line, save->column);
        node->save_expr.data = data;
        if (path && path->lexema) {
            node->save_expr.save_path = strdup(path->lexema);
        } else {
            node->save_expr.save_path = strdup("");
        }
        return node;
    }
    
    // Expressão entre parênteses
    if (match(p, 1, TOKEN_LPAREN)) {
        ASTNode* expr = parse_expression(p);
        consume(p, TOKEN_RPAREN, "Esperado ')' após expressão");
        return expr;
    }
    
    // Array literal
    if (match(p, 1, TOKEN_LBRACKET)) {
        Token* bracket = previous(p);
        ASTNode* node = create_node(AST_ARRAY_LITERAL, bracket->line, bracket->column);
        
        int capacity = 5;
        node->array_literal.elements = malloc(capacity * sizeof(ASTNode*));
        node->array_literal.element_count = 0;
        
        if (!check(p, TOKEN_RBRACKET)) {
            do {
                if (node->array_literal.element_count >= capacity) {
                    capacity *= 2;
                    node->array_literal.elements = realloc(
                        node->array_literal.elements, capacity * sizeof(ASTNode*));
                }
                node->array_literal.elements[node->array_literal.element_count++] = 
                    parse_expression(p);
            } while (match(p, 1, TOKEN_COMMA));
        }
        
        consume(p, TOKEN_RBRACKET, "Esperado ']' após elementos do array");
        return node;
    }
    
    error(p, "Esperado expressão");
    return NULL;
}