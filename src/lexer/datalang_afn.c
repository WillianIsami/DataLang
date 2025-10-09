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
    
    AFN* afn = create_afn(100, 256);
    afn->start_state = 0;
    
    int next_state = 1;

    // === WHITESPACE ===
    int ws_start = next_state++;
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

    // === IDENTIFICADORES ===
    int id_start = next_state++;
    for (int c = 'a'; c <= 'z'; c++) add_afn_transition(afn, 0, c, id_start);
    for (int c = 'A'; c <= 'Z'; c++) add_afn_transition(afn, 0, c, id_start);
    add_afn_transition(afn, 0, '_', id_start);
    
    for (int c = 'a'; c <= 'z'; c++) add_afn_transition(afn, id_start, c, id_start);
    for (int c = 'A'; c <= 'Z'; c++) add_afn_transition(afn, id_start, c, id_start);
    for (int c = '0'; c <= '9'; c++) add_afn_transition(afn, id_start, c, id_start);
    add_afn_transition(afn, id_start, '_', id_start);
    
    afn->final_states[id_start] = true;
    afn->token_types[id_start] = TOKEN_IDENTIFIER;

    // === NÚMEROS INTEIROS ===
    int int_start = next_state++;
    for (int c = '0'; c <= '9'; c++) add_afn_transition(afn, 0, c, int_start);
    for (int c = '0'; c <= '9'; c++) add_afn_transition(afn, int_start, c, int_start);
    
    afn->final_states[int_start] = true;
    afn->token_types[int_start] = TOKEN_INTEGER;

    // === NÚMEROS DECIMAIS ===
    int decimal_start = next_state++;
    add_afn_transition(afn, int_start, '.', decimal_start);
    for (int c = '0'; c <= '9'; c++) add_afn_transition(afn, decimal_start, c, decimal_start);
    
    afn->final_states[decimal_start] = true;
    afn->token_types[decimal_start] = TOKEN_FLOAT;

    // === STRINGS ===
    int str_start = next_state++;
    add_afn_transition(afn, 0, '"', str_start);
    
    // Caracteres normais dentro da string
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

    // === OPERADORES RELACIONAIS COMPOSTOS ===
    
    // = e ==
    int assign_start = next_state++;
    add_afn_transition(afn, 0, '=', assign_start);
    afn->final_states[assign_start] = true;
    afn->token_types[assign_start] = TOKEN_ASSIGN;
    
    int equal_op = next_state++;
    add_afn_transition(afn, assign_start, '=', equal_op);
    afn->final_states[equal_op] = true;
    afn->token_types[equal_op] = TOKEN_EQUAL;
    
    // ! e !=
    int not_op = next_state++;
    add_afn_transition(afn, 0, '!', not_op);
    afn->final_states[not_op] = true;
    afn->token_types[not_op] = TOKEN_NOT;
    
    int not_equal = next_state++;
    add_afn_transition(afn, not_op, '=', not_equal);
    afn->final_states[not_equal] = true;
    afn->token_types[not_equal] = TOKEN_NOT_EQUAL;
    
    // < e <=
    int less_op = next_state++;
    add_afn_transition(afn, 0, '<', less_op);
    afn->final_states[less_op] = true;
    afn->token_types[less_op] = TOKEN_LESS;
    
    int less_equal = next_state++;
    add_afn_transition(afn, less_op, '=', less_equal);
    afn->final_states[less_equal] = true;
    afn->token_types[less_equal] = TOKEN_LESS_EQUAL;
    
    // > e >=  
    int greater_op = next_state++;
    add_afn_transition(afn, 0, '>', greater_op);
    afn->final_states[greater_op] = true;
    afn->token_types[greater_op] = TOKEN_GREATER;
    
    int greater_equal = next_state++;
    add_afn_transition(afn, greater_op, '=', greater_equal);
    afn->final_states[greater_equal] = true;
    afn->token_types[greater_equal] = TOKEN_GREATER_EQUAL;

    // === OPERADORES ARITMÉTICOS SIMPLES ===
    int plus_op = next_state++;
    add_afn_transition(afn, 0, '+', plus_op);
    afn->final_states[plus_op] = true;
    afn->token_types[plus_op] = TOKEN_PLUS;

    int minus_op = next_state++;
    add_afn_transition(afn, 0, '-', minus_op);
    afn->final_states[minus_op] = true;
    afn->token_types[minus_op] = TOKEN_MINUS;

    int mult_op = next_state++;
    add_afn_transition(afn, 0, '*', mult_op);
    afn->final_states[mult_op] = true;
    afn->token_types[mult_op] = TOKEN_MULT;

    int div_op = next_state++;
    add_afn_transition(afn, 0, '/', div_op);
    afn->final_states[div_op] = true;
    afn->token_types[div_op] = TOKEN_DIV;

    int mod_op = next_state++;
    add_afn_transition(afn, 0, '%', mod_op);
    afn->final_states[mod_op] = true;
    afn->token_types[mod_op] = TOKEN_MOD;

    // === DELIMITADORES ===
    int lparen = next_state++;
    add_afn_transition(afn, 0, '(', lparen);
    afn->final_states[lparen] = true;
    afn->token_types[lparen] = TOKEN_LPAREN;

    int rparen = next_state++;
    add_afn_transition(afn, 0, ')', rparen);
    afn->final_states[rparen] = true;
    afn->token_types[rparen] = TOKEN_RPAREN;

    int lbrace = next_state++;
    add_afn_transition(afn, 0, '{', lbrace);
    afn->final_states[lbrace] = true;
    afn->token_types[lbrace] = TOKEN_LBRACE;

    int rbrace = next_state++;
    add_afn_transition(afn, 0, '}', rbrace);
    afn->final_states[rbrace] = true;
    afn->token_types[rbrace] = TOKEN_RBRACE;

    int comma = next_state++;
    add_afn_transition(afn, 0, ',', comma);
    afn->final_states[comma] = true;
    afn->token_types[comma] = TOKEN_COMMA;

    int semicolon = next_state++;
    add_afn_transition(afn, 0, ';', semicolon);
    afn->final_states[semicolon] = true;
    afn->token_types[semicolon] = TOKEN_SEMICOLON;

    int colon = next_state++;
    add_afn_transition(afn, 0, ':', colon);
    afn->final_states[colon] = true;
    afn->token_types[colon] = TOKEN_COLON;

    printf("AFN unificado criado com %d estados\n", next_state);
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