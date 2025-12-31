/*
 * DataLang - Gerador de Código LLVM IR
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include "codegen.h"

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
    if (!value) value = "";
    
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

// ==================== DATA TYPES MANAGEMENT ====================

typedef struct {
    char* name;
    char** field_names;
    char** field_types;
    int field_count;
} DataTypeInfo;

static DataTypeInfo* data_types = NULL;
static int data_type_count = 0;
static int data_type_capacity = 0;

static void register_data_type(const char* name, char** field_names, char** field_types, int field_count) {
    if (data_type_count >= data_type_capacity) {
        data_type_capacity = data_type_capacity == 0 ? 8 : data_type_capacity * 2;
        data_types = realloc(data_types, data_type_capacity * sizeof(DataTypeInfo));
    }
    
    DataTypeInfo* dt = &data_types[data_type_count++];
    dt->name = strdup(name);
    dt->field_count = field_count;
    dt->field_names = malloc(field_count * sizeof(char*));
    dt->field_types = malloc(field_count * sizeof(char*));
    
    for (int i = 0; i < field_count; i++) {
        dt->field_names[i] = strdup(field_names[i]);
        dt->field_types[i] = strdup(field_types[i]);
    }
}

static DataTypeInfo* find_data_type(const char* name) {
    for (int i = 0; i < data_type_count; i++) {
        if (strcmp(data_types[i].name, name) == 0) {
            return &data_types[i];
        }
    }
    return NULL;
}

static int get_field_index(DataTypeInfo* dt, const char* field_name) {
    for (int i = 0; i < dt->field_count; i++) {
        if (strcmp(dt->field_names[i], field_name) == 0) {
            return i;
        }
    }
    return -1;
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
        case TYPE_CUSTOM: {
            snprintf(buffer, 256, "%%struct.%s*", type->custom_name);
            return buffer;
        }
        case TYPE_DATAFRAME: return "i8*";
        default: return "i8*";
    }
}

// ==================== DECLARAÇÕES FORWARD ====================

static char* generate_expr(CodeGenContext* ctx, ASTNode* node);
static void generate_stmt(CodeGenContext* ctx, ASTNode* node);
static void generate_function(CodeGenContext* ctx, ASTNode* node);
static void generate_let_decl(CodeGenContext* ctx, ASTNode* node);
static char* generate_pipeline_expr(CodeGenContext* ctx, ASTNode* node);
static char* generate_index_expr(CodeGenContext* ctx, ASTNode* node);
static char* generate_member_expr(CodeGenContext* ctx, ASTNode* node);
static char* normalize_array_to_i64(CodeGenContext* ctx, char* array_val, Type* array_type);
static char* generate_select_from_array(CodeGenContext* ctx, ASTNode* node, char* array_val, Type* array_type);
static char* generate_groupby_from_array(CodeGenContext* ctx, ASTNode* node, char* array_val, Type* array_type);
static char* generate_dataframe_from_array(CodeGenContext* ctx, char* array_val, Type* array_type, DataTypeInfo* dt);
static void generate_global_initializers_fn(CodeGenContext* ctx, ASTNode* program);
static char* generate_reduce_transform(CodeGenContext* ctx, ASTNode* node, char* input_array, Type* array_type);

// Helpers para DataFrame pipelines
static bool extract_df_filter_info(ASTNode* lambda, char** column, int* op, double* threshold) {
    if (!lambda || lambda->type != AST_LAMBDA_EXPR) return false;
    ASTNode* body = lambda->lambda_expr.lambda_body;
    if (!body || body->type != AST_BINARY_EXPR) return false;
    ASTNode* left = body->binary_expr.left;
    ASTNode* right = body->binary_expr.right;
    if (!left || !right) return false;
    ASTNode* member = NULL;
    ASTNode* literal = NULL;
    bool member_on_left = false;
    if (left->type == AST_MEMBER_EXPR && right->type == AST_LITERAL) {
        member = left; literal = right; member_on_left = true;
    } else if (right->type == AST_MEMBER_EXPR && left->type == AST_LITERAL) {
        member = right; literal = left; member_on_left = false;
    } else {
        return false;
    }
    if (!member->member_expr.object || member->member_expr.object->type != AST_IDENTIFIER) return false;
    *column = member->member_expr.member;
    switch (body->binary_expr.op) {
        case BINOP_GT: *op = member_on_left ? 2 : 4; break;
        case BINOP_GTE: *op = member_on_left ? 3 : 5; break;
        case BINOP_LT: *op = member_on_left ? 4 : 2; break;
        case BINOP_LTE: *op = member_on_left ? 5 : 3; break;
        case BINOP_EQ: *op = 0; break;
        case BINOP_NEQ: *op = 1; break;
        default: return false;
    }
    if (literal->literal.literal_type == TOKEN_INTEGER) {
        *threshold = (double)literal->literal.int_value;
    } else if (literal->literal.literal_type == TOKEN_FLOAT) {
        *threshold = literal->literal.float_value;
    } else {
        return false;
    }
    return true;
}

static bool extract_df_filter_string(ASTNode* lambda, char** column, char** literal_val, int* op) {
    if (!lambda || lambda->type != AST_LAMBDA_EXPR) return false;
    ASTNode* body = lambda->lambda_expr.lambda_body;
    if (!body || body->type != AST_BINARY_EXPR) return false;
    ASTNode* left = body->binary_expr.left;
    ASTNode* right = body->binary_expr.right;
    if (!left || !right) return false;
    ASTNode* member = NULL;
    ASTNode* lit = NULL;
    if (left->type == AST_MEMBER_EXPR && right->type == AST_LITERAL && right->literal.literal_type == TOKEN_STRING) {
        member = left; lit = right;
    } else if (right->type == AST_MEMBER_EXPR && left->type == AST_LITERAL && left->literal.literal_type == TOKEN_STRING) {
        member = right; lit = left;
    } else {
        return false;
    }
    if (!member->member_expr.object || member->member_expr.object->type != AST_IDENTIFIER) return false;
    *column = member->member_expr.member;
    *literal_val = lit->literal.string_value;
    switch (body->binary_expr.op) {
        case BINOP_EQ: *op = 0; break;
        case BINOP_NEQ: *op = 1; break;
        default: return false;
    }
    return true;
}

static bool extract_df_map_info(ASTNode* lambda, char** column, double* scale, double* add) {
    if (!lambda || lambda->type != AST_LAMBDA_EXPR) return false;
    ASTNode* body = lambda->lambda_expr.lambda_body;
    if (!body) return false;
    *scale = 1.0; *add = 0.0;
    if (body->type == AST_MEMBER_EXPR) {
        if (!body->member_expr.object || body->member_expr.object->type != AST_IDENTIFIER) return false;
        *column = body->member_expr.member;
        return true;
    }
    if (body->type == AST_BINARY_EXPR) {
        ASTNode* left = body->binary_expr.left;
        ASTNode* right = body->binary_expr.right;
        if (!left || !right) return false;
        ASTNode* member = NULL;
        ASTNode* literal = NULL;
        bool member_on_left = false;
        if (left->type == AST_MEMBER_EXPR && right->type == AST_LITERAL) {
            member = left; literal = right; member_on_left = true;
        } else if (right->type == AST_MEMBER_EXPR && left->type == AST_LITERAL) {
            member = right; literal = left; member_on_left = false;
        } else {
            return false;
        }
        if (!member->member_expr.object || member->member_expr.object->type != AST_IDENTIFIER) return false;
        *column = member->member_expr.member;
        if (literal->literal.literal_type == TOKEN_INTEGER) {
            *scale = (double)literal->literal.int_value;
        } else if (literal->literal.literal_type == TOKEN_FLOAT) {
            *scale = literal->literal.float_value;
        } else {
            return false;
        }
        switch (body->binary_expr.op) {
            case BINOP_MUL:
                if (!member_on_left) return false; // only member * literal supported
                *add = 0.0;
                break;
            case BINOP_ADD:
                *add = *scale;
                *scale = 1.0;
                break;
            case BINOP_SUB:
                if (member_on_left) { *add = -(*scale); *scale = 1.0; }
                else { return false; }
                break;
            default: return false;
        }
        return true;
    }
    return false;
}

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
            char* val = malloc(64);
            snprintf(val, 64, "%.15e", node->literal.float_value);
            return val;
        }
        case TOKEN_STRING: {
            char* str_global = register_string_literal(ctx, node->literal.string_value);
            int len = node->literal.string_value ? strlen(node->literal.string_value) : 0;
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
        // Variable not found - return type-appropriate default value
        Symbol* symbol = lookup_symbol(ctx->analyzer->symbol_table, node->identifier.id_name);
        Type* var_type = symbol ? symbol->type : create_primitive_type(TYPE_INT);

        char* default_val = malloc(64);
        switch (var_type->kind) {
            case TYPE_FLOAT:
                strcpy(default_val, "0.000000e+00");
                break;
            case TYPE_BOOL:
                strcpy(default_val, "false");
                break;
            case TYPE_STRING:
                strcpy(default_val, "null");
                break;
            default:
                strcpy(default_val, "0");
                break;
        }

        fprintf(stderr, "Aviso: Variável '%s' não encontrada. Usando %s.\n",
                node->identifier.id_name, default_val);
        return default_val;
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
    Type* right_type = analyze_expression(ctx->analyzer, node->binary_expr.right);
    
    bool is_float = (left_type && left_type->kind == TYPE_FLOAT) ||
                    (right_type && right_type->kind == TYPE_FLOAT);
    
    bool is_string = (left_type && left_type->kind == TYPE_STRING) ||
                     (right_type && right_type->kind == TYPE_STRING);
    
    bool is_bool = (left_type && left_type->kind == TYPE_BOOL) ||
                   (right_type && right_type->kind == TYPE_BOOL);
    
    switch (node->binary_expr.op) {
        case BINOP_ADD:
            emit(ctx, is_float ? "  %s = fadd double %s, %s\n" : "  %s = add i64 %s, %s\n", 
                 result, left, right); 
            break;
        case BINOP_SUB:
            emit(ctx, is_float ? "  %s = fsub double %s, %s\n" : "  %s = sub i64 %s, %s\n", 
                 result, left, right); 
            break;
        case BINOP_MUL:
            emit(ctx, is_float ? "  %s = fmul double %s, %s\n" : "  %s = mul i64 %s, %s\n", 
                 result, left, right); 
            break;
        case BINOP_DIV:
            emit(ctx, is_float ? "  %s = fdiv double %s, %s\n" : "  %s = sdiv i64 %s, %s\n", 
                 result, left, right); 
            break;
        case BINOP_MOD:
            emit(ctx, "  %s = srem i64 %s, %s\n", result, left, right); 
            break;
            
        case BINOP_EQ:
            if (is_string) {
                // Check for null strings first to avoid strcmp crash
                char* left_is_null = gen_temp(ctx);
                char* right_is_null = gen_temp(ctx);
                char* both_null = gen_temp(ctx);
                char* any_null = gen_temp(ctx);
                char* strcmp_result = gen_temp(ctx);
                char* strcmp_eq = gen_temp(ctx);

                emit(ctx, "  %s = icmp eq i8* %s, null\n", left_is_null, left);
                emit(ctx, "  %s = icmp eq i8* %s, null\n", right_is_null, right);
                emit(ctx, "  %s = and i1 %s, %s\n", both_null, left_is_null, right_is_null);
                emit(ctx, "  %s = or i1 %s, %s\n", any_null, left_is_null, right_is_null);
                emit(ctx, "  br i1 %s, label %%str_null_%d, label %%str_cmp_%d\n",
                     any_null, ctx->temp_counter, ctx->temp_counter);
                emit(ctx, "str_null_%d:\n", ctx->temp_counter);
                emit(ctx, "  br label %%str_merge_%d\n", ctx->temp_counter);
                emit(ctx, "str_cmp_%d:\n", ctx->temp_counter);
                emit(ctx, "  %s = call i32 @strcmp(i8* %s, i8* %s)\n",
                     strcmp_result, left, right);
                emit(ctx, "  %s = icmp eq i32 %s, 0\n", strcmp_eq, strcmp_result);
                emit(ctx, "  br label %%str_merge_%d\n", ctx->temp_counter);
                emit(ctx, "str_merge_%d:\n", ctx->temp_counter);
                emit(ctx, "  %s = phi i1 [%s, %%str_null_%d], [%s, %%str_cmp_%d]\n",
                     result, both_null, ctx->temp_counter, strcmp_eq, ctx->temp_counter);
                ctx->temp_counter++;
            } else if (is_bool) {
                emit(ctx, "  %s = icmp eq i1 %s, %s\n", result, left, right);
            } else if (is_float) {
                emit(ctx, "  %s = fcmp oeq double %s, %s\n", result, left, right);
            } else {
                emit(ctx, "  %s = icmp eq i64 %s, %s\n", result, left, right);
            }
            break;
            
        case BINOP_NEQ:
            if (is_string) {
                char* strcmp_result = gen_temp(ctx);
                emit(ctx, "  %s = call i32 @strcmp(i8* %s, i8* %s)\n", 
                     strcmp_result, left, right);
                emit(ctx, "  %s = icmp ne i32 %s, 0\n", result, strcmp_result);
            } else if (is_bool) {
                // Comparação booleana usa i1
                emit(ctx, "  %s = icmp ne i1 %s, %s\n", result, left, right);
            } else if (is_float) {
                emit(ctx, "  %s = fcmp one double %s, %s\n", result, left, right);
            } else {
                emit(ctx, "  %s = icmp ne i64 %s, %s\n", result, left, right);
            }
            break;
            
        case BINOP_LT:
            if (is_string) {
                char* strcmp_result = gen_temp(ctx);
                emit(ctx, "  %s = call i32 @strcmp(i8* %s, i8* %s)\n", 
                     strcmp_result, left, right);
                emit(ctx, "  %s = icmp slt i32 %s, 0\n", result, strcmp_result);
            } else if (is_float) {
                emit(ctx, "  %s = fcmp olt double %s, %s\n", result, left, right);
            } else {
                emit(ctx, "  %s = icmp slt i64 %s, %s\n", result, left, right);
            }
            break;
            
        case BINOP_LTE:
            if (is_string) {
                char* strcmp_result = gen_temp(ctx);
                emit(ctx, "  %s = call i32 @strcmp(i8* %s, i8* %s)\n", 
                     strcmp_result, left, right);
                emit(ctx, "  %s = icmp sle i32 %s, 0\n", result, strcmp_result);
            } else if (is_float) {
                emit(ctx, "  %s = fcmp ole double %s, %s\n", result, left, right);
            } else {
                emit(ctx, "  %s = icmp sle i64 %s, %s\n", result, left, right);
            }
            break;
            
        case BINOP_GT:
            if (is_string) {
                char* strcmp_result = gen_temp(ctx);
                emit(ctx, "  %s = call i32 @strcmp(i8* %s, i8* %s)\n", 
                     strcmp_result, left, right);
                emit(ctx, "  %s = icmp sgt i32 %s, 0\n", result, strcmp_result);
            } else if (is_float) {
                emit(ctx, "  %s = fcmp ogt double %s, %s\n", result, left, right);
            } else {
                emit(ctx, "  %s = icmp sgt i64 %s, %s\n", result, left, right);
            }
            break;
            
        case BINOP_GTE:
            if (is_string) {
                char* strcmp_result = gen_temp(ctx);
                emit(ctx, "  %s = call i32 @strcmp(i8* %s, i8* %s)\n", 
                     strcmp_result, left, right);
                emit(ctx, "  %s = icmp sge i32 %s, 0\n", result, strcmp_result);
            } else if (is_float) {
                emit(ctx, "  %s = fcmp oge double %s, %s\n", result, left, right);
            } else {
                emit(ctx, "  %s = icmp sge i64 %s, %s\n", result, left, right);
            }
            break;
            
        case BINOP_AND:
            emit(ctx, "  %s = and i1 %s, %s\n", result, left, right); 
            break;
        case BINOP_OR:
            emit(ctx, "  %s = or i1 %s, %s\n", result, left, right); 
            break;
        default: 
            return "0";
    }
    return result;
}

static char* generate_unary_expr(CodeGenContext* ctx, ASTNode* node) {
    char* operand = generate_expr(ctx, node->unary_expr.operand);
    char* result = gen_temp(ctx);
    
    Type* operand_type = analyze_expression(ctx->analyzer, node->unary_expr.operand);
    bool is_float = operand_type && operand_type->kind == TYPE_FLOAT;
    
    switch (node->unary_expr.op) {
        case UNOP_NEG:
            if (is_float) {
                emit(ctx, "  %s = fneg double %s\n", result, operand);
            } else {
                emit(ctx, "  %s = sub i64 0, %s\n", result, operand);
            }
            break;
        case UNOP_NOT:
            emit(ctx, "  %s = xor i1 %s, true\n", result, operand);
            break;
        default:
            return operand;
    }
    return result;
}

// Verifica se é uma função builtin de agregação
static bool is_builtin_aggregate(const char* name) {
    return strcmp(name, "sum") == 0 ||
           strcmp(name, "mean") == 0 ||
           strcmp(name, "min") == 0 ||
           strcmp(name, "max") == 0 ||
           strcmp(name, "count") == 0;
}

// Verifica se é um construtor de tipo customizado
static __attribute__((unused)) bool is_custom_type_constructor(const char* name) {
    return find_data_type(name) != NULL;
}

static char* generate_call_expr(CodeGenContext* ctx, ASTNode* node) {
    char* func_name = node->call_expr.callee->identifier.id_name;
    
    // ========== CONSTRUTORES DE TIPOS CUSTOMIZADOS ==========
    DataTypeInfo* dt = find_data_type(func_name);
    if (dt != NULL) {
        // É um construtor de tipo
        
        // Validação: número correto de argumentos
        if (node->call_expr.arg_count != dt->field_count) {
            fprintf(stderr, "Erro: Construtor '%s' espera %d argumentos, mas recebeu %d\n",
                    func_name, dt->field_count, node->call_expr.arg_count);
            return "null";
        }
        
        // Calcula tamanho total da estrutura
        int total_size = 0;
        for (int i = 0; i < dt->field_count; i++) {
            // Cada campo ocupa 8 bytes (simplificação)
            total_size += 8;
        }
        
        // Aloca memória
        char* malloc_result = gen_temp(ctx);
        emit(ctx, "  %s = call i8* @malloc(i64 %d)\n", malloc_result, total_size);
        
        // Converte para tipo correto
        char* typed_ptr = gen_temp(ctx);
        emit(ctx, "  %s = bitcast i8* %s to %%struct.%s*\n", 
             typed_ptr, malloc_result, dt->name);
        
        // Inicializa cada campo
        for (int i = 0; i < node->call_expr.arg_count; i++) {
            // Gera valor do argumento
            char* arg_val = generate_expr(ctx, node->call_expr.arguments[i]);
            
            // Obtém ponteiro para o campo
            char* field_ptr = gen_temp(ctx);
            emit(ctx, "  %s = getelementptr %%struct.%s, %%struct.%s* %s, i32 0, i32 %d\n",
                 field_ptr, dt->name, dt->name, typed_ptr, i);
            
            // Armazena valor
            emit(ctx, "  store %s %s, %s* %s\n", 
                 dt->field_types[i], arg_val, dt->field_types[i], field_ptr);
        }
        
        return typed_ptr;
    }
    
    // ========== FUNÇÕES NORMAIS ==========
    Symbol* func_symbol = lookup_symbol(ctx->analyzer->symbol_table, func_name);
    
    if (!func_symbol) {
        fprintf(stderr, "Erro: Função '%s' não declarada\n", func_name);
        return "null";
    }
    
    if (func_symbol->is_lambda_variable) {
        // Carrega ponteiro de função
        const char* var_ptr = get_var_llvm_name(ctx, func_name);
        char* func_ptr = gen_temp(ctx);
        emit(ctx, "  %s = load i8*, i8** %s\n", func_ptr, var_ptr);
        
        // Converte para tipo correto e chama
        const char* ret_type = type_to_llvm(func_symbol->type->return_type);
        char* result = gen_temp(ctx);
        
        emit(ctx, "  %s = call %s %s(", result, ret_type, func_symbol->lambda_func_name);
        
        for (int i = 0; i < node->call_expr.arg_count; i++) {
            if (i > 0) emit(ctx, ", ");
            char* arg = generate_expr(ctx, node->call_expr.arguments[i]);
            emit(ctx, "%s %s", type_to_llvm(func_symbol->param_types[i]), arg);
        }
        emit(ctx, ")\n");
        
        return result;
    }
    
    if (func_symbol->kind != SYMBOL_FUNCTION) {
        fprintf(stderr, "Erro: '%s' não é uma função\n", func_name);
        return "null";
    }
    
    // Verifica número de argumentos
    if (node->call_expr.arg_count != func_symbol->param_count) {
        fprintf(stderr, "Erro: Função '%s' espera %d args mas recebeu %d\n",
                func_name, func_symbol->param_count, node->call_expr.arg_count);
    }
    
    // Gera argumentos
    char** arg_vals = malloc(node->call_expr.arg_count * sizeof(char*));
    Type** arg_types = malloc(node->call_expr.arg_count * sizeof(Type*));

    for (int i = 0; i < node->call_expr.arg_count; i++) {
        arg_vals[i] = generate_expr(ctx, node->call_expr.arguments[i]);
        arg_types[i] = analyze_expression(ctx->analyzer, node->call_expr.arguments[i]);
    }

    if (is_builtin_aggregate(func_name) &&
        node->call_expr.arg_count > 0 &&
        arg_types[0] && arg_types[0]->kind == TYPE_DATAFRAME) {
        if (strcmp(func_name, "count") == 0) {
            char* df_count = gen_temp(ctx);
            emit(ctx, "  %s = call i64 @datalang_df_count(i8* %s)\n", df_count, arg_vals[0]);
            free(arg_vals);
            free(arg_types);
            return df_count;
        }
        free(arg_vals);
        free(arg_types);
        return "0";
    }

    const char* ret_type = type_to_llvm(func_symbol->type->return_type);
    bool is_void = (strcmp(ret_type, "void") == 0);

    char* result = is_void ? NULL : gen_temp(ctx);

    // Detecta agregados Float e normaliza arrays de tipos customizados
    const char* llvm_func_name;
    bool is_float_aggregate = false;

    if (strcmp(func_name, "main") == 0) {
        llvm_func_name = "user_main";
    } else if (is_builtin_aggregate(func_name) && strcmp(func_name, "count") != 0 &&
               node->call_expr.arg_count > 0 &&
               arg_types[0] && arg_types[0]->kind == TYPE_ARRAY &&
               arg_types[0]->element_type && arg_types[0]->element_type->kind == TYPE_FLOAT) {
        // Agregado em Float array (exceto count) - use versão _float
        char* float_func = malloc(128);
        snprintf(float_func, 128, "%s_float", func_name);
        llvm_func_name = float_func;
        is_float_aggregate = true;
        // Atualiza ret_type
        ret_type = "double";
    } else {
        llvm_func_name = func_name;
        // Para agregados não-Float (ou count em qualquer array), normalize arrays
        if (is_builtin_aggregate(func_name)) {
            for (int i = 0; i < node->call_expr.arg_count; i++) {
                if (arg_types[i] && arg_types[i]->kind == TYPE_ARRAY) {
                    // For count(), we need to convert Float arrays to {i64, i64*} too
                    if (strcmp(func_name, "count") == 0 &&
                        arg_types[i]->element_type &&
                        arg_types[i]->element_type->kind == TYPE_FLOAT) {
                        // Convert {i64, double*} to {i64, i64*}
                        char* size = gen_temp(ctx);
                        char* data = gen_temp(ctx);
                        char* data_as_i64 = gen_temp(ctx);
                        char* temp1 = gen_temp(ctx);
                        char* temp2 = gen_temp(ctx);
                        emit(ctx, "  %s = extractvalue {i64, double*} %s, 0\n", size, arg_vals[i]);
                        emit(ctx, "  %s = extractvalue {i64, double*} %s, 1\n", data, arg_vals[i]);
                        emit(ctx, "  %s = bitcast double* %s to i64*\n", data_as_i64, data);
                        emit(ctx, "  %s = insertvalue {i64, i64*} undef, i64 %s, 0\n", temp1, size);
                        emit(ctx, "  %s = insertvalue {i64, i64*} %s, i64* %s, 1\n", temp2, temp1, data_as_i64);
                        arg_vals[i] = temp2;
                    } else {
                        // For other aggregates, normalize custom type arrays only
                        arg_vals[i] = normalize_array_to_i64(ctx, arg_vals[i], arg_types[i]);
                    }
                }
            }
        }
    }

    // Emite chamada
    if (is_void) {
        emit(ctx, "  call %s @%s(", ret_type, llvm_func_name);
    } else {
        emit(ctx, "  %s = call %s @%s(", result, ret_type, llvm_func_name);
    }

    for (int i = 0; i < node->call_expr.arg_count; i++) {
        if (i > 0) emit(ctx, ", ");

        // Use the correct parameter type
        const char* param_type;
        if (is_float_aggregate && i == 0) {
            // For Float aggregates, use {i64, double*}
            param_type = "{i64, double*}";
        } else {
            param_type = type_to_llvm(func_symbol->param_types[i]);
        }
        emit(ctx, "%s %s", param_type, arg_vals[i]);
    }
    emit(ctx, ")\n");

    free(arg_vals);
    free(arg_types);
    return is_void ? "0" : result;
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
    
    if (node->aggregate_transform.agg_arg_count == 0) {
        // Para uso em pipeline - retorna um marcador especial
        char* result = gen_temp(ctx);
        // Marca que é uma transformação pendente no pipeline
        emit(ctx, "  ; Aggregate transform: %s (to be applied in pipeline)\n", func_name);
        return result;
    }
    
    char* array_val = generate_expr(ctx, node->aggregate_transform.agg_args[0]);
    
    char* result = gen_temp(ctx);
    emit(ctx, "  %s = call %s @%s({i64, i64*} %s)\n", result, ret_type, func_name, array_val);
    
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
    
    bool is_custom = (elem_type->kind == TYPE_CUSTOM);
    const char* external_type = type_to_llvm(elem_type);
    // Custom types are stored as i64 (pointer-to-int) internally, other types use their real LLVM type
    const char* internal_type = is_custom ? "i64" : external_type;

    int elem_size = 8;
    if (!is_custom) {
        switch (elem_type->kind) {
            case TYPE_BOOL: elem_size = 1; break;
            case TYPE_INT: elem_size = 8; break;
            case TYPE_FLOAT: elem_size = 8; break;
            case TYPE_STRING: elem_size = 8; break;  // pointers
            default: elem_size = 8; break;
        }
    }

    char* array_ptr = gen_temp(ctx);
    int size_bytes = (count == 0) ? 8 : count * elem_size;
    emit(ctx, "  %s = call i8* @malloc(i64 %d)\n", array_ptr, size_bytes);

    char* typed_ptr = gen_temp(ctx);
    emit(ctx, "  %s = bitcast i8* %s to %s*\n", typed_ptr, array_ptr, internal_type);

    for (int i = 0; i < count; i++) {
        char* val = generate_expr(ctx, node->array_literal.elements[i]);

        // Convert struct pointers to i64 for uniform storage
        if (is_custom) {
            char* ptr_as_int = gen_temp(ctx);
            emit(ctx, "  %s = ptrtoint %s %s to i64\n",
                 ptr_as_int, external_type, val);
            val = ptr_as_int;
        }

        char* elem_ptr = gen_temp(ctx);
        emit(ctx, "  %s = getelementptr %s, %s* %s, i64 %d\n",
             elem_ptr, internal_type, internal_type, typed_ptr, i);
        emit(ctx, "  store %s %s, %s* %s\n", internal_type, val, internal_type, elem_ptr);
    }

    // Create the array structure with internal type (always i64* for data)
    char* struct_ptr = gen_temp(ctx);
    emit(ctx, "  %s = alloca {i64, %s*}\n", struct_ptr, internal_type);

    char* size_ptr = gen_temp(ctx);
    emit(ctx, "  %s = getelementptr {i64, %s*}, {i64, %s*}* %s, i32 0, i32 0\n",
         size_ptr, internal_type, internal_type, struct_ptr);
    emit(ctx, "  store i64 %d, i64* %s\n", count, size_ptr);

    char* data_ptr = gen_temp(ctx);
    emit(ctx, "  %s = getelementptr {i64, %s*}, {i64, %s*}* %s, i32 0, i32 1\n",
         data_ptr, internal_type, internal_type, struct_ptr);
    emit(ctx, "  store %s* %s, %s** %s\n", internal_type, typed_ptr, internal_type, data_ptr);

    char* loaded_array = gen_temp(ctx);
    emit(ctx, "  %s = load {i64, %s*}, {i64, %s*}* %s\n",
         loaded_array, internal_type, internal_type, struct_ptr);

    // If it's a custom type array, reconstruct with correct pointer type
    if (is_custom) {
        // Extract size and data pointer from the i64* array
        char* size_val = gen_temp(ctx);
        emit(ctx, "  %s = extractvalue {i64, i64*} %s, 0\n", size_val, loaded_array);
        char* data_val = gen_temp(ctx);
        emit(ctx, "  %s = extractvalue {i64, i64*} %s, 1\n", data_val, loaded_array);

        // Bitcast the data pointer from i64* to the correct struct pointer type
        char* bitcast_data = gen_temp(ctx);
        emit(ctx, "  %s = bitcast i64* %s to %s*\n", bitcast_data, data_val, external_type);

        // Reconstruct the array with the correct type
        char* final_struct = gen_temp(ctx);
        emit(ctx, "  %s = alloca {i64, %s*}\n", final_struct, external_type);

        char* final_size_ptr = gen_temp(ctx);
        emit(ctx, "  %s = getelementptr {i64, %s*}, {i64, %s*}* %s, i32 0, i32 0\n",
             final_size_ptr, external_type, external_type, final_struct);
        emit(ctx, "  store i64 %s, i64* %s\n", size_val, final_size_ptr);

        char* final_data_ptr = gen_temp(ctx);
        emit(ctx, "  %s = getelementptr {i64, %s*}, {i64, %s*}* %s, i32 0, i32 1\n",
             final_data_ptr, external_type, external_type, final_struct);
        emit(ctx, "  store %s* %s, %s** %s\n", external_type, bitcast_data, external_type, final_data_ptr);

        char* final_val = gen_temp(ctx);
        emit(ctx, "  %s = load {i64, %s*}, {i64, %s*}* %s\n",
             final_val, external_type, external_type, final_struct);
        return final_val;
    }

    return loaded_array;
}

static char* generate_range_expr(CodeGenContext* ctx, ASTNode* node) {
    char* start = generate_expr(ctx, node->range_expr.range_start);
    char* end = generate_expr(ctx, node->range_expr.range_end);
    
    char* size = gen_temp(ctx);
    emit(ctx, "  %s = sub i64 %s, %s\n", size, end, start);
    char* size_plus_one = gen_temp(ctx);
    emit(ctx, "  %s = add i64 %s, 1\n", size_plus_one, size);
    
    char* bytes_needed = gen_temp(ctx);
    emit(ctx, "  %s = mul i64 %s, 8\n", bytes_needed, size_plus_one);
    char* array_ptr = gen_temp(ctx);
    emit(ctx, "  %s = call i8* @malloc(i64 %s)\n", array_ptr, bytes_needed);
    char* typed_ptr = gen_temp(ctx);
    emit(ctx, "  %s = bitcast i8* %s to i64*\n", typed_ptr, array_ptr);
    
    char* loop_cond = gen_label(ctx);
    char* loop_body = gen_label(ctx);
    char* loop_end = gen_label(ctx);
    
    char* idx_ptr = gen_temp(ctx);
    emit(ctx, "  %s = alloca i64\n", idx_ptr);
    emit(ctx, "  store i64 0, i64* %s\n", idx_ptr);
    
    char* val_ptr = gen_temp(ctx);
    emit(ctx, "  %s = alloca i64\n", val_ptr);
    emit(ctx, "  store i64 %s, i64* %s\n", start, val_ptr);
    
    emit(ctx, "  br label %%%s\n", loop_cond);
    emit(ctx, "\n%s:\n", loop_cond);
    
    char* idx_val = gen_temp(ctx);
    emit(ctx, "  %s = load i64, i64* %s\n", idx_val, idx_ptr);
    char* cmp = gen_temp(ctx);
    emit(ctx, "  %s = icmp slt i64 %s, %s\n", cmp, idx_val, size_plus_one);
    emit(ctx, "  br i1 %s, label %%%s, label %%%s\n", cmp, loop_body, loop_end);
    
    emit(ctx, "\n%s:\n", loop_body);
    char* current_val = gen_temp(ctx);
    emit(ctx, "  %s = load i64, i64* %s\n", current_val, val_ptr);
    char* elem_ptr = gen_temp(ctx);
    emit(ctx, "  %s = getelementptr i64, i64* %s, i64 %s\n", elem_ptr, typed_ptr, idx_val);
    emit(ctx, "  store i64 %s, i64* %s\n", current_val, elem_ptr);
    
    char* next_val = gen_temp(ctx);
    emit(ctx, "  %s = add i64 %s, 1\n", next_val, current_val);
    emit(ctx, "  store i64 %s, i64* %s\n", next_val, val_ptr);
    char* next_idx = gen_temp(ctx);
    emit(ctx, "  %s = add i64 %s, 1\n", next_idx, idx_val);
    emit(ctx, "  store i64 %s, i64* %s\n", next_idx, idx_ptr);
    emit(ctx, "  br label %%%s\n", loop_cond);
    
    emit(ctx, "\n%s:\n", loop_end);
    
    char* struct_ptr = gen_temp(ctx);
    emit(ctx, "  %s = alloca {i64, i64*}\n", struct_ptr);
    
    char* size_field = gen_temp(ctx);
    emit(ctx, "  %s = getelementptr {i64, i64*}, {i64, i64*}* %s, i32 0, i32 0\n", size_field, struct_ptr);
    emit(ctx, "  store i64 %s, i64* %s\n", size_plus_one, size_field);
    
    char* data_field = gen_temp(ctx);
    emit(ctx, "  %s = getelementptr {i64, i64*}, {i64, i64*}* %s, i32 0, i32 1\n", data_field, struct_ptr);
    emit(ctx, "  store i64* %s, i64** %s\n", typed_ptr, data_field);
    
    char* final_val = gen_temp(ctx);
    emit(ctx, "  %s = load {i64, i64*}, {i64, i64*}* %s\n", final_val, struct_ptr);
    return final_val;
}

static char* generate_index_expr(CodeGenContext* ctx, ASTNode* node) {
    char* array_val = generate_expr(ctx, node->index_expr.object);
    char* index_val = generate_expr(ctx, node->index_expr.index);

    Type* array_type = analyze_expression(ctx->analyzer, node->index_expr.object);
    Type* elem_type = NULL;
    if (array_type && array_type->kind == TYPE_ARRAY && array_type->element_type) {
        elem_type = array_type->element_type;
    } else {
        elem_type = create_primitive_type(TYPE_INT);
    }

    char elem_type_str[256];
    strncpy(elem_type_str, type_to_llvm(elem_type), 255);

    char* data_ptr = gen_temp(ctx);
    emit(ctx,
         "  %s = extractvalue {i64, %s*} %s, 1\n",
         data_ptr, elem_type_str, array_val);

    char* elem_ptr = gen_temp(ctx);
    emit(ctx,
         "  %s = getelementptr %s, %s* %s, i64 %s\n",
         elem_ptr, elem_type_str, elem_type_str, data_ptr, index_val);

    char* elem_val = gen_temp(ctx);
    emit(ctx,
         "  %s = load %s, %s* %s\n",
         elem_val, elem_type_str, elem_type_str, elem_ptr);

    return elem_val;
}

// ==================== PIPELINE COM FILTER/MAP/REDUCE ====================

// Helper: Convert custom type arrays to {i64, i64*} for transforms and aggregates
static char* normalize_array_to_i64(CodeGenContext* ctx, char* array_val, Type* array_type) {
    if (!array_type || array_type->kind != TYPE_ARRAY) {
        return array_val;  // Not an array, return as-is
    }

    Type* elem_type = array_type->element_type;
    if (!elem_type) {
        return array_val;
    }

    // Only normalize CUSTOM types (structs), not primitive types
    if (elem_type->kind != TYPE_CUSTOM) {
        return array_val;  // Primitive type arrays (Int, Float, etc.) stay as-is
    }

    const char* elem_llvm_type = type_to_llvm(elem_type);

    // Extract size and data from the typed array
    char* size_val = gen_temp(ctx);
    char* data_val = gen_temp(ctx);

    emit(ctx, "  %s = extractvalue {i64, %s*} %s, 0\n", size_val, elem_llvm_type, array_val);
    emit(ctx, "  %s = extractvalue {i64, %s*} %s, 1\n", data_val, elem_llvm_type, array_val);

    // Bitcast the data pointer to i64*
    char* bitcast_data = gen_temp(ctx);
    emit(ctx, "  %s = bitcast %s* %s to i64*\n", bitcast_data, elem_llvm_type, data_val);

    // Reconstruct as {i64, i64*}
    char* generic_struct = gen_temp(ctx);
    emit(ctx, "  %s = alloca {i64, i64*}\n", generic_struct);

    char* size_ptr = gen_temp(ctx);
    emit(ctx, "  %s = getelementptr {i64, i64*}, {i64, i64*}* %s, i32 0, i32 0\n",
         size_ptr, generic_struct);
    emit(ctx, "  store i64 %s, i64* %s\n", size_val, size_ptr);

    char* data_ptr = gen_temp(ctx);
    emit(ctx, "  %s = getelementptr {i64, i64*}, {i64, i64*}* %s, i32 0, i32 1\n",
         data_ptr, generic_struct);
    emit(ctx, "  store i64* %s, i64** %s\n", bitcast_data, data_ptr);

    char* result = gen_temp(ctx);
    emit(ctx, "  %s = load {i64, i64*}, {i64, i64*}* %s\n", result, generic_struct);

    return result;
}

// Wrapper for backward compatibility
static char* normalize_array_for_transform(CodeGenContext* ctx, char* array_val, Type* array_type) {
    return normalize_array_to_i64(ctx, array_val, array_type);
}

static char* generate_filter_transform(CodeGenContext* ctx, ASTNode* node, char* input_array) {
    // filter(|x| predicate) sobre input_array
    // Cria um novo array com elementos que passam no predicado
    
    char* loop_cond = gen_label(ctx);
    char* loop_body = gen_label(ctx);
    char* add_elem = gen_label(ctx);
    char* skip_elem = gen_label(ctx);
    char* loop_end = gen_label(ctx);
    
    // Extrai size e data do array de entrada
    char* input_size = gen_temp(ctx);
    emit(ctx, "  %s = extractvalue {i64, i64*} %s, 0\n", input_size, input_array);
    char* input_data = gen_temp(ctx);
    emit(ctx, "  %s = extractvalue {i64, i64*} %s, 1\n", input_data, input_array);
    
    // Aloca array de saída (tamanho máximo = input_size)
    char* output_bytes = gen_temp(ctx);
    emit(ctx, "  %s = mul i64 %s, 8\n", output_bytes, input_size);
    char* output_raw = gen_temp(ctx);
    emit(ctx, "  %s = call i8* @malloc(i64 %s)\n", output_raw, output_bytes);
    char* output_data = gen_temp(ctx);
    emit(ctx, "  %s = bitcast i8* %s to i64*\n", output_data, output_raw);
    
    // Índices
    char* i_ptr = gen_temp(ctx);
    emit(ctx, "  %s = alloca i64\n", i_ptr);
    emit(ctx, "  store i64 0, i64* %s\n", i_ptr);
    
    char* out_idx_ptr = gen_temp(ctx);
    emit(ctx, "  %s = alloca i64\n", out_idx_ptr);
    emit(ctx, "  store i64 0, i64* %s\n", out_idx_ptr);
    
    emit(ctx, "  br label %%%s\n", loop_cond);
    emit(ctx, "\n%s:\n", loop_cond);
    
    char* i_val = gen_temp(ctx);
    emit(ctx, "  %s = load i64, i64* %s\n", i_val, i_ptr);
    char* cmp = gen_temp(ctx);
    emit(ctx, "  %s = icmp slt i64 %s, %s\n", cmp, i_val, input_size);
    emit(ctx, "  br i1 %s, label %%%s, label %%%s\n", cmp, loop_body, loop_end);
    
    emit(ctx, "\n%s:\n", loop_body);
    
    // Carrega elemento atual
    char* elem_ptr = gen_temp(ctx);
    emit(ctx, "  %s = getelementptr i64, i64* %s, i64 %s\n", elem_ptr, input_data, i_val);
    char* elem_val = gen_temp(ctx);
    emit(ctx, "  %s = load i64, i64* %s\n", elem_val, elem_ptr);
    
    // Aplica o predicado lambda
    ASTNode* lambda = node->filter_transform.filter_predicate;
    char* pred_result = "true";
    
    if (lambda && lambda->type == AST_LAMBDA_EXPR && lambda->lambda_expr.lambda_param_count > 0) {
        char* param_name = lambda->lambda_expr.lambda_params[0]->param.param_name;
        Type* param_type = NULL;
        
        if (lambda->lambda_expr.lambda_params[0]->param.param_type) {
            param_type = ast_type_to_type(ctx->analyzer, 
                                        lambda->lambda_expr.lambda_params[0]->param.param_type);
        }
        
        if (!is_symbol_in_current_scope(ctx->analyzer->symbol_table, param_name)) {
            Symbol* ps = declare_symbol(ctx->analyzer->symbol_table, param_name, SYMBOL_PARAMETER, 
                        param_type ? param_type : create_primitive_type(TYPE_INT), 0, 0);
            if (ps) ps->initialized = true;
        }
        
        if (param_type && param_type->kind == TYPE_CUSTOM) {
            // Cria variável para o parâmetro apontando para a struct
            char* param_ptr = gen_temp(ctx);
            emit(ctx, "  %s = alloca %%struct.%s*\n", param_ptr, param_type->custom_name);
            
            // elem_val já é o ponteiro para struct, converte de i64
            char* struct_ptr = gen_temp(ctx);
            emit(ctx, "  %s = inttoptr i64 %s to %%struct.%s*\n", 
                struct_ptr, elem_val, param_type->custom_name);
            emit(ctx, "  store %%struct.%s* %s, %%struct.%s** %s\n",
                param_type->custom_name, struct_ptr, param_type->custom_name, param_ptr);
            
            add_var_mapping(ctx, param_name, param_ptr);
        } else {
            // Tipo primitivo - comportamento atual
            char* param_ptr = gen_temp(ctx);
            emit(ctx, "  %s = alloca i64\n", param_ptr);
            emit(ctx, "  store i64 %s, i64* %s\n", elem_val, param_ptr);
            add_var_mapping(ctx, param_name, param_ptr);
        }
        
        // Marca símbolo como inicializado
        Symbol* param_sym = lookup_symbol(ctx->analyzer->symbol_table, param_name);
        if (param_sym) param_sym->initialized = true;
        
        pred_result = generate_expr(ctx, lambda->lambda_expr.lambda_body);
    }
    
    emit(ctx, "  br i1 %s, label %%%s, label %%%s\n", pred_result, add_elem, skip_elem);
    
    emit(ctx, "\n%s:\n", add_elem);
    char* out_idx = gen_temp(ctx);
    emit(ctx, "  %s = load i64, i64* %s\n", out_idx, out_idx_ptr);
    char* out_elem_ptr = gen_temp(ctx);
    emit(ctx, "  %s = getelementptr i64, i64* %s, i64 %s\n", out_elem_ptr, output_data, out_idx);
    emit(ctx, "  store i64 %s, i64* %s\n", elem_val, out_elem_ptr);
    char* next_out_idx = gen_temp(ctx);
    emit(ctx, "  %s = add i64 %s, 1\n", next_out_idx, out_idx);
    emit(ctx, "  store i64 %s, i64* %s\n", next_out_idx, out_idx_ptr);
    emit(ctx, "  br label %%%s\n", skip_elem);
    
    emit(ctx, "\n%s:\n", skip_elem);
    char* next_i = gen_temp(ctx);
    emit(ctx, "  %s = add i64 %s, 1\n", next_i, i_val);
    emit(ctx, "  store i64 %s, i64* %s\n", next_i, i_ptr);
    emit(ctx, "  br label %%%s\n", loop_cond);
    
    emit(ctx, "\n%s:\n", loop_end);
    
    // Cria struct do resultado
    char* final_out_idx = gen_temp(ctx);
    emit(ctx, "  %s = load i64, i64* %s\n", final_out_idx, out_idx_ptr);
    
    char* result_ptr = gen_temp(ctx);
    emit(ctx, "  %s = alloca {i64, i64*}\n", result_ptr);
    
    char* size_field = gen_temp(ctx);
    emit(ctx, "  %s = getelementptr {i64, i64*}, {i64, i64*}* %s, i32 0, i32 0\n", size_field, result_ptr);
    emit(ctx, "  store i64 %s, i64* %s\n", final_out_idx, size_field);
    
    char* data_field = gen_temp(ctx);
    emit(ctx, "  %s = getelementptr {i64, i64*}, {i64, i64*}* %s, i32 0, i32 1\n", data_field, result_ptr);
    emit(ctx, "  store i64* %s, i64** %s\n", output_data, data_field);
    
    char* result = gen_temp(ctx);
    emit(ctx, "  %s = load {i64, i64*}, {i64, i64*}* %s\n", result, result_ptr);
    
    return result;
}

static char* generate_map_transform(CodeGenContext* ctx, ASTNode* node, char* input_array) {
    char* loop_cond = gen_label(ctx);
    char* loop_body = gen_label(ctx);
    char* loop_end = gen_label(ctx);
    
    // Deduz tipo de elemento a partir do parâmetro da lambda
    const char* elem_llvm_type = "i64";
    if (node->map_transform.map_function &&
        node->map_transform.map_function->type == AST_LAMBDA_EXPR &&
        node->map_transform.map_function->lambda_expr.lambda_param_count > 0 &&
        node->map_transform.map_function->lambda_expr.lambda_params[0]->param.param_type) {
        Type* ptype = ast_type_to_type(ctx->analyzer,
            node->map_transform.map_function->lambda_expr.lambda_params[0]->param.param_type);
        elem_llvm_type = type_to_llvm(ptype);
    }
    char struct_type[64];
    snprintf(struct_type, 64, "{i64, %s*}", elem_llvm_type);

    // Converte somente se for array de tipo custom normalizado para {i64, i64*}
    char* typed_array = input_array;
    bool param_is_custom = (node->map_transform.map_function &&
        node->map_transform.map_function->type == AST_LAMBDA_EXPR &&
        node->map_transform.map_function->lambda_expr.lambda_param_count > 0 &&
        node->map_transform.map_function->lambda_expr.lambda_params[0]->param.param_type &&
        ast_type_to_type(ctx->analyzer,
            node->map_transform.map_function->lambda_expr.lambda_params[0]->param.param_type)->kind == TYPE_CUSTOM);
    if (param_is_custom) {
        char* sz = gen_temp(ctx);
        char* data = gen_temp(ctx);
        emit(ctx, "  %s = extractvalue {i64, i64*} %s, 0\n", sz, input_array);
        emit(ctx, "  %s = extractvalue {i64, i64*} %s, 1\n", data, input_array);
        char* cast_data = gen_temp(ctx);
        emit(ctx, "  %s = bitcast i64* %s to %s*\n", cast_data, data, elem_llvm_type);
        char* tmp = gen_temp(ctx);
        emit(ctx, "  %s = alloca %s\n", tmp, struct_type);
        char* sz_ptr = gen_temp(ctx);
        emit(ctx, "  %s = getelementptr %s, %s* %s, i32 0, i32 0\n", sz_ptr, struct_type, struct_type, tmp);
        emit(ctx, "  store i64 %s, i64* %s\n", sz, sz_ptr);
        char* data_ptr = gen_temp(ctx);
        emit(ctx, "  %s = getelementptr %s, %s* %s, i32 0, i32 1\n", data_ptr, struct_type, struct_type, tmp);
        emit(ctx, "  store %s* %s, %s** %s\n", elem_llvm_type, cast_data, elem_llvm_type, data_ptr);
        typed_array = gen_temp(ctx);
        emit(ctx, "  %s = load %s, %s* %s\n", typed_array, struct_type, struct_type, tmp);
    }

    char* input_size = gen_temp(ctx);
    emit(ctx, "  %s = extractvalue %s %s, 0\n", input_size, struct_type, typed_array);
    char* input_data = gen_temp(ctx);
    emit(ctx, "  %s = extractvalue %s %s, 1\n", input_data, struct_type, typed_array);
    
    Type* output_elem_type = create_primitive_type(TYPE_INT);
    ASTNode* lambda = node->map_transform.map_function;
    
    if (lambda && lambda->type == AST_LAMBDA_EXPR && lambda->lambda_expr.lambda_body) {
        // Temporarily declare parameter to analyze body type
        if (lambda->lambda_expr.lambda_param_count > 0) {
            enter_scope(ctx->analyzer->symbol_table);
            
            char* param_name = lambda->lambda_expr.lambda_params[0]->param.param_name;
            Type* param_type = NULL;
            
            if (lambda->lambda_expr.lambda_params[0]->param.param_type) {
                param_type = ast_type_to_type(ctx->analyzer,
                                             lambda->lambda_expr.lambda_params[0]->param.param_type);
            } else {
                param_type = create_primitive_type(TYPE_INT);
            }
            
            Symbol* ps = declare_symbol(ctx->analyzer->symbol_table, param_name, SYMBOL_PARAMETER, 
                          param_type, 0, 0);
            if (ps) ps->initialized = true;
            
            // Now analyze the body with parameter in scope
            output_elem_type = analyze_expression(ctx->analyzer, lambda->lambda_expr.lambda_body);
            if (!output_elem_type) output_elem_type = create_primitive_type(TYPE_INT);
            
            exit_scope(ctx->analyzer->symbol_table);
        }
    }
    
    const char* output_llvm_type = type_to_llvm(output_elem_type);
    
    char* output_bytes = gen_temp(ctx);
    emit(ctx, "  %s = mul i64 %s, 8\n", output_bytes, input_size);
    char* output_raw = gen_temp(ctx);
    emit(ctx, "  %s = call i8* @malloc(i64 %s)\n", output_raw, output_bytes);
    char* output_data = gen_temp(ctx);
    emit(ctx, "  %s = bitcast i8* %s to %s*\n", output_data, output_raw, output_llvm_type);
    
    char* i_ptr = gen_temp(ctx);
    emit(ctx, "  %s = alloca i64\n", i_ptr);
    emit(ctx, "  store i64 0, i64* %s\n", i_ptr);
    
    emit(ctx, "  br label %%%s\n", loop_cond);
    emit(ctx, "\n%s:\n", loop_cond);
    
    char* i_val = gen_temp(ctx);
    emit(ctx, "  %s = load i64, i64* %s\n", i_val, i_ptr);
    char* cmp = gen_temp(ctx);
    emit(ctx, "  %s = icmp slt i64 %s, %s\n", cmp, i_val, input_size);
    emit(ctx, "  br i1 %s, label %%%s, label %%%s\n", cmp, loop_body, loop_end);
    
    emit(ctx, "\n%s:\n", loop_body);
    
    char* elem_ptr = gen_temp(ctx);
    emit(ctx, "  %s = getelementptr %s, %s* %s, i64 %s\n", elem_ptr, elem_llvm_type, elem_llvm_type, input_data, i_val);
    char* elem_val = gen_temp(ctx);
    emit(ctx, "  %s = load %s, %s* %s\n", elem_val, elem_llvm_type, elem_llvm_type, elem_ptr);
    
    char* mapped_val = elem_val;
    
    if (lambda && lambda->type == AST_LAMBDA_EXPR && lambda->lambda_expr.lambda_param_count > 0) {
        char* param_name = lambda->lambda_expr.lambda_params[0]->param.param_name;
        Type* param_type = NULL;
        
        if (lambda->lambda_expr.lambda_params[0]->param.param_type) {
            param_type = ast_type_to_type(ctx->analyzer,
                                         lambda->lambda_expr.lambda_params[0]->param.param_type);
        }
        
        if (!is_symbol_in_current_scope(ctx->analyzer->symbol_table, param_name)) {
            Symbol* ps = declare_symbol(ctx->analyzer->symbol_table, param_name, SYMBOL_PARAMETER, 
                          param_type ? param_type : create_primitive_type(TYPE_INT), 0, 0);
            if (ps) ps->initialized = true;
        }
        
        if (param_type && param_type->kind == TYPE_CUSTOM) {
            char* param_ptr = gen_temp(ctx);
            emit(ctx, "  %s = alloca %%struct.%s*\n", param_ptr, param_type->custom_name);
            char* struct_ptr = elem_val;
            if (strcmp(elem_llvm_type, "i64") == 0) {
                struct_ptr = gen_temp(ctx);
                emit(ctx, "  %s = inttoptr i64 %s to %%struct.%s*\n",
                     struct_ptr, elem_val, param_type->custom_name);
            } else if (strcmp(elem_llvm_type, type_to_llvm(param_type)) != 0) {
                char* castp = gen_temp(ctx);
                emit(ctx, "  %s = bitcast %s %s to %%struct.%s*\n", castp, elem_llvm_type, elem_val, param_type->custom_name);
                struct_ptr = castp;
            }
            emit(ctx, "  store %%struct.%s* %s, %%struct.%s** %s\n",
                 param_type->custom_name, struct_ptr, param_type->custom_name, param_ptr);
            
            add_var_mapping(ctx, param_name, param_ptr);
        } else {
            char* param_ptr = gen_temp(ctx);
            emit(ctx, "  %s = alloca %s\n", param_ptr, elem_llvm_type);
            emit(ctx, "  store %s %s, %s* %s\n", elem_llvm_type, elem_val, elem_llvm_type, param_ptr);
            add_var_mapping(ctx, param_name, param_ptr);
        }
        
        Symbol* param_sym = lookup_symbol(ctx->analyzer->symbol_table, param_name);
        if (param_sym) param_sym->initialized = true;
        
        mapped_val = generate_expr(ctx, lambda->lambda_expr.lambda_body);
    }
    
    char* out_elem_ptr = gen_temp(ctx);
    emit(ctx, "  %s = getelementptr %s, %s* %s, i64 %s\n", 
         out_elem_ptr, output_llvm_type, output_llvm_type, output_data, i_val);
    emit(ctx, "  store %s %s, %s* %s\n", 
         output_llvm_type, mapped_val, output_llvm_type, out_elem_ptr);
    
    char* next_i = gen_temp(ctx);
    emit(ctx, "  %s = add i64 %s, 1\n", next_i, i_val);
    emit(ctx, "  store i64 %s, i64* %s\n", next_i, i_ptr);
    emit(ctx, "  br label %%%s\n", loop_cond);
    
    emit(ctx, "\n%s:\n", loop_end);
    
    char* result_ptr = gen_temp(ctx);
    emit(ctx, "  %s = alloca {i64, %s*}\n", result_ptr, output_llvm_type);
    
    char* size_field = gen_temp(ctx);
    emit(ctx, "  %s = getelementptr {i64, %s*}, {i64, %s*}* %s, i32 0, i32 0\n", 
         size_field, output_llvm_type, output_llvm_type, result_ptr);
    emit(ctx, "  store i64 %s, i64* %s\n", input_size, size_field);
    
    char* data_field = gen_temp(ctx);
    emit(ctx, "  %s = getelementptr {i64, %s*}, {i64, %s*}* %s, i32 0, i32 1\n", 
         data_field, output_llvm_type, output_llvm_type, result_ptr);
    emit(ctx, "  store %s* %s, %s** %s\n", 
         output_llvm_type, output_data, output_llvm_type, data_field);
    
    char* result = gen_temp(ctx);
    emit(ctx, "  %s = load {i64, %s*}, {i64, %s*}* %s\n", 
         result, output_llvm_type, output_llvm_type, result_ptr);
    
    return result;
}

static char* generate_reduce_transform(CodeGenContext* ctx, ASTNode* node, char* input_array, Type* array_type) {
    char* loop_cond = gen_label(ctx);
    char* loop_body = gen_label(ctx);
    char* loop_end = gen_label(ctx);
    
    Type* elem_type = (array_type && array_type->kind == TYPE_ARRAY) ? array_type->element_type : NULL;
    const char* elem_llvm = "i64";
    bool is_float = false;
    if (elem_type && elem_type->kind == TYPE_FLOAT) {
        elem_llvm = "double";
        is_float = true;
    }

    char array_struct_type[64];
    snprintf(array_struct_type, 64, "{i64, %s*}", elem_llvm);

    // Converte {i64, i64*} genérico para {i64, T*} tipado apenas para tipos customizados
    char* typed_array = input_array;
    if (elem_type && elem_type->kind == TYPE_CUSTOM) {
        char* sz = gen_temp(ctx);
        char* data = gen_temp(ctx);
        emit(ctx, "  %s = extractvalue {i64, i64*} %s, 0\n", sz, input_array);
        emit(ctx, "  %s = extractvalue {i64, i64*} %s, 1\n", data, input_array);
        char* cast_data = gen_temp(ctx);
        emit(ctx, "  %s = bitcast i64* %s to %s*\n", cast_data, data, elem_llvm);
        char* tmp = gen_temp(ctx);
        emit(ctx, "  %s = alloca %s\n", tmp, array_struct_type);
        char* sz_ptr = gen_temp(ctx);
        emit(ctx, "  %s = getelementptr %s, %s* %s, i32 0, i32 0\n", sz_ptr, array_struct_type, array_struct_type, tmp);
        emit(ctx, "  store i64 %s, i64* %s\n", sz, sz_ptr);
        char* data_ptr = gen_temp(ctx);
        emit(ctx, "  %s = getelementptr %s, %s* %s, i32 0, i32 1\n", data_ptr, array_struct_type, array_struct_type, tmp);
        emit(ctx, "  store %s* %s, %s** %s\n", elem_llvm, cast_data, elem_llvm, data_ptr);
        typed_array = gen_temp(ctx);
        emit(ctx, "  %s = load %s, %s* %s\n", typed_array, array_struct_type, array_struct_type, tmp);
    }

    char* input_size = gen_temp(ctx);
    emit(ctx, "  %s = extractvalue %s %s, 0\n", input_size, array_struct_type, typed_array);
    char* input_data = gen_temp(ctx);
    emit(ctx, "  %s = extractvalue %s %s, 1\n", input_data, array_struct_type, typed_array);
    
    // Valor inicial
    char* init_val = generate_expr(ctx, node->reduce_transform.initial_value);
    
    char* acc_ptr = gen_temp(ctx);
    emit(ctx, "  %s = alloca %s\n", acc_ptr, elem_llvm);
    emit(ctx, "  store %s %s, %s* %s\n", elem_llvm, init_val, elem_llvm, acc_ptr);
    
    char* i_ptr = gen_temp(ctx);
    emit(ctx, "  %s = alloca i64\n", i_ptr);
    emit(ctx, "  store i64 0, i64* %s\n", i_ptr);
    
    emit(ctx, "  br label %%%s\n", loop_cond);
    emit(ctx, "\n%s:\n", loop_cond);
    
    char* i_val = gen_temp(ctx);
    emit(ctx, "  %s = load i64, i64* %s\n", i_val, i_ptr);
    char* cmp = gen_temp(ctx);
    emit(ctx, "  %s = icmp slt i64 %s, %s\n", cmp, i_val, input_size);
    emit(ctx, "  br i1 %s, label %%%s, label %%%s\n", cmp, loop_body, loop_end);
    
    emit(ctx, "\n%s:\n", loop_body);
    
    char* acc_val = gen_temp(ctx);
    emit(ctx, "  %s = load %s, %s* %s\n", acc_val, elem_llvm, elem_llvm, acc_ptr);
    
    char* elem_ptr = gen_temp(ctx);
    emit(ctx, "  %s = getelementptr %s, %s* %s, i64 %s\n", elem_ptr, elem_llvm, elem_llvm, input_data, i_val);
    char* elem_val = gen_temp(ctx);
    emit(ctx, "  %s = load %s, %s* %s\n", elem_val, elem_llvm, elem_llvm, elem_ptr);
    
    // Operação padrão: acc + val (somatório)
    char* new_acc = gen_temp(ctx);
    if (is_float) emit(ctx, "  %s = fadd double %s, %s\n", new_acc, acc_val, elem_val);
    else emit(ctx, "  %s = add i64 %s, %s\n", new_acc, acc_val, elem_val);
    
    emit(ctx, "  store %s %s, %s* %s\n", elem_llvm, new_acc, elem_llvm, acc_ptr);
    
    char* next_i = gen_temp(ctx);
    emit(ctx, "  %s = add i64 %s, 1\n", next_i, i_val);
    emit(ctx, "  store i64 %s, i64* %s\n", next_i, i_ptr);
    emit(ctx, "  br label %%%s\n", loop_cond);
    
    emit(ctx, "\n%s:\n", loop_end);
    
    char* result = gen_temp(ctx);
    emit(ctx, "  %s = load %s, %s* %s\n", result, elem_llvm, elem_llvm, acc_ptr);
    
    return result;
}

static char* generate_select_transform(CodeGenContext* ctx, ASTNode* node, char* input_df) {
    char* result = gen_temp(ctx);
    
    emit(ctx, "  ; select transform with %d columns\n", node->select_transform.column_count);

    // Prepara ponteiros para cada nome de coluna
    char** col_ptrs = malloc(node->select_transform.column_count * sizeof(char*));
    for (int i = 0; i < node->select_transform.column_count; i++) {
        char* col_str = register_string_literal(ctx, node->select_transform.columns[i]);
        int len = strlen(node->select_transform.columns[i]);
        col_ptrs[i] = gen_temp(ctx);
        emit(ctx, "  %s = getelementptr [%d x i8], [%d x i8]* %s, i32 0, i32 0\n",
             col_ptrs[i], len + 1, len + 1, col_str);
    }

    emit(ctx, "  %s = call i8* (i8*, i32, ...) @datalang_select(i8* %s, i32 %d",
         result, input_df, node->select_transform.column_count);
    for (int i = 0; i < node->select_transform.column_count; i++) {
        emit(ctx, ", i8* %s", col_ptrs[i]);
    }
    emit(ctx, ")\n");
    free(col_ptrs);
    
    return result;
}

static char* generate_groupby_transform(CodeGenContext* ctx, ASTNode* node, char* input_df) {
    // Para DataFrames - chama função runtime
    char* result = gen_temp(ctx);
    
    emit(ctx, "  ; groupby transform with %d columns\n", node->groupby_transform.group_column_count);
    char** col_ptrs = malloc(node->groupby_transform.group_column_count * sizeof(char*));
    for (int i = 0; i < node->groupby_transform.group_column_count; i++) {
        char* col_str = register_string_literal(ctx, node->groupby_transform.group_columns[i]);
        int len = strlen(node->groupby_transform.group_columns[i]);
        col_ptrs[i] = gen_temp(ctx);
        emit(ctx, "  %s = getelementptr [%d x i8], [%d x i8]* %s, i32 0, i32 0\n",
             col_ptrs[i], len + 1, len + 1, col_str);
    }
    emit(ctx, "  %s = call i8* (i8*, i32, ...) @datalang_groupby(i8* %s, i32 %d",
         result, input_df, node->groupby_transform.group_column_count);
    for (int i = 0; i < node->groupby_transform.group_column_count; i++) {
        emit(ctx, ", i8* %s", col_ptrs[i]);
    }
    emit(ctx, ")\n");
    free(col_ptrs);
    
    return result;
}

// ==================== SELECT / GROUPBY PARA ARRAYS ====================

static char* generate_select_from_array(CodeGenContext* ctx, ASTNode* node, char* array_val, Type* array_type) {
    if (!array_type || array_type->kind != TYPE_ARRAY || !array_type->element_type) {
        return "null";
    }
    
    // Apenas suporta arrays de tipos customizados por enquanto
    if (array_type->element_type->kind != TYPE_CUSTOM) {
        return "null";
    }
    
    const char* struct_name = array_type->element_type->custom_name;
    DataTypeInfo* dt = find_data_type(struct_name);
    if (!dt) return "null";
    
    // Prepara nomes das colunas
    char** col_ptrs = malloc(node->select_transform.column_count * sizeof(char*));
    for (int i = 0; i < node->select_transform.column_count; i++) {
        char* col_str = register_string_literal(ctx, node->select_transform.columns[i]);
        int len = strlen(node->select_transform.columns[i]);
        col_ptrs[i] = gen_temp(ctx);
        emit(ctx, "  %s = getelementptr [%d x i8], [%d x i8]* %s, i32 0, i32 0\n",
             col_ptrs[i], len + 1, len + 1, col_str);
    }
    
    // Cria DataFrame
    char* df = gen_temp(ctx);
    emit(ctx, "  %s = call i8* (i32, ...) @datalang_df_create(i32 %d", df, node->select_transform.column_count);
    for (int i = 0; i < node->select_transform.column_count; i++) {
        emit(ctx, ", i8* %s", col_ptrs[i]);
    }
    emit(ctx, ")\n");
    
    // Normaliza array para {i64, i64*}
    char* arr_ptr = gen_temp(ctx);
    emit(ctx, "  %s = alloca %s\n", arr_ptr, type_to_llvm(array_type));
    emit(ctx, "  store %s %s, %s* %s\n", type_to_llvm(array_type), array_val, type_to_llvm(array_type), arr_ptr);
    char* cast_ptr = gen_temp(ctx);
    emit(ctx, "  %s = bitcast %s* %s to {i64, i64*}*\n", cast_ptr, type_to_llvm(array_type), arr_ptr);
    char* casted = gen_temp(ctx);
    emit(ctx, "  %s = load {i64, i64*}, {i64, i64*}* %s\n", casted, cast_ptr);
    
    char* size = gen_temp(ctx);
    emit(ctx, "  %s = extractvalue {i64, i64*} %s, 0\n", size, casted);
    char* data_ptr = gen_temp(ctx);
    emit(ctx, "  %s = extractvalue {i64, i64*} %s, 1\n", data_ptr, casted);
    
    // Loop
    char* loop_cond = gen_label(ctx);
    char* loop_body = gen_label(ctx);
    char* loop_end = gen_label(ctx);
    char* i_ptr = gen_temp(ctx);
    emit(ctx, "  %s = alloca i64\n", i_ptr);
    emit(ctx, "  store i64 0, i64* %s\n", i_ptr);
    emit(ctx, "  br label %%%s\n", loop_cond);
    emit(ctx, "\n%s:\n", loop_cond);
    char* i_val = gen_temp(ctx);
    emit(ctx, "  %s = load i64, i64* %s\n", i_val, i_ptr);
    char* cmp = gen_temp(ctx);
    emit(ctx, "  %s = icmp slt i64 %s, %s\n", cmp, i_val, size);
    emit(ctx, "  br i1 %s, label %%%s, label %%%s\n", cmp, loop_body, loop_end);
    
    emit(ctx, "\n%s:\n", loop_body);
    char* elem_ptr = gen_temp(ctx);
    emit(ctx, "  %s = getelementptr i64, i64* %s, i64 %s\n", elem_ptr, data_ptr, i_val);
    char* elem_raw = gen_temp(ctx);
    emit(ctx, "  %s = load i64, i64* %s\n", elem_raw, elem_ptr);
    char* elem_struct = gen_temp(ctx);
    emit(ctx, "  %s = inttoptr i64 %s to %%struct.%s*\n", elem_struct, elem_raw, struct_name);
    
    // Extrai campos selecionados
    char** value_ptrs = malloc(node->select_transform.column_count * sizeof(char*));
    for (int i = 0; i < node->select_transform.column_count; i++) {
        int field_idx = get_field_index(dt, node->select_transform.columns[i]);
        if (field_idx < 0) {
            value_ptrs[i] = register_string_literal(ctx, "null");
            continue;
        }
        // Calcula ponteiro para campo
        char* field_ptr = gen_temp(ctx);
        emit(ctx, "  %s = getelementptr %%struct.%s, %%struct.%s* %s, i32 0, i32 %d\n",
             field_ptr, struct_name, struct_name, elem_struct, field_idx);
        
        const char* ftype = dt->field_types[field_idx];
        if (strcmp(ftype, "i8*") == 0) {
            // String já é i8*
            value_ptrs[i] = gen_temp(ctx);
            emit(ctx, "  %s = load i8*, i8** %s\n", value_ptrs[i], field_ptr);
        } else if (strcmp(ftype, "double") == 0) {
            char* fval = gen_temp(ctx);
            emit(ctx, "  %s = load double, double* %s\n", fval, field_ptr);
            value_ptrs[i] = gen_temp(ctx);
            emit(ctx, "  %s = call i8* @datalang_format_float(double %s)\n", value_ptrs[i], fval);
        } else if (strcmp(ftype, "i64") == 0) {
            char* fval = gen_temp(ctx);
            emit(ctx, "  %s = load i64, i64* %s\n", fval, field_ptr);
            value_ptrs[i] = gen_temp(ctx);
            emit(ctx, "  %s = call i8* @datalang_format_int(i64 %s)\n", value_ptrs[i], fval);
        } else if (strcmp(ftype, "i1") == 0) {
            char* fval = gen_temp(ctx);
            emit(ctx, "  %s = load i1, i1* %s\n", fval, field_ptr);
            value_ptrs[i] = gen_temp(ctx);
            emit(ctx, "  %s = call i8* @datalang_format_bool(i1 %s)\n", value_ptrs[i], fval);
        } else {
            value_ptrs[i] = register_string_literal(ctx, "null");
        }
    }
    
    emit(ctx, "  call void (i8*, i32, ...) @datalang_df_add_row(i8* %s, i32 %d", df, node->select_transform.column_count);
    for (int i = 0; i < node->select_transform.column_count; i++) {
        emit(ctx, ", i8* %s", value_ptrs[i]);
    }
    emit(ctx, ")\n");
    free(value_ptrs);
    
    char* next_i = gen_temp(ctx);
    emit(ctx, "  %s = add i64 %s, 1\n", next_i, i_val);
    emit(ctx, "  store i64 %s, i64* %s\n", next_i, i_ptr);
    emit(ctx, "  br label %%%s\n", loop_cond);
    
    emit(ctx, "\n%s:\n", loop_end);
    free(col_ptrs);
    return df;
}

static char* generate_groupby_from_array(CodeGenContext* ctx, ASTNode* node, char* array_val, Type* array_type) {
    // Implementa groupby como select das colunas de agrupamento (sem agregação real)
    // Reutiliza lógica de select
    // Monta AST-like estrutura temporária
    ASTNode fake_select = {0};
    fake_select.select_transform.column_count = node->groupby_transform.group_column_count;
    fake_select.select_transform.columns = node->groupby_transform.group_columns;
    return generate_select_from_array(ctx, &fake_select, array_val, array_type);
}

static char* generate_dataframe_from_array(CodeGenContext* ctx, char* array_val, Type* array_type, DataTypeInfo* dt) {
    if (!dt || !array_type || array_type->kind != TYPE_ARRAY) return "null";
    
    // Cria cabeçalhos com todos os campos
    char** col_ptrs = malloc(dt->field_count * sizeof(char*));
    for (int i = 0; i < dt->field_count; i++) {
        char* col_str = register_string_literal(ctx, dt->field_names[i]);
        int len = strlen(dt->field_names[i]);
        col_ptrs[i] = gen_temp(ctx);
        emit(ctx, "  %s = getelementptr [%d x i8], [%d x i8]* %s, i32 0, i32 0\n",
             col_ptrs[i], len + 1, len + 1, col_str);
    }
    
    char* df = gen_temp(ctx);
    emit(ctx, "  %s = call i8* (i32, ...) @datalang_df_create(i32 %d", df, dt->field_count);
    for (int i = 0; i < dt->field_count; i++) {
        emit(ctx, ", i8* %s", col_ptrs[i]);
    }
    emit(ctx, ")\n");
    
    // Normaliza array para {i64, i64*}
    char* arr_ptr = gen_temp(ctx);
    emit(ctx, "  %s = alloca %s\n", arr_ptr, type_to_llvm(array_type));
    emit(ctx, "  store %s %s, %s* %s\n", type_to_llvm(array_type), array_val, type_to_llvm(array_type), arr_ptr);
    char* cast_ptr = gen_temp(ctx);
    emit(ctx, "  %s = bitcast %s* %s to {i64, i64*}*\n", cast_ptr, type_to_llvm(array_type), arr_ptr);
    char* casted = gen_temp(ctx);
    emit(ctx, "  %s = load {i64, i64*}, {i64, i64*}* %s\n", casted, cast_ptr);
    
    char* size = gen_temp(ctx);
    emit(ctx, "  %s = extractvalue {i64, i64*} %s, 0\n", size, casted);
    char* data_ptr = gen_temp(ctx);
    emit(ctx, "  %s = extractvalue {i64, i64*} %s, 1\n", data_ptr, casted);
    
    // Loop sobre elementos
    char* loop_cond = gen_label(ctx);
    char* loop_body = gen_label(ctx);
    char* loop_end = gen_label(ctx);
    char* i_ptr = gen_temp(ctx);
    emit(ctx, "  %s = alloca i64\n", i_ptr);
    emit(ctx, "  store i64 0, i64* %s\n", i_ptr);
    emit(ctx, "  br label %%%s\n", loop_cond);
    emit(ctx, "\n%s:\n", loop_cond);
    char* i_val = gen_temp(ctx);
    emit(ctx, "  %s = load i64, i64* %s\n", i_val, i_ptr);
    char* cmp = gen_temp(ctx);
    emit(ctx, "  %s = icmp slt i64 %s, %s\n", cmp, i_val, size);
    emit(ctx, "  br i1 %s, label %%%s, label %%%s\n", cmp, loop_body, loop_end);
    
    emit(ctx, "\n%s:\n", loop_body);
    char* elem_ptr = gen_temp(ctx);
    emit(ctx, "  %s = getelementptr i64, i64* %s, i64 %s\n", elem_ptr, data_ptr, i_val);
    char* elem_raw = gen_temp(ctx);
    emit(ctx, "  %s = load i64, i64* %s\n", elem_raw, elem_ptr);
    char* elem_struct = gen_temp(ctx);
    emit(ctx, "  %s = inttoptr i64 %s to %%struct.%s*\n", elem_struct, elem_raw, dt->name);
    
    // Extrai todos os campos
    char** value_ptrs = malloc(dt->field_count * sizeof(char*));
    for (int f = 0; f < dt->field_count; f++) {
        char* field_ptr = gen_temp(ctx);
        emit(ctx, "  %s = getelementptr %%struct.%s, %%struct.%s* %s, i32 0, i32 %d\n",
             field_ptr, dt->name, dt->name, elem_struct, f);
        
        const char* ftype = dt->field_types[f];
        if (strcmp(ftype, "i8*") == 0) {
            value_ptrs[f] = gen_temp(ctx);
            emit(ctx, "  %s = load i8*, i8** %s\n", value_ptrs[f], field_ptr);
        } else if (strcmp(ftype, "double") == 0) {
            char* fval = gen_temp(ctx);
            emit(ctx, "  %s = load double, double* %s\n", fval, field_ptr);
            value_ptrs[f] = gen_temp(ctx);
            emit(ctx, "  %s = call i8* @datalang_format_float(double %s)\n", value_ptrs[f], fval);
        } else if (strcmp(ftype, "i64") == 0) {
            char* fval = gen_temp(ctx);
            emit(ctx, "  %s = load i64, i64* %s\n", fval, field_ptr);
            value_ptrs[f] = gen_temp(ctx);
            emit(ctx, "  %s = call i8* @datalang_format_int(i64 %s)\n", value_ptrs[f], fval);
        } else if (strcmp(ftype, "i1") == 0) {
            char* fval = gen_temp(ctx);
            emit(ctx, "  %s = load i1, i1* %s\n", fval, field_ptr);
            value_ptrs[f] = gen_temp(ctx);
            emit(ctx, "  %s = call i8* @datalang_format_bool(i1 %s)\n", value_ptrs[f], fval);
        } else {
            value_ptrs[f] = register_string_literal(ctx, "null");
        }
    }
    
    emit(ctx, "  call void (i8*, i32, ...) @datalang_df_add_row(i8* %s, i32 %d", df, dt->field_count);
    for (int f = 0; f < dt->field_count; f++) {
        emit(ctx, ", i8* %s", value_ptrs[f]);
    }
    emit(ctx, ")\n");
    free(value_ptrs);
    
    char* next_i = gen_temp(ctx);
    emit(ctx, "  %s = add i64 %s, 1\n", next_i, i_val);
    emit(ctx, "  store i64 %s, i64* %s\n", next_i, i_ptr);
    emit(ctx, "  br label %%%s\n", loop_cond);
    
    emit(ctx, "\n%s:\n", loop_end);
    free(col_ptrs);
    return df;
}

static char* generate_pipeline_expr(CodeGenContext* ctx, ASTNode* node) {
    if (node->pipeline_expr.stage_count == 0) return "0";
    
    // First stage: base expression
    char* current = generate_expr(ctx, node->pipeline_expr.stages[0]);
    Type* current_type = analyze_expression(ctx->analyzer, node->pipeline_expr.stages[0]);
    bool current_is_generic_custom = false;
    
    // Apply each subsequent transformation
    for (int i = 1; i < node->pipeline_expr.stage_count; i++) {
        ASTNode* stage = node->pipeline_expr.stages[i];
        Type* stage_type = current_type;

        switch (stage->type) {
            case AST_FILTER_TRANSFORM: {
                // Normalize custom type arrays to {i64, i64*} before filtering
                if (stage_type && stage_type->kind == TYPE_DATAFRAME) {
                    char* col = NULL; int op = 0; double thr = 0.0;
                    if (extract_df_filter_info(stage->filter_transform.filter_predicate, &col, &op, &thr)) {
                        char* col_str = register_string_literal(ctx, col);
                        int len = strlen(col);
                        char* col_ptr = gen_temp(ctx);
                        emit(ctx, "  %s = getelementptr [%d x i8], [%d x i8]* %s, i32 0, i32 0\n",
                             col_ptr, len + 1, len + 1, col_str);
                        char* filtered = gen_temp(ctx);
                        emit(ctx, "  %s = call i8* @datalang_df_filter_numeric(i8* %s, i8* %s, i32 %d, double %f)\n",
                             filtered, current, col_ptr, op, thr);
                        current = filtered;
                    } else {
                        char* lit = NULL;
                        if (extract_df_filter_string(stage->filter_transform.filter_predicate, &col, &lit, &op)) {
                            char* col_str = register_string_literal(ctx, col);
                            int len = strlen(col);
                            char* col_ptr = gen_temp(ctx);
                            emit(ctx, "  %s = getelementptr [%d x i8], [%d x i8]* %s, i32 0, i32 0\n",
                                 col_ptr, len + 1, len + 1, col_str);
                            char* lit_str = register_string_literal(ctx, lit ? lit : "");
                            int llen = strlen(lit ? lit : "");
                            char* lit_ptr = gen_temp(ctx);
                            emit(ctx, "  %s = getelementptr [%d x i8], [%d x i8]* %s, i32 0, i32 0\n",
                                 lit_ptr, llen + 1, llen + 1, lit_str);
                            char* filtered = gen_temp(ctx);
                            emit(ctx, "  %s = call i8* @datalang_df_filter_string(i8* %s, i8* %s, i8* %s, i32 %d)\n",
                                 filtered, current, col_ptr, lit_ptr, op);
                            current = filtered;
                        }
                    }
                } else {
                    char* normalized = normalize_array_for_transform(ctx, current, stage_type);
                    current = generate_filter_transform(ctx, stage, normalized);
                    current_is_generic_custom = (stage_type && stage_type->kind == TYPE_ARRAY &&
                        stage_type->element_type && stage_type->element_type->kind == TYPE_CUSTOM);
                }
                // current_type permanece o mesmo (DataFrame ou array)
                break;
            }
            case AST_MAP_TRANSFORM: {
                // Normalize custom type arrays before mapping
                if (stage_type && stage_type->kind == TYPE_ARRAY) {
                    bool is_custom_elem = stage_type->element_type && stage_type->element_type->kind == TYPE_CUSTOM;
                    char* array_for_map = current;
                    if (is_custom_elem) {
                        if (!current_is_generic_custom) {
                            array_for_map = normalize_array_for_transform(ctx, current, stage_type);
                            current_is_generic_custom = true;
                        }
                    } else {
                        array_for_map = normalize_array_for_transform(ctx, current, stage_type);
                    }

                    current = generate_map_transform(ctx, stage, array_for_map);
                    current_type = analyze_expression(ctx->analyzer, stage);
                    current_is_generic_custom = (current_type && current_type->kind == TYPE_ARRAY &&
                        current_type->element_type && current_type->element_type->kind == TYPE_CUSTOM);
                } else if (stage_type && stage_type->kind == TYPE_DATAFRAME) {
                    char* col = NULL; double scale = 1.0, add = 0.0;
                    if (extract_df_map_info(stage->map_transform.map_function, &col, &scale, &add)) {
                        char* col_str = register_string_literal(ctx, col);
                        int len = strlen(col);
                        char* col_ptr = gen_temp(ctx);
                        emit(ctx, "  %s = getelementptr [%d x i8], [%d x i8]* %s, i32 0, i32 0\n",
                             col_ptr, len + 1, len + 1, col_str);
                        char* arr = gen_temp(ctx);
                        emit(ctx, "  %s = call {i64, double*} @datalang_df_column_double(i8* %s, i8* %s, double %f, double %f)\n",
                             arr, current, col_ptr, scale, add);
                        current = arr;
                        current_type = create_array_type(create_primitive_type(TYPE_FLOAT));
                        current_is_generic_custom = false;
                    }
                } else {
                    current = current;
                }
                break;
            }
            case AST_REDUCE_TRANSFORM: {
                // Normalize custom type arrays before reducing
                if (stage_type && stage_type->kind == TYPE_ARRAY) {
                    bool is_custom_elem = stage_type->element_type && stage_type->element_type->kind == TYPE_CUSTOM;
                    char* array_for_reduce = current;
                    if (is_custom_elem) {
                        if (!current_is_generic_custom) {
                            array_for_reduce = normalize_array_for_transform(ctx, current, stage_type);
                            current_is_generic_custom = true;
                        }
                    } else {
                        array_for_reduce = normalize_array_for_transform(ctx, current, stage_type);
                    }

                    current = generate_reduce_transform(ctx, stage, array_for_reduce, stage_type);
                    current_type = analyze_expression(ctx->analyzer, stage);
                    current_is_generic_custom = false; // reduce retorna escalar
                } else if (stage_type && stage_type->kind == TYPE_DATAFRAME) {
                    char* rows = gen_temp(ctx);
                    emit(ctx, "  %s = call i64 @datalang_df_count(i8* %s)\n", rows, current);
                    Type* target_type = stage->reduce_transform.initial_value ?
                        analyze_expression(ctx->analyzer, stage->reduce_transform.initial_value) :
                        create_primitive_type(TYPE_INT);
                    if (target_type && target_type->kind == TYPE_FLOAT) {
                        char* rows_f = gen_temp(ctx);
                        emit(ctx, "  %s = sitofp i64 %s to double\n", rows_f, rows);
                        current = rows_f;
                    } else {
                        current = rows;
                    }
                    current_type = target_type;
                } else {
                    // Fallback: apenas avalia valor inicial
                    if (stage->reduce_transform.initial_value) {
                        current = generate_expr(ctx, stage->reduce_transform.initial_value);
                        current_type = analyze_expression(ctx->analyzer, stage->reduce_transform.initial_value);
                    }
                }
                break;
            }
            case AST_SELECT_TRANSFORM: {
                if (stage_type && stage_type->kind == TYPE_ARRAY) {
                    current = generate_select_from_array(ctx, stage, current, stage_type);
                    current_type = create_primitive_type(TYPE_DATAFRAME);
                } else {
                    char* input_df = (stage_type && stage_type->kind == TYPE_DATAFRAME) ? current : "null";
                    current = generate_select_transform(ctx, stage, input_df);
                    current_type = create_primitive_type(TYPE_DATAFRAME);
                }
                current_is_generic_custom = false;
                break;
            }
            case AST_GROUPBY_TRANSFORM: {
                if (stage_type && stage_type->kind == TYPE_ARRAY) {
                    current = generate_groupby_from_array(ctx, stage, current, stage_type);
                    current_type = create_primitive_type(TYPE_DATAFRAME);
                } else {
                    char* input_df = (stage_type && stage_type->kind == TYPE_DATAFRAME) ? current : "null";
                    current = generate_groupby_transform(ctx, stage, input_df);
                    current_type = create_primitive_type(TYPE_DATAFRAME);
                }
                current_is_generic_custom = false;
                break;
            }
            case AST_AGGREGATE_TRANSFORM: {
                // Apply aggregation to current array
                const char* func_name = NULL;
                const char* ret_type = "i64";
                switch (stage->aggregate_transform.agg_type) {
                    case AGG_SUM: func_name = "sum"; break;
                    case AGG_MIN: func_name = "min"; break;
                    case AGG_MAX: func_name = "max"; break;
                    case AGG_COUNT: func_name = "count"; break;
                    case AGG_MEAN: func_name = "mean"; ret_type = "double"; break;
                    default: func_name = "sum"; break;
                }
                // Normalize array to {i64, i64*} before aggregation
                Type* stage_type_prev = current_type;
                char* normalized = current;
                bool is_custom_elem = stage_type_prev && stage_type_prev->kind == TYPE_ARRAY &&
                    stage_type_prev->element_type && stage_type_prev->element_type->kind == TYPE_CUSTOM;
                if (!(is_custom_elem && current_is_generic_custom)) {
                    normalized = normalize_array_to_i64(ctx, current, stage_type_prev);
                }
                char* result = gen_temp(ctx);
                emit(ctx, "  %s = call %s @%s({i64, i64*} %s)\n",
                     result, ret_type, func_name, normalized);
                current = result;
                current_type = (ret_type && strcmp(ret_type, "double") == 0)
                    ? create_primitive_type(TYPE_FLOAT)
                    : create_primitive_type(TYPE_INT);
                current_is_generic_custom = false;
                break;
            }
            case AST_CALL_EXPR: {
                // Check if it's an aggregate function call with no arguments in pipeline
                if (stage->call_expr.callee &&
                    stage->call_expr.callee->type == AST_IDENTIFIER &&
                    is_builtin_aggregate(stage->call_expr.callee->identifier.id_name) &&
                    stage->call_expr.arg_count == 0) {
                    // It's an aggregate in pipeline - pass current value as argument
                    const char* func_name = stage->call_expr.callee->identifier.id_name;
                    Type* stage_type = analyze_expression(ctx->analyzer, node->pipeline_expr.stages[i-1]);

                    // Check if we're working with Float arrays
                    bool is_float_array = (stage_type && stage_type->kind == TYPE_ARRAY &&
                                           stage_type->element_type &&
                                           stage_type->element_type->kind == TYPE_FLOAT);
                    bool is_custom_array = (stage_type && stage_type->kind == TYPE_ARRAY &&
                                            stage_type->element_type &&
                                            stage_type->element_type->kind == TYPE_CUSTOM);

                    const char* actual_func_name = func_name;
                    const char* ret_type = "i64";
                    const char* array_sig = "{i64, i64*}";
                    char* array_to_pass = current;

                    if (is_float_array && strcmp(func_name, "count") != 0) {
                        // Use Float-specific aggregate functions (except count)
                        char* float_func = malloc(128);
                        snprintf(float_func, 128, "%s_float", func_name);
                        actual_func_name = float_func;
                        ret_type = "double";
                        array_sig = "{i64, double*}";
                        // Don't normalize Float arrays - keep as {i64, double*}
                    } else {
                        // For Int/custom type arrays (or count on any array), normalize to {i64, i64*}
                        // Special handling for count() on Float arrays
                        if (strcmp(func_name, "count") == 0 && is_float_array) {
                            // Convert {i64, double*} to {i64, i64*}
                            char* size = gen_temp(ctx);
                            char* data = gen_temp(ctx);
                            char* data_as_i64 = gen_temp(ctx);
                            char* temp1 = gen_temp(ctx);
                            char* temp2 = gen_temp(ctx);
                            emit(ctx, "  %s = extractvalue {i64, double*} %s, 0\n", size, current);
                            emit(ctx, "  %s = extractvalue {i64, double*} %s, 1\n", data, current);
                            emit(ctx, "  %s = bitcast double* %s to i64*\n", data_as_i64, data);
                            emit(ctx, "  %s = insertvalue {i64, i64*} undef, i64 %s, 0\n", temp1, size);
                            emit(ctx, "  %s = insertvalue {i64, i64*} %s, i64* %s, 1\n", temp2, temp1, data_as_i64);
                            array_to_pass = temp2;
                        } else {
                            if (is_custom_array && current_is_generic_custom) {
                                array_to_pass = current;
                            } else {
                                array_to_pass = normalize_array_to_i64(ctx, current, stage_type);
                            }
                        }
                        if (strcmp(func_name, "mean") == 0) {
                            ret_type = "double";
                        }
                    }

                    char* result = gen_temp(ctx);
                    emit(ctx, "  %s = call %s @%s(%s %s)\n",
                         result, ret_type, actual_func_name, array_sig, array_to_pass);
                    current = result;
                    current_is_generic_custom = false;
                } else {
                    // Regular call expression
                    current = generate_expr(ctx, stage);
                }
                break;
            }
            default:
                // Generic expression
                current = generate_expr(ctx, stage);
                break;
        }
    }

    // Check if we need to convert back from {i64, i64*} to the original element type
    // This happens when the pipeline result should be an array of custom types
    Type* final_type = analyze_expression(ctx->analyzer, node);
    if (final_type && final_type->kind == TYPE_ARRAY && final_type->element_type &&
        final_type->element_type->kind == TYPE_CUSTOM) {
        // Need to convert {i64, i64*} back to {i64, %struct.Foo**}
        const char* elem_llvm_type = type_to_llvm(final_type->element_type);

        // Extract size and data from the i64* array
        char* size_val = gen_temp(ctx);
        emit(ctx, "  %s = extractvalue {i64, i64*} %s, 0\n", size_val, current);
        char* data_val = gen_temp(ctx);
        emit(ctx, "  %s = extractvalue {i64, i64*} %s, 1\n", data_val, current);

        // Bitcast back to the correct type
        char* bitcast_data = gen_temp(ctx);
        emit(ctx, "  %s = bitcast i64* %s to %s*\n", bitcast_data, data_val, elem_llvm_type);

        // Reconstruct as {i64, %struct.Foo**}
        char* final_struct = gen_temp(ctx);
        emit(ctx, "  %s = alloca {i64, %s*}\n", final_struct, elem_llvm_type);

        char* final_size_ptr = gen_temp(ctx);
        emit(ctx, "  %s = getelementptr {i64, %s*}, {i64, %s*}* %s, i32 0, i32 0\n",
             final_size_ptr, elem_llvm_type, elem_llvm_type, final_struct);
        emit(ctx, "  store i64 %s, i64* %s\n", size_val, final_size_ptr);

        char* final_data_ptr = gen_temp(ctx);
        emit(ctx, "  %s = getelementptr {i64, %s*}, {i64, %s*}* %s, i32 0, i32 1\n",
             final_data_ptr, elem_llvm_type, elem_llvm_type, final_struct);
        emit(ctx, "  store %s* %s, %s** %s\n", elem_llvm_type, bitcast_data, elem_llvm_type, final_data_ptr);

        char* result = gen_temp(ctx);
        emit(ctx, "  %s = load {i64, %s*}, {i64, %s*}* %s\n",
             result, elem_llvm_type, elem_llvm_type, final_struct);
        return result;
    }

    return current;
}

// ==================== DATA TYPES - MEMBER ACCESS ====================

static char* generate_member_expr(CodeGenContext* ctx, ASTNode* node) {
    Type* object_type = analyze_expression(ctx->analyzer, node->member_expr.object);
    
    if (object_type->kind != TYPE_CUSTOM) {
        if (node->member_expr.object->type == AST_IDENTIFIER) {
            Symbol* sym = lookup_symbol(ctx->analyzer->symbol_table, 
                                       node->member_expr.object->identifier.id_name);
            if (sym && sym->type && sym->type->kind == TYPE_CUSTOM) {
                object_type = sym->type;
            }
        }
        
        if (object_type->kind != TYPE_CUSTOM) {
            char* fallback = generate_expr(ctx, node->member_expr.object);
            return fallback;
        }
    }

    if (object_type->kind == TYPE_CUSTOM && strcmp(object_type->custom_name, "Row") == 0) {
        // Row é dinâmica; usa heurísticas simples para tipos comuns
        const char* field = node->member_expr.member;
        if (strcmp(field, "idade") == 0 || strcmp(field, "id") == 0) {
            return "0";
        }
        if (strcmp(field, "salario") == 0 || strcmp(field, "valor") == 0) {
            return "0.0";
        }
        return "null";
    }
    
    DataTypeInfo* dt = find_data_type(object_type->custom_name);
    if (!dt) {
        fprintf(stderr, "Erro: Tipo '%s' não encontrado\n", object_type->custom_name);
        return "0";
    }
    
    int field_idx = get_field_index(dt, node->member_expr.member);
    if (field_idx < 0) {
        fprintf(stderr, "Erro: Campo '%s' não encontrado no tipo '%s'\n", 
                node->member_expr.member, object_type->custom_name);
        return "0";
    }
    
    char* object_ptr;
    
    if (node->member_expr.object->type == AST_IDENTIFIER) {
        // Variável: pegar o ponteiro armazenado
        const char* var_name = node->member_expr.object->identifier.id_name;
        const char* var_mapping = get_var_llvm_name(ctx, var_name);

        if (!var_mapping) {
            fprintf(stderr, "Erro: Variável '%s' não encontrada\n", var_name);
            return "0";
        }
        
        object_ptr = gen_temp(ctx);
        emit(ctx, "  %s = load %%struct.%s*, %%struct.%s** %s\n",
             object_ptr, object_type->custom_name, object_type->custom_name, var_mapping);
    } else {
        // Expressão: já retorna o ponteiro
        object_ptr = generate_expr(ctx, node->member_expr.object);
    }
    
    char* field_ptr = gen_temp(ctx);
    emit(ctx, "  %s = getelementptr %%struct.%s, %%struct.%s* %s, i32 0, i32 %d\n",
         field_ptr, object_type->custom_name, object_type->custom_name, object_ptr, field_idx);
    
    char* result = gen_temp(ctx);
    const char* field_llvm_type = dt->field_types[field_idx];
    emit(ctx, "  %s = load %s, %s* %s\n",
         result, field_llvm_type, field_llvm_type, field_ptr);
    
    return result;
}

static char* generate_cstring_value(CodeGenContext* ctx, const char* value) {
    if (!value) value = "";

    char* result = gen_temp(ctx);
    char* str_global = register_string_literal(ctx, value);
    int len = (int)strlen(value);

    emit(ctx,
        "  %s = getelementptr [%d x i8], [%d x i8]* %s, i32 0, i32 0\n",
        result, len + 1, len + 1, str_global);

    return result;
}

static char* generate_load_expr(CodeGenContext* ctx, ASTNode* node) {
    // Converte string literal para C-string
    char* path_val = generate_cstring_value(ctx, node->load_expr.file_path);

    char* result = gen_temp(ctx);
    emit(ctx, "  %s = call i8* @datalang_load(i8* %s)\n", result, path_val);

    return result;
}

static char* generate_save_expr(CodeGenContext* ctx, ASTNode* node) {
    // Gera expressão de dados
    Type* data_type = analyze_expression(ctx->analyzer, node->save_expr.data);
    char* data_val = generate_expr(ctx, node->save_expr.data);

    // Se for array de tipo customizado, converte em DataFrame antes de salvar
    if (data_type && data_type->kind == TYPE_ARRAY &&
        data_type->element_type && data_type->element_type->kind == TYPE_CUSTOM) {
        DataTypeInfo* dt = find_data_type(data_type->element_type->custom_name);
        if (dt) {
            data_val = generate_dataframe_from_array(ctx, data_val, data_type, dt);
            data_type = create_primitive_type(TYPE_DATAFRAME);
        }
    }

    if (!data_type || data_type->kind != TYPE_DATAFRAME) {
        data_val = "null";
    }

    // Converte caminho para C-string
    char* path_val = generate_cstring_value(ctx, node->save_expr.save_path);

    // save() retorna void, mas usamos como statement
    emit(ctx, "  call void @datalang_save(i8* %s, i8* %s)\n", data_val, path_val);

    return "0";  // Retorna dummy value
}


static char* generate_assign_expr(CodeGenContext* ctx, ASTNode* node) {
    // Gera valor a ser atribuído
    char* value = generate_expr(ctx, node->assign_expr.value);
    
    // Encontra o alvo
    ASTNode* target = node->assign_expr.target;
    
    if (target->type == AST_IDENTIFIER) {
        char* var_ptr = get_var_llvm_name(ctx, target->identifier.id_name);
        if (var_ptr) {
            Symbol* symbol = lookup_symbol(ctx->analyzer->symbol_table, target->identifier.id_name);
            const char* type_str = type_to_llvm(symbol ? symbol->type : create_primitive_type(TYPE_INT));
            emit(ctx, "  store %s %s, %s* %s\n", type_str, value, type_str, var_ptr);
        }
    }
    // TODO: Handle index and member assignment
    
    return value;
}

static char* generate_lambda_expr(CodeGenContext* ctx, ASTNode* node) {
    (void)ctx;
    (void)node;
    return "null";
}

static int lambda_counter = 0;

static char* generate_lambda_as_value(CodeGenContext* ctx, ASTNode* node) {
    // Gera uma função LLVM anônima para a lambda
    char func_name[64];
    snprintf(func_name, 64, "lambda_%d", lambda_counter++);
    
    // Determina tipo de retorno
    Type* ret_type = create_primitive_type(TYPE_INT);
    if (node->lambda_expr.lambda_body) {
        ret_type = analyze_expression(ctx->analyzer, node->lambda_expr.lambda_body);
        if (!ret_type) ret_type = create_primitive_type(TYPE_INT);
    }
    
    // Emite definição da função
    emit(ctx, "\ndefine %s @%s(", type_to_llvm(ret_type), func_name);
    
    // Parâmetros
    for (int i = 0; i < node->lambda_expr.lambda_param_count; i++) {
        if (i > 0) emit(ctx, ", ");
        ASTNode* param = node->lambda_expr.lambda_params[i];
        Type* param_type = ast_type_to_type(ctx->analyzer, param->param.param_type);
        emit(ctx, "%s %%p%d", type_to_llvm(param_type), i);
    }
    emit(ctx, ") {\nentry:\n");
    
    // Corpo da função
    enter_scope(ctx->analyzer->symbol_table);
    
    for (int i = 0; i < node->lambda_expr.lambda_param_count; i++) {
        ASTNode* param = node->lambda_expr.lambda_params[i];
        char* param_name = param->param.param_name;
        Type* param_type = ast_type_to_type(ctx->analyzer, param->param.param_type);
        
        declare_symbol(ctx->analyzer->symbol_table, param_name, SYMBOL_PARAMETER, param_type, 0, 0);
        
        char* param_ptr = gen_temp(ctx);
        emit(ctx, "  %s = alloca %s\n", param_ptr, type_to_llvm(param_type));
        emit(ctx, "  store %s %%p%d, %s* %s\n", 
             type_to_llvm(param_type), i, type_to_llvm(param_type), param_ptr);
        add_var_mapping(ctx, param_name, param_ptr);
    }
    
    char* result = generate_expr(ctx, node->lambda_expr.lambda_body);
    emit(ctx, "  ret %s %s\n}\n", type_to_llvm(ret_type), result);
    
    exit_scope(ctx->analyzer->symbol_table);
    
    // Retorna ponteiro para a função
    char* func_ptr = malloc(128);
    snprintf(func_ptr, 128, "@%s", func_name);
    return func_ptr;
}

static char* generate_expr(CodeGenContext* ctx, ASTNode* node) {
    if (!node) return "0";
    
    switch (node->type) {
        case AST_LITERAL: return generate_literal(ctx, node);
        case AST_IDENTIFIER: return generate_identifier(ctx, node);
        case AST_BINARY_EXPR: return generate_binary_expr(ctx, node);
        case AST_UNARY_EXPR: return generate_unary_expr(ctx, node);
        case AST_ARRAY_LITERAL: return generate_array_literal(ctx, node);
        case AST_CALL_EXPR: return generate_call_expr(ctx, node);
        case AST_RANGE_EXPR: return generate_range_expr(ctx, node);
        case AST_AGGREGATE_TRANSFORM: return generate_aggregate_transform(ctx, node);
        case AST_LOAD_EXPR: return generate_load_expr(ctx, node);
        case AST_SAVE_EXPR: return generate_save_expr(ctx, node);
        case AST_PIPELINE_EXPR: return generate_pipeline_expr(ctx, node);
        case AST_INDEX_EXPR: return generate_index_expr(ctx, node);
        case AST_MEMBER_EXPR: return generate_member_expr(ctx, node);
        case AST_ASSIGN_EXPR: return generate_assign_expr(ctx, node);
        case AST_LAMBDA_EXPR: return generate_lambda_expr(ctx, node);
        default: return "0";
    }
}

static __attribute__((unused)) char* ensure_bool(CodeGenContext* ctx, char* value, Type* from_type) {
    if (!from_type || from_type->kind == TYPE_BOOL) return value;

    char* result = gen_temp(ctx);

    if (from_type->kind == TYPE_INT) {
        emit(ctx, "  %s = icmp ne i64 %s, 0\n", result, value);
    } else if (from_type->kind == TYPE_FLOAT) {
        emit(ctx, "  %s = fcmp one double %s, 0.0\n", result, value);
    } else {
        // pointer, custom, string, array, DataFrame → not null
        const char* llvm_ty = type_to_llvm(from_type);
        emit(ctx, "  %s = icmp ne %s %s, null\n", result, llvm_ty, value);
    }

    return result;
}

static char* cast_value_if_needed(CodeGenContext* ctx,
                                  char* value,
                                  Type* from_type,
                                  Type* to_type) {
    if (!from_type || !to_type) return value;

    if (from_type->kind == to_type->kind) {
        return value;
    }

    if (from_type->kind == TYPE_INT && to_type->kind == TYPE_FLOAT) {
        char* casted = gen_temp(ctx);
        emit(ctx, "  %s = sitofp i64 %s to double\n", casted, value);
        return casted;
    }

    return value;
}

// ==================== STATEMENTS ====================

static void generate_let_decl(CodeGenContext* ctx, ASTNode* node) {
    Type* init_type = NULL;
    if (node->let_decl.initializer) {
        init_type = analyze_expression(ctx->analyzer, node->let_decl.initializer);

        // Fix type for Float aggregate functions (except count)
        // The semantic analyzer thinks sum/mean/etc return Int, but sum_float/mean_float return Float
        if (node->let_decl.initializer->type == AST_CALL_EXPR &&
            node->let_decl.initializer->call_expr.callee &&
            node->let_decl.initializer->call_expr.callee->type == AST_IDENTIFIER &&
            is_builtin_aggregate(node->let_decl.initializer->call_expr.callee->identifier.id_name) &&
            node->let_decl.initializer->call_expr.arg_count > 0) {

            const char* func_name = node->let_decl.initializer->call_expr.callee->identifier.id_name;

            // Check if argument is Float array (and function is not count)
            if (strcmp(func_name, "count") != 0) {
                Type* arg_type = analyze_expression(ctx->analyzer, node->let_decl.initializer->call_expr.arguments[0]);
                if (arg_type && arg_type->kind == TYPE_ARRAY &&
                    arg_type->element_type && arg_type->element_type->kind == TYPE_FLOAT) {
                    // This will actually call sum_float/mean_float/etc which return double
                    if (strcmp(func_name, "sum") == 0 || strcmp(func_name, "mean") == 0 ||
                        strcmp(func_name, "min") == 0 || strcmp(func_name, "max") == 0) {
                        init_type = create_primitive_type(TYPE_FLOAT);
                    }
                }
            }
        }
    }

    Type* var_type = NULL;
    if (node->let_decl.type_annotation) {
        var_type = ast_type_to_type(ctx->analyzer, node->let_decl.type_annotation);
    } else if (init_type) {
        var_type = init_type;
    } else {
        var_type = create_primitive_type(TYPE_INT);
    }

    if (node->let_decl.initializer && node->let_decl.initializer->type == AST_LAMBDA_EXPR) {
        char* lambda_func = generate_lambda_as_value(ctx, node->let_decl.initializer);
        
        // Armazena ponteiro de função
        char* var_ptr = gen_temp(ctx);
        emit(ctx, "  %s = alloca i8*\n", var_ptr);
        
        // Converte ponteiro de função para i8*
        char* func_as_ptr = gen_temp(ctx);
        emit(ctx, "  %s = bitcast i8* %s to i8*\n", func_as_ptr, lambda_func);
        emit(ctx, "  store i8* %s, i8** %s\n", func_as_ptr, var_ptr);
        
        add_var_mapping(ctx, node->let_decl.name, var_ptr);
        
        int param_count = node->let_decl.initializer->lambda_expr.lambda_param_count;
        Type** param_types = malloc(param_count * sizeof(Type*));
        for (int i = 0; i < param_count; i++) {
            param_types[i] = ast_type_to_type(ctx->analyzer, 
                node->let_decl.initializer->lambda_expr.lambda_params[i]->param.param_type);
        }
        
        Type* func_type = create_function_type(param_types, param_count, var_type);
        Symbol* sym = declare_symbol(ctx->analyzer->symbol_table, node->let_decl.name,
                                     SYMBOL_FUNCTION, func_type, 0, 0);
        sym->initialized = true;
        sym->is_lambda_variable = true; // Nova flag
        strncpy(sym->lambda_func_name, lambda_func, 127);
        
        return;
    }

    if (var_type->kind == TYPE_ERROR) {
        var_type = create_primitive_type(TYPE_INT);
    }

    if (!is_symbol_in_current_scope(ctx->analyzer->symbol_table, node->let_decl.name)) {
        declare_symbol(ctx->analyzer->symbol_table,
                       node->let_decl.name,
                       SYMBOL_VARIABLE,
                       var_type,
                       0, 0);
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

        if (init_value && init_type) {
            char* final_value = cast_value_if_needed(ctx, init_value, init_type, var_type);
            emit(ctx, "  store %s %s, %s* %s\n",
                 type_str, final_value, type_str, var_ptr);
        }
    }
    
    Symbol* sym = lookup_symbol(ctx->analyzer->symbol_table, node->let_decl.name);
    if (sym) sym->initialized = true;
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

    Type* elem_type = (iterable_type && iterable_type->kind == TYPE_ARRAY)
                        ? iterable_type->element_type
                        : create_primitive_type(TYPE_INT);

    // Detecta se é tipo customizado
    bool is_custom = (elem_type->kind == TYPE_CUSTOM);

    const char* elem_llvm_type = type_to_llvm(elem_type);
    char elem_type_str[256];
    if (is_custom) {
        snprintf(elem_type_str, sizeof(elem_type_str), "%%struct.%s*", elem_type->custom_name);
    } else {
        strncpy(elem_type_str, type_to_llvm(elem_type), 255);
        elem_type_str[255] = '\0';
    }

    declare_symbol(ctx->analyzer->symbol_table,
                   node->for_stmt.iterator,
                   SYMBOL_VARIABLE,
                   elem_type,
                   node->line, node->column);

    // Extract size and data using the ACTUAL array element type
    char* size = gen_temp(ctx);
    emit(ctx,
         "  %s = extractvalue {i64, %s*} %s, 0\n",
         size, elem_llvm_type, array_struct);

    char* data_ptr = gen_temp(ctx);
    emit(ctx,
         "  %s = extractvalue {i64, %s*} %s, 1\n",
         data_ptr, elem_llvm_type, array_struct);

    char* i_ptr = gen_temp(ctx);
    emit(ctx, "  %s = alloca i64\n", i_ptr);
    emit(ctx, "  store i64 0, i64* %s\n", i_ptr);

    // Aloca variável do iterador com tipo correto
    char* iter_ptr = gen_temp(ctx);
    emit(ctx, "  %s = alloca %s\n", iter_ptr, elem_type_str);
    add_var_mapping(ctx, node->for_stmt.iterator, iter_ptr);

    char* loop_cond = gen_label(ctx);
    char* loop_body = gen_label(ctx);
    char* loop_end  = gen_label(ctx);

    emit(ctx, "  br label %%%s\n", loop_cond);
    emit(ctx, "\n%s:\n", loop_cond);

    char* i_val = gen_temp(ctx);
    emit(ctx, "  %s = load i64, i64* %s\n", i_val, i_ptr);

    char* cmp = gen_temp(ctx);
    emit(ctx, "  %s = icmp slt i64 %s, %s\n", cmp, i_val, size);
    emit(ctx, "  br i1 %s, label %%%s, label %%%s\n", cmp, loop_body, loop_end);

    emit(ctx, "\n%s:\n", loop_body);

    // Load element from array (using actual element type)
    char* elem_ptr = gen_temp(ctx);
    emit(ctx,
         "  %s = getelementptr %s, %s* %s, i64 %s\n",
         elem_ptr, elem_llvm_type, elem_llvm_type, data_ptr, i_val);

    char* elem_val = gen_temp(ctx);
    emit(ctx,
         "  %s = load %s, %s* %s\n",
         elem_val, elem_llvm_type, elem_llvm_type, elem_ptr);

    // Store directly to iterator variable (no conversion needed - already correct type)
    emit(ctx,
         "  store %s %s, %s* %s\n",
         elem_type_str, elem_val, elem_type_str, iter_ptr);

    // Marca como inicializado
    Symbol* iter_sym = lookup_symbol(ctx->analyzer->symbol_table, node->for_stmt.iterator);
    if (iter_sym) iter_sym->initialized = true;

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

// PRINT COM MÚLTIPLOS ARGUMENTOS

static void generate_print_stmt(CodeGenContext* ctx, ASTNode* node) {
    // Itera sobre todas as expressões
    for (int i = 0; i < node->print_stmt.expr_count; i++) {
        ASTNode* expr = node->print_stmt.expressions[i];

        char* val = generate_expr(ctx, expr);
        Type* type = analyze_expression(ctx->analyzer, expr);

        if (!type) continue;

        // Fix type for Float aggregate functions (except count)
        // The semantic analyzer thinks sum/mean/etc return Int, but sum_float/mean_float return Float
        if (expr->type == AST_CALL_EXPR &&
            expr->call_expr.callee &&
            expr->call_expr.callee->type == AST_IDENTIFIER &&
            is_builtin_aggregate(expr->call_expr.callee->identifier.id_name) &&
            expr->call_expr.arg_count > 0) {

            const char* func_name = expr->call_expr.callee->identifier.id_name;

            // Check if argument is Float array (and function is not count)
            if (strcmp(func_name, "count") != 0) {
                Type* arg_type = analyze_expression(ctx->analyzer, expr->call_expr.arguments[0]);
                if (arg_type && arg_type->kind == TYPE_ARRAY &&
                    arg_type->element_type && arg_type->element_type->kind == TYPE_FLOAT) {
                    // This will actually call sum_float/mean_float/etc which return double
                    if (strcmp(func_name, "sum") == 0 || strcmp(func_name, "mean") == 0 ||
                        strcmp(func_name, "min") == 0 || strcmp(func_name, "max") == 0) {
                        type = create_primitive_type(TYPE_FLOAT);
                    }
                }
            }
        }

        if (type->kind == TYPE_CUSTOM) {
            // Para tipos customizados, imprime representação genérica
            char buffer[256];
            snprintf(buffer, 256, "<%s>", type->custom_name);
            char* type_str = generate_cstring_value(ctx, buffer);
            
            if (i < node->print_stmt.expr_count - 1) {
                emit(ctx, "  call void @print_string_no_nl(i8* %s)\n", type_str);
                char* space = generate_cstring_value(ctx, " ");
                emit(ctx, "  call void @print_string_no_nl(i8* %s)\n", space);
            } else {
                emit(ctx, "  call void @print_string(i8* %s)\n", type_str);
            }
            continue; // Pula para próxima expressão
        }

        // Determina função de print apropriada
        if (type->kind == TYPE_ARRAY && type->element_type) {
            switch (type->element_type->kind) {
                case TYPE_INT:
                    emit(ctx, "  call void @print_int_array({i64, i64*} %s)\n", val);
                    break;
                case TYPE_FLOAT:
                    emit(ctx, "  call void @print_float_array({i64, double*} %s)\n", val);
                    break;
                case TYPE_BOOL:
                    emit(ctx, "  call void @print_bool_array({i64, i1*} %s)\n", val);
                    break;
                case TYPE_STRING:
                    emit(ctx, "  call void @print_string_array({i64, i8**} %s)\n", val);
                    break;
                default:
                    break;
            }
        }
        else if (type->kind == TYPE_STRING) {
            // Se não for o último, usa versão sem newline
            if (i < node->print_stmt.expr_count - 1) {
                emit(ctx, "  call void @print_string_no_nl(i8* %s)\n", val);
                // Adiciona espaço entre argumentos
                char* space = generate_cstring_value(ctx, " ");
                emit(ctx, "  call void @print_string_no_nl(i8* %s)\n", space);
            } else {
                emit(ctx, "  call void @print_string(i8* %s)\n", val);
            }
        }
        else if (type->kind == TYPE_FLOAT) {
            if (i < node->print_stmt.expr_count - 1) {
                emit(ctx, "  call void @print_float_no_nl(double %s)\n", val);
                char* space = generate_cstring_value(ctx, " ");
                emit(ctx, "  call void @print_string_no_nl(i8* %s)\n", space);
            } else {
                emit(ctx, "  call void @print_float(double %s)\n", val);
            }
        }
        else if (type->kind == TYPE_BOOL) {
            if (i < node->print_stmt.expr_count - 1) {
                emit(ctx, "  call void @print_bool_no_nl(i1 %s)\n", val);
                char* space = generate_cstring_value(ctx, " ");
                emit(ctx, "  call void @print_string_no_nl(i8* %s)\n", space);
            } else {
                emit(ctx, "  call void @print_bool(i1 %s)\n", val);
            }
        }
        else if (type->kind == TYPE_DATAFRAME) {
            emit(ctx, "  call void @datalang_print_dataframe(i8* %s)\n", val);
        }
        else {
            // TYPE_INT ou default
            if (i < node->print_stmt.expr_count - 1) {
                emit(ctx, "  call void @print_int_no_nl(i64 %s)\n", val);
                char* space = generate_cstring_value(ctx, " ");
                emit(ctx, "  call void @print_string_no_nl(i8* %s)\n", space);
            } else {
                emit(ctx, "  call void @print_int(i64 %s)\n", val);
            }
        }
    }
    
    // Se não há expressões, apenas imprime newline
    if (node->print_stmt.expr_count == 0) {
        emit(ctx, "  call void @print_newline()\n");
    }
}

static void generate_stmt(CodeGenContext* ctx, ASTNode* node) {
    if (!node) return;
    switch (node->type) {
        case AST_LET_DECL: generate_let_decl(ctx, node); break;
        case AST_IF_STMT: generate_if_stmt(ctx, node); break;
        case AST_FOR_STMT: generate_for_stmt(ctx, node); break;
        case AST_PRINT_STMT: generate_print_stmt(ctx, node); break;
        case AST_BLOCK: generate_block(ctx, node); break;
        
        case AST_RETURN_STMT: {
            if (node->return_stmt.value) {
                Type* expected_type = ctx->analyzer->current_function_return_type;
                if (!expected_type) {
                    expected_type = create_primitive_type(TYPE_VOID);
                }

                Type* expr_type = analyze_expression(ctx->analyzer,
                                                    node->return_stmt.value);
                char* val = generate_expr(ctx, node->return_stmt.value);

                if (expected_type->kind == TYPE_VOID) {
                    emit(ctx, "  ret void\n");
                } else {
                    char* final_val = cast_value_if_needed(ctx, val, expr_type, expected_type);
                    emit(ctx, "  ret %s %s\n", type_to_llvm(expected_type), final_val);
                }
            } else {
                emit(ctx, "  ret void\n");
            }
            break;
        }
        case AST_EXPR_STMT: generate_expr(ctx, node->expr_stmt.expression); break;
        default: break;
    }
}

// DATA DECLARATIONS

static void generate_data_decl(CodeGenContext* ctx, ASTNode* node) {
    // Coleta informações dos campos
    char** field_names = malloc(node->data_decl.field_count * sizeof(char*));
    char** field_types = malloc(node->data_decl.field_count * sizeof(char*));
    
    emit(ctx, "\n; Data type: %s\n", node->data_decl.name);
    emit(ctx, "%%struct.%s = type { ", node->data_decl.name);
    
    for (int i = 0; i < node->data_decl.field_count; i++) {
        if (i > 0) emit(ctx, ", ");
        Type* field_type = ast_type_to_type(ctx->analyzer, 
                                            node->data_decl.fields[i]->field_decl.field_type);
        const char* llvm_type = type_to_llvm(field_type);
        emit(ctx, "%s", llvm_type);
        
        field_names[i] = strdup(node->data_decl.fields[i]->field_decl.field_name);
        field_types[i] = strdup(llvm_type);
    }
    emit(ctx, " }\n");
    
    // Registra o tipo para uso posterior
    register_data_type(node->data_decl.name, field_names, field_types, node->data_decl.field_count);
    
    // Não precisa liberar field_names e field_types aqui pois register_data_type faz cópias
}

static void generate_function(CodeGenContext* ctx, ASTNode* node) {
    int saved_var_count = ctx->var_map.count; // preserve globals
    ctx->in_function = true;
    ctx->current_function = node->fn_decl.name;
    
    Type* return_type = node->fn_decl.return_type ?
        ast_type_to_type(ctx->analyzer, node->fn_decl.return_type) :
        create_primitive_type(TYPE_VOID);

    Type* prev_ret_type = ctx->analyzer->current_function_return_type;
    ctx->analyzer->current_function_return_type = return_type;

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

    // Inicializa globais antes da lógica do main do usuário
    if (strcmp(node->fn_decl.name, "main") == 0) {
        emit(ctx, "  call void @__init_globals()\n");
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
        else if (ret_type->kind == TYPE_STRING || ret_type->kind == TYPE_DATAFRAME || ret_type->kind == TYPE_CUSTOM) {
            emit(ctx, "  ret %s null\n", ret_type_str);
        } else {
            emit(ctx, "  ret %s 0\n", ret_type_str);
        }
    }
    emit(ctx, "}\n");
    
    exit_scope(ctx->analyzer->symbol_table);
    ctx->analyzer->current_function_return_type = prev_ret_type;
    ctx->in_function = false;
    ctx->var_map.count = saved_var_count; // drop local/param mappings
}

static void generate_main_function(CodeGenContext* ctx, ASTNode* program) {
    int saved_var_count = ctx->var_map.count;
    emit(ctx, "define i64 @user_main() {\nentry:\n");

    // Garante que variáveis globais com inicializadores complexos sejam avaliadas
    emit(ctx, "  call void @__init_globals()\n");
    
    for (int i = 0; i < program->program.decl_count; i++) {
        ASTNode* decl = program->program.declarations[i];
        // Top-level lets já foram materializados como globais; não recriar aqui
        if (decl->type == AST_LET_DECL) continue;
        else if (decl->type == AST_PRINT_STMT) generate_stmt(ctx, decl);
        else if (decl->type == AST_FOR_STMT) generate_for_stmt(ctx, decl);
        else if (decl->type == AST_IF_STMT) generate_if_stmt(ctx, decl);
        else if (decl->type == AST_EXPR_STMT) generate_stmt(ctx, decl);
    }

    emit(ctx, "  ret i64 0\n}\n");
    ctx->var_map.count = saved_var_count;
}

// Gera uma função especial que avalia inicializadores de variáveis globais
static void generate_global_initializers_fn(CodeGenContext* ctx, ASTNode* program) {
    int saved_var_count = ctx->var_map.count;
    bool prev_in_function = ctx->in_function;
    ctx->in_function = true;
    ctx->current_function = "__init_globals";

    emit(ctx, "\n; Inicialização de variáveis globais\n");
    emit(ctx, "define void @__init_globals() {\nentry:\n");

    for (int i = 0; i < program->program.decl_count; i++) {
        ASTNode* decl = program->program.declarations[i];
        if (!decl || decl->type != AST_LET_DECL) continue;
        if (!decl->let_decl.initializer) continue;

        // Evita recursão infinita: não chama main() durante a inicialização global
        if (decl->let_decl.initializer->type == AST_CALL_EXPR &&
            decl->let_decl.initializer->call_expr.callee &&
            decl->let_decl.initializer->call_expr.callee->type == AST_IDENTIFIER &&
            strcmp(decl->let_decl.initializer->call_expr.callee->identifier.id_name, "main") == 0) {
            continue;
        }

        char* global_ptr = get_var_llvm_name(ctx, decl->let_decl.name);
        if (!global_ptr) continue;

        Type* var_type = decl->let_decl.type_annotation ?
            ast_type_to_type(ctx->analyzer, decl->let_decl.type_annotation) :
            analyze_expression(ctx->analyzer, decl->let_decl.initializer);
        const char* llvm_type = type_to_llvm(var_type);

        char* init_val = generate_expr(ctx, decl->let_decl.initializer);
        emit(ctx, "  store %s %s, %s* %s\n", llvm_type, init_val, llvm_type, global_ptr);
    }

    emit(ctx, "  ret void\n}\n");
    ctx->var_map.count = saved_var_count;
    ctx->in_function = prev_in_function;
}

void emit_runtime_functions(CodeGenContext* ctx) {
    emit(ctx, "; ==================== RUNTIME FUNCTIONS ====================\n\n");
    emit(ctx, "declare i32 @printf(i8*, ...)\n");
    emit(ctx, "declare i8* @malloc(i64)\n");
    emit(ctx, "declare void @free(i8*)\n");
    emit(ctx, "declare i32 @strcmp(i8*, i8*)\n\n");

    emit(ctx, "@.fmt.int = private constant [6 x i8] c\"%%lld\\0A\\00\"\n");
    emit(ctx, "@.fmt.float = private constant [7 x i8] c\"%%0.6f\\0A\\00\"\n");
    emit(ctx, "@.fmt.str = private constant [4 x i8] c\"%%s\\0A\\00\"\n");
    emit(ctx, "@.fmt.true = private constant [6 x i8] c\"true\\0A\\00\"\n");
    emit(ctx, "@.fmt.false = private constant [7 x i8] c\"false\\0A\\00\"\n\n");

    // Formatos sem newline para print com múltiplos argumentos
    emit(ctx, "@.fmt.int_no_nl = private constant [5 x i8] c\"%%lld\\00\"\n");
    emit(ctx, "@.fmt.float_no_nl = private constant [6 x i8] c\"%%0.6f\\00\"\n");
    emit(ctx, "@.fmt.str_no_nl = private constant [3 x i8] c\"%%s\\00\"\n");
    emit(ctx, "@.fmt.newline = private constant [2 x i8] c\"\\0A\\00\"\n");
    emit(ctx, "@.fmt.space = private constant [2 x i8] c\" \\00\"\n");
    emit(ctx, "@.fmt.lbracket = private constant [2 x i8] c\"[\\00\"\n");
    emit(ctx, "@.fmt.rbracket_nl = private constant [3 x i8] c\"]\\0A\\00\"\n");
    emit(ctx, "@.fmt.comma_space = private constant [3 x i8] c\", \\00\"\n");
    emit(ctx, "@.fmt.bool_word_true = private constant [5 x i8] c\"true\\00\"\n");
    emit(ctx, "@.fmt.bool_word_false = private constant [6 x i8] c\"false\\00\"\n\n");

    // DataFrame operations
    emit(ctx, "; DataFrame operations\n");
    emit(ctx, "declare i8* @datalang_select(i8*, i32, ...)\n");
    emit(ctx, "declare i8* @datalang_groupby(i8*, i32, ...)\n");
    emit(ctx, "declare i64 @datalang_df_count(i8*)\n");
    emit(ctx, "declare i8* @datalang_df_filter_numeric(i8*, i8*, i32, double)\n");
    emit(ctx, "declare i8* @datalang_df_filter_string(i8*, i8*, i8*, i32)\n");
    emit(ctx, "declare {i64, double*} @datalang_df_column_double(i8*, i8*, double, double)\n");
    emit(ctx, "declare i8* @datalang_load(i8*)\n");
    emit(ctx, "declare void @datalang_save(i8*, i8*)\n");
    emit(ctx, "declare void @datalang_print_dataframe(i8*)\n");
    emit(ctx, "declare i8* @datalang_df_create(i32, ...)\n");
    emit(ctx, "declare void @datalang_df_add_row(i8*, i32, ...)\n");
    emit(ctx, "declare i8* @datalang_format_int(i64)\n");
    emit(ctx, "declare i8* @datalang_format_float(double)\n");
    emit(ctx, "declare i8* @datalang_format_bool(i1)\n");

    // Print functions com newline
    emit(ctx, "define void @print_int(i64 %%val) {\n");
    emit(ctx, "  %%fmt = getelementptr [6 x i8], [6 x i8]* @.fmt.int, i32 0, i32 0\n");
    emit(ctx, "  call i32 (i8*, ...) @printf(i8* %%fmt, i64 %%val)\n");
    emit(ctx, "  ret void\n");
    emit(ctx, "}\n\n");
    
    emit(ctx, "define void @print_float(double %%val) {\n");
    emit(ctx, "  %%fmt = getelementptr [7 x i8], [7 x i8]* @.fmt.float, i32 0, i32 0\n");
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

    // Print functions SEM newline (para múltiplos argumentos)
    emit(ctx, "define void @print_int_no_nl(i64 %%val) {\n");
    emit(ctx, "  %%fmt = getelementptr [5 x i8], [5 x i8]* @.fmt.int_no_nl, i32 0, i32 0\n");
    emit(ctx, "  call i32 (i8*, ...) @printf(i8* %%fmt, i64 %%val)\n");
    emit(ctx, "  ret void\n");
    emit(ctx, "}\n\n");
    
    emit(ctx, "define void @print_float_no_nl(double %%val) {\n");
    emit(ctx, "  %%fmt = getelementptr [6 x i8], [6 x i8]* @.fmt.float_no_nl, i32 0, i32 0\n");
    emit(ctx, "  call i32 (i8*, ...) @printf(i8* %%fmt, double %%val)\n");
    emit(ctx, "  ret void\n");
    emit(ctx, "}\n\n");
    
    emit(ctx, "define void @print_string_no_nl(i8* %%val) {\n");
    emit(ctx, "  %%fmt = getelementptr [3 x i8], [3 x i8]* @.fmt.str_no_nl, i32 0, i32 0\n");
    emit(ctx, "  call i32 (i8*, ...) @printf(i8* %%fmt, i8* %%val)\n");
    emit(ctx, "  ret void\n");
    emit(ctx, "}\n\n");
    
    emit(ctx, "define void @print_bool_no_nl(i1 %%val) {\n");
    emit(ctx, "  br i1 %%val, label %%is_true, label %%is_false\n");
    emit(ctx, "is_true:\n");
    emit(ctx, "  %%fmt_t = getelementptr [5 x i8], [5 x i8]* @.fmt.bool_word_true, i32 0, i32 0\n");
    emit(ctx, "  call i32 (i8*, ...) @printf(i8* %%fmt_t)\n");
    emit(ctx, "  ret void\n");
    emit(ctx, "is_false:\n");
    emit(ctx, "  %%fmt_f = getelementptr [6 x i8], [6 x i8]* @.fmt.bool_word_false, i32 0, i32 0\n");
    emit(ctx, "  call i32 (i8*, ...) @printf(i8* %%fmt_f)\n");
    emit(ctx, "  ret void\n");
    emit(ctx, "}\n\n");
    
    emit(ctx, "define void @print_newline() {\n");
    emit(ctx, "  %%fmt = getelementptr [2 x i8], [2 x i8]* @.fmt.newline, i32 0, i32 0\n");
    emit(ctx, "  call i32 (i8*, ...) @printf(i8* %%fmt)\n");
    emit(ctx, "  ret void\n");
    emit(ctx, "}\n\n");

    // Print arrays - implementações completas
    emit(ctx, "; print_int_array: imprime [1, 2, 3] em uma única linha\n");
    emit(ctx, "define void @print_int_array({i64, i64*} %%array) {\n");
    emit(ctx, "entry:\n");
    emit(ctx, "  %%size = extractvalue {i64, i64*} %%array, 0\n");
    emit(ctx, "  %%data = extractvalue {i64, i64*} %%array, 1\n");
    emit(ctx, "  %%fmt_lb = getelementptr [2 x i8], [2 x i8]* @.fmt.lbracket, i32 0, i32 0\n");
    emit(ctx, "  call i32 (i8*, ...) @printf(i8* %%fmt_lb)\n");
    emit(ctx, "  %%i = alloca i64\n");
    emit(ctx, "  store i64 0, i64* %%i\n");
    emit(ctx, "  br label %%loop_cond\n");
    emit(ctx, "loop_cond:\n");
    emit(ctx, "  %%i_val = load i64, i64* %%i\n");
    emit(ctx, "  %%cmp = icmp slt i64 %%i_val, %%size\n");
    emit(ctx, "  br i1 %%cmp, label %%loop_body, label %%loop_end\n");
    emit(ctx, "loop_body:\n");
    emit(ctx, "  %%ptr = getelementptr i64, i64* %%data, i64 %%i_val\n");
    emit(ctx, "  %%val = load i64, i64* %%ptr\n");
    emit(ctx, "  %%fmt_elem = getelementptr [5 x i8], [5 x i8]* @.fmt.int_no_nl, i32 0, i32 0\n");
    emit(ctx, "  call i32 (i8*, ...) @printf(i8* %%fmt_elem, i64 %%val)\n");
    emit(ctx, "  %%i_next = add i64 %%i_val, 1\n");
    emit(ctx, "  store i64 %%i_next, i64* %%i\n");
    emit(ctx, "  %%has_next = icmp slt i64 %%i_next, %%size\n");
    emit(ctx, "  br i1 %%has_next, label %%print_sep, label %%loop_cond\n");
    emit(ctx, "print_sep:\n");
    emit(ctx, "  %%fmt_sep = getelementptr [3 x i8], [3 x i8]* @.fmt.comma_space, i32 0, i32 0\n");
    emit(ctx, "  call i32 (i8*, ...) @printf(i8* %%fmt_sep)\n");
    emit(ctx, "  br label %%loop_cond\n");
    emit(ctx, "loop_end:\n");
    emit(ctx, "  %%fmt_rb = getelementptr [3 x i8], [3 x i8]* @.fmt.rbracket_nl, i32 0, i32 0\n");
    emit(ctx, "  call i32 (i8*, ...) @printf(i8* %%fmt_rb)\n");
    emit(ctx, "  ret void\n");
    emit(ctx, "}\n\n");

    emit(ctx, "define void @print_float_array({i64, double*} %%array) {\n");
    emit(ctx, "entry:\n");
    emit(ctx, "  %%size = extractvalue {i64, double*} %%array, 0\n");
    emit(ctx, "  %%data = extractvalue {i64, double*} %%array, 1\n");
    emit(ctx, "  %%fmt_lb = getelementptr [2 x i8], [2 x i8]* @.fmt.lbracket, i32 0, i32 0\n");
    emit(ctx, "  call i32 (i8*, ...) @printf(i8* %%fmt_lb)\n");
    emit(ctx, "  %%i = alloca i64\n");
    emit(ctx, "  store i64 0, i64* %%i\n");
    emit(ctx, "  br label %%loop_cond\n");
    emit(ctx, "loop_cond:\n");
    emit(ctx, "  %%i_val = load i64, i64* %%i\n");
    emit(ctx, "  %%cmp = icmp slt i64 %%i_val, %%size\n");
    emit(ctx, "  br i1 %%cmp, label %%loop_body, label %%loop_end\n");
    emit(ctx, "loop_body:\n");
    emit(ctx, "  %%ptr = getelementptr double, double* %%data, i64 %%i_val\n");
    emit(ctx, "  %%val = load double, double* %%ptr\n");
    emit(ctx, "  %%fmt_elem = getelementptr [6 x i8], [6 x i8]* @.fmt.float_no_nl, i32 0, i32 0\n");
    emit(ctx, "  call i32 (i8*, ...) @printf(i8* %%fmt_elem, double %%val)\n");
    emit(ctx, "  %%i_next = add i64 %%i_val, 1\n");
    emit(ctx, "  store i64 %%i_next, i64* %%i\n");
    emit(ctx, "  %%has_next = icmp slt i64 %%i_next, %%size\n");
    emit(ctx, "  br i1 %%has_next, label %%print_sep, label %%loop_cond\n");
    emit(ctx, "print_sep:\n");
    emit(ctx, "  %%fmt_sep = getelementptr [3 x i8], [3 x i8]* @.fmt.comma_space, i32 0, i32 0\n");
    emit(ctx, "  call i32 (i8*, ...) @printf(i8* %%fmt_sep)\n");
    emit(ctx, "  br label %%loop_cond\n");
    emit(ctx, "loop_end:\n");
    emit(ctx, "  %%fmt_rb = getelementptr [3 x i8], [3 x i8]* @.fmt.rbracket_nl, i32 0, i32 0\n");
    emit(ctx, "  call i32 (i8*, ...) @printf(i8* %%fmt_rb)\n");
    emit(ctx, "  ret void\n");
    emit(ctx, "}\n\n");

    emit(ctx, "define void @print_bool_array({i64, i1*} %%array) {\n");
    emit(ctx, "entry:\n");
    emit(ctx, "  %%size = extractvalue {i64, i1*} %%array, 0\n");
    emit(ctx, "  %%data = extractvalue {i64, i1*} %%array, 1\n");
    emit(ctx, "  %%fmt_lb = getelementptr [2 x i8], [2 x i8]* @.fmt.lbracket, i32 0, i32 0\n");
    emit(ctx, "  call i32 (i8*, ...) @printf(i8* %%fmt_lb)\n");
    emit(ctx, "  %%i = alloca i64\n");
    emit(ctx, "  store i64 0, i64* %%i\n");
    emit(ctx, "  br label %%loop_cond\n");
    emit(ctx, "loop_cond:\n");
    emit(ctx, "  %%i_val = load i64, i64* %%i\n");
    emit(ctx, "  %%cmp = icmp slt i64 %%i_val, %%size\n");
    emit(ctx, "  br i1 %%cmp, label %%loop_body, label %%loop_end\n");
    emit(ctx, "loop_body:\n");
    emit(ctx, "  %%ptr = getelementptr i1, i1* %%data, i64 %%i_val\n");
    emit(ctx, "  %%val = load i1, i1* %%ptr\n");
    emit(ctx, "  br i1 %%val, label %%print_true, label %%print_false\n");
    emit(ctx, "print_true:\n");
    emit(ctx, "  %%fmt_t = getelementptr [5 x i8], [5 x i8]* @.fmt.bool_word_true, i32 0, i32 0\n");
    emit(ctx, "  call i32 (i8*, ...) @printf(i8* %%fmt_t)\n");
    emit(ctx, "  br label %%after_print\n");
    emit(ctx, "print_false:\n");
    emit(ctx, "  %%fmt_f = getelementptr [6 x i8], [6 x i8]* @.fmt.bool_word_false, i32 0, i32 0\n");
    emit(ctx, "  call i32 (i8*, ...) @printf(i8* %%fmt_f)\n");
    emit(ctx, "  br label %%after_print\n");
    emit(ctx, "after_print:\n");
    emit(ctx, "  %%i_val2 = load i64, i64* %%i\n");
    emit(ctx, "  %%i_next = add i64 %%i_val2, 1\n");
    emit(ctx, "  store i64 %%i_next, i64* %%i\n");
    emit(ctx, "  %%has_next = icmp slt i64 %%i_next, %%size\n");
    emit(ctx, "  br i1 %%has_next, label %%print_sep, label %%loop_cond\n");
    emit(ctx, "print_sep:\n");
    emit(ctx, "  %%fmt_sep = getelementptr [3 x i8], [3 x i8]* @.fmt.comma_space, i32 0, i32 0\n");
    emit(ctx, "  call i32 (i8*, ...) @printf(i8* %%fmt_sep)\n");
    emit(ctx, "  br label %%loop_cond\n");
    emit(ctx, "loop_end:\n");
    emit(ctx, "  %%fmt_rb = getelementptr [3 x i8], [3 x i8]* @.fmt.rbracket_nl, i32 0, i32 0\n");
    emit(ctx, "  call i32 (i8*, ...) @printf(i8* %%fmt_rb)\n");
    emit(ctx, "  ret void\n");
    emit(ctx, "}\n\n");

    emit(ctx, "define void @print_string_array({i64, i8**} %%array) {\n");
    emit(ctx, "entry:\n");
    emit(ctx, "  %%size = extractvalue {i64, i8**} %%array, 0\n");
    emit(ctx, "  %%data = extractvalue {i64, i8**} %%array, 1\n");
    emit(ctx, "  %%fmt_lb = getelementptr [2 x i8], [2 x i8]* @.fmt.lbracket, i32 0, i32 0\n");
    emit(ctx, "  call i32 (i8*, ...) @printf(i8* %%fmt_lb)\n");
    emit(ctx, "  %%i = alloca i64\n");
    emit(ctx, "  store i64 0, i64* %%i\n");
    emit(ctx, "  br label %%loop_cond\n");
    emit(ctx, "loop_cond:\n");
    emit(ctx, "  %%i_val = load i64, i64* %%i\n");
    emit(ctx, "  %%cmp = icmp slt i64 %%i_val, %%size\n");
    emit(ctx, "  br i1 %%cmp, label %%loop_body, label %%loop_end\n");
    emit(ctx, "loop_body:\n");
    emit(ctx, "  %%ptr = getelementptr i8*, i8** %%data, i64 %%i_val\n");
    emit(ctx, "  %%val = load i8*, i8** %%ptr\n");
    emit(ctx, "  %%fmt_elem = getelementptr [3 x i8], [3 x i8]* @.fmt.str_no_nl, i32 0, i32 0\n");
    emit(ctx, "  call i32 (i8*, ...) @printf(i8* %%fmt_elem, i8* %%val)\n");
    emit(ctx, "  %%i_next = add i64 %%i_val, 1\n");
    emit(ctx, "  store i64 %%i_next, i64* %%i\n");
    emit(ctx, "  %%has_next = icmp slt i64 %%i_next, %%size\n");
    emit(ctx, "  br i1 %%has_next, label %%print_sep, label %%loop_cond\n");
    emit(ctx, "print_sep:\n");
    emit(ctx, "  %%fmt_sep = getelementptr [3 x i8], [3 x i8]* @.fmt.comma_space, i32 0, i32 0\n");
    emit(ctx, "  call i32 (i8*, ...) @printf(i8* %%fmt_sep)\n");
    emit(ctx, "  br label %%loop_cond\n");
    emit(ctx, "loop_end:\n");
    emit(ctx, "  %%fmt_rb = getelementptr [3 x i8], [3 x i8]* @.fmt.rbracket_nl, i32 0, i32 0\n");
    emit(ctx, "  call i32 (i8*, ...) @printf(i8* %%fmt_rb)\n");
    emit(ctx, "  ret void\n");
    emit(ctx, "}\n\n");
    
    // Aggregate functions
    emit(ctx, "; sum: sums all elements in an integer array\n");
    emit(ctx, "define i64 @sum({i64, i64*} %%array) {\n");
    emit(ctx, "entry:\n");
    emit(ctx, "  %%size = extractvalue {i64, i64*} %%array, 0\n");
    emit(ctx, "  %%data = extractvalue {i64, i64*} %%array, 1\n");
    emit(ctx, "  %%size_zero = icmp eq i64 %%size, 0\n");
    emit(ctx, "  br i1 %%size_zero, label %%return_zero, label %%loop_cond\n");
    emit(ctx, "return_zero:\n");
    emit(ctx, "  ret i64 0\n");
    emit(ctx, "loop_cond:\n");
    emit(ctx, "  %%i = phi i64 [0, %%entry], [%%i_next, %%loop_body]\n");
    emit(ctx, "  %%acc = phi i64 [0, %%entry], [%%acc_next, %%loop_body]\n");
    emit(ctx, "  %%cmp = icmp slt i64 %%i, %%size\n");
    emit(ctx, "  br i1 %%cmp, label %%loop_body, label %%loop_end\n");
    emit(ctx, "loop_body:\n");
    emit(ctx, "  %%ptr = getelementptr i64, i64* %%data, i64 %%i\n");
    emit(ctx, "  %%val = load i64, i64* %%ptr\n");
    emit(ctx, "  %%acc_next = add i64 %%acc, %%val\n");
    emit(ctx, "  %%i_next = add i64 %%i, 1\n");
    emit(ctx, "  br label %%loop_cond\n");
    emit(ctx, "loop_end:\n");
    emit(ctx, "  ret i64 %%acc\n");
    emit(ctx, "}\n\n");
    
    emit(ctx, "; mean: calculates the average of an integer array, returns double\n");
    emit(ctx, "define double @mean({i64, i64*} %%array) {\n");
    emit(ctx, "entry:\n");
    emit(ctx, "  %%size = extractvalue {i64, i64*} %%array, 0\n");
    emit(ctx, "  %%data = extractvalue {i64, i64*} %%array, 1\n");
    emit(ctx, "  %%size_zero = icmp eq i64 %%size, 0\n");
    emit(ctx, "  br i1 %%size_zero, label %%return_zero, label %%loop_cond\n");
    emit(ctx, "return_zero:\n");
    emit(ctx, "  ret double 0.0\n");
    emit(ctx, "loop_cond:\n");
    emit(ctx, "  %%i = phi i64 [0, %%entry], [%%i_next, %%loop_body]\n");
    emit(ctx, "  %%acc = phi i64 [0, %%entry], [%%acc_next, %%loop_body]\n");
    emit(ctx, "  %%cmp = icmp slt i64 %%i, %%size\n");
    emit(ctx, "  br i1 %%cmp, label %%loop_body, label %%loop_end\n");
    emit(ctx, "loop_body:\n");
    emit(ctx, "  %%ptr = getelementptr i64, i64* %%data, i64 %%i\n");
    emit(ctx, "  %%val = load i64, i64* %%ptr\n");
    emit(ctx, "  %%acc_next = add i64 %%acc, %%val\n");
    emit(ctx, "  %%i_next = add i64 %%i, 1\n");
    emit(ctx, "  br label %%loop_cond\n");
    emit(ctx, "loop_end:\n");
    emit(ctx, "  %%sum_f = sitofp i64 %%acc to double\n");
    emit(ctx, "  %%size_f = sitofp i64 %%size to double\n");
    emit(ctx, "  %%result = fdiv double %%sum_f, %%size_f\n");
    emit(ctx, "  ret double %%result\n");
    emit(ctx, "}\n\n");
    
    emit(ctx, "; count: returns the number of elements in an array\n");
    emit(ctx, "define i64 @count({i64, i64*} %%array) {\n");
    emit(ctx, "  %%size = extractvalue {i64, i64*} %%array, 0\n");
    emit(ctx, "  ret i64 %%size\n");
    emit(ctx, "}\n\n");
    
    emit(ctx, "; min: finds the minimum element in an integer array\n");
    emit(ctx, "define i64 @min({i64, i64*} %%array) {\n");
    emit(ctx, "entry:\n");
    emit(ctx, "  %%size = extractvalue {i64, i64*} %%array, 0\n");
    emit(ctx, "  %%data = extractvalue {i64, i64*} %%array, 1\n");
    emit(ctx, "  %%size_zero = icmp eq i64 %%size, 0\n");
    emit(ctx, "  br i1 %%size_zero, label %%return_zero, label %%init\n");
    emit(ctx, "return_zero:\n");
    emit(ctx, "  ret i64 0\n");
    emit(ctx, "init:\n");
    emit(ctx, "  %%first_ptr = getelementptr i64, i64* %%data, i64 0\n");
    emit(ctx, "  %%first_val = load i64, i64* %%first_ptr\n");
    emit(ctx, "  br label %%loop_cond\n");
    emit(ctx, "loop_cond:\n");
    emit(ctx, "  %%i = phi i64 [1, %%init], [%%i_next, %%loop_body]\n");
    emit(ctx, "  %%min_val = phi i64 [%%first_val, %%init], [%%new_min, %%loop_body]\n");
    emit(ctx, "  %%cmp = icmp slt i64 %%i, %%size\n");
    emit(ctx, "  br i1 %%cmp, label %%loop_body, label %%loop_end\n");
    emit(ctx, "loop_body:\n");
    emit(ctx, "  %%ptr = getelementptr i64, i64* %%data, i64 %%i\n");
    emit(ctx, "  %%val = load i64, i64* %%ptr\n");
    emit(ctx, "  %%is_less = icmp slt i64 %%val, %%min_val\n");
    emit(ctx, "  %%new_min = select i1 %%is_less, i64 %%val, i64 %%min_val\n");
    emit(ctx, "  %%i_next = add i64 %%i, 1\n");
    emit(ctx, "  br label %%loop_cond\n");
    emit(ctx, "loop_end:\n");
    emit(ctx, "  ret i64 %%min_val\n");
    emit(ctx, "}\n\n");

    emit(ctx, "; max: finds the maximum element in an integer array\n");
    emit(ctx, "define i64 @max({i64, i64*} %%array) {\n");
    emit(ctx, "entry:\n");
    emit(ctx, "  %%size = extractvalue {i64, i64*} %%array, 0\n");
    emit(ctx, "  %%data = extractvalue {i64, i64*} %%array, 1\n");
    emit(ctx, "  %%size_zero = icmp eq i64 %%size, 0\n");
    emit(ctx, "  br i1 %%size_zero, label %%return_zero, label %%init\n");
    emit(ctx, "return_zero:\n");
    emit(ctx, "  ret i64 0\n");
    emit(ctx, "init:\n");
    emit(ctx, "  %%first_ptr = getelementptr i64, i64* %%data, i64 0\n");
    emit(ctx, "  %%first_val = load i64, i64* %%first_ptr\n");
    emit(ctx, "  br label %%loop_cond\n");
    emit(ctx, "loop_cond:\n");
    emit(ctx, "  %%i = phi i64 [1, %%init], [%%i_next, %%loop_body]\n");
    emit(ctx, "  %%max_val = phi i64 [%%first_val, %%init], [%%new_max, %%loop_body]\n");
    emit(ctx, "  %%cmp = icmp slt i64 %%i, %%size\n");
    emit(ctx, "  br i1 %%cmp, label %%loop_body, label %%loop_end\n");
    emit(ctx, "loop_body:\n");
    emit(ctx, "  %%ptr = getelementptr i64, i64* %%data, i64 %%i\n");
    emit(ctx, "  %%val = load i64, i64* %%ptr\n");
    emit(ctx, "  %%is_greater = icmp sgt i64 %%val, %%max_val\n");
    emit(ctx, "  %%new_max = select i1 %%is_greater, i64 %%val, i64 %%max_val\n");
    emit(ctx, "  %%i_next = add i64 %%i, 1\n");
    emit(ctx, "  br label %%loop_cond\n");
    emit(ctx, "loop_end:\n");
    emit(ctx, "  ret i64 %%max_val\n");
    emit(ctx, "}\n\n");

    // ==================== FLOAT ARRAY AGGREGATES ====================

    emit(ctx, "; sum_float: sums all elements in a float array\n");
    emit(ctx, "define double @sum_float({i64, double*} %%array) {\n");
    emit(ctx, "entry:\n");
    emit(ctx, "  %%size = extractvalue {i64, double*} %%array, 0\n");
    emit(ctx, "  %%data = extractvalue {i64, double*} %%array, 1\n");
    emit(ctx, "  %%size_zero = icmp eq i64 %%size, 0\n");
    emit(ctx, "  br i1 %%size_zero, label %%return_zero, label %%loop_cond\n");
    emit(ctx, "return_zero:\n");
    emit(ctx, "  ret double 0.0\n");
    emit(ctx, "loop_cond:\n");
    emit(ctx, "  %%i = phi i64 [0, %%entry], [%%i_next, %%loop_body]\n");
    emit(ctx, "  %%acc = phi double [0.0, %%entry], [%%acc_next, %%loop_body]\n");
    emit(ctx, "  %%cmp = icmp slt i64 %%i, %%size\n");
    emit(ctx, "  br i1 %%cmp, label %%loop_body, label %%loop_end\n");
    emit(ctx, "loop_body:\n");
    emit(ctx, "  %%ptr = getelementptr double, double* %%data, i64 %%i\n");
    emit(ctx, "  %%val = load double, double* %%ptr\n");
    emit(ctx, "  %%acc_next = fadd double %%acc, %%val\n");
    emit(ctx, "  %%i_next = add i64 %%i, 1\n");
    emit(ctx, "  br label %%loop_cond\n");
    emit(ctx, "loop_end:\n");
    emit(ctx, "  ret double %%acc\n");
    emit(ctx, "}\n\n");

    emit(ctx, "; mean_float: calculates the average of a float array\n");
    emit(ctx, "define double @mean_float({i64, double*} %%array) {\n");
    emit(ctx, "entry:\n");
    emit(ctx, "  %%size = extractvalue {i64, double*} %%array, 0\n");
    emit(ctx, "  %%data = extractvalue {i64, double*} %%array, 1\n");
    emit(ctx, "  %%size_zero = icmp eq i64 %%size, 0\n");
    emit(ctx, "  br i1 %%size_zero, label %%return_zero, label %%loop_cond\n");
    emit(ctx, "return_zero:\n");
    emit(ctx, "  ret double 0.0\n");
    emit(ctx, "loop_cond:\n");
    emit(ctx, "  %%i = phi i64 [0, %%entry], [%%i_next, %%loop_body]\n");
    emit(ctx, "  %%acc = phi double [0.0, %%entry], [%%acc_next, %%loop_body]\n");
    emit(ctx, "  %%cmp = icmp slt i64 %%i, %%size\n");
    emit(ctx, "  br i1 %%cmp, label %%loop_body, label %%loop_end\n");
    emit(ctx, "loop_body:\n");
    emit(ctx, "  %%ptr = getelementptr double, double* %%data, i64 %%i\n");
    emit(ctx, "  %%val = load double, double* %%ptr\n");
    emit(ctx, "  %%acc_next = fadd double %%acc, %%val\n");
    emit(ctx, "  %%i_next = add i64 %%i, 1\n");
    emit(ctx, "  br label %%loop_cond\n");
    emit(ctx, "loop_end:\n");
    emit(ctx, "  %%size_f = sitofp i64 %%size to double\n");
    emit(ctx, "  %%result = fdiv double %%acc, %%size_f\n");
    emit(ctx, "  ret double %%result\n");
    emit(ctx, "}\n\n");

    emit(ctx, "; min_float: finds the minimum element in a float array\n");
    emit(ctx, "define double @min_float({i64, double*} %%array) {\n");
    emit(ctx, "entry:\n");
    emit(ctx, "  %%size = extractvalue {i64, double*} %%array, 0\n");
    emit(ctx, "  %%data = extractvalue {i64, double*} %%array, 1\n");
    emit(ctx, "  %%size_zero = icmp eq i64 %%size, 0\n");
    emit(ctx, "  br i1 %%size_zero, label %%return_zero, label %%init\n");
    emit(ctx, "return_zero:\n");
    emit(ctx, "  ret double 0.0\n");
    emit(ctx, "init:\n");
    emit(ctx, "  %%first_ptr = getelementptr double, double* %%data, i64 0\n");
    emit(ctx, "  %%first_val = load double, double* %%first_ptr\n");
    emit(ctx, "  br label %%loop_cond\n");
    emit(ctx, "loop_cond:\n");
    emit(ctx, "  %%i = phi i64 [1, %%init], [%%i_next, %%loop_body]\n");
    emit(ctx, "  %%min_val = phi double [%%first_val, %%init], [%%new_min, %%loop_body]\n");
    emit(ctx, "  %%cmp = icmp slt i64 %%i, %%size\n");
    emit(ctx, "  br i1 %%cmp, label %%loop_body, label %%loop_end\n");
    emit(ctx, "loop_body:\n");
    emit(ctx, "  %%ptr = getelementptr double, double* %%data, i64 %%i\n");
    emit(ctx, "  %%val = load double, double* %%ptr\n");
    emit(ctx, "  %%is_less = fcmp olt double %%val, %%min_val\n");
    emit(ctx, "  %%new_min = select i1 %%is_less, double %%val, double %%min_val\n");
    emit(ctx, "  %%i_next = add i64 %%i, 1\n");
    emit(ctx, "  br label %%loop_cond\n");
    emit(ctx, "loop_end:\n");
    emit(ctx, "  ret double %%min_val\n");
    emit(ctx, "}\n\n");

    emit(ctx, "; max_float: finds the maximum element in a float array\n");
    emit(ctx, "define double @max_float({i64, double*} %%array) {\n");
    emit(ctx, "entry:\n");
    emit(ctx, "  %%size = extractvalue {i64, double*} %%array, 0\n");
    emit(ctx, "  %%data = extractvalue {i64, double*} %%array, 1\n");
    emit(ctx, "  %%size_zero = icmp eq i64 %%size, 0\n");
    emit(ctx, "  br i1 %%size_zero, label %%return_zero, label %%init\n");
    emit(ctx, "return_zero:\n");
    emit(ctx, "  ret double 0.0\n");
    emit(ctx, "init:\n");
    emit(ctx, "  %%first_ptr = getelementptr double, double* %%data, i64 0\n");
    emit(ctx, "  %%first_val = load double, double* %%first_ptr\n");
    emit(ctx, "  br label %%loop_cond\n");
    emit(ctx, "loop_cond:\n");
    emit(ctx, "  %%i = phi i64 [1, %%init], [%%i_next, %%loop_body]\n");
    emit(ctx, "  %%max_val = phi double [%%first_val, %%init], [%%new_max, %%loop_body]\n");
    emit(ctx, "  %%cmp = icmp slt i64 %%i, %%size\n");
    emit(ctx, "  br i1 %%cmp, label %%loop_body, label %%loop_end\n");
    emit(ctx, "loop_body:\n");
    emit(ctx, "  %%ptr = getelementptr double, double* %%data, i64 %%i\n");
    emit(ctx, "  %%val = load double, double* %%ptr\n");
    emit(ctx, "  %%is_greater = fcmp ogt double %%val, %%max_val\n");
    emit(ctx, "  %%new_max = select i1 %%is_greater, double %%val, double %%max_val\n");
    emit(ctx, "  %%i_next = add i64 %%i, 1\n");
    emit(ctx, "  br label %%loop_cond\n");
    emit(ctx, "loop_end:\n");
    emit(ctx, "  ret double %%max_val\n");
    emit(ctx, "}\n\n");
}

bool generate_llvm_ir(CodeGenContext* ctx, ASTNode* program) {
    if (!ctx || !program || program->type != AST_PROGRAM) return false;
    
    printf("\n[CodeGen] Iniciando geração de código LLVM IR...\n");
    
    emit(ctx, "; DataLang - LLVM IR\n\n");
    emit_runtime_functions(ctx);
    emit(ctx, "; ==================== DECLARAÇÕES ====================\n\n");
    
    bool has_user_main = false;
    for (int i = 0; i < program->program.decl_count; i++) {
        ASTNode* decl = program->program.declarations[i];
        if (decl && decl->type == AST_FN_DECL && 
            strcmp(decl->fn_decl.name, "main") == 0) {
            has_user_main = true;
            break;
        }
    }
    
    for (int i = 0; i < program->program.decl_count; i++) {
        ASTNode* decl = program->program.declarations[i];
        if (decl && decl->type == AST_DATA_DECL) {
            generate_data_decl(ctx, decl);
        }
    }

    emit(ctx, "\n; ==================== GLOBAL VARIABLES ====================\n\n");
    for (int i = 0; i < program->program.decl_count; i++) {
        ASTNode* decl = program->program.declarations[i];
        if (decl && decl->type == AST_LET_DECL) {
            // Generate global variable
            Type* var_type = NULL;
            if (decl->let_decl.type_annotation) {
                var_type = ast_type_to_type(ctx->analyzer, decl->let_decl.type_annotation);
            } else if (decl->let_decl.initializer) {
                var_type = analyze_expression(ctx->analyzer, decl->let_decl.initializer);
            } else {
                var_type = create_primitive_type(TYPE_INT);
            }

            const char* llvm_type = type_to_llvm(var_type);
            char* global_name = malloc(256);
            snprintf(global_name, 256, "@global_%s", decl->let_decl.name);

            // Declare the global variable
            emit(ctx, "%s = global %s ", global_name, llvm_type);

            // Initialize with default value or initializer
            if (decl->let_decl.initializer && decl->let_decl.initializer->type == AST_LITERAL) {
                switch (var_type->kind) {
                    case TYPE_INT:
                        emit(ctx, "%lld\n", decl->let_decl.initializer->literal.int_value);
                        break;
                    case TYPE_FLOAT:
                        emit(ctx, "%e\n", decl->let_decl.initializer->literal.float_value);
                        break;
                    case TYPE_BOOL:
                        emit(ctx, "%s\n", decl->let_decl.initializer->literal.bool_value ? "true" : "false");
                        break;
                    case TYPE_STRING:
                        if (decl->let_decl.initializer->literal.string_value) {
                            char* str_global = register_string_literal(ctx, decl->let_decl.initializer->literal.string_value);
                            int len = strlen(decl->let_decl.initializer->literal.string_value);
                            emit(ctx, "getelementptr inbounds ([%d x i8], [%d x i8]* %s, i32 0, i32 0)\n",
                                 len + 1, len + 1, str_global);
                        } else {
                            emit(ctx, "null\n");
                        }
                        break;
                    default:
                        emit(ctx, "zeroinitializer\n");
                        break;
                }
            } else {
                // Default initialization
                switch (var_type->kind) {
                    case TYPE_INT:
                        emit(ctx, "0\n");
                        break;
                    case TYPE_FLOAT:
                        emit(ctx, "0.0\n");
                        break;
                    case TYPE_BOOL:
                        emit(ctx, "false\n");
                        break;
                    case TYPE_STRING:
                        emit(ctx, "null\n");
                        break;
                    default:
                        emit(ctx, "zeroinitializer\n");
                        break;
                }
            }

            // Register the global variable
            add_var_mapping(ctx, decl->let_decl.name, global_name);
        }
    }
    
    generate_global_initializers_fn(ctx, program);

    for (int i = 0; i < program->program.decl_count; i++) {
        ASTNode* decl = program->program.declarations[i];
        if (decl && decl->type == AST_FN_DECL) {
            generate_function(ctx, decl);
        }
    }
    
    if (!has_user_main) {
        generate_main_function(ctx, program);
    }
    
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
