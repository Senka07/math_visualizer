#ifndef CALC_H
#define CALC_H

#include "ast.h"

int side_limit(const Node *f, double x0, int dir, double *out);

double integrate(const Node *f, double a, double b);

#endif // CALC_H
