#include <math.h>
#include "calc.h"
#include "eval.h"

int side_limit(const Node *f, double x0, int dir, double *out) {
    double prev = NAN;
    int stable = 0;
    for (int k = 1; k <= 14; k++) {
        double h = dir * pow(10.0, -k);
        double y = value_at(f, x0 + h);
        if (!isfinite(y))
            continue;
        if (isfinite(prev) && fabs(y - prev) <= 1e-9 * (1.0 + fabs(y))){
            if (++stable >= 2) {
                *out = y;
                return 1;
            }
        }
        else stable = 0;
        prev = y;
    }
    if (isfinite(prev)) {
        *out = prev;
        return 1;
    }
    return 0;
}

double integrate(const Node *f, double a, double b) {
    if (a == b)
        return 0.0;
    const int N = 2000;
    double h = (b - a) / N;
    double s = value_at(f, a) + value_at(f, b);
    if (!isfinite(s))
        return NAN;
    for (int i = 1; i < N; i++) {
        double y = value_at(f, a + i*h);
        if (!isfinite(y))
            return NAN;
        s+= (i & 1) ? 4.0*y : 2.0*y;
    }
    return s * h / 3.0;
}


