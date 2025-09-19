#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdbool.h>
#include <time.h>

// Token types
typedef enum {
    TOKEN_KEYWORD,
    TOKEN_TYPE,
    TOKEN_IDENTIFIER,
    TOKEN_INTEGER,
    TOKEN_FLOAT,
    TOKEN_STRING,
    TOKEN_BOOLEAN,
    TOKEN_OPERATOR,
    TOKEN_DELIMITER,
    TOKEN_WHITESPACE,
    TOKEN_COMMENT_LINE,
    TOKEN_COMMENT_BLOCK,
    TOKEN_UNKNOWN,
    TOKEN_EOF
} TokenType;

// Token structure
typedef struct {
    TokenType type;
    char* value;
    int line;
    int column;
} Token;

// Function declarations
extern Token* tokenize(const char* input);
extern const char* token_type_name(TokenType type);
extern void run_comprehensive_tests(void);

// Safe memory allocation for large strings
char* safe_malloc_string(size_t size) {
    char* ptr = malloc(size);
    if (!ptr) {
        printf("Erro: falha na alocação de memória\n");
        return NULL;
    }
    return ptr;
}

// Safe string concatenation with bounds checking
void safe_strcat(char* dest, const char* src, size_t dest_size) {
    size_t dest_len = strlen(dest);
    size_t src_len = strlen(src);
    
    if (dest_len + src_len + 1 <= dest_size) {
        strcat(dest, src);
    }
}

// Stress test with large input
void stress_test_large_input() {
    printf("=== Teste de Stress - Entrada Grande ===\n");
    
    // Create a large input to test performance
    const size_t buffer_size = 50000; // Increased buffer size
    char* large_input = safe_malloc_string(buffer_size);
    if (!large_input) {
        printf("Erro: não foi possível alocar memória para teste de stress\n");
        return;
    }
    
    strcpy(large_input, "// Teste de stress com entrada grande\n");
    
    for (int i = 0; i < 100 && strlen(large_input) < buffer_size - 100; i++) {
        char line[100];
        snprintf(line, sizeof(line), "let variable%d = %d\n", i, i * 42);
        safe_strcat(large_input, line, buffer_size);
    }
    
    printf("Testando entrada com %zu caracteres...\n", strlen(large_input));
    
    // Tokenize
    clock_t start = clock();
    Token* tokens = tokenize(large_input);
    clock_t end = clock();
    
    if (!tokens) {
        printf("Erro: falha na tokenização\n");
        free(large_input);
        return;
    }
    
    // Count tokens
    int token_count = 0;
    for (int i = 0; tokens[i].type != TOKEN_EOF; i++) {
        token_count++;
    }
    
    double time_taken = ((double)(end - start)) / CLOCKS_PER_SEC;
    printf("Tokens gerados: %d\n", token_count);
    printf("Tempo de processamento: %f segundos\n", time_taken);
    if (time_taken > 0) {
        printf("Performance: %.0f tokens/segundo\n", token_count / time_taken);
    }
    
    // Free memory
    for (int i = 0; tokens[i].type != TOKEN_EOF; i++) {
        free(tokens[i].value);
    }
    free(tokens);
    free(large_input);
    
    printf("\n");
}

// Test DataLang specific patterns
void test_datalang_patterns() {
    printf("=== Teste de Padrões Específicos do DataLang ===\n");
    
    const char* datalang_code = 
        "// DataLang - Pipeline de análise de dados\n"
        "let data: DataFrame = load(\"dataset.csv\")\n"
        "let result = data\n"
        "  |> filter(|row| row[\"age\"] > 18)\n"
        "  |> groupby(\"department\")\n"
        "  |> map(|group| {\n"
        "    let avg_salary = group |> mean(\"salary\")\n"
        "    return avg_salary\n"
        "  })\n"
        "  |> select([\"department\", \"avg_salary\"])\n"
        "\n"
        "export result as \"output.csv\"";
    
    printf("Código DataLang:\n%s\n\n", datalang_code);
    
    Token* tokens = tokenize(datalang_code);
    if (!tokens) {
        printf("Erro: falha na tokenização\n");
        return;
    }
    
    // Statistical analysis of tokens
    int counts[20] = {0}; // Array to count token types
    int line_count = 1;
    
    printf("Análise detalhada dos tokens:\n");
    for (int i = 0; tokens[i].type != TOKEN_EOF; i++) {
        if (tokens[i].type < 20) {
            counts[tokens[i].type]++;
        }
        
        if (tokens[i].type != TOKEN_WHITESPACE) {
            printf("L%d:C%d %s: '%s'\n", 
                   tokens[i].line, tokens[i].column,
                   token_type_name(tokens[i].type),
                   tokens[i].value ? tokens[i].value : "(null)");
        }
        
        if (tokens[i].type == TOKEN_WHITESPACE && 
            tokens[i].value && strchr(tokens[i].value, '\n')) {
            line_count++;
        }
    }
    
    printf("\nEstatísticas:\n");
    printf("  Linhas processadas: %d\n", line_count);
    printf("  Keywords: %d\n", counts[TOKEN_KEYWORD]);
    printf("  Tipos: %d\n", counts[TOKEN_TYPE]);
    printf("  Identificadores: %d\n", counts[TOKEN_IDENTIFIER]);
    printf("  Números: %d\n", counts[TOKEN_INTEGER] + counts[TOKEN_FLOAT]);
    printf("  Strings: %d\n", counts[TOKEN_STRING]);
    printf("  Operadores: %d\n", counts[TOKEN_OPERATOR]);
    printf("  Delimitadores: %d\n", counts[TOKEN_DELIMITER]);
    printf("  Comentários: %d\n", counts[TOKEN_COMMENT_LINE] + counts[TOKEN_COMMENT_BLOCK]);
    
    // Free tokens
    for (int i = 0; tokens[i].type != TOKEN_EOF; i++) {
        free(tokens[i].value);
    }
    free(tokens);
    
    printf("\n");
}

// Test edge cases
void test_edge_cases() {
    printf("=== Teste de Casos Extremos ===\n");
    
    const char* edge_cases[] = {
        "", // Empty string
        " ", // Only space
        "\n", // Only newline
        "\t\r\n", // Only whitespace
        "123", // Only number
        "\"\"", // Empty string
        "//", // Empty comment
        "/**/", // Empty block comment
        "let", // Only keyword
        "variable", // Only identifier
        "+", // Only operator
        "(", // Only delimiter
        "1.2.3", // Malformed number
        "\"unclosed", // Unclosed string
        "/* unclosed", // Unclosed comment
        "123abc", // Number followed by letters
        "abc123def", // Identifier with numbers
        "...", // Multiple dots
        "====", // Multiple equals
        NULL
    };
    
    for (int i = 0; edge_cases[i] != NULL; i++) {
        printf("Caso extremo: '%s'\n", 
               edge_cases[i][0] == '\0' ? "(vazio)" : edge_cases[i]);
        
        Token* tokens = tokenize(edge_cases[i]);
        if (!tokens) {
            printf("  Erro: falha na tokenização\n");
            continue;
        }
        
        printf("  Tokens: ");
        for (int j = 0; tokens[j].type != TOKEN_EOF; j++) {
            if (tokens[j].type != TOKEN_WHITESPACE) {
                printf("[%s:'%s'] ", token_type_name(tokens[j].type), 
                       tokens[j].value ? tokens[j].value : "(null)");
            }
        }
        printf("\n");
        
        // Free tokens
        for (int j = 0; tokens[j].type != TOKEN_EOF; j++) {
            free(tokens[j].value);
        }
        free(tokens);
    }
    
    printf("\n");
}

// Consistency test: same result in multiple runs
void test_consistency() {
    printf("=== Teste de Consistência ===\n");
    
    const char* test_code = "let x = 42.5 + 3.14e-2";
    
    printf("Testando consistência para: '%s'\n", test_code);
    
    // Run tokenization multiple times
    const int num_runs = 5;
    Token* results[num_runs];
    bool allocation_failed = false;
    
    for (int run = 0; run < num_runs; run++) {
        results[run] = tokenize(test_code);
        if (!results[run]) {
            printf("Erro: falha na tokenização na execução %d\n", run + 1);
            allocation_failed = true;
            break;
        }
    }
    
    if (allocation_failed) {
        // Clean up allocated results
        for (int run = 0; run < num_runs; run++) {
            if (results[run]) {
                for (int i = 0; results[run][i].type != TOKEN_EOF; i++) {
                    free(results[run][i].value);
                }
                free(results[run]);
            }
        }
        return;
    }
    
    // Compare results
    bool all_consistent = true;
    int token_count = 0;
    
    // Count tokens from first result
    for (int i = 0; results[0][i].type != TOKEN_EOF; i++) {
        token_count++;
    }
    
    // Check if all runs have the same result
    for (int run = 1; run < num_runs; run++) {
        for (int i = 0; i < token_count; i++) {
            if (results[0][i].type != results[run][i].type) {
                all_consistent = false;
                printf("  Inconsistência na execução %d, token %d (tipo)\n", run + 1, i);
                break;
            }
            
            // Compare values safely
            bool values_equal = false;
            if (results[0][i].value == NULL && results[run][i].value == NULL) {
                values_equal = true;
            } else if (results[0][i].value != NULL && results[run][i].value != NULL) {
                values_equal = (strcmp(results[0][i].value, results[run][i].value) == 0);
            }
            
            if (!values_equal) {
                all_consistent = false;
                printf("  Inconsistência na execução %d, token %d (valor)\n", run + 1, i);
                break;
            }
        }
        if (!all_consistent) break;
    }
    
    printf("Resultado: %s\n", all_consistent ? "CONSISTENTE" : "INCONSISTENTE");
    
    // Free memory
    for (int run = 0; run < num_runs; run++) {
        for (int i = 0; results[run][i].type != TOKEN_EOF; i++) {
            free(results[run][i].value);
        }
        free(results[run]);
    }
    
    printf("\n");
}

// Simple benchmark
void benchmark_performance() {
    printf("=== Benchmark de Performance ===\n");
    
    const char* benchmark_code = 
        "fn fibonacci(n: Int) -> Int {\n"
        "  if n <= 1 {\n"
        "    return n\n"
        "  } else {\n"
        "    return fibonacci(n-1) + fibonacci(n-2)\n"
        "  }\n"
        "}\n"
        "let result = fibonacci(10)";
    
    const int iterations = 1000;
    
    printf("Executando %d iterações de tokenização...\n", iterations);
    printf("Código de teste: fibonacci function\n");
    
    clock_t start = clock();
    bool all_successful = true;
    
    for (int i = 0; i < iterations; i++) {
        Token* tokens = tokenize(benchmark_code);
        
        if (!tokens) {
            printf("Erro: falha na tokenização na iteração %d\n", i + 1);
            all_successful = false;
            break;
        }
        
        // Free tokens
        for (int j = 0; tokens[j].type != TOKEN_EOF; j++) {
            free(tokens[j].value);
        }
        free(tokens);
    }
    
    clock_t end = clock();
    
    if (all_successful) {
        double total_time = ((double)(end - start)) / CLOCKS_PER_SEC;
        double avg_time = total_time / iterations;
        
        printf("Tempo total: %.4f segundos\n", total_time);
        printf("Tempo médio por iteração: %.6f segundos\n", avg_time);
        if (avg_time > 0) {
            printf("Iterações por segundo: %.0f\n", 1.0 / avg_time);
        }
    } else {
        printf("Benchmark interrompido devido a falhas\n");
    }
    
    printf("\n");
}

// Test memory management
void test_memory_management() {
    printf("=== Teste de Gerenciamento de Memória ===\n");
    
    const char* test_code = "let x = \"hello world\"";
    
    // Test multiple allocations and deallocations
    for (int i = 0; i < 100; i++) {
        Token* tokens = tokenize(test_code);
        if (!tokens) {
            printf("Erro: falha na alocação na iteração %d\n", i + 1);
            return;
        }
        
        // Verify tokens are valid
        bool valid = true;
        for (int j = 0; tokens[j].type != TOKEN_EOF; j++) {
            if (tokens[j].type != TOKEN_WHITESPACE && !tokens[j].value) {
                valid = false;
                break;
            }
        }
        
        if (!valid) {
            printf("Erro: tokens inválidos na iteração %d\n", i + 1);
            // Still free what we can
            for (int j = 0; tokens[j].type != TOKEN_EOF; j++) {
                free(tokens[j].value);
            }
            free(tokens);
            return;
        }
        
        // Free tokens
        for (int j = 0; tokens[j].type != TOKEN_EOF; j++) {
            free(tokens[j].value);
        }
        free(tokens);
    }
    
    printf("100 iterações de alocação/liberação completadas com sucesso\n");
    printf("\n");
}

// Main function
int main() {
    printf("DataLang AFDs - Suite Estendida de Testes\n");
    printf("==========================================\n\n");
    
    // Run main tests
    run_comprehensive_tests();
    
    printf("\n==========================================\n");
    printf("TESTES ADICIONAIS\n");
    printf("==========================================\n\n");
    
    // Run additional tests
    test_datalang_patterns();
    test_edge_cases();
    test_consistency();
    test_memory_management();
    stress_test_large_input();
    benchmark_performance();
    
    printf("==========================================\n");
    printf("Todos os testes estendidos concluídos!\n");
    printf("==========================================\n");
    
    return 0;
}