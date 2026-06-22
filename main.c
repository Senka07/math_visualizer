/* main.c — Estado da aplicacao e laco principal.
 * Conhece todos os modulos; nenhum modulo o conhece. */
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "raylib.h"
#include "ast.h"
#include "parser.h"
#include "eval.h"
#include "render.h"

#define TOPBAR_H 46.0f
#define MAX_EXPR 200

int main(void) {
    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_MSAA_4X_HINT);
    InitWindow(1100, 720, "Visualizador de funcoes, derivadas e limites");
    SetTargetFPS(60);

    char expr[MAX_EXPR + 1] = "sin(x)/x";
    char errmsg[PARSER_ERRMSG_LEN] = "";

    /* Foco do teclado: qual campo de texto, se algum, esta' em edicao. */
    enum { FOCUS_NONE, FOCUS_EXPR, FOCUS_X0 } focus = FOCUS_NONE;

    Node *f = parse_expression(expr, errmsg);

    View v = { -6.5, 6.5, -3.5, 3.5 };
    double x0 = 1.0;
    char   x0buf[32] = "1";              /* texto editavel do campo x0   */
    char   x0err[PARSER_ERRMSG_LEN] = "";
    double a = -1.0, b = 2.0;
    int showF = 1, showD = 1, showT = 1, showL = 0, showI = 0, showGrid = 1;

    while (!WindowShouldClose()) {
        int W = GetScreenWidth(), H = GetScreenHeight();
        render_set_viewport(W, H, TOPBAR_H, (float)H - TOPBAR_H);

        /* Barra superior: campo f(x) a' esquerda, campo x0 a' direita. */
        float x0w = 150.0f;
        Rectangle exprRect = { 8, 8, (float)W - 16 - x0w - 8, 30 };
        Rectangle x0Rect   = { exprRect.x + exprRect.width + 8, 8, x0w, 30 };
        Vector2 mouse = GetMousePosition();

        /* ---- selecao de foco por clique ---- */
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            if      (CheckCollisionPointRec(mouse, exprRect)) focus = FOCUS_EXPR;
            else if (CheckCollisionPointRec(mouse, x0Rect))   focus = FOCUS_X0;
            else                                              focus = FOCUS_NONE;
        }

        /* ---- edicao do campo f(x) ---- */
        if (focus == FOCUS_EXPR) {
            int ch = GetCharPressed();
            while (ch > 0) {
                int len = (int)strlen(expr);
                if (ch >= 32 && ch < 127 && len < MAX_EXPR) { expr[len] = (char)ch; expr[len+1] = '\0'; }
                ch = GetCharPressed();
            }
            if (IsKeyPressed(KEY_BACKSPACE) || IsKeyPressedRepeat(KEY_BACKSPACE)) {
                int len = (int)strlen(expr);
                if (len > 0) expr[len-1] = '\0';
            }
            if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER)) {
                char newerr[PARSER_ERRMSG_LEN];
                Node *nf = parse_expression(expr, newerr);
                if (nf) { ast_free(f); f = nf; errmsg[0] = '\0'; }
                else    snprintf(errmsg, sizeof errmsg, "%s", newerr);
                focus = FOCUS_NONE;
            }
            if (IsKeyPressed(KEY_ESCAPE)) focus = FOCUS_NONE;
        }

        /* ---- edicao do campo x0 ---- */
        if (focus == FOCUS_X0) {
            int ch = GetCharPressed();
            while (ch > 0) {
                int len = (int)strlen(x0buf);
                if (ch >= 32 && ch < 127 && len < (int)sizeof x0buf - 1) { x0buf[len] = (char)ch; x0buf[len+1] = '\0'; }
                ch = GetCharPressed();
            }
            if (IsKeyPressed(KEY_BACKSPACE) || IsKeyPressedRepeat(KEY_BACKSPACE)) {
                int len = (int)strlen(x0buf);
                if (len > 0) x0buf[len-1] = '\0';
            }
            if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER)) {
                double val;
                if (parse_number_expr(x0buf, &val, x0err)) { x0 = val; x0err[0] = '\0'; }
                focus = FOCUS_NONE;
            }
            if (IsKeyPressed(KEY_ESCAPE)) focus = FOCUS_NONE;
        }


        /* ---- atalhos (apenas quando nenhum campo tem foco) ---- */
        int over_plot = mouse.y > TOPBAR_H;
        if (focus == FOCUS_NONE) {
            if (IsKeyPressed(KEY_F)) showF = !showF;
            if (IsKeyPressed(KEY_D)) showD = !showD;
            if (IsKeyPressed(KEY_T)) showT = !showT;
            if (IsKeyPressed(KEY_L)) showL = !showL;
            if (IsKeyPressed(KEY_I)) showI = !showI;
            if (IsKeyPressed(KEY_G)) showGrid = !showGrid;
            if (IsKeyPressed(KEY_R)) { v = (View){ -6.5, 6.5, -3.5, 3.5 }; }

            double x0_prev = x0;
            double step = (v.xmax - v.xmin) * 0.02;
            if (IsKeyDown(KEY_RIGHT)) x0 += step;
            if (IsKeyDown(KEY_LEFT))  x0 -= step;
            if (IsKeyPressed(KEY_X) && over_plot) x0 = to_wx(v, mouse.x);
            /* manter o texto do campo x0 coerente com mudancas via plano */
            if (x0 != x0_prev) snprintf(x0buf, sizeof x0buf, "%.4g", x0);

            if (IsKeyPressed(KEY_A) && over_plot) a  = to_wx(v, mouse.x);
            if (IsKeyPressed(KEY_B) && over_plot) b  = to_wx(v, mouse.x);

            if (IsMouseButtonDown(MOUSE_BUTTON_LEFT) && over_plot) {
                Vector2 d = GetMouseDelta();
                double dx = (double)d.x / W * (v.xmax - v.xmin);
                double dy = (double)d.y / ((double)H - TOPBAR_H) * (v.ymax - v.ymin);
                v.xmin -= dx; v.xmax -= dx;
                v.ymin += dy; v.ymax += dy;
            }
            float wheel = GetMouseWheelMove();
            if (wheel != 0 && over_plot) {
                double k = pow(0.85, wheel);
                double mx = to_wx(v, mouse.x), my = to_wy(v, mouse.y);
                v.xmin = mx + (v.xmin - mx)*k; v.xmax = mx + (v.xmax - mx)*k;
                v.ymin = my + (v.ymin - my)*k; v.ymax = my + (v.ymax - my)*k;
            }
        }

        /* ---- desenho ---- */
        BeginDrawing();
        ClearBackground((Color){ 18, 18, 22, 255 });

        if (showGrid) draw_grid_axes(v);
        if (f) {
            if (showI) fill_integral(f, v, fmin(a, b), fmax(a, b));
            if (showF) plot_curve(f, v, 0.0, 0, (Color){  74, 162, 255, 255 });
            if (showD) plot_curve(f, v, 1.0, 1, (Color){ 255, 168,  64, 255 });
            if (showT) draw_tangent(f, v, x0);
            if (showL) draw_limit_marks(f, v);
            if (showI) draw_integral_bounds(v, a, b);
            draw_panel(f, x0, showI, a, b);
        }
        draw_legend(showF, showD, showT, showL, showI);

        if (over_plot)
            DrawText(TextFormat("(%.3g, %.3g)", to_wx(v, mouse.x), to_wy(v, mouse.y)),
                     (int)mouse.x + 12, (int)mouse.y - 4, 12, (Color){130,134,148,255});

        /* barra superior: dois campos */
        DrawRectangle(0, 0, W, (int)TOPBAR_H, (Color){ 26, 27, 34, 235 });

        /* campo f(x) */
        int ef = (focus == FOCUS_EXPR);
        DrawRectangleRec(exprRect, (Color){ 34, 36, 44, 255 });
        DrawRectangleLinesEx(exprRect, ef ? 2.0f : 1.0f,
                             ef ? (Color){74,162,255,255} : (Color){58,61,72,255});
        DrawText("f(x) =", (int)exprRect.x + 8, (int)exprRect.y + 8, 16, (Color){130,134,148,255});
        int prefixF = MeasureText("f(x) =", 16) + 16;
        DrawText(expr, (int)exprRect.x + prefixF, (int)exprRect.y + 8, 16, (Color){214,217,226,255});
        if (ef) {
            int cw = MeasureText(expr, 16);
            DrawText("_", (int)exprRect.x + prefixF + cw + 1, (int)exprRect.y + 8, 16, (Color){74,162,255,255});
        }

        /* campo x0 */
        int ex = (focus == FOCUS_X0);
        DrawRectangleRec(x0Rect, (Color){ 34, 36, 44, 255 });
        DrawRectangleLinesEx(x0Rect, ex ? 2.0f : 1.0f,
                             ex ? (Color){74,162,255,255} : (Color){58,61,72,255});
        DrawText("x0 =", (int)x0Rect.x + 8, (int)x0Rect.y + 8, 16, (Color){130,134,148,255});
        int prefixX = MeasureText("x0 =", 16) + 14;
        DrawText(x0buf, (int)x0Rect.x + prefixX, (int)x0Rect.y + 8, 16, (Color){214,217,226,255});
        if (ex) {
            int cw = MeasureText(x0buf, 16);
            DrawText("_", (int)x0Rect.x + prefixX + cw + 1, (int)x0Rect.y + 8, 16, (Color){74,162,255,255});
        }

        /* mensagens de erro (abaixo da barra) */
        if (errmsg[0])
            DrawText(TextFormat("f(x): %s", errmsg),
                     (int)exprRect.x + 8, (int)exprRect.y + 34, 12, (Color){236,110,152,255});
        if (x0err[0])
            DrawText(TextFormat("x0: %s", x0err),
                     (int)x0Rect.x + 8, (int)x0Rect.y + 34, 12, (Color){236,110,152,255});

        EndDrawing();
    }

    ast_free(f);
    CloseWindow();
    return 0;
}
