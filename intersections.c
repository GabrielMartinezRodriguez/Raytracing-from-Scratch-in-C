#include "header.h"

t_intersection *returnNear(t_intersection *first, t_intersection *second)
{
    if(first == NULL)
        return (second);
    else if(second == NULL)
        return (first);
    if(first->lambda > 0 && second ->lambda > 0)
    {
        if(first->lambda > second->lambda)
            return(second);
        else
            return(first);
    }
    else if(first->lambda > 0)
        return (first);
    else if(second->lambda > 0)
        return (second);
    else
        return (NULL);
}