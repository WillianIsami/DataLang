/*
 * DataLang - Analisador Semântico - Implementação Corrigida
 * 
 * 1. Verificação rigorosa de tipos em operações aritméticas
 * 2. Detecção de variáveis não declaradas
 * 3. Verificação de compatibilidade de tipos em atribuições
 * 4. Validação de tipos em operações lógicas
 * 5. Verificação de arrays homogêneos
 * 6. Detecção de redeclarações
 */

#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "semantic_analyzer.h"

// ==================== DECLARAÇÕES FORWARD ====================

// Análise de expressões (da parte 2)
Type* analyze_member_expr(SemanticAnalyzer* analyzer, ASTNode* node);
Type* analyze_index_expr(SemanticAnalyzer* analyzer, ASTNode* node);
Type* analyze_assign_expr(SemanticAnalyzer* analyzer, ASTNode* node);
Type* analyze_array_literal(SemanticAnalyzer* analyzer, ASTNode* node);
Type* analyze_range_expr(SemanticAnalyzer* analyzer, ASTNode* node);
Type* analyze_load_expr(SemanticAnalyzer* analyzer, ASTNode* node);
Type* analyze_save_expr(SemanticAnalyzer* analyzer, ASTNode* node);
Type* analyze_print_stmt(SemanticAnalyzer* analyzer, ASTNode* node);
Type* analyze_aggregate_transform(SemanticAnalyzer* analyzer, ASTNode* node);

// ==================== CRIAÇÃO E DESTRUIÇÃO ====================

SemanticAnalyzer* create_semantic_analyzer() {
    SemanticAnalyzer* analyzer = calloc(1, sizeof(SemanticAnalyzer));
    analyzer->symbol_table = create_symbol_table();
    analyzer->inference_ctx = create_inference_context();
    analyzer->had_error = false;
    analyzer->warning_count = 0;
    analyzer->in_function = false;
    analyzer->current_function_return_type = NULL;
    analyzer->in_loop = false;
    return analyzer;
}

void free_semantic_analyzer(SemanticAnalyzer* analyzer) {
    if (!analyzer) return;
    free_symbol_table(analyzer->symbol_table);
    free_inference_context(analyzer->inference_ctx);
    if (analyzer->current_function_return_type) {
        free_type(analyzer->current_function_return_type);
    }
    free(analyzer);
}

// ==================== FUNÇÕES BUILT-IN ====================

void register_builtin_functions(SemanticAnalyzer* analyzer) {
    printf("[Semantic] Registrando funções built-in...\n");
    
    // print(x) -> Void (polimórfico - aceita qualquer tipo)
    {
        Type* param_types[1] = {create_primitive_type(TYPE_INT)};
        Type* return_type = create_primitive_type(TYPE_VOID);
        declare_function(analyzer->symbol_table, "print", return_type, param_types, 1, 0, 0);
    }
    
    // sum(array: [Int]) -> Int
    {
        Type* param_types[1] = {
            create_array_type(create_primitive_type(TYPE_INT))
        };
        Type* return_type = create_primitive_type(TYPE_INT);
        declare_function(analyzer->symbol_table, "sum", return_type, param_types, 1, 0, 0);
    }
    
    // mean(array: [Int]) -> Float
    {
        Type* param_types[1] = {
            create_array_type(create_primitive_type(TYPE_INT))
        };
        Type* return_type = create_primitive_type(TYPE_FLOAT);
        declare_function(analyzer->symbol_table, "mean", return_type, param_types, 1, 0, 0);
    }
    
    // count(array: [Int]) -> Int
    {
        Type* param_types[1] = {
            create_array_type(create_primitive_type(TYPE_INT))
        };
        Type* return_type = create_primitive_type(TYPE_INT);
        declare_function(analyzer->symbol_table, "count", return_type, param_types, 1, 0, 0);
    }
    
    // min(array: [Int]) -> Int
    {
        Type* param_types[1] = {
            create_array_type(create_primitive_type(TYPE_INT))
        };
        Type* return_type = create_primitive_type(TYPE_INT);
        declare_function(analyzer->symbol_table, "min", return_type, param_types, 1, 0, 0);
    }
    
    // max(array: [Int]) -> Int
    {
        Type* param_types[1] = {
            create_array_type(create_primitive_type(TYPE_INT))
        };
        Type* return_type = create_primitive_type(TYPE_INT);
        declare_function(analyzer->symbol_table, "max", return_type, param_types, 1, 0, 0);
    }
    
    printf("[Semantic] Funções built-in registradas com sucesso\n");
}

// ==================== ANÁLISE PRINCIPAL ====================

bool analyze_semantics(SemanticAnalyzer* analyzer, ASTNode* program) {
    if (!analyzer || !program) return false;
    
    printf("\n╔═══════════════════════════════════════════════════════════╗\n");
    printf("║          ANÁLISE SEMÂNTICA E INFERÊNCIA DE TIPOS          ║\n");
    printf("╚═══════════════════════════════════════════════════════════╝\n\n");
    
    if (program->type != AST_PROGRAM) {
        fprintf(stderr, "Erro: Nodo raiz não é um programa\n");
        return false;
    }
    
    // Registra funções built-in ANTES de processar declarações
    register_builtin_functions(analyzer);
    
    // Fase 1: Coleta de Declarações (Funções e Tipos)
    printf("[Fase 1] Coletando declarações de nível superior...\n");
    for (int i = 0; i < program->program.decl_count; i++) {
        ASTNode* decl = program->program.declarations[i];
        
        if (decl->type == AST_FN_DECL) {
            Symbol* existing = lookup_symbol(analyzer->symbol_table, decl->fn_decl.name);
            if (existing && existing->kind == SYMBOL_FUNCTION) {
                symbol_table_error(analyzer->symbol_table, decl->line, decl->column,
                    "Função '%s' já foi declarada anteriormente na linha %d",
                    decl->fn_decl.name, existing->line);
                analyzer->had_error = true;
                continue;
            }
            
            Type** param_types = malloc(decl->fn_decl.param_count * sizeof(Type*));
            for (int j = 0; j < decl->fn_decl.param_count; j++) {
                param_types[j] = ast_type_to_type(analyzer, decl->fn_decl.params[j]->param.param_type);
            }
            
            Type* return_type = decl->fn_decl.return_type ? 
                ast_type_to_type(analyzer, decl->fn_decl.return_type) :
                create_primitive_type(TYPE_VOID);
            
            declare_function(analyzer->symbol_table, decl->fn_decl.name,
                           return_type, param_types, decl->fn_decl.param_count,
                           decl->line, decl->column);
            free(param_types);
        }
        else if (decl->type == AST_DATA_DECL) {
            Symbol* existing = lookup_symbol(analyzer->symbol_table, decl->data_decl.name);
            if (existing && existing->kind == SYMBOL_TYPE) {
                symbol_table_error(analyzer->symbol_table, decl->line, decl->column,
                    "Tipo '%s' já foi declarado anteriormente na linha %d",
                    decl->data_decl.name, existing->line);
                analyzer->had_error = true;
                continue;
            }
            
            Symbol** fields = malloc(decl->data_decl.field_count * sizeof(Symbol*));
            for (int j = 0; j < decl->data_decl.field_count; j++) {
                ASTNode* field = decl->data_decl.fields[j];
                Type* field_type = ast_type_to_type(analyzer, field->field_decl.field_type);
                
                Symbol* field_symbol = calloc(1, sizeof(Symbol));
                field_symbol->name = strdup(field->field_decl.field_name);
                field_symbol->kind = SYMBOL_FIELD;
                field_symbol->type = field_type;
                field_symbol->line = field->line;
                field_symbol->column = field->column;
                
                fields[j] = field_symbol;
            }
            
            declare_type(analyzer->symbol_table, decl->data_decl.name,
                        fields, decl->data_decl.field_count,
                        decl->line, decl->column);
            free(fields);
        }
    }
    
    // Fase 2: Analisa CORPOS e STATEMENTS DE NÍVEL SUPERIOR
    printf("[Fase 2] Analisando corpos e instruções...\n");
    for (int i = 0; i < program->program.decl_count; i++) {
        ASTNode* decl = program->program.declarations[i];
        
        switch (decl->type) {
            case AST_FN_DECL:
                // Funções já foram declaradas, agora analisamos o corpo
                if (!analyzer->had_error || !lookup_symbol(analyzer->symbol_table, decl->fn_decl.name)) {
                    analyze_fn_decl(analyzer, decl);
                }
                break;
            case AST_DATA_DECL:
                // Data types já foram declarados, analisamos estrutura interna
                analyze_data_decl(analyzer, decl);
                break;
            default:
                // Para qualquer outra coisa (Let, If, For, Print, ExprStmt),
                // usamos analyze_statement para garantir que a validação ocorra.
                analyze_statement(analyzer, decl);
                break;
        }
    }
    
    // Fase 3: Verificações finais
    printf("[Fase 3] Verificações finais...\n");
    check_unused_symbols(analyzer->symbol_table);
    
    print_semantic_analysis_report(analyzer);
    
    return !analyzer->had_error;
}

// ==================== ANÁLISE DE DECLARAÇÕES ====================

Type* analyze_let_decl(SemanticAnalyzer* analyzer, ASTNode* node) {
    // Analisa o inicializador
    Type* init_type = analyze_expression(analyzer, node->let_decl.initializer);
    
    // Se há anotação de tipo, verifica compatibilidade
    Type* declared_type = NULL;
    if (node->let_decl.type_annotation) {
        declared_type = ast_type_to_type(analyzer, node->let_decl.type_annotation);
        
        if (!types_compatible(declared_type, init_type)) {
            symbol_table_error(analyzer->symbol_table, node->line, node->column,
                "Tipo de inicializador (%s) incompatível com tipo declarado (%s)",
                type_to_string(init_type), type_to_string(declared_type));
            analyzer->had_error = true;
            return create_error_type();
        }
    }
    
    // Declara a variável
    Type* var_type = declared_type ? declared_type : init_type;
    Symbol* symbol = declare_symbol(analyzer->symbol_table, 
                                    node->let_decl.name, SYMBOL_VARIABLE,
                                    clone_type(var_type), node->line, node->column);
    
    if (symbol) {
        symbol->initialized = true;
    } else {
        analyzer->had_error = true;
    }
    
    return var_type;
}

Type* analyze_fn_decl(SemanticAnalyzer* analyzer, ASTNode* node) {
    // Entra em novo escopo para a função
    enter_scope(analyzer->symbol_table);
    
    // Marca que estamos em uma função
    bool was_in_function = analyzer->in_function;
    analyzer->in_function = true;
    
    // Determina tipo de retorno
    Type* return_type = node->fn_decl.return_type ?
        ast_type_to_type(analyzer, node->fn_decl.return_type) :
        create_primitive_type(TYPE_VOID);
    
    Type* old_return_type = analyzer->current_function_return_type;
    analyzer->current_function_return_type = return_type;
    
    // Declara parâmetros
    for (int i = 0; i < node->fn_decl.param_count; i++) {
        ASTNode* param = node->fn_decl.params[i];
        Type* param_type = ast_type_to_type(analyzer, param->param.param_type);
        
        Symbol* param_symbol = declare_symbol(analyzer->symbol_table,
                                              param->param.param_name,
                                              SYMBOL_PARAMETER,
                                              param_type,
                                              param->line, param->column);
        if (param_symbol) {
            param_symbol->initialized = true;
        }
    }
    
    // Analisa corpo
    analyze_block(analyzer, node->fn_decl.body);
    
    // Verifica se todos os caminhos retornam (exceto para void)
    if (return_type->kind != TYPE_VOID) {
        if (!check_all_paths_return(analyzer, node->fn_decl.body)) {
            symbol_table_error(analyzer->symbol_table, node->line, node->column,
                "Função '%s' não retorna valor em todos os caminhos",
                node->fn_decl.name);
            analyzer->had_error = true;
        }
    }
    
    // Restaura contexto
    analyzer->in_function = was_in_function;
    analyzer->current_function_return_type = old_return_type;
    
    // Sai do escopo
    exit_scope(analyzer->symbol_table);
    
    // Cria tipo de função
    Type** param_types = malloc(node->fn_decl.param_count * sizeof(Type*));
    for (int i = 0; i < node->fn_decl.param_count; i++) {
        param_types[i] = ast_type_to_type(analyzer, 
            node->fn_decl.params[i]->param.param_type);
    }
    
    Type* fn_type = create_function_type(param_types, node->fn_decl.param_count, 
                                         return_type);
    free(param_types);
    
    return fn_type;
}

Type* analyze_data_decl(SemanticAnalyzer* analyzer, ASTNode* node) {
    // Tipo já foi pré-declarado na fase 1
    // Aqui apenas verificamos os campos
    
    for (int i = 0; i < node->data_decl.field_count; i++) {
        ASTNode* field = node->data_decl.fields[i];
        ast_type_to_type(analyzer, field->field_decl.field_type);
    }
    
    return create_custom_type(node->data_decl.name);
}

// ==================== ANÁLISE DE STATEMENTS ====================

Type* analyze_statement(SemanticAnalyzer* analyzer, ASTNode* node) {
    if (!node) {
        return create_error_type();
    }
    
    switch (node->type) {
        case AST_LET_DECL:
            return analyze_let_decl(analyzer, node);
        case AST_IF_STMT:
            return analyze_if_stmt(analyzer, node);
        case AST_FOR_STMT:
            return analyze_for_stmt(analyzer, node);
        case AST_RETURN_STMT:
            return analyze_return_stmt(analyzer, node);
        case AST_PRINT_STMT:
            return analyze_print_stmt(analyzer, node);
        case AST_EXPR_STMT:
            if (node->expr_stmt.expression) {
                return analyze_expression(analyzer, node->expr_stmt.expression);
            } else {
                symbol_table_error(analyzer->symbol_table, node->line, node->column,
                    "Statement de expressão vazio");
                analyzer->had_error = true;
                return create_error_type();
            }
        case AST_BLOCK:
            return analyze_block(analyzer, node);
        default:
            symbol_table_error(analyzer->symbol_table, node->line, node->column,
                "Tipo de statement não suportado: %d", node->type);
            analyzer->had_error = true;
            return create_error_type();
    }
}

Type* analyze_if_stmt(SemanticAnalyzer* analyzer, ASTNode* node) {
    // Analisa condição
    Type* cond_type = analyze_expression(analyzer, node->if_stmt.condition);
    
    // Condição deve ser booleana
    if (cond_type->kind != TYPE_BOOL && cond_type->kind != TYPE_ERROR) {
        symbol_table_error(analyzer->symbol_table, node->line, node->column,
            "Condição do 'if' deve ser booleana, mas encontrado %s",
            type_to_string(cond_type));
        analyzer->had_error = true;
    }
    
    // Analisa blocos
    analyze_block(analyzer, node->if_stmt.then_block);
    if (node->if_stmt.else_block) {
        if (node->if_stmt.else_block->type == AST_BLOCK) {
            analyze_block(analyzer, node->if_stmt.else_block);
        } else {
            analyze_if_stmt(analyzer, node->if_stmt.else_block);
        }
    }
    
    return create_primitive_type(TYPE_VOID);
}

Type* analyze_for_stmt(SemanticAnalyzer* analyzer, ASTNode* node) {
    // Entra em novo escopo para o loop
    enter_scope(analyzer->symbol_table);
    
    bool was_in_loop = analyzer->in_loop;
    analyzer->in_loop = true;
    
    // Analisa iterável
    Type* iterable_type = analyze_expression(analyzer, node->for_stmt.iterable);
    
    // Determina tipo do elemento
    Type* element_type = NULL;
    if (iterable_type->kind == TYPE_ARRAY) {
        element_type = clone_type(iterable_type->element_type);
    } else if (iterable_type->kind != TYPE_ERROR) {
        symbol_table_error(analyzer->symbol_table, node->line, node->column,
            "Tipo %s não é iterável", type_to_string(iterable_type));
        analyzer->had_error = true;
        element_type = create_error_type();
    } else {
        element_type = create_error_type();
    }
    
    // Declara variável do iterador
    Symbol* iter_symbol = declare_symbol(analyzer->symbol_table, node->for_stmt.iterator,
                  SYMBOL_VARIABLE, element_type, node->line, node->column);
    if (iter_symbol) {
        mark_symbol_initialized(analyzer->symbol_table, node->for_stmt.iterator);
    }
    
    // Analisa corpo
    analyze_block(analyzer, node->for_stmt.body);
    
    analyzer->in_loop = was_in_loop;
    exit_scope(analyzer->symbol_table);
    
    return create_primitive_type(TYPE_VOID);
}

Type* analyze_return_stmt(SemanticAnalyzer* analyzer, ASTNode* node) {
    if (!analyzer->in_function) {
        symbol_table_error(analyzer->symbol_table, node->line, node->column,
            "'return' fora de função");
        analyzer->had_error = true;
        return create_error_type();
    }
    
    Type* return_value_type = node->return_stmt.value ?
        analyze_expression(analyzer, node->return_stmt.value) :
        create_primitive_type(TYPE_VOID);
    
    // Verifica compatibilidade com tipo de retorno da função
    if (analyzer->current_function_return_type) {
        if (!types_compatible(analyzer->current_function_return_type, return_value_type)) {
            symbol_table_error(analyzer->symbol_table, node->line, node->column,
                "Tipo de retorno (%s) incompatível com tipo declarado da função (%s)",
                type_to_string(return_value_type),
                type_to_string(analyzer->current_function_return_type));
            analyzer->had_error = true;
        }
    }
    
    return return_value_type;
}

Type* analyze_print_stmt(SemanticAnalyzer* analyzer, ASTNode* node) {
    // Analisa a expressão sendo impressa
    Type* expr_type = analyze_expression(analyzer, node->print_stmt.expression);

    if (expr_type->kind != TYPE_ERROR) {
        bool is_supported = false;

        if (expr_type->kind == TYPE_INT ||
            expr_type->kind == TYPE_FLOAT ||
            expr_type->kind == TYPE_STRING ||
            expr_type->kind == TYPE_BOOL) {
            is_supported = true;
        } else if (expr_type->kind == TYPE_ARRAY &&
                   expr_type->element_type &&
                   (expr_type->element_type->kind == TYPE_INT ||
                    expr_type->element_type->kind == TYPE_FLOAT ||
                    expr_type->element_type->kind == TYPE_STRING ||
                    expr_type->element_type->kind == TYPE_BOOL)) {
            is_supported = true;
        }

        if (!is_supported) {
            analyzer->warning_count++;
            printf("Aviso [linha %d]: print() com tipo complexo %s pode não ter representação adequada\n",
                   node->line, type_to_string(expr_type));
        }
    }
    return create_primitive_type(TYPE_VOID);
}

Type* analyze_block(SemanticAnalyzer* analyzer, ASTNode* node) {
    Type* last_type = create_primitive_type(TYPE_VOID);
    
    for (int i = 0; i < node->block.stmt_count; i++) {
        if (last_type) free_type(last_type);
        last_type = analyze_statement(analyzer, node->block.statements[i]);
    }
    
    return last_type;
}

// ==================== ANÁLISE DE EXPRESSÕES ====================

Type* analyze_expression(SemanticAnalyzer* analyzer, ASTNode* node) {
    if (!node) return create_error_type();
    
    switch (node->type) {
        case AST_BINARY_EXPR:
            return analyze_binary_expr(analyzer, node);
        case AST_UNARY_EXPR:
            return analyze_unary_expr(analyzer, node);
        case AST_CALL_EXPR:
            return analyze_call_expr(analyzer, node);
        case AST_LAMBDA_EXPR:
            return analyze_lambda_expr(analyzer, node);
        case AST_PIPELINE_EXPR:
            return analyze_pipeline_expr(analyzer, node);
        case AST_LITERAL:
            return analyze_literal(analyzer, node);
        case AST_IDENTIFIER:
            return analyze_identifier(analyzer, node);
        case AST_MEMBER_EXPR:
            return analyze_member_expr(analyzer, node);
        case AST_INDEX_EXPR:
            return analyze_index_expr(analyzer, node);
        case AST_ASSIGN_EXPR:
            return analyze_assign_expr(analyzer, node);
        case AST_ARRAY_LITERAL:
            return analyze_array_literal(analyzer, node);
        case AST_RANGE_EXPR:
            return analyze_range_expr(analyzer, node);
        case AST_LOAD_EXPR:
            return analyze_load_expr(analyzer, node);
        case AST_SAVE_EXPR:
            return analyze_save_expr(analyzer, node);
        case AST_AGGREGATE_TRANSFORM:
            return analyze_aggregate_transform(analyzer, node);
        default:
            return create_error_type();
    }
}

// Análise de AggregateTransform (sum, mean, count, min, max como transformações)
Type* analyze_aggregate_transform(SemanticAnalyzer* analyzer, ASTNode* node) {
    // Analisa os argumentos
    for (int i = 0; i < node->aggregate_transform.agg_arg_count; i++) {
        analyze_expression(analyzer, node->aggregate_transform.agg_args[i]);
    }
    
    // Retorna o tipo baseado na agregação
    switch (node->aggregate_transform.agg_type) {
        case AGG_SUM:
        case AGG_MIN:
        case AGG_MAX:
        case AGG_COUNT:
            return create_primitive_type(TYPE_INT);
        case AGG_MEAN:
            return create_primitive_type(TYPE_FLOAT);
        default:
            return create_error_type();
    }
}

Type* analyze_binary_expr(SemanticAnalyzer* analyzer, ASTNode* node) {
    if (!node->binary_expr.left || !node->binary_expr.right) {
        symbol_table_error(analyzer->symbol_table, node->line, node->column,
            "Expressão binária com operandos inválidos");
        analyzer->had_error = true;
        return create_error_type();
    }

    Type* left = analyze_expression(analyzer, node->binary_expr.left);
    Type* right = analyze_expression(analyzer, node->binary_expr.right);
    
    // Determina o operador
    const char* op_str = NULL;
    switch (node->binary_expr.op) {
        case BINOP_ADD: op_str = "+"; break;
        case BINOP_SUB: op_str = "-"; break;
        case BINOP_MUL: op_str = "*"; break;
        case BINOP_DIV: op_str = "/"; break;
        case BINOP_MOD: op_str = "%"; break;
        case BINOP_EQ: op_str = "=="; break;
        case BINOP_NEQ: op_str = "!="; break;
        case BINOP_LT: op_str = "<"; break;
        case BINOP_LTE: op_str = "<="; break;
        case BINOP_GT: op_str = ">"; break;
        case BINOP_GTE: op_str = ">="; break;
        case BINOP_AND: op_str = "&&"; break;
        case BINOP_OR: op_str = "||"; break;
        case BINOP_RANGE: op_str = ".."; break;
        default: op_str = "?"; break;
    }
    
    // Verifica compatibilidade rigorosa
    if (!check_binary_op_types(analyzer, left, right, op_str, 
                                node->line, node->column)) {
        analyzer->had_error = true;
        return create_error_type();
    }
    
    // Retorna tipo resultante
    return get_result_type_binary_op(left, right, op_str);
}

Type* analyze_unary_expr(SemanticAnalyzer* analyzer, ASTNode* node) {
    Type* operand = analyze_expression(analyzer, node->unary_expr.operand);
    
    // Verifica tipo apropriado
    if (node->unary_expr.op == UNOP_NEG) {
        if (!is_numeric_type(operand) && operand->kind != TYPE_ERROR) {
            symbol_table_error(analyzer->symbol_table, node->line, node->column,
                "Operador '-' requer tipo numérico, mas encontrado %s",
                type_to_string(operand));
            analyzer->had_error = true;
            return create_error_type();
        }
        return clone_type(operand);
    }
    
    if (node->unary_expr.op == UNOP_NOT) {
        if (operand->kind != TYPE_BOOL && operand->kind != TYPE_ERROR) {
            symbol_table_error(analyzer->symbol_table, node->line, node->column,
                "Operador '!' requer tipo booleano, mas encontrado %s",
                type_to_string(operand));
            analyzer->had_error = true;
            return create_error_type();
        }
        return create_primitive_type(TYPE_BOOL);
    }
    
    return create_error_type();
}

Type* analyze_call_expr(SemanticAnalyzer* analyzer, ASTNode* node) {
    char* func_name = node->call_expr.callee->identifier.id_name;
    
    // Verifica se é print (tratamento especial - aceita qualquer tipo)
    if (strcmp(func_name, "print") == 0) {
        if (node->call_expr.arg_count != 1) {
            symbol_table_error(analyzer->symbol_table, node->line, node->column,
                "Função 'print' espera 1 argumento, mas recebeu %d",
                node->call_expr.arg_count);
            analyzer->had_error = true;
            return create_error_type();
        }
        
        // Analisa o argumento (pode ser qualquer tipo)
        analyze_expression(analyzer, node->call_expr.arguments[0]);
        return create_primitive_type(TYPE_VOID);
    }
    
    // Busca a função na tabela de símbolos
    Symbol* func_symbol = lookup_symbol(analyzer->symbol_table, func_name);
    
    if (!func_symbol) {
        symbol_table_error(analyzer->symbol_table, node->line, node->column,
            "Função '%s' não foi declarada", func_name);
        analyzer->had_error = true;
        return create_error_type();
    }
    
    if (func_symbol->kind != SYMBOL_FUNCTION) {
        symbol_table_error(analyzer->symbol_table, node->line, node->column,
            "'%s' não é uma função", func_name);
        analyzer->had_error = true;
        return create_error_type();
    }
    
    // Verifica número de argumentos
    if (node->call_expr.arg_count != func_symbol->param_count) {
        symbol_table_error(analyzer->symbol_table, node->line, node->column,
            "Função '%s' espera %d argumentos mas recebeu %d",
            func_name, func_symbol->param_count, node->call_expr.arg_count);
        analyzer->had_error = true;
        return create_error_type();
    }
    
    // Verifica tipos dos argumentos
    for (int i = 0; i < node->call_expr.arg_count; i++) {
        Type* arg_type = analyze_expression(analyzer, node->call_expr.arguments[i]);
        Type* param_type = func_symbol->param_types[i];
        
        if (!types_compatible(param_type, arg_type)) {
            symbol_table_error(analyzer->symbol_table, 
                node->call_expr.arguments[i]->line,
                node->call_expr.arguments[i]->column,
                "Argumento %d de '%s': esperado %s mas encontrado %s",
                i + 1, func_name, type_to_string(param_type), type_to_string(arg_type));
            analyzer->had_error = true;
        }
    }
    
    // Marca função como usada
    mark_symbol_used(analyzer->symbol_table, func_name);
    
    return clone_type(func_symbol->type->return_type);
}

Type* analyze_lambda_expr(SemanticAnalyzer* analyzer, ASTNode* node) {
    // Entra em novo escopo
    enter_scope(analyzer->symbol_table);
    
    // Declara parâmetros
    Type** param_types = malloc(node->lambda_expr.lambda_param_count * sizeof(Type*));
    for (int i = 0; i < node->lambda_expr.lambda_param_count; i++) {
        ASTNode* param = node->lambda_expr.lambda_params[i];
        
        Type* param_type;
        if (param->param.param_type) {
            param_type = ast_type_to_type(analyzer, param->param.param_type);
        } else {
            // Inferência de tipo
            param_type = fresh_type_var(analyzer->inference_ctx->var_gen);
        }
        
        param_types[i] = param_type;
        
        Symbol* param_symbol = declare_symbol(analyzer->symbol_table,
                                              param->param.param_name,
                                              SYMBOL_PARAMETER,
                                              clone_type(param_type),
                                              param->line, param->column);
        if (param_symbol) {
            param_symbol->initialized = true;
        }
    }
    
    // Analisa corpo
    Type* body_type = analyze_expression(analyzer, node->lambda_expr.lambda_body);
    
    // Sai do escopo
    exit_scope(analyzer->symbol_table);
    
    // Cria tipo de função
    Type* lambda_type = create_function_type(param_types, 
                                             node->lambda_expr.lambda_param_count,
                                             body_type);
    free(param_types);
    
    return lambda_type;
}

Type* analyze_pipeline_expr(SemanticAnalyzer* analyzer, ASTNode* node) {
    Type* current_type = NULL;
    
    for (int i = 0; i < node->pipeline_expr.stage_count; i++) {
        ASTNode* stage = node->pipeline_expr.stages[i];
        Type* stage_type = analyze_expression(analyzer, stage);
        
        if (i == 0) {
            current_type = stage_type;
        } else {
            current_type = stage_type;
        }
    }
    
    return current_type;
}

Type* analyze_literal(SemanticAnalyzer* analyzer, ASTNode* node) {
    (void)analyzer; // Parâmetro não usado nesta função
    switch (node->literal.literal_type) {
        case TOKEN_INTEGER:
            return create_primitive_type(TYPE_INT);
        case TOKEN_FLOAT:
            return create_primitive_type(TYPE_FLOAT);
        case TOKEN_STRING:
            return create_primitive_type(TYPE_STRING);
        case TOKEN_BOOL_TYPE:
            return create_primitive_type(TYPE_BOOL);
        default:
            return create_error_type();
    }
}

Type* analyze_identifier(SemanticAnalyzer* analyzer, ASTNode* node) {
    Symbol* symbol = lookup_symbol(analyzer->symbol_table, node->identifier.id_name);
    
    // Verifica se foi declarado
    if (!symbol) {
        symbol_table_error(analyzer->symbol_table, node->line, node->column,
            "Identificador '%s' não declarado", node->identifier.id_name);
        analyzer->had_error = true;
        return create_error_type();
    }
    
    // Verifica se foi inicializado
    if (!symbol->initialized && symbol->kind == SYMBOL_VARIABLE) {
        symbol_table_error(analyzer->symbol_table, node->line, node->column,
            "Variável '%s' pode estar sendo usada antes de ser inicializada",
            node->identifier.id_name);
        analyzer->warning_count++;
    }
    
    // Marca como usado
    mark_symbol_used(analyzer->symbol_table, node->identifier.id_name);
    
    return clone_type(symbol->type);
}

Type* analyze_array_literal(SemanticAnalyzer* analyzer, ASTNode* node) {
    if (node->array_literal.element_count == 0) {
        // Array vazio: tipo [T] onde T é variável
        Type* elem_type = fresh_type_var(analyzer->inference_ctx->var_gen);
        return create_array_type(elem_type);
    }
    
    // Analisa primeiro elemento
    Type* elem_type = analyze_expression(analyzer, node->array_literal.elements[0]);
    
    // Verifica que todos os elementos têm o mesmo tipo
    for (int i = 1; i < node->array_literal.element_count; i++) {
        Type* current = analyze_expression(analyzer, node->array_literal.elements[i]);
        
        if (!types_compatible(elem_type, current)) {
            symbol_table_error(analyzer->symbol_table,
                node->array_literal.elements[i]->line,
                node->array_literal.elements[i]->column,
                "Elemento do array tem tipo %s mas esperava-se %s",
                type_to_string(current), type_to_string(elem_type));
            analyzer->had_error = true;
        }
    }
    
    return create_array_type(elem_type);
}

// Implementações restantes
Type* analyze_member_expr(SemanticAnalyzer* analyzer, ASTNode* node) {
    Type* object_type = analyze_expression(analyzer, node->member_expr.object);
    
    if (object_type->kind == TYPE_CUSTOM) {
        Symbol* type_symbol = lookup_symbol(analyzer->symbol_table, 
                                           object_type->custom_name);
        if (type_symbol && type_symbol->kind == SYMBOL_TYPE) {
            for (int i = 0; i < type_symbol->field_count; i++) {
                if (strcmp(type_symbol->fields[i]->name, 
                          node->member_expr.member) == 0) {
                    return clone_type(type_symbol->fields[i]->type);
                }
            }
        }
        
        symbol_table_error(analyzer->symbol_table, node->line, node->column,
            "Tipo '%s' não possui campo '%s'",
            object_type->custom_name, node->member_expr.member);
        analyzer->had_error = true;
        return create_error_type();
    }
    
    if (object_type->kind == TYPE_DATAFRAME) {
        return fresh_type_var(analyzer->inference_ctx->var_gen);
    }
    
    symbol_table_error(analyzer->symbol_table, node->line, node->column,
        "Tipo %s não suporta acesso a membros", type_to_string(object_type));
    analyzer->had_error = true;
    return create_error_type();
}

Type* analyze_index_expr(SemanticAnalyzer* analyzer, ASTNode* node) {
    Type* object_type = analyze_expression(analyzer, node->index_expr.object);
    Type* index_type = analyze_expression(analyzer, node->index_expr.index);
    
    // Índice deve ser inteiro
    if (index_type->kind != TYPE_INT && index_type->kind != TYPE_ERROR) {
        symbol_table_error(analyzer->symbol_table, node->line, node->column,
            "Índice deve ser inteiro, mas encontrado %s", type_to_string(index_type));
        analyzer->had_error = true;
    }
    
    // Retorna tipo do elemento
    if (object_type->kind == TYPE_ARRAY) {
        return clone_type(object_type->element_type);
    }
    
    if (object_type->kind == TYPE_ERROR) {
        return create_error_type();
    }
    
    symbol_table_error(analyzer->symbol_table, node->line, node->column,
        "Tipo %s não é indexável", type_to_string(object_type));
    analyzer->had_error = true;
    return create_error_type();
}

Type* analyze_assign_expr(SemanticAnalyzer* analyzer, ASTNode* node) {
    // Verifica que o alvo é um lvalue
    if (!is_lvalue(node->assign_expr.target)) {
        symbol_table_error(analyzer->symbol_table, node->line, node->column,
            "Lado esquerdo da atribuição não é um lvalue válido");
        analyzer->had_error = true;
        return create_error_type();
    }
    
    Type* target_type = analyze_expression(analyzer, node->assign_expr.target);
    Type* value_type = analyze_expression(analyzer, node->assign_expr.value);
    
    // Verifica compatibilidade de tipos
    if (!types_compatible(target_type, value_type)) {
        symbol_table_error(analyzer->symbol_table, node->line, node->column,
            "Tipo do valor (%s) incompatível com tipo do alvo (%s)",
            type_to_string(value_type), type_to_string(target_type));
        analyzer->had_error = true;
        return create_error_type();
    }
    
    return clone_type(target_type);
}

Type* analyze_range_expr(SemanticAnalyzer* analyzer, ASTNode* node) {
    Type* start_type = analyze_expression(analyzer, node->range_expr.range_start);
    Type* end_type = analyze_expression(analyzer, node->range_expr.range_end);
    
    // Ambos devem ser inteiros
    if (start_type->kind != TYPE_INT && start_type->kind != TYPE_ERROR) {
        symbol_table_error(analyzer->symbol_table, node->line, node->column,
            "Início do range deve ser inteiro, mas encontrado %s",
            type_to_string(start_type));
        analyzer->had_error = true;
    }
    
    if (end_type->kind != TYPE_INT && end_type->kind != TYPE_ERROR) {
        symbol_table_error(analyzer->symbol_table, node->line, node->column,
            "Fim do range deve ser inteiro, mas encontrado %s",
            type_to_string(end_type));
        analyzer->had_error = true;
    }
    
    // Range produz um array de inteiros
    return create_array_type(create_primitive_type(TYPE_INT));
}

Type* analyze_load_expr(SemanticAnalyzer* analyzer, ASTNode* node) {
    (void)analyzer; // Parâmetro não usado
    (void)node;     // Parâmetro não usado (path da string não é validado aqui)
    // load() retorna DataFrame
    return create_primitive_type(TYPE_DATAFRAME);
}

Type* analyze_save_expr(SemanticAnalyzer* analyzer, ASTNode* node) {
    // Analisa dados sendo salvos
    analyze_expression(analyzer, node->save_expr.data);
    
    // save() retorna void
    return create_primitive_type(TYPE_VOID);
}

// ==================== VERIFICAÇÕES ESPECÍFICAS ====================

bool check_all_paths_return(SemanticAnalyzer* analyzer, ASTNode* block) {
    if (!block || block->type != AST_BLOCK) return false;
    
    for (int i = 0; i < block->block.stmt_count; i++) {
        ASTNode* stmt = block->block.statements[i];
        
        // Return direto
        if (stmt->type == AST_RETURN_STMT) {
            return true;
        }
        
        // If-else onde ambos os ramos retornam
        if (stmt->type == AST_IF_STMT) {
            if (stmt->if_stmt.else_block) {
                bool then_returns = false;
                bool else_returns = false;
                
                if (stmt->if_stmt.then_block->type == AST_BLOCK) {
                    then_returns = check_all_paths_return(analyzer, 
                                                         stmt->if_stmt.then_block);
                }
                
                if (stmt->if_stmt.else_block->type == AST_BLOCK) {
                    else_returns = check_all_paths_return(analyzer,
                                                         stmt->if_stmt.else_block);
                } else if (stmt->if_stmt.else_block->type == AST_IF_STMT) {
                    else_returns = check_all_paths_return(analyzer,
                        stmt->if_stmt.else_block);
                }
                
                if (then_returns && else_returns) {
                    return true;
                }
            }
        }
    }
    
    return false;
}

bool is_lvalue(ASTNode* node) {
    if (!node) return false;
    
    switch (node->type) {
        case AST_IDENTIFIER:
            return true;
        case AST_MEMBER_EXPR:
            return is_lvalue(node->member_expr.object);
        case AST_INDEX_EXPR:
            return is_lvalue(node->index_expr.object);
        default:
            return false;
    }
}

bool check_binary_op_types(SemanticAnalyzer* analyzer, 
                           Type* left, Type* right, const char* op,
                           int line, int column) {
    // Ignora erros em cascata
    if (left->kind == TYPE_ERROR || right->kind == TYPE_ERROR) {
        return true;
    }
    
    // Operadores aritméticos
    if (strcmp(op, "+") == 0 || strcmp(op, "-") == 0 ||
        strcmp(op, "*") == 0 || strcmp(op, "/") == 0 || strcmp(op, "%") == 0) {
        
        if (!is_numeric_type(left)) {
            symbol_table_error(analyzer->symbol_table, line, column,
                "Operador '%s' requer operando numérico esquerdo, mas encontrado %s",
                op, type_to_string(left));
            return false;
        }
        
        if (!is_numeric_type(right)) {
            symbol_table_error(analyzer->symbol_table, line, column,
                "Operador '%s' requer operando numérico direito, mas encontrado %s",
                op, type_to_string(right));
            return false;
        }
        
        // Ambos devem ser do mesmo tipo ou compatíveis
        if (!types_compatible(left, right)) {
            symbol_table_error(analyzer->symbol_table, line, column,
                "Operador '%s' requer operandos do mesmo tipo, mas encontrado %s e %s",
                op, type_to_string(left), type_to_string(right));
            return false;
        }
        
        return true;
    }
    
    // Operadores de comparação
    if (strcmp(op, "<") == 0 || strcmp(op, "<=") == 0 ||
        strcmp(op, ">") == 0 || strcmp(op, ">=") == 0) {
        
        if (!is_comparable_type(left) || !is_comparable_type(right)) {
            symbol_table_error(analyzer->symbol_table, line, column,
                "Operador '%s' requer operandos comparáveis", op);
            return false;
        }
        
        if (!types_compatible(left, right)) {
            symbol_table_error(analyzer->symbol_table, line, column,
                "Operador '%s' requer operandos do mesmo tipo, mas encontrado %s e %s",
                op, type_to_string(left), type_to_string(right));
            return false;
        }
        
        return true;
    }
    
    // Operadores de igualdade
    if (strcmp(op, "==") == 0 || strcmp(op, "!=") == 0) {
        if (!types_compatible(left, right)) {
            symbol_table_error(analyzer->symbol_table, line, column,
                "Operador '%s' requer operandos compatíveis, mas encontrado %s e %s",
                op, type_to_string(left), type_to_string(right));
            return false;
        }
        return true;
    }
    
    // Operadores lógicos
    if (strcmp(op, "&&") == 0 || strcmp(op, "||") == 0) {
        if (left->kind != TYPE_BOOL) {
            symbol_table_error(analyzer->symbol_table, line, column,
                "Operador '%s' requer operando booleano esquerdo, mas encontrado %s",
                op, type_to_string(left));
            return false;
        }
        
        if (right->kind != TYPE_BOOL) {
            symbol_table_error(analyzer->symbol_table, line, column,
                "Operador '%s' requer operando booleano direito, mas encontrado %s",
                op, type_to_string(right));
            return false;
        }
        
        return true;
    }
    
    // Range
    if (strcmp(op, "..") == 0) {
        if (left->kind != TYPE_INT) {
            symbol_table_error(analyzer->symbol_table, line, column,
                "Operador '..' requer inteiro à esquerda, mas encontrado %s",
                type_to_string(left));
            return false;
        }
        
        if (right->kind != TYPE_INT) {
            symbol_table_error(analyzer->symbol_table, line, column,
                "Operador '..' requer inteiro à direita, mas encontrado %s",
                type_to_string(right));
            return false;
        }
        
        return true;
    }
    
    return true;
}

// ==================== CONVERSÃO DE TIPOS DA AST ====================

Type* ast_type_to_type(SemanticAnalyzer* analyzer, ASTNode* type_node) {
    if (!type_node || type_node->type != AST_TYPE) {
        return create_error_type();
    }
    
    switch (type_node->type_node.type_kind) {
        case TOKEN_INT_TYPE:
            return create_primitive_type(TYPE_INT);
        case TOKEN_FLOAT_TYPE:
            return create_primitive_type(TYPE_FLOAT);
        case TOKEN_STRING_TYPE:
            return create_primitive_type(TYPE_STRING);
        case TOKEN_BOOL_TYPE:
            return create_primitive_type(TYPE_BOOL);
        case TOKEN_DATAFRAME_TYPE:
            return create_primitive_type(TYPE_DATAFRAME);
        case TOKEN_VECTOR_TYPE:
            // Vector é tratado como array de tipo genérico
            return create_array_type(create_primitive_type(TYPE_INT));
        case TOKEN_SERIES_TYPE:
            // Series é tratado como array de tipo genérico
            return create_array_type(create_primitive_type(TYPE_INT));
            
        case TOKEN_LBRACKET: {
            // Array type
            Type* elem_type = ast_type_to_type(analyzer, type_node->type_node.inner_type);
            return create_array_type(elem_type);
        }
        
        case TOKEN_LPAREN: {
            // Tuple type
            Type** types = malloc(type_node->type_node.tuple_type_count * sizeof(Type*));
            for (int i = 0; i < type_node->type_node.tuple_type_count; i++) {
                types[i] = ast_type_to_type(analyzer, type_node->type_node.tuple_types[i]);
            }
            return create_tuple_type(types, type_node->type_node.tuple_type_count);
        }
        
        case TOKEN_IDENTIFIER: {
            if (strcmp(type_node->type_node.type_name, "Void") == 0) {
                return create_primitive_type(TYPE_VOID);
            }
            return create_custom_type(type_node->type_node.type_name);
        }
        
        default:
            return create_error_type();
    }
}

// ==================== RELATÓRIOS E DIAGNÓSTICOS ====================

void print_semantic_analysis_report(SemanticAnalyzer* analyzer) {
    printf("\n╔═══════════════════════════════════════════════════════════╗\n");
    printf("║           RELATÓRIO DE ANÁLISE SEMÂNTICA                  ║\n");
    printf("╚═══════════════════════════════════════════════════════════╝\n\n");
    
    // Erros da tabela de símbolos
    if (analyzer->symbol_table->error_count > 0) {
        print_symbol_table_errors(analyzer->symbol_table);
    }
    
    // Erros de inferência
    if (analyzer->inference_ctx->error_count > 0) {
        print_inference_errors(analyzer->inference_ctx);
    }
    
    // Estatísticas
    printf("Estatísticas:\n");
    printf("────────────────────────────────────────────────────────────\n");
    printf("  Erros: %d\n", analyzer->symbol_table->error_count + 
                               analyzer->inference_ctx->error_count);
    printf("  Avisos: %d\n", analyzer->warning_count);
    
    // Tabela de símbolos
    if (analyzer->had_error) {
        printf("\n❌ Análise semântica concluída com erros\n");
    } else {
        printf("\n✅ Análise semântica concluída com sucesso!\n");
        printf("\nTabela de Símbolos Final:\n");
        printf("────────────────────────────────────────────────────────────\n");
        print_symbol_table(analyzer->symbol_table);
    }
}