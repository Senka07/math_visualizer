#ifndef RENDER_H
#define RENDER_H

#include "ast.h"

/* Janela do mundo matematico visivel. */
typedef struct { double xmin, xmax, ymin, ymax; } View;

/* Informa as dimensoes correntes (chamar a cada quadro, pois a
 * janela e' redimensionavel). Estado interno do modulo. */
void render_set_viewport(int w, int h, float plot_y0, float plot_h);

/* Transformacoes mundo <-> tela (dependem do viewport corrente). */
float  to_sx(View v, double x);
float  to_sy(View v, double y);
double to_wx(View v, float px);
double to_wy(View v, float py);

/* Elementos da cena. */
void draw_grid_axes(View v);
void plot_curve(const Node *f, View v, double seed, int use_dual, Color col);
void fill_integral(const Node *f, View v, double lo, double hi);
void draw_tangent(const Node *f, View v, double x0);
void draw_limit_marks(const Node *f, View v);
void draw_integral_bounds(View v, double a, double b);
void draw_panel(const Node *f, double x0, int show_integral, double a, double b);
void draw_legend(int showF, int showD, int showT, int showL, int showI);

#endif /* RENDER_H */
