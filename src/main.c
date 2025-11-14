/*
 * DataLang - Ponto de Entrada Principal do Compilador
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lexer/lexer.h"
#include "parser/parser.h"
#include "semantic/semantic_analyzer.h"

// Declarações externas
extern AFD* create_datalang_afd_from_afn();
extern TokenStream* tokenize(const char* input, AFD* afd);
extern void free_afd(AFD* afd);
extern void free_token_stream(TokenStream* stream);

void print_usage(const char* program_name) {
    printf("Uso: %s <arquivo.datalang>\n", program_name);
    printf("\nOpções:\n");
    printf("  <arquivo>    Compila o arquivo DataLang especificado\n");
    printf("  --help       Mostra esta ajuda\n");
    printf("\nExemplo:\n");
    printf("  %s examples/exemplo.datalang\n", program_name);
}

int compile_file(const char* filename) {
    printf("\n╔════════════════════════════════════════════════════════════╗\n");
    printf("║               COMPILADOR DATALANG v0.3.0                 ║\n");
    printf("╚════════════════════════════════════════════════════════════╝\n\n");
    
    // Ler arquivo
    FILE* file = fopen(filename, "r");
    if (!file) {
        fprintf(stderr, "❌ Erro: Não foi possível abrir o arquivo '%s'\n", filename);
        return 1;
    }
    
    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    char* code = malloc(size + 1);
    fread(code, 1, size, file);
    code[size] = '\0';
    fclose(file);
    
    printf("📄 Arquivo: %s (%ld bytes)\n", filename, size);
    printf("════════════════════════════════════════════════════════════\n\n");
    
    // 1. Análise Léxica
    printf("[FASE 1] Análise Léxica\n");
    printf("────────────────────────────────────────────────────────────\n");
    
    AFD* afd = create_datalang_afd_from_afn();
    if (!afd) {
        fprintf(stderr, "❌ Erro ao criar AFD\n");
        free(code);
        return 1;
    }
    
    TokenStream* tokens = tokenize(code, afd);
    if (!tokens) {
        fprintf(stderr, "❌ Erro na tokenização\n");
        free_afd(afd);
        free(code);
        return 1;
    }
    
    printf("✓ Tokens reconhecidos: %d\n", tokens->count - 1);
    
    // 2. Análise Sintática
    printf("\n[FASE 2] Análise Sintática\n");
    printf("────────────────────────────────────────────────────────────\n");
    
    Parser* parser = create_parser(tokens);
    ASTNode* ast = parse(parser);
    
    if (!ast || parser->had_error) {
        fprintf(stderr, "❌ Erro na análise sintática\n");
        if (ast) free_ast(ast);
        free_parser(parser);
        free_token_stream(tokens);
        free_afd(afd);
        free(code);
        return 1;
    }
    
    printf("✓ AST construída com sucesso\n");
    
    // 3. Análise Semântica
    printf("\n[FASE 3] Análise Semântica\n");
    printf("────────────────────────────────────────────────────────────\n");
    
    SemanticAnalyzer* analyzer = create_semantic_analyzer();
    bool semantic_result = analyze_semantics(analyzer, ast);
    
    // Resultado final
    printf("\n════════════════════════════════════════════════════════════\n");
    if (semantic_result) {
        printf("✅ COMPILAÇÃO BEM-SUCEDIDA!\n");
    } else {
        printf("❌ ERROS ENCONTRADOS NA COMPILAÇÃO\n");
    }
    printf("════════════════════════════════════════════════════════════\n");
    
    // Limpeza
    free_semantic_analyzer(analyzer);
    free_ast(ast);
    free_parser(parser);
    free_token_stream(tokens);
    free_afd(afd);
    free(code);
    
    return semantic_result ? 0 : 1;
}

int main(int argc, char** argv) {
    if (argc != 2) {
        print_usage(argv[0]);
        return 1;
    }
    
    if (strcmp(argv[1], "--help") == 0) {
        print_usage(argv[0]);
        return 0;
    }
    
    return compile_file(argv[1]);
}