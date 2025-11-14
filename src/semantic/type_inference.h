/*
 * DataLang - Inferência de Tipos
 * Algoritmo de unificação (Hindley-Milner simplificado)
 */

#ifndef TYPE_INFERENCE_H
#define TYPE_INFERENCE_H

#include <stdbool.h>
#include "type_system.h"

// ==================== SUBSTITUIÇÃO DE TIPOS ====================

typedef struct Substitution {
    Type** from_types;      // Variáveis de tipo
    Type** to_types;        // Tipos concretos
    int count;
    int capacity;
} Substitution;

// Criação e destruição
Substitution* create_substitution();
void free_substitution(Substitution* sub);

// Operações
void add_substitution(Substitution* sub, Type* from, Type* to);
Type* apply_substitution(Substitution* sub, Type* type);
Substitution* compose_substitutions(Substitution* s1, Substitution* s2);

// Debug
void print_substitution(Substitution* sub);

// ==================== UNIFICAÇÃO ====================

typedef struct UnificationResult {
    bool success;
    Substitution* substitution;
    char* error_message;
} UnificationResult;

// Tenta unificar dois tipos
UnificationResult* unify(Type* type1, Type* type2);

// Verifica se uma variável de tipo ocorre em um tipo (occurs check)
bool occurs_check(Type* var, Type* type);

// Libera resultado
void free_unification_result(UnificationResult* result);

// ==================== GERADOR DE VARIÁVEIS DE TIPO ====================

typedef struct TypeVarGenerator {
    int next_id;
} TypeVarGenerator;

TypeVarGenerator* create_type_var_generator();
Type* fresh_type_var(TypeVarGenerator* gen);
void free_type_var_generator(TypeVarGenerator* gen);

// ==================== CONTEXTO DE INFERÊNCIA ====================

typedef struct InferenceContext {
    TypeVarGenerator* var_gen;
    Substitution* substitution;
    int error_count;
    char** error_messages;
    int error_capacity;
} InferenceContext;

InferenceContext* create_inference_context();
void free_inference_context(InferenceContext* ctx);

// Adiciona erro
void inference_error(InferenceContext* ctx, const char* format, ...);
void print_inference_errors(InferenceContext* ctx);

#endif // TYPE_INFERENCE_H