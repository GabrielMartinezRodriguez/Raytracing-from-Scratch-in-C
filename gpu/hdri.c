/*
** Lector de imagenes .hdr (Radiance RGBE, con compresion RLE por lineas) y
** extraccion del sol para usarlo como luz de disco.
*/

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "hdri.h"

static void	rgbe_to_float(const unsigned char *e, float *out)
{
	if (e[3] == 0)
	{
		out[0] = out[1] = out[2] = 0;
		return ;
	}
	float f = ldexpf(1.0f, e[3] - (128 + 8));
	out[0] = e[0] * f;
	out[1] = e[1] * f;
	out[2] = e[2] * f;
}

static int	read_scanline(FILE *fp, unsigned char *line, int w)
{
	unsigned char	head[4];

	if (fread(head, 1, 4, fp) != 4)
		return (-1);
	if (head[0] != 2 || head[1] != 2 || (head[2] & 0x80))
	{
		/* formato antiguo sin RLE */
		memcpy(line, head, 4);
		return (fread(line + 4, 4, w - 1, fp) == (size_t)(w - 1) ? 0 : -1);
	}
	for (int c = 0; c < 4; c++)
	{
		int x = 0;
		while (x < w)
		{
			int n = fgetc(fp);
			if (n == EOF)
				return (-1);
			if (n > 128)
			{
				n -= 128;
				int v = fgetc(fp);
				while (n-- > 0 && x < w)
					line[(x++) * 4 + c] = (unsigned char)v;
			}
			else
				while (n-- > 0 && x < w)
					line[(x++) * 4 + c] = (unsigned char)fgetc(fp);
		}
	}
	return (0);
}

int			load_hdr(const char *path, t_hdri *img)
{
	FILE			*fp = fopen(path, "rb");
	char			buf[256];
	int				w = 0;
	int				h = 0;

	if (!fp)
		return (-1);
	while (fgets(buf, sizeof(buf), fp) && buf[0] != '\n')
		;
	if (!fgets(buf, sizeof(buf), fp) || sscanf(buf, "-Y %d +X %d", &h, &w) != 2)
	{
		fclose(fp);
		return (-1);
	}
	unsigned char *line = malloc(w * 4);
	img->rgb = malloc(sizeof(float) * 3 * w * h);
	img->w = w;
	img->h = h;
	for (int y = 0; y < h; y++)
	{
		if (read_scanline(fp, line, w) < 0)
			break ;
		for (int x = 0; x < w; x++)
			rgbe_to_float(line + x * 4, img->rgb + (y * w + x) * 3);
	}
	free(line);
	fclose(fp);
	return (0);
}

static float	lum(const float *c)
{
	return (0.2126f * c[0] + 0.7152f * c[1] + 0.0722f * c[2]);
}

/*
** Direccion de un pixel, con la misma convencion que el shader:
** theta desde +Y, phi = atan2(x, -z).
*/

static void		pixel_dir(int x, int y, int w, int h, float d[3])
{
	float	th = (y + 0.5f) / h * (float)M_PI;
	float	ph = ((x + 0.5f) / w - 0.5f) * 2 * (float)M_PI;

	d[0] = sinf(th) * sinf(ph);
	d[1] = cosf(th);
	d[2] = -sinf(th) * cosf(ph);
}

void			extract_sun(t_hdri *img, float dir[3], float color[3],
					float *cos_radius)
{
	int		w = img->w;
	int		h = img->h;
	int		best = 0;
	float	sd[3] = {0, 0, 0};
	float	maxl = 0;
	double	max_ang = 0;

	for (int i = 0; i < w * h; i++)
		if (lum(img->rgb + i * 3) > maxl)
		{
			maxl = lum(img->rgb + i * 3);
			best = i;
		}
	float bd[3];
	pixel_dir(best % w, best / w, w, h, bd);
	/*
	** Umbral de "demasiado brillante para ser cielo": 40 veces el brillo
	** medio. El halo alrededor del sol (mas de eso pero menos del 2% del
	** pico) tambien pasa al disco; si se quedara en el mapa, los pocos rayos
	** de rebote que lo encuentran darian puntos blancos sueltos.
	*/
	double total = 0;
	for (int i = 0; i < w * h; i++)
		total += lum(img->rgb + i * 3);
	float hot = 40.0f * (float)(total / ((double)w * h));
	/*
	** Sol = pixeles a menos de 3 grados del mas brillante y por encima del
	** umbral. Su luz (radiancia por angulo solido) se suma y esos
	** pixeles se sustituyen por el brillo del cielo de alrededor.
	*/
	color[0] = color[1] = color[2] = 0;
	float limit = cosf(3.0f * (float)M_PI / 180);
	float sky[3] = {0, 0, 0};
	int nsky = 0;
	for (int y = 0; y < h; y++)
	{
		float th = (y + 0.5f) / h * (float)M_PI;
		float domega = (2 * (float)M_PI / w) * ((float)M_PI / h) * sinf(th);
		for (int x = 0; x < w; x++)
		{
			float d[3];
			float *c = img->rgb + (y * w + x) * 3;
			pixel_dir(x, y, w, h, d);
			float cs = d[0] * bd[0] + d[1] * bd[1] + d[2] * bd[2];
			if (cs < limit)
			{
				if (cs > cosf(6.0f * (float)M_PI / 180))
				{
					for (int k = 0; k < 3; k++)
						sky[k] += c[k];
					nsky++;
				}
				continue ;
			}
			if (lum(c) < hot)
				continue ;
			for (int k = 0; k < 3; k++)
			{
				color[k] += c[k] * domega;
				sd[k] += d[k] * lum(c) * domega;
			}
			if (lum(c) >= 0.02f * maxl && acos(fmin(cs, 1.0)) > max_ang)
				max_ang = acos(fmin(cs, 1.0));
			c[0] = c[1] = c[2] = -1;
		}
	}
	for (int k = 0; k < 3; k++)
		sky[k] = nsky ? sky[k] / nsky : 0;
	/*
	** Cualquier otro punto muy brillante (reflejos, nubes al sol) se recorta
	** al umbral por la misma razon.
	*/
	for (int i = 0; i < w * h; i++)
	{
		float *c = img->rgb + i * 3;
		float l = c[0] < 0 ? 0 : lum(c);
		if (l > hot)
			for (int k = 0; k < 3; k++)
				c[k] *= hot / l;
	}
	for (int i = 0; i < w * h; i++)
		if (img->rgb[i * 3] < 0)
			for (int k = 0; k < 3; k++)
				img->rgb[i * 3 + k] = sky[k];
	float n = sqrtf(sd[0] * sd[0] + sd[1] * sd[1] + sd[2] * sd[2]);
	for (int k = 0; k < 3; k++)
		dir[k] = n > 0 ? sd[k] / n : bd[k];
	/*
	** El sol real mide ~0,53 grados; el disco usado no baja de eso.
	*/
	*cos_radius = cosf(fmaxf((float)max_ang, 0.27f * (float)M_PI / 180));
}
