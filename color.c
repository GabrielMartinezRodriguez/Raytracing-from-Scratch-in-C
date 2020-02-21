#include "header.h"


t_color angleColor(t_color color, t_vect3 normal, t_vect3 rayDir)
{
    double cos;

    cos = angleTwoVectors(normal, rayDir);
    cos = absoluto(cos);
    color.red *= cos;
    color.green *= cos;
    color.blue *= cos;
    return (color);
}

t_color reflectedColor(t_color colorObject, t_color colorLight)
{
    t_color refColor;

    refColor.red = colorLight.red * (colorObject.red / 255);
    refColor.green = colorLight.green * (colorObject.green / 255);
    refColor.blue = colorLight.blue * (colorObject.blue / 255);
    return (refColor);
}
t_color intensityColor(t_color color, double intensity)
{
    t_color color1;

    color1.red = color.red * intensity;
    color1.green = color.green * intensity;
    color1.blue = color.blue * intensity;
    return (color1);
}
unsigned int colorToint(t_color color)
{
    unsigned int color_int;

    color_int = 0;
    color_int += color.red * pow(256, 2);
    color_int += color.green * pow(256, 1);
    color_int += color.blue;
    return (color_int);
}
t_color addColor(t_color color1, t_color color2)
{
    t_color color;

    if(color1.red + color2.red > 255)
        color.red = 255;
    if(color1.green + color2.green > 255)
        color.green = 255;
    if(color1.blue + color2.blue > 255)
        color.blue = 255;
    return (color);
}