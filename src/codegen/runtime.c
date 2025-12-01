/*
 * DataLang Runtime - Versão Completa
 * Suporte para concatenação de strings, DataFrames e I/O com CSV real
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <stdarg.h>

// ==================== STRING CONCATENATION ====================

char* __str_concat(char* s1, char* s2) {
    if (!s1) s1 = "";
    if (!s2) s2 = "";
    
    size_t len1 = strlen(s1);
    size_t len2 = strlen(s2);
    
    char* result = (char*)malloc(len1 + len2 + 1);
    if (!result) {
        fprintf(stderr, "Erro: Falha ao alocar memória para concatenação\n");
        exit(1);
    }
    
    strcpy(result, s1);
    strcat(result, s2);
    
    return result;
}

// ==================== DATAFRAME ESTRUTURA ====================

typedef struct {
    int64_t id;
    char* source_file;
    int64_t row_count;
    int64_t col_count;
    char** column_names;
    char*** data;  // [row][col] = string value
} DataFrame;
static int64_t df_counter = 0;

typedef struct {
    int64_t size;
    double* data;
} DFArrayDouble;

// ==================== FUNÇÕES AUXILIARES ====================

bool file_exists(const char* path) {
    struct stat buffer;
    return (stat(path, &buffer) == 0);
}

static int find_column_index(DataFrame* df, const char* name) {
    if (!df || !name) return -1;
    for (int64_t i = 0; i < df->col_count; i++) {
        if (strcmp(df->column_names[i], name) == 0) return (int)i;
    }
    return -1;
}

static char* strdup_or_null(const char* s) {
    return s ? strdup(s) : NULL;
}

char* trim_whitespace(char* str) {
    char* end;
    
    // Trim leading space
    while (*str == ' ' || *str == '\t') str++;
    
    if (*str == 0) return str;
    
    // Trim trailing space
    end = str + strlen(str) - 1;
    while (end > str && (*end == ' ' || *end == '\t' || *end == '\n' || *end == '\r')) {
        end--;
    }
    
    *(end + 1) = '\0';
    return str;
}

char** parse_csv_line(char* line, int* count) {
    int capacity = 10;
    char** fields = malloc(capacity * sizeof(char*));
    *count = 0;
    
    char* start = line;
    char* current = line;
    bool in_quotes = false;
    
    while (*current) {
        if (*current == '"') {
            in_quotes = !in_quotes;
        } else if (*current == ',' && !in_quotes) {
            // End of field
            *current = '\0';
            
            char* field = trim_whitespace(start);
            
            // Remove quotes if present
            if (*field == '"' && field[strlen(field)-1] == '"') {
                field++;
                field[strlen(field)-1] = '\0';
            }
            
            if (*count >= capacity) {
                capacity *= 2;
                fields = realloc(fields, capacity * sizeof(char*));
            }
            
            fields[(*count)++] = strdup(field);
            start = current + 1;
        }
        current++;
    }
    
    // Last field
    char* field = trim_whitespace(start);
    if (*field == '"' && field[strlen(field)-1] == '"') {
        field++;
        field[strlen(field)-1] = '\0';
    }
    
    if (*count >= capacity) {
        capacity *= 2;
        fields = realloc(fields, capacity * sizeof(char*));
    }
    fields[(*count)++] = strdup(field);
    
    return fields;
}

// ==================== LOAD CSV ====================

void* datalang_load(char* path) {
    printf("[Runtime] Carregando DataFrame de: %s\n", path);
    
    // Verifica se arquivo existe
    if (!file_exists(path)) {
        fprintf(stderr, "Erro: Arquivo '%s' não encontrado\n", path);
        exit(1);
    }
    
    FILE* file = fopen(path, "r");
    if (!file) {
        fprintf(stderr, "Erro: Não foi possível abrir o arquivo '%s'\n", path);
        exit(1);
    }
    
    DataFrame* df = (DataFrame*)calloc(1, sizeof(DataFrame));
    df->id = 1;
    df->source_file = strdup(path);
    
    char line[4096];
    int line_num = 0;
    int row_capacity = 100;
    
    // Lê primeira linha (header)
    if (fgets(line, sizeof(line), file)) {
        int col_count;
        df->column_names = parse_csv_line(line, &col_count);
        df->col_count = col_count;
        printf("[Runtime] Colunas detectadas: %ld\n", df->col_count);
        for (int i = 0; i < df->col_count; i++) {
            printf("  - %s\n", df->column_names[i]);
        }
        line_num++;
    } else {
        fprintf(stderr, "Erro: Arquivo CSV vazio\n");
        fclose(file);
        free(df);
        return NULL;
    }
    
    // Aloca array de dados
    df->data = malloc(row_capacity * sizeof(char**));
    df->row_count = 0;
    
    // Lê linhas de dados
    while (fgets(line, sizeof(line), file)) {
        if (strlen(trim_whitespace(line)) == 0) continue; // Pula linhas vazias
        
        int field_count;
        char** fields = parse_csv_line(line, &field_count);
        
        if (field_count != df->col_count) {
            fprintf(stderr, "Aviso: Linha %d tem %d campos, esperado %ld\n", 
                    line_num, field_count, df->col_count);
        }
        
        if (df->row_count >= row_capacity) {
            row_capacity *= 2;
            df->data = realloc(df->data, row_capacity * sizeof(char**));
        }
        
        df->data[df->row_count++] = fields;
        line_num++;
    }
    
    fclose(file);
    
    printf("[Runtime] DataFrame carregado: %ld linhas x %ld colunas\n", 
           df->row_count, df->col_count);
    
    return (void*)df;
}

int64_t datalang_df_count(void* df_ptr) {
    if (!df_ptr) return 0;
    DataFrame* df = (DataFrame*)df_ptr;
    return df->row_count;
}

// ==================== SAVE CSV ====================

void datalang_save(void* df_ptr, char* path) {
    if (!df_ptr) {
        // Sem DataFrame real (ex.: select/groupby em arrays) – operação vira no-op
        return;
    }
    
    DataFrame* df = (DataFrame*)df_ptr;
    printf("[Runtime] Salvando DataFrame (id=%ld) em: %s\n", df->id, path);
    
    FILE* file = fopen(path, "w");
    if (!file) {
        fprintf(stderr, "Erro: Não foi possível criar o arquivo '%s'\n", path);
        return;
    }
    
    // Escreve header
    for (int64_t i = 0; i < df->col_count; i++) {
        fprintf(file, "%s", df->column_names[i]);
        if (i < df->col_count - 1) fprintf(file, ",");
    }
    fprintf(file, "\n");
    
    // Escreve dados
    for (int64_t row = 0; row < df->row_count; row++) {
        for (int64_t col = 0; col < df->col_count; col++) {
            // Escapa campos com vírgulas ou aspas
            char* value = df->data[row][col];
            bool needs_quotes = (strchr(value, ',') != NULL || strchr(value, '"') != NULL);
            
            if (needs_quotes) {
                fprintf(file, "\"");
                // Escapa aspas internas
                for (char* p = value; *p; p++) {
                    if (*p == '"') fprintf(file, "\"\"");
                    else fputc(*p, file);
                }
                fprintf(file, "\"");
            } else {
                fprintf(file, "%s", value);
            }
            
            if (col < df->col_count - 1) fprintf(file, ",");
        }
        fprintf(file, "\n");
    }
    
    fclose(file);
    printf("[Runtime] DataFrame salvo com sucesso: %ld linhas\n", df->row_count);
}

// ==================== SELECT (PROJECT) ====================

void* datalang_select(void* df_ptr, int32_t column_count, ...) {
    if (!df_ptr) return NULL;
    DataFrame* src = (DataFrame*)df_ptr;

    int* col_indices = (int*)calloc(column_count, sizeof(int));
    char** col_names = (char**)calloc(column_count, sizeof(char*));

    va_list args;
    va_start(args, column_count);
    for (int32_t i = 0; i < column_count; i++) {
        char* col = va_arg(args, char*);
        col_names[i] = strdup_or_null(col);
        col_indices[i] = find_column_index(src, col);
    }
    va_end(args);

    DataFrame* df = (DataFrame*)calloc(1, sizeof(DataFrame));
    df->id = ++df_counter;
    df->row_count = src->row_count;
    df->col_count = column_count;
    df->column_names = (char**)calloc(column_count, sizeof(char*));
    df->data = (char***)calloc(df->row_count, sizeof(char**));

    for (int32_t i = 0; i < column_count; i++) {
        df->column_names[i] = strdup_or_null(col_names[i]);
    }

    for (int64_t r = 0; r < df->row_count; r++) {
        df->data[r] = (char**)calloc(column_count, sizeof(char*));
        for (int32_t c = 0; c < column_count; c++) {
            int idx = col_indices[c];
            if (idx >= 0 && idx < src->col_count) {
                df->data[r][c] = strdup_or_null(src->data[r][idx]);
            } else {
                df->data[r][c] = strdup("null");
            }
        }
    }

    for (int i = 0; i < column_count; i++) free(col_names[i]);
    free(col_names);
    free(col_indices);

    df->source_file = strdup("select(runtime)");
    return (void*)df;
}

// ==================== GROUPBY (DISTINCT) ====================

void* datalang_groupby(void* df_ptr, int32_t group_count, ...) {
    if (!df_ptr) return NULL;
    DataFrame* src = (DataFrame*)df_ptr;

    int* group_idx = (int*)calloc(group_count, sizeof(int));
    char** group_cols = (char**)calloc(group_count, sizeof(char*));

    va_list args;
    va_start(args, group_count);
    for (int32_t i = 0; i < group_count; i++) {
        char* col = va_arg(args, char*);
        group_cols[i] = strdup_or_null(col);
        group_idx[i] = find_column_index(src, col);
    }
    va_end(args);

    DataFrame* df = (DataFrame*)calloc(1, sizeof(DataFrame));
    df->id = ++df_counter;
    df->col_count = group_count;
    df->column_names = (char**)calloc(group_count, sizeof(char*));
    for (int32_t i = 0; i < group_count; i++) {
        df->column_names[i] = strdup_or_null(group_cols[i]);
    }

    df->data = NULL;
    df->row_count = 0;

    // Naive distinct
    for (int64_t r = 0; r < src->row_count; r++) {
        bool exists = false;
        for (int64_t dr = 0; dr < df->row_count; dr++) {
            bool all_eq = true;
            for (int32_t g = 0; g < group_count; g++) {
                int idx = group_idx[g];
                const char* v_src = (idx >= 0) ? src->data[r][idx] : "null";
                const char* v_dst = df->data[dr][g];
                if (strcmp(v_src ? v_src : "null", v_dst ? v_dst : "null") != 0) {
                    all_eq = false;
                    break;
                }
            }
            if (all_eq) { exists = true; break; }
        }
        if (exists) continue;

        df->data = (char***)realloc(df->data, (df->row_count + 1) * sizeof(char**));
        df->data[df->row_count] = (char**)calloc(group_count, sizeof(char*));
        for (int32_t g = 0; g < group_count; g++) {
            int idx = group_idx[g];
            const char* v = (idx >= 0) ? src->data[r][idx] : "null";
            df->data[df->row_count][g] = strdup_or_null(v);
        }
        df->row_count++;
    }

    for (int i = 0; i < group_count; i++) free(group_cols[i]);
    free(group_cols);
    free(group_idx);
    df->source_file = strdup("groupby(runtime)");
    return (void*)df;
}

// ==================== FILTER (numeric) ====================

// op: 0 ==, 1 !=, 2 >, 3 >=, 4 <, 5 <=
void* datalang_df_filter_numeric(void* df_ptr, char* column, int32_t op, double threshold) {
    if (!df_ptr) return NULL;
    DataFrame* src = (DataFrame*)df_ptr;
    int idx = find_column_index(src, column);
    if (idx < 0) return df_ptr;

    DataFrame* df = (DataFrame*)calloc(1, sizeof(DataFrame));
    df->id = ++df_counter;
    df->col_count = src->col_count;
    df->column_names = (char**)calloc(df->col_count, sizeof(char*));
    for (int64_t i = 0; i < df->col_count; i++) df->column_names[i] = strdup_or_null(src->column_names[i]);

    df->data = NULL;
    df->row_count = 0;

    for (int64_t r = 0; r < src->row_count; r++) {
        const char* val = src->data[r][idx];
        double num = val ? strtod(val, NULL) : 0.0;
        bool keep = false;
        switch (op) {
            case 0: keep = num == threshold; break;
            case 1: keep = num != threshold; break;
            case 2: keep = num >  threshold; break;
            case 3: keep = num >= threshold; break;
            case 4: keep = num <  threshold; break;
            case 5: keep = num <= threshold; break;
            default: keep = false; break;
        }
        if (!keep) continue;

        df->data = (char***)realloc(df->data, (df->row_count + 1) * sizeof(char**));
        df->data[df->row_count] = (char**)calloc(df->col_count, sizeof(char*));
        for (int64_t c = 0; c < df->col_count; c++) {
            df->data[df->row_count][c] = strdup_or_null(src->data[r][c]);
        }
        df->row_count++;
    }

    df->source_file = strdup("filter(runtime)");
    return (void*)df;
}

// ==================== FILTER (string igualdade) ====================
// op: 0 ==, 1 !=
void* datalang_df_filter_string(void* df_ptr, char* column, char* literal, int32_t op) {
    if (!df_ptr || !literal) return df_ptr;
    DataFrame* src = (DataFrame*)df_ptr;
    int idx = find_column_index(src, column);
    if (idx < 0) return df_ptr;

    DataFrame* df = (DataFrame*)calloc(1, sizeof(DataFrame));
    df->id = ++df_counter;
    df->col_count = src->col_count;
    df->column_names = (char**)calloc(df->col_count, sizeof(char*));
    for (int64_t i = 0; i < df->col_count; i++) df->column_names[i] = strdup_or_null(src->column_names[i]);

    df->data = NULL;
    df->row_count = 0;

    for (int64_t r = 0; r < src->row_count; r++) {
        const char* val = src->data[r][idx];
        bool keep = false;
        int cmp = strcmp(val ? val : "null", literal);
        if (op == 0) keep = (cmp == 0);
        else if (op == 1) keep = (cmp != 0);
        if (!keep) continue;

        df->data = (char***)realloc(df->data, (df->row_count + 1) * sizeof(char**));
        df->data[df->row_count] = (char**)calloc(df->col_count, sizeof(char*));
        for (int64_t c = 0; c < df->col_count; c++) {
            df->data[df->row_count][c] = strdup_or_null(src->data[r][c]);
        }
        df->row_count++;
    }

    df->source_file = strdup("filter(runtime)");
    return (void*)df;
}

// ==================== EXTRACT COLUMN AS DOUBLE ARRAY ====================

DFArrayDouble datalang_df_column_double(void* df_ptr, char* column, double scale, double add) {
    DFArrayDouble arr = {0, NULL};
    if (!df_ptr) return arr;
    DataFrame* df = (DataFrame*)df_ptr;
    int idx = find_column_index(df, column);
    if (idx < 0) return arr;

    arr.size = df->row_count;
    arr.data = (double*)calloc(arr.size, sizeof(double));
    for (int64_t r = 0; r < arr.size; r++) {
        double v = strtod(df->data[r][idx], NULL);
        arr.data[r] = v * scale + add;
    }
    return arr;
}
// ==================== PRINT DATAFRAME ====================

void datalang_print_dataframe(void* df_ptr) {
    if (!df_ptr) {
        printf("<DataFrame vazio>\n");
        return;
    }

    DataFrame* df = (DataFrame*)df_ptr;

    // Cabeçalho
    printf("[");
    for (int64_t c = 0; c < df->col_count; c++) {
        printf("%s", df->column_names[c] ? df->column_names[c] : "col");
        if (c < df->col_count - 1) printf(", ");
    }
    printf("]\n");

    // Linhas
    for (int64_t r = 0; r < df->row_count; r++) {
        printf("[");
        for (int64_t c = 0; c < df->col_count; c++) {
            char* val = df->data ? df->data[r][c] : NULL;
            printf("%s", val ? val : "null");
            if (c < df->col_count - 1) printf(", ");
        }
        printf("]\n");
    }
}

// ==================== DATAFRAME BUILD HELPERS ====================

void* datalang_df_create(int32_t col_count, ...) {
    DataFrame* df = (DataFrame*)calloc(1, sizeof(DataFrame));
    df->id = ++df_counter;
    df->col_count = col_count;
    df->row_count = 0;
    df->column_names = (char**)calloc(col_count, sizeof(char*));
    df->data = NULL;
    df->source_file = strdup("df_from_array");
    
    va_list args;
    va_start(args, col_count);
    for (int32_t i = 0; i < col_count; i++) {
        char* name = va_arg(args, char*);
        df->column_names[i] = strdup(name ? name : "col");
    }
    va_end(args);
    
    return (void*)df;
}

void datalang_df_add_row(void* df_ptr, int32_t col_count, ...) {
    if (!df_ptr) return;
    DataFrame* df = (DataFrame*)df_ptr;
    
    df->data = (char***)realloc(df->data, (df->row_count + 1) * sizeof(char**));
    df->data[df->row_count] = (char**)calloc(col_count, sizeof(char*));
    
    va_list args;
    va_start(args, col_count);
    for (int32_t i = 0; i < col_count; i++) {
        char* val = va_arg(args, char*);
        df->data[df->row_count][i] = strdup(val ? val : "null");
    }
    va_end(args);
    
    df->row_count++;
}

char* datalang_format_int(int64_t v) {
    char buf[32];
    snprintf(buf, sizeof(buf), "%lld", (long long)v);
    return strdup(buf);
}

char* datalang_format_float(double v) {
    char buf[64];
    snprintf(buf, sizeof(buf), "%0.6f", v);
    return strdup(buf);
}

char* datalang_format_bool(bool v) {
    return strdup(v ? "true" : "false");
}

// ==================== FREE DATAFRAME ====================

void datalang_free_dataframe(void* df_ptr) {
    if (!df_ptr) return;
    
    DataFrame* df = (DataFrame*)df_ptr;
    
    // Libera column names
    if (df->column_names) {
        for (int64_t i = 0; i < df->col_count; i++) {
            free(df->column_names[i]);
        }
        free(df->column_names);
    }
    
    // Libera data
    if (df->data) {
        for (int64_t row = 0; row < df->row_count; row++) {
            for (int64_t col = 0; col < df->col_count; col++) {
                free(df->data[row][col]);
            }
            free(df->data[row]);
        }
        free(df->data);
    }
    
    free(df->source_file);
    free(df);
}

// ==================== ARRAY UTILITIES ====================

void print_int_array_runtime(int64_t size, int64_t* data) {
    printf("[");
    for (int64_t i = 0; i < size; i++) {
        printf("%ld", data[i]);
        if (i < size - 1) printf(", ");
    }
    printf("]\n");
}

void print_float_array_runtime(int64_t size, double* data) {
    printf("[");
    for (int64_t i = 0; i < size; i++) {
        printf("%f", data[i]);
        if (i < size - 1) printf(", ");
    }
    printf("]\n");
}

// ==================== MAIN WRAPPER ====================

extern int64_t user_main();

int main(int argc, char** argv) {
    printf("=== DataLang Runtime Iniciado ===\n\n");
    
    int64_t exit_code = user_main();
    
    printf("\n=== DataLang Runtime Finalizado ===\n");
    printf("Código de saída: %ld\n", exit_code);
    
    return (int)exit_code;
}
