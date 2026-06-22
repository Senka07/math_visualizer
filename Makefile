# Makefile — visualizador de calculo modularizado
CC      = cc
CFLAGS  = -std=c11 -Wall -Wextra -O2
LDLIBS  = -lraylib -lm

OBJS = ast.o parser.o eval.o calc.o render.o main.o

grafico: $(OBJS)
	$(CC) $(OBJS) -o $@ $(LDLIBS)

# nucleo sem GUI, para testes em maquinas sem raylib
test_core: ast.o parser.o eval.o calc.o test_core.o
	$(CC) $^ -o $@ -lm
	./test_core

# dependencias de cabecalho
ast.o:    ast.c ast.h
parser.o: parser.c parser.h ast.h eval.h dual.h
eval.o:   eval.c eval.h ast.h dual.h
calc.o:   calc.c calc.h ast.h eval.h
render.o: render.c render.h ast.h eval.h calc.h dual.h
main.o:   main.c render.h parser.h eval.h ast.h

%.o: %.c
	$(CC) $(CFLAGS) -c $<

clean:
	rm -f $(OBJS) test_core.o grafico test_core

.PHONY: clean
