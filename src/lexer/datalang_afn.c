#include "datalang_afn.h"

// -=-=-=-FUNÇÕES DE GERENCIAMENTO DE CONJUNTOS-=-=-=-

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

// -=-=-=-FUNÇÕES DO AFN-=-=-=-

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
    
    // Aloca tipos de token
    afn->token_types = (TokenType *)calloc(num_states, sizeof(TokenType));
    
    // Inicializa todos os tipos de token como TOKEN_ERROR
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
        afn->transitions[from][idx].input = input;
        afn->transitions[from][idx].to_states = create_state_set();
        add_state(afn->transitions[from][idx].to_states, to);
    } else {
        // Adiciona estado ao conjunto existente
        add_state(afn->transitions[from][found].to_states, to);
    }
}

// -=-=-=-FECHO EPSILON-=-=-=-

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

// -=-=-=-FUNÇÃO MOVE-=-=-=-

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

// -=-=-=-AFN UNIFICADO COMPLETO PARA DATALANG-=-=-=-

AFN* create_unified_datalang_afn() {
    printf("Criando AFN unificado para DataLang...\n");
    
    // Cria um AFN com estados suficientes para todos os padrões
    AFN* afn = create_afn(200, 256);
    
    // Configura estado inicial
    afn->start_state = 0;
    
    int next_state = 1; // Próximo estado livre
    
    // === WHITESPACE (simples) ===
    int ws_start = next_state;
    add_afn_transition(afn, 0, ' ', ws_start);
    add_afn_transition(afn, 0, '\t', ws_start);
    add_afn_transition(afn, 0, '\n', ws_start);
    add_afn_transition(afn, 0, '\r', ws_start);
    
    add_afn_transition(afn, ws_start, ' ', ws_start);
    add_afn_transition(afn, ws_start, '\t', ws_start);
    add_afn_transition(afn, ws_start, '\n', ws_start);
    add_afn_transition(afn, ws_start, '\r', ws_start);
    
    afn->final_states[ws_start] = true;
    afn->token_types[ws_start] = TOKEN_WHITESPACE;
    next_state++;

    // === IDENTIFICADORES E PALAVRAS-CHAVE ===
    int id_start = next_state;
    for (int c = 'a'; c <= 'z'; c++) add_afn_transition(afn, 0, c, id_start);
    for (int c = 'A'; c <= 'Z'; c++) add_afn_transition(afn, 0, c, id_start);
    add_afn_transition(afn, 0, '_', id_start);
    
    for (int c = 'a'; c <= 'z'; c++) add_afn_transition(afn, id_start, c, id_start);
    for (int c = 'A'; c <= 'Z'; c++) add_afn_transition(afn, id_start, c, id_start);
    for (int c = '0'; c <= '9'; c++) add_afn_transition(afn, id_start, c, id_start);
    add_afn_transition(afn, id_start, '_', id_start);
    
    afn->final_states[id_start] = true;
    afn->token_types[id_start] = TOKEN_IDENTIFIER;
    next_state++;

    // === NÚMEROS INTEIROS ===
    int int_start = next_state;
    for (int c = '0'; c <= '9'; c++) add_afn_transition(afn, 0, c, int_start);
    
    for (int c = '0'; c <= '9'; c++) add_afn_transition(afn, int_start, c, int_start);
    
    afn->final_states[int_start] = true;
    afn->token_types[int_start] = TOKEN_INTEGER;
    next_state++;

    // === NÚMEROS DECIMAIS ===
    int float_start = next_state;
    add_afn_transition(afn, int_start, '.', float_start);
    
    for (int c = '0'; c <= '9'; c++) add_afn_transition(afn, float_start, c, float_start);
    
    afn->final_states[float_start] = true;
    afn->token_types[float_start] = TOKEN_FLOAT;
    next_state++;

    // === STRINGS ===
    int str_start = next_state;
    add_afn_transition(afn, 0, '"', str_start);
    
    // Dentro da string
    for (int c = 0; c < 256; c++) {
        if (c != '"' && c != '\\' && c != '\n') {
            add_afn_transition(afn, str_start, c, str_start);
        }
    }
    
    // Escape
    int str_escape = next_state++;
    add_afn_transition(afn, str_start, '\\', str_escape);
    add_afn_transition(afn, str_escape, '"', str_start);
    add_afn_transition(afn, str_escape, '\\', str_start);
    add_afn_transition(afn, str_escape, 'n', str_start);
    add_afn_transition(afn, str_escape, 't', str_start);
    add_afn_transition(afn, str_escape, 'r', str_start);
    
    // Fim da string
    int str_end = next_state++;
    add_afn_transition(afn, str_start, '"', str_end);
    
    afn->final_states[str_end] = true;
    afn->token_types[str_end] = TOKEN_STRING;
    next_state++;

    // === OPERADORES SIMPLES ===
    // +
    int op_plus = next_state++;
    add_afn_transition(afn, 0, '+', op_plus);
    afn->final_states[op_plus] = true;
    afn->token_types[op_plus] = TOKEN_OPERATOR;

    // -
    int op_minus = next_state++;
    add_afn_transition(afn, 0, '-', op_minus);
    afn->final_states[op_minus] = true;
    afn->token_types[op_minus] = TOKEN_OPERATOR;

    // *
    int op_mult = next_state++;
    add_afn_transition(afn, 0, '*', op_mult);
    afn->final_states[op_mult] = true;
    afn->token_types[op_mult] = TOKEN_OPERATOR;

    // /
    int op_div = next_state++;
    add_afn_transition(afn, 0, '/', op_div);
    afn->final_states[op_div] = true;
    afn->token_types[op_div] = TOKEN_OPERATOR;

    // =
    int op_assign = next_state++;
    add_afn_transition(afn, 0, '=', op_assign);
    afn->final_states[op_assign] = true;
    afn->token_types[op_assign] = TOKEN_OPERATOR;

    // === DELIMITADORES ===
    // (
    int delim_paren_open = next_state++;
    add_afn_transition(afn, 0, '(', delim_paren_open);
    afn->final_states[delim_paren_open] = true;
    afn->token_types[delim_paren_open] = TOKEN_DELIMITER;

    // )
    int delim_paren_close = next_state++;
    add_afn_transition(afn, 0, ')', delim_paren_close);
    afn->final_states[delim_paren_close] = true;
    afn->token_types[delim_paren_close] = TOKEN_DELIMITER;

    // {
    int delim_brace_open = next_state++;
    add_afn_transition(afn, 0, '{', delim_brace_open);
    afn->final_states[delim_brace_open] = true;
    afn->token_types[delim_brace_open] = TOKEN_DELIMITER;

    // }
    int delim_brace_close = next_state++;
    add_afn_transition(afn, 0, '}', delim_brace_close);
    afn->final_states[delim_brace_close] = true;
    afn->token_types[delim_brace_close] = TOKEN_DELIMITER;

    // [
    int delim_bracket_open = next_state++;
    add_afn_transition(afn, 0, '[', delim_bracket_open);
    afn->final_states[delim_bracket_open] = true;
    afn->token_types[delim_bracket_open] = TOKEN_DELIMITER;

    // ]
    int delim_bracket_close = next_state++;
    add_afn_transition(afn, 0, ']', delim_bracket_close);
    afn->final_states[delim_bracket_close] = true;
    afn->token_types[delim_bracket_close] = TOKEN_DELIMITER;

    // ,
    int delim_comma = next_state++;
    add_afn_transition(afn, 0, ',', delim_comma);
    afn->final_states[delim_comma] = true;
    afn->token_types[delim_comma] = TOKEN_DELIMITER;

    // ;
    int delim_semicolon = next_state++;
    add_afn_transition(afn, 0, ';', delim_semicolon);
    afn->final_states[delim_semicolon] = true;
    afn->token_types[delim_semicolon] = TOKEN_DELIMITER;

    // :
    int delim_colon = next_state++;
    add_afn_transition(afn, 0, ':', delim_colon);
    afn->final_states[delim_colon] = true;
    afn->token_types[delim_colon] = TOKEN_DELIMITER;

    // === COMENTÁRIOS DE LINHA ===
    int comment_line_start = next_state++;
    add_afn_transition(afn, op_div, '/', comment_line_start);
    
    for (int c = 0; c < 256; c++) {
        if (c != '\n' && c != '\r') {
            add_afn_transition(afn, comment_line_start, c, comment_line_start);
        }
    }
    
    int comment_line_end = next_state++;
    add_afn_transition(afn, comment_line_start, '\n', comment_line_end);
    add_afn_transition(afn, comment_line_start, '\r', comment_line_end);
    
    afn->final_states[comment_line_end] = true;
    afn->token_types[comment_line_end] = TOKEN_COMMENT_LINE;
    next_state++;

    printf("AFN unificado criado com %d estados\n", next_state);
    printf("Tokens suportados: whitespace, identificadores, números, strings, operadores, delimitadores, comentários\n");
    
    return afn;
}

// -=-=-=-FUNÇÕES DO AFD-=-=-=-

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