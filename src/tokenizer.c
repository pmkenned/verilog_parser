#include "common.h"
#include "tokenizer.h"
#include <string.h>
#include <assert.h>
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdbool.h>

// TODO: literals e.g. 4'd7

// TODO: use X-macros for these
#if 1
const char * token_strs[] = {
    [TOK_NONE] = "NONE",
    [','] = ",",
    [':'] = ":",
    [';'] = ";",
    ['\''] = "'",
    ['='] = "=",
    ['.'] = ".",
    ['('] = "(",
    [')'] = ")",
    ['['] = "[",
    [']'] = "]",
    ['|'] = "|",
    ['@'] = "@",
    ['\n'] = "\\n",
    [TOK_MODULE]        = "MODULE",
    [TOK_ENDMODULE]     = "ENDMODULE",
    [TOK_WIRE]          = "WIRE",
    [TOK_REG]           = "REG",
    [TOK_ALWAYS]        = "ALWAYS",
    [TOK_INITIAL]       = "INITIAL",
    [TOK_BEGIN]         = "BEGIN",
    [TOK_END]           = "END",
    [TOK_IF]            = "IF",
    [TOK_ELSE]          = "ELSE",
    [TOK_ASSIGN]        = "ASSIGN",
    [TOK_INPUT]         = "INPUT",
    [TOK_OUTPUT]        = "OUTPUT",
//  [TOK_NON_BLOCKING]  = "NON_BLOCKING",
    [TOK_IDENT]         = "IDENT",
    [TOK_NUMBER]        = "NUMBER",
    [TOK_LITERAL]       = "LITERAL",
    [TOK_POSEDGE]       = "POSEDGE",
    [TOK_NEGEDGE]       = "NEGEDGE",
    [TOK_OR]            = "OR",
    [TOK_LOGICAL_AND]   = "LOGICAL_AND",
    [TOK_LOGICAL_OR]    = "LOGICAL_OR",
    [TOK_EQ]            = "EQ",
    [TOK_NEQ]           = "NEQ",
    [TOK_EOF]           = "EOF",
    [TOK_STRING]        = "STRING",
    [TOK_LTE]           = "LTE",
    [TOK_GTE]           = "GTE",
    [TOK_LSH]           = "LSH",
    [TOK_RSH]           = "RSH",
    [TOK_INVALID]       = "INVALID"
};
#endif

typedef struct {
    char * str;
    size_t len;
    TokenType tok_type;
} Keyword;

static Keyword keywords[] = {
    { "module",     6,  TOK_MODULE      },
    { "endmodule",  9,  TOK_ENDMODULE   },
    { "wire",       4,  TOK_WIRE        },
    { "reg",        3,  TOK_REG         },
    { "always",     6,  TOK_ALWAYS      },
    { "initial",    7,  TOK_INITIAL     },
    { "begin",      5,  TOK_BEGIN       },
    { "end",        3,  TOK_END         },
    { "if",         2,  TOK_IF          },
    { "else",       4,  TOK_ELSE        },
    { "assign",     6,  TOK_ASSIGN      },
    { "input",      5,  TOK_INPUT       },
    { "output",     6,  TOK_OUTPUT      },
    { "posedge",    7,  TOK_POSEDGE     },
    { "negedge",    7,  TOK_NEGEDGE     },
    { "or",         2,  TOK_OR          },
};

static char
get_char(Tokenizer * tz)
{
    if (tz->buf_pos >= tz->buffer.len)
        return '\0';
    return tz->buffer.p[tz->buf_pos++];
}

static char
peek_char(Tokenizer * tz)
{
    if (tz->buf_pos >= tz->buffer.len)
        return '\0';
    return tz->buffer.p[tz->buf_pos];
}

static Token
init_token(Tokenizer * tz, TokenType type)
{
    return (Token) {
        .type = type,
        .str = &tz->buffer.p[tz->buf_pos],
        .len = 0
    };
}

static Token
get_ident_or_keyword(Tokenizer * tz)
{
    Token tok = init_token(tz, TOK_IDENT);

    while (1) {
        char c = peek_char(tz);
        if (!isalnum(c) && c != '_')
            break;
        get_char(tz);
        tok.len++;
    }

    for (size_t i = 0; i < NELEMS(keywords); i++) {
        if (tok.len == keywords[i].len &&
            !strncmp(tok.str, keywords[i].str, tok.len)) {
            tok.type = keywords[i].tok_type;
            break;
        }
    }

    return tok;
}

static Token
get_literal(Tokenizer * tz)
{
    Token tok = init_token(tz, TOK_LITERAL);
    char c = peek_char(tz);
    if (isdigit(c)) {
        while (1) {
            get_char(tz);
            tok.len++;
            c = peek_char(tz);
            if (c == '\'') {
                tok.len++;
                break;
            } else if (!isdigit(c)) {
                tok.type = TOK_NUMBER;
                break;
            }
        }
    } else if (c == '\'') {
        tok.len++;
    } else {
        assert(0);
    }
    if (tok.type == TOK_NUMBER)
        return tok;
    assert(c == '\'');
    get_char(tz); // discard '

    c = get_char(tz);
    tok.len++;
    int base;
    if      (c == 'b') base = 2;
    else if (c == 'o') base = 8;
    else if (c == 'd') base = 10;
    else if (c == 'h') base = 16;
    else assert(0); // TODO: error-handling

    while (1) {
        c = peek_char(tz);
        if (base == 2) {
            if (c != '0' && c != '1')
                break;
        } else if (base == 8) {
            if (c < '0' || c > '7')
                break;
        } else if (base == 10) {
            if (!isdigit(c))
                break;
        } else if (base == 16) {
            if (!isxdigit(c))
                break;
        }
        get_char(tz);
        tok.len++;
    }
    return tok;
}

#if 0
static Token
get_number(Tokenizer * tz)
{
    Token tok = init_token(tz, TOK_NUMBER);
    while (1) {
        char c = peek_char(tz);
        if (!isdigit(c))
            break;
        get_char(tz);
        tok.len++;
    }
    return tok;
}
#endif

static Token
get_string(Tokenizer * tz)
{
    get_char(tz); // discard "
    Token tok = init_token(tz, TOK_STRING);
    int prev_esc = 0;
    while (1) {
        char c = get_char(tz);
        if ((c == '"' && !prev_esc) || c == '\0')
            break;
        prev_esc = c == '\\';
        tok.len++;
    }
    return tok;
}

static Token
get_syntax(Tokenizer * tz)
{
    Token tok = init_token(tz, TOK_STRING);

    tok.type = get_char(tz);
    TokenType tok_type = TOK_NONE;
    char c = peek_char(tz);
    switch (tok.type) {
        case '|':
            if      (c == '|') tok_type = TOK_LOGICAL_OR;
            break;
        case '&':
            if      (c == '&') tok_type = TOK_LOGICAL_AND;
            break;
        case '<':
            if      (c == '=') tok_type = TOK_LTE;
            else if (c == '<') tok_type = TOK_LSH;
            break;
        case '>':
            if      (c == '=') tok_type = TOK_GTE;
            else if (c == '>') tok_type = TOK_RSH;
            break;
        case '=':
            if      (c == '=') tok_type = TOK_EQ;
            break;
        //case '+':
        //    if      (c == '=') tok_type = TOK_PLUS_EQ;
        //    else if (c == '+') tok_type = TOK_INC;
        //    break;
    }

    tok.len = 1;
    if (tok_type != TOK_NONE) {
        get_char(tz);
        tok.len++;
        tok.type = tok_type;
    }

    return tok;
}

Tokenizer
init_tokenizer(Buffer buffer)
{
    Tokenizer tz = {
        .buffer = buffer,
        .buf_pos = 0,
    };
    return tz;
}

void
print_token(Token tok)
{
    char tmp[32];
    assert(tok.len < sizeof(tmp));
    strncpy(tmp, tok.str, tok.len);
    tmp[tok.len] = '\0';
    fprintf(stderr, "%s '%s'\n", token_strs[tok.type], tmp);
}

Token
get_token(Tokenizer * tz)
{
    while (isspace(peek_char(tz))) {
        get_char(tz);
    }
    Token tok;
    char c = peek_char(tz);
    if      (c == '\0')                 tok = (Token) { .type = TOK_EOF, .str = "", .len = 0 };
    else if (isalpha(c) || c == '_')    tok = get_ident_or_keyword(tz);
    else if (isdigit(c) || c == '\'')   tok = get_literal(tz);
    else if (c == '"')                  tok = get_string(tz);
    else                                tok = get_syntax(tz);
    //print_token(tok);
    return tok;
}
