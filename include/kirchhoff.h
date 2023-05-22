#ifndef KIRCHOFF_H
#define KIRCHOFF_H

#include <histg_lib.h>

typedef struct IMatrix
{
    long long int *data;
    int rows;
    int columns;
} IMatrix;

IMatrix igraph_laplacian(Graph *graph);

void iprint_matrix_to_output(Output *output, IMatrix *matrix);

void iset_element(IMatrix *matrix, int row, int column, long long int value);
long long int iget_element(IMatrix *matrix, int row, int column);

IMatrix icreate_submatrix(IMatrix *matrix, int del_row, int del_column);

long long int determinant(IMatrix *matrix);

void bareiss(IMatrix *matrix);

long long int kirchhoff(Graph *graph);

#endif
