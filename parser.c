#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include "parser.h"
#include "eval.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
#ifndef M_E
#define M_E  2.71828182845904523536
#endif

/* ----- tokenizador (interno) ----- */

typedef enum { T_NUM, T_IDENT, T_OP, T_LPAREN, T_RPAREN, T_END } TokKind;
typedef struct { TokKind kind; double num; char text[32]; char op; } Token;

#define MAX_TOK 256
typedef struct {
    Token tok[MAX_TOK];
    int ntok, pos, error;
    char msg[PARSER_ERRMSG_LEN];
} Parser;

static void fail(Parser *p, const char *m) {
    if (!p->error) { p->error = 1; snprintf(p->msg, sizeof p->msg, "%s", m); }
}

static int tokenize(Parser *p, const char *s) {
    p->ntok = 0; p->pos = 0; p->error = 0; p->msg[0] = '\0';
    while (*s) {
        if (isspace((unsigned char)*s)) { s++; continue; }
        if (p->ntok >= MAX_TOK - 1) { fail(p, "expressao longa demais"); return -1; }
        Token *t = &p->tok[p->ntok];
        if (isdigit((unsigned char)*s) || *s == '.') {
            char *end; t->kind = T_NUM; t->num = strtod(s, &end);
            if (end == s) { fail(p, "numero malformado"); return -1; }
            s = end; p->ntok++; continue;
        }
        if (isalpha((unsigned char)*s)) {
            int i = 0; t->kind = T_IDENT;
            while (isalnum((unsigned char)*s) && i < (int)sizeof t->text - 1) t->text[i++] = *s++;
            t->text[i] = '\0'; p->ntok++; continue;
        }
        switch (*s) {
            case '+': case '-': case '*': case '/': case '^':
                t->kind = T_OP; t->op = *s; p->ntok++; s++; continue;
            case '(': t->kind = T_LPAREN; p->ntok++; s++; continue;
            case ')': t->kind = T_RPAREN; p->ntok++; s++; continue;
            default: fail(p, "caractere inesperado"); return -1;
        }
    }
    p->tok[p->ntok].kind = T_END;
    return 0;
}

/* ----- tabelas de nomes (internas) ----- */

static int lookup_func(const char *s, FuncId *out) {
    static const struct { const char *name; FuncId id; } tbl[] = {
        {"sin",F_SIN},{"cos",F_COS},{"tan",F_TAN},{"cot",F_COT},
        {"sec",F_SEC},{"csc",F_CSC},{"asin",F_ASIN},{"acos",F_ACOS},
        {"atan",F_ATAN},{"sinh",F_SINH},{"cosh",F_COSH},{"tanh",F_TANH},
        {"exp",F_EXP},{"ln",F_LN},{"log",F_LN},{"log10",F_LOG10},
        {"log2",F_LOG2},{"sqrt",F_SQRT},{"cbrt",F_CBRT},{"abs",F_ABS},
    };
    for (size_t i = 0; i < sizeof tbl / sizeof tbl[0]; i++)
        if (strcmp(s, tbl[i].name) == 0) { *out = tbl[i].id; return 1; }
    return 0;
}

static int lookup_const(const char *s, double *out) {
    if (strcmp(s, "pi") == 0) { *out = M_PI; return 1; }
    if (strcmp(s, "e")  == 0) { *out = M_E;  return 1; }
    return 0;
}

/* ----- descida recursiva (interna) ----- */

static Node *parse_expr(Parser *p);
static Node *parse_power(Parser *p);

static Token *peek(Parser *p)    { return &p->tok[p->pos]; }
static Token *advance(Parser *p) { return &p->tok[p->pos++]; }
static int begins_factor(Token *t) {
    return t->kind == T_NUM || t->kind == T_IDENT || t->kind == T_LPAREN;
}

static Node *parse_primary(Parser *p) {
    Token *t = peek(p);
    if (t->kind == T_NUM) { advance(p); Node *n = ast_new(N_NUM); n->value = t->num; return n; }
    if (t->kind == T_IDENT) {
        FuncId fid; double cval;
        if (lookup_func(t->text, &fid)) {
            advance(p);
            if (peek(p)->kind != T_LPAREN) { fail(p, "esperava '(' apos funcao"); return NULL; }
            advance(p);
            Node *arg = parse_expr(p);
            if (peek(p)->kind != T_RPAREN) { fail(p, "esperava ')'"); ast_free(arg); return NULL; }
            advance(p);
            Node *n = ast_new(N_FUNC); n->func = fid; n->a = arg; return n;
        }
        if (strcmp(t->text, "x") == 0) { advance(p); return ast_new(N_VAR); }
        if (lookup_const(t->text, &cval)) { advance(p); Node *n = ast_new(N_NUM); n->value = cval; return n; }
        fail(p, "identificador desconhecido"); return NULL;
    }
    if (t->kind == T_LPAREN) {
        advance(p);
        Node *e = parse_expr(p);
        if (peek(p)->kind != T_RPAREN) { fail(p, "esperava ')'"); ast_free(e); return NULL; }
        advance(p); return e;
    }
    fail(p, "expressao primaria invalida"); return NULL;
}

static Node *parse_unary(Parser *p) {
    Token *t = peek(p);
    if (t->kind == T_OP && (t->op == '+' || t->op == '-')) {
        char op = t->op; advance(p);
        Node *operand = parse_unary(p);
        if (p->error) return operand;
        if (op == '+') return operand;
        Node *n = ast_new(N_NEG); n->a = operand; return n;
    }
    return parse_power(p);
}

static Node *parse_power(Parser *p) {
    Node *base = parse_primary(p);
    if (p->error) return base;
    Token *t = peek(p);
    if (t->kind == T_OP && t->op == '^') {
        advance(p);
        Node *exp = parse_unary(p);
        Node *n = ast_new(N_POW); n->a = base; n->b = exp; return n;
    }
    return base;
}

static Node *parse_term(Parser *p) {
    Node *left = parse_unary(p);
    if (p->error) return left;
    for (;;) {
        Token *t = peek(p);
        if (t->kind == T_OP && (t->op == '*' || t->op == '/')) {
            char op = t->op; advance(p);
            Node *right = parse_unary(p);
            if (p->error) { ast_free(left); return right; }
            Node *n = ast_new(op == '*' ? N_MUL : N_DIV); n->a = left; n->b = right; left = n;
        } else if (begins_factor(t)) {
            Node *right = parse_unary(p);
            if (p->error) { ast_free(left); return right; }
            Node *n = ast_new(N_MUL); n->a = left; n->b = right; left = n;
        } else break;
    }
    return left;
}

static Node *parse_expr(Parser *p) {
    Node *left = parse_term(p);
    if (p->error) return left;
    for (;;) {
        Token *t = peek(p);
        if (t->kind == T_OP && (t->op == '+' || t->op == '-')) {
            char op = t->op; advance(p);
            Node *right = parse_term(p);
            if (p->error) { ast_free(left); return right; }
            Node *n = ast_new(op == '+' ? N_ADD : N_SUB); n->a = left; n->b = right; left = n;
        } else break;
    }
    return left;
}

/* ----- API publica ----- */

Node *parse_expression(const char *src, char *errmsg) {
    Parser P;
    if (errmsg) errmsg[0] = '\0';
    if (tokenize(&P, src) != 0) {
        if (errmsg) snprintf(errmsg, PARSER_ERRMSG_LEN, "%s", P.msg);
        return NULL;
    }
    Node *root = parse_expr(&P);
    if (!P.error && peek(&P)->kind != T_END) fail(&P, "simbolos sobrando ao final");
    if (P.error) {
        ast_free(root);
        if (errmsg) snprintf(errmsg, PARSER_ERRMSG_LEN, "%s", P.msg);
        return NULL;
    }
    return root;
}

int parse_number_expr(const char *src, double *out, char *errmsg) {
    Node *root = parse_expression(src, errmsg);
    if (!root)
        return 0;
    if (ast_has_var(root)) {
        ast_free(root);
        if (errmsg)
            snprintf(errmsg, PARSER_ERRMSG_LEN, "The value cannot contain an x.");
        return 0;
    }
    double v = value_at(root, 0.0);
    ast_free(root);
    if (!isfinite(v)) {
        if (errmsg)
            snprintf(errmsg, PARSER_ERRMSG_LEN, "Value is not infinite");
        return 0;
    }
    *out = v;
    if (errmsg)
        errmsg[0] = '\0';
    return 1;
}
