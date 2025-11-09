#ifndef LEXER_H
#define LEXER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>
#include "datalang_afn.h"

// ==================== ESTRUTURAS DE DADOS ====================

// Buffer para armazenar arquivo em memória
typedef struct {
    char* data;
    size_t size;
    size_t capacity;
} FileBuffer;

// Stream de tokens
typedef struct {
    Token* tokens;
    int count;
    int capacity;
} TokenStream;

// Analisador léxico
typedef struct {
    const char* input;
    int position;
    int line;
    int column;
    int length;
    AFD* afd;
} Lexer;

// ==================== FUNÇÕES DE BUFFER ====================

/**
 * Lê um arquivo e carrega seu conteúdo em um buffer em memória
 * @param filename Nome do arquivo a ser lido
 * @return Ponteiro para FileBuffer ou NULL em caso de erro
 */
FileBuffer* read_file_to_buffer(const char* filename);

/**
 * Libera a memória alocada para um FileBuffer
 * @param buffer Buffer a ser liberado
 */
void free_file_buffer(FileBuffer* buffer);

// ==================== FUNÇÕES DE PALAVRAS-CHAVE ====================

/**
 * Busca uma palavra na tabela de palavras-chave
 * @param str String a ser verificada
 * @return TokenType correspondente ou TOKEN_IDENTIFIER se não for palavra-chave
 */
TokenType lookup_keyword(const char* str);

/**
 * Retorna o nome do tipo de token como string
 * @param type Tipo do token
 * @return Nome do tipo de token
 */
const char* token_type_name(TokenType type);

// ==================== FUNÇÕES DO LEXER ====================

/**
 * Cria um novo analisador léxico
 * @param input String de entrada a ser tokenizada
 * @param afd AFD para reconhecimento de tokens
 * @return Ponteiro para Lexer ou NULL em caso de erro
 */
Lexer* create_lexer(const char* input, AFD* afd);

/**
 * Libera a memória alocada para um Lexer
 * @param lexer Lexer a ser liberado
 */
void free_lexer(Lexer* lexer);

/**
 * Reconhece o próximo token na entrada usando o AFD
 * @param lexer Analisador léxico
 * @return Token reconhecido
 */
Token recognize_token(Lexer* lexer);

// ==================== FUNÇÕES DE STREAM DE TOKENS ====================

/**
 * Cria um novo stream de tokens
 * @return Ponteiro para TokenStream
 */
TokenStream* create_token_stream();

/**
 * Adiciona um token ao stream
 * @param stream Stream de tokens
 * @param token Token a ser adicionado
 */
void add_token(TokenStream* stream, Token token);

/**
 * Libera a memória alocada para um TokenStream
 * @param stream Stream de tokens a ser liberado
 */
void free_token_stream(TokenStream* stream);

/**
 * Tokeniza uma string de entrada completa
 * @param input String de entrada
 * @param afd AFD para reconhecimento
 * @return Stream de tokens resultante
 */
TokenStream* tokenize(const char* input, AFD* afd);

// ==================== INTEGRAÇÃO COM AFN/AFD ====================

/**
 * Cria o AFD unificado para a linguagem DataLang a partir do AFN
 * @return Ponteiro para AFD ou NULL em caso de erro
 */
AFD* create_datalang_afd_from_afn();

// ==================== FUNÇÕES DE VISUALIZAÇÃO ====================

/**
 * Imprime todos os tokens de um stream formatados
 * @param stream Stream de tokens a ser impresso
 */
void print_tokens(TokenStream* stream);

/**
 * Imprime informações sobre o AFD
 * @param afd AFD a ser analisado
 */
void print_afd_info(AFD* afd);

// ==================== FUNÇÕES DE TESTE ====================

/**
 * Testa o lexer com um arquivo específico
 * @param filename Nome do arquivo a ser testado
 */
void test_with_file(const char* filename);

/**
 * Teste com código simples
 */
void test_simple_code();

/**
 * Teste com código complexo
 */
void test_complex_code();

/**
 * Teste com casos especiais
 */
void test_edge_cases();

/**
 * Teste com tratamento de erros
 */
void test_error_handling();

#endif /* LEXER_H */