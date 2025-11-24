/*
 * DataLang - Gerador de Código LLVM IR - COMPLETO E CORRIGIDO (v9)
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include "codegen.h"
#include "symbol_table.h" 

// ==================== FUNÇÕES AUXILIARES INTERNAS ====================

static char* gen_temp(CodeGenContext* ctx) {
    char* temp = malloc(32);
    snprintf(temp, 32, "%%t%d", ctx->temp_counter++);
    return temp;
}

static char* gen_label(CodeGenContext* ctx) {
    char* label = malloc(32);
    snprintf(label, 32, "L%d", ctx->label_counter++);
    return label;
}

static void emit(CodeGenContext* ctx, const char* format, ...) {
    va_list args;
    va_start(args, format);
    vfprintf(ctx->output, format, args);
    va_end(args);
}

static char* register_string_literal(CodeGenContext* ctx, const char* value) {
    for (int i = 0; i < ctx->string_literals.count; i++) {
        if (strcmp(ctx->string_literals.values[i], value) == 0) {
            return ctx->string_literals.llvm_names[i];
        }
    }
    
    if (ctx->string_literals.count >= ctx->string_literals.capacity) {
        ctx->string_literals.capacity = (ctx->string_literals.capacity == 0) ? 16 : ctx->string_literals.capacity * 2;
        ctx->string_literals.values = realloc(ctx->string_literals.values,
            ctx->string_literals.capacity * sizeof(char*));
        ctx->string_literals.llvm_names = realloc(ctx->string_literals.llvm_names,
            ctx->string_literals.capacity * sizeof(char*));
    }
    
    char* llvm_name = malloc(64);
    snprintf(llvm_name, 64, "@.str.%d", ctx->string_counter++);
    
    ctx->string_literals.values[ctx->string_literals.count] = strdup(value);
    ctx->string_literals.llvm_names[ctx->string_literals.count] = llvm_name;
    ctx->string_literals.count++;
    
    return llvm_name;
}

static void add_var_mapping(CodeGenContext* ctx, const char* name, const char* llvm_name) {
    if (ctx->var_map.count >= ctx->var_map.capacity) {
        ctx->var_map.capacity = (ctx->var_map.capacity == 0) ? 16 : ctx->var_map.capacity * 2;
        ctx->var_map.names = realloc(ctx->var_map.names,
            ctx->var_map.capacity * sizeof(char*));
        ctx->var_map.llvm_names = realloc(ctx->var_map.llvm_names,
            ctx->var_map.capacity * sizeof(char*));
    }
    
    ctx->var_map.names[ctx->var_map.count] = strdup(name);
    ctx->var_map.llvm_names[ctx->var_map.count] = strdup(llvm_name);
    ctx->var_map.count++;
}

static char* get_var_llvm_name(CodeGenContext* ctx, const char* name) {
    for (int i = ctx->var_map.count - 1; i >= 0; i--) {
        if (strcmp(ctx->var_map.names[i], name) == 0) {
            return ctx->var_map.llvm_names[i];
        }
    }
    return NULL;
}

// ==================== CONVERSÃO DE TIPOS ====================

const char* type_to_llvm(Type* type) {
    static char buffers[16][256];
    static int idx = 0;
    
    idx = (idx + 1) % 16;
    char* buffer = buffers[idx];

    if (!type) return "void";
    
    switch (type->kind) {
        case TYPE_INT: return "i64";
        case TYPE_FLOAT: return "double";
        case TYPE_STRING: return "i8*";
        case TYPE_BOOL: return "i1";
        case TYPE_VOID: return "void";
        case TYPE_ARRAY: {
            snprintf(buffer, 256, "{i64, %s*}", type_to_llvm(type->element_type));
            return buffer;
        }
        default: return "i8*";
    }
}

// ==================== DECLARAÇÕES FORWARD ====================

static char* generate_expr(CodeGenContext* ctx, ASTNode* node);
static void generate_stmt(CodeGenContext* ctx, ASTNode* node);
static void generate_function(CodeGenContext* ctx, ASTNode* node);
static void generate_let_decl(CodeGenContext* ctx, ASTNode* node);

// ==================== GERAÇÃO DE EXPRESSÕES ====================

static char* generate_literal(CodeGenContext* ctx, ASTNode* node) {
    char* result = gen_temp(ctx);
    switch (node->literal.literal_type) {
        case TOKEN_INTEGER: {
            char* val = malloc(32);
            snprintf(val, 32, "%lld", node->literal.int_value);
            return val;
        }
        case TOKEN_FLOAT: {
            char* val = malloc(32);
            snprintf(val, 32, "%f", node->literal.float_value);
            return val;
        }
        case TOKEN_STRING: {
            char* str_global = register_string_literal(ctx, node->literal.string_value);
            int len = strlen(node->literal.string_value);
            emit(ctx, "  %s = getelementptr [%d x i8], [%d x i8]* %s, i32 0, i32 0\n",
                result, len + 1, len + 1, str_global);
            return result;
        }
        case TOKEN_BOOL_TYPE: {
            return node->literal.bool_value ? "true" : "false";
        }
        default: return "0";
    }
}

static char* generate_identifier(CodeGenContext* ctx, ASTNode* node) {
    char* var_ptr = get_var_llvm_name(ctx, node->identifier.id_name);
    if (!var_ptr) {
        fprintf(stderr, "Aviso: Variável '%s' não encontrada. Usando 0.\n", node->identifier.id_name);
        return "0"; 
    }
    
    Symbol* symbol = lookup_symbol(ctx->analyzer->symbol_table, node->identifier.id_name);
    const char* type_str = type_to_llvm(symbol ? symbol->type : create_primitive_type(TYPE_INT));
    
    char* result = gen_temp(ctx);
    emit(ctx, "  %s = load %s, %s* %s\n", result, type_str, type_str, var_ptr);
    return result;
}

static char* generate_binary_expr(CodeGenContext* ctx, ASTNode* node) {
    char* left = generate_expr(ctx, node->binary_expr.left);
    char* right = generate_expr(ctx, node->binary_expr.right);
    char* result = gen_temp(ctx);
    
    Type* left_type = analyze_expression(ctx->analyzer, node->binary_expr.left);
    bool is_float = (left_type && left_type->kind == TYPE_FLOAT);
    
    switch (node->binary_expr.op) {
        case BINOP_ADD:
            emit(ctx, is_float ? "  %s = fadd double %s, %s\n" : "  %s = add i64 %s, %s\n", result, left, right); break;
        case BINOP_SUB:
            emit(ctx, is_float ? "  %s = fsub double %s, %s\n" : "  %s = sub i64 %s, %s\n", result, left, right); break;
        case BINOP_MUL:
            emit(ctx, is_float ? "  %s = fmul double %s, %s\n" : "  %s = mul i64 %s, %s\n", result, left, right); break;
        case BINOP_DIV:
            emit(ctx, is_float ? "  %s = fdiv double %s, %s\n" : "  %s = sdiv i64 %s, %s\n", result, left, right); break;
        case BINOP_MOD:
            emit(ctx, "  %s = srem i64 %s, %s\n", result, left, right); break;
        case BINOP_EQ:
            emit(ctx, is_float ? "  %s = fcmp oeq double %s, %s\n" : "  %s = icmp eq i64 %s, %s\n", result, left, right); break;
        case BINOP_NEQ:
            emit(ctx, is_float ? "  %s = fcmp one double %s, %s\n" : "  %s = icmp ne i64 %s, %s\n", result, left, right); break;
        case BINOP_LT:
            emit(ctx, is_float ? "  %s = fcmp olt double %s, %s\n" : "  %s = icmp slt i64 %s, %s\n", result, left, right); break;
        case BINOP_LTE:
            emit(ctx, is_float ? "  %s = fcmp ole double %s, %s\n" : "  %s = icmp sle i64 %s, %s\n", result, left, right); break;
        case BINOP_GT:
            emit(ctx, is_float ? "  %s = fcmp ogt double %s, %s\n" : "  %s = icmp sgt i64 %s, %s\n", result, left, right); break;
        case BINOP_GTE:
            emit(ctx, is_float ? "  %s = fcmp oge double %s, %s\n" : "  %s = icmp sge i64 %s, %s\n", result, left, right); break;
        case BINOP_AND:
            emit(ctx, "  %s = and i1 %s, %s\n", result, left, right); break;
        case BINOP_OR:
            emit(ctx, "  %s = or i1 %s, %s\n", result, left, right); break;
        default: return "0";
    }
    return result;
}

static char* generate_call_expr(CodeGenContext* ctx, ASTNode* node) {
    char* func_name = node->call_expr.callee->identifier.id_name;
    
    // Tratamento para print() (MANTIDO IGUAL)
    if (strcmp(func_name, "print") == 0) {
        if (node->call_expr.arg_count != 1) return "null";
        
        char* arg = generate_expr(ctx, node->call_expr.arguments[0]);
        Type* arg_type = analyze_expression(ctx->analyzer, node->call_expr.arguments[0]);
        
        if (node->call_expr.arguments[0]->type == AST_LITERAL && 
            node->call_expr.arguments[0]->literal.literal_type == TOKEN_STRING) {
            emit(ctx, "  call void @print_string(i8* %s)\n", arg);
        }
        else if (arg_type->kind == TYPE_INT) emit(ctx, "  call void @print_int(i64 %s)\n", arg);
        else if (arg_type->kind == TYPE_FLOAT) emit(ctx, "  call void @print_float(double %s)\n", arg);
        else if (arg_type->kind == TYPE_STRING) emit(ctx, "  call void @print_string(i8* %s)\n", arg);
        else if (arg_type->kind == TYPE_BOOL) emit(ctx, "  call void @print_bool(i1 %s)\n", arg);
        else {
            emit(ctx, "  call void @print_int(i64 %s)\n", arg);
        }
        return "null";
    }
    
    Symbol* func_symbol = lookup_symbol(ctx->analyzer->symbol_table, func_name);
    int arg_count = node->call_expr.arg_count;
    
    char** arg_vals = malloc(arg_count * sizeof(char*));
    for (int i = 0; i < arg_count; i++) {
        arg_vals[i] = generate_expr(ctx, node->call_expr.arguments[i]);
    }

    // Determina o tipo de retorno
    const char* ret_type = type_to_llvm(func_symbol ? func_symbol->type->return_type : create_primitive_type(TYPE_VOID));
    
    bool is_void = (strcmp(ret_type, "void") == 0);
    char* result = NULL;

    if (!is_void) {
        // Se NÃO é void, precisamos de uma variável temporária para guardar o resultado
        result = gen_temp(ctx);
    } else {
        // Se É void, retornamos "0" ou "null" apenas para não quebrar o fluxo, 
        // mas não geramos %temp = ...
        result = "0"; 
    }
    
    const char* llvm_func_name = func_name;
    if (strcmp(func_name, "main") == 0) {
        llvm_func_name = "user_main";
    }
    
    // Gera a instrução call correta
    if (is_void) {
        emit(ctx, "  call %s @%s(", ret_type, llvm_func_name);
    } else {
        emit(ctx, "  %s = call %s @%s(", result, ret_type, llvm_func_name);
    }
    
    for (int i = 0; i < arg_count; i++) {
        if (i > 0) emit(ctx, ", ");
        
        const char* param_type;
        if (func_symbol && i < func_symbol->param_count) {
            param_type = type_to_llvm(func_symbol->param_types[i]);
        } else {
            Type* arg_t = analyze_expression(ctx->analyzer, node->call_expr.arguments[i]);
            param_type = type_to_llvm(arg_t);
        }
        
        emit(ctx, "%s %s", param_type, arg_vals[i]);
    }
    emit(ctx, ")\n");
    
    free(arg_vals);
    return result;
}

static char* generate_aggregate_transform(CodeGenContext* ctx, ASTNode* node) {
    const char* func_name = NULL;
    const char* ret_type = "i64"; 
    
    switch (node->aggregate_transform.agg_type) {
        case AGG_SUM: func_name = "sum"; break;
        case AGG_MIN: func_name = "min"; break;
        case AGG_MAX: func_name = "max"; break;
        case AGG_COUNT: func_name = "count"; break;
        case AGG_MEAN: func_name = "mean"; ret_type = "double"; break;
        default: return "0";
    }
    
    int count = node->aggregate_transform.agg_arg_count;
    char** arg_vals = malloc(count * sizeof(char*));
    
    for (int i = 0; i < count; i++) {
        arg_vals[i] = generate_expr(ctx, node->aggregate_transform.agg_args[i]);
    }
    
    char* result = gen_temp(ctx);
    emit(ctx, "  %s = call %s @%s(", result, ret_type, func_name);
    
    for (int i = 0; i < count; i++) {
        if (i > 0) emit(ctx, ", ");
        Type* t = analyze_expression(ctx->analyzer, node->aggregate_transform.agg_args[i]);
        emit(ctx, "%s %s", type_to_llvm(t), arg_vals[i]);
    }
    emit(ctx, ")\n");
    
    free(arg_vals);
    return result;
}

static char* generate_array_literal(CodeGenContext* ctx, ASTNode* node) {
    int count = node->array_literal.element_count;
    Type* elem_type;
    if (count > 0) {
        elem_type = analyze_expression(ctx->analyzer, node->array_literal.elements[0]);
    } else {
        elem_type = create_primitive_type(TYPE_INT); 
    }
    
    char type_str[256];
    strncpy(type_str, type_to_llvm(elem_type), 255);
    
    char* array_ptr = gen_temp(ctx);
    int size_bytes = (count == 0) ? 8 : count * 8; 
    emit(ctx, "  %s = call i8* @malloc(i64 %d)\n", array_ptr, size_bytes);
    
    char* typed_ptr = gen_temp(ctx);
    emit(ctx, "  %s = bitcast i8* %s to %s*\n", typed_ptr, array_ptr, type_str);
    
    for (int i = 0; i < count; i++) {
        char* val = generate_expr(ctx, node->array_literal.elements[i]);
        char* elem_ptr = gen_temp(ctx);
        emit(ctx, "  %s = getelementptr %s, %s* %s, i64 %d\n", 
             elem_ptr, type_str, type_str, typed_ptr, i);
        emit(ctx, "  store %s %s, %s* %s\n", type_str, val, type_str, elem_ptr);
    }
    
    char* struct_ptr = gen_temp(ctx);
    emit(ctx, "  %s = alloca {i64, %s*}\n", struct_ptr, type_str);
    
    char* size_ptr = gen_temp(ctx);
    emit(ctx, "  %s = getelementptr {i64, %s*}, {i64, %s*}* %s, i32 0, i32 0\n",
         size_ptr, type_str, type_str, struct_ptr);
    emit(ctx, "  store i64 %d, i64* %s\n", count, size_ptr);
    
    char* data_ptr = gen_temp(ctx);
    emit(ctx, "  %s = getelementptr {i64, %s*}, {i64, %s*}* %s, i32 0, i32 1\n",
         data_ptr, type_str, type_str, struct_ptr);
    emit(ctx, "  store %s* %s, %s** %s\n", type_str, typed_ptr, type_str, data_ptr);
    
    char* final_val = gen_temp(ctx);
    emit(ctx, "  %s = load {i64, %s*}, {i64, %s*}* %s\n", 
         final_val, type_str, type_str, struct_ptr);
         
    return final_val;
}

static char* generate_range_expr(CodeGenContext* ctx, ASTNode* node) {
    char* struct_ptr = gen_temp(ctx);
    emit(ctx, "  %s = alloca {i64, i64*}\n", struct_ptr);
    emit(ctx, "  %%range_data = call i8* @malloc(i64 80)\n");
    emit(ctx, "  %%range_typed = bitcast i8* %%range_data to i64*\n");
    char* data_ptr = gen_temp(ctx);
    emit(ctx, "  %s = getelementptr {i64, i64*}, {i64, i64*}* %s, i32 0, i32 1\n", data_ptr, struct_ptr);
    emit(ctx, "  store i64* %%range_typed, i64** %s\n", data_ptr);
    char* final_val = gen_temp(ctx);
    emit(ctx, "  %s = load {i64, i64*}, {i64, i64*}* %s\n", final_val, struct_ptr);
    return final_val;
}

static char* generate_expr(CodeGenContext* ctx, ASTNode* node) {
    if (!node) return "0";
    
    switch (node->type) {
        case AST_LITERAL: return generate_literal(ctx, node);
        case AST_IDENTIFIER: return generate_identifier(ctx, node);
        case AST_BINARY_EXPR: return generate_binary_expr(ctx, node);
        case AST_ARRAY_LITERAL: return generate_array_literal(ctx, node);
        case AST_CALL_EXPR: return generate_call_expr(ctx, node);
        case AST_RANGE_EXPR: return generate_range_expr(ctx, node);
        case AST_AGGREGATE_TRANSFORM: return generate_aggregate_transform(ctx, node);
        default: return "0";
    }
}

// ==================== STATEMENTS ====================

static void generate_let_decl(CodeGenContext* ctx, ASTNode* node) {
    Type* var_type = NULL;
    if (node->let_decl.type_annotation) {
        var_type = ast_type_to_type(ctx->analyzer, node->let_decl.type_annotation);
    } else if (node->let_decl.initializer) {
        var_type = analyze_expression(ctx->analyzer, node->let_decl.initializer);
    } else {
        var_type = create_primitive_type(TYPE_INT);
    }
    
    if (var_type->kind == TYPE_ERROR) var_type = create_primitive_type(TYPE_INT);

    if (!is_symbol_in_current_scope(ctx->analyzer->symbol_table, node->let_decl.name)) {
        declare_symbol(ctx->analyzer->symbol_table, node->let_decl.name, SYMBOL_VARIABLE, var_type, 0, 0);
    } else {
        Symbol* existing = lookup_symbol(ctx->analyzer->symbol_table, node->let_decl.name);
        if (existing) existing->type = var_type;
    }
    
    char type_str[256];
    strncpy(type_str, type_to_llvm(var_type), 255);
    
    char* var_ptr = gen_temp(ctx);
    emit(ctx, "  %s = alloca %s\n", var_ptr, type_str);
    add_var_mapping(ctx, node->let_decl.name, var_ptr);
    
    if (node->let_decl.initializer) {
        char* init_value = generate_expr(ctx, node->let_decl.initializer);
        if (init_value) {
            emit(ctx, "  store %s %s, %s* %s\n", type_str, init_value, type_str, var_ptr);
        }
    }
}

static void generate_if_stmt(CodeGenContext* ctx, ASTNode* node) {
    char* cond = generate_expr(ctx, node->if_stmt.condition);
    char* then_label = gen_label(ctx);
    char* else_label = gen_label(ctx);
    char* merge_label = gen_label(ctx);
    
    emit(ctx, "  br i1 %s, label %%%s, label %%%s\n", cond, then_label, node->if_stmt.else_block ? else_label : merge_label);
    emit(ctx, "\n%s:\n", then_label);
    generate_stmt(ctx, node->if_stmt.then_block);
    emit(ctx, "  br label %%%s\n", merge_label);
    
    if (node->if_stmt.else_block) {
        emit(ctx, "\n%s:\n", else_label);
        generate_stmt(ctx, node->if_stmt.else_block);
        emit(ctx, "  br label %%%s\n", merge_label);
    }
    emit(ctx, "\n%s:\n", merge_label);
}

static void generate_for_stmt(CodeGenContext* ctx, ASTNode* node) {
    enter_scope(ctx->analyzer->symbol_table);

    char* array_struct = generate_expr(ctx, node->for_stmt.iterable);
    Type* iterable_type = analyze_expression(ctx->analyzer, node->for_stmt.iterable);
    Type* elem_type = (iterable_type && iterable_type->kind == TYPE_ARRAY) ? iterable_type->element_type : create_primitive_type(TYPE_INT);
    
    declare_symbol(ctx->analyzer->symbol_table, node->for_stmt.iterator, SYMBOL_VARIABLE, elem_type, 0, 0);

    char* size = gen_temp(ctx);
    emit(ctx, "  %s = extractvalue {i64, i64*} %s, 0\n", size, array_struct);
    
    char* i_ptr = gen_temp(ctx);
    emit(ctx, "  %s = alloca i64\n", i_ptr);
    emit(ctx, "  store i64 0, i64* %s\n", i_ptr);
    
    char* loop_cond = gen_label(ctx);
    char* loop_body = gen_label(ctx);
    char* loop_end = gen_label(ctx);
    
    emit(ctx, "  br label %%%s\n", loop_cond);
    emit(ctx, "\n%s:\n", loop_cond);
    
    char* i_val = gen_temp(ctx);
    emit(ctx, "  %s = load i64, i64* %s\n", i_val, i_ptr);
    
    char* cmp = gen_temp(ctx);
    emit(ctx, "  %s = icmp slt i64 %s, %s\n", cmp, i_val, size);
    emit(ctx, "  br i1 %s, label %%%s, label %%%s\n", cmp, loop_body, loop_end);
    
    emit(ctx, "\n%s:\n", loop_body);
    generate_stmt(ctx, node->for_stmt.body);
    
    char* i_next = gen_temp(ctx);
    emit(ctx, "  %s = add i64 %s, 1\n", i_next, i_val);
    emit(ctx, "  store i64 %s, i64* %s\n", i_next, i_ptr);
    emit(ctx, "  br label %%%s\n", loop_cond);
    
    emit(ctx, "\n%s:\n", loop_end);
    exit_scope(ctx->analyzer->symbol_table);
}

static void generate_block(CodeGenContext* ctx, ASTNode* node) {
    for (int i = 0; i < node->block.stmt_count; i++) {
        generate_stmt(ctx, node->block.statements[i]);
    }
}

static void generate_stmt(CodeGenContext* ctx, ASTNode* node) {
    if (!node) return;
    switch (node->type) {
        case AST_LET_DECL: generate_let_decl(ctx, node); break;
        case AST_IF_STMT: generate_if_stmt(ctx, node); break;
        case AST_FOR_STMT: generate_for_stmt(ctx, node); break;
        
        case AST_PRINT_STMT: {
             // 1. Gera o código da expressão (retorna o nome da variável/registrador LLVM)
             char* val = generate_expr(ctx, node->print_stmt.expression);
             
             // 2. Descobre o TIPO SEMÂNTICO real da expressão
             Type* type = analyze_expression(ctx->analyzer, node->print_stmt.expression);
             
             // 3. Escolhe a função de print correta baseada no tipo
             if (type->kind == TYPE_STRING) {
                 emit(ctx, "  call void @print_string(i8* %s)\n", val);
             } 
             else if (type->kind == TYPE_FLOAT) {
                 emit(ctx, "  call void @print_float(double %s)\n", val);
             } 
             else if (type->kind == TYPE_BOOL) {
                 emit(ctx, "  call void @print_bool(i1 %s)\n", val);
             } 
             else {
                 emit(ctx, "  call void @print_int(i64 %s)\n", val);
             }
             break;
        }

        case AST_BLOCK: generate_block(ctx, node); break;
        
        case AST_RETURN_STMT: {
            if (node->return_stmt.value) {
                char* val = generate_expr(ctx, node->return_stmt.value);
                Type* type = analyze_expression(ctx->analyzer, node->return_stmt.value);
                emit(ctx, "  ret %s %s\n", type_to_llvm(type), val);
            } else {
                emit(ctx, "  ret void\n");
            }
            break;
        }
        case AST_EXPR_STMT: generate_expr(ctx, node->expr_stmt.expression); break;
        default: break;
    }
}

static void generate_function(CodeGenContext* ctx, ASTNode* node) {
    ctx->in_function = true;
    ctx->current_function = node->fn_decl.name;
    
    enter_scope(ctx->analyzer->symbol_table);
    
    const char* llvm_func_name = node->fn_decl.name;
    if (strcmp(node->fn_decl.name, "main") == 0) {
        llvm_func_name = "user_main";
    }
    
    Type* ret_type = node->fn_decl.return_type ? 
                     ast_type_to_type(ctx->analyzer, node->fn_decl.return_type) : 
                     create_primitive_type(TYPE_VOID);
    
    char ret_type_str[64];
    strncpy(ret_type_str, type_to_llvm(ret_type), 63);

    emit(ctx, "\ndefine %s @%s(", ret_type_str, llvm_func_name);
    
    for (int i=0; i < node->fn_decl.param_count; i++) {
        if (i>0) emit(ctx, ", ");
        ASTNode* param = node->fn_decl.params[i];
        Type* param_type = ast_type_to_type(ctx->analyzer, param->param.param_type);
        emit(ctx, "%s %%p%d", type_to_llvm(param_type), i);
    }
    emit(ctx, ") {\nentry:\n");
    
    for (int i = 0; i < node->fn_decl.param_count; i++) {
        ASTNode* param = node->fn_decl.params[i];
        char* param_name = param->param.param_name;
        Type* param_type = ast_type_to_type(ctx->analyzer, param->param.param_type);
        
        declare_symbol(ctx->analyzer->symbol_table, param_name, SYMBOL_PARAMETER, param_type, 0, 0);
        
        char param_type_str[64];
        strncpy(param_type_str, type_to_llvm(param_type), 63);
        
        char* var_ptr = gen_temp(ctx);
        emit(ctx, "  %s = alloca %s\n", var_ptr, param_type_str);
        emit(ctx, "  store %s %%p%d, %s* %s\n", param_type_str, i, param_type_str, var_ptr);
        add_var_mapping(ctx, param_name, var_ptr);
    }
    
    generate_block(ctx, node->fn_decl.body);
    
    bool has_return = false;
    if (node->fn_decl.body && node->fn_decl.body->type == AST_BLOCK) {
        int count = node->fn_decl.body->block.stmt_count;
        if (count > 0 && node->fn_decl.body->block.statements[count-1]->type == AST_RETURN_STMT) {
            has_return = true;
        }
    }
    
    if (!has_return) {
        if (ret_type->kind == TYPE_VOID) emit(ctx, "  ret void\n");
        else if (ret_type->kind == TYPE_FLOAT) emit(ctx, "  ret double 0.0\n");
        else emit(ctx, "  ret %s 0\n", ret_type_str);
    }
    emit(ctx, "}\n");
    
    exit_scope(ctx->analyzer->symbol_table);
    ctx->in_function = false;
}

static void generate_main_function(CodeGenContext* ctx, ASTNode* program) {
    emit(ctx, "define i32 @main() {\nentry:\n");
    
    for (int i = 0; i < program->program.decl_count; i++) {
        ASTNode* decl = program->program.declarations[i];
        if (decl->type == AST_LET_DECL) generate_let_decl(ctx, decl);
        else if (decl->type == AST_PRINT_STMT) generate_stmt(ctx, decl);
        else if (decl->type == AST_FOR_STMT) generate_for_stmt(ctx, decl);
        else if (decl->type == AST_IF_STMT) generate_if_stmt(ctx, decl);
        else if (decl->type == AST_EXPR_STMT) generate_stmt(ctx, decl);
    }
    
    emit(ctx, "  ret i32 0\n}\n");
}

void emit_runtime_functions(CodeGenContext* ctx) {
    emit(ctx, "; ==================== RUNTIME FUNCTIONS ====================\n\n");
    emit(ctx, "declare i32 @printf(i8*, ...)\n");
    emit(ctx, "declare i8* @malloc(i64)\n");
    emit(ctx, "declare void @free(i8*)\n\n");

    emit(ctx, "@.fmt.int = private constant [6 x i8] c\"%%lld\\0A\\00\"\n");
    emit(ctx, "@.fmt.float = private constant [4 x i8] c\"%%f\\0A\\00\"\n");
    emit(ctx, "@.fmt.str = private constant [4 x i8] c\"%%s\\0A\\00\"\n");
    emit(ctx, "@.fmt.true = private constant [6 x i8] c\"true\\0A\\00\"\n");
    emit(ctx, "@.fmt.false = private constant [7 x i8] c\"false\\0A\\00\"\n\n");
    
    emit(ctx, "define void @print_int(i64 %%val) {\n");
    emit(ctx, "  %%fmt = getelementptr [6 x i8], [6 x i8]* @.fmt.int, i32 0, i32 0\n");
    emit(ctx, "  call i32 (i8*, ...) @printf(i8* %%fmt, i64 %%val)\n");
    emit(ctx, "  ret void\n");
    emit(ctx, "}\n\n");
    
    emit(ctx, "define void @print_float(double %%val) {\n");
    emit(ctx, "  %%fmt = getelementptr [4 x i8], [4 x i8]* @.fmt.float, i32 0, i32 0\n");
    emit(ctx, "  call i32 (i8*, ...) @printf(i8* %%fmt, double %%val)\n");
    emit(ctx, "  ret void\n");
    emit(ctx, "}\n\n");
    
    emit(ctx, "define void @print_string(i8* %%val) {\n");
    emit(ctx, "  %%fmt = getelementptr [4 x i8], [4 x i8]* @.fmt.str, i32 0, i32 0\n");
    emit(ctx, "  call i32 (i8*, ...) @printf(i8* %%fmt, i8* %%val)\n");
    emit(ctx, "  ret void\n");
    emit(ctx, "}\n\n");
    
    emit(ctx, "define void @print_bool(i1 %%val) {\n");
    emit(ctx, "  br i1 %%val, label %%is_true, label %%is_false\n");
    emit(ctx, "is_true:\n");
    emit(ctx, "  %%fmt_t = getelementptr [6 x i8], [6 x i8]* @.fmt.true, i32 0, i32 0\n");
    emit(ctx, "  call i32 (i8*, ...) @printf(i8* %%fmt_t)\n");
    emit(ctx, "  ret void\n");
    emit(ctx, "is_false:\n");
    emit(ctx, "  %%fmt_f = getelementptr [7 x i8], [7 x i8]* @.fmt.false, i32 0, i32 0\n");
    emit(ctx, "  call i32 (i8*, ...) @printf(i8* %%fmt_f)\n");
    emit(ctx, "  ret void\n");
    emit(ctx, "}\n\n");
    
    emit(ctx, "define i64 @sum(i64 %%a, i64 %%b) {\n");
    emit(ctx, "  %%result = add i64 %%a, %%b\n");
    emit(ctx, "  ret i64 %%result\n");
    emit(ctx, "}\n\n");
    
    emit(ctx, "define i64 @min(i64 %%a, i64 %%b) {\n");
    emit(ctx, "  %%cmp = icmp slt i64 %%a, %%b\n");
    emit(ctx, "  %%result = select i1 %%cmp, i64 %%a, i64 %%b\n");
    emit(ctx, "  ret i64 %%result\n");
    emit(ctx, "}\n\n");
    
    emit(ctx, "define i64 @max(i64 %%a, i64 %%b) {\n");
    emit(ctx, "  %%cmp = icmp sgt i64 %%a, %%b\n");
    emit(ctx, "  %%result = select i1 %%cmp, i64 %%a, i64 %%b\n");
    emit(ctx, "  ret i64 %%result\n");
    emit(ctx, "}\n\n");
    
    emit(ctx, "define double @mean(double %%a, double %%b) {\n");
    emit(ctx, "  %%sum = fadd double %%a, %%b\n");
    emit(ctx, "  %%result = fdiv double %%sum, 2.0\n");
    emit(ctx, "  ret double %%result\n");
    emit(ctx, "}\n\n");
    
    emit(ctx, "define i64 @count({i64, i64*} %%array) {\n");
    emit(ctx, "  %%size = extractvalue {i64, i64*} %%array, 0\n");
    emit(ctx, "  ret i64 %%size\n");
    emit(ctx, "}\n\n");
}

bool generate_llvm_ir(CodeGenContext* ctx, ASTNode* program) {
    if (!ctx || !program || program->type != AST_PROGRAM) return false;
    
    printf("\n[CodeGen] Iniciando geração de código LLVM IR...\n");
    
    emit(ctx, "; DataLang - LLVM IR\n\n");
    emit_runtime_functions(ctx);
    emit(ctx, "; ==================== DECLARAÇÕES ====================\n\n");
    
    for (int i = 0; i < program->program.decl_count; i++) {
        ASTNode* decl = program->program.declarations[i];
        if (decl && decl->type == AST_FN_DECL) {
            generate_function(ctx, decl);
        }
    }
    
    generate_main_function(ctx, program);
    
    if (ctx->string_literals.count > 0) {
        emit(ctx, "\n; ==================== STRING LITERALS ====================\n\n");
        for (int i = 0; i < ctx->string_literals.count; i++) {
            int len = strlen(ctx->string_literals.values[i]);
            emit(ctx, "%s = private constant [%d x i8] c\"%s\\00\"\n",
                ctx->string_literals.llvm_names[i], len + 1, ctx->string_literals.values[i]);
        }
    }
    
    return true;
}

CodeGenContext* create_codegen_context(SemanticAnalyzer* analyzer, FILE* output) {
    CodeGenContext* ctx = calloc(1, sizeof(CodeGenContext));
    ctx->output = output;
    ctx->analyzer = analyzer;
    ctx->var_map.capacity = 16;
    ctx->var_map.names = malloc(16 * sizeof(char*));
    ctx->var_map.llvm_names = malloc(16 * sizeof(char*));
    ctx->string_literals.capacity = 16;
    ctx->string_literals.values = malloc(16 * sizeof(char*));
    ctx->string_literals.llvm_names = malloc(16 * sizeof(char*));
    return ctx;
}

void free_codegen_context(CodeGenContext* ctx) {
    if (!ctx) return;
    free(ctx->var_map.names);
    free(ctx->var_map.llvm_names);
    free(ctx->string_literals.values);
    free(ctx->string_literals.llvm_names);
    free(ctx);
}