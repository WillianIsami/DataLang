#ifndef DATALANG_AFN_H
#define DATALANG_AFN_H

#include <stdbool.h>

#define MAX_TRANSITIONS 256
#define EPSILON -1  // Representa transição epsilon

// ==================== ESTRUTURAS ====================

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

// Estrutura para AFD
typedef struct {
    int num_states;
    int alphabet_size;
    int** transition_table;
    bool* final_states;
    int start_state;
} AFD;

// ==================== DECLARAÇÕES DE FUNÇÕES ====================

// Funções de gerenciamento de conjuntos de estados
StateSet* create_state_set();
void free_state_set(StateSet* set);
void add_state(StateSet* set, int state);
bool contains_state(StateSet* set, int state);
StateSet* copy_state_set(StateSet* set);
bool sets_equal(StateSet* a, StateSet* b);

// Funções do AFN
AFN* create_afn(int num_states, int alphabet_size);
void add_afn_transition(AFN* afn, int from, int input, int to);
void free_afn(AFN* afn);

// Funções de fecho epsilon e movimento
StateSet* epsilon_closure(AFN* afn, StateSet* states);
StateSet* move(AFN* afn, StateSet* states, int input);

// Conversão AFN → AFD
AFD* afn_to_afd(AFN* afn);

// AFNs para padrões complexos (LEGADO - manter para compatibilidade)
AFN* create_string_afn();
AFN* create_comment_afn();
AFN* create_identifier_afn();

// Liberação de memória do AFD
void free_afd(AFD* afd);

#endif // DATALANG_AFN_H