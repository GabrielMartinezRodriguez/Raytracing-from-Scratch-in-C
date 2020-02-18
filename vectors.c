#include "header.h"

t_vect3 newVector(double x, double y, double z)
{
    t_vect3 vector;

    vector.x = x;
    vector.y = y;
    vector.z = z;
    return (vector);
}
t_vect3 crossProduct(t_vect3 first, t_vect3 second)
{
    t_vect3 result;

    result.x = first.y * second.z - first.z * second.y;
    result.y = first.z * second.x - first.x * second.z;
    result.z = first.x * second.y - first.y * second.x;
    return (result);
}

t_vect3 changeLenght(t_vect3 vector, double module)
{
    double moduleOriginal;
    double factor;

    moduleOriginal = sqrt(pow(vector.x, 2) + pow(vector.y, 2) + pow(vector.z, 2));
    factor = moduleOriginal / module;
    vector.x /= factor;
    vector.y /= factor;
    vector.z /= factor;
    return(vector);
}

t_vect3 mulVector(t_vect3 vector, double number)
{
    vector.x *= number;
    vector.y *= number;
    vector.z *= number;
    return (vector);
}

t_vect3 perpendicular_rand_vect(t_vect3 vector)
{
    t_tupla equation;
    t_tupla solution;
    t_vect3 new_vector;

    equation.tamaño=2;
    new_vector.x = 1;
    new_vector.y = 1;
    new_vector.z = 1;
    if (vector.x != 0)
    {
        equation.tupla[1] = vector.y + vector.z;
        equation.tupla[0] = vector.x;
        solution = solveEquation(equation, 0);
        new_vector.x = solution.tupla[0];
        
    }
    else if(vector.y != 0)
    {
        equation.tupla[1] = vector.x + vector.z;
        equation.tupla[0] = vector.y;
        solution = solveEquation(equation, 0);
        new_vector.y = solution.tupla[0];
    }
    else
    {
        equation.tupla[1] = vector.x + vector.y;
        equation.tupla[0] = vector.z;
        solution = solveEquation(equation, 0);
        new_vector.z = solution.tupla[0];
    }
    return (new_vector);
}

double moduleVector(t_vect3 vector)
{
    double module;

    module = 0;
    module += pow(vector.x, 2) + pow(vector.y, 2) + pow(vector.z, 2);
    module = sqrt(module);
    return (module);

}

t_vect3 pointRay(t_rayo ray, double lamda)
{
    t_vect3 point;

    point.x = ray.punto.x + lamda * ray.vector.x;
    point.y = ray.punto.y + lamda * ray.vector.y;
    point.z = ray.punto.z + lamda * ray.vector.z;
    return (point);
}

t_vect3 distanceVector(t_vect3 point1, t_vect3 point2)
{
    t_vect3 vect;

    vect.x = point1.x - point2.x;
    vect.y = point1.y - point2.y;
    vect.z = point1.z - point2.z;
    return (vect);
}
t_vect3 addVector(t_vect3 vect1, t_vect3 vect2)
{
    t_vect3 vector;

    vector.x = vect1.x + vect2.x;
    vector.y = vect1.y + vect2.y;
    vector.z = vect1.z + vect2.z;
    return (vector);
}
double distancePoint(t_vect3 point1, t_vect3 point2)
{
    double distance;

    distance = 0;
    distance += pow(point1.x - point2.x, 2);
    distance += pow(point1.y - point2.y, 2);
    distance += pow(point1.z - point2.z, 2);
    distance = sqrt(distance);
    return (distance);
}

t_vect3 pointVectorPoint(t_vect3 point, t_vect3 vect, double lambda)
{
    t_vect3 returned;

    returned.x = point.x + lambda * vect.x;
    returned.y = point.y + lambda * vect.y;
    returned.z = point.z + lambda * vect.z;
    return(returned);
}

t_tupla vectorialSpaceSolution(t_vect3 origin, t_vect3 vect1, t_vect3 vect2, t_vect3 point)
{
    t_tupla equation[4];
    t_tupla solution;

    if(vect1.x != 0)
    {
        equation[0] = newTupla((vect2.x / vect1.x), (point.x - origin.x) / vect1.x);
        equation[1] = mulTupla(equation[0], vect1.y);
        equation[2] = newTupla(vect2.y, origin.y);
        equation[3] = addTupla(equation[1], equation[2]);
        solution = solveEquation(equation[3], point.y);
    }
    else if(vect1.y != 0)
    {
        equation[0] = newTupla((vect2.y / vect1.y), (point.y - origin.y) / vect1.y);
        equation[1] = mulTupla(equation[0], vect1.x);
        equation[2] = newTupla(vect2.x, origin.x);
        equation[3] = addTupla(equation[1], equation[2]);
        solution = solveEquation(equation[3], point.x);
    }
    else
    {
        equation[0] = newTupla((vect2.z / vect1.z), (point.z - origin.z) / vect1.z);
        equation[1] = mulTupla(equation[0], vect1.x);
        equation[2] = newTupla(vect2.x, origin.x);
        equation[3] = addTupla(equation[1], equation[2]);
        solveEquation(equation[3], point.y);
    }
    if(solution.tamaño <= 0)
        return (solution);
    solution.tamaño++;
    solution.tupla[1] = solution.tupla[1] = equation[0].tupla[0] * solution.tupla[0] + equation[0].tupla[1];
    return (solution);
}
double angleTwoVectors(t_vect3 vect1, t_vect3 vect2)
{
    double scalarProduct;
    double cos;

    scalarProduct += vect1.x * vect2.x + vect1.y * vect2.y + vect1.z * vect2.z;
    cos = scalarProduct / (moduleVector(vect1) * moduleVector(vect2));
    if(cos < -1 || cos > 1)
        cos = cos;
    return (cos);
}