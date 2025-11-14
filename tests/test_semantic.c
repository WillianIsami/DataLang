/*
 * DataLang - Teste do Analisador Semântico
 * 
 * Este arquivo testa todas as funcionalidades do analisador semântico:
 * - Verificação de tipos
 * - Inferência de tipos
 * - Tabela de símbolos
 * - Detecção de erros semânticos
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../src/lexer/datalang_afn.h"
#include "../src/parser/parser.h"
#include "../src/semantic/semantic_analyzer.h"

// Declarações externas
extern AFD* create_datalang_afd_from_afn();
extern TokenStream* tokenize(const char* input, AFD* afd);
extern void free_afd(AFD* afd);
extern void free_token_stream(TokenStream* stream);

// ==================== FUNÇÕES AUXILIARES ====================

void print_separator(const char* title) {
    printf("\n");
    printf("╔════════════════════════════════════════════════════════════╗\n");
    printf("║  %-56s  ║\n", title);
    printf("╚════════════════════════════════════════════════════════════╝\n");
    printf("\n");
}

void test_code(const char* test_name, const char* code, bool should_pass) {
    print_separator(test_name);
    
    printf("Código:\n");
    printf("────────────────────────────────────────────────────────────\n");
    printf("%s\n", code);
    printf("────────────────────────────────────────────────────────────\n\n");
    
    // Análise léxica
    AFD* afd = create_datalang_afd_from_afn();
    if (!afd) {
        printf("❌ Erro ao criar AFD\n");
        return;
    }
    
    TokenStream* tokens = tokenize(code, afd);
    if (!tokens) {
        printf("❌ Erro na tokenização\n");
        free_afd(afd);
        return;
    }
    
    // Análise sintática
    Parser* parser = create_parser(tokens);
    ASTNode* ast = parse(parser);
    
    if (!ast || parser->had_error) {
        printf("❌ Erro na análise sintática\n");
        if (ast) free_ast(ast);
        free_parser(parser);
        free_token_stream(tokens);
        free_afd(afd);
        return;
    }
    
    // Análise semântica
    SemanticAnalyzer* analyzer = create_semantic_analyzer();
    bool result = analyze_semantics(analyzer, ast);
    
    // Verifica resultado esperado
    if (should_pass && result) {
        printf("✅ PASSOU: Código semanticamente correto como esperado\n");
    } else if (!should_pass && !result) {
        printf("✅ PASSOU: Erros detectados como esperado\n");
    } else if (should_pass && !result) {
        printf("❌ FALHOU: Código correto foi rejeitado\n");
    } else {
        printf("❌ FALHOU: Código incorreto foi aceito\n");
    }
    
    // Limpeza
    free_semantic_analyzer(analyzer);
    free_ast(ast);
    free_parser(parser);
    free_token_stream(tokens);
    free_afd(afd);
}

// ==================== TESTES ====================

void test_basic_declarations() {
    print_separator("TESTE 1: Declarações Básicas");
    
    const char* code = 
        "let x = 42;\n"
        "let y = 3.14;\n"
        "let name = \"DataLang\";\n"
        "let flag = true;\n";
    
    test_code("Declarações com Inferência", code, true);
}

void test_type_annotations() {
    print_separator("TESTE 2: Anotações de Tipo");
    
    const char* code = 
        "let x: Int = 42;\n"
        "let y: Float = 3.14;\n"
        "let name: String = \"DataLang\";\n"
        "let numbers: [Int] = [1, 2, 3];\n";
    
    test_code("Declarações com Anotações Explícitas", code, true);
}

void test_type_errors() {
    print_separator("TESTE 3: Erros de Tipo");
    
    const char* code = 
        "let x: Int = \"não é um inteiro\";\n"
        "let y: Float = true;\n";
    
    test_code("Tipos Incompatíveis", code, false);
}

void test_undeclared_variables() {
    print_separator("TESTE 4: Variáveis Não Declaradas");
    
    const char* code = 
        "let x = 10;\n"
        "let y = z + 5;\n";  // z não foi declarado
    
    test_code("Uso de Variável Não Declarada", code, false);
}

void test_function_declarations() {
    print_separator("TESTE 5: Declarações de Funções");
    
    const char* code = 
        "fn soma(a: Int, b: Int) -> Int {\n"
        "    return a + b;\n"
        "}\n"
        "\n"
        "let resultado = soma(5, 3);\n";
    
    test_code("Função com Tipo de Retorno", code, true);
}

void test_function_return_type_error() {
    print_separator("TESTE 6: Erro no Tipo de Retorno");
    
    const char* code = 
        "fn soma(a: Int, b: Int) -> Int {\n"
        "    return \"não é um inteiro\";\n"
        "}\n";
    
    test_code("Tipo de Retorno Incorreto", code, false);
}

void test_missing_return() {
    print_separator("TESTE 7: Retorno Ausente");
    
    const char* code = 
        "fn calcula(x: Int) -> Int {\n"
        "    let y = x * 2;\n"
        "}\n";  // Falta return
    
    test_code("Função Sem Return", code, false);
}

void test_arithmetic_operations() {
    print_separator("TESTE 8: Operações Aritméticas");
    
    const char* code = 
        "let a = 10;\n"
        "let b = 20;\n"
        "let c = a + b;\n"
        "let d = a * b;\n"
        "let e = b / a;\n";
    
    test_code("Operações Aritméticas Válidas", code, true);
}

void test_arithmetic_type_errors() {
    print_separator("TESTE 9: Erros em Operações Aritméticas");
    
    const char* code = 
        "let x = \"texto\";\n"
        "let y = 5;\n"
        "let z = x + y;\n";  // Não pode somar string com int
    
    test_code("Soma de Tipos Incompatíveis", code, false);
}

void test_comparison_operations() {
    print_separator("TESTE 10: Operações de Comparação");
    
    const char* code = 
        "let a = 10;\n"
        "let b = 20;\n"
        "let maior = a > b;\n"
        "let igual = a == 10;\n";
    
    test_code("Comparações Válidas", code, true);
}

void test_logical_operations() {
    print_separator("TESTE 11: Operações Lógicas");
    
    const char* code = 
        "let a = true;\n"
        "let b = false;\n"
        "let c = a && b;\n"
        "let d = a || b;\n"
        "let e = !a;\n";
    
    test_code("Operações Lógicas Válidas", code, true);
}

void test_logical_type_errors() {
    print_separator("TESTE 12: Erros em Operações Lógicas");
    
    const char* code = 
        "let x = 5;\n"
        "let y = 10;\n"
        "let z = x && y;\n";  // && requer booleanos
    
    test_code("Operação Lógica com Tipos Incorretos", code, false);
}

void test_array_literals() {
    print_separator("TESTE 13: Arrays Literais");
    
    const char* code = 
        "let numbers = [1, 2, 3, 4, 5];\n"
        "let names = [\"Alice\", \"Bob\", \"Carol\"];\n";
    
    test_code("Arrays com Tipos Homogêneos", code, true);
}

void test_array_type_errors() {
    print_separator("TESTE 14: Erros em Arrays");
    
    const char* code = 
        "let mixed = [1, \"dois\", 3];\n";  // Tipos heterogêneos
    
    test_code("Array com Tipos Misturados", code, false);
}

void test_scopes() {
    print_separator("TESTE 15: Escopos");
    
    const char* code = 
        "let x = 10;\n"
        "\n"
        "fn teste() -> Int {\n"
        "    let x = 20;\n"  // Shadowing permitido
        "    return x;\n"
        "}\n"
        "\n"
        "let y = teste();\n";
    
    test_code("Shadowing de Variáveis", code, true);
}

void test_complex_expressions() {
    print_separator("TESTE 16: Expressões Complexas");
    
    const char* code = 
        "let a = 5;\n"
        "let b = 10;\n"
        "let c = 15;\n"
        "let resultado = (a + b) * c - (b / 2);\n";
    
    test_code("Expressão Aritmética Complexa", code, true);
}

void test_if_statements() {
    print_separator("TESTE 17: Statements If");
    
    const char* code = 
        "let x = 10;\n"
        "\n"
        "if (x > 5) {\n"
        "    let y = x * 2;\n"
        "} else {\n"
        "    let z = x / 2;\n"
        "}\n";
    
    test_code("If-Else Válido", code, true);
}

void test_for_loops() {
    print_separator("TESTE 18: Loops For");
    
    const char* code = 
        "let numbers = [1, 2, 3, 4, 5];\n"
        "\n"
        "for item in numbers {\n"
        "    let doubled = item * 2;\n"
        "}\n";
    
    test_code("Loop For Válido", code, true);
}

void test_lambda_expressions() {
    print_separator("TESTE 19: Expressões Lambda");
    
    const char* code = 
        "let add = |a, b| a + b;\n"
        "let result = add(5, 3);\n";
    
    test_code("Lambda com Inferência", code, true);
}

void test_redeclaration_error() {
    print_separator("TESTE 20: Erro de Redeclaração");
    
    const char* code = 
        "let x = 10;\n"
        "let x = 20;\n";  // Redeclaração no mesmo escopo
    
    test_code("Redeclaração de Variável", code, false);
}

// ==================== MAIN ====================

// int main() {
//     printf("\n");
//     printf("╔════════════════════════════════════════════════════════════╗\n");
//     printf("║     SUITE DE TESTES - ANALISADOR SEMÂNTICO DATALANG       ║\n");
//     printf("╚════════════════════════════════════════════════════════════╝\n");
    
//     // Executa todos os testes
//     test_basic_declarations();
//     test_type_annotations();
//     test_type_errors();
//     test_undeclared_variables();
//     test_function_declarations();
//     test_function_return_type_error();
//     test_missing_return();
//     test_arithmetic_operations();
//     test_arithmetic_type_errors();
//     test_comparison_operations();
//     test_logical_operations();
//     test_logical_type_errors();
//     test_array_literals();
//     test_array_type_errors();
//     test_scopes();
//     test_complex_expressions();
//     test_if_statements();
//     test_for_loops();
//     test_lambda_expressions();
//     test_redeclaration_error();
    
//     print_separator("TESTES CONCLUÍDOS");
//     printf("Todos os testes foram executados!\n");
//     printf("Verifique os resultados acima para detalhes.\n\n");
    
//     return 0;
// }