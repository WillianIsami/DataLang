/*
 * DataLang - Compilador Completo
 * Léxico -> Sintático -> Semântico -> LLVM IR
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "lexer/lexer.h"
#include "parser/parser.h"
#include "semantic/semantic_analyzer.h"
#include "codegen/codegen.h"

// Declarações externas
extern AFD* create_datalang_afd_from_afn();
extern TokenStream* tokenize(const char* input, AFD* afd);
extern void free_afd(AFD* afd);
extern void free_token_stream(TokenStream* stream);

void print_usage(const char* program_name) {
    printf("Uso: %s <arquivo.datalang> [-o output.ll]\n\n", program_name);
    printf("Opções:\n");
    printf("  -o <arquivo>    Especifica arquivo de saída LLVM IR\n");
    printf("  -h, --help      Mostra esta ajuda\n");
    printf("  -v, --verbose   Modo verboso\n\n");
}

char* read_file(const char* filename) {
    FILE* file = fopen(filename, "r");
    if (!file) {
        fprintf(stderr, "Erro: Não foi possível abrir o arquivo '%s'\n", filename);
        return NULL;
    }
    
    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    char* content = malloc(size + 1);
    if (!content) {
        fclose(file);
        return NULL;
    }
    
    size_t read_size = fread(content, 1, size, file);
    content[read_size] = '\0';
    fclose(file);
    
    return content;
}

int main(int argc, char** argv) {
    // Parse argumentos
    if (argc < 2) {
        print_usage(argv[0]);
        return 1;
    }
    
    char* input_file = NULL;
    char* output_file = NULL;
    bool verbose = false;
    
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            print_usage(argv[0]);
            return 0;
        } else if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--verbose") == 0) {
            verbose = true;
        } else if (strcmp(argv[i], "-o") == 0) {
            if (i + 1 < argc) {
                output_file = argv[++i];
            } else {
                fprintf(stderr, "Erro: Opção -o requer um argumento\n");
                return 1;
            }
        } else {
            input_file = argv[i];
        }
    }
    
    if (verbose) {
        printf("Modo verboso ativado.\n");
    }
    
    if (!input_file) {
        fprintf(stderr, "Erro: Nenhum arquivo de entrada especificado\n");
        print_usage(argv[0]);
        return 1;
    }
    
    // Define arquivo de saída padrão
    if (!output_file) {
        output_file = malloc(strlen(input_file) + 4);
        strcpy(output_file, input_file);
        char* dot = strrchr(output_file, '.');
        if (dot) *dot = '\0';
        strcat(output_file, ".ll");
    }
    
    printf("Compilando: %s -> %s\n", input_file, output_file);
    
    // FASE 1: LEITURA
    char* source_code = read_file(input_file);
    if (!source_code) return 1;
    
    // FASE 2: LÉXICO
    AFD* afd = create_datalang_afd_from_afn();
    if (!afd) {
        free(source_code);
        return 1;
    }
    
    TokenStream* tokens = tokenize(source_code, afd);
    if (!tokens) {
        free_afd(afd);
        free(source_code);
        return 1;
    }
    
    // FASE 3: SINTÁTICO
    Parser* parser = create_parser(tokens);
    ASTNode* ast = parse(parser);
    if (!ast || parser->had_error) {
        if (ast) free_ast(ast);
        free_parser(parser);
        free_token_stream(tokens);
        free_afd(afd);
        free(source_code);
        return 1;
    }
    
    // FASE 4: SEMÂNTICO
    SemanticAnalyzer* analyzer = create_semantic_analyzer();
    if (!analyze_semantics(analyzer, ast)) {
        free_semantic_analyzer(analyzer);
        free_ast(ast);
        free_parser(parser);
        free_token_stream(tokens);
        free_afd(afd);
        free(source_code);
        return 1;
    }
    
    // FASE 5: GERAÇÃO DE CÓDIGO
    FILE* output = fopen(output_file, "w");
    if (!output) {
        free_semantic_analyzer(analyzer);
        free_ast(ast);
        free_parser(parser);
        free_token_stream(tokens);
        free_afd(afd);
        free(source_code);
        return 1;
    }
    
    CodeGenContext* codegen = create_codegen_context(analyzer, output);
    if (!generate_llvm_ir(codegen, ast)) {
        fclose(output);
        free_codegen_context(codegen);
        free_semantic_analyzer(analyzer);
        free_ast(ast);
        free_parser(parser);
        free_token_stream(tokens);
        free_afd(afd);
        free(source_code);
        return 1;
    }
    
    fclose(output);
    printf("Sucesso! Código gerado em %s\n", output_file);
    
    free_codegen_context(codegen);
    free_semantic_analyzer(analyzer);
    free_ast(ast);
    free_parser(parser);
    free_token_stream(tokens);
    free_afd(afd);
    free(source_code);

    if (output_file) {
        bool should_free = true;
        for (int i = 1; i < argc; i++) {
            if (argv[i] == output_file || (i + 1 < argc && argv[i + 1] == output_file)) {
                should_free = false;
                break;
            }
        }
        if (should_free) {
            free(output_file);
        }
    }
    return 0;
}