#include <math.h>
#include "eval.h"

Dual eval(const Node *n, double x, double seed) {
    switch (n->kind) {
        case N_NUM: return d_const(n->value);
        case N_VAR: return (Dual){ x, seed };
        case N_NEG: { Dual a = eval(n->a, x, seed); return (Dual){ -a.re, -a.du }; }
        case N_ADD: return d_add(eval(n->a, x, seed), eval(n->b, x, seed));
        case N_SUB: return d_sub(eval(n->a, x, seed), eval(n->b, x, seed));
        case N_MUL: return d_mul(eval(n->a, x, seed), eval(n->b, x, seed));
        case N_DIV: return d_div(eval(n->a, x, seed), eval(n->b, x, seed));
        case N_POW: {
            Dual a = eval(n->a, x, seed);
            if (!ast_has_var(n->b)) {                 /* expoente constante */
                double k  = value_at(n->b, x);
                double v  = pow(a.re, k);
                double dv = k * pow(a.re, k - 1.0) * a.du;
                return (Dual){ v, dv };
            }
            Dual b = eval(n->b, x, seed);             /* caso geral u^v */
            double v  = pow(a.re, b.re);
            double dv = v * (b.du * log(a.re) + b.re * a.du / a.re);
            return (Dual){ v, dv };
        }
        case N_FUNC: {
            Dual a = eval(n->a, x, seed);
            double u = a.re, du = a.du;
            switch (n->func) {
                case F_SIN:  return (Dual){ sin(u),  cos(u) * du };
                case F_COS:  return (Dual){ cos(u), -sin(u) * du };
                case F_TAN:  { double c = cos(u); return (Dual){ tan(u), du / (c*c) }; }
                case F_COT:  { double s = sin(u); return (Dual){ cos(u)/sin(u), -du / (s*s) }; }
                case F_SEC:  { double c = cos(u); return (Dual){ 1.0/c, sin(u)/(c*c) * du }; }
                case F_CSC:  { double s = sin(u); return (Dual){ 1.0/s, -cos(u)/(s*s) * du }; }
                case F_ASIN: return (Dual){ asin(u), du / sqrt(1.0 - u*u) };
                case F_ACOS: return (Dual){ acos(u), -du / sqrt(1.0 - u*u) };
                case F_ATAN: return (Dual){ atan(u), du / (1.0 + u*u) };
                case F_SINH: return (Dual){ sinh(u), cosh(u) * du };
                case F_COSH: return (Dual){ cosh(u), sinh(u) * du };
                case F_TANH: { double t = tanh(u); return (Dual){ t, (1.0 - t*t) * du }; }
                case F_EXP:  { double v = exp(u);  return (Dual){ v, v * du }; }
                case F_LN:   return (Dual){ log(u), du / u };
                case F_LOG10:return (Dual){ log10(u), du / (u * log(10.0)) };
                case F_LOG2: return (Dual){ log2(u),  du / (u * log(2.0)) };
                case F_SQRT: { double v = sqrt(u); return (Dual){ v, du / (2.0*v) }; }
                case F_CBRT: { double v = cbrt(u); return (Dual){ v, du / (3.0*v*v) }; }
                case F_ABS:  return (Dual){ fabs(u), (u >= 0 ? 1.0 : -1.0) * du };
            }
        }
    }
    return d_const(NAN);   /* inalcancavel */
}

double value_at(const Node *n, double x) { return eval(n, x, 0.0).re; }
