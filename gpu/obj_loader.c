/*
** Cargador de modelos .obj (Wavefront) para la version GPU.
**
** Lee vertices (v), caras (f, poligonos de n lados que se parten en
** triangulos en abanico) y materiales (usemtl + mtllib, solo el color
** difuso Kd). Cada triangulo se anade a la escena como un objeto
** GPU_TRIANGLE. Para que 10 millones de triangulos carguen en segundos,
** el fichero se mapea en memoria y los numeros se parsean a mano.
*/

#include <fcntl.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include "obj_loader.h"

typedef struct	s_material
{
	char		name[128];
	t_vec4		color;
}				t_material;

typedef struct	s_obj
{
	simd_float3	*verts;
	size_t		nverts;
	size_t		capverts;
	t_material	*mats;
	int			nmats;
	int			capmats;
	t_vec4		current;
}				t_obj;

static const char	*skip_spaces(const char *p, const char *end)
{
	while (p < end && (*p == ' ' || *p == '\t'))
		p++;
	return (p);
}

static const char	*next_line(const char *p, const char *end)
{
	while (p < end && *p != '\n')
		p++;
	return (p < end ? p + 1 : end);
}

static float		parse_float(const char **pp, const char *end)
{
	const char	*p = skip_spaces(*pp, end);
	double		sign = 1;
	double		value = 0;
	double		scale = 1;
	int			exp = 0;
	int			esign = 1;

	if (p < end && (*p == '-' || *p == '+'))
		sign = (*p++ == '-') ? -1 : 1;
	while (p < end && *p >= '0' && *p <= '9')
		value = value * 10 + (*p++ - '0');
	if (p < end && *p == '.')
		for (p++; p < end && *p >= '0' && *p <= '9'; p++)
			value += (*p - '0') * (scale /= 10);
	if (p < end && (*p == 'e' || *p == 'E'))
	{
		p++;
		if (p < end && (*p == '-' || *p == '+'))
			esign = (*p++ == '-') ? -1 : 1;
		while (p < end && *p >= '0' && *p <= '9')
			exp = exp * 10 + (*p++ - '0');
		value *= pow(10, esign * exp);
	}
	*pp = p;
	return ((float)(sign * value));
}

/*
** Lee el indice de vertice de un "v", "v/vt", "v//vn" o "v/vt/vn" e ignora
** el resto. Los indices negativos cuentan desde el final.
*/

static long			parse_index(const char **pp, const char *end, size_t nverts)
{
	const char	*p = skip_spaces(*pp, end);
	long		sign = 1;
	long		value = 0;

	if (p >= end || *p == '\n' || *p == '\r')
		return (0);
	if (*p == '-')
	{
		sign = -1;
		p++;
	}
	while (p < end && *p >= '0' && *p <= '9')
		value = value * 10 + (*p++ - '0');
	while (p < end && *p != ' ' && *p != '\t' && *p != '\n' && *p != '\r')
		p++;
	*pp = p;
	if (value == 0)
		return (0);
	return (sign > 0 ? value : (long)nverts - value + 1);
}

static void			read_name(const char *p, const char *end, char *out)
{
	int i = 0;

	p = skip_spaces(p, end);
	while (p < end && *p != '\n' && *p != '\r' && i < 127)
		out[i++] = *p++;
	while (i > 0 && (out[i - 1] == ' ' || out[i - 1] == '\t'))
		i--;
	out[i] = 0;
}

static void			*map_file(const char *path, size_t *size)
{
	int			fd = open(path, O_RDONLY);
	struct stat	st;
	void		*data;

	if (fd < 0 || fstat(fd, &st) < 0)
		return (NULL);
	*size = st.st_size;
	data = mmap(NULL, *size, PROT_READ, MAP_PRIVATE, fd, 0);
	close(fd);
	return (data == MAP_FAILED ? NULL : data);
}

static void			load_mtl(t_obj *obj, const char *path)
{
	size_t		size;
	const char	*data = map_file(path, &size);
	const char	*p;
	const char	*end;

	if (!data)
	{
		fprintf(stderr, "aviso: no se pudo abrir %s\n", path);
		return ;
	}
	end = data + size;
	for (p = data; p < end; p = next_line(p, end))
	{
		p = skip_spaces(p, end);
		if (end - p > 7 && strncmp(p, "newmtl", 6) == 0)
		{
			if (obj->nmats == obj->capmats)
			{
				obj->capmats = obj->capmats ? obj->capmats * 2 : 64;
				obj->mats = realloc(obj->mats, sizeof(t_material) * obj->capmats);
			}
			read_name(p + 6, end, obj->mats[obj->nmats].name);
			obj->mats[obj->nmats++].color = (t_vec4){0.8f, 0.8f, 0.8f, 0};
		}
		else if (end - p > 3 && p[0] == 'K' && p[1] == 'd' && obj->nmats > 0)
		{
			const char *q = p + 2;
			float r = parse_float(&q, end);
			float g = parse_float(&q, end);
			float b = parse_float(&q, end);
			obj->mats[obj->nmats - 1].color = (t_vec4){r, g, b, 0};
		}
	}
	munmap((void *)data, size);
}

static void			use_material(t_obj *obj, const char *name)
{
	for (int i = 0; i < obj->nmats; i++)
		if (strcmp(obj->mats[i].name, name) == 0)
		{
			obj->current = obj->mats[i].color;
			return ;
		}
	obj->current = (t_vec4){0.8f, 0.8f, 0.8f, 0};
}

static void			add_triangle(t_gpu_scene *scene, size_t *cap, t_obj *obj,
						long a, long b, long c)
{
	t_gpu_object	*o;

	if (a < 1 || b < 1 || c < 1 || (size_t)a > obj->nverts
		|| (size_t)b > obj->nverts || (size_t)c > obj->nverts)
		return ;
	if ((size_t)scene->nobjects + 1 >= *cap)
	{
		*cap *= 2;
		scene->objects = realloc(scene->objects, sizeof(t_gpu_object) * *cap);
	}
	o = &scene->objects[scene->nobjects++];
	memset(o, 0, sizeof(*o));
	o->a = simd_make_float4(obj->verts[a - 1], 0);
	o->b = simd_make_float4(obj->verts[b - 1], 0);
	o->c = simd_make_float4(obj->verts[c - 1], 0);
	o->color = obj->current;
	o->type = GPU_TRIANGLE;
}

static void			dir_of(const char *path, char *out, size_t len)
{
	const char *slash = strrchr(path, '/');

	if (!slash)
	{
		snprintf(out, len, ".");
		return ;
	}
	snprintf(out, len, "%.*s", (int)(slash - path), path);
}

int					load_obj(t_gpu_scene *scene, const char *path)
{
	size_t		size;
	const char	*data = map_file(path, &size);
	const char	*p;
	const char	*end;
	t_obj		obj;
	size_t		cap;
	char		dir[1024];
	char		name[128];
	long		idx[64];
	int			before = scene->nobjects;

	if (!data)
	{
		fprintf(stderr, "error: no se pudo abrir %s\n", path);
		return (-1);
	}
	memset(&obj, 0, sizeof(obj));
	obj.current = (t_vec4){0.8f, 0.8f, 0.8f, 0};
	obj.capverts = 1 << 20;
	obj.verts = malloc(sizeof(simd_float3) * obj.capverts);
	cap = scene->nobjects + (1 << 20);
	scene->objects = realloc(scene->objects, sizeof(t_gpu_object) * cap);
	dir_of(path, dir, sizeof(dir));
	end = data + size;
	for (p = data; p < end; p = next_line(p, end))
	{
		p = skip_spaces(p, end);
		if (end - p < 2)
			continue ;
		if (p[0] == 'v' && (p[1] == ' ' || p[1] == '\t'))
		{
			const char *q = p + 1;
			if (obj.nverts == obj.capverts)
			{
				obj.capverts *= 2;
				obj.verts = realloc(obj.verts, sizeof(simd_float3) * obj.capverts);
			}
			float x = parse_float(&q, end);
			float y = parse_float(&q, end);
			float z = parse_float(&q, end);
			obj.verts[obj.nverts++] = simd_make_float3(x, y, z);
		}
		else if (p[0] == 'f' && (p[1] == ' ' || p[1] == '\t'))
		{
			const char	*q = p + 1;
			int			n = 0;
			long		v;
			while (n < 64 && (v = parse_index(&q, end, obj.nverts)) != 0)
				idx[n++] = v;
			for (int i = 1; i + 1 < n; i++)
				add_triangle(scene, &cap, &obj, idx[0], idx[i], idx[i + 1]);
		}
		else if (strncmp(p, "usemtl", 6) == 0)
		{
			read_name(p + 6, end, name);
			use_material(&obj, name);
		}
		else if (strncmp(p, "mtllib", 6) == 0)
		{
			char mtl[1200];
			read_name(p + 6, end, name);
			snprintf(mtl, sizeof(mtl), "%s/%s", dir, name);
			load_mtl(&obj, mtl);
		}
	}
	munmap((void *)data, size);
	printf("obj: %zu vertices, %d triangulos, %d materiales\n", obj.nverts,
		scene->nobjects - before, obj.nmats);
	free(obj.verts);
	free(obj.mats);
	return (scene->nobjects - before);
}
