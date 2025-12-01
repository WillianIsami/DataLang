#ifndef SEMANTIC_ANALYZER_H
#define SEMANTIC_ANALYZER_H

#include <stdbool.h>
#include "../parser/parser.h"
#include "symbol_table.h"
#include "type_system.h"
#include "type_inference.h"

// ==================== ESTRUTURA PRINCIPAL ====================

typedef struct SemanticAnalyzer {
    SymbolTable* symbol_table;
    InferenceContext* inference_ctx;
    
    bool had_error;
    int warning_count;
    
    // Para rastreamento de contexto
    bool in_function;
    Type* current_function_return_type;
    bool in_loop;
} SemanticAnalyzer;

// ==================== CRIAÇÃO E DESTRUIÇÃO ====================

SemanticAnalyzer* create_semantic_analyzer();
void free_semantic_analyzer(SemanticAnalyzer* analyzer);

// ==================== ANÁLISE PRINCIPAL ====================

// Analisa um programa completo
bool analyze_semantics(SemanticAnalyzer* analyzer, ASTNode* program);

// ==================== ANÁLISE POR TIPO DE NODO ====================

// Declarações
Type* analyze_let_decl(SemanticAnalyzer* analyzer, ASTNode* node);
Type* analyze_fn_decl(SemanticAnalyzer* analyzer, ASTNode* node);
Type* analyze_data_decl(SemanticAnalyzer* analyzer, ASTNode* node);

// Statements
Type* analyze_statement(SemanticAnalyzer* analyzer, ASTNode* node);
Type* analyze_if_stmt(SemanticAnalyzer* analyzer, ASTNode* node);
Type* analyze_for_stmt(SemanticAnalyzer* analyzer, ASTNode* node);
Type* analyze_return_stmt(SemanticAnalyzer* analyzer, ASTNode* node);
Type* analyze_block(SemanticAnalyzer* analyzer, ASTNode* node);

// Expressões
Type* analyze_expression(SemanticAnalyzer* analyzer, ASTNode* node);
Type* analyze_binary_expr(SemanticAnalyzer* analyzer, ASTNode* node);
Type* analyze_unary_expr(SemanticAnalyzer* analyzer, ASTNode* node);
Type* analyze_call_expr(SemanticAnalyzer* analyzer, ASTNode* node);
Type* analyze_lambda_expr(SemanticAnalyzer* analyzer, ASTNode* node);
Type* analyze_pipeline_expr(SemanticAnalyzer* analyzer, ASTNode* node);
Type* analyze_literal(SemanticAnalyzer* analyzer, ASTNode* node);
Type* analyze_identifier(SemanticAnalyzer* analyzer, ASTNode* node);

// ==================== VERIFICAÇÕES ESPECÍFICAS ====================

// Verifica se todos os caminhos de uma função retornam
bool check_all_paths_return(SemanticAnalyzer* analyzer, ASTNode* block);

// Verifica se uma expressão é atribuível (lvalue)
bool is_lvalue(ASTNode* node);

// Verifica compatibilidade de tipos para operações
bool check_binary_op_types(SemanticAnalyzer* analyzer, 
                           Type* left, Type* right, const char* op,
                           int line, int column);

// ==================== CONVERSÃO DE TIPOS DA AST ====================

Type* ast_type_to_type(SemanticAnalyzer* analyzer, ASTNode* type_node);

// ==================== UTILITÁRIOS ====================

void print_semantic_analysis_report(SemanticAnalyzer* analyzer);
void annotate_ast_with_types(ASTNode* node);

#endif // SEMANTIC_ANALYZER_H