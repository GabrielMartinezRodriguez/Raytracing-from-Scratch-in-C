#include "header.h"

t_vect3 rotateVector(t_matrix3 matrix, t_vect3 vector)
{
    t_vect3     rotatedVector;

    rotatedVector.x = matrix.vect1.x * vector.x + matrix.vect2.x * vector.y + matrix.vect3.x * vector.z;
    rotatedVector.y = matrix.vect1.y * vector.x + matrix.vect2.y * vector.y + matrix.vect3.y * vector.z;
    rotatedVector.z = matrix.vect1.z * vector.x + matrix.vect2.z * vector.y + matrix.vect3.z * vector.z;
    return (rotatedVector);
}

t_matrix3 inverseMatrix(t_matrix3 matrix)
{
    int i;
    int j;
    t_matrix3 inverse;
    double det;

    det = detMatrix(matrix);
    matrix = attachedMatrix(matrix);
    i = 0;
    j = 0;
    while (i < 3)
    {
        while(j < 3)
        {
            set_index_matrix(&inverse, i , j, get_index_matrix(matrix, i, j) / det);
            j++;
        }
        j = 0;
        i++;
    }
    inverse = transposeMatrix(inverse);
    return (inverse);
}

double detMatrix(t_matrix3 matrix)
{
    double det;

    det = 0;
    det += matrix.vect1.x * matrix.vect2.y * matrix.vect3.z + matrix.vect1.y * matrix.vect2.z * matrix.vect3.x + matrix.vect1.z  * matrix.vect2.x * matrix.vect3.y;
    det -= matrix.vect3.x * matrix.vect2.y * matrix.vect1.z + matrix.vect2.x * matrix.vect1.y * matrix.vect3.z + matrix.vect3.y * matrix.vect2.z * matrix.vect1.x;
    return det;
}

t_matrix3 attachedMatrix(t_matrix3 matrix)
{
    t_matrix3 attached;
    int i;
    int j;

    i = 0;
    j = 0;
    while (i < 3)
    {
        while (j < 3)
        {
            set_index_matrix(&attached, i, j, subMatrixDet(matrix, i, j));
            j++;
        }
        j = 0;
        i++;
    }
    return attached;

}

double subMatrixDet(t_matrix3 matrix, int i, int j)
{
    int iniFila;
    int iniCol;
    int aux;
    double det;
    double **newMatrix;

    newMatrix = t_matrixTomatrix(matrix);
    if (i > 0)
        iniCol = 0;
    else
        iniCol = 1;
    if (j > 0)
        iniFila = 0;
    else
        iniFila = 1;
    if(iniCol + 1 != i && iniFila + 1 != j)
        det = newMatrix[iniFila][iniCol] * newMatrix[iniFila + 1][iniCol + 1] - newMatrix[iniFila + 1][iniCol] * newMatrix[iniFila][iniCol + 1];
    else if(iniCol + 2 != i && iniFila + 1 != j)
        det = newMatrix[iniFila][iniCol] * newMatrix[iniFila + 1][iniCol + 2] - newMatrix[iniFila + 1][iniCol] * newMatrix[iniFila][iniCol + 2];
    else if (iniCol + 1 != i && iniFila + 2 != j)
        det = newMatrix[iniFila][iniCol] * newMatrix[iniFila + 2][iniCol + 1] - newMatrix[iniFila + 2][iniCol] * newMatrix[iniFila][iniCol + 1];
    else if(iniCol + 2 != i && iniFila + 2 != j)
        det = newMatrix[iniFila][iniCol] * newMatrix[iniFila + 2][iniCol + 2] - newMatrix[iniFila + 2][iniCol] * newMatrix[iniFila][iniCol + 2];
    if((i + j)%2 != 0)
        det = -det;
    return det;
}

double **t_matrixTomatrix(t_matrix3 matrix)
{
    double **newMatrix;

    newMatrix = ft_calloc(3, sizeof(double *));
    newMatrix[0] = ft_calloc(3, sizeof(double));
    newMatrix[1] = ft_calloc(3, sizeof(double));
    newMatrix[2] = ft_calloc(3, sizeof(double));

    newMatrix[0][0] = matrix.vect1.x;
    newMatrix[1][0] = matrix.vect1.y;
    newMatrix[2][0] = matrix.vect1.z;
    newMatrix[0][1] = matrix.vect2.x;
    newMatrix[1][1] = matrix.vect2.y;
    newMatrix[2][1] = matrix.vect2.z;
    newMatrix[0][2] = matrix.vect3.x;
    newMatrix[1][2] = matrix.vect3.y;
    newMatrix[2][2] = matrix.vect3.z;
    return newMatrix;
}

void set_index_matrix(t_matrix3 *matrix, int i, int j,double value)
{
    if(i == 0)
    {
        if(j == 0)
            matrix->vect1.x = value;
        else if(j == 1)
            matrix->vect1.y = value;
        else
            matrix->vect1.z = value;
    }
    else if(i == 1)
    {
        if(j == 0)
            matrix->vect2.x = value;
        else if(j == 1)
            matrix->vect2.y = value;
        else
            matrix->vect2.z = value;
    }
    else
    {
        if(j == 0)
            matrix->vect3.x = value;
        else if(j == 1)
            matrix->vect3.y = value;
        else
            matrix->vect3.z = value;
    }

}

double get_index_matrix(t_matrix3 matrix, int i, int j)
{
    double value;
    if(i == 0)
    {
        if(j == 0)
            value = matrix.vect1.x;
        else if(j == 1)
            value = matrix.vect1.y;
        else
            value = matrix.vect1.z;
    }
    else if(i == 1)
    {
        if(j == 0)
            value = matrix.vect2.x;
        else if(j == 1)
            value = matrix.vect2.y;
        else
            value = matrix.vect2.z;
    }
    else
    {
        if(j == 0)
            value = matrix.vect3.x;
        else if(j == 1)
            value = matrix.vect3.y;
        else
            value = matrix.vect3.z;
    }
    return (value);
}

t_matrix3 transposeMatrix(t_matrix3 matrix)
{
    double aux;

    aux = matrix.vect1.y;
    matrix.vect1.y = matrix.vect2.x;
    matrix.vect2.x = aux;
    aux = matrix.vect1.z;
    matrix.vect1.z = matrix.vect3.x;
    matrix.vect3.x = aux;
    aux = matrix.vect2.z;
    matrix.vect2.z = matrix.vect3.y;
    matrix.vect3.y = aux;
    return (matrix);
}

t_matrix3 iniMatrix(t_vect3 first, t_vect3 second, t_vect3 third)
{
    t_matrix3 new;

    new.vect1 = first;
    new.vect2 = second;
    new.vect3 = third;
    return (new);
}