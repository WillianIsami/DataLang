/*
 * DataLang - Analisador Semântico
 */

#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "semantic_analyzer.h"

// ==================== DECLARAÇÕES FORWARD ====================

Type* analyze_member_expr(SemanticAnalyzer* analyzer, ASTNode* node);
Type* analyze_index_expr(SemanticAnalyzer* analyzer, ASTNode* node);
Type* analyze_assign_expr(SemanticAnalyzer* analyzer, ASTNode* node);
Type* analyze_array_literal(SemanticAnalyzer* analyzer, ASTNode* node);
Type* analyze_range_expr(SemanticAnalyzer* analyzer, ASTNode* node);
Type* analyze_load_expr(SemanticAnalyzer* analyzer, ASTNode* node);
Type* analyze_save_expr(SemanticAnalyzer* analyzer, ASTNode* node);
Type* analyze_print_stmt(SemanticAnalyzer* analyzer, ASTNode* node);
Type* analyze_aggregate_transform(SemanticAnalyzer* analyzer, ASTNode* node);
static Type* analyze_lambda_with_expectations(SemanticAnalyzer* analyzer, ASTNode* node,
                                              Type** expected_params, int expected_param_count,
                                              Type* expected_return);
Type* analyze_filter_transform(SemanticAnalyzer* analyzer, ASTNode* node, Type* input_type);
Type* analyze_map_transform(SemanticAnalyzer* analyzer, ASTNode* node, Type* input_type);
Type* analyze_reduce_transform(SemanticAnalyzer* analyzer, ASTNode* node, Type* input_type);
Type* analyze_select_transform(SemanticAnalyzer* analyzer, ASTNode* node);
Type* analyze_groupby_transform(SemanticAnalyzer* analyzer, ASTNode* node);

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
    
    // print(x) -> Void (polimórfico)
    {
        Type* param_types[1] = {create_primitive_type(TYPE_INT)};
        Type* return_type = create_primitive_type(TYPE_VOID);
        declare_function(analyzer->symbol_table, "print", return_type, param_types, 1, 0, 0);
    }
    
    // sum(array: [Int]) -> Int
    {
        Type* param_types[1] = { create_array_type(create_primitive_type(TYPE_INT)) };
        Type* return_type = create_primitive_type(TYPE_INT);
        declare_function(analyzer->symbol_table, "sum", return_type, param_types, 1, 0, 0);
    }
    
    // mean(array: [Int]) -> Float
    {
        Type* param_types[1] = { create_array_type(create_primitive_type(TYPE_INT)) };
        Type* return_type = create_primitive_type(TYPE_FLOAT);
        declare_function(analyzer->symbol_table, "mean", return_type, param_types, 1, 0, 0);
    }
    
    // count(array: [Int]) -> Int
    {
        Type* param_types[1] = { create_array_type(create_primitive_type(TYPE_INT)) };
        Type* return_type = create_primitive_type(TYPE_INT);
        declare_function(analyzer->symbol_table, "count", return_type, param_types, 1, 0, 0);
    }
    
    // min(array: [Int]) -> Int
    {
        Type* param_types[1] = { create_array_type(create_primitive_type(TYPE_INT)) };
        Type* return_type = create_primitive_type(TYPE_INT);
        declare_function(analyzer->symbol_table, "min", return_type, param_types, 1, 0, 0);
    }
    
    // max(array: [Int]) -> Int
    {
        Type* param_types[1] = { create_array_type(create_primitive_type(TYPE_INT)) };
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
    
    register_builtin_functions(analyzer);
    
    // Fase 1: Coleta de Declarações
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
            if (existing) {
                // Já declarado, pular silenciosamente
                continue;
            }
            
            // Registra campos
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
            
            // Registra tipo APENAS UMA VEZ
            declare_type(analyzer->symbol_table, decl->data_decl.name,
                        fields, decl->data_decl.field_count,
                        decl->line, decl->column);
            
            free(fields);
        }
    }
    
    // Fase 2: Analisa corpos
    printf("[Fase 2] Analisando corpos e instruções...\n");
    for (int i = 0; i < program->program.decl_count; i++) {
        ASTNode* decl = program->program.declarations[i];
        
        switch (decl->type) {
            case AST_FN_DECL:
                if (!analyzer->had_error || !lookup_symbol(analyzer->symbol_table, decl->fn_decl.name)) {
                    analyze_fn_decl(analyzer, decl);
                }
                break;
            case AST_DATA_DECL:
                analyze_data_decl(analyzer, decl);
                break;
            default:
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
    Type* init_type = NULL;
    
    if (node->let_decl.initializer) {
        init_type = analyze_expression(analyzer, node->let_decl.initializer);
    }
    
    Type* declared_type = NULL;
    if (node->let_decl.type_annotation) {
        declared_type = ast_type_to_type(analyzer, node->let_decl.type_annotation);
        
        if (declared_type->kind == TYPE_ARRAY && init_type && init_type->kind == TYPE_ARRAY) {
            // Arrays são compatíveis se elementos são do mesmo tipo base
            if (declared_type->element_type->kind == TYPE_CUSTOM && 
                init_type->element_type->kind == TYPE_CUSTOM) {
                // Ambos são arrays de tipos customizados - aceitar
                if (strcmp(declared_type->element_type->custom_name,
                          init_type->element_type->custom_name) != 0) {
                    symbol_table_error(analyzer->symbol_table, node->line, node->column,
                        "Tipo de inicializador ([%s]) incompatível com tipo declarado ([%s])",
                        init_type->element_type->custom_name, 
                        declared_type->element_type->custom_name);
                    analyzer->had_error = true;
                    return create_error_type();
                }
            } else if (!types_compatible(declared_type->element_type, init_type->element_type)) {
                symbol_table_error(analyzer->symbol_table, node->line, node->column,
                    "Tipo de elemento de array incompatível");
                analyzer->had_error = true;
                return create_error_type();
            }
        } else if (!types_compatible(declared_type, init_type)) {
            symbol_table_error(analyzer->symbol_table, node->line, node->column,
                "Tipo de inicializador (%s) incompatível com tipo declarado (%s)",
                type_to_string(init_type), type_to_string(declared_type));
            analyzer->had_error = true;
            return create_error_type();
        }
    }
    
    Type* var_type = declared_type ? declared_type : init_type;
    Symbol* symbol = declare_symbol(analyzer->symbol_table, 
                                    node->let_decl.name, SYMBOL_VARIABLE,
                                    clone_type(var_type), node->line, node->column);
    
    if (symbol) {
        symbol->initialized = true;
        symbol->used = false;
    } else {
        analyzer->had_error = true;
    }
    
    return var_type;
}

Type* analyze_fn_decl(SemanticAnalyzer* analyzer, ASTNode* node) {
    enter_scope(analyzer->symbol_table);
    
    bool was_in_function = analyzer->in_function;
    analyzer->in_function = true;
    
    Type* return_type = node->fn_decl.return_type ?
        ast_type_to_type(analyzer, node->fn_decl.return_type) :
        create_primitive_type(TYPE_VOID);
    
    Type* old_return_type = analyzer->current_function_return_type;
    analyzer->current_function_return_type = return_type;
    
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
    
    analyze_block(analyzer, node->fn_decl.body);
    
    if (return_type->kind != TYPE_VOID) {
        if (!check_all_paths_return(analyzer, node->fn_decl.body)) {
            // Aviso apenas, não erro
            analyzer->warning_count++;
            printf("Aviso [linha %d]: Função '%s' pode não retornar valor em todos os caminhos\n",
                   node->line, node->fn_decl.name);
        }
    }
    
    analyzer->in_function = was_in_function;
    analyzer->current_function_return_type = old_return_type;
    
    exit_scope(analyzer->symbol_table);
    
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
    for (int i = 0; i < node->data_decl.field_count; i++) {
        if (node->data_decl.fields[i]->type == AST_FIELD_DECL) {
            ASTNode* field = node->data_decl.fields[i];
            Type* field_type = ast_type_to_type(analyzer, field->field_decl.field_type);
            if (field_type->kind == TYPE_ERROR) {
                symbol_table_error(analyzer->symbol_table, field->line, field->column,
                    "Tipo inválido para campo '%s'", field->field_decl.field_name);
                analyzer->had_error = true;
            }
        }
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
        case AST_IMPORT_DECL:
        case AST_EXPORT_DECL:
            // Módulos ainda não implementados - ignorar silenciosamente
            return create_primitive_type(TYPE_VOID);
        case AST_EXPR_STMT:
            if (node->expr_stmt.expression) {
                return analyze_expression(analyzer, node->expr_stmt.expression);
            } else {
                return create_error_type();
            }
        case AST_BLOCK:
            return analyze_block(analyzer, node);
        default:
            return create_error_type();
    }
}

Type* analyze_if_stmt(SemanticAnalyzer* analyzer, ASTNode* node) {
    Type* cond_type = analyze_expression(analyzer, node->if_stmt.condition);
    
    if (cond_type->kind != TYPE_BOOL && cond_type->kind != TYPE_ERROR) {
        symbol_table_error(analyzer->symbol_table, node->line, node->column,
            "Condição do 'if' deve ser booleana, mas encontrado %s",
            type_to_string(cond_type));
        analyzer->had_error = true;
    }
    
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
    enter_scope(analyzer->symbol_table);
    
    bool was_in_loop = analyzer->in_loop;
    analyzer->in_loop = true;
    
    Type* iterable_type = analyze_expression(analyzer, node->for_stmt.iterable);
    
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
    
    Symbol* iter_symbol = declare_symbol(analyzer->symbol_table, node->for_stmt.iterator,
                  SYMBOL_VARIABLE, element_type, node->line, node->column);
    if (iter_symbol) {
        mark_symbol_initialized(analyzer->symbol_table, node->for_stmt.iterator);
    }
    
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
    // Analisa todas as expressões
    for (int i = 0; i < node->print_stmt.expr_count; i++) {
        Type* expr_type = analyze_expression(analyzer, node->print_stmt.expressions[i]);
        
        // Print aceita qualquer tipo, mas avisa sobre tipos complexos
        if (expr_type->kind != TYPE_INT && 
            expr_type->kind != TYPE_FLOAT &&
            expr_type->kind != TYPE_STRING &&
            expr_type->kind != TYPE_BOOL &&
            expr_type->kind != TYPE_ARRAY &&
            expr_type->kind != TYPE_ERROR) {
            
            analyzer->warning_count++;
            printf("Aviso [linha %d]: print() argumento %d com tipo complexo %s\n",
                   node->line, i+1, type_to_string(expr_type));
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
        case AST_FILTER_TRANSFORM:
            return analyze_filter_transform(analyzer, node, NULL);
        case AST_MAP_TRANSFORM:
            return analyze_map_transform(analyzer, node, NULL);
        case AST_REDUCE_TRANSFORM:
            return analyze_reduce_transform(analyzer, node, NULL);
        case AST_SELECT_TRANSFORM:
            return analyze_select_transform(analyzer, node);
        case AST_GROUPBY_TRANSFORM:
            return analyze_groupby_transform(analyzer, node);
        default:
            return create_error_type();
    }
}

// Análise de transformações de pipeline
Type* analyze_filter_transform(SemanticAnalyzer* analyzer, ASTNode* node, Type* input_type) {
    Type* elem_type = NULL;
    if (input_type) {
        if (input_type->kind == TYPE_ARRAY) {
            elem_type = input_type->element_type;
        } else if (input_type->kind == TYPE_DATAFRAME) {
            // Permite filter em DataFrame com linha genérica
            elem_type = create_custom_type("Row");
        } else {
            symbol_table_error(analyzer->symbol_table, node->line, node->column,
                "Transformação filter espera um array ou DataFrame como entrada, mas recebeu %s",
                type_to_string(input_type));
            analyzer->had_error = true;
        }
    }
    
    if (node->filter_transform.filter_predicate) {
        if (elem_type) {
            Type* expected_params[1] = { elem_type };
            analyze_lambda_with_expectations(analyzer, node->filter_transform.filter_predicate,
                                             expected_params, 1,
                                             create_primitive_type(TYPE_BOOL));
        } else {
            analyze_expression(analyzer, node->filter_transform.filter_predicate);
        }
    }
    // Filter retorna mesmo tipo da entrada
    if (input_type && (input_type->kind == TYPE_ARRAY || input_type->kind == TYPE_DATAFRAME)) {
        return clone_type(input_type);
    }
    return create_array_type(create_primitive_type(TYPE_INT));
}

Type* analyze_map_transform(SemanticAnalyzer* analyzer, ASTNode* node, Type* input_type) {
    Type* elem_type = NULL;
    bool input_is_df = false;
    if (input_type) {
        if (input_type->kind == TYPE_ARRAY) {
            elem_type = input_type->element_type;
        } else if (input_type->kind == TYPE_DATAFRAME) {
            input_is_df = true;
            elem_type = create_custom_type("Row");
        } else {
            symbol_table_error(analyzer->symbol_table, node->line, node->column,
                "Transformação map espera um array ou DataFrame como entrada, mas recebeu %s",
                type_to_string(input_type));
            analyzer->had_error = true;
        }
    }
    
    Type* lambda_type = NULL;
    if (node->map_transform.map_function) {
        if (elem_type) {
            Type* expected_params[1] = { elem_type };
            lambda_type = analyze_lambda_with_expectations(analyzer, node->map_transform.map_function,
                                                           expected_params, 1, NULL);
        } else {
            lambda_type = analyze_expression(analyzer, node->map_transform.map_function);
        }
    }
    
    // Se for DataFrame, devolve array do tipo de retorno da lambda (quando conhecido)
    if (input_is_df) {
        if (lambda_type && lambda_type->kind == TYPE_FUNCTION && lambda_type->return_type) {
            return create_array_type(clone_type(lambda_type->return_type));
        }
        return create_primitive_type(TYPE_DATAFRAME);
    }
    
    if (lambda_type && lambda_type->kind == TYPE_FUNCTION && lambda_type->return_type) {
        return create_array_type(clone_type(lambda_type->return_type));
    }
    
    if (input_type && input_type->kind == TYPE_ARRAY) {
        return create_array_type(clone_type(input_type->element_type));
    }
    
    return create_array_type(create_primitive_type(TYPE_INT));
}

Type* analyze_reduce_transform(SemanticAnalyzer* analyzer, ASTNode* node, Type* input_type) {
    Type* init_type = NULL;
    if (node->reduce_transform.initial_value) {
        init_type = analyze_expression(analyzer, node->reduce_transform.initial_value);
    }
    
    Type* elem_type = NULL;
    if (input_type) {
        if (input_type->kind == TYPE_ARRAY) {
            elem_type = input_type->element_type;
        } else if (input_type->kind == TYPE_DATAFRAME) {
            elem_type = create_custom_type("Row");
        } else {
            symbol_table_error(analyzer->symbol_table, node->line, node->column,
                "Transformação reduce espera um array ou DataFrame como entrada, mas recebeu %s",
                type_to_string(input_type));
            analyzer->had_error = true;
        }
    }
    
    if (node->reduce_transform.reducer) {
        if (init_type && elem_type) {
            Type* expected_params[2] = { init_type, elem_type };
            Type* lambda_type = analyze_lambda_with_expectations(
                analyzer, node->reduce_transform.reducer,
                expected_params, 2, init_type);
            
            if (lambda_type && lambda_type->kind == TYPE_FUNCTION &&
                lambda_type->return_type && init_type->kind != TYPE_ERROR &&
                !types_compatible(init_type, lambda_type->return_type)) {
                symbol_table_error(analyzer->symbol_table, node->line, node->column,
                    "Retorno do reduce (%s) incompatível com tipo do acumulador (%s)",
                    type_to_string(lambda_type->return_type), type_to_string(init_type));
                analyzer->had_error = true;
            }
        } else {
            analyze_expression(analyzer, node->reduce_transform.reducer);
        }
    }
    
    if (init_type) {
        return clone_type(init_type);
    }
    return create_primitive_type(TYPE_INT);
}

Type* analyze_select_transform(SemanticAnalyzer* analyzer, ASTNode* node) {
    (void)analyzer;
    (void)node;
    return create_primitive_type(TYPE_DATAFRAME);
}

Type* analyze_groupby_transform(SemanticAnalyzer* analyzer, ASTNode* node) {
    (void)analyzer;
    (void)node;
    return create_primitive_type(TYPE_DATAFRAME);
}

Type* analyze_aggregate_transform(SemanticAnalyzer* analyzer, ASTNode* node) {
    for (int i = 0; i < node->aggregate_transform.agg_arg_count; i++) {
        analyze_expression(analyzer, node->aggregate_transform.agg_args[i]);
    }
    
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
    
    if (!check_binary_op_types(analyzer, left, right, op_str, 
                                node->line, node->column)) {
        analyzer->had_error = true;
        return create_error_type();
    }
    
    return get_result_type_binary_op(left, right, op_str);
}

Type* analyze_unary_expr(SemanticAnalyzer* analyzer, ASTNode* node) {
    Type* operand = analyze_expression(analyzer, node->unary_expr.operand);
    
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
    
    // Tratamento especial para print
    if (strcmp(func_name, "print") == 0) {
        for (int i = 0; i < node->call_expr.arg_count; i++) {
            analyze_expression(analyzer, node->call_expr.arguments[i]);
        }
        return create_primitive_type(TYPE_VOID);
    }
    
    Symbol* func_symbol = lookup_symbol(analyzer->symbol_table, func_name);
    
    if (!func_symbol) {
        symbol_table_error(analyzer->symbol_table, node->line, node->column,
            "Função '%s' não foi declarada", func_name);
        analyzer->had_error = true;
        return create_error_type();
    }
    
    if (func_symbol->kind == SYMBOL_TYPE) {
        // É um construtor - deve retornar o tipo customizado
        return create_custom_type(func_name);
    }
    
    if (func_symbol->kind != SYMBOL_FUNCTION) {
        symbol_table_error(analyzer->symbol_table, node->line, node->column,
            "'%s' não é uma função", func_name);
        analyzer->had_error = true;
        return create_error_type();
    }
    
    // Verifica tipos dos argumentos
    for (int i = 0; i < node->call_expr.arg_count; i++) {
        analyze_expression(analyzer, node->call_expr.arguments[i]);
    }
    
    mark_symbol_used(analyzer->symbol_table, func_name);
    
    if (func_symbol->type && func_symbol->type->return_type) {
        return clone_type(func_symbol->type->return_type);
    }
    return create_primitive_type(TYPE_VOID);
}

// Analisa uma lambda, opcionalmente usando expectativas de tipos para parâmetros e retorno
static Type* analyze_lambda_with_expectations(SemanticAnalyzer* analyzer, ASTNode* node,
                                              Type** expected_params, int expected_param_count,
                                              Type* expected_return) {
    if (expected_params && expected_param_count > 0 &&
        node->lambda_expr.lambda_param_count != expected_param_count) {
        symbol_table_error(analyzer->symbol_table, node->line, node->column,
            "Lambda espera %d parâmetro(s) pelo contexto, mas recebeu %d",
            expected_param_count, node->lambda_expr.lambda_param_count);
        analyzer->had_error = true;
    }
    
    enter_scope(analyzer->symbol_table);
    
    Type** param_types = malloc(node->lambda_expr.lambda_param_count * sizeof(Type*));
    for (int i = 0; i < node->lambda_expr.lambda_param_count; i++) {
        ASTNode* param = node->lambda_expr.lambda_params[i];
        
        Type* param_type;
        if (param->param.param_type) {
            param_type = ast_type_to_type(analyzer, param->param.param_type);
        } else if (expected_params && i < expected_param_count && expected_params[i]) {
            param_type = clone_type(expected_params[i]);
        } else {
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
    
    Type* body_type = analyze_expression(analyzer, node->lambda_expr.lambda_body);
    
    if (expected_return && body_type &&
        body_type->kind != TYPE_ERROR &&
        !types_compatible(expected_return, body_type)) {
        symbol_table_error(analyzer->symbol_table, node->line, node->column,
            "Tipo de retorno da lambda (%s) incompatível com o esperado (%s)",
            type_to_string(body_type), type_to_string(expected_return));
        analyzer->had_error = true;
    }
    
    exit_scope(analyzer->symbol_table);
    
    Type* lambda_type = create_function_type(param_types, 
                                             node->lambda_expr.lambda_param_count,
                                             body_type);
    free(param_types);
    
    return lambda_type;
}

Type* analyze_lambda_expr(SemanticAnalyzer* analyzer, ASTNode* node) {
    return analyze_lambda_with_expectations(analyzer, node, NULL, 0, NULL);
}

Type* analyze_pipeline_expr(SemanticAnalyzer* analyzer, ASTNode* node) {
    if (node->pipeline_expr.stage_count == 0) {
        return create_error_type();
    }
    
    Type* current_type = analyze_expression(analyzer, node->pipeline_expr.stages[0]);
    
    for (int i = 1; i < node->pipeline_expr.stage_count; i++) {
        ASTNode* stage = node->pipeline_expr.stages[i];
        
        if (stage->type == AST_FILTER_TRANSFORM) {
            current_type = analyze_filter_transform(analyzer, stage, current_type);
        }
        else if (stage->type == AST_MAP_TRANSFORM) {
            current_type = analyze_map_transform(analyzer, stage, current_type);
        }
        else if (stage->type == AST_REDUCE_TRANSFORM) {
            current_type = analyze_reduce_transform(analyzer, stage, current_type);
        }
        else if (stage->type == AST_AGGREGATE_TRANSFORM) {
            current_type = analyze_expression(analyzer, stage);
        }
        else {
            current_type = analyze_expression(analyzer, stage);
        }
    }
    
    return current_type;
}

Type* analyze_literal(SemanticAnalyzer* analyzer, ASTNode* node) {
    (void)analyzer;
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
    
    if (!symbol) {
        symbol_table_error(analyzer->symbol_table, node->line, node->column,
            "Identificador '%s' não declarado", node->identifier.id_name);
        analyzer->had_error = true;
        return create_error_type();
    }
    
    if (!symbol->initialized && symbol->kind == SYMBOL_VARIABLE) {
        analyzer->warning_count++;
        printf("Aviso [linha %d]: Variável '%s' pode estar sendo usada antes de ser inicializada\n",
               node->line, node->identifier.id_name);
    }
    
    mark_symbol_used(analyzer->symbol_table, node->identifier.id_name);
    
    return clone_type(symbol->type);
}

Type* analyze_array_literal(SemanticAnalyzer* analyzer, ASTNode* node) {
    if (node->array_literal.element_count == 0) {
        Type* elem_type = fresh_type_var(analyzer->inference_ctx->var_gen);
        return create_array_type(elem_type);
    }
    
    Type* elem_type = analyze_expression(analyzer, node->array_literal.elements[0]);
    
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

Type* analyze_member_expr(SemanticAnalyzer* analyzer, ASTNode* node) {
    Type* object_type = analyze_expression(analyzer, node->member_expr.object);
    
    if (object_type->kind == TYPE_CUSTOM) {
        // Trate Row (DataFrame genérica) como dinâmica: qualquer campo é permitido
        if (strcmp(object_type->custom_name, "Row") == 0) {
            // Heurística simples de tipos para colunas comuns
            const char* field = node->member_expr.member;
            if (strcmp(field, "idade") == 0 || strcmp(field, "id") == 0) {
                return create_primitive_type(TYPE_INT);
            }
            if (strcmp(field, "salario") == 0 || strcmp(field, "valor") == 0) {
                return create_primitive_type(TYPE_FLOAT);
            }
            // Default: string
            return create_primitive_type(TYPE_STRING);
        }
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
    
    // Para outros tipos, retorna tipo genérico
    return fresh_type_var(analyzer->inference_ctx->var_gen);
}

Type* analyze_index_expr(SemanticAnalyzer* analyzer, ASTNode* node) {
    Type* object_type = analyze_expression(analyzer, node->index_expr.object);
    Type* index_type = analyze_expression(analyzer, node->index_expr.index);
    
    if (index_type->kind != TYPE_INT && index_type->kind != TYPE_ERROR) {
        symbol_table_error(analyzer->symbol_table, node->line, node->column,
            "Índice deve ser inteiro, mas encontrado %s", type_to_string(index_type));
        analyzer->had_error = true;
    }
    
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
    if (!is_lvalue(node->assign_expr.target)) {
        symbol_table_error(analyzer->symbol_table, node->line, node->column,
            "Lado esquerdo da atribuição não é um lvalue válido");
        analyzer->had_error = true;
        return create_error_type();
    }
    
    Type* target_type = analyze_expression(analyzer, node->assign_expr.target);
    Type* value_type = analyze_expression(analyzer, node->assign_expr.value);
    
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
    
    return create_array_type(create_primitive_type(TYPE_INT));
}

Type* analyze_load_expr(SemanticAnalyzer* analyzer, ASTNode* node) {
    (void)analyzer;
    (void)node;
    return create_primitive_type(TYPE_DATAFRAME);
}

Type* analyze_save_expr(SemanticAnalyzer* analyzer, ASTNode* node) {
    analyze_expression(analyzer, node->save_expr.data);
    return create_primitive_type(TYPE_VOID);
}

// ==================== VERIFICAÇÕES ESPECÍFICAS ====================

bool check_all_paths_return(SemanticAnalyzer* analyzer, ASTNode* block) {
    if (!block || block->type != AST_BLOCK) return false;
    
    for (int i = 0; i < block->block.stmt_count; i++) {
        ASTNode* stmt = block->block.statements[i];
        
        if (stmt->type == AST_RETURN_STMT) {
            return true;
        }
        
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
        
        return true;
    }
    
    // Operadores de igualdade
    if (strcmp(op, "==") == 0 || strcmp(op, "!=") == 0) {
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
            return create_array_type(create_primitive_type(TYPE_INT));
        case TOKEN_SERIES_TYPE:
            return create_array_type(create_primitive_type(TYPE_INT));
            
        case TOKEN_LBRACKET: {
            Type* elem_type = ast_type_to_type(analyzer, type_node->type_node.inner_type);
            return create_array_type(elem_type);
        }
        
        case TOKEN_LPAREN: {
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

// ==================== RELATÓRIOS ====================

void print_semantic_analysis_report(SemanticAnalyzer* analyzer) {
    printf("\n╔═══════════════════════════════════════════════════════════╗\n");
    printf("║           RELATÓRIO DE ANÁLISE SEMÂNTICA                  ║\n");
    printf("╚═══════════════════════════════════════════════════════════╝\n\n");
    
    if (analyzer->symbol_table->error_count > 0) {
        print_symbol_table_errors(analyzer->symbol_table);
    }
    
    if (analyzer->inference_ctx->error_count > 0) {
        print_inference_errors(analyzer->inference_ctx);
    }
    
    printf("Estatísticas:\n");
    printf("────────────────────────────────────────────────────────────\n");
    printf("  Erros: %d\n", analyzer->symbol_table->error_count + 
                               analyzer->inference_ctx->error_count);
    printf("  Avisos: %d\n", analyzer->warning_count);
    
    if (analyzer->had_error) {
        printf("\nAnálise semântica concluída com erros\n");
    } else {
        printf("\nAnálise semântica concluída com sucesso\n");
        printf("\nTabela de Símbolos Final:\n");
        printf("────────────────────────────────────────────────────────────\n");
        print_symbol_table(analyzer->symbol_table);
    }
}
