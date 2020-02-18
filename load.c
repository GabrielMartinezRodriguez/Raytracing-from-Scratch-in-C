#include "header.h"


static void freeSpace(char **words)
{
    int i;

    i = 0;
    while (words[i] != NULL)
    {
        free(words[i]);
        i++;
    }
    free(words);
}
static int countWords(char **words)
{
    int i;

    i = 0;
    while(words[i] != NULL)
        i++;
    return (i);
}
void loadResolution(t_scene *scene, char **words)
{
    static int countCalls = 0;

    countCalls++;
    if(countCalls > 1)
        //error
    if(countWords(words) != 3)
        //error
    scene->resolution.x = ft_atoi(words[1]);
    scene->resolution.y = ft_atoi(words[2]);
}

t_vect3 chargePoint(char *point)
{
    t_vect3 point;
    char **numbers;

    numbers = ft_split(numbers, ',');
    if(countWords(numbers) != 3)
        //error
    point.x = ft_atoi_double(numbers[0]);
    point.y = ft_atoi_double(numbers[1]);
    point.z = ft_atoi_double(numbers[2]);
    freeSpace(numbers);
    return(point);
}
t_vect3 chargeNormalizedVector(char *vector)
{
    t_vect3 vector;
    char **numbers;
    numbers = ft_split(vector, ',');
    if(countWords(numbers) != 3)
        //error
    vector.x = ft_atoi_double(numbers[0]);
    vector.y = ft_atoi_double(numbers[1]);
    vector.z = ft_atoi_double(numbers[2]);
    if(vector.x >= -1 && vector.x <= 1 && vector.y >= -1 && vector.y <= 1 && vector.z >= -1 && vector.z <= 1)
        return (vector);
    //error
}
void loadAmbientLight(t_scene *scene, char **words)
{
    static int countCalls = 0;

    countCalls++;
    if(countCalls > 1)
        //error
    if(countWords(words) != 3)
        //error
    
}

static void processString(t_scene *scene, char **words)
{
    if(countWords(words) <= 0)
    {
        perror("error");
        exit(1);
    }
    if(ft_strncmp("R", words[0], ft_strlen(words[0])))
        loadResolution(scene, words);
    else if(ft_strncmp("A", words[0], ft_strlen(words[0])))
        loadAmbientLight(scene, words);
    else if(ft_strncmp("c", words[0], ft_strlen(words[0])))
        loadCamera(scene, words);
    else if(ft_strncmp("l", words[0], ft_strlen(words[0])))
        loadLight(scene, words);
    else if(ft_strncmp("sp", words[0], ft_strlen(words[0])))
        loadSphere(scene, words);
    else if(ft_strncmp("pl", words[0], ft_strlen(words[0])))
        loadPlane(scene, words);
    else if(ft_strncmp("sq", words[0], ft_strlen(words[0])))
        loadSquare(scene, words);
    else if(ft_strncmp("cy", words[0], ft_strlen(words[0])))
        loadCylinder(scene, words);
    else if(ft_strncmp("tr", words[0], ft_strlen(words[0])))
        loadTriangle(scene, words);
    else
        //error
        
    

}
int loadScene(t_scene *scene, char *fileName)
{
    int i;
    char *line;
    char **words;
    int fd;

    fd = open(fileName, O_RDONLY);
    if(fd > 0)
    while(get_next_line(fd, &line) > 0)
    {
        words = ft_split(line, ' ');
        processString(scene, words);
        freeSpace(words);
        free(line);
    }
}

