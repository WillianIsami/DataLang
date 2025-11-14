/*
 * DataLang - Implementação da Tabela de Símbolos
 */

#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include "symbol_table.h"

// ==================== FUNÇÕES AUXILIARES ====================

static Symbol* create_symbol(const char* name, SymbolKind kind, Type* type,
                             int line, int column) {
    Symbol* symbol = calloc(1, sizeof(Symbol));
    symbol->name = strdup(name);
    symbol->kind = kind;
    symbol->type = type;
    symbol->line = line;
    symbol->column = column;
    symbol->initialized = false;
    symbol->used = false;
    symbol->is_mutable = true;
    return symbol;
}

static void free_symbol(Symbol* symbol) {
    if (!symbol) return;
    free(symbol->name);
    if (symbol->type) free_type(symbol->type);
    if (symbol->param_types) {
        for (int i = 0; i < symbol->param_count; i++) {
            free_type(symbol->param_types[i]);
        }
        free(symbol->param_types);
    }
    if (symbol->fields) {
        for (int i = 0; i < symbol->field_count; i++) {
            free_symbol(symbol->fields[i]);
        }
        free(symbol->fields);
    }
    free(symbol);
}

static Scope* create_scope(Scope* parent) {
    Scope* scope = calloc(1, sizeof(Scope));
    scope->symbol_capacity = 10;
    scope->symbols = malloc(scope->symbol_capacity * sizeof(Symbol*));
    scope->symbol_count = 0;
    scope->parent = parent;
    scope->depth = parent ? parent->depth + 1 : 0;
    return scope;
}

static void free_scope(Scope* scope) {
    if (!scope) return;
    for (int i = 0; i < scope->symbol_count; i++) {
        free_symbol(scope->symbols[i]);
    }
    free(scope->symbols);
    free(scope);
}

// ==================== CRIAÇÃO E DESTRUIÇÃO ====================

SymbolTable* create_symbol_table() {
    SymbolTable* table = calloc(1, sizeof(SymbolTable));
    table->global_scope = create_scope(NULL);
    table->current_scope = table->global_scope;
    table->error_capacity = 10;
    table->error_messages = malloc(table->error_capacity * sizeof(char*));
    table->error_count = 0;
    return table;
}

void free_symbol_table(SymbolTable* table) {
    if (!table) return;
    
    // Libera escopos aninhados
    Scope* scope = table->current_scope;
    while (scope) {
        Scope* parent = scope->parent;
        free_scope(scope);
        scope = parent;
    }
    
    // Libera mensagens de erro
    for (int i = 0; i < table->error_count; i++) {
        free(table->error_messages[i]);
    }
    free(table->error_messages);
    free(table);
}

// ==================== GERENCIAMENTO DE ESCOPOS ====================

void enter_scope(SymbolTable* table) {
    Scope* new_scope = create_scope(table->current_scope);
    table->current_scope = new_scope;
}

void exit_scope(SymbolTable* table) {
    if (!table->current_scope->parent) {
        fprintf(stderr, "Erro: tentativa de sair do escopo global!\n");
        return;
    }
    
    // Verifica símbolos não utilizados antes de sair
    for (int i = 0; i < table->current_scope->symbol_count; i++) {
        Symbol* sym = table->current_scope->symbols[i];
        if (!sym->used && sym->kind == SYMBOL_VARIABLE) {
            symbol_table_error(table, sym->line, sym->column,
                "Aviso: Variável '%s' declarada mas nunca usada", sym->name);
        }
    }
    
    Scope* old_scope = table->current_scope;
    table->current_scope = old_scope->parent;
    free_scope(old_scope);
}

int get_scope_depth(SymbolTable* table) {
    return table->current_scope->depth;
}

// ==================== DECLARAÇÃO DE SÍMBOLOS ====================

Symbol* declare_symbol(SymbolTable* table, const char* name, SymbolKind kind,
                       Type* type, int line, int column) {
    // Verifica redeclaração no escopo atual
    Symbol* existing = lookup_in_current_scope(table, name);
    if (existing) {
        symbol_table_error(table, line, column,
            "Símbolo '%s' já declarado neste escopo (primeira declaração na linha %d)",
            name, existing->line);
        return NULL;
    }
    
    Symbol* symbol = create_symbol(name, kind, type, line, column);
    
    // Adiciona ao escopo atual
    if (table->current_scope->symbol_count >= table->current_scope->symbol_capacity) {
        table->current_scope->symbol_capacity *= 2;
        table->current_scope->symbols = realloc(table->current_scope->symbols,
            table->current_scope->symbol_capacity * sizeof(Symbol*));
    }
    
    table->current_scope->symbols[table->current_scope->symbol_count++] = symbol;
    return symbol;
}

Symbol* declare_function(SymbolTable* table, const char* name, Type* return_type,
                         Type** param_types, int param_count, int line, int column) {
    // Cria tipo de função
    Type* func_type = create_function_type(param_types, param_count, return_type);
    
    Symbol* symbol = declare_symbol(table, name, SYMBOL_FUNCTION, func_type, line, column);
    if (symbol) {
        symbol->param_count = param_count;
        symbol->param_types = malloc(param_count * sizeof(Type*));
        for (int i = 0; i < param_count; i++) {
            symbol->param_types[i] = clone_type(param_types[i]);
        }
    }
    
    return symbol;
}

Symbol* declare_type(SymbolTable* table, const char* name, Symbol** fields,
                     int field_count, int line, int column) {
    Type* type = create_custom_type(name);
    Symbol* symbol = declare_symbol(table, name, SYMBOL_TYPE, type, line, column);
    
    if (symbol) {
        symbol->field_count = field_count;
        symbol->fields = malloc(field_count * sizeof(Symbol*));
        for (int i = 0; i < field_count; i++) {
            symbol->fields[i] = fields[i];
        }
    }
    
    return symbol;
}

// ==================== BUSCA DE SÍMBOLOS ====================

Symbol* lookup_symbol(SymbolTable* table, const char* name) {
    Scope* scope = table->current_scope;
    
    // Busca do escopo mais interno para o mais externo
    while (scope) {
        for (int i = 0; i < scope->symbol_count; i++) {
            if (strcmp(scope->symbols[i]->name, name) == 0) {
                return scope->symbols[i];
            }
        }
        scope = scope->parent;
    }
    
    return NULL;
}

Symbol* lookup_in_current_scope(SymbolTable* table, const char* name) {
    for (int i = 0; i < table->current_scope->symbol_count; i++) {
        if (strcmp(table->current_scope->symbols[i]->name, name) == 0) {
            return table->current_scope->symbols[i];
        }
    }
    return NULL;
}

// ==================== MARCAÇÃO DE USO ====================

void mark_symbol_used(SymbolTable* table, const char* name) {
    Symbol* symbol = lookup_symbol(table, name);
    if (symbol) {
        symbol->used = true;
    }
}

void mark_symbol_initialized(SymbolTable* table, const char* name) {
    Symbol* symbol = lookup_symbol(table, name);
    if (symbol) {
        symbol->initialized = true;
    }
}

// ==================== VERIFICAÇÕES ====================

bool is_symbol_declared(SymbolTable* table, const char* name) {
    return lookup_symbol(table, name) != NULL;
}

bool is_symbol_in_current_scope(SymbolTable* table, const char* name) {
    return lookup_in_current_scope(table, name) != NULL;
}

void check_unused_symbols(SymbolTable* table) {
    Scope* scope = table->current_scope;
    while (scope) {
        for (int i = 0; i < scope->symbol_count; i++) {
            Symbol* sym = scope->symbols[i];
            if (!sym->used && sym->kind == SYMBOL_VARIABLE) {
                printf("Aviso [linha %d]: Variável '%s' declarada mas nunca usada\n",
                       sym->line, sym->name);
            }
        }
        scope = scope->parent;
    }
}

void check_uninitialized_usage(SymbolTable* table __attribute__((unused))) {
    // Esta verificação é feita durante a análise semântica
    // quando uma variável é usada
}

// ==================== TRATAMENTO DE ERROS ====================

void symbol_table_error(SymbolTable* table, int line, int column,
                        const char* format, ...) {
    if (table->error_count >= table->error_capacity) {
        table->error_capacity *= 2;
        table->error_messages = realloc(table->error_messages,
            table->error_capacity * sizeof(char*));
    }
    
    char buffer[512];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    
    char full_message[600];
    snprintf(full_message, sizeof(full_message),
             "Erro semântico [linha %d, coluna %d]: %s", line, column, buffer);
    
    table->error_messages[table->error_count++] = strdup(full_message);
}

void print_symbol_table_errors(SymbolTable* table) {
    if (table->error_count > 0) {
        printf("\n╔════════════════════════════════════════════════════════════╗\n");
        printf("║         ERROS SEMÂNTICOS ENCONTRADOS (%d)                  \n", 
               table->error_count);
        printf("╚════════════════════════════════════════════════════════════╝\n\n");
        
        for (int i = 0; i < table->error_count; i++) {
            printf("  %d. %s\n", i + 1, table->error_messages[i]);
        }
        printf("\n");
    }
}

// ==================== DEBUG ====================

void print_symbol_table(SymbolTable* table) {
    printf("\n╔════════════════════════════════════════════════════════════╗\n");
    printf("║              TABELA DE SÍMBOLOS COMPLETA                   ║\n");
    printf("╚════════════════════════════════════════════════════════════╝\n\n");
    
    Scope* scope = table->current_scope;
    int level = 0;
    
    // Conta níveis
    Scope* temp = scope;
    while (temp) {
        level++;
        temp = temp->parent;
    }
    
    // Imprime do global para o mais interno
    Scope** scopes = malloc(level * sizeof(Scope*));
    temp = scope;
    for (int i = level - 1; i >= 0; i--) {
        scopes[i] = temp;
        temp = temp->parent;
    }
    
    for (int i = 0; i < level; i++) {
        printf("Escopo nível %d (profundidade %d):\n", i, scopes[i]->depth);
        printf("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n");
        
        for (int j = 0; j < scopes[i]->symbol_count; j++) {
            Symbol* sym = scopes[i]->symbols[j];
            const char* kind_str = 
                sym->kind == SYMBOL_VARIABLE ? "VAR" :
                sym->kind == SYMBOL_FUNCTION ? "FN" :
                sym->kind == SYMBOL_PARAMETER ? "PARAM" :
                sym->kind == SYMBOL_TYPE ? "TYPE" : "FIELD";
            
            printf("  %-8s %-20s : ", kind_str, sym->name);
            print_type(sym->type);
            printf(" (linha %d)", sym->line);
            
            if (sym->kind == SYMBOL_VARIABLE) {
                printf(" [%s%s]",
                       sym->initialized ? "init" : "uninit",
                       sym->used ? ", usado" : ", não usado");
            }
            printf("\n");
        }
        printf("\n");
    }
    
    free(scopes);
}

void print_current_scope(SymbolTable* table) {
    printf("Escopo atual (profundidade %d):\n", table->current_scope->depth);
    for (int i = 0; i < table->current_scope->symbol_count; i++) {
        Symbol* sym = table->current_scope->symbols[i];
        printf("  %s : ", sym->name);
        print_type(sym->type);
        printf("\n");
    }
}