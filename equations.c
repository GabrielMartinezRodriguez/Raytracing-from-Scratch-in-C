#include "header.h"

t_tupla solveEquation(t_tupla equation, double equal)
{
    t_tupla solution;

    if (equation.tamaño == 3)
        solution = solveEquation2(equation, equal);
    else if(equation.tamaño == 2)
        solution = solveEquation1(equation, equal);
    return solution;
}

t_tupla solveEquation2(t_tupla equation, double equal)
{
    t_tupla solution;
    double insideRoot;
    double outsideRoot;
    double root;

    equation.tupla[2] -= equal;
    insideRoot = pow(equation.tupla[1], 2) - 4 * equation.tupla[0] * equation.tupla[2];
    root = sqrt(insideRoot);
    outsideRoot = -(equation.tupla[1]);
    if (insideRoot < 0)
        solution.tamaño = 0;
    else if (insideRoot == 0)
    {
        solution.tamaño = 1;
        solution.tupla[0] = (outsideRoot + root) / (2 * equation.tupla[0]);
    }
    else
    {
        solution.tamaño = 2;
        solution.tupla[0] = (outsideRoot + root) / (2 * equation.tupla[0]);
        solution.tupla[1] = (outsideRoot - root) / (2 * equation.tupla[0]);
    }
    return (solution);
}

t_tupla solveEquation1(t_tupla equation, double equal)
{
    t_tupla solution;

    if (equation.tupla[0] == 0)
        solution.tamaño = 0;
    else
    {
        solution.tamaño = 1;
        solution.tupla[0] = (equal - equation.tupla[1]) / equation.tupla[0];
    }
    return (solution);
}
