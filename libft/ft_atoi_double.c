#include "libft.h"

double ft_atoi_double(char *number)
{
    int     i;
    double     after_point;
    double  returned;
    int sign;

    i = 0;
    after_point = 0;
    returned = 0;
    sign = 1;
    if(number[0] == '-')
        sign = -1;
    while(number[i] != '\0')
    {
        if(number[i] == '.')
            after_point = 10;
        else if(number[i] >= '0' && number[i] <= '9')
        {
            if(after_point == 0)
                returned = returned * 10 + number[i] - '0';
            else
            {
                returned += (number[i] - '0') / after_point;
                after_point *= 10;
            }
        }
        i++;
    }
    return (sign * returned);
}
