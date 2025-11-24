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
    char* buffer = (char*)malloc(length); 
    if (!buffer) return NULL;
    int j = 0; 
    for (int i = 1; i < length - 1; i++) {
        if (lexema[i] == '\\' && i + 1 < length - 1) {
            i++; 
            switch (lexema[i]) {
                case 'n': buffer[j++] = '\n'; break;
                case 't': buffer[j++] = '\t'; break;
                case 'r': buffer[j++] = '\r'; break;
                case '"': buffer[j++] = '"'; break;
                case '\\': buffer[j++] = '\\'; break;
                default: buffer[j++] = '\\'; buffer[j++] = lexema[i]; break;
            }
        } else {
            buffer[j++] = lexema[i];
        }
    }
    buffer[j] = '\0';
    char* final_buffer = (char*)realloc(buffer, j + 1);
    if (!final_buffer) { free(buffer); return strdup(""); }
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
    Token* t = consume(p, TOKEN_FILTER, "Esperado 'filter'"); if(!t) return NULL;
    ASTNode* n = create_node(AST_FILTER_TRANSFORM, t->line, t->column);
    consume(p, TOKEN_LPAREN, "("); n->filter_transform.filter_predicate = parse_lambda_expr(p); consume(p, TOKEN_RPAREN, ")");
    return n;
}

// MapTransform = "map" "(" LambdaExpr ")"
static ASTNode* parse_map_transform(Parser* p) {
    Token* t = consume(p, TOKEN_MAP, "Esperado 'map'"); if(!t) return NULL;
    ASTNode* n = create_node(AST_MAP_TRANSFORM, t->line, t->column);
    consume(p, TOKEN_LPAREN, "("); n->map_transform.map_function = parse_lambda_expr(p); consume(p, TOKEN_RPAREN, ")");
    return n;
}

// ReduceTransform = "reduce" "(" Expr "," LambdaExpr ")"
static ASTNode* parse_reduce_transform(Parser* p) {
    Token* t = consume(p, TOKEN_REDUCE, "Esperado 'reduce'"); if(!t) return NULL;
    ASTNode* n = create_node(AST_REDUCE_TRANSFORM, t->line, t->column);
    consume(p, TOKEN_LPAREN, "("); n->reduce_transform.initial_value = parse_expression(p);
    consume(p, TOKEN_COMMA, ","); n->reduce_transform.reducer = parse_lambda_expr(p); consume(p, TOKEN_RPAREN, ")");
    return n;
}

// SelectTransform = "select" "(" IdentList ")"
static ASTNode* parse_select_transform(Parser* p) {
    Token* t = consume(p, TOKEN_SELECT, "Esperado 'select'"); if(!t) return NULL;
    ASTNode* n = create_node(AST_SELECT_TRANSFORM, t->line, t->column);
    consume(p, TOKEN_LPAREN, "("); 
    int cap = 5; n->select_transform.columns = malloc(cap * sizeof(char*)); n->select_transform.column_count = 0;
    do { Token* c = consume(p, TOKEN_IDENTIFIER, "ID"); if(!c) break;
         if(n->select_transform.column_count >= cap) { cap*=2; n->select_transform.columns=realloc(n->select_transform.columns, cap*sizeof(char*)); }
         n->select_transform.columns[n->select_transform.column_count++] = strdup(c->lexema);
    } while(match(p, 1, TOKEN_COMMA));
    consume(p, TOKEN_RPAREN, ")"); return n;
}

// GroupByTransform = "groupby" "(" IdentList ")"
static ASTNode* parse_groupby_transform(Parser* p) {
    Token* t = consume(p, TOKEN_GROUPBY, "Esperado 'groupby'"); if(!t) return NULL;
    ASTNode* n = create_node(AST_GROUPBY_TRANSFORM, t->line, t->column);
    consume(p, TOKEN_LPAREN, "(");
    int cap = 5; n->groupby_transform.group_columns = malloc(cap * sizeof(char*)); n->groupby_transform.group_column_count = 0;
    do { Token* c = consume(p, TOKEN_IDENTIFIER, "ID"); if(!c) break;
         if(n->groupby_transform.group_column_count >= cap) { cap*=2; n->groupby_transform.group_columns=realloc(n->groupby_transform.group_columns, cap*sizeof(char*)); }
         n->groupby_transform.group_columns[n->groupby_transform.group_column_count++] = strdup(c->lexema);
    } while(match(p, 1, TOKEN_COMMA));
    consume(p, TOKEN_RPAREN, ")"); return n;
}

// AggregateTransform = ("sum" | "mean" | "count" | "min" | "max") "(" [ ExprList ] ")"
static ASTNode* parse_aggregate_transform(Parser* p) {
    Token* t = advance(p);
    ASTNode* n = create_node(AST_AGGREGATE_TRANSFORM, t->line, t->column);
    switch (t->type) {
        case TOKEN_SUM: n->aggregate_transform.agg_type = AGG_SUM; break;
        case TOKEN_MEAN: n->aggregate_transform.agg_type = AGG_MEAN; break;
        case TOKEN_COUNT: n->aggregate_transform.agg_type = AGG_COUNT; break;
        case TOKEN_MIN: n->aggregate_transform.agg_type = AGG_MIN; break;
        case TOKEN_MAX: n->aggregate_transform.agg_type = AGG_MAX; break;
        default: break;
    }
    consume(p, TOKEN_LPAREN, "(");
    int cap = 5; n->aggregate_transform.agg_args = malloc(cap * sizeof(ASTNode*)); n->aggregate_transform.agg_arg_count = 0;
    if(!check(p, TOKEN_RPAREN)) {
        do { if(n->aggregate_transform.agg_arg_count >= cap) { cap*=2; n->aggregate_transform.agg_args = realloc(n->aggregate_transform.agg_args, cap*sizeof(ASTNode*)); }
             n->aggregate_transform.agg_args[n->aggregate_transform.agg_arg_count++] = parse_expression(p);
        } while(match(p, 1, TOKEN_COMMA));
    }
    consume(p, TOKEN_RPAREN, ")"); return n;
}

// ==================== HIERARQUIA DE PRECEDÊNCIA ====================

// AssignExpr = LogicOrExpr { "=" LogicOrExpr }
static ASTNode* parse_assign_expr(Parser* p) {
    ASTNode* expr = parse_logic_or_expr(p);
    if (match(p, 1, TOKEN_ASSIGN)) {
        Token* op = previous(p); ASTNode* value = parse_assign_expr(p);
        ASTNode* assign = create_node(AST_ASSIGN_EXPR, op->line, op->column);
        assign->assign_expr.target = expr; assign->assign_expr.value = value; return assign;
    } return expr;
}

// LogicOrExpr = LogicAndExpr { "||" LogicAndExpr }
static ASTNode* parse_logic_or_expr(Parser* p) {
    ASTNode* left = parse_logic_and_expr(p);
    while (match(p, 1, TOKEN_OR)) {
        Token* op = previous(p); ASTNode* right = parse_logic_and_expr(p);
        ASTNode* binary = create_node(AST_BINARY_EXPR, op->line, op->column);
        binary->binary_expr.op = BINOP_OR; binary->binary_expr.left = left; binary->binary_expr.right = right; left = binary;
    } return left;
}

// LogicAndExpr = EqualityExpr { "&&" EqualityExpr }
static ASTNode* parse_logic_and_expr(Parser* p) {
    ASTNode* left = parse_equality_expr(p);
    while (match(p, 1, TOKEN_AND)) {
        Token* op = previous(p); ASTNode* right = parse_equality_expr(p);
        ASTNode* binary = create_node(AST_BINARY_EXPR, op->line, op->column);
        binary->binary_expr.op = BINOP_AND; binary->binary_expr.left = left; binary->binary_expr.right = right; left = binary;
    } return left;
}

// EqualityExpr = RelationalExpr { ("==" | "!=") RelationalExpr }
static ASTNode* parse_equality_expr(Parser* p) {
    ASTNode* left = parse_relational_expr(p);
    while (match(p, 2, TOKEN_EQUAL, TOKEN_NOT_EQUAL)) {
        Token* op = previous(p); ASTNode* right = parse_relational_expr(p);
        ASTNode* binary = create_node(AST_BINARY_EXPR, op->line, op->column);
        binary->binary_expr.op = (op->type == TOKEN_EQUAL) ? BINOP_EQ : BINOP_NEQ;
        binary->binary_expr.left = left; binary->binary_expr.right = right; left = binary;
    } return left;
}

// RelationalExpr = RangeExpr { ("<" | "<=" | ">" | ">=") RangeExpr }
static ASTNode* parse_relational_expr(Parser* p) {
    ASTNode* left = parse_range_expr(p);
    while (match(p, 4, TOKEN_LESS, TOKEN_LESS_EQUAL, TOKEN_GREATER, TOKEN_GREATER_EQUAL)) {
        Token* op = previous(p); ASTNode* right = parse_range_expr(p);
        ASTNode* binary = create_node(AST_BINARY_EXPR, op->line, op->column);
        switch (op->type) { case TOKEN_LESS: binary->binary_expr.op = BINOP_LT; break; case TOKEN_LESS_EQUAL: binary->binary_expr.op = BINOP_LTE; break; case TOKEN_GREATER: binary->binary_expr.op = BINOP_GT; break; case TOKEN_GREATER_EQUAL: binary->binary_expr.op = BINOP_GTE; break; default: break; }
        binary->binary_expr.left = left; binary->binary_expr.right = right; left = binary;
    } return left;
}

// RangeExpr = AddExpr [ ".." AddExpr ]
static ASTNode* parse_range_expr(Parser* p) {
    ASTNode* left = parse_add_expr(p);
    if (match(p, 1, TOKEN_RANGE)) {
        Token* op = previous(p); ASTNode* right = parse_add_expr(p);
        ASTNode* range = create_node(AST_RANGE_EXPR, op->line, op->column);
        range->range_expr.range_start = left; range->range_expr.range_end = right; return range;
    } return left;
}

// AddExpr = MultExpr { ("+" | "-") MultExpr }
static ASTNode* parse_add_expr(Parser* p) {
    ASTNode* left = parse_mult_expr(p);
    while (match(p, 2, TOKEN_PLUS, TOKEN_MINUS)) {
        Token* op = previous(p); ASTNode* right = parse_mult_expr(p);
        ASTNode* binary = create_node(AST_BINARY_EXPR, op->line, op->column);
        binary->binary_expr.op = (op->type == TOKEN_PLUS) ? BINOP_ADD : BINOP_SUB;
        binary->binary_expr.left = left; binary->binary_expr.right = right; left = binary;
    } return left;
}

// MultExpr = UnaryExpr { ("*" | "/" | "%") UnaryExpr }
static ASTNode* parse_mult_expr(Parser* p) {
    ASTNode* left = parse_unary_expr(p);
    while (match(p, 3, TOKEN_MULT, TOKEN_DIV, TOKEN_MOD)) {
        Token* op = previous(p); ASTNode* right = parse_unary_expr(p);
        ASTNode* binary = create_node(AST_BINARY_EXPR, op->line, op->column);
        switch (op->type) { case TOKEN_MULT: binary->binary_expr.op = BINOP_MUL; break; case TOKEN_DIV: binary->binary_expr.op = BINOP_DIV; break; case TOKEN_MOD: binary->binary_expr.op = BINOP_MOD; break; default: break; }
        binary->binary_expr.left = left; binary->binary_expr.right = right; left = binary;
    } return left;
}

// UnaryExpr = ("-" | "!") UnaryExpr | PostfixExpr
static ASTNode* parse_unary_expr(Parser* p) {
    if (match(p, 2, TOKEN_MINUS, TOKEN_NOT)) {
        Token* op = previous(p); ASTNode* operand = parse_unary_expr(p);
        ASTNode* unary = create_node(AST_UNARY_EXPR, op->line, op->column);
        unary->unary_expr.op = (op->type == TOKEN_MINUS) ? UNOP_NEG : UNOP_NOT;
        unary->unary_expr.operand = operand; return unary;
    } return parse_postfix_expr(p);
}

// PostfixExpr = Primary { Postfix }
// Postfix = "(" [ ExprList ] ")" | "[" Expr "]" | "." Ident
static ASTNode* parse_postfix_expr(Parser* p) {
    ASTNode* expr = parse_primary(p);
    while (true) {
        if (match(p, 1, TOKEN_LPAREN)) {
            Token* paren = previous(p); ASTNode* call = create_node(AST_CALL_EXPR, paren->line, paren->column);
            call->call_expr.callee = expr;
            int cap = 5; call->call_expr.arguments = malloc(cap * sizeof(ASTNode*)); call->call_expr.arg_count = 0;
            if (!check(p, TOKEN_RPAREN)) { do { if (call->call_expr.arg_count >= cap) { cap*=2; call->call_expr.arguments = realloc(call->call_expr.arguments, cap*sizeof(ASTNode*)); } call->call_expr.arguments[call->call_expr.arg_count++] = parse_expression(p); } while (match(p, 1, TOKEN_COMMA)); }
            consume(p, TOKEN_RPAREN, ")"); expr = call;
        } else if (match(p, 1, TOKEN_LBRACKET)) {
            Token* bracket = previous(p); ASTNode* idx = create_node(AST_INDEX_EXPR, bracket->line, bracket->column);
            idx->index_expr.object = expr; idx->index_expr.index = parse_expression(p);
            consume(p, TOKEN_RBRACKET, "]"); expr = idx;
        } else if (match(p, 1, TOKEN_DOT)) {
            Token* dot = previous(p); Token* mem = consume(p, TOKEN_IDENTIFIER, "ID"); if(!mem) break;
            ASTNode* member = create_node(AST_MEMBER_EXPR, dot->line, dot->column);
            member->member_expr.object = expr; member->member_expr.member = strdup(mem->lexema); expr = member;
        } else { break; }
    } return expr;
}

// Helper para verificar se o token é '|' (pipe para lambda)
static bool check_pipe_lambda(Parser* p) {
    Token* t = peek(p); return (t->type == TOKEN_DELIMITER && t->lexema && strcmp(t->lexema, "|") == 0);
}

// LambdaExpr = "|" [ LambdaParams ] "|" Expr
static ASTNode* parse_lambda_expr(Parser* p) {
    if (!check_pipe_lambda(p)) { error(p, "Esperado '|'"); return NULL; }
    Token* pipe = advance(p); ASTNode* node = create_node(AST_LAMBDA_EXPR, pipe->line, pipe->column);
    int cap = 5; node->lambda_expr.lambda_params = malloc(cap * sizeof(ASTNode*)); node->lambda_expr.lambda_param_count = 0;
    if (!check_pipe_lambda(p)) { do { Token* pn = consume(p, TOKEN_IDENTIFIER, "ID"); if(!pn) break;
            ASTNode* param = create_node(AST_PARAM, pn->line, pn->column); param->param.param_name = strdup(pn->lexema);
            if (match(p, 1, TOKEN_COLON)) param->param.param_type = parse_type(p); else param->param.param_type = NULL;
            if (node->lambda_expr.lambda_param_count >= cap) { cap*=2; node->lambda_expr.lambda_params = realloc(node->lambda_expr.lambda_params, cap*sizeof(ASTNode*)); }
            node->lambda_expr.lambda_params[node->lambda_expr.lambda_param_count++] = param;
    } while (match(p, 1, TOKEN_COMMA)); }
    if (!check_pipe_lambda(p)) { error(p, "Esperado '|'"); return NULL; }
    advance(p); node->lambda_expr.lambda_body = parse_expression(p); return node;
}

// Primary = Literal | Ident | LambdaExpr | LoadExpr | SaveExpr | "(" Expr ")" | "[" [ ExprList ] "]"
static ASTNode* parse_primary(Parser* p) {
    if (match(p, 3, TOKEN_INTEGER, TOKEN_FLOAT, TOKEN_STRING)) {
        Token* lit = previous(p); ASTNode* node = create_node(AST_LITERAL, lit->line, lit->column);
        node->literal.literal_type = lit->type;
        switch (lit->type) {
            case TOKEN_INTEGER: node->literal.int_value = lit->lexema ? atoll(lit->lexema) : 0; break;
            case TOKEN_FLOAT: node->literal.float_value = lit->lexema ? atof(lit->lexema) : 0.0; break;
            case TOKEN_STRING: node->literal.string_value = process_string_literal(lit->lexema); break;
            default: break;
        } return node;
    }
    if (match(p, 2, TOKEN_TRUE, TOKEN_FALSE)) {
        Token* lit = previous(p); ASTNode* node = create_node(AST_LITERAL, lit->line, lit->column);
        node->literal.literal_type = TOKEN_BOOL_TYPE; node->literal.bool_value = (lit->type == TOKEN_TRUE); return node;
    }
    if (match(p, 1, TOKEN_IDENTIFIER)) {
        Token* id = previous(p); ASTNode* node = create_node(AST_IDENTIFIER, id->line, id->column);
        node->identifier.id_name = strdup(id->lexema ? id->lexema : "unknown"); return node;
    }
    if (check_pipe_lambda(p)) return parse_lambda_expr(p);
    if (match(p, 1, TOKEN_LOAD)) {
        Token* l = previous(p); consume(p, TOKEN_LPAREN, "("); Token* path = consume(p, TOKEN_STRING, "Path"); consume(p, TOKEN_RPAREN, ")");
        ASTNode* n = create_node(AST_LOAD_EXPR, l->line, l->column); n->load_expr.file_path = strdup(path ? path->lexema : ""); return n;
    }
    if (match(p, 1, TOKEN_SAVE)) {
        Token* s = previous(p); consume(p, TOKEN_LPAREN, "("); ASTNode* data = parse_expression(p); consume(p, TOKEN_COMMA, ",");
        Token* path = consume(p, TOKEN_STRING, "Path"); consume(p, TOKEN_RPAREN, ")");
        ASTNode* n = create_node(AST_SAVE_EXPR, s->line, s->column); n->save_expr.data = data; n->save_expr.save_path = strdup(path ? path->lexema : ""); return n;
    }
    if (match(p, 1, TOKEN_LPAREN)) { ASTNode* e = parse_expression(p); consume(p, TOKEN_RPAREN, ")"); return e; }
    if (match(p, 1, TOKEN_LBRACKET)) {
        Token* b = previous(p); ASTNode* n = create_node(AST_ARRAY_LITERAL, b->line, b->column);
        int cap = 5; n->array_literal.elements = malloc(cap * sizeof(ASTNode*)); n->array_literal.element_count = 0;
        if (!check(p, TOKEN_RBRACKET)) { do {
            if (n->array_literal.element_count >= cap) { cap*=2; n->array_literal.elements = realloc(n->array_literal.elements, cap*sizeof(ASTNode*)); }
            n->array_literal.elements[n->array_literal.element_count++] = parse_expression(p);
        } while (match(p, 1, TOKEN_COMMA)); }
        consume(p, TOKEN_RBRACKET, "]"); return n;
    }
    error(p, "Esperado expressão"); return NULL;
}