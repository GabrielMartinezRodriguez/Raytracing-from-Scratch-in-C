#include "header.h"


void error(char *s)
{
    write(2, "Error\n", 6);
    write(2, s, ft_strlen(s));
    write(2, "\n", 1);
    exit(1);

}
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
void addObject(t_scene *scene, void *object)
{
    static t_list_obj *list;
    if(list == NULL)
        scene->objets = object;
    else
        list->next = object;
    list = object;
}

t_vect3 chargePoint(char *point)
{
    t_vect3 point1;
    char **numbers;

    numbers = ft_split(point, ',');
    if(countWords(numbers) != 3)
        error("Mal formato de punts en el espacio");
    point1.x = ft_atoi_double(numbers[0]);
    point1.y = ft_atoi_double(numbers[1]);
    point1.z = ft_atoi_double(numbers[2]);
    freeSpace(numbers);
    return(point1);
}
t_color chargeColor(char *rgb)
{
    char    **words;
    t_color returned;

    words = ft_split(rgb, ',');
    if(countWords(words) != 3)
        error("Mal formato de colores RGB");
    returned.red = ft_atoi(words[0]);
    returned.green = ft_atoi(words[1]);
    returned.blue = ft_atoi(words[2]);
    freeSpace(words);
    if(returned.red >= 0 && returned.red <= 255 && returned.green >= 0 && returned.green <= 255 && returned.blue >= 0 && returned.blue <= 255)
        return(returned);
    else
        error("Los componentes de un color codificado en RGB deben estar en el rango [0,255]");
}
t_vect3 chargeNormalizedVector(char *vector)
{
    t_vect3 vector1;
    char **numbers;
    numbers = ft_split(vector, ',');
    if(countWords(numbers) != 3)
        error("Mal formato de vector");
    vector1.x = ft_atoi_double(numbers[0]);
    vector1.y = ft_atoi_double(numbers[1]);
    vector1.z = ft_atoi_double(numbers[2]);
    if(vector1.x >= -1 && vector1.x <= 1 && vector1.y >= -1 && vector1.y <= 1 && vector1.z >= -1 && vector1.z <= 1)
        return (vector1);
    else
        error("Los componentes de un vector de direccion deben estar en el rango [-1,1]");
}
void loadAmbientLight(t_scene *scene, char **words)
{
    static int countCalls = 0;

    countCalls++;
    if(countCalls > 1)
        error("Solo debe de haber una luz ambiental");
    if(countWords(words) != 3)
        error("Mal formato de luz ambiental");
    scene->env_light.intensity = ft_atoi_double(words[1]);
    scene->env_light.color =   chargeColor(words[2]);
}

void loadResolution(t_scene *scene, char **words)
{
    static int countCalls = 0;

    countCalls++;
    if(countCalls > 1)
        error("Solo debe de haber una resolucion");
    if(countWords(words) != 3)
        error("Mal formato de resolucion");
    scene->resolution.x = ft_atoi(words[1]);
    scene->resolution.y = ft_atoi(words[2]);
}

void loadCamera(t_scene *scene, char **words)
{
    static int countCalls = 0;

    countCalls++;
    if(countCalls > 1)
        error("Solo puede haber una camara");
    if(countWords(words) != 4)
        error("Mal formato de camara");
    scene->camera.origin = chargePoint(words[1]);
    scene->camera.direction = chargeNormalizedVector(words[2]);
    scene->camera.FOV_H = ft_atoi_double(words[3]);
}

void loadLight(t_scene *scene, char **words)
{
    static t_list_light *light_list;

    if(countWords(words) != 4)
        error("Mal formato de luz");
    if(light_list == NULL)
        light_list = ft_calloc(1, sizeof(t_list_light));
    else
    {
        light_list->next = ft_calloc(1, sizeof(t_list_light));
        light_list = light_list->next;
    }
    light_list->point.point = chargePoint(words[1]);
    light_list->point.intensity = ft_atoi_double(words[2]);
    light_list->point.color = chargeColor(words[3]);
}

void loadSphere(t_scene *scene, char **words)
{
    t_list_obj *lista;
    t_esfera *sp;

    if(countWords(words) != 4)
        error("Mal formato de esfera");
    lista = ft_calloc(1, sizeof(t_list_obj));
    lista->type = sphere;
    sp = ft_calloc(1, sizeof(t_esfera));
    sp->punto = chargePoint(words[1]);
    sp->radio = ft_atoi_double(words[2]) / 2;
    sp->color = chargeColor(words[3]);
    lista->object = sp;
    addObject(scene, lista);
}
void loadPlane(t_scene *scene, char **words)
{
    t_list_obj *lista;
    t_plane *pl;

    if(countWords(words) != 4)
        error("Mal formato de plano");
    lista = ft_calloc(1, sizeof(t_list_obj));
    lista->type = plane;
    pl = ft_calloc(1, sizeof(t_plane));
    pl->point = chargePoint(words[1]);
    pl->normal = chargeNormalizedVector(words[2]);
    pl->color = chargeColor(words[3]);
    lista->object = pl;
    addObject(scene, lista);
}
void loadSquare(t_scene *scene, char **words)
{
    t_list_obj *lista;
    t_square *sq;

    if(countWords(words) != 5)
        error("Mal formato de cuadrado");
    lista = ft_calloc(1, sizeof(t_list_obj));
    lista->type = square;
    sq = ft_calloc(1, sizeof(t_square));
    sq->point = chargePoint(words[1]);
    sq->normal = chargeNormalizedVector(words[2]);
    sq->height = ft_atoi_double(words[3]);
    sq->color = chargeColor(words[4]);
    lista->object = sq;
    addObject(scene, lista);
}
void loadCylinder(t_scene *scene, char **words)
{
    t_list_obj *lista;
    t_cylinder *cyl;

    if(countWords(words) != 6)
        error("Mal formato de cilindro");
    lista = ft_calloc(1, sizeof(t_list_obj));
    lista->type = cylinder;
    cyl = ft_calloc(1, sizeof(t_cylinder));
    cyl->point = chargePoint(words[1]);
    cyl->normal = chargeNormalizedVector(words[2]);
    cyl->diameter = ft_atoi_double(words[3]);
    cyl->height = ft_atoi_double(words[4]);
    cyl->color = chargeColor(words[5]);
    lista->object = cyl;
    addObject(scene, lista);
}
void loadTriangle(t_scene *scene, char **words)
{
    t_list_obj *lista;
    t_triangle *tri;

    if(countWords(words) != 5)
        error("Mal formato de triangulo");
    lista = ft_calloc(1, sizeof(t_list_obj));
    lista->type = triangle;
    tri = ft_calloc(1, sizeof(t_cylinder));
    tri->point1 = chargePoint(words[1]);
    tri->point2 = chargePoint(words[2]);
    tri->point3 = chargePoint(words[3]);
    tri->s_color = chargeColor(words[4]);
    lista->object = tri;
    addObject(scene, lista);
}
static void processString(t_scene *scene, char **words)
{
    if(ft_strncmp("R", words[0], ft_strlen(words[0])) == 0)
        loadResolution(scene, words);
    else if(ft_strncmp("A", words[0], ft_strlen(words[0])) == 0)
        loadAmbientLight(scene, words);
    else if(ft_strncmp("c", words[0], ft_strlen(words[0])) == 0)
        loadCamera(scene, words);
    else if(ft_strncmp("l", words[0], ft_strlen(words[0])) == 0)
        loadLight(scene, words);
    else if(ft_strncmp("sp", words[0], ft_strlen(words[0])) == 0)
        loadSphere(scene, words);
    else if(ft_strncmp("pl", words[0], ft_strlen(words[0])) == 0)
        loadPlane(scene, words);
    else if(ft_strncmp("sq", words[0], ft_strlen(words[0])) == 0)
        loadSquare(scene, words);
    else if(ft_strncmp("cy", words[0], ft_strlen(words[0])) == 0)
        loadCylinder(scene, words);
    else if(ft_strncmp("tr", words[0], ft_strlen(words[0])) == 0)
        loadTriangle(scene, words);
    else
        error("No se ha reconocido un identificador");
}
int loadScene(t_scene *scene, char *fileName)
{
    int i;
    char *line;
    char **words;
    int fd;
    int end;

    fd = open(fileName, O_RDONLY);
    if(fd < 0)
        error("No se ha podido abrir el fichero indicado");
    end = 0;
    while(!end)
    {
        if(get_next_line(fd, &line) == 0)
            end = 1;
        words = ft_split(line, ' ');
        processString(scene, words);
        freeSpace(words);
        free(line);
    }
}
