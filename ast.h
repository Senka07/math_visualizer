#ifndef AST_H
#define AST_H

typedef enum {
  N_NUM,
  N_VAR,
  N_NEG,
  N_ADD,
  N_SUB,
  N_MUL,
  N_DIV,
  N_POW,
  N_FUNC
} NodeKind;

typedef enum {
  F_SIN,
  F_COS,
  F_TAN,
  F_COT,
  F_SEC,
  F_CSC,
  F_ASIN,
  F_ACOS,
  F_ATAN,
  F_SINH,
  F_COSH,
  F_TANH,
  F_EXP,
  F_LN,
  F_LOG10,
  F_LOG2,
  F_SQRT,
  F_CBRT,
  F_ABS
} FuncId;

typedef struct Node {
    NodeKind kind;
    double value;
    FuncId func;
    struct Node *a, *b;
} Node;

Node *ast_new(NodeKind k);

void ast_free(Node *n);

int ast_has_var(const Node *n);

#endif
