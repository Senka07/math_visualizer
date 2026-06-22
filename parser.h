#ifndef PARSER_H
#define PARSER_H

#include "ast.h"

#define PARSER_ERRMSG_LEN 128

Node *parse_expression(const char *src, char *errmsg);

int parse_number_expr(const char *src, double *out, char *errmsg);
// Analisa uma expressao numerica constante (nao precisa inserir a variavel
// x), aceita literais, pi, e, e operacoes entre eles (ex: -2, pi/2, pi*2
// etc)
#endif // PARSER_H
