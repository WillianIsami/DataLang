/*
 * DataLang - Sistema de Tipos
 * Definição e verificação de tipos
 */

#ifndef TYPE_SYSTEM_H
#define TYPE_SYSTEM_H

#include <stdbool.h>

// ==================== TIPOS PRIMITIVOS E COMPOSTOS ====================

typedef enum {
    TYPE_INT,
    TYPE_FLOAT,
    TYPE_STRING,
    TYPE_BOOL,
    TYPE_DATAFRAME,
    TYPE_VECTOR,
    TYPE_SERIES,
    TYPE_ARRAY,
    TYPE_TUPLE,
    TYPE_FUNCTION,
    TYPE_CUSTOM,
    TYPE_VAR,
    TYPE_ERROR,
    TYPE_VOID
} TypeKind;

typedef struct Type {
    TypeKind kind;
    
    // Para TYPE_VAR (inferência)
    char* var_name;
    int var_id;
    
    // Para TYPE_ARRAY
    struct Type* element_type;
    
    // Para TYPE_TUPLE
    struct Type** tuple_types;
    int tuple_count;
    
    // Para TYPE_FUNCTION
    struct Type** param_types;
    int param_count;
    struct Type* return_type;
    
    // Para TYPE_CUSTOM
    char* custom_name;
    
} Type;

// ==================== CRIAÇÃO DE TIPOS ====================

Type* create_primitive_type(TypeKind kind);
Type* create_array_type(Type* element_type);
Type* create_tuple_type(Type** types, int count);
Type* create_function_type(Type** param_types, int param_count, Type* return_type);
Type* create_custom_type(const char* name);
Type* create_type_var(int id);
Type* create_error_type();

// ==================== OPERAÇÕES COM TIPOS ====================

Type* clone_type(Type* type);
void free_type(Type* type);
bool types_equal(Type* a, Type* b);
bool types_compatible(Type* a, Type* b);

// Verificações de tipo
bool is_numeric_type(Type* type);
bool is_comparable_type(Type* type);
bool is_arithmetic_compatible(Type* type1, Type* type2);

// Conversão de tipos
Type* get_result_type_binary_op(Type* left, Type* right, const char* op);
Type* get_result_type_unary_op(Type* operand, const char* op);

// ==================== UTILITÁRIOS ====================

const char* type_to_string(Type* type);
void print_type(Type* type);

// Para debug
void print_type_tree(Type* type, int indent);

#endif // TYPE_SYSTEM_H