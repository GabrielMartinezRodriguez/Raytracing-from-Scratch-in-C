#include "header.h"

t_tupla notableProduct(double first, double second)
{
    t_tupla tupla;

    tupla.tamaño = 3;
    tupla.tupla[0] = pow(first, 2);
    tupla.tupla[1] = 2 * first * second;
    tupla.tupla[2] = pow(second, 2);
    return (tupla);
}

t_tupla addTupla(t_tupla first, t_tupla second)
{
    t_tupla tupla;
    int     i;

    i = 0;
    if(first.tamaño > second.tamaño)
        tupla.tamaño = first.tamaño;
    else
        tupla.tamaño = second.tamaño;
    while(i < 3)
    {
        tupla.tupla[i] = first.tupla[i] + second.tupla[i];
        i++;
    }
    return (tupla);
}

t_tupla mulTupla(t_tupla tupla, double number)
{
    t_tupla mul;
    int i;

    mul.tamaño = tupla.tamaño;
    i = 0;
    while (i < tupla.tamaño)
    {
        mul.tupla[i] = tupla.tupla[i] * number;
        i++;
    }
    return (mul);
}

t_tupla newTupla(double first, double second)
{
    t_tupla tupla;

    tupla.tamaño = 2;
    tupla.tupla[0] = first;
    tupla.tupla[1] = second;
    return (tupla);
}

double lowerNumber(t_tupla tupla)
{
    int i;
    double lower;

    lower = tupla.tupla[0];
    i = 1;
    while(i < tupla.tamaño)
    {
        if(tupla.tupla[i] < lower && tupla.tupla[i] >= 0)
            lower = tupla.tupla[i];
        i++;
    }
    return (lower);
}

double absoluto(double abs)
{
    if(abs < 0)
        return -abs;
    return abs;
}