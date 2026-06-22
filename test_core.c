/* test_core.c — testes do nucleo analitico, sem GUI. */
#include <stdio.h>
#include <math.h>
#include "parser.h"
#include "eval.h"
#include "calc.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

static int fails = 0;
static void chk(const char *name, double got, double want, double tol) {
    int ok = fabs(got - want) <= tol * (1.0 + fabs(want));
    printf("%-34s got=%-14.8g want=%-12.8g %s\n", name, got, want, ok ? "ok" : "FALHOU");
    if (!ok) fails++;
}

int main(void) {
    char err[PARSER_ERRMSG_LEN];
    Node *f;
    double L;

    f = parse_expression("x^2 + 3x - 1", err);
    chk("polinomio: f(3)",  eval(f, 3, 1).re, 17.0, 1e-12);
    chk("polinomio: f'(3)", eval(f, 3, 1).du,  9.0, 1e-12);
    ast_free(f);

    f = parse_expression("sin(x)/x", err);
    side_limit(f, 0, +1, &L);
    chk("limite fundamental (dir)", L, 1.0, 1e-8);
    chk("derivada sin(x)/x em 1", eval(f, 1, 1).du, (cos(1)*1 - sin(1))/1.0, 1e-12);
    ast_free(f);

    f = parse_expression("exp(-x^2)", err);
    chk("precedencia -x^2: f(2)", value_at(f, 2), exp(-4.0), 1e-12);
    ast_free(f);

    f = parse_expression("sin(x)", err);
    chk("Simpson int_0^pi sin", integrate(f, 0, M_PI), 2.0, 1e-9);
    ast_free(f);

    f = parse_expression("x^3", err);
    chk("Simpson exato grau 3", integrate(f, -1, 2), 3.75, 1e-12);
    ast_free(f);

    f = parse_expression("2++", err);
    printf("%-34s %s\n", "erro sintatico detectado",
           (!f && err[0]) ? "ok" : "FALHOU");
    if (f || !err[0]) fails++;

    /* parse_number_expr: campo x0 */
    double nv;
    printf("%-34s %s\n", "num: pi/2",
           (parse_number_expr("pi/2", &nv, err) && fabs(nv - M_PI/2) < 1e-12) ? "ok" : "FALHOU");
    printf("%-34s %s\n", "num: -3",
           (parse_number_expr("-3", &nv, err) && nv == -3.0) ? "ok" : "FALHOU");
    printf("%-34s %s\n", "num rejeita x",
           (!parse_number_expr("2x", &nv, err) && err[0]) ? "ok" : "FALHOU");
    if (!(parse_number_expr("pi/2", &nv, err) && fabs(nv-M_PI/2)<1e-12)) fails++;
    if (!(parse_number_expr("-3", &nv, err) && nv==-3.0)) fails++;
    if (parse_number_expr("2x", &nv, err)) fails++;

    printf("\n%s (%d falha(s))\n", fails ? "REPROVADO" : "APROVADO", fails);
    return fails ? 1 : 0;
}
