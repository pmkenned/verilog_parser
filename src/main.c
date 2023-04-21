#define STB_DS_IMPLEMENTATION
#include "stb_ds.h"
#include "common.h"
#include "tokenizer.h"
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>

// TODO: consider tokenizing entire file up front
// TODO: use arena allocator
// TODO: helpful error messages
// TODO: TOK_XNOR: "^~" and "~^"?

typedef enum {
    AST_ROOT,
    AST_MODULE_DEF,
    AST_MODULE_BODY,
    AST_PORT_LIST,
    AST_BITRANGE,
    AST_NUMBER,
    AST_INPUT,
    AST_OUTPUT,
    AST_PARAM_LIST,
    AST_INSTANTIATION,
    AST_PORT_MAP_LIST,
    AST_PORT_MAP,
    AST_CONT_ASSIGN,
    AST_ALWAYS,
    AST_SENSITIVITY_LIST,
    AST_INITIAL,
    AST_IF,
    AST_NON_BLOCKING,
    AST_BLOCKING,
    AST_DPI,
    AST_WIRE_DECL,
    AST_REG_DECL,
    AST_IDENT,
    AST_BITWISE_OR,
    AST_BITWISE_AND,
    AST_BITWISE_XOR,
    AST_BITWISE_INVERT,
    AST_LOGICAL_AND,
    AST_LOGICAL_OR,
    AST_EQ,
    AST_NEQ,
    AST_PAREN,
    AST_INDEX,
    AST_CONCAT,
    AST_LITERAL,
    AST_DELAY,
    AST_BLOCK,
} AstNodeType;

typedef struct AstNode {
    AstNodeType type;
    size_t len;
    union {
        char * label;
        int64_t number;
    };
    struct AstNode ** children;
} AstNode;

static void
print_ast_depth(const AstNode * ast, FILE * fp, int depth)
{
    //for (int i = 0; i < depth; i++)
    //    fprintf(fp, "  ");
    switch (ast->type) {
        case AST_ROOT: {
                for (size_t i = 0; i < arrlenu(ast->children); i++) {
                    print_ast_depth(ast->children[i], fp, depth + 1);
                }
            }
            break;
        case AST_MODULE_DEF: {
                fprintf(fp, "module %.*s (\n", (int) ast->len, ast->label);
                print_ast_depth(ast->children[1], fp, depth + 1);
                fprintf(fp, ")\n");
                print_ast_depth(ast->children[2], fp, depth + 1);
                fprintf(fp, "endmodule\n\n");
            }
            break;
        case AST_MODULE_BODY: {
                for (size_t i = 0; i < arrlenu(ast->children); i++) {
                    print_ast_depth(ast->children[i], fp, depth + 1);
                }
            }
            break;
        case AST_PORT_LIST: {
                for (size_t i = 0; i < arrlenu(ast->children); i++) {
                    print_ast_depth(ast->children[i], fp, depth + 1);
                }
            }
            break;
        case AST_BITRANGE: {
                fprintf(fp, "[%ld:%ld]", ast->children[0]->number, ast->children[1]->number);
            }
            break;
        case AST_NUMBER:
            fprintf(fp, "%ld", ast->number);
            break;
        case AST_INPUT: {
                fprintf(fp, "input ");
                for (size_t i = 0; i < arrlenu(ast->children); i++) {
                    print_ast_depth(ast->children[i], fp, depth + 1);
                }
                fprintf(fp, " %.*s,\n", (int) ast->len, ast->label);
            }
            break;
        case AST_OUTPUT: {
                fprintf(fp, "output ");
                for (size_t i = 0; i < arrlenu(ast->children); i++) {
                    print_ast_depth(ast->children[i], fp, depth + 1);
                }
                fprintf(fp, " %.*s,\n", (int) ast->len, ast->label);
            }
            break;
        case AST_PARAM_LIST:
            break;
        case AST_INSTANTIATION:
            fprintf(fp, "%.*s %.*s(\n", (int) ast->children[0]->len, ast->children[0]->label, (int) ast->children[0]->len, ast->children[1]->label);
            print_ast_depth(ast->children[2], fp, depth + 1);
            fprintf(fp, "\n);\n");
            break;
        case AST_PORT_MAP_LIST: {
                for (size_t i = 0; i < arrlenu(ast->children); i++) {
                    if (i > 0)
                        fprintf(fp, ",\n");
                    print_ast_depth(ast->children[i], fp, depth + 1);
                }
            }
            break;
        case AST_PORT_MAP:
            fprintf(fp, ".");
            fprintf(fp, "%.*s", (int) ast->children[0]->len, ast->children[0]->label);
            fprintf(fp, "(");
            print_ast_depth(ast->children[1], fp, depth + 1);
            fprintf(fp, ")");
            break;
        case AST_CONT_ASSIGN:
            fprintf(fp, "assign ");
            print_ast_depth(ast->children[0], fp, depth + 1);
            fprintf(fp, " = ");
            print_ast_depth(ast->children[1], fp, depth + 1);
            fprintf(fp, ";\n");
            break;
        case AST_ALWAYS:
            fprintf(fp, "always ");
            print_ast_depth(ast->children[0], fp, depth + 1);
            fprintf(fp, " ");
            print_ast_depth(ast->children[1], fp, depth + 1);
            break;
        case AST_SENSITIVITY_LIST: {
                fprintf(fp, "@(");
                for (size_t i = 0; i < arrlenu(ast->children); i++) {
                    if (i > 0)
                        fprintf(fp, ",\n");
                    print_ast_depth(ast->children[i], fp, depth + 1);
                }
                fprintf(fp, ")");
            }
            break;
        case AST_INITIAL: {
                fprintf(fp, "initial\n");
                for (size_t i = 0; i < arrlenu(ast->children); i++) {
                    print_ast_depth(ast->children[i], fp, depth + 1);
                }
            }
            break;
        case AST_IF: {
                fprintf(fp, "if (");
                print_ast_depth(ast->children[0], fp, depth + 1);
                fprintf(fp,  ")\n");
                print_ast_depth(ast->children[1], fp, depth + 1);
                if (ast->children[2]) {
                    fprintf(fp, "else\n");
                    print_ast_depth(ast->children[2], fp, depth + 1);
                }
            }
            break;
        case AST_NON_BLOCKING:
            print_ast_depth(ast->children[0], fp, depth + 1);
            fprintf(fp, " <= ");
            print_ast_depth(ast->children[1], fp, depth + 1);
            fprintf(fp, ";\n");
            break;
        case AST_BLOCKING:
            print_ast_depth(ast->children[0], fp, depth + 1);
            fprintf(fp, " = ");
            print_ast_depth(ast->children[1], fp, depth + 1);
            fprintf(fp, ";\n");
            break;
        case AST_DPI:
            fprintf(fp, "$%.*s();\n", (int) ast->len, ast->label);
            break;
        case AST_WIRE_DECL:
        case AST_REG_DECL: {
                if (ast->type == AST_WIRE_DECL)
                    fprintf(fp, "wire");
                else
                    fprintf(fp, "reg");
                if (ast->children[0]) {
                    fprintf(fp, " ");
                    print_ast_depth(ast->children[0], fp, depth + 1);
                }
                fprintf(fp, " %.*s ", (int) ast->len, ast->label);
                for (size_t i = 1; i < arrlenu(ast->children); i++) {
                    print_ast_depth(ast->children[i], fp, depth + 1);
                }
                fprintf(fp, ";\n");
            }
            break;
        case AST_IDENT:
            fprintf(fp, "%.*s", (int) ast->len, ast->label);
            break;
        case AST_BITWISE_OR:
            print_ast_depth(ast->children[0], fp, depth + 1);
            fprintf(fp, " | ");
            print_ast_depth(ast->children[1], fp, depth + 1);
            break;
        case AST_BITWISE_AND:
            print_ast_depth(ast->children[0], fp, depth + 1);
            fprintf(fp, " & ");
            print_ast_depth(ast->children[1], fp, depth + 1);
            break;
        case AST_BITWISE_XOR:
            print_ast_depth(ast->children[0], fp, depth + 1);
            fprintf(fp, " ^ ");
            print_ast_depth(ast->children[1], fp, depth + 1);
            break;
        case AST_BITWISE_INVERT:
            fprintf(fp, "~");
            print_ast_depth(ast->children[0], fp, depth + 1);
            break;
        case AST_LOGICAL_AND:
            print_ast_depth(ast->children[0], fp, depth + 1);
            fprintf(fp, " && ");
            print_ast_depth(ast->children[1], fp, depth + 1);
            break;
        case AST_LOGICAL_OR:
            print_ast_depth(ast->children[0], fp, depth + 1);
            fprintf(fp, " || ");
            print_ast_depth(ast->children[1], fp, depth + 1);
            break;
        case AST_EQ:
            print_ast_depth(ast->children[0], fp, depth + 1);
            fprintf(fp, " == ");
            print_ast_depth(ast->children[1], fp, depth + 1);
            break;
        case AST_NEQ:
            print_ast_depth(ast->children[0], fp, depth + 1);
            fprintf(fp, " != ");
            print_ast_depth(ast->children[1], fp, depth + 1);
            break;
        case AST_PAREN:
            fprintf(fp, "(");
            print_ast_depth(ast->children[0], fp, depth + 1);
            fprintf(fp, ")");
            break;
        case AST_INDEX:
            print_ast_depth(ast->children[0], fp, depth + 1);
            fprintf(fp, "[");
            print_ast_depth(ast->children[1], fp, depth + 1);
            fprintf(fp, "]");
            break;
        case AST_CONCAT:
            break;
        case AST_LITERAL:
            break;
        case AST_DELAY:
            break;
        case AST_BLOCK: {
                fprintf(fp, "begin\n");
                for (size_t i = 0; i < arrlenu(ast->children); i++) {
                    print_ast_depth(ast->children[i], fp, depth + 1);
                }
                fprintf(fp, "end\n");
            }
            break;
        default: assert(0);
    }
}

static void
print_ast(const AstNode * ast, FILE * fp)
{
    print_ast_depth(ast, fp, 0);
}

static void
ast_destroy(AstNode * ast)
{
    if (!ast)
        return;
    for (size_t i = 0; i < arrlenu(ast->children); i++)
        ast_destroy(ast->children[i]);
    arrfree(ast->children);
    free(ast);
}

static Tokenizer tz;

#if 0
static AstNode *
parse_dummy()
{
    Tokenizer saved_tz = tz;
no_match:
    tz = saved_tz;
    return NULL;
}
#endif

// TODO: use this
typedef struct {
    int64_t value;
    int width;
    int is_signed;
} Literal;

static int64_t
parse_literal(char * s)
{
    long size = 32;
    char * endptr = s;
    if (*s != '\'')
        size = strtol(s, &endptr, 10);
    assert(*endptr == '\'');
    endptr++;
    int base = 0;
    if      (*endptr == 'b') base = 2;
    else if (*endptr == 'o') base = 8;
    else if (*endptr == 'd') base = 10;
    else if (*endptr == 'h') base = 16;
    else assert(0);
    endptr++;
    long number = strtol(endptr, NULL, base);
    return number;
}

static AstNode *
parse_bitrange()
{
    Tokenizer saved_tz = tz;

    Token tok1, tok2;
    if (get_token(&tz).type != '[') goto no_match;
    tok1 = get_token(&tz);
    if (tok1.type != TOK_NUMBER)    goto no_match;
    if (get_token(&tz).type != ':') goto no_match;
    tok2 = get_token(&tz);
    if (tok2.type != TOK_NUMBER)    goto no_match;
    if (get_token(&tz).type != ']') goto no_match;

    AstNode * node = malloc(sizeof(*node));
    *node = (AstNode) { .type = AST_BITRANGE, .label = "" };
    AstNode * lchild = malloc(sizeof(*lchild));
    *lchild = (AstNode) { .type = AST_NUMBER, .number = strtol(tok1.str, NULL, 0) };
    AstNode * rchild = malloc(sizeof(*rchild));
    *rchild = (AstNode) { .type = AST_NUMBER, .number = strtol(tok2.str, NULL, 0) };
    arrput(node->children, lchild);
    arrput(node->children, rchild);
    return node;

no_match:
    tz = saved_tz;
    return NULL;
}

static AstNode *
parse_port_decl(int leading_comma)
{
    Tokenizer saved_tz = tz;

    // TODO: handle K&R style

    if (leading_comma && get_token(&tz).type != ',')
        goto no_match;

    AstNodeType port_type;
    Token tok = get_token(&tz);
    if      (tok.type == TOK_INPUT)     port_type = AST_INPUT;
    else if (tok.type == TOK_OUTPUT)    port_type = AST_OUTPUT;
    else                                goto no_match;

    AstNode * bitrange = parse_bitrange();
    tok = get_token(&tz);
    if (tok.type != TOK_IDENT) goto no_match;

    AstNode * node = malloc(sizeof(*node));
    *node = (AstNode) { .type = port_type, .label = tok.str, .len = tok.len };
    if (bitrange) {
        arrput(node->children, bitrange);
    }
    return node;

no_match:
    tz = saved_tz;
    return NULL;
}

static AstNode *
parse_port_list()
{
    AstNode * node = malloc(sizeof(*node));
    *node = (AstNode) { .type = AST_PORT_LIST, .label = "" };
    int leading_comma = 0;
    while (1) {
        AstNode * port_decl = parse_port_decl(leading_comma);
        if (!port_decl)
            break;
        arrput(node->children, port_decl);
        leading_comma = 1;
    }
    return node;
}

static AstNode *
parse_index()
{
    Tokenizer saved_tz = tz;

    Token tok1, tok2;
    tok1 = get_token(&tz);
    if (tok1.type != TOK_IDENT)     goto no_match;
    if (get_token(&tz).type != '[') goto no_match;
    tok2 = get_token(&tz);
    if (tok2.type != TOK_NUMBER)    goto no_match;
    if (get_token(&tz).type != ']') goto no_match;

    AstNode * node = malloc(sizeof(*node));
    *node = (AstNode) { .type = AST_INDEX, .label = "" };
    AstNode * ident = malloc(sizeof(*node));
    *ident = (AstNode) { .type = AST_IDENT, .label = tok1.str, .len = tok1.len };
    AstNode * index = malloc(sizeof(*node));
    *index = (AstNode) { .type = AST_NUMBER, .number = strtol(tok2.str, NULL, 0) };
    arrput(node->children, ident);
    arrput(node->children, index);
    return node;

no_match:
    tz = saved_tz;
    return NULL;
}

static AstNode *
parse_lvalue()
{
    Tokenizer saved_tz = tz;

    AstNode * index = parse_index();
    if (index) return index;
    Token tok = get_token(&tz);
    if (tok.type != TOK_IDENT) goto no_match;

    AstNode * node = malloc(sizeof(*node));
    *node = (AstNode) { .type = AST_IDENT, .label = tok.str, .len = tok.len };
    return node;

no_match:
    tz = saved_tz;
    return NULL;
}

static AstNode * parse_expr();

static AstNode *
parse_parentheses()
{
    Tokenizer saved_tz = tz;

    Token tok = get_token(&tz);
    if (tok.type != '(')    goto no_match;
    AstNode * expr = parse_expr();
    tok = get_token(&tz);
    if (tok.type != ')')    goto no_match;

    AstNode * node = malloc(sizeof(*node));
    *node = (AstNode) { .type = AST_PAREN, .label = "" };
    arrput(node->children, expr);
    return node;

no_match:
    tz = saved_tz;
    return NULL;
}

static AstNode *
parse_unary_op()
{
    // TODO:
    //  parse_and_reduce()
    //  parse_nand_reduce()
    //  parse_or_reduce()
    //  parse_nor_reduce()
    //  parse_xor_reduce()
    //  parse_xnor_reduce()
    //  parse_bitwise_not()
}

static AstNode *
parse_rvalue()
{
    Tokenizer saved_tz = tz;

    AstNode * lvalue = parse_lvalue();
    if (lvalue) return lvalue;
    AstNode * paren = parse_parentheses();
    if (paren) return paren;
    // TODO: parse_unary()
    Token tok = get_token(&tz);
    if (tok.type != TOK_LITERAL) goto no_match;

    AstNode * node = malloc(sizeof(*node));
    *node = (AstNode) { .type = AST_NUMBER, .number = parse_literal(tok.str) };
    return node;

no_match:
    tz = saved_tz;
    return NULL;
}

static AstNode *
parse_binary_op()
{
    Tokenizer saved_tz = tz;

    AstNode * lhs = parse_rvalue();
    if (!lhs)                       goto no_match;
    Token tok = get_token(&tz);
    AstNodeType op_type;
    if      (tok.type == '|')               op_type = AST_BITWISE_OR;
    else if (tok.type == '&')               op_type = AST_BITWISE_AND;
    else if (tok.type == '^')               op_type = AST_BITWISE_XOR;
    else if (tok.type == TOK_LOGICAL_AND)   op_type = AST_LOGICAL_AND;
    else if (tok.type == TOK_LOGICAL_OR)    op_type = AST_LOGICAL_OR;
    else if (tok.type == TOK_EQ)            op_type = AST_EQ;
    else if (tok.type == TOK_NEQ)           op_type = AST_NEQ;
    else                                    goto no_match;
    AstNode * rhs = parse_expr();
    if (!rhs)                       goto no_match;

    AstNode * node = malloc(sizeof(*node));
    *node = (AstNode) { .type = op_type, .label = "" };
    arrput(node->children, lhs);
    arrput(node->children, rhs);
    return node;

no_match:
    if (lhs) ast_destroy(lhs);
    tz = saved_tz;
    return NULL;
}

// TODO: operator precedence
static AstNode *
parse_expr()
{
    AstNode * node;

    // TODO: parse_ternary()
    if      (node = parse_binary_op())  return node;
    else if (node = parse_rvalue())     return node;

    return NULL;
}

static AstNode * parse_procedural_stmt();

static AstNode *
parse_blocking_non_blocking()
{
    Tokenizer saved_tz = tz;

    // TODO: inter- and intra-assignment delays
    AstNode * dst = parse_lvalue();
    if (!dst)                       goto no_match;
    Token tok = get_token(&tz);
    AstNodeType node_type;
    if      (tok.type == TOK_LTE)   node_type = AST_NON_BLOCKING;
    else if (tok.type == '=')       node_type = AST_BLOCKING;
    else                            goto no_match;
    AstNode * expr = parse_expr();
    if (!expr)                      goto no_match;
    if (get_token(&tz).type != ';') goto no_match;

    AstNode * node = malloc(sizeof(*node));
    *node = (AstNode) { .type = node_type, .label = "" };
    arrput(node->children, dst);
    arrput(node->children, expr);
    return node;

no_match:
    tz = saved_tz;
    return NULL;
}

static AstNode *
parse_else()
{
    Tokenizer saved_tz = tz;

    if (get_token(&tz).type != TOK_ELSE) goto no_match;
    AstNode * stmt = parse_procedural_stmt();
    if (!stmt) goto no_match;
    return stmt;

no_match:
    tz = saved_tz;
    return NULL;
}

static AstNode *
parse_if_stmt()
{
    Tokenizer saved_tz = tz;

    if (get_token(&tz).type != TOK_IF)  goto no_match;
    if (get_token(&tz).type != '(')     goto no_match;
    AstNode * cond = parse_expr();
    if (!cond)                          goto no_match;
    if (get_token(&tz).type != ')')     goto no_match;
    AstNode * stmt = parse_procedural_stmt();
    if (!stmt)                          goto no_match;
    AstNode * else_node = parse_else();

    // TODO: should else-if be handled specifically?
    AstNode * node = malloc(sizeof(*node));
    *node = (AstNode) { .type = AST_IF, .label = "" };
    arrput(node->children, cond);
    arrput(node->children, stmt);
    arrput(node->children, else_node);
    return node;

no_match:
    tz = saved_tz;
    return NULL;
}

static AstNode *
parse_delay()
{
    Tokenizer saved_tz = tz;

    if (get_token(&tz).type != '#') goto no_match;
    Token tok = get_token(&tz);
    if (tok.type != TOK_NUMBER)     goto no_match;

    AstNode * node = malloc(sizeof(*node));
    *node = (AstNode) { .type = AST_DELAY, .number = strtol(tok.str, NULL, 0) };
    return node;

no_match:
    tz = saved_tz;
    return NULL;
}

static AstNode *
parse_dpi()
{
    Tokenizer saved_tz = tz;

    if (get_token(&tz).type != '$') goto no_match;
    Token tok = get_token(&tz);
    if (tok.type != TOK_IDENT)      goto no_match;
    if (get_token(&tz).type != '(') goto no_match;
    // TODO: arguments
    if (get_token(&tz).type != ')') goto no_match;

    AstNode * node = malloc(sizeof(*node));
    *node = (AstNode) { .type = AST_DPI, .label = tok.str, .len = tok.len };
    return node;

no_match:
    tz = saved_tz;
    return NULL;
}

static AstNode *
parse_block()
{
    Tokenizer saved_tz = tz;

    if (get_token(&tz).type != TOK_BEGIN)   goto no_match;
    AstNode * node = malloc(sizeof(*node));
    *node = (AstNode) { .type = AST_BLOCK, .label = "" };
    while (1) {
        AstNode * stmt = parse_procedural_stmt();
        if (!stmt)
            break;
        arrput(node->children, stmt);
    }
    if (get_token(&tz).type != TOK_END)     goto no_match;
    return node;

no_match:
    tz = saved_tz;
    return NULL;
}

static AstNode *
parse_empty_stmt()
{
    Tokenizer saved_tz = tz;
    if (get_token(&tz).type != ';') goto no_match;
no_match:
    tz = saved_tz;
    return NULL;
}

static AstNode *
parse_procedural_stmt()
{
    Tokenizer saved_tz = tz;

    AstNode * stmt;
    if      (stmt = parse_block())                  ;
    else if (stmt = parse_if_stmt())                ;
    else if (stmt = parse_blocking_non_blocking())  ;
    else if (stmt = parse_delay())                  ;
    else if (stmt = parse_dpi())                    ;
    else if (stmt = parse_empty_stmt())             ;
    else                                            goto no_match;

    return stmt;

no_match:
    tz = saved_tz;
    return NULL;
}

static AstNode *
parse_sensitivity_list()
{
    Tokenizer saved_tz = tz;

    // TODO: this is hard-coded
    if (get_token(&tz).type != TOK_POSEDGE) goto no_match;
    if (get_token(&tz).type != TOK_IDENT)   goto no_match;
    if (get_token(&tz).type != TOK_OR)      goto no_match;
    if (get_token(&tz).type != TOK_POSEDGE) goto no_match;
    if (get_token(&tz).type != TOK_IDENT)   goto no_match;

    AstNode * node = malloc(sizeof(*node));
    *node = (AstNode) { .type = AST_SENSITIVITY_LIST, .label = "" };
    return node;

no_match:
    tz = saved_tz;
    return NULL;
}

static AstNode *
parse_always()
{
    Tokenizer saved_tz = tz;

    if (get_token(&tz).type != TOK_ALWAYS)  goto no_match;
    if (get_token(&tz).type != '@')         goto no_match;
    if (get_token(&tz).type != '(')         goto no_match;
    AstNode * sensitivity_list = parse_sensitivity_list();
    if (!sensitivity_list)                  goto no_match;
    if (get_token(&tz).type != ')')         goto no_match;
    AstNode * stmt = parse_block();

    AstNode * node = malloc(sizeof(*node));
    *node = (AstNode) { .type = AST_ALWAYS, .label = "" };
    arrput(node->children, sensitivity_list);
    arrput(node->children, stmt);
    return node;

no_match:
    tz = saved_tz;
    return NULL;
}

static AstNode *
parse_initial()
{
    Tokenizer saved_tz = tz;

    if (get_token(&tz).type != TOK_INITIAL) goto no_match;
    AstNode * stmt = parse_block();

    AstNode * node = malloc(sizeof(*node));
    *node = (AstNode) { .type = AST_INITIAL, .label = "" };
    arrput(node->children, stmt);
    return node;

no_match:
    tz = saved_tz;
    return NULL;
}

static AstNode *
parse_port_map(int leading_comma)
{
    Tokenizer saved_tz = tz;

    if (leading_comma && get_token(&tz).type != ',')    goto no_match;

    Token tok1, tok2;
    if (get_token(&tz).type != '.') goto no_match;
    tok1 = get_token(&tz);
    if (tok1.type != TOK_IDENT)     goto no_match;
    if (get_token(&tz).type != '(') goto no_match;
    AstNode * rchild = parse_expr();
    if (!rchild)                    goto no_match;
    if (get_token(&tz).type != ')') goto no_match;

    AstNode * node = malloc(sizeof(*node));
    *node = (AstNode) { .type = AST_PORT_MAP, .label = "" };
    AstNode * lchild = malloc(sizeof(*node));
    *lchild = (AstNode) { .type = AST_IDENT, .label = tok1.str, .len = tok1.len };
    arrput(node->children, lchild);
    arrput(node->children, rchild);
    return node;

no_match:
    tz = saved_tz;
    return NULL;
}

static AstNode *
parse_port_map_list()
{
    AstNode * node = malloc(sizeof(*node));
    *node = (AstNode) { .type = AST_PORT_MAP_LIST, .label = "" };

    int leading_comma = 0;
    while (1) {
        AstNode * port_map = parse_port_map(leading_comma);
        if (!port_map)
            break;
        arrput(node->children, port_map);
        leading_comma = 1;
    }

    return node;
}

static AstNode *
parse_instantiation()
{
    Tokenizer saved_tz = tz;

    Token tok1, tok2;
    tok1 = get_token(&tz); // module name
    if (tok1.type != TOK_IDENT)     goto no_match;
    tok2 = get_token(&tz); // instance name
    if (tok2.type != TOK_IDENT)     goto no_match;
    if (get_token(&tz).type != '(') goto no_match;
    AstNode * port_map_list = parse_port_map_list();
    if (get_token(&tz).type != ')') goto no_match;
    if (get_token(&tz).type != ';') goto no_match;

    AstNode * node = malloc(sizeof(*node));
    *node = (AstNode) { .type = AST_INSTANTIATION, .label = "" };
    AstNode * module_name = malloc(sizeof(*module_name));
    *module_name = (AstNode) { .type = AST_IDENT, .label = tok1.str, .len = tok1.len };
    AstNode * instance_name = malloc(sizeof(*instance_name));
    *instance_name = (AstNode) { .type = AST_IDENT, .label = tok2.str, .len = tok2.len };
    arrput(node->children, module_name);
    arrput(node->children, instance_name);
    arrput(node->children, port_map_list);
    return node;

no_match:
    tz = saved_tz;
    return NULL;
}

static AstNode *
parse_signal_decl()
{
    Tokenizer saved_tz = tz;

    AstNode * node = NULL;

    Token tok = get_token(&tz);
    AstNodeType signal_type;
    if      (tok.type == TOK_WIRE)  signal_type = AST_WIRE_DECL;
    else if (tok.type == TOK_REG)   signal_type = AST_REG_DECL;
    else                            goto no_match;

    AstNode * bitrange = parse_bitrange();
    tok = get_token(&tz);
    // TODO: multiple declaration: wire a, b, c;
    if (tok.type != TOK_IDENT)      goto no_match;

    node = malloc(sizeof(*node));
    *node = (AstNode) { .type = signal_type, .label = tok.str, .len = tok.len };
    arrput(node->children, bitrange);
    AstNode * array;
    while (array = parse_bitrange()) {
        arrput(node->children, array);
    }
    if (get_token(&tz).type != ';') goto no_match;

    return node;

no_match:
    if (node) ast_destroy(node);
    tz = saved_tz;
    return NULL;
}

static AstNode *
parse_param_decl()
{
    Tokenizer saved_tz = tz;
    // TODO
no_match:
    tz = saved_tz;
    return NULL;
}

static AstNode *
parse_assign()
{
    Tokenizer saved_tz = tz;

    if (get_token(&tz).type != TOK_ASSIGN)  goto no_match;
    Token tok = get_token(&tz);
    if (tok.type != TOK_IDENT)              goto no_match;
    if (get_token(&tz).type != '=')         goto no_match;
    AstNode * expr = parse_expr();
    if (!expr)                              goto no_match;
    if (get_token(&tz).type != ';')         goto no_match;

    AstNode * assign = malloc(sizeof(*assign));
    *assign = (AstNode) { .type = AST_CONT_ASSIGN, .label = "" };
    AstNode * dst = malloc(sizeof(*assign));
    *dst = (AstNode) { .type = AST_IDENT, .label = tok.str, .len = tok.len };
    arrput(assign->children, dst);
    arrput(assign->children, expr);
    return assign;

no_match:
    tz = saved_tz;
    return NULL;
}

static AstNode *
parse_module_body()
{
    AstNode * body = malloc(sizeof(*body));
    *body = (AstNode) { .type = AST_MODULE_BODY, .label = "" };

    while (1) {
        AstNode * stmt;
        if      (stmt = parse_signal_decl())    ;
        else if (stmt = parse_param_decl())     ;
        else if (stmt = parse_assign())         ;
        else if (stmt = parse_instantiation())  ;
        else if (stmt = parse_always())         ;
        else if (stmt = parse_initial())        ;
        else                                    break;
        arrput(body->children, stmt);
    }

    return body;
}

static AstNode *
parse_module_def()
{
    Tokenizer saved_tz = tz;

    Token tok;
    if (get_token(&tz).type != TOK_MODULE)          goto no_match;
    if ((tok = get_token(&tz)).type != TOK_IDENT)   goto no_match;
    if (get_token(&tz).type != '(')                 goto no_match;
    AstNode * port_list = parse_port_list();
    if (get_token(&tz).type != ')')                 goto no_match;
    if (get_token(&tz).type != ';')                 goto no_match;
    AstNode * body = parse_module_body();
    if (!body)                                      goto no_match;
    if (get_token(&tz).type != TOK_ENDMODULE)       goto no_match;

    AstNode * node = malloc(sizeof(*node));
    *node = (AstNode) { .type = AST_MODULE_DEF, .label = tok.str, .len = tok.len };
    arrput(node->children, NULL);
    arrput(node->children, port_list);
    arrput(node->children, body);
    return node;

no_match:
    tz = saved_tz;
    return NULL;
}

static AstNode *
parse_verilog(Buffer input)
{
    tz = init_tokenizer(input);

    AstNode * node = malloc(sizeof(*node));
    *node = (AstNode) { .type = AST_ROOT, .label = "" };
    AstNode * module_def = parse_module_def();
    if (module_def) {
        arrput(node->children, module_def);
    } else {
        // TODO: error
    }
    return node;
}

int main(int argc, char * argv[])
{
    const char * filename = NULL;
    for (int i = 1; i < argc; i++) {
        // TODO: allow for multiple input files
        if (filename == NULL)
            filename = argv[i];
        else
            die("error: cannot specify multiple input files\n");
    }

    if (filename == NULL) {
        die("usage: %s FILE\n", argv[0]);
    }

    Buffer file_contents = read_file(filename);
    // TODO: strip_comments(file_contents.p, file_contents.len);
    AstNode * ast = parse_verilog(file_contents);
    print_ast(ast, stdout);
    ast_destroy(ast);

    return 0;
}
