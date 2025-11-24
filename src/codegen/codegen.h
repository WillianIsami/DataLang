/*
 * DataLang - Gerador de Código LLVM IR
 * Header para geração de código intermediário
 */

#ifndef CODEGEN_H
#define CODEGEN_H

#include <stdbool.h>
#include "parser.h"
#include "semantic_analyzer.h"

// ==================== ESTRUTURAS ====================

// Contexto de geração de código
typedef struct CodeGenContext {
    FILE* output;                    // Arquivo de saída para LLVM IR
    SemanticAnalyzer* analyzer;      // Analisador semântico
    
    // Contadores para nomes únicos
    int temp_counter;                // Temporários
    int label_counter;               // Labels
    int string_counter;              // String literals
    
    // Estado atual
    char* current_function;          // Função sendo gerada
    bool in_function;                // Se está dentro de função
    
    // Mapeamento de variáveis para valores LLVM
    struct {
        char** names;
        char** llvm_names;
        int count;
        int capacity;
    } var_map;
    
    // Strings literais globais
    struct {
        char** values;
        char** llvm_names;
        int count;
        int capacity;
    } string_literals;
    
} CodeGenContext;

// ==================== FUNÇÕES PÚBLICAS ====================

// Criação e destruição
CodeGenContext* create_codegen_context(SemanticAnalyzer* analyzer, FILE* output);
void free_codegen_context(CodeGenContext* ctx);

// Geração principal
bool generate_llvm_ir(CodeGenContext* ctx, ASTNode* program);

// Funções auxiliares públicas
const char* type_to_llvm(Type* type);
void emit_runtime_functions(CodeGenContext* ctx);

#endif // CODEGEN_H