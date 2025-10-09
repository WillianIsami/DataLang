#include "datalang_afn.h"
#include <stdio.h>

// Função auxiliar: encontra índice de um StateSet no array
int find_state_set_index(StateSet** sets, int count, StateSet* target) {
    for (int i = 0; i < count; i++) {
        if (sets_equal(sets[i], target)) {
            return i;
        }
    }
    return -1;
}

// Algoritmo de conversão AFN para AFD
AFD* afn_to_afd(AFN* afn) {
    if (!afn) return NULL;
    
    printf("Iniciando conversão AFN->AFD...\n");
    printf("AFN: %d estados, alfabeto: %d\n", afn->num_states, afn->alphabet_size);
    
    // Fase 1: Inicialização
    StateSet* start_set = create_state_set();
    add_state(start_set, afn->start_state);
    StateSet* start_closure = epsilon_closure(afn, start_set);
    free_state_set(start_set);
    
    printf("Estado inicial do AFD: ");
    for (int i = 0; i < start_closure->size; i++) {
        printf("%d ", start_closure->states[i]);
    }
    printf("\n");
    
    // Estruturas para construção do AFD
    StateSet** afd_states = malloc(1000 * sizeof(StateSet*));
    int afd_state_count = 0;
    int worklist[1000];
    int worklist_count = 0;
    
    // Adiciona estado inicial
    afd_states[afd_state_count] = start_closure;
    worklist[worklist_count++] = afd_state_count;
    afd_state_count++;
    
    // Cria AFD
    AFD* afd = create_afd(1000, afn->alphabet_size);
    if (!afd) {
        printf("Erro ao criar AFD\n");
        return NULL;
    }
    afd->start_state = 0;
    
    // Fase 2: Construção iterativa
    while (worklist_count > 0) {
        int current_index = worklist[--worklist_count];
        StateSet* current_set = afd_states[current_index];
        
        printf("Processando estado AFD %d: ", current_index);
        for (int i = 0; i < current_set->size; i++) {
            printf("%d ", current_set->states[i]);
        }
        printf("\n");
        
        // Para cada símbolo do alfabeto (exceto epsilon)
        for (int symbol = 0; symbol < afn->alphabet_size; symbol++) {
            if (symbol == EPSILON) continue;
            
            // Calcula MOVE e depois epsilon-closure
            StateSet* moved = move(afn, current_set, symbol);
            if (moved->size > 0) {
                StateSet* new_set = epsilon_closure(afn, moved);
                free_state_set(moved);
                
                // Verifica se é um novo estado
                int existing_index = find_state_set_index(afd_states, afd_state_count, new_set);
                
                if (existing_index == -1) {
                    // Novo estado
                    afd_states[afd_state_count] = new_set;
                    afd->transition_table[current_index][symbol] = afd_state_count;
                    worklist[worklist_count++] = afd_state_count;
                    
                    printf("  '%c' -> novo estado %d: ", (char)symbol, afd_state_count);
                    for (int i = 0; i < new_set->size; i++) {
                        printf("%d ", new_set->states[i]);
                    }
                    printf("\n");
                    
                    afd_state_count++;
                } else {
                    // Estado já existe
                    afd->transition_table[current_index][symbol] = existing_index;
                    free_state_set(new_set);
                    
                    // printf("  '%c' -> estado existente %d\n", (char)symbol, existing_index);
                }
            } else {
                free_state_set(moved);
                afd->transition_table[current_index][symbol] = -1; // Rejeição
            }
        }
    }
    
    // Fase 3: Estados finais e tipos de token
    printf("Definindo estados finais...\n");
    for (int i = 0; i < afd_state_count; i++) {
        StateSet* state_set = afd_states[i];
        
        // Verifica se contém algum estado final do AFN
        for (int j = 0; j < state_set->size; j++) {
            int afn_state = state_set->states[j];
            if (afn->final_states[afn_state]) {
                afd->final_states[i] = true;
                afd->token_types[i] = afn->token_types[afn_state];
                
                printf("Estado AFD %d é final -> %d\n", i, afd->token_types[i]);
                break;
            }
        }
    }
    
    // Redimensiona AFD para o número real de estados
    afd->num_states = afd_state_count;
    
    // Limpeza
    for (int i = 0; i < afd_state_count; i++) {
        free_state_set(afd_states[i]);
    }
    free(afd_states);
    
    printf("Conversão concluída: AFD com %d estados\n", afd_state_count);
    return afd;
}

// Implementação das funções do AFD
AFD* create_afd(int num_states, int alphabet_size) {
    AFD* afd = malloc(sizeof(AFD));
    if (!afd) return NULL;
    
    afd->num_states = num_states;
    afd->alphabet_size = alphabet_size;
    afd->start_state = 0;
    
    // Aloca tabela de transições
    afd->transition_table = malloc(num_states * sizeof(int*));
    for (int i = 0; i < num_states; i++) {
        afd->transition_table[i] = malloc(alphabet_size * sizeof(int));
        for (int j = 0; j < alphabet_size; j++) {
            afd->transition_table[i][j] = -1; // Rejeição padrão
        }
    }
    
    // Aloca arrays de estados finais e tipos
    afd->final_states = calloc(num_states, sizeof(bool));
    afd->token_types = malloc(num_states * sizeof(TokenType));
    for (int i = 0; i < num_states; i++) {
        afd->token_types[i] = TOKEN_ERROR;
    }
    
    return afd;
}