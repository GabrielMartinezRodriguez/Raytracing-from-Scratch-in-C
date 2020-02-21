#include "header.h"

t_intersection *returnNear(t_intersection *first, t_intersection *second)
{
    if(first == NULL)
    {
        if(second != NULL && second->lambda > 0)
            return second;
        else
            return NULL;
    }
    else if(second == NULL)
    {
        if(first->lambda > 0)
            return first;
        else
            return NULL;
    }
    if(first->lambda > 0 && second ->lambda > 0)
    {
        if(first->lambda > second->lambda)
        {
            free(first);
            return(second);
        }
        else
        {
            free(second);
            return(first);
        }
    }
    else if(first->lambda > 0)
    {
        free(second);
        return (first);
    }
    else if(second->lambda > 0)
    {
        free(first);
        return (second);
    }
    else
        return (NULL);
}