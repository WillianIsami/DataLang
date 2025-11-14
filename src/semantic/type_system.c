/*
 * DataLang - Sistema de Tipos - Melhorado
 * Verificação rigorosa de compatibilidade de tipos
 */

#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "type_system.h"

// ==================== CRIAÇÃO DE TIPOS ====================

Type* create_primitive_type(TypeKind kind) {
    Type* type = calloc(1, sizeof(Type));
    type->kind = kind;
    return type;
}

Type* create_array_type(Type* element_type) {
    Type* type = calloc(1, sizeof(Type));
    type->kind = TYPE_ARRAY;
    type->element_type = element_type;
    return type;
}

Type* create_tuple_type(Type** types, int count) {
    Type* type = calloc(1, sizeof(Type));
    type->kind = TYPE_TUPLE;
    type->tuple_types = malloc(count * sizeof(Type*));
    type->tuple_count = count;
    for (int i = 0; i < count; i++) {
        type->tuple_types[i] = types[i];
    }
    return type;
}

Type* create_function_type(Type** param_types, int param_count, Type* return_type) {
    Type* type = calloc(1, sizeof(Type));
    type->kind = TYPE_FUNCTION;
    type->param_types = malloc(param_count * sizeof(Type*));
    type->param_count = param_count;
    for (int i = 0; i < param_count; i++) {
        type->param_types[i] = param_types[i];
    }
    type->return_type = return_type;
    return type;
}

Type* create_custom_type(const char* name) {
    Type* type = calloc(1, sizeof(Type));
    type->kind = TYPE_CUSTOM;
    type->custom_name = strdup(name);
    return type;
}

Type* create_type_var(int id) {
    Type* type = calloc(1, sizeof(Type));
    type->kind = TYPE_VAR;
    type->var_id = id;
    
    // Gera nome único
    char buffer[32];
    snprintf(buffer, sizeof(buffer), "T%d", id);
    type->var_name = strdup(buffer);
    
    return type;
}

Type* create_error_type() {
    return create_primitive_type(TYPE_ERROR);
}

// ==================== OPERAÇÕES COM TIPOS ====================

Type* clone_type(Type* type) {
    if (!type) return NULL;
    
    Type* clone = calloc(1, sizeof(Type));
    clone->kind = type->kind;
    
    switch (type->kind) {
        case TYPE_VAR:
            clone->var_id = type->var_id;
            clone->var_name = strdup(type->var_name);
            break;
            
        case TYPE_ARRAY:
            clone->element_type = clone_type(type->element_type);
            break;
            
        case TYPE_TUPLE:
            clone->tuple_count = type->tuple_count;
            clone->tuple_types = malloc(type->tuple_count * sizeof(Type*));
            for (int i = 0; i < type->tuple_count; i++) {
                clone->tuple_types[i] = clone_type(type->tuple_types[i]);
            }
            break;
            
        case TYPE_FUNCTION:
            clone->param_count = type->param_count;
            clone->param_types = malloc(type->param_count * sizeof(Type*));
            for (int i = 0; i < type->param_count; i++) {
                clone->param_types[i] = clone_type(type->param_types[i]);
            }
            clone->return_type = clone_type(type->return_type);
            break;
            
        case TYPE_CUSTOM:
            clone->custom_name = strdup(type->custom_name);
            break;
            
        default:
            break;
    }
    
    return clone;
}

void free_type(Type* type) {
    if (!type) return;
    
    switch (type->kind) {
        case TYPE_VAR:
            free(type->var_name);
            break;
            
        case TYPE_ARRAY:
            free_type(type->element_type);
            break;
            
        case TYPE_TUPLE:
            for (int i = 0; i < type->tuple_count; i++) {
                free_type(type->tuple_types[i]);
            }
            free(type->tuple_types);
            break;
            
        case TYPE_FUNCTION:
            for (int i = 0; i < type->param_count; i++) {
                free_type(type->param_types[i]);
            }
            free(type->param_types);
            free_type(type->return_type);
            break;
            
        case TYPE_CUSTOM:
            free(type->custom_name);
            break;
            
        default:
            break;
    }
    
    free(type);
}

bool types_equal(Type* a, Type* b) {
    if (!a || !b) return false;
    if (a->kind != b->kind) return false;
    
    switch (a->kind) {
        case TYPE_INT:
        case TYPE_FLOAT:
        case TYPE_STRING:
        case TYPE_BOOL:
        case TYPE_DATAFRAME:
        case TYPE_VECTOR:
        case TYPE_SERIES:
        case TYPE_VOID:
        case TYPE_ERROR:
            return true;
            
        case TYPE_VAR:
            return a->var_id == b->var_id;
            
        case TYPE_ARRAY:
            return types_equal(a->element_type, b->element_type);
            
        case TYPE_TUPLE:
            if (a->tuple_count != b->tuple_count) return false;
            for (int i = 0; i < a->tuple_count; i++) {
                if (!types_equal(a->tuple_types[i], b->tuple_types[i])) {
                    return false;
                }
            }
            return true;
            
        case TYPE_FUNCTION:
            if (a->param_count != b->param_count) return false;
            for (int i = 0; i < a->param_count; i++) {
                if (!types_equal(a->param_types[i], b->param_types[i])) {
                    return false;
                }
            }
            return types_equal(a->return_type, b->return_type);
            
        case TYPE_CUSTOM:
            return strcmp(a->custom_name, b->custom_name) == 0;
            
        default:
            return false;
    }
}

bool types_compatible(Type* a, Type* b) {
    // TYPE_ERROR é compatível com tudo (para recuperação de erros)
    if (a->kind == TYPE_ERROR || b->kind == TYPE_ERROR) return true;
    
    // Igualdade estrita
    if (types_equal(a, b)) return true;
    
    // Conversões implícitas mais restritas
    // Apenas permite int -> float, não o contrário
    if (a->kind == TYPE_FLOAT && b->kind == TYPE_INT) {
        return true;  // int pode ser promovido a float
    }
    
    // NÃO permite float -> int (perda de precisão)
    // NÃO permite string + número
    // NÃO permite bool + número
    
    return false;
}

// ==================== VERIFICAÇÕES DE TIPO ====================

bool is_numeric_type(Type* type) {
    return type && (type->kind == TYPE_INT || type->kind == TYPE_FLOAT);
}

bool is_comparable_type(Type* type) {
    if (!type) return false;
    return type->kind == TYPE_INT || 
           type->kind == TYPE_FLOAT || 
           type->kind == TYPE_STRING ||
           type->kind == TYPE_BOOL;
}

bool is_arithmetic_compatible(Type* type1, Type* type2) {
    // Ambos devem ser numéricos
    if (!is_numeric_type(type1) || !is_numeric_type(type2)) {
        return false;
    }
    
    // Verifica compatibilidade exata
    // int + int -> int
    // float + float -> float
    // int + float -> float (promoção)
    // float + int -> float (promoção)
    
    return true;
}

// ==================== INFERÊNCIA DE TIPOS PARA OPERAÇÕES ====================

Type* get_result_type_binary_op(Type* left, Type* right, const char* op) {
    // Operadores aritméticos
    if (strcmp(op, "+") == 0 || strcmp(op, "-") == 0 ||
        strcmp(op, "*") == 0 || strcmp(op, "/") == 0 || strcmp(op, "%") == 0) {
        
        if (!is_numeric_type(left) || !is_numeric_type(right)) {
            return create_error_type();
        }
        
        // Promoção de tipo
        // Se qualquer um for float, resultado é float
        if (left->kind == TYPE_FLOAT || right->kind == TYPE_FLOAT) {
            return create_primitive_type(TYPE_FLOAT);
        }
        return create_primitive_type(TYPE_INT);
    }
    
    // Operadores de comparação
    if (strcmp(op, "==") == 0 || strcmp(op, "!=") == 0 ||
        strcmp(op, "<") == 0 || strcmp(op, "<=") == 0 ||
        strcmp(op, ">") == 0 || strcmp(op, ">=") == 0) {
        
        // Verifica compatibilidade
        if (!types_compatible(left, right)) {
            return create_error_type();
        }
        return create_primitive_type(TYPE_BOOL);
    }
    
    // Operadores lógicos
    if (strcmp(op, "&&") == 0 || strcmp(op, "||") == 0) {
        if (left->kind != TYPE_BOOL || right->kind != TYPE_BOOL) {
            return create_error_type();
        }
        return create_primitive_type(TYPE_BOOL);
    }
    
    // Concatenação de strings
    if (strcmp(op, "++") == 0) {
        if (left->kind == TYPE_STRING && right->kind == TYPE_STRING) {
            return create_primitive_type(TYPE_STRING);
        }
    }
    
    // Range
    if (strcmp(op, "..") == 0) {
        if (left->kind == TYPE_INT && right->kind == TYPE_INT) {
            return create_array_type(create_primitive_type(TYPE_INT));
        }
    }
    
    return create_error_type();
}

Type* get_result_type_unary_op(Type* operand, const char* op) {
    if (strcmp(op, "-") == 0) {
        if (is_numeric_type(operand)) {
            return clone_type(operand);
        }
    }
    
    if (strcmp(op, "!") == 0) {
        if (operand->kind == TYPE_BOOL) {
            return create_primitive_type(TYPE_BOOL);
        }
    }
    
    return create_error_type();
}

// ==================== UTILITÁRIOS ====================

const char* type_to_string(Type* type) {
    if (!type) return "null";
    
    static char buffer[256];
    
    switch (type->kind) {
        case TYPE_INT: return "Int";
        case TYPE_FLOAT: return "Float";
        case TYPE_STRING: return "String";
        case TYPE_BOOL: return "Bool";
        case TYPE_DATAFRAME: return "DataFrame";
        case TYPE_VECTOR: return "Vector";
        case TYPE_SERIES: return "Series";
        case TYPE_VOID: return "Void";
        case TYPE_ERROR: return "Error";
        
        case TYPE_VAR:
            snprintf(buffer, sizeof(buffer), "'%s", type->var_name);
            return buffer;
        
        case TYPE_ARRAY:
            snprintf(buffer, sizeof(buffer), "[%s]", 
                     type_to_string(type->element_type));
            return buffer;
        
        case TYPE_CUSTOM:
            return type->custom_name;
        
        case TYPE_TUPLE: {
            char* ptr = buffer;
            ptr += sprintf(ptr, "(");
            for (int i = 0; i < type->tuple_count; i++) {
                if (i > 0) ptr += sprintf(ptr, ", ");
                ptr += sprintf(ptr, "%s", type_to_string(type->tuple_types[i]));
            }
            sprintf(ptr, ")");
            return buffer;
        }
        
        case TYPE_FUNCTION: {
            char* ptr = buffer;
            ptr += sprintf(ptr, "(");
            for (int i = 0; i < type->param_count; i++) {
                if (i > 0) ptr += sprintf(ptr, ", ");
                ptr += sprintf(ptr, "%s", type_to_string(type->param_types[i]));
            }
            ptr += sprintf(ptr, ") -> %s", type_to_string(type->return_type));
            return buffer;
        }
        
        default:
            return "Unknown";
    }
}

void print_type(Type* type) {
    printf("%s", type_to_string(type));
}

void print_type_tree(Type* type, int indent) {
    for (int i = 0; i < indent; i++) printf("  ");
    
    if (!type) {
        printf("null\n");
        return;
    }
    
    printf("%s", type_to_string(type));
    
    switch (type->kind) {
        case TYPE_ARRAY:
            printf(" {\n");
            print_type_tree(type->element_type, indent + 1);
            for (int i = 0; i < indent; i++) printf("  ");
            printf("}\n");
            break;
            
        case TYPE_TUPLE:
            printf(" {\n");
            for (int i = 0; i < type->tuple_count; i++) {
                print_type_tree(type->tuple_types[i], indent + 1);
            }
            for (int i = 0; i < indent; i++) printf("  ");
            printf("}\n");
            break;
            
        case TYPE_FUNCTION:
            printf(" {\n");
            for (int i = 0; i < indent + 1; i++) printf("  ");
            printf("params:\n");
            for (int i = 0; i < type->param_count; i++) {
                print_type_tree(type->param_types[i], indent + 2);
            }
            for (int i = 0; i < indent + 1; i++) printf("  ");
            printf("return:\n");
            print_type_tree(type->return_type, indent + 2);
            for (int i = 0; i < indent; i++) printf("  ");
            printf("}\n");
            break;
            
        default:
            printf("\n");
            break;
    }
}