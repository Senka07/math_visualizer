#ifndef DUAL_H
#define DUAL_H

typedef struct {
    double re;
    double du;

} Dual;

static inline Dual d_const(double v) { return (Dual){v, 0.0}; }
static inline Dual d_add(Dual a, Dual b) {
    return (Dual){a.re + b.re, a.du + b.du};
}
static inline Dual d_sub(Dual a, Dual b) {
    return(Dual){a.re - b.re, a.du - b.du};
}
static inline Dual d_mul(Dual a, Dual b) {
    return (Dual){a.re * b.re, a.re * b.du + a.du * b.re};
}
static inline Dual d_div(Dual a, Dual b) {
    return (Dual){a.re / b.re, (a.du * b.re - a.re * b.du) / (b.re * b.re)};
}
#endif // DUAL_H




