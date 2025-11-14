/*
 * DataLang - Tabela de Símbolos
 * Gerenciamento de escopos aninhados para análise semântica
 */

#ifndef SYMBOL_TABLE_H
#define SYMBOL_TABLE_H

#include <stdbool.h>
#include "type_system.h"

// ==================== ESTRUTURAS ====================

typedef enum {
    SYMBOL_VARIABLE,
    SYMBOL_FUNCTION,
    SYMBOL_PARAMETER,
    SYMBOL_TYPE,
    SYMBOL_FIELD
} SymbolKind;

typedef struct Symbol {
    char* name;              // Nome do símbolo
    SymbolKind kind;         // Tipo de símbolo
    Type* type;              // Tipo do símbolo
    int line;                // Linha onde foi declarado
    int column;              // Coluna onde foi declarado
    bool initialized;        // Se foi inicializado
    bool used;               // Se foi usado
    bool is_mutable;         // Se é mutável (let vs const)
    
    // Para funções
    int param_count;
    Type** param_types;
    
    // Para tipos de dados (struct)
    int field_count;
    struct Symbol** fields;
} Symbol;

typedef struct Scope {
    Symbol** symbols;        // Array de símbolos neste escopo
    int symbol_count;
    int symbol_capacity;
    struct Scope* parent;    // Escopo pai (NULL se global)
    int depth;               // Profundidade do escopo (0 = global)
} Scope;

typedef struct SymbolTable {
    Scope* current_scope;    // Escopo atual
    Scope* global_scope;     // Escopo global (sempre acessível)
    int error_count;
    char** error_messages;
    int error_capacity;
} SymbolTable;

// ==================== FUNÇÕES PÚBLICAS ====================

// Criação e destruição
SymbolTable* create_symbol_table();
void free_symbol_table(SymbolTable* table);

// Gerenciamento de escopos
void enter_scope(SymbolTable* table);
void exit_scope(SymbolTable* table);
int get_scope_depth(SymbolTable* table);

// Declaração de símbolos
Symbol* declare_symbol(SymbolTable* table, const char* name, SymbolKind kind,
                       Type* type, int line, int column);
Symbol* declare_function(SymbolTable* table, const char* name, Type* return_type,
                         Type** param_types, int param_count, int line, int column);
Symbol* declare_type(SymbolTable* table, const char* name, Symbol** fields,
                     int field_count, int line, int column);

// Busca de símbolos
Symbol* lookup_symbol(SymbolTable* table, const char* name);
Symbol* lookup_in_current_scope(SymbolTable* table, const char* name);

// Marcação de uso
void mark_symbol_used(SymbolTable* table, const char* name);
void mark_symbol_initialized(SymbolTable* table, const char* name);

// Verificações
bool is_symbol_declared(SymbolTable* table, const char* name);
bool is_symbol_in_current_scope(SymbolTable* table, const char* name);

// Relatórios
void check_unused_symbols(SymbolTable* table);
void check_uninitialized_usage(SymbolTable* table);

// Erros
void symbol_table_error(SymbolTable* table, int line, int column, 
                        const char* format, ...);
void print_symbol_table_errors(SymbolTable* table);

// Debug
void print_symbol_table(SymbolTable* table);
void print_current_scope(SymbolTable* table);

#endif // SYMBOL_TABLE_H