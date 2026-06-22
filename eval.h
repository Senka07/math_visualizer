#ifndef EVAL_H
#define EVAL_H

#include "ast.h"
#include "dual.h"

Dual eval(const Node *n, double x, double seed);

double value_at(const Node *n, double x);

#endif // EVAL_H
