#ifndef TOKENIZER_H
#define TOKENIZER_H

typedef enum {
    TOK_NONE=0,
    TOK_MODULE=256,
    TOK_ENDMODULE,
    TOK_WIRE,
    TOK_REG,
    TOK_ALWAYS,
    TOK_INITIAL,
    TOK_BEGIN,
    TOK_END,
    TOK_IF,
    TOK_ELSE,
    TOK_ASSIGN,
    TOK_INPUT,
    TOK_OUTPUT,
//  TOK_NON_BLOCKING,
    TOK_IDENT,
    TOK_NUMBER,
    TOK_LITERAL,
    TOK_POSEDGE,
    TOK_NEGEDGE,
    TOK_OR,
    TOK_LOGICAL_AND,
    TOK_LOGICAL_OR,
    TOK_EQ,
    TOK_NEQ,
    TOK_EOF,
    TOK_STRING,
    TOK_LTE,
    TOK_GTE,
    TOK_LSH,
    TOK_RSH,
    TOK_INVALID
} TokenType;

typedef struct {
    TokenType type;
    char * str;
    size_t len;
} Token;

typedef struct {
    Buffer buffer;
    //int ln; // TODO
    size_t buf_pos;
} Tokenizer;

Tokenizer init_tokenizer(Buffer buffer);
void print_token(Token tok);
Token get_token(Tokenizer * tz);

#endif /* TOKENIZER_H */
