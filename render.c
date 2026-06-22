#include <math.h>
#include <stdio.h>
#include "raylib.h"
#include "render.h"
#include "eval.h"
#include "calc.h"
#include "dual.h"

/* ----- paleta (interna) ----- */
#define COL_BG        (Color){  18,  18,  22, 255 }
#define COL_GRID_MIN  (Color){  38,  40,  48, 255 }
#define COL_GRID_MAJ  (Color){  58,  61,  72, 255 }
#define COL_AXIS      (Color){ 150, 155, 170, 255 }
#define COL_TEXT      (Color){ 214, 217, 226, 255 }
#define COL_DIM       (Color){ 130, 134, 148, 255 }
#define COL_F         (Color){  74, 162, 255, 255 }
#define COL_D         (Color){ 255, 168,  64, 255 }
#define COL_TAN       (Color){  86, 222, 142, 255 }
#define COL_LIM       (Color){ 236, 110, 152, 255 }
#define COL_INT       (Color){ 170, 134, 255, 255 }
#define COL_FILL_POS  (Color){ 120, 180, 255,  64 }
#define COL_FILL_NEG  (Color){ 255, 120, 120,  64 }
#define COL_PANEL     (Color){  26,  27,  34, 235 }

/* ----- viewport (estado interno do modulo) ----- */
static int   g_W, g_H;
static float g_plotY0, g_plotH;

void render_set_viewport(int w, int h, float plot_y0, float plot_h) {
    g_W = w; g_H = h; g_plotY0 = plot_y0; g_plotH = plot_h;
}

/* ----- transformacoes ----- */
float  to_sx(View v, double x) { return (float)((x - v.xmin)/(v.xmax - v.xmin) * g_W); }
float  to_sy(View v, double y) { return g_plotY0 + (float)((1.0 - (y - v.ymin)/(v.ymax - v.ymin)) * g_plotH); }
double to_wx(View v, float px) { return v.xmin + (double)px/g_W*(v.xmax - v.xmin); }
double to_wy(View v, float py) { return v.ymin + (1.0 - (double)(py - g_plotY0)/g_plotH)*(v.ymax - v.ymin); }

/* ----- auxiliares internos ----- */
static double nice_step(double range, double target_divs) {
    double raw = range / target_divs;
    double mag = pow(10.0, floor(log10(raw)));
    double norm = raw / mag;
    double step = (norm < 1.5) ? 1 : (norm < 3) ? 2 : (norm < 7) ? 5 : 10;
    return step * mag;
}

static void dashed_v(float x, float y0, float y1, float dash, float gap, Color c) {
    if (y1 < y0) { float t = y0; y0 = y1; y1 = t; }
    for (float y = y0; y < y1; y += dash + gap) {
        float ye = fminf(y + dash, y1);
        DrawLineEx((Vector2){x, y}, (Vector2){x, ye}, 1.0f, c);
    }
}

/* ----- cena ----- */
void draw_grid_axes(View v) {
    double sx = nice_step(v.xmax - v.xmin, 12.0);
    double sy = nice_step(v.ymax - v.ymin, 8.0);
    for (double gx = ceil(v.xmin/sx)*sx; gx <= v.xmax; gx += sx) {
        float px = to_sx(v, gx);
        int major = (fabs(gx) < sx*0.5);
        DrawLineEx((Vector2){px, g_plotY0}, (Vector2){px, g_plotY0+g_plotH}, 1.0f,
                   major ? COL_AXIS : COL_GRID_MIN);
        if (!major)
            DrawText(TextFormat("%.4g", gx), (int)px+3, (int)(g_plotY0+g_plotH)-16, 10, COL_DIM);
    }
    for (double gy = ceil(v.ymin/sy)*sy; gy <= v.ymax; gy += sy) {
        float py = to_sy(v, gy);
        int major = (fabs(gy) < sy*0.5);
        DrawLineEx((Vector2){0, py}, (Vector2){(float)g_W, py}, 1.0f,
                   major ? COL_AXIS : COL_GRID_MIN);
        if (!major)
            DrawText(TextFormat("%.4g", gy), 4, (int)py+2, 10, COL_DIM);
    }
}

void plot_curve(const Node *f, View v, double seed, int use_dual, Color col) {
    int has_prev = 0; float psx = 0, psy = 0;
    const float BREAK = g_plotH * 2.5f;
    for (int px = 0; px <= g_W; px++) {
        double x = to_wx(v, (float)px);
        Dual r = eval(f, x, seed);
        double y = use_dual ? r.du : r.re;
        if (!isfinite(y)) { has_prev = 0; continue; }
        float sx = (float)px, sy = to_sy(v, y);
        if (has_prev && fabsf(sy - psy) < BREAK)
            DrawLineEx((Vector2){psx, psy}, (Vector2){sx, sy}, 2.0f, col);
        psx = sx; psy = sy; has_prev = 1;
    }
}

void fill_integral(const Node *f, View v, double lo, double hi) {
    float y0s = to_sy(v, 0.0);
    float pmin = fmaxf(to_sx(v, lo), 0.0f);
    float pmax = fminf(to_sx(v, hi), (float)g_W);
    for (int px = (int)floorf(pmin); px <= (int)ceilf(pmax); px++) {
        double x = to_wx(v, (float)px);
        if (x < lo || x > hi) continue;
        double y = value_at(f, x);
        if (!isfinite(y)) continue;
        float cy = to_sy(v, y);
        float top = fminf(cy, y0s), bot = fmaxf(cy, y0s);
        if (top < g_plotY0)            top = g_plotY0;
        if (bot > g_plotY0 + g_plotH)  bot = g_plotY0 + g_plotH;
        DrawLine(px, (int)top, px, (int)bot, (y >= 0) ? COL_FILL_POS : COL_FILL_NEG);
    }
}

void draw_tangent(const Node *f, View v, double x0) {
    Dual r = eval(f, x0, 1.0);
    if (!isfinite(r.re) || !isfinite(r.du)) return;
    double yL = r.re + r.du*(v.xmin - x0);
    double yR = r.re + r.du*(v.xmax - x0);
    DrawLineEx((Vector2){ to_sx(v, v.xmin), to_sy(v, yL) },
               (Vector2){ to_sx(v, v.xmax), to_sy(v, yR) }, 1.5f, COL_TAN);
    DrawCircleV((Vector2){ to_sx(v, x0), to_sy(v, r.re) }, 5, COL_TAN);
}

void draw_limit_marks(const Node *f, View v) {
    const double x0 = 0.0;
    float px = to_sx(v, x0);
    dashed_v(px, g_plotY0, g_plotY0 + g_plotH, 6, 5, COL_LIM);
    double Lr, Ll;
    if (side_limit(f, x0, +1, &Lr) && isfinite(Lr))
        DrawCircleLines((int)px, (int)to_sy(v, Lr), 6, COL_LIM);
    if (side_limit(f, x0, -1, &Ll) && isfinite(Ll))
        DrawCircleLines((int)px, (int)to_sy(v, Ll), 6, COL_LIM);
}

void draw_integral_bounds(View v, double a, double b) {
    float pa = to_sx(v, a), pb = to_sx(v, b);
    DrawLineEx((Vector2){pa, g_plotY0}, (Vector2){pa, g_plotY0+g_plotH}, 1.5f, COL_INT);
    DrawLineEx((Vector2){pb, g_plotY0}, (Vector2){pb, g_plotY0+g_plotH}, 1.5f, COL_INT);
    DrawText("a", (int)pa+3, (int)g_plotY0+4, 14, COL_INT);
    DrawText("b", (int)pb+3, (int)g_plotY0+4, 14, COL_INT);
}

void draw_panel(const Node *f, double x0, int show_integral, double a, double b) {
    Dual r = eval(f, x0, 1.0);  // valor e derivada no ponto movel x0
    double Lr, Ll;
    int okr = side_limit(f, 0.0, +1, &Lr); // limite sempre tende a origem
    int okl = side_limit(f, 0.0, -1, &Ll);

    int pw = 320, ph = show_integral ? 196 : 150, m = 10;
    Rectangle box = { (float)m, (float)(g_H - ph - m), (float)pw, (float)ph };
    DrawRectangleRec(box, COL_PANEL);
    DrawRectangleLinesEx(box, 1.0f, COL_GRID_MAJ);

    int tx = (int)box.x + 12, ty = (int)box.y + 10, lh = 18;
    DrawText(TextFormat("x0 = %.6g", x0), tx, ty, 14, COL_TEXT); ty += lh;
    DrawText(TextFormat("f(x0)  = %.6g", r.re), tx, ty, 14, COL_F); ty += lh;
    DrawText(TextFormat("f'(x0) = %.6g", r.du), tx, ty, 14, COL_D); ty += lh;

    char lim[96];
    if (okr && okl && fabs(Lr - Ll) <= 1e-6*(1.0+fabs(Lr)))
        snprintf(lim, sizeof lim, "lim x->0 = %.6g", 0.5*(Lr+Ll));
    else if (okr || okl)
        snprintf(lim, sizeof lim, "lim 0+ %.4g | lim 0- %.4g", okr?Lr:NAN, okl?Ll:NAN);
    else
        snprintf(lim, sizeof lim, "lim x->0 : indeterminate");
    DrawText(lim, tx, ty, 14, COL_LIM); ty += lh;

    if (show_integral) {
        double lo = fmin(a, b), hi = fmax(a, b);
        double I = integrate(f, lo, hi);
        DrawText(TextFormat("integral [%.4g, %.4g]", lo, hi), tx, ty, 14, COL_INT); ty += lh;
        if (isfinite(I)) DrawText(TextFormat("  = %.6g", I), tx, ty, 14, COL_INT);
        else             DrawText("  = indefinido (singularidade)", tx, ty, 14, COL_INT);
        ty += lh;
    }
    ty += 4;
    DrawText("arrows/X/field: x0 A/B: limits integral", tx, ty, 11, COL_DIM); ty += 14;
    DrawText("F/D/T/L/I: f f' tangente limite integral", tx, ty, 11, COL_DIM);
}

void draw_legend(int showF, int showD, int showT, int showL, int showI) {
    int x = g_W - 196, y = (int)g_plotY0 + 10;
    Rectangle box = { (float)x - 8, (float)y - 6, 196, 116 };
    DrawRectangleRec(box, COL_PANEL);
    DrawRectangleLinesEx(box, 1.0f, COL_GRID_MAJ);
    DrawText(TextFormat("[F] f(x)        %s", showF?"on":"off"),  x, y,    13, showF?COL_F:COL_DIM);
    DrawText(TextFormat("[D] f'(x)       %s", showD?"on":"off"),  x, y+20, 13, showD?COL_D:COL_DIM);
    DrawText(TextFormat("[T] tangente    %s", showT?"on":"off"),  x, y+40, 13, showT?COL_TAN:COL_DIM);
    DrawText(TextFormat("[L] limite      %s", showL?"on":"off"),  x, y+60, 13, showL?COL_LIM:COL_DIM);
    DrawText(TextFormat("[I] integral    %s", showI?"on":"off"),  x, y+80, 13, showI?COL_INT:COL_DIM);
}
