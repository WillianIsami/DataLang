#include "datalang_afn.h"
#include <stdio.h>

/* ============================================================================
   FUNÇÕES DE GERENCIAMENTO DE CONJUNTOS DE ESTADOS
   ============================================================================ */

StateSet* create_state_set() {
    StateSet* set = (StateSet*)malloc(sizeof(StateSet));
    if (!set) return NULL;
    
    set->capacity = 32;
    set->size = 0;
    set->states = (int*)malloc(set->capacity * sizeof(int));
    
    if (!set->states) {
        free(set);
        return NULL;
    }
    return set;
}

void free_state_set(StateSet* set) {
    if (set) {
        free(set->states);
        free(set);
    }
}

void add_state(StateSet* set, int state) {
    if (!set) return;
    
    // Verifica se já existe
    for (int i = 0; i < set->size; i++) {
        if (set->states[i] == state) return;
    }
    
    // Expande se necessário
    if (set->size >= set->capacity) {
        set->capacity *= 2;
        set->states = (int*)realloc(set->states, set->capacity * sizeof(int));
    }
    
    set->states[set->size++] = state;
}

bool contains_state(StateSet* set, int state) {
    if (!set) return false;
    
    for (int i = 0; i < set->size; i++) {
        if (set->states[i] == state) return true;
    }
    return false;
}

StateSet* copy_state_set(StateSet* set) {
    if (!set) return NULL;
    
    StateSet* new_set = create_state_set();
    if (!new_set) return NULL;
    
    for (int i = 0; i < set->size; i++) {
        add_state(new_set, set->states[i]);
    }
    return new_set;
}

bool sets_equal(StateSet* a, StateSet* b) {
    if (!a || !b) return false;
    if (a->size != b->size) return false;
    
    for (int i = 0; i < a->size; i++) {
        if (!contains_state(b, a->states[i])) return false;
    }
    return true;
}

/* ============================================================================
   FUNÇÕES DO AFN
   ============================================================================ */

AFN* create_afn(int num_states, int alphabet_size) {
    AFN* afn = (AFN*)malloc(sizeof(AFN));
    if (!afn) return NULL;
    
    afn->num_states = num_states;
    afn->alphabet_size = alphabet_size;
    afn->start_state = 0;
    
    // Aloca transitions
    afn->transitions = (AFNTransition**)calloc(num_states, sizeof(AFNTransition*));
    afn->transition_counts = (int*)calloc(num_states, sizeof(int));
    
    // Aloca estados finais
    afn->final_states = (bool*)calloc(num_states, sizeof(bool));
    
    // Aloca tipos de token
    afn->token_types = (TokenType*)calloc(num_states, sizeof(TokenType));
    
    for (int i = 0; i < num_states; i++) {
        afn->token_types[i] = TOKEN_ERROR;
    }
    
    return afn;
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
        free(afn->token_types);
        free(afn);
    }
}

void add_afn_transition(AFN* afn, int from, int input, int to) {
    if (!afn || from >= afn->num_states || to >= afn->num_states) return;
    
    // Procura transição existente com mesmo input
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
        afn->transitions[from] = (AFNTransition*)realloc(
            afn->transitions[from],
            afn->transition_counts[from] * sizeof(AFNTransition)
        );
        
        int idx = afn->transition_counts[from] - 1;
        afn->transitions[from][idx].input = input;
        afn->transitions[from][idx].to_states = create_state_set();
        add_state(afn->transitions[from][idx].to_states, to);
    } else {
        // Adiciona ao conjunto existente
        add_state(afn->transitions[from][found].to_states, to);
    }
}

/* ============================================================================
   ALGORITMO EPSILON-CLOSURE
   ============================================================================ */

StateSet* epsilon_closure(AFN* afn, StateSet* states) {
    if (!afn || !states) return NULL;
    
    StateSet* closure = copy_state_set(states);
    StateSet* to_process = copy_state_set(states);
    
    while (to_process->size > 0) {
        int current = to_process->states[--to_process->size];
        
        // Procura transições epsilon
        for (int i = 0; i < afn->transition_counts[current]; i++) {
            if (afn->transitions[current][i].input == EPSILON) {
                StateSet* targets = afn->transitions[current][i].to_states;
                
                for (int j = 0; j < targets->size; j++) {
                    int target = targets->states[j];
                    
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

/* ============================================================================
   ALGORITMO MOVE
   ============================================================================ */

StateSet* move(AFN* afn, StateSet* states, int input) {
    if (!afn || !states) return NULL;
    
    StateSet* result = create_state_set();
    if (!result) return NULL;
    
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

/* ============================================================================
   CONVERSÃO DE MÚLTIPLOS AFDs PARA UM ÚNICO AFN
   ============================================================================ */

/*
 * Converte um AFD individual para um AFN equivalente e integra ao AFN unificado.
 * 
 * Parâmetros:
 *   afn_unificado: o AFN sendo construído
 *   afd: o AFD individual a converter
 *   state_offset: offset de estado para este AFD no AFN unificado
 *   token_type: tipo de token que este AFD reconhece
 *   initial_state: estado que será conectado com epsilon-transição do estado 0
 * 
 * Retorno: número de estados usados por este AFD
 */
int integrate_afd_to_afn(
    AFN* afn_unificado,
    int** afd_transitions,
    bool* afd_final_states,
    int afd_num_states,
    int afd_alphabet_size,
    int state_offset,
    TokenType token_type,
    int* final_state_out
) {
    if (!afn_unificado || !afd_transitions || !afd_final_states) return 0;
    
    printf("  Integrando AFD com %d estados (offset: %d)\n", afd_num_states, state_offset);
    
    // Adiciona epsilon-transição do estado inicial (0) para o estado inicial do AFD
    add_afn_transition(afn_unificado, 0, EPSILON, state_offset);
    
    // Converte transições do AFD para o AFN
    for (int from = 0; from < afd_num_states; from++) {
        int afn_from = state_offset + from;
        
        for (int input = 0; input < afd_alphabet_size; input++) {
            int to = afd_transitions[from][input];
            
            if (to >= 0 && to < afd_num_states) {
                int afn_to = state_offset + to;
                add_afn_transition(afn_unificado, afn_from, input, afn_to);
            }
        }
        
        // Marca estados finais
        if (afd_final_states[from]) {
            afn_unificado->final_states[afn_from] = true;
            afn_unificado->token_types[afn_from] = token_type;
            
            if (final_state_out) {
                *final_state_out = afn_from;
            }
        }
    }
    
    return afd_num_states;
}

/* ============================================================================
   CRIAÇÃO DO AFN UNIFICADO PARA DATALANG
   
   Integra todos os AFDs definidos em um único AFN.
   ============================================================================ */

AFN* create_unified_datalang_afn() {
    printf("\n╔════════════════════════════════════════════════════════════╗\n");
    printf("║  CRIANDO AFN UNIFICADO PARA DATALANG                     ║\n");
    printf("╚════════════════════════════════════════════════════════════╝\n");
    
    // Cria AFN com espaço suficiente para todos os AFDs
    // Estado 0: estado inicial unificado
    // Estados 1+: sub-AFNs integrados
    int total_states = 1 + 
                       2 +   // whitespace
                       2 +   // identifier
                       9 +   // number
                       4 +   // string
                       22 +  // operator
                       12 +  // delimiter
                       4 +   // line comment
                       5 +   // block comment
                       50;   // buffer extra
    
    AFN* afn = create_afn(total_states, 256);
    if (!afn) {
        fprintf(stderr, "ERRO: Não foi possível alocar AFN\n");
        return NULL;
    }
    
    afn->start_state = 0;
    int next_state = 1;
    int final_state;
    
    printf("\nIntegrando AFDs individuais:\n");
    printf("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n");
    
    /* ────────────────────────────────────────────────────────────
       WHITESPACE
       ──────────────────────────────────────────────────────────── */
    {
        printf("\n1. WHITESPACE\n");
        
        // Cria AFD de whitespace
        int num_states = 2;
        int** trans = (int**)malloc(num_states * sizeof(int*));
        bool* finals = (bool*)calloc(num_states, sizeof(bool));
        
        for (int i = 0; i < num_states; i++) {
            trans[i] = (int*)malloc(256 * sizeof(int));
            for (int j = 0; j < 256; j++) trans[i][j] = -1;
        }
        
        // Estado 0 -> 1 com whitespace
        trans[0][' '] = trans[0]['\t'] = trans[0]['\n'] = trans[0]['\r'] = 1;
        trans[1][' '] = trans[1]['\t'] = trans[1]['\n'] = trans[1]['\r'] = 1;
        
        finals[1] = true;
        
        int used = integrate_afd_to_afn(afn, trans, finals, num_states, 256, 
                                        next_state, TOKEN_WHITESPACE, &final_state);
        next_state += used;
        
        // Libera
        for (int i = 0; i < num_states; i++) free(trans[i]);
        free(trans);
        free(finals);
    }
    
    /* ────────────────────────────────────────────────────────────
       IDENTIFICADORES E PALAVRAS-CHAVE
       ──────────────────────────────────────────────────────────── */
    {
        printf("\n2. IDENTIFICADORES\n");
        
        int num_states = 2;
        int** trans = (int**)malloc(num_states * sizeof(int*));
        bool* finals = (bool*)calloc(num_states, sizeof(bool));
        
        for (int i = 0; i < num_states; i++) {
            trans[i] = (int*)malloc(256 * sizeof(int));
            for (int j = 0; j < 256; j++) trans[i][j] = -1;
        }
        
        // Inicial: letras e underscore
        for (int c = 'a'; c <= 'z'; c++) trans[0][c] = 1;
        for (int c = 'A'; c <= 'Z'; c++) trans[0][c] = 1;
        trans[0]['_'] = 1;
        
        // Contínuo: letras, dígitos, underscore
        for (int c = 'a'; c <= 'z'; c++) trans[1][c] = 1;
        for (int c = 'A'; c <= 'Z'; c++) trans[1][c] = 1;
        for (int c = '0'; c <= '9'; c++) trans[1][c] = 1;
        trans[1]['_'] = 1;
        
        finals[1] = true;
        
        int used = integrate_afd_to_afn(afn, trans, finals, num_states, 256,
                                        next_state, TOKEN_IDENTIFIER, &final_state);
        next_state += used;
        
        for (int i = 0; i < num_states; i++) free(trans[i]);
        free(trans);
        free(finals);
    }
    
    /* ────────────────────────────────────────────────────────────
       NÚMEROS (inteiros, decimais, científicos)
       ──────────────────────────────────────────────────────────── */
    {
        printf("\n3. NÚMEROS\n");
        
        int num_states = 9;
        int** trans = (int**)malloc(num_states * sizeof(int*));
        bool* finals = (bool*)calloc(num_states, sizeof(bool));
        
        for (int i = 0; i < num_states; i++) {
            trans[i] = (int*)malloc(256 * sizeof(int));
            for (int j = 0; j < 256; j++) trans[i][j] = -1;
        }
        
        // Estado 0: início
        trans[0]['+'] = 1;
        trans[0]['-'] = 1;
        for (int c = '0'; c <= '9'; c++) trans[0][c] = 2;
        trans[0]['.'] = 3;
        
        // Estado 1: após sinal
        for (int c = '0'; c <= '9'; c++) trans[1][c] = 2;
        trans[1]['.'] = 3;
        
        // Estado 2: dígitos inteiros
        for (int c = '0'; c <= '9'; c++) trans[2][c] = 2;
        trans[2]['e'] = trans[2]['E'] = 5;
        finals[2] = true; // Inteiro válido
        trans[2]['.'] = 8; 
        
        // Estado 3: após ponto inicial (ex: .14)
        for (int c = '0'; c <= '9'; c++) trans[3][c] = 4;
        
        // Estado 4: parte decimal (ex: .14, 3.14)
        for (int c = '0'; c <= '9'; c++) trans[4][c] = 4;
        trans[4]['e'] = trans[4]['E'] = 5;
        finals[4] = true; // Decimal válido
        
        // Estado 5: após 'e' ou 'E'
        trans[5]['+'] = trans[5]['-'] = 6;
        for (int c = '0'; c <= '9'; c++) trans[5][c] = 7;
        
        // Estado 6: após sinal do expoente
        for (int c = '0'; c <= '9'; c++) trans[6][c] = 7;
        
        // Estado 7: dígitos do expoente
        for (int c = '0'; c <= '9'; c++) trans[7][c] = 7;
        finals[7] = true; // Científico válido

        // Estado 8: após ponto vindo de inteiro (ex: "3.")
        // Este estado NÃO é final. Só vira float se seguir um dígito.
        for (int c = '0'; c <= '9'; c++) trans[8][c] = 4; // Vai para o estado decimal
        
        int used = integrate_afd_to_afn(afn, trans, finals, num_states, 256,
                                        next_state, TOKEN_INTEGER, &final_state);
        
        // Ajusta tipos de token para números
        for (int i = next_state; i < next_state + num_states; i++) {
            int state_index = i - next_state;
            if (finals[state_index]) {
                if (state_index == 2) {
                    // Estado 2: Inteiro válido
                    afn->token_types[i] = TOKEN_INTEGER;
                } else if (state_index == 4) {
                    // Estado 4: Decimal válido
                    afn->token_types[i] = TOKEN_FLOAT;
                } else if (state_index == 7) {
                    // Estado 7: Científico válido
                    afn->token_types[i] = TOKEN_FLOAT;
                }
            }
        }
        
        next_state += used;
        
        for (int i = 0; i < num_states; i++) free(trans[i]);
        free(trans);
        free(finals);
    }
    
    /* ────────────────────────────────────────────────────────────
       STRINGS
       ──────────────────────────────────────────────────────────── */
    {
        printf("\n4. STRINGS\n");
        
        int num_states = 4;
        int** trans = (int**)malloc(num_states * sizeof(int*));
        bool* finals = (bool*)calloc(num_states, sizeof(bool));
        
        for (int i = 0; i < num_states; i++) {
            trans[i] = (int*)malloc(256 * sizeof(int));
            for (int j = 0; j < 256; j++) trans[i][j] = -1;
        }
        
        // Estado 0 -> 1 com aspa
        trans[0]['"'] = 1;
        
        // Estado 1: dentro da string
        for (int c = 0; c < 256; c++) {
            if (c != '"' && c != '\\' && c != '\n') {
                trans[1][c] = 1;
            }
        }
        trans[1]['\\'] = 2; // escape
        trans[1]['"'] = 3;  // fechamento
        
        // Estado 2: após backslash
        trans[2]['"'] = 1;
        trans[2]['\\'] = 1;
        trans[2]['n'] = 1;
        trans[2]['t'] = 1;
        trans[2]['r'] = 1;
        
        finals[3] = true;
        
        int used = integrate_afd_to_afn(afn, trans, finals, num_states, 256,
                                        next_state, TOKEN_STRING, &final_state);
        next_state += used;
        
        for (int i = 0; i < num_states; i++) free(trans[i]);
        free(trans);
        free(finals);
    }
    
    /* ────────────────────────────────────────────────────────────
       OPERADORES
       ──────────────────────────────────────────────────────────── */
    {
        printf("\n5. OPERADORES\n");
        
        int num_states = 25;
        int** trans = (int**)malloc(num_states * sizeof(int*));
        bool* finals = (bool*)calloc(num_states, sizeof(bool));
        
        for (int i = 0; i < num_states; i++) {
            trans[i] = (int*)malloc(256 * sizeof(int));
            for (int j = 0; j < 256; j++) trans[i][j] = -1;
        }
        
        // Operadores simples
        trans[0]['+'] = 1;  finals[1] = true;
        trans[0]['-'] = 2;  finals[2] = true;
        trans[0]['*'] = 3;  finals[3] = true;
        trans[0]['/'] = 4;  finals[4] = true;
        trans[0]['%'] = 5;  finals[5] = true;
        
        // = e ==
        trans[0]['='] = 6;  finals[6] = true;
        trans[6]['='] = 7;  finals[7] = true;
        
        // ! e !=
        trans[0]['!'] = 8;  finals[8] = true;
        trans[8]['='] = 9;  finals[9] = true;
        
        // < e <=
        trans[0]['<'] = 10; finals[10] = true;
        trans[10]['='] = 11; finals[11] = true;
        
        // > e >=
        trans[0]['>'] = 12; finals[12] = true;
        trans[12]['='] = 13; finals[13] = true;
        
        // & e &&
        trans[0]['&'] = 14; finals[14] = true;
        trans[14]['&'] = 15; finals[15] = true;
        
        // | e || e |>
        trans[0]['|'] = 16; finals[16] = true;
        trans[16]['|'] = 17; finals[17] = true;
        trans[16]['>'] = 18; finals[18] = true;

        trans[0]['.'] = 19; finals[19] = true;  // DOT operator
        trans[0][':'] = 20; finals[20] = true;  // COLON operator
        trans[0][','] = 21; finals[21] = true;  // COMMA operator

        trans[2]['>'] = 22; finals[22] = true;
        
        int used = integrate_afd_to_afn(afn, trans, finals, num_states, 256,
                                        next_state, TOKEN_OPERATOR, &final_state);
        next_state += used;
        
        for (int i = 0; i < num_states; i++) free(trans[i]);
        free(trans);
        free(finals);
    }
    
    /* ────────────────────────────────────────────────────────────
       DELIMITADORES
       ──────────────────────────────────────────────────────────── */
    {
        printf("\n6. DELIMITADORES\n");
        
        int num_states = 13;
        int** trans = (int**)malloc(num_states * sizeof(int*));
        bool* finals = (bool*)calloc(num_states, sizeof(bool));
        
        for (int i = 0; i < num_states; i++) {
            trans[i] = (int*)malloc(256 * sizeof(int));
            for (int j = 0; j < 256; j++) trans[i][j] = -1;
        }
        
        trans[0]['('] = 1;  finals[1] = true;
        trans[0][')'] = 2;  finals[2] = true;
        trans[0]['['] = 3;  finals[3] = true;
        trans[0][']'] = 4;  finals[4] = true;
        trans[0]['{'] = 5;  finals[5] = true;
        trans[0]['}'] = 6;  finals[6] = true;
        trans[0][';'] = 7;  finals[7] = true;
        trans[0][','] = 8;  finals[8] = true;
        trans[0][':'] = 9;  finals[9] = true;
        trans[0]['.'] = 10; finals[10] = true;
        trans[10]['.'] = 11; finals[11] = true;
        trans[0]['='] = 12; finals[12] = true;
        
        int used = integrate_afd_to_afn(afn, trans, finals, num_states, 256,
                                        next_state, TOKEN_DELIMITER, &final_state);
        next_state += used;
        
        for (int i = 0; i < num_states; i++) free(trans[i]);
        free(trans);
        free(finals);
    }
    
    /* ────────────────────────────────────────────────────────────
       COMENTÁRIOS DE LINHA
       ──────────────────────────────────────────────────────────── */
    {
        printf("\n7. COMENTÁRIOS DE LINHA\n");
        
        int num_states = 4;
        int** trans = (int**)malloc(num_states * sizeof(int*));
        bool* finals = (bool*)calloc(num_states, sizeof(bool));
        
        for (int i = 0; i < num_states; i++) {
            trans[i] = (int*)malloc(256 * sizeof(int));
            for (int j = 0; j < 256; j++) trans[i][j] = -1;
        }
        
        trans[0]['/'] = 1;
        trans[1]['/'] = 2;
        for (int c = 0; c < 256; c++) {
            if (c != '\n' && c != '\r') trans[2][c] = 2;
        }
        trans[2]['\n'] = 3; finals[3] = true;
        trans[2]['\r'] = 3; finals[3] = true;
        finals[2] = true; // Aceita mesmo sem newline (EOF)
        
        int used = integrate_afd_to_afn(afn, trans, finals, num_states, 256,
                                        next_state, TOKEN_COMMENT, &final_state);
        next_state += used;
        
        for (int i = 0; i < num_states; i++) free(trans[i]);
        free(trans);
        free(finals);
    }
    
    /* ────────────────────────────────────────────────────────────
       COMENTÁRIOS DE BLOCO
       ──────────────────────────────────────────────────────────── */
    {
        printf("\n8. COMENTÁRIOS DE BLOCO\n");
        
        int num_states = 5;
        int** trans = (int**)malloc(num_states * sizeof(int*));
        bool* finals = (bool*)calloc(num_states, sizeof(bool));
        
        for (int i = 0; i < num_states; i++) {
            trans[i] = (int*)malloc(256 * sizeof(int));
            for (int j = 0; j < 256; j++) trans[i][j] = -1;
        }
        
        trans[0]['/'] = 1;
        trans[1]['*'] = 2;
        for (int c = 0; c < 256; c++) {
            if (c != '*') trans[2][c] = 2;
        }
        trans[2]['*'] = 3;
        for (int c = 0; c < 256; c++) {
            if (c != '*' && c != '/') trans[3][c] = 2;
        }
        trans[3]['*'] = 3;
        trans[3]['/'] = 4; finals[4] = true;
        
        int used = integrate_afd_to_afn(afn, trans, finals, num_states, 256,
                                        next_state, TOKEN_COMMENT, &final_state);
        next_state += used;
        
        for (int i = 0; i < num_states; i++) free(trans[i]);
        free(trans);
        free(finals);
    }
    
    printf("\n━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n");
    printf("\nAFN unificado criado com %d estados\n", next_state);
    printf("Total de transições epsilon: %d\n", afn->transition_counts[0]);
    
    afn->num_states = next_state;
    
    printf("\n╚════════════════════════════════════════════════════════════╝\n\n");
    
    return afn;
}

/* ============================================================================
   FUNÇÕES DO AFD
   ============================================================================ */

void free_afd(AFD* afd) {
    if (afd) {
        if (afd->transition_table) {
            for (int i = 0; i < afd->num_states; i++) {
                free(afd->transition_table[i]);
            }
            free(afd->transition_table);
        }
        free(afd->final_states);
        free(afd->token_types);
        free(afd);
    }
}
