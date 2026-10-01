/*
** Cargador de modelos .obj (Wavefront) para la version GPU.
**
** Lee vertices (v), coordenadas de textura (vt), normales (vn), caras (f,
** poligonos de n lados que se parten en triangulos en abanico) y
** materiales (usemtl + mtllib: color difuso Kd y textura map_Kd). Para que
** 10 millones de triangulos carguen en segundos, el fichero se mapea en
** memoria y los numeros se parsean a mano.
*/

#include <fcntl.h>
#include <math.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include "obj_loader.h"

typedef struct	s_obj
{
	simd_float3	*v;
	simd_float2	*vt;
	simd_float3	*vn;
	size_t		nv;
	size_t		nvt;
	size_t		nvn;
	size_t		capv;
	size_t		capvt;
	size_t		capvn;
	unsigned int	material;
}				t_obj;

typedef struct	s_corner
{
	long		v;
	long		vt;
	long		vn;
}				t_corner;

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

static long			parse_int(const char **pp, const char *end, size_t count)
{
	const char	*p = *pp;
	long		sign = 1;
	long		value = 0;

	if (p < end && *p == '-')
	{
		sign = -1;
		p++;
	}
	while (p < end && *p >= '0' && *p <= '9')
		value = value * 10 + (*p++ - '0');
	*pp = p;
	if (value == 0)
		return (0);
	return (sign > 0 ? value : (long)count - value + 1);
}

/*
** Una esquina de cara: "v", "v/vt", "v//vn" o "v/vt/vn". Los indices
** negativos cuentan desde el final. Devuelve 0 si no hay mas esquinas.
*/

static int			parse_corner(const char **pp, const char *end, t_obj *obj,
						t_corner *c)
{
	const char	*p = skip_spaces(*pp, end);

	c->v = 0;
	c->vt = 0;
	c->vn = 0;
	if (p >= end || *p == '\n' || *p == '\r' || *p == '#')
		return (0);
	c->v = parse_int(&p, end, obj->nv);
	if (p < end && *p == '/')
	{
		p++;
		if (p < end && *p != '/')
			c->vt = parse_int(&p, end, obj->nvt);
		if (p < end && *p == '/')
		{
			p++;
			c->vn = parse_int(&p, end, obj->nvn);
		}
	}
	while (p < end && *p != ' ' && *p != '\t' && *p != '\n' && *p != '\r')
		p++;
	*pp = p;
	return (c->v != 0);
}

static void			read_name(const char *p, const char *end, char *out,
						size_t len)
{
	size_t i = 0;

	p = skip_spaces(p, end);
	while (p < end && *p != '\n' && *p != '\r' && i + 1 < len)
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

	if (fd < 0)
		return (NULL);
	if (fstat(fd, &st) < 0)
	{
		close(fd);
		return (NULL);
	}
	*size = st.st_size;
	data = mmap(NULL, *size, PROT_READ, MAP_PRIVATE, fd, 0);
	close(fd);
	return (data == MAP_FAILED ? NULL : data);
}

static t_mesh_material	*new_material(t_gpu_mesh *mesh, const char *name)
{
	t_mesh_material	*m;

	if (mesh->nmats == mesh->capmats)
	{
		mesh->capmats = mesh->capmats ? mesh->capmats * 2 : 64;
		mesh->mats = realloc(mesh->mats, sizeof(t_mesh_material) * mesh->capmats);
	}
	m = &mesh->mats[mesh->nmats++];
	memset(m, 0, sizeof(*m));
	snprintf(m->name, sizeof(m->name), "%s", name);
	m->kd = simd_make_float4(0.8f, 0.8f, 0.8f, 0);
	m->roughness = 0.9f;
	m->metallic = 0;
	return (m);
}

/*
** Las rutas del .mtl de San Miguel vienen con barras de Windows.
*/

static void			map_path(const char *p, const char *end, const char *dir,
						char *out, size_t len);

static void			load_mtl(t_gpu_mesh *mesh, const char *path, const char *dir)
{
	size_t			size;
	const char		*data = map_file(path, &size);
	const char		*p;
	const char		*end;
	t_mesh_material	*m = NULL;
	char			file[1024];

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
			read_name(p + 6, end, file, sizeof(file));
			m = new_material(mesh, file);
		}
		else if (m && end - p > 3 && p[0] == 'K' && p[1] == 'd')
		{
			const char *q = p + 2;
			float r = parse_float(&q, end);
			float g = parse_float(&q, end);
			float b = parse_float(&q, end);
			m->kd = simd_make_float4(r, g, b, 0);
		}
		else if (m && end - p > 7 && strncmp(p, "map_Kd", 6) == 0)
			map_path(p + 6, end, dir, m->texture, sizeof(m->texture));
		else if (m && end - p > 7 && (strncmp(p, "map_Bump", 8) == 0
			|| strncmp(p, "map_bump", 8) == 0 || strncmp(p, "norm ", 5) == 0))
			map_path(p + (p[0] == 'n' ? 4 : 8), end, dir, m->normal_map,
				sizeof(m->normal_map));
		else if (m && end - p > 7 && strncmp(p, "map_Pr", 6) == 0)
			map_path(p + 6, end, dir, m->rough_map, sizeof(m->rough_map));
		else if (m && end - p > 7 && strncmp(p, "map_Pm", 6) == 0)
			map_path(p + 6, end, dir, m->metal_map, sizeof(m->metal_map));
		else if (m && end - p > 3 && p[0] == 'P' && p[1] == 'r' && p[2] == ' ')
		{
			const char *q = p + 2;
			m->roughness = parse_float(&q, end);
		}
		else if (m && end - p > 3 && p[0] == 'P' && p[1] == 'm' && p[2] == ' ')
		{
			const char *q = p + 2;
			m->metallic = parse_float(&q, end);
		}
	}
	munmap((void *)data, size);
}

/*
** Ruta de un mapa del .mtl: salta opciones como "-bm 1.0" y convierte las
** barras de Windows.
*/

static void			map_path(const char *p, const char *end, const char *dir,
						char *out, size_t len)
{
	char	file[1024];
	char	*name = file;

	read_name(p, end, file, sizeof(file));
	while (*name == '-')
	{
		while (*name && *name != ' ')
			name++;
		while (*name == ' ')
			name++;
		while (*name && *name != ' ' && (*name == '.' || *name == '-'
			|| (*name >= '0' && *name <= '9')))
			name++;
		while (*name == ' ')
			name++;
	}
	for (char *c = name; *c; c++)
		if (*c == '\\')
			*c = '/';
	snprintf(out, len, "%s/%s", dir, name);
}

static unsigned int	find_material(t_gpu_mesh *mesh, const char *name)
{
	for (int i = 1; i < mesh->nmats; i++)
		if (strcmp(mesh->mats[i].name, name) == 0)
			return (i);
	return (0);
}

static short		to_snorm(float x)
{
	x = fminf(fmaxf(x, -1), 1);
	return ((short)lrintf(x * 32767.0f));
}

/*
** Normal unitaria comprimida en 32 bits (proyeccion octaedrica). 0 = sin
** normal: la GPU usara la normal geometrica del triangulo.
*/

unsigned int		pack_normal(simd_float3 n)
{
	float		l1 = fabsf(n.x) + fabsf(n.y) + fabsf(n.z);
	simd_float2	p;

	if (l1 < 1e-12f)
		return (0);
	n /= l1;
	p = simd_make_float2(n.x, n.y);
	if (n.z < 0)
		p = simd_make_float2((1 - fabsf(n.y)) * (n.x >= 0 ? 1 : -1),
			(1 - fabsf(n.x)) * (n.y >= 0 ? 1 : -1));
	return ((unsigned short)to_snorm(p.x)
		| ((unsigned int)(unsigned short)to_snorm(p.y) << 16));
}

static void			add_triangle(t_gpu_mesh *mesh, t_obj *obj, t_corner *c)
{
	t_gpu_tri	*t;
	float		*pos;

	for (int k = 0; k < 3; k++)
		if (c[k].v < 1 || (size_t)c[k].v > obj->nv)
			return ;
	if (mesh->ntris == mesh->cap)
	{
		mesh->cap = mesh->cap ? mesh->cap * 2 : 1 << 20;
		mesh->tris = realloc(mesh->tris, sizeof(t_gpu_tri) * mesh->cap);
		mesh->pos = realloc(mesh->pos, sizeof(float) * 9 * mesh->cap);
	}
	t = &mesh->tris[mesh->ntris];
	pos = &mesh->pos[mesh->ntris * 9];
	mesh->ntris++;
	for (int k = 0; k < 3; k++)
	{
		simd_float3 v = obj->v[c[k].v - 1];
		pos[k * 3] = v.x;
		pos[k * 3 + 1] = v.y;
		pos[k * 3 + 2] = v.z;
		simd_float2 uv = (c[k].vt >= 1 && (size_t)c[k].vt <= obj->nvt)
			? obj->vt[c[k].vt - 1] : simd_make_float2(0, 0);
		t->uv[k * 2] = uv.x;
		t->uv[k * 2 + 1] = uv.y;
		t->n[k] = (c[k].vn >= 1 && (size_t)c[k].vn <= obj->nvn)
			? pack_normal(obj->vn[c[k].vn - 1]) : 0;
	}
	t->material = obj->material;
}

static void			dir_of(const char *path, char *out, size_t len)
{
	const char *slash = strrchr(path, '/');

	if (!slash)
		snprintf(out, len, ".");
	else
		snprintf(out, len, "%.*s", (int)(slash - path), path);
}

#define GROW(arr, n, cap) do { if ((n) == (cap)) { (cap) = (cap) ? (cap) * 2 \
	: 1 << 16; (arr) = realloc((arr), sizeof(*(arr)) * (cap)); } } while (0)

long				load_obj(t_gpu_mesh *mesh, const char *path)
{
	size_t		size;
	const char	*data = map_file(path, &size);
	const char	*p;
	const char	*end;
	t_obj		obj;
	char		dir[1024];
	char		name[1024];
	t_corner	c[3];

	if (!data)
	{
		fprintf(stderr, "error: no se pudo abrir %s\n", path);
		return (-1);
	}
	memset(&obj, 0, sizeof(obj));
	if (mesh->nmats == 0)
		new_material(mesh, "(por defecto)");
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
			GROW(obj.v, obj.nv, obj.capv);
			float x = parse_float(&q, end);
			float y = parse_float(&q, end);
			float z = parse_float(&q, end);
			obj.v[obj.nv++] = simd_make_float3(x, y, z);
		}
		else if (p[0] == 'v' && p[1] == 't')
		{
			const char *q = p + 2;
			GROW(obj.vt, obj.nvt, obj.capvt);
			float u = parse_float(&q, end);
			float v = parse_float(&q, end);
			obj.vt[obj.nvt++] = simd_make_float2(u, v);
		}
		else if (p[0] == 'v' && p[1] == 'n')
		{
			const char *q = p + 2;
			GROW(obj.vn, obj.nvn, obj.capvn);
			float x = parse_float(&q, end);
			float y = parse_float(&q, end);
			float z = parse_float(&q, end);
			obj.vn[obj.nvn++] = simd_make_float3(x, y, z);
		}
		else if (p[0] == 'f' && (p[1] == ' ' || p[1] == '\t'))
		{
			const char *q = p + 1;
			if (!parse_corner(&q, end, &obj, &c[0])
				|| !parse_corner(&q, end, &obj, &c[1]))
				continue ;
			while (parse_corner(&q, end, &obj, &c[2]))
			{
				add_triangle(mesh, &obj, c);
				c[1] = c[2];
			}
		}
		else if (strncmp(p, "usemtl", 6) == 0)
		{
			read_name(p + 6, end, name, sizeof(name));
			obj.material = find_material(mesh, name);
		}
		else if (strncmp(p, "mtllib", 6) == 0)
		{
			char mtl[2100];
			read_name(p + 6, end, name, sizeof(name));
			snprintf(mtl, sizeof(mtl), "%s/%s", dir, name);
			load_mtl(mesh, mtl, dir);
		}
	}
	munmap((void *)data, size);
	printf("obj: %zu vertices, %zu coordenadas de textura, %zu normales, "
		"%zu triangulos, %d materiales\n", obj.nv, obj.nvt, obj.nvn,
		mesh->ntris, mesh->nmats - 1);
	free(obj.v);
	free(obj.vt);
	free(obj.vn);
	return ((long)mesh->ntris);
}

typedef struct	s_cache_header
{
	char		magic[8];
	uint64_t	obj_size;
	int64_t		obj_mtime;
	uint64_t	ntris;
	int32_t		nmats;
	int32_t		pad;
}				t_cache_header;

static const char	g_magic[8] = "RTMESH2";

static void			obj_stat(const char *path, uint64_t *size, int64_t *mtime)
{
	struct stat	st;

	*size = 0;
	*mtime = 0;
	if (stat(path, &st) == 0)
	{
		*size = st.st_size;
		*mtime = st.st_mtimespec.tv_sec;
	}
}

static int			save_cache(t_gpu_mesh *mesh, const char *obj, const char *cache)
{
	t_cache_header	h;
	FILE			*f = fopen(cache, "wb");

	if (!f)
		return (-1);
	memset(&h, 0, sizeof(h));
	memcpy(h.magic, g_magic, 8);
	obj_stat(obj, &h.obj_size, &h.obj_mtime);
	h.ntris = mesh->ntris;
	h.nmats = mesh->nmats;
	fwrite(&h, sizeof(h), 1, f);
	fwrite(mesh->mats, sizeof(t_mesh_material), mesh->nmats, f);
	fwrite(mesh->pos, sizeof(float) * 9, mesh->ntris, f);
	fwrite(mesh->tris, sizeof(t_gpu_tri), mesh->ntris, f);
	fclose(f);
	return (0);
}

static long			load_cache(t_gpu_mesh *mesh, const char *obj, const char *cache)
{
	size_t				size;
	char				*data = map_file(cache, &size);
	const t_cache_header	*h = (const t_cache_header *)data;
	uint64_t			osize;
	int64_t				omtime;

	if (!data)
		return (-1);
	obj_stat(obj, &osize, &omtime);
	if (size < sizeof(*h) || memcmp(h->magic, g_magic, 8) != 0
		|| h->obj_size != osize || h->obj_mtime != omtime
		|| size != sizeof(*h) + h->nmats * sizeof(t_mesh_material)
		+ h->ntris * (sizeof(float) * 9 + sizeof(t_gpu_tri)))
	{
		munmap(data, size);
		return (-1);
	}
	mesh->nmats = h->nmats;
	mesh->capmats = h->nmats;
	mesh->mats = malloc(sizeof(t_mesh_material) * h->nmats);
	memcpy(mesh->mats, data + sizeof(*h), sizeof(t_mesh_material) * h->nmats);
	mesh->ntris = h->ntris;
	mesh->cap = h->ntris;
	mesh->pos = (float *)(data + sizeof(*h) + sizeof(t_mesh_material) * h->nmats);
	mesh->tris = (t_gpu_tri *)((char *)mesh->pos + sizeof(float) * 9 * h->ntris);
	mesh->map = data;
	mesh->map_size = size;
	return ((long)mesh->ntris);
}

long				load_obj_cached(t_gpu_mesh *mesh, const char *path)
{
	char	cache[2048];
	long	n;

	snprintf(cache, sizeof(cache), "%s.rtcache", path);
	if ((n = load_cache(mesh, path, cache)) >= 0)
	{
		printf("obj: %ld triangulos desde la cache %s\n", n, cache);
		return (n);
	}
	if ((n = load_obj(mesh, path)) >= 0 && save_cache(mesh, path, cache) == 0)
		printf("cache guardada en %s\n", cache);
	return (n);
}

void				free_mesh_data(t_gpu_mesh *mesh)
{
	if (mesh->map)
		munmap(mesh->map, mesh->map_size);
	else
	{
		free(mesh->pos);
		free(mesh->tris);
	}
	mesh->map = NULL;
	mesh->pos = NULL;
	mesh->tris = NULL;
}
