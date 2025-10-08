/*
 * DataLang - Conversão AFN -> AFD (Algoritmo de Construção de Subconjuntos)
 * 
 * Este arquivo implementa o algoritmo de construção de subconjuntos (subset construction)
 * para converter um AFN unificado em um AFD determinístico equivalente.
 * 
 * Baseado na teoria de Linguagens Formais e Autômatos.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "datalang_afn.h"

#define MAX_AFD_STATES 1000

// ==================== ESTRUTURAS ESPECÍFICAS PARA A CONVERSÃO ====================

// Estrutura auxiliar para o AFD durante a conversão
typedef struct {
    StateSet** dfa_states;      // Conjunto de estados do AFD (cada um é um conjunto de estados do AFN)
    int num_dfa_states;
    int capacity;
    int** transition_table;     // Tabela de transições [estado_afd][símbolo] -> próximo_estado_afd
    bool* final_states;         // Estados finais do AFD
    TokenType* token_types;     // Tipo de token para cada estado final
    int start_state;            // Estado inicial (sempre 0)
    int alphabet_size;
} AFDConverter;

// ==================== FUNÇÕES AUXILIARES ESPECÍFICAS ====================

// Adiciona estado ao conjunto (evita duplicatas) - versão específica para conversão
bool add_state_to_set(StateSet* set, int state) {
    // Verifica se já existe
    for (int i = 0; i < set->size; i++) {
        if (set->states[i] == state) return false;
    }
    
    // Expande se necessário
    if (set->size >= set->capacity) {
        set->capacity *= 2;
        set->states = (int*)realloc(set->states, set->capacity * sizeof(int));
    }
    
    set->states[set->size++] = state;
    return true;
}

// Ordena os estados em um conjunto (para normalização)
void sort_state_set(StateSet* set) {
    for (int i = 0; i < set->size - 1; i++) {
        for (int j = i + 1; j < set->size; j++) {
            if (set->states[i] > set->states[j]) {
                int temp = set->states[i];
                set->states[i] = set->states[j];
                set->states[j] = temp;
            }
        }
    }
}

// Imprime conjunto de estados (para debug)
void print_state_set(StateSet* set) {
    printf("{");
    for (int i = 0; i < set->size; i++) {
        printf("%d", set->states[i]);
        if (i < set->size - 1) printf(", ");
    }
    printf("}");
}

/*
 * MOVE(T, a) - Versão otimizada para conversão
 * Calcula o conjunto de estados alcançáveis a partir de T lendo o símbolo 'a'.
 */
StateSet* move_set(AFN* afn, StateSet* T, int symbol) {
    StateSet* result = create_state_set();
    
    // Para cada estado em T
    for (int i = 0; i < T->size; i++) {
        int state = T->states[i];
        
        // Para cada transição do estado
        for (int j = 0; j < afn->transition_counts[state]; j++) {
            AFNTransition* trans = &afn->transitions[state][j];
            
            // Se a transição é com o símbolo 'symbol'
            if (trans->input == symbol) {
                // Adiciona todos os estados destino
                for (int k = 0; k < trans->to_states->size; k++) {
                    add_state(result, trans->to_states->states[k]);
                }
            }
        }
    }
    
    sort_state_set(result);
    return result;
}

// ==================== CRIAÇÃO E GERENCIAMENTO DO CONVERSOR AFD ====================

AFDConverter* create_afd_converter(int alphabet_size) {
    AFDConverter* converter = (AFDConverter*)malloc(sizeof(AFDConverter));
    if (!converter) return NULL;
    
    converter->capacity = 100;
    converter->num_dfa_states = 0;
    converter->alphabet_size = alphabet_size;
    converter->start_state = 0;
    
    // Aloca arrays principais
    converter->dfa_states = (StateSet**)malloc(converter->capacity * sizeof(StateSet*));
    converter->transition_table = (int**)malloc(converter->capacity * sizeof(int*));
    converter->final_states = (bool*)malloc(converter->capacity * sizeof(bool));
    converter->token_types = (TokenType*)malloc(converter->capacity * sizeof(TokenType));
    
    // Inicializa tabela de transições
    for (int i = 0; i < converter->capacity; i++) {
        converter->transition_table[i] = (int*)malloc(alphabet_size * sizeof(int));
        for (int j = 0; j < alphabet_size; j++) {
            converter->transition_table[i][j] = -1; // Estado de erro
        }
        converter->final_states[i] = false;
        converter->token_types[i] = TOKEN_ERROR;
    }
    
    return converter;
}

void free_afd_converter(AFDConverter* converter) {
    if (!converter) return;
    
    for (int i = 0; i < converter->num_dfa_states; i++) {
        free_state_set(converter->dfa_states[i]);
        free(converter->transition_table[i]);
    }
    
    free(converter->dfa_states);
    free(converter->transition_table);
    free(converter->final_states);
    free(converter->token_types);
    free(converter);
}

// Encontra índice de um conjunto de estados no AFD (-1 se não existe)
int find_dfa_state(AFDConverter* converter, StateSet* set) {
    for (int i = 0; i < converter->num_dfa_states; i++) {
        if (sets_equal(converter->dfa_states[i], set)) {
            return i;
        }
    }
    return -1;
}

// Adiciona novo estado ao AFD
int add_dfa_state(AFDConverter* converter, StateSet* set) {
    if (converter->num_dfa_states >= converter->capacity) {
        // Expande capacidade
        converter->capacity *= 2;
        converter->dfa_states = (StateSet**)realloc(converter->dfa_states, converter->capacity * sizeof(StateSet*));
        converter->transition_table = (int**)realloc(converter->transition_table, converter->capacity * sizeof(int*));
        converter->final_states = (bool*)realloc(converter->final_states, converter->capacity * sizeof(bool));
        converter->token_types = (TokenType*)realloc(converter->token_types, converter->capacity * sizeof(TokenType));
        
        // Inicializa novos espaços
        for (int i = converter->num_dfa_states; i < converter->capacity; i++) {
            converter->transition_table[i] = (int*)malloc(converter->alphabet_size * sizeof(int));
            for (int j = 0; j < converter->alphabet_size; j++) {
                converter->transition_table[i][j] = -1;
            }
            converter->final_states[i] = false;
            converter->token_types[i] = TOKEN_ERROR;
        }
    }
    
    converter->dfa_states[converter->num_dfa_states] = copy_state_set(set);
    return converter->num_dfa_states++;
}

// ==================== ALGORITMO PRINCIPAL: CONSTRUÇÃO DE SUBCONJUNTOS ====================

/*
 * ALGORITMO DE CONSTRUÇÃO DE SUBCONJUNTOS (Subset Construction)
 * 
 * Converte um AFN em um AFD equivalente.
 * 
 * Fases:
 * 1. Inicialização: Calcula estado inicial do AFD (ε-closure do estado inicial do AFN)
 * 2. Expansão: Para cada estado não processado, calcula transições para todos os símbolos
 * 3. Identificação de estados finais: Marca estados que contêm estados finais do AFN
 * 4. Construção final: Retorna o AFD completo
 */
AFD* afn_to_afd(AFN* afn) {
    printf("\n╔════════════════════════════════════════════════════════════╗\n");
    printf("║     CONVERSÃO AFN -> AFD (Construção de Subconjuntos)      ║\n");
    printf("╚════════════════════════════════════════════════════════════╝\n\n");
    
    printf("AFN de entrada:\n");
    printf("  - Estados: %d\n", afn->num_states);
    printf("  - Alfabeto: %d símbolos\n", afn->alphabet_size);
    printf("  - Estado inicial: %d\n\n", afn->start_state);
    
    // ===== FASE 1: INICIALIZAÇÃO =====
    printf("─────────────────────────────────────────────────────────────\n");
    printf("FASE 1: INICIALIZAÇÃO\n");
    printf("─────────────────────────────────────────────────────────────\n");
    
    AFDConverter* converter = create_afd_converter(afn->alphabet_size);
    
    // Estado inicial: ε-closure({q0})
    StateSet* initial_set = create_state_set();
    add_state(initial_set, afn->start_state);
    StateSet* initial_closure = epsilon_closure(afn, initial_set);
    
    printf("Estado inicial do AFN: {%d}\n", afn->start_state);
    printf("ε-closure({%d}): ", afn->start_state);
    print_state_set(initial_closure);
    printf("\n\n");
    
    add_dfa_state(converter, initial_closure);
    
    // WorkList: estados a processar
    int* worklist = (int*)malloc(MAX_AFD_STATES * sizeof(int));
    int worklist_size = 1;
    worklist[0] = 0;
    
    bool* processed = (bool*)calloc(MAX_AFD_STATES, sizeof(bool));
    
    free_state_set(initial_set);
    free_state_set(initial_closure);
    
    // ===== FASE 2: EXPANSÃO ITERATIVA =====
    printf("─────────────────────────────────────────────────────────────\n");
    printf("FASE 2: EXPANSÃO ITERATIVA\n");
    printf("─────────────────────────────────────────────────────────────\n\n");
    
    int iteration = 0;
    
    while (worklist_size > 0) {
        // Remove estado da worklist
        int current_dfa_state = worklist[--worklist_size];
        
        if (processed[current_dfa_state]) continue;
        processed[current_dfa_state] = true;
        
        printf("Iteração %d: Processando estado D%d = ", ++iteration, current_dfa_state);
        print_state_set(converter->dfa_states[current_dfa_state]);
        printf("\n");
        
        // Para cada símbolo do alfabeto (exceto ε)
        int transitions_created = 0;
        
        for (int symbol = 0; symbol < afn->alphabet_size; symbol++) {
            // Calcula MOVE(S, symbol)
            StateSet* moved = move_set(afn, converter->dfa_states[current_dfa_state], symbol);
            
            if (moved->size == 0) {
                free_state_set(moved);
                continue;
            }
            
            // Calcula ε-closure(MOVE(S, symbol))
            StateSet* next_closure = epsilon_closure(afn, moved);
            free_state_set(moved);
            
            // Verifica se já existe
            int next_dfa_state = find_dfa_state(converter, next_closure);
            
            if (next_dfa_state == -1) {
                // Novo estado descoberto
                next_dfa_state = add_dfa_state(converter, next_closure);
                worklist[worklist_size++] = next_dfa_state;
                
                // printf("  símbolo '%c' -> D%d ", symbol, next_dfa_state);
                // print_state_set(next_closure);
                // printf(" [NOVO]\n");
            } else {
                // printf("  símbolo '%c' -> D%d (já existe)\n", symbol, next_dfa_state);
            }
            
            // Define transição no AFD
            converter->transition_table[current_dfa_state][symbol] = next_dfa_state;
            transitions_created++;
            
            free_state_set(next_closure);
        }
        
        if (transitions_created == 0) {
            printf("  (nenhuma transição válida)\n");
        }
        printf("\n");
    }
    
    free(worklist);
    free(processed);
    
    // ===== FASE 3: IDENTIFICAÇÃO DE ESTADOS FINAIS =====
    printf("─────────────────────────────────────────────────────────────\n");
    printf("FASE 3: IDENTIFICAÇÃO DE ESTADOS FINAIS\n");
    printf("─────────────────────────────────────────────────────────────\n\n");
    
    for (int i = 0; i < converter->num_dfa_states; i++) {
        StateSet* dfa_state = converter->dfa_states[i];
        
        // Verifica se contém algum estado final do AFN
        for (int j = 0; j < dfa_state->size; j++) {
            int afn_state = dfa_state->states[j];
            
            if (afn->final_states[afn_state]) {
                converter->final_states[i] = true;
                converter->token_types[i] = afn->token_types[afn_state];
                
                printf("D%d = ", i);
                print_state_set(dfa_state);
                printf(" é FINAL (contém q%d) -> Token: %d\n", afn_state, converter->token_types[i]);
                break;
            }
        }
    }
    
    // ===== FASE 4: CONVERSÃO PARA AFD FINAL =====
    printf("\n─────────────────────────────────────────────────────────────\n");
    printf("FASE 4: CONVERSÃO PARA AFD FINAL\n");
    printf("─────────────────────────────────────────────────────────────\n\n");
    
    // Cria o AFD final
    AFD* afd = (AFD*)malloc(sizeof(AFD));
    afd->num_states = converter->num_dfa_states;
    afd->alphabet_size = afn->alphabet_size;
    afd->start_state = 0;
    
    // Copia tabela de transições
    afd->transition_table = (int**)malloc(afd->num_states * sizeof(int*));
    for (int i = 0; i < afd->num_states; i++) {
        afd->transition_table[i] = (int*)malloc(afd->alphabet_size * sizeof(int));
        memcpy(afd->transition_table[i], converter->transition_table[i], 
               afd->alphabet_size * sizeof(int));
    }
    
    // Copia estados finais e tipos de token
    afd->final_states = (bool*)malloc(afd->num_states * sizeof(bool));
    afd->token_types = (TokenType*)malloc(afd->num_states * sizeof(TokenType));
    
    for (int i = 0; i < afd->num_states; i++) {
        afd->final_states[i] = converter->final_states[i];
        afd->token_types[i] = converter->token_types[i];
    }
    
    // ===== RELATÓRIO FINAL =====
    printf("Estatísticas:\n");
    printf("  - Estados do AFD: %d\n", afd->num_states);
    printf("  - Estados finais: ");
    int final_count = 0;
    for (int i = 0; i < afd->num_states; i++) {
        if (afd->final_states[i]) final_count++;
    }
    printf("%d\n", final_count);
    printf("  - Redução: %.1f%%\n", 
           100.0 * (1.0 - (double)afd->num_states / afn->num_states));
    
    printf("\n✓ Conversão concluída com sucesso!\n\n");
    
    // Libera o conversor
    free_afd_converter(converter);
    
    return afd;
}

// ==================== FUNÇÕES DE VISUALIZAÇÃO ====================

void print_afd_table(AFD* afd) {
    printf("\nTabela de Transições do AFD:\n");
    printf("─────────────────────────────────────────────────────────────\n");
    
    // Cabeçalho
    printf("Estado │ ");
    for (int i = 0; i < afd->alphabet_size && i < 10; i++) {
        printf("'%c' ", (char)i);
    }
    printf("│ Final? │ Token\n");
    printf("───────┼");
    for (int i = 0; i < afd->alphabet_size && i < 10; i++) {
        printf("────");
    }
    printf("┼────────┼──────\n");
    
    // Linhas
    for (int i = 0; i < afd->num_states; i++) {
        printf("  D%-3d │ ", i);
        
        for (int j = 0; j < afd->alphabet_size && j < 10; j++) {
            int next = afd->transition_table[i][j];
            if (next >= 0) {
                printf("D%-2d ", next);
            } else {
                printf(" -  ");
            }
        }
        
        printf("│ %-6s │ ", afd->final_states[i] ? "✓" : " ");
        if (afd->final_states[i]) {
            printf("%d", afd->token_types[i]);
        } else {
            printf("-");
        }
        printf("\n");
    }
    
    printf("─────────────────────────────────────────────────────────────\n");
}

#ifdef TEST_AFN_TO_AFD
int main() {
    printf("Teste do Conversor AFN -> AFD\n");
    printf("================================\n\n");
    
    // Cria um AFN unificado de exemplo
    AFN* afn = create_unified_datalang_afn();
    
    if (!afn) {
        printf("Erro ao criar AFN unificado\n");
        return 1;
    }
    
    printf("   AFN unificado criado com sucesso:\n");
    printf("   - Estados: %d\n", afn->num_states);
    printf("   - Alfabeto: %d símbolos\n", afn->alphabet_size);
    printf("   - Estado inicial: %d\n\n", afn->start_state);
    
    // Converte para AFD
    AFD* afd = afn_to_afd(afn);
    
    if (!afd) {
        printf("Erro na conversão AFN -> AFD\n");
        free_afn(afn);
        return 1;
    }
    
    // Exibe a tabela do AFD
    print_afd_table(afd);
    
    // Limpeza
    free_afd(afd);
    free_afn(afn);
    
    printf("Teste concluído com sucesso!\n");
    return 0;
}
#endif