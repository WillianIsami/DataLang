/*
 * DataLang - Implementação da Inferência de Tipos
 * Baseado no algoritmo de unificação de Hindley-Milner
 */

#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include "type_inference.h"

// ==================== SUBSTITUIÇÃO ====================

Substitution* create_substitution() {
    Substitution* sub = calloc(1, sizeof(Substitution));
    sub->capacity = 10;
    sub->from_types = malloc(sub->capacity * sizeof(Type*));
    sub->to_types = malloc(sub->capacity * sizeof(Type*));
    sub->count = 0;
    return sub;
}

void free_substitution(Substitution* sub) {
    if (!sub) return;
    
    for (int i = 0; i < sub->count; i++) {
        // Nota: não liberamos os tipos aqui, apenas as referências
    }
    
    free(sub->from_types);
    free(sub->to_types);
    free(sub);
}

void add_substitution(Substitution* sub, Type* from, Type* to) {
    if (sub->count >= sub->capacity) {
        sub->capacity *= 2;
        sub->from_types = realloc(sub->from_types, sub->capacity * sizeof(Type*));
        sub->to_types = realloc(sub->to_types, sub->capacity * sizeof(Type*));
    }
    
    // Aplica substituições existentes ao novo tipo
    Type* resolved = apply_substitution(sub, to);
    
    sub->from_types[sub->count] = from;
    sub->to_types[sub->count] = resolved;
    sub->count++;
    
    // Aplica a nova substituição a todas as substituições existentes
    for (int i = 0; i < sub->count - 1; i++) {
        Type* updated = apply_substitution(sub, sub->to_types[i]);
        sub->to_types[i] = updated;
    }
}

Type* apply_substitution(Substitution* sub, Type* type) {
    if (!type || !sub) return type;
    
    // Se for variável de tipo, procura substituição
    if (type->kind == TYPE_VAR) {
        for (int i = 0; i < sub->count; i++) {
            if (sub->from_types[i]->kind == TYPE_VAR &&
                sub->from_types[i]->var_id == type->var_id) {
                // Aplica recursivamente (para resolver cadeias)
                return apply_substitution(sub, sub->to_types[i]);
            }
        }
        return type;
    }
    
    // Aplica recursivamente para tipos compostos
    Type* result = calloc(1, sizeof(Type));
    result->kind = type->kind;
    
    switch (type->kind) {
        case TYPE_ARRAY:
            result->element_type = apply_substitution(sub, type->element_type);
            break;
            
        case TYPE_TUPLE:
            result->tuple_count = type->tuple_count;
            result->tuple_types = malloc(type->tuple_count * sizeof(Type*));
            for (int i = 0; i < type->tuple_count; i++) {
                result->tuple_types[i] = apply_substitution(sub, type->tuple_types[i]);
            }
            break;
            
        case TYPE_FUNCTION:
            result->param_count = type->param_count;
            result->param_types = malloc(type->param_count * sizeof(Type*));
            for (int i = 0; i < type->param_count; i++) {
                result->param_types[i] = apply_substitution(sub, type->param_types[i]);
            }
            result->return_type = apply_substitution(sub, type->return_type);
            break;
            
        case TYPE_CUSTOM:
            result->custom_name = strdup(type->custom_name);
            break;
            
        default:
            // Tipos primitivos não mudam
            free(result);
            return type;
    }
    
    return result;
}

Substitution* compose_substitutions(Substitution* s1, Substitution* s2) {
    Substitution* result = create_substitution();
    
    // Adiciona s1 com s2 aplicado
    for (int i = 0; i < s1->count; i++) {
        Type* applied = apply_substitution(s2, s1->to_types[i]);
        add_substitution(result, s1->from_types[i], applied);
    }
    
    // Adiciona s2 (excluindo variáveis já em s1)
    for (int i = 0; i < s2->count; i++) {
        bool already_in = false;
        for (int j = 0; j < s1->count; j++) {
            if (s1->from_types[j]->kind == TYPE_VAR &&
                s2->from_types[i]->kind == TYPE_VAR &&
                s1->from_types[j]->var_id == s2->from_types[i]->var_id) {
                already_in = true;
                break;
            }
        }
        if (!already_in) {
            add_substitution(result, s2->from_types[i], s2->to_types[i]);
        }
    }
    
    return result;
}

void print_substitution(Substitution* sub) {
    printf("Substituições:\n");
    for (int i = 0; i < sub->count; i++) {
        printf("  ");
        print_type(sub->from_types[i]);
        printf(" = ");
        print_type(sub->to_types[i]);
        printf("\n");
    }
}

// ==================== UNIFICAÇÃO ====================

bool occurs_check(Type* var, Type* type) {
    if (!type) return false;
    if (type->kind == TYPE_ERROR) return false;
    
    if (type->kind == TYPE_VAR) {
        return var->var_id == type->var_id;
    }
    
    switch (type->kind) {
        case TYPE_ARRAY:
            return occurs_check(var, type->element_type);
            
        case TYPE_TUPLE:
            for (int i = 0; i < type->tuple_count; i++) {
                if (occurs_check(var, type->tuple_types[i])) {
                    return true;
                }
            }
            return false;
            
        case TYPE_FUNCTION:
            for (int i = 0; i < type->param_count; i++) {
                if (occurs_check(var, type->param_types[i])) {
                    return true;
                }
            }
            return occurs_check(var, type->return_type);
            
        default:
            return false;
    }
}

UnificationResult* unify(Type* type1, Type* type2) {
    UnificationResult* result = calloc(1, sizeof(UnificationResult));
    result->substitution = create_substitution();
    result->success = true;
    
    // Caso 1: Tipos idênticos
    if (types_equal(type1, type2)) {
        return result;
    }
    
    // Caso 2: type1 é variável de tipo
    if (type1->kind == TYPE_VAR) {
        if (occurs_check(type1, type2)) {
            result->success = false;
            result->error_message = strdup("Tipo recursivo infinito detectado");
            return result;
        }
        add_substitution(result->substitution, type1, type2);
        return result;
    }
    
    // Caso 3: type2 é variável de tipo
    if (type2->kind == TYPE_VAR) {
        if (occurs_check(type2, type1)) {
            result->success = false;
            result->error_message = strdup("Tipo recursivo infinito detectado");
            return result;
        }
        add_substitution(result->substitution, type2, type1);
        return result;
    }
    
    // Caso 4: Ambos são arrays
    if (type1->kind == TYPE_ARRAY && type2->kind == TYPE_ARRAY) {
        UnificationResult* elem_result = unify(type1->element_type, type2->element_type);
        if (!elem_result->success) {
            free_substitution(result->substitution);
            free(result);
            return elem_result;
        }
        result->substitution = elem_result->substitution;
        free(elem_result);
        return result;
    }
    
    // Caso 5: Ambos são tuplas
    if (type1->kind == TYPE_TUPLE && type2->kind == TYPE_TUPLE) {
        if (type1->tuple_count != type2->tuple_count) {
            result->success = false;
            result->error_message = strdup("Tuplas com números diferentes de elementos");
            return result;
        }
        
        for (int i = 0; i < type1->tuple_count; i++) {
            Type* t1 = apply_substitution(result->substitution, type1->tuple_types[i]);
            Type* t2 = apply_substitution(result->substitution, type2->tuple_types[i]);
            
            UnificationResult* elem_result = unify(t1, t2);
            if (!elem_result->success) {
                free_substitution(result->substitution);
                free(result);
                return elem_result;
            }
            
            Substitution* composed = compose_substitutions(result->substitution, 
                                                           elem_result->substitution);
            free_substitution(result->substitution);
            result->substitution = composed;
            free_unification_result(elem_result);
        }
        return result;
    }
    
    // Caso 6: Ambos são funções
    if (type1->kind == TYPE_FUNCTION && type2->kind == TYPE_FUNCTION) {
        if (type1->param_count != type2->param_count) {
            result->success = false;
            result->error_message = strdup("Funções com números diferentes de parâmetros");
            return result;
        }
        
        // Unifica parâmetros
        for (int i = 0; i < type1->param_count; i++) {
            Type* p1 = apply_substitution(result->substitution, type1->param_types[i]);
            Type* p2 = apply_substitution(result->substitution, type2->param_types[i]);
            
            UnificationResult* param_result = unify(p1, p2);
            if (!param_result->success) {
                free_substitution(result->substitution);
                free(result);
                return param_result;
            }
            
            Substitution* composed = compose_substitutions(result->substitution,
                                                           param_result->substitution);
            free_substitution(result->substitution);
            result->substitution = composed;
            free_unification_result(param_result);
        }
        
        // Unifica tipo de retorno
        Type* r1 = apply_substitution(result->substitution, type1->return_type);
        Type* r2 = apply_substitution(result->substitution, type2->return_type);
        
        UnificationResult* ret_result = unify(r1, r2);
        if (!ret_result->success) {
            free_substitution(result->substitution);
            free(result);
            return ret_result;
        }
        
        Substitution* composed = compose_substitutions(result->substitution,
                                                       ret_result->substitution);
        free_substitution(result->substitution);
        result->substitution = composed;
        free_unification_result(ret_result);
        
        return result;
    }
    
    // Caso 7: Tipos incompatíveis
    result->success = false;
    char buffer[256];
    snprintf(buffer, sizeof(buffer), "Não é possível unificar %s com %s",
             type_to_string(type1), type_to_string(type2));
    result->error_message = strdup(buffer);
    
    return result;
}

void free_unification_result(UnificationResult* result) {
    if (!result) return;
    if (result->substitution) free_substitution(result->substitution);
    if (result->error_message) free(result->error_message);
    free(result);
}

// ==================== GERADOR DE VARIÁVEIS DE TIPO ====================

TypeVarGenerator* create_type_var_generator() {
    TypeVarGenerator* gen = calloc(1, sizeof(TypeVarGenerator));
    gen->next_id = 0;
    return gen;
}

Type* fresh_type_var(TypeVarGenerator* gen) {
    return create_type_var(gen->next_id++);
}

void free_type_var_generator(TypeVarGenerator* gen) {
    free(gen);
}

// ==================== CONTEXTO DE INFERÊNCIA ====================

InferenceContext* create_inference_context() {
    InferenceContext* ctx = calloc(1, sizeof(InferenceContext));
    ctx->var_gen = create_type_var_generator();
    ctx->substitution = create_substitution();
    ctx->error_capacity = 10;
    ctx->error_messages = malloc(ctx->error_capacity * sizeof(char*));
    ctx->error_count = 0;
    return ctx;
}

void free_inference_context(InferenceContext* ctx) {
    if (!ctx) return;
    
    free_type_var_generator(ctx->var_gen);
    free_substitution(ctx->substitution);
    
    for (int i = 0; i < ctx->error_count; i++) {
        free(ctx->error_messages[i]);
    }
    free(ctx->error_messages);
    free(ctx);
}

void inference_error(InferenceContext* ctx, const char* format, ...) {
    if (ctx->error_count >= ctx->error_capacity) {
        ctx->error_capacity *= 2;
        ctx->error_messages = realloc(ctx->error_messages,
            ctx->error_capacity * sizeof(char*));
    }
    
    char buffer[512];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    
    ctx->error_messages[ctx->error_count++] = strdup(buffer);
}

void print_inference_errors(InferenceContext* ctx) {
    if (ctx->error_count > 0) {
        printf("\n╔════════════════════════════════════════════════════════════╗\n");
        printf("║         ERROS DE INFERÊNCIA DE TIPOS (%d)                  \n", 
               ctx->error_count);
        printf("╚════════════════════════════════════════════════════════════╝\n\n");
        
        for (int i = 0; i < ctx->error_count; i++) {
            printf("  %d. %s\n", i + 1, ctx->error_messages[i]);
        }
        printf("\n");
    }
}