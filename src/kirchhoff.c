#include <stdlib.h>
#include <math.h>

#include <histg_lib.h>
#include <kirchhoff.h>

/*
 * Integer Matrix
 */
IMatrix igraph_laplacian(Graph *graph)
{
    IMatrix matrix;
    matrix.rows = graph->vertices;
    matrix.columns = graph->vertices;
    matrix.data = calloc(matrix.rows * matrix.columns, sizeof(long long int));

    for (int row = 0; row < graph->vertices; row++)
    {
        uint64_t adjacencies = graph->adjacency_matrix[row];
        long long int degree = vertex_degree(adjacencies);

        for (int col = 0; col < graph->vertices; col++)
        {
            if (row == col)
            {
                iset_element(&matrix, row, col, degree);
            }
            else if (adjacencies & (FIRST_BIT >> col))
            {
                iset_element(&matrix, row, col, -1);
            }
        }
    }

    return matrix;
}

void iprint_matrix_to_output(Output *output, IMatrix *matrix)
{
    for (int row = 0; row < matrix->rows; row++)
    {
        for (int col = 0; col < matrix->columns; col++)
        {
            long long int val = iget_element(matrix, row, col);
            char *format = val < 0 ? "%lli\t" : " %lli\t";
            fprintf(output->output_file, format, val);
        }
        fprintf(output->output_file, "\n");
    }
}

void iset_element(IMatrix *matrix, int row, int column, long long int value)
{
    int index = row * matrix->columns + column;
    matrix->data[index] = value;
}

long long int iget_element(IMatrix *matrix, int row, int column)
{
    int index = row * matrix->columns + column;
    return matrix->data[index];
}

IMatrix icreate_submatrix(IMatrix *matrix, int del_row, int del_column)
{
    IMatrix submatrix;
    submatrix.rows = matrix->rows - 1;
    submatrix.columns = matrix->columns - 1;
    submatrix.data = calloc(submatrix.rows * submatrix.columns, sizeof(long long int));

    int srow = 0;

    for (int row = 0; row < matrix->rows; row++)
    {
        if (row == del_row)
            continue;

        int scol = 0;

        for (int col = 0; col < matrix->columns; col++)
        {
            if (col == del_column)
                continue;

            long long int val = iget_element(matrix, row, col);
            iset_element(&submatrix, srow, scol, val);

            scol++;
        }

        srow++;
    }

    return submatrix;
}

long long int determinant_main(IMatrix *matrix)
{
    if (matrix->rows == 2)
    {
        long long int a = iget_element(matrix, 0, 0);
        long long int b = iget_element(matrix, 0, 1);
        long long int c = iget_element(matrix, 1, 0);
        long long int d = iget_element(matrix, 1, 1);
        return a * d - b * c;
    }

    long long int sign = 1;
    long long int determinant_result = 0;

    int row = 0;

    for (int column = 0; column < matrix->columns; column++)
    {
        long long int value = iget_element(matrix, row, column);

        if (value == 0)
        {
            sign *= -1;
            continue;
        }

        IMatrix submatrix = icreate_submatrix(matrix, row, column);

        determinant_result += sign * value * determinant(&submatrix);

        sign *= -1;
    }

    return determinant_result;
}

long long int determinant(IMatrix *matrix)
{
    if (matrix->rows != matrix->columns)
    {
        fprintf(stderr, "Attempting to calculate determinant for non-square matrix.\n");
        exit(EXIT_FAILURE);
    }

    return determinant_main(matrix);
}

void bareiss(IMatrix *matrix)
{
    if (matrix->rows != matrix->columns)
    {
        fprintf(stderr, "Attempting to execute Bareiss' algorithmn on non-square matrix.\n");
        exit(EXIT_FAILURE);
    }

    int n = matrix->rows;

    for (int k = 0; k < n; k++)
    {
        long long int mkk = iget_element(matrix, k, k);
        long long int mkk1 = k == 0 ? 1 : iget_element(matrix, k - 1, k - 1);

        for (int i = k + 1; i < n; i++)
        {
            long long int mik = iget_element(matrix, i, k);

            for (int j = k + 1; j < n; j++)
            {
                long long int mij = iget_element(matrix, i, j);
                long long int mkj = iget_element(matrix, k, j);

                long long int value = (mij * mkk - mik * mkj) / mkk1;

                iset_element(matrix, i, j, value);
            }
        }
    }
}

long long int kirchhoff(Graph *graph)
{
    IMatrix laplacian = igraph_laplacian(graph);
    IMatrix sublaplacian = icreate_submatrix(&laplacian, 0, 0);
    bareiss(&sublaplacian);
    int i = sublaplacian.rows - 1;
    long long int result = iget_element(&sublaplacian, i, i);
    free(laplacian.data);
    free(sublaplacian.data);
    return result;
}