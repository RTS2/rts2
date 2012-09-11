#include <math.h>
#include <assert.h>

#include <vector>
#include <string>
#include <algorithm>
#include <functional>

#include "rts2fits/image.h"

#define APP_SIZE        3

using namespace std;

namespace rts2image
{

struct pint
{
	float radius;
	unsigned short value;
};

class comparePixel
{
	public:
		bool operator () (struct pixel x, struct pixel y) const
		{
			return x.value > y.value;
		}
};

class comparePint
{
	public:
		bool operator     () (struct pint x, struct pint y) const
		{
			return x.value > y.value;
		}
};

}

using namespace rts2image;

static int cmpdouble (const void *a, const void *b)
{
	if (*((double *) a) > *((double *) b))
		return 1;
	if (*((double *) a) < *((double *) b))
		return -1;
	return 0;
}

double Image::classicMedian (double *q, int n, double *retsigma)
{
	int i;
	double *f, M, S;

	f = (double *) malloc (n * sizeof (double));

	memcpy (f, q, n * sizeof (double));
	qsort (f, n, sizeof (double), cmpdouble);

	if (n % 2)
		M = f[n / 2];
	else
		M = f[n / 2] / 2 + f[n / 2 + 1] / 2;

	if (retsigma)
	{
		for (i = 0; i < n; i++)
			f[i] = fabs (f[i] - M) * 0.6745;

		qsort (f, n, sizeof (double), cmpdouble);

		if (n % 2)
			S = f[n / 2];
		else
			S = f[n / 2] / 2 + f[n / 2 + 1] / 2;

		*retsigma = S;
	}
	free (f);

	return M;
}

int Image::findMaxIntensity (unsigned short *in_data, struct pixel *ret)
{
	int x, y, max_x, max_y, pix = 0;

	for (x = 0; x < getChannelWidth (0); x++)
	{
		for (y = 0; y < getChannelHeight (0); y++)
		{
			if (getPixel (in_data, x, y) > pix)
			{
				max_x = x;
				max_y = y;
				pix = getPixel (in_data, x, y);
			}
		}
	}
	ret->x = max_x;
	ret->y = max_y;

	return 0;
}

unsigned short Image::getPixel (unsigned short *in_data, int x, int y)
{
	return in_data[x + (getChannelWidth (0) * y)];
}

int Image::findStar (unsigned short *in_data)
{
	float cols_sum[APP_SIZE];
	unsigned short *row_start_ptr, *data_ptr;
	float sum;
	int r, c, i, j, x;
	struct pixel tmp;
	bool first = true;
	bool sedi = true;

	assert (getChannelWidth (0) >= APP_SIZE && getChannelHeight (0) >= APP_SIZE);

	for (r = 0; r < getChannelHeight (0) - APP_SIZE; r++)
	{
		row_start_ptr = in_data + r * getChannelWidth (0);
		sum = 0;

		for (i = 0; i < APP_SIZE - 1; i++)
		{
			data_ptr = row_start_ptr;
			cols_sum[i] = 0;

			for (j = 0; j < APP_SIZE; j++)
			{
				cols_sum[i] += *data_ptr;
				data_ptr += getChannelWidth (0);
			}
			sum += cols_sum[i];
			row_start_ptr++;
		}

		cols_sum[APP_SIZE - 1] = 0;

		for (c = APP_SIZE - 1; c < getChannelWidth (0); c++)
		{
			data_ptr = row_start_ptr;
			sum -= cols_sum[c % APP_SIZE];
			cols_sum[c % APP_SIZE] = 0;

			for (j = 0; j < APP_SIZE; j++)
			{
				cols_sum[c % APP_SIZE] += *data_ptr;
				data_ptr += getChannelWidth (0);
			}

			sum += cols_sum[c % APP_SIZE];

			if (sum / (float) (APP_SIZE * APP_SIZE) > median + 10 * (sigma))
			{
				#ifdef VERBOSE
				fprintf (stderr, "%4i %4i\n", c, r);
				#endif
				if (first)
				{
					tmp.x = c;
					tmp.y = r;
					list.push_back (tmp);
					first = false;
					#ifdef VERBOSE
					printf ("%d %d\n", c, r);
					#endif
				}
				else
				{
					sedi = false;
					for (x = 0; x < (int) list.size (); x++)
					{
						tmp = list[x];
						if (fabs ((double) (c - tmp.x)) < 10
							|| fabs ((double) (r - tmp.y)) < 10)
						{
							sedi = true;
						}
					}

					if (!sedi)
					{
						tmp.x = c;
						tmp.y = r;
						list.push_back (tmp);
						#ifdef VERBOSE
						printf ("%d %d\n", c, r);
						#endif
					}
				}
			}
			row_start_ptr++;
		}
	}
	return 0;
}

int Image::aperture (unsigned short *in_data, struct pixel pix, struct pixel *ret)
{
	int i, j;
	struct pixel tmp;
	vector < pixel > rada;

	for (j = pix.y; j < pix.y + 10; j++)
	{
		for (i = pix.x; i < pix.x + 10; i++)
		{
			tmp.x = i;
			tmp.y = j;
			tmp.value = getPixel (in_data, i, j);
			rada.push_back (tmp);
		}
	}

	sort (rada.begin (), rada.end (), comparePixel ());
	#ifdef VERBOSE
	for (i = 0; i < (int) rada.size (); i++)
	{
		tmp = rada[i];
		printf ("%d %d %d\n", tmp.x, tmp.y, tmp.value);
	}
	#else
	tmp = rada[0];
	#ifdef VERBOSE
	printf ("%d %d %d\n", tmp.x, tmp.y, tmp.value);
	#endif
	#endif
	*ret = tmp;
	return 0;
}

int Image::centroid (unsigned short *in_data, struct pixel pix, float *px, float *py)
{
	int i, j;
	float cmean, total, subtotal;

	for (cmean = 0.0, total = 0.0, i = pix.x - 3; i < pix.x + 4; i++)
	{
		for (subtotal = 0.0, j = pix.y - 3; j < pix.y + 4; j++)
		{
			if (getPixel (in_data, i, j) > median + 6 * sigma)
				subtotal += getPixel (in_data, i, j);
		}
		total += subtotal;
		cmean += (i * subtotal);
	}
	*px = cmean / total;

	for (cmean = 0.0, total = 0.0, j = pix.y - 3; j < pix.y + 4; j++)
	{
		for (subtotal = 0.0, i = pix.x - 3; i < pix.x + 4; i++)
		{
			if (getPixel (in_data, i, j) > median + 6 * sigma)
				subtotal += getPixel (in_data, i, j);
		}
		total += subtotal;
		cmean += (j * subtotal);
	}
	*py = cmean / total;

	return 0;
}

int Image::radius (unsigned short *in_data, double px, double py, int rmax)
{
	int i, j, r, inrr, outrr, xyrr, yrr, np;
	double sum, cmean;
	unsigned short dp;

	for (r = 2; r <= rmax; r++)
	{
		inrr = r * r;
		outrr = (r + 1) * (r + 1);
		np = 0;
		sum = 0.0;

		for (j = (int) py - r; j <= (int) py + r; j++)
		{
			yrr = (j - (int) py) * (j - (int) py);
			for (i = (int) px - r; i <= (int) px + r; i++)
			{
				xyrr = (i - (int) px) * (i - (int) px) + yrr;
				if (xyrr >= inrr && xyrr < outrr)
				{
					dp = getPixel (in_data, i, j);
					sum += dp;
					np++;
				}
			}
		}
		cmean = (sum / np) - (median + sigma);

		if (cmean < 0.77)
		{
			break;
		}
	}

	return (r);
}

#ifndef RTS2_HAVE_ROUND
#define round(x)  ((int) x)
#endif

int Image::integrate (unsigned short *in_data, double px, double py, int size, float *ret)
{
	int i, j;
	float rad;
	struct pint part;
	vector < pint > integral;
	unsigned short res = 0, subres = 0;

	for (i = (int) round (px) - size; i <= (int) round (px) + size; i++)
		for (j = (int) round (py) - size; j <= (int) round (py) + size; j++)
	{
		rad =
			sqrt (fabs (px - i) * fabs (px - i) +
			fabs (py - j) * fabs (py - j));
		if (rad <= size && (getPixel (in_data, i, j) > median + (6 * sigma)))
		{
			part.radius = rad;
			part.value = getPixel (in_data, i, j);
			res += part.value;
			integral.push_back (part);
		}
	}

	sort (integral.begin (), integral.end (), comparePint ());

	for (i = 0; i < (int) integral.size (); i++)
	{
		part = integral[i];
		subres += part.value;
		if (subres > res / 2)
		{
			#ifdef VERBOSE
			printf ("%3.2lf %3.2lf %2.2lf\n", px, py, part.radius);
			#endif
			break;
		}
	}
	*ret = part.radius;
	return 0;
}
