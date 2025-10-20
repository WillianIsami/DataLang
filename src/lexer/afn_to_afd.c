#include "datalang_afn.h"
#include <stdio.h>

int find_state_set_index(StateSet** sets, int count, StateSet* target) {
    if (!sets || !target) return -1;
    
    for (int i = 0; i < count; i++) {
        if (sets[i] && sets_equal(sets[i], target)) {
            return i;
        }
    }
    return -1;
}

/* ============================================================================
   ALGORITMO DE CONVERSÃO AFN → AFD (Construção de Subconjuntos)
   ============================================================================ */

AFD* afn_to_afd(AFN* afn) {
    if (!afn) {
        fprintf(stderr, "ERRO: AFN nulo\n");
        return NULL;
    }
    
    printf("\n╔════════════════════════════════════════════════════════════╗\n");
    printf("║  CONVERSÃO AFN → AFD (Construção de Subconjuntos)        ║\n");
    printf("╚════════════════════════════════════════════════════════════╝\n\n");
    
    printf("AFN de entrada:\n");
    printf("  - Estados: %d\n", afn->num_states);
    printf("  - Alfabeto: %d\n", afn->alphabet_size);
    printf("  - Estado inicial: %d\n", afn->start_state);
    
    /* ────────────────────────────────────────────────────────────
       FASE 1: INICIALIZAÇÃO
       ──────────────────────────────────────────────────────────── */
    
    printf("\n[FASE 1] Inicializando...\n");
    printf("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n");
    
    // Cria estado inicial do AFD
    StateSet* initial_afn = create_state_set();
    add_state(initial_afn, afn->start_state);
    
    StateSet* q0_afd = epsilon_closure(afn, initial_afn);
    free_state_set(initial_afn);
    
    printf("Estado inicial do AFD (ε-closure): { ");
    for (int i = 0; i < q0_afd->size; i++) {
        printf("%d ", q0_afd->states[i]);
    }
    printf("}\n");
    
    // Estruturas para construção
    StateSet** afd_states = (StateSet**)malloc(2000 * sizeof(StateSet*));
    int afd_state_count = 0;
    int worklist[2000];
    int worklist_count = 0;
    
    // Adiciona estado inicial
    afd_states[afd_state_count] = q0_afd;
    worklist[worklist_count++] = afd_state_count;
    afd_state_count++;
    
    // Cria AFD
    AFD* afd = (AFD*)malloc(sizeof(AFD));
    if (!afd) {
        fprintf(stderr, "ERRO: Falha ao alocar AFD\n");
        return NULL;
    }
    
    afd->num_states = 2000;
    afd->alphabet_size = afn->alphabet_size;
    afd->start_state = 0;
    
    // Aloca tabela de transições
    afd->transition_table = (int**)malloc(2000 * sizeof(int*));
    for (int i = 0; i < 2000; i++) {
        afd->transition_table[i] = (int*)malloc(256 * sizeof(int));
        for (int j = 0; j < 256; j++) {
            afd->transition_table[i][j] = -1;
        }
    }
    
    afd->final_states = (bool*)calloc(2000, sizeof(bool));
    afd->token_types = (TokenType*)malloc(2000 * sizeof(TokenType));
    for (int i = 0; i < 2000; i++) {
        afd->token_types[i] = TOKEN_ERROR;
    }
    
    /* ────────────────────────────────────────────────────────────
       FASE 2: CONSTRUÇÃO ITERATIVA
       ──────────────────────────────────────────────────────────── */
    
    printf("\n[FASE 2] Construção iterativa...\n");
    printf("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n");
    
    int iteration = 0;
    while (worklist_count > 0) {
        int current_idx = worklist[--worklist_count];
        StateSet* current_set = afd_states[current_idx];
        
        iteration++;
        if (iteration <= 20 || iteration % 50 == 0) {  // Log reduzido
            printf("[Iter %d] Processando estado AFD %d: { ", iteration, current_idx);
            for (int i = 0; i < current_set->size && i < 5; i++) {
                printf("%d ", current_set->states[i]);
            }
            if (current_set->size > 5) printf("...");
            printf("}\n");
        }
        
        // Para cada símbolo do alfabeto
        for (int symbol = 0; symbol < 256; symbol++) {
            if (symbol == EPSILON) continue; // Skip epsilon
            
            // Calcula MOVE(S, a)
            StateSet* moved = move(afn, current_set, symbol);
            
            if (moved->size > 0) {
                // Calcula ε-closure(MOVE(S, a))
                StateSet* new_set = epsilon_closure(afn, moved);
                free_state_set(moved);
                
                // Procura se já existe
                int existing_idx = find_state_set_index(afd_states, afd_state_count, new_set);
                
                if (existing_idx == -1) {
                    // Novo estado descoberto
                    afd_states[afd_state_count] = new_set;
                    afd->transition_table[current_idx][symbol] = afd_state_count;
                    worklist[worklist_count++] = afd_state_count;
                    
                    if (iteration <= 20 || iteration % 50 == 0) {
                        printf("  '%c' (0x%02x) → novo estado %d\n", 
                               (symbol >= 32 && symbol < 127) ? (char)symbol : '?',
                               symbol, afd_state_count);
                    }
                    
                    afd_state_count++;
                } else {
                    // Estado existente
                    afd->transition_table[current_idx][symbol] = existing_idx;
                    free_state_set(new_set);
                }
            } else {
                free_state_set(moved);
                afd->transition_table[current_idx][symbol] = -1;
            }
        }
    }
    
    printf("\n  Iterações totais: %d\n", iteration);
    
    /* ────────────────────────────────────────────────────────────
       FASE 3: IDENTIFICAÇÃO DE ESTADOS FINAIS
       ──────────────────────────────────────────────────────────── */
    
    printf("\n[FASE 3] Identificando estados finais...\n");
    printf("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n");
    
    int final_count = 0;
    for (int i = 0; i < afd_state_count; i++) {
        StateSet* state_set = afd_states[i];
        
        // Verifica se contém algum estado final do AFN
        // Prioridade: escolhe o tipo de token da ordem de prioridade
        TokenType best_type = TOKEN_ERROR;
        int best_priority = 999;
        
        // Na função afn_to_afd, modifique a seção de prioridade:
        const TokenType priority_order[] = {
            TOKEN_STRING,       // 0 - Strings primeiro (maior prioridade)
            TOKEN_COMMENT,      // 1 - Comentários
            TOKEN_FLOAT,        // 2 - Números decimais
            TOKEN_INTEGER,      // 3 - Inteiros
            TOKEN_OPERATOR,     // 4 - Operadores
            TOKEN_DELIMITER,    // 5 - Delimitadores
            TOKEN_IDENTIFIER,   // 6 - Identificadores
            TOKEN_WHITESPACE    // 7 - Whitespace (menor prioridade)
        };
        
        for (int j = 0; j < state_set->size; j++) {
            int afn_state = state_set->states[j];
            
            if (afn->final_states[afn_state]) {
                TokenType type = afn->token_types[afn_state];
                
                // Encontra prioridade
                for (int p = 0; p < 8; p++) {
                    if (priority_order[p] == type && p < best_priority) {
                        best_type = type;
                        best_priority = p;
                    }
                }
            }
        }
        
        if (best_type != TOKEN_ERROR) {
            afd->final_states[i] = true;
            afd->token_types[i] = best_type;
            final_count++;
            
            if (final_count <= 30) {
                printf("  Estado AFD %d é final → %d (%s)\n", i, best_type,
                       (best_type == TOKEN_INTEGER ? "INTEGER" :
                        best_type == TOKEN_FLOAT ? "FLOAT" :
                        best_type == TOKEN_STRING ? "STRING" :
                        best_type == TOKEN_IDENTIFIER ? "IDENTIFIER" :
                        best_type == TOKEN_OPERATOR ? "OPERATOR" :
                        best_type == TOKEN_DELIMITER ? "DELIMITER" :
                        best_type == TOKEN_COMMENT ? "COMMENT" :
                        best_type == TOKEN_WHITESPACE ? "WHITESPACE" : "???"));
            }
        }
    }
    
    printf("  Total de estados finais: %d\n", final_count);
    
    /* ────────────────────────────────────────────────────────────
       FASE 4: FINALIZAÇÃO
       ──────────────────────────────────────────────────────────── */
    
    printf("\n[FASE 4] Finalizando...\n");
    printf("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n");
    
    // Redimensiona para o número real de estados
    afd->num_states = afd_state_count;
    
    // Libera conjuntos de estados
    for (int i = 0; i < afd_state_count; i++) {
        free_state_set(afd_states[i]);
    }
    free(afd_states);
    
    printf("\n Conversão concluída com sucesso!\n");
    printf(" AFD resultante:\n");
    printf("  - Estados: %d\n", afd->num_states);
    printf("  - Estados finais: %d\n", final_count);
    printf("  - Alfabeto: %d\n", afd->alphabet_size);
    
    printf("\n╚════════════════════════════════════════════════════════════╝\n\n");
    
    return afd;
}