#include <stdio.h>
#include <stdlib.h>
#include "ast.h"

Node *ast_new(NodeKind k) {
    Node *n = calloc(1, sizeof *n);
    if (!n) {
        fprintf(stderr, "memory depleted");
        exit(EXIT_FAILURE);
    }
    n->kind = k;
    return n;
}

void ast_free(Node *n) {
    if (!n)
        return;
    ast_free(n->a);
    ast_free(n->b);
    free(n);
}

int ast_has_var(const Node *n) {
    if (!n)
        return 0;
    if (n->kind == N_VAR)
        return 1;
    return ast_has_var(n->a) || ast_has_var(n->b);
}
