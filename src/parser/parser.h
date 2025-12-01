#ifndef DATALANG_PARSER_H
#define DATALANG_PARSER_H

#include <stdbool.h>
#include "../lexer/lexer.h"

// ==================== TIPOS DE NÓS DA AST ====================

typedef enum {
    // Programa e Declarações
    AST_PROGRAM,
    AST_LET_DECL,
    AST_FN_DECL,
    AST_DATA_DECL,
    AST_IMPORT_DECL,
    AST_EXPORT_DECL,
    AST_FIELD_DECL,
    AST_PARAM,
    
    // Statements
    AST_IF_STMT,
    AST_FOR_STMT,
    AST_RETURN_STMT,
    AST_PRINT_STMT,
    AST_EXPR_STMT,
    AST_BLOCK,
    
    // Expressões
    AST_BINARY_EXPR,
    AST_UNARY_EXPR,
    AST_CALL_EXPR,
    AST_INDEX_EXPR,
    AST_MEMBER_EXPR,
    AST_ASSIGN_EXPR,
    AST_LAMBDA_EXPR,
    AST_PIPELINE_EXPR,
    
    // Transformações
    AST_FILTER_TRANSFORM,
    AST_MAP_TRANSFORM,
    AST_REDUCE_TRANSFORM,
    AST_SELECT_TRANSFORM,
    AST_GROUPBY_TRANSFORM,
    AST_AGGREGATE_TRANSFORM,
    
    // Primários
    AST_LITERAL,
    AST_IDENTIFIER,
    AST_ARRAY_LITERAL,
    AST_LOAD_EXPR,
    AST_SAVE_EXPR,
    AST_RANGE_EXPR,
    
    // Tipo
    AST_TYPE
} ASTNodeType;

typedef enum {
    BINOP_ADD, BINOP_SUB, BINOP_MUL, BINOP_DIV, BINOP_MOD,
    BINOP_EQ, BINOP_NEQ, BINOP_LT, BINOP_LTE, BINOP_GT, BINOP_GTE,
    BINOP_AND, BINOP_OR, BINOP_RANGE
} BinaryOp;

typedef enum {
    UNOP_NEG, UNOP_NOT
} UnaryOp;

typedef enum {
    AGG_SUM, AGG_MEAN, AGG_COUNT, AGG_MIN, AGG_MAX
} AggregateType;

// ==================== ESTRUTURA DA AST ====================

typedef struct ASTNode ASTNode;

struct ASTNode {
    ASTNodeType type;
    int line;
    int column;
    
    union {
        // Program
        struct {
            ASTNode** declarations;
            int decl_count;
        } program;
        
        // Let Declaration
        struct {
            char* name;
            ASTNode* type_annotation;
            ASTNode* initializer;
        } let_decl;

        // Print Statement
        struct {
            ASTNode** expressions;  // array de ponteiros
            int expr_count;         // Contador
        } print_stmt;
        
        // Function Declaration
        struct {
            char* name;
            ASTNode** params;
            int param_count;
            ASTNode* return_type;
            ASTNode* body;
        } fn_decl;
        
        // Data Declaration
        struct {
            char* name;
            ASTNode** fields;
            int field_count;
        } data_decl;
        
        // Field Declaration
        struct {
            char* field_name;
            ASTNode* field_type;
        } field_decl;
        
        // Parameter
        struct {
            char* param_name;
            ASTNode* param_type;
        } param;
        
        // Import/Export
        struct {
            char* module_path;
            char* alias;
        } import_decl;
        
        struct {
            char* export_name;
        } export_decl;
        
        // If Statement
        struct {
            ASTNode* condition;
            ASTNode* then_block;
            ASTNode* else_block;
        } if_stmt;
        
        // For Statement
        struct {
            char* iterator;
            ASTNode* iterable;
            ASTNode* body;
        } for_stmt;
        
        // Return Statement
        struct {
            ASTNode* value;
        } return_stmt;
        
        // Expression Statement
        struct {
            ASTNode* expression;
        } expr_stmt;
        
        // Block
        struct {
            ASTNode** statements;
            int stmt_count;
        } block;
        
        // Binary Expression
        struct {
            BinaryOp op;
            ASTNode* left;
            ASTNode* right;
        } binary_expr;
        
        // Unary Expression
        struct {
            UnaryOp op;
            ASTNode* operand;
        } unary_expr;
        
        // Call Expression
        struct {
            ASTNode* callee;
            ASTNode** arguments;
            int arg_count;
        } call_expr;
        
        // Index Expression
        struct {
            ASTNode* object;
            ASTNode* index;
        } index_expr;
        
        // Member Expression
        struct {
            ASTNode* object;
            char* member;
        } member_expr;
        
        // Assignment Expression
        struct {
            ASTNode* target;
            ASTNode* value;
        } assign_expr;
        
        // Lambda Expression
        struct {
            ASTNode** lambda_params;
            int lambda_param_count;
            ASTNode* lambda_body;
        } lambda_expr;
        
        // Pipeline Expression
        struct {
            ASTNode** stages;
            int stage_count;
        } pipeline_expr;
        
        // Filter Transform
        struct {
            ASTNode* filter_predicate;
        } filter_transform;
        
        // Map Transform
        struct {
            ASTNode* map_function;
        } map_transform;
        
        // Reduce Transform
        struct {
            ASTNode* initial_value;
            ASTNode* reducer;
        } reduce_transform;
        
        // Select Transform
        struct {
            char** columns;
            int column_count;
        } select_transform;
        
        // GroupBy Transform
        struct {
            char** group_columns;
            int group_column_count;
        } groupby_transform;
        
        // Aggregate Transform
        struct {
            AggregateType agg_type;
            ASTNode** agg_args;
            int agg_arg_count;
        } aggregate_transform;
        
        // Literal
        struct {
            TokenType literal_type;
            long long int_value;
            double float_value;
            char* string_value;
            bool bool_value;
        } literal;
        
        // Identifier
        struct {
            char* id_name;
        } identifier;
        
        // Array Literal
        struct {
            ASTNode** elements;
            int element_count;
        } array_literal;
        
        // Load Expression
        struct {
            char* file_path;
        } load_expr;
        
        // Save Expression
        struct {
            ASTNode* data;
            char* save_path;
        } save_expr;
        
        // Range Expression
        struct {
            ASTNode* range_start;
            ASTNode* range_end;
        } range_expr;
        
        // Type
        struct {
            int type_kind;
            char* type_name;
            ASTNode* inner_type;
            ASTNode** tuple_types;
            int tuple_type_count;
        } type_node;
    };
};

// ==================== ESTRUTURA DO PARSER ====================

typedef struct {
    Token* tokens;
    int token_count;
    int current;
    bool had_error;
    bool panic_mode;
    char** error_messages;
    int error_count;
    int error_capacity;
} Parser;

// ==================== FUNÇÕES PÚBLICAS ====================

// Funções principais do parser
Parser* create_parser(TokenStream* stream);
void free_parser(Parser* parser);
ASTNode* parse(Parser* parser);

// Funções utilitárias (públicas para uso em parser_expr.c)
Token* peek(Parser* p);
Token* previous(Parser* p);
Token* advance(Parser* p);
bool is_at_end(Parser* p);
bool check(Parser* p, TokenType type);
bool match(Parser* p, int count, ...);
void error(Parser* p, const char* message);
void error_at(Parser* p, Token* token, const char* message);
Token* consume(Parser* p, TokenType type, const char* message);
void synchronize(Parser* p);
ASTNode* create_node(ASTNodeType type, int line, int column);
ASTNode* parse_program(Parser* p);

// Funções de parsing (públicas para uso entre módulos)
ASTNode* parse_expression(Parser* p);
ASTNode* parse_statement(Parser* p);
ASTNode* parse_type(Parser* p);
ASTNode* parse_block(Parser* p);

// Funções para liberar AST
void free_ast(ASTNode* node);
void print_ast(ASTNode* node, int indent);


#endif // DATALANG_PARSER_H