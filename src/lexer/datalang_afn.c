#ifndef DATALANG_AFN_H
#define DATALANG_AFN_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#define MAX_TRANSITIONS 256
#define EPSILON -1  // Representa transição epsilon

// Estrutura para conjunto de estados
typedef struct {
    int* states;
    int size;
    int capacity;
} StateSet;

// Estrutura para transição do AFN
typedef struct {
    int from_state;
    int input;  // EPSILON para ε-transições
    StateSet* to_states;
} AFNTransition;

// Estrutura para AFN
typedef struct {
    int num_states;
    int alphabet_size;
    AFNTransition** transitions;  // Array de listas de transições
    int* transition_counts;       // Contador de transições por estado
    bool* final_states;
    int start_state;
} AFN;

// Estrutura para AFD (compatível com a implementação anterior)
typedef struct {
    int num_states;
    int alphabet_size;
    int** transition_table;
    bool* final_states;
    int start_state;
} AFD;

// ==================== FUNÇÕES DE GERENCIAMENTO DE CONJUNTOS ====================

StateSet* create_state_set() {
    StateSet* set = (StateSet*)malloc(sizeof(StateSet));
    set->capacity = 10;
    set->size = 0;
    set->states = (int*)malloc(set->capacity * sizeof(int));
    return set;
}

void free_state_set(StateSet* set) {
    if (set) {
        free(set->states);
        free(set);
    }
}

void add_state(StateSet* set, int state) {
    // Verifica se o estado já existe
    for (int i = 0; i < set->size; i++) {
        if (set->states[i] == state) return;
    }
    
    // Expande o array se necessário
    if (set->size >= set->capacity) {
        set->capacity *= 2;
        set->states = (int *)realloc(set->states, set->capacity * sizeof(int));
    }
    
    set->states[set->size++] = state;
}

bool contains_state(StateSet* set, int state) {
    for (int i = 0; i < set->size; i++) {
        if (set->states[i] == state) return true;
    }
    return false;
}

StateSet* copy_state_set(StateSet* set) {
    StateSet* new_set = create_state_set();
    for (int i = 0; i < set->size; i++) {
        add_state(new_set, set->states[i]);
    }
    return new_set;
}

bool sets_equal(StateSet* a, StateSet* b) {
    if (a->size != b->size) return false;
    for (int i = 0; i < a->size; i++) {
        if (!contains_state(b, a->states[i])) return false;
    }
    return true;
}

// ==================== FUNÇÕES DO AFN ====================

AFN* create_afn(int num_states, int alphabet_size) {
    AFN* afn = (AFN*)malloc(sizeof(AFN));
    afn->num_states = num_states;
    afn->alphabet_size = alphabet_size;
    afn->start_state = 0;
    
    // Aloca array de transições
    afn->transitions = (AFNTransition**)calloc(num_states, sizeof(AFNTransition*));
    afn->transition_counts = (int *)calloc(num_states, sizeof(int));
    
    // Aloca estados finais
    afn->final_states = (bool *)calloc(num_states, sizeof(bool));
    
    return afn;
}

void add_afn_transition(AFN* afn, int from, int input, int to) {
    // Encontra ou cria a transição
    int found = -1;
    for (int i = 0; i < afn->transition_counts[from]; i++) {
        if (afn->transitions[from][i].input == input) {
            found = i;
            break;
        }
    }
    
    if (found == -1) {
        // Cria nova transição
        afn->transition_counts[from]++;
        afn->transitions[from] = (AFNTransition*)realloc(afn->transitions[from], 
                                         afn->transition_counts[from] * sizeof(AFNTransition));
        
        int idx = afn->transition_counts[from] - 1;
        afn->transitions[from][idx].from_state = from;
        afn->transitions[from][idx].input = input;
        afn->transitions[from][idx].to_states = create_state_set();
        add_state(afn->transitions[from][idx].to_states, to);
    } else {
        // Adiciona estado ao conjunto existente
        add_state(afn->transitions[from][found].to_states, to);
    }
}

void free_afn(AFN* afn) {
    if (afn) {
        for (int i = 0; i < afn->num_states; i++) {
            for (int j = 0; j < afn->transition_counts[i]; j++) {
                free_state_set(afn->transitions[i][j].to_states);
            }
            free(afn->transitions[i]);
        }
        free(afn->transitions);
        free(afn->transition_counts);
        free(afn->final_states);
        free(afn);
    }
}

// ==================== FECHO EPSILON ====================

StateSet* epsilon_closure(AFN* afn, StateSet* states) {
    StateSet* closure = copy_state_set(states);
    StateSet* to_process = copy_state_set(states);
    
    while (to_process->size > 0) {
        int current = to_process->states[--to_process->size];
        
        // Procura transições epsilon
        for (int i = 0; i < afn->transition_counts[current]; i++) {
            if (afn->transitions[current][i].input == EPSILON) {
                StateSet* epsilon_targets = afn->transitions[current][i].to_states;
                for (int j = 0; j < epsilon_targets->size; j++) {
                    int target = epsilon_targets->states[j];
                    if (!contains_state(closure, target)) {
                        add_state(closure, target);
                        add_state(to_process, target);
                    }
                }
            }
        }
    }
    
    free_state_set(to_process);
    return closure;
}

// ==================== CONVERSÃO AFN → AFD ====================

StateSet* move(AFN* afn, StateSet* states, int input) {
    StateSet* result = create_state_set();
    
    for (int i = 0; i < states->size; i++) {
        int state = states->states[i];
        
        for (int j = 0; j < afn->transition_counts[state]; j++) {
            if (afn->transitions[state][j].input == input) {
                StateSet* targets = afn->transitions[state][j].to_states;
                for (int k = 0; k < targets->size; k++) {
                    add_state(result, targets->states[k]);
                }
            }
        }
    }
    
    return result;
}

AFD* afn_to_afd(AFN* afn) {
    printf("Iniciando conversão AFN → AFD...\n");
    
    // Lista de conjuntos de estados do AFD
    StateSet** dfa_states = (StateSet**)malloc(1000 * sizeof(StateSet*));
    int num_dfa_states = 0;
    
    // Fila de estados não processados
    StateSet** unmarked = (StateSet**)malloc(1000 * sizeof(StateSet*));
    int unmarked_count = 0;
    
    // Estado inicial do AFD é o fecho epsilon do estado inicial do AFN
    StateSet* initial_set = create_state_set();
    add_state(initial_set, afn->start_state);
    StateSet* initial_closure = epsilon_closure(afn, initial_set);
    free_state_set(initial_set);
    
    dfa_states[num_dfa_states++] = initial_closure;
    unmarked[unmarked_count++] = initial_closure;
    
    // Tabela de transições do AFD
    int** dfa_transitions = (int**)malloc(1000 * sizeof(int*));
    for (int i = 0; i < 1000; i++) {
        dfa_transitions[i] = (int *)malloc(afn->alphabet_size * sizeof(int));
        for (int j = 0; j < afn->alphabet_size; j++) {
            dfa_transitions[i][j] = -1;  // Estado de erro
        }
    }
    
    // Algoritmo de construção de subconjuntos
    while (unmarked_count > 0) {
        StateSet* current = unmarked[--unmarked_count];
        int current_idx = -1;
        
        // Encontra índice do estado atual
        for (int i = 0; i < num_dfa_states; i++) {
            if (sets_equal(dfa_states[i], current)) {
                current_idx = i;
                break;
            }
        }
        
        printf("Processando estado %d (conjunto com %d estados do AFN)\n", 
               current_idx, current->size);
        
        // Para cada símbolo do alfabeto
        for (int symbol = 0; symbol < afn->alphabet_size; symbol++) {
            StateSet* next = move(afn, current, symbol);
            
            if (next->size == 0) {
                free_state_set(next);
                continue;
            }
            
            StateSet* next_closure = epsilon_closure(afn, next);
            free_state_set(next);
            
            // Verifica se o conjunto já existe
            int existing_idx = -1;
            for (int i = 0; i < num_dfa_states; i++) {
                if (sets_equal(dfa_states[i], next_closure)) {
                    existing_idx = i;
                    break;
                }
            }
            
            if (existing_idx == -1) {
                // Novo estado do AFD
                dfa_states[num_dfa_states] = next_closure;
                unmarked[unmarked_count++] = next_closure;
                dfa_transitions[current_idx][symbol] = num_dfa_states;
                num_dfa_states++;
            } else {
                // Estado já existe
                dfa_transitions[current_idx][symbol] = existing_idx;
                free_state_set(next_closure);
            }
        }
    }
    
    printf("Conversão concluída: %d estados no AFD\n", num_dfa_states);
    
    // Cria o AFD final
    AFD* afd = (AFD*)malloc(sizeof(AFD));
    afd->num_states = num_dfa_states;
    afd->alphabet_size = afn->alphabet_size;
    afd->start_state = 0;
    
    // Copia tabela de transições
    afd->transition_table = (int **)malloc(num_dfa_states * sizeof(int*));
    for (int i = 0; i < num_dfa_states; i++) {
        afd->transition_table[i] = (int *)malloc(afn->alphabet_size * sizeof(int));
        memcpy(afd->transition_table[i], dfa_transitions[i], 
               afn->alphabet_size * sizeof(int));
    }
    
    // Define estados finais
    afd->final_states = (bool *)malloc(num_dfa_states * sizeof(bool));
    for (int i = 0; i < num_dfa_states; i++) {
        afd->final_states[i] = false;
        for (int j = 0; j < dfa_states[i]->size; j++) {
            if (afn->final_states[dfa_states[i]->states[j]]) {
                afd->final_states[i] = true;
                break;
            }
        }
    }
    
    // Libera memória temporária
    for (int i = 0; i < 1000; i++) {
        free(dfa_transitions[i]);
    }
    free(dfa_transitions);
    free(unmarked);
    
    for (int i = 0; i < num_dfa_states; i++) {
        free_state_set(dfa_states[i]);
    }
    free(dfa_states);
    
    return afd;
}

// ==================== AFNs PARA PADRÕES COMPLEXOS ====================

// AFN para literais string com escape complexo
AFN* create_string_afn() {
    AFN* afn = create_afn(10, 256);
    
    // Estado 0: inicial
    add_afn_transition(afn, 0, '"', 1);
    
    // Estado 1: dentro da string
    for (int c = 0; c < 256; c++) {
        if (c != '"' && c != '\\' && c != '\n') {
            add_afn_transition(afn, 1, c, 1);
        }
    }
    
    // Transições para escape
    add_afn_transition(afn, 1, '\\', 2);
    
    // Estado 2: após backslash - aceita qualquer escape
    add_afn_transition(afn, 2, '"', 1);
    add_afn_transition(afn, 2, '\\', 1);
    add_afn_transition(afn, 2, 'n', 1);
    add_afn_transition(afn, 2, 't', 1);
    add_afn_transition(afn, 2, 'r', 1);
    add_afn_transition(afn, 2, '0', 1);
    
    // Unicode escape \uXXXX
    add_afn_transition(afn, 2, 'u', 3);
    for (int i = 0; i < 4; i++) {
        for (int c = '0'; c <= '9'; c++) add_afn_transition(afn, 3+i, c, 4+i);
        for (int c = 'a'; c <= 'f'; c++) add_afn_transition(afn, 3+i, c, 4+i);
        for (int c = 'A'; c <= 'F'; c++) add_afn_transition(afn, 3+i, c, 4+i);
    }
    add_afn_transition(afn, 7, EPSILON, 1);
    
    // Estado 1 -> Estado final com aspas de fechamento
    add_afn_transition(afn, 1, '"', 8);
    
    afn->final_states[8] = true;
    return afn;
}

// AFN para comentários (linha e bloco)
AFN* create_comment_afn() {
    AFN* afn = create_afn(20, 256);
    
    // Comentário de linha: // até newline
    add_afn_transition(afn, 0, '/', 1);
    add_afn_transition(afn, 1, '/', 2);
    
    for (int c = 0; c < 256; c++) {
        if (c != '\n' && c != '\r') {
            add_afn_transition(afn, 2, c, 2);
        }
    }
    add_afn_transition(afn, 2, '\n', 3);
    add_afn_transition(afn, 2, EPSILON, 3);  // EOF também aceita
    
    afn->final_states[3] = true;
    
    // Comentário de bloco: /* ... */
    add_afn_transition(afn, 1, '*', 10);
    
    for (int c = 0; c < 256; c++) {
        if (c != '*') {
            add_afn_transition(afn, 10, c, 10);
        }
    }
    
    add_afn_transition(afn, 10, '*', 11);
    add_afn_transition(afn, 11, '*', 11);  // Múltiplos asteriscos
    
    for (int c = 0; c < 256; c++) {
        if (c != '*' && c != '/') {
            add_afn_transition(afn, 11, c, 10);
        }
    }
    
    add_afn_transition(afn, 11, '/', 12);
    afn->final_states[12] = true;
    
    return afn;
}

// AFN para identificadores com Unicode (simplificado)
AFN* create_identifier_afn() {
    AFN* afn = create_afn(3, 256);
    
    // Estado 0 -> 1 com letra ou underscore
    for (int c = 'a'; c <= 'z'; c++) add_afn_transition(afn, 0, c, 1);
    for (int c = 'A'; c <= 'Z'; c++) add_afn_transition(afn, 0, c, 1);
    add_afn_transition(afn, 0, '_', 1);
    
    // Estado 1 -> 1 com letra, dígito ou underscore
    for (int c = 'a'; c <= 'z'; c++) add_afn_transition(afn, 1, c, 1);
    for (int c = 'A'; c <= 'Z'; c++) add_afn_transition(afn, 1, c, 1);
    for (int c = '0'; c <= '9'; c++) add_afn_transition(afn, 1, c, 1);
    add_afn_transition(afn, 1, '_', 1);
    
    afn->final_states[1] = true;
    return afn;
}

void free_afd(AFD* afd) {
    if (afd) {
        if (afd->transition_table) {
            for (int i = 0; i < afd->num_states; i++) {
                free(afd->transition_table[i]);
            }
            free(afd->transition_table);
        }
        free(afd->final_states);
        free(afd);
    }
}

#endif // DATALANG_AFN_H