#include <fitsio.h>
#include <string.h>

void
printerror (int status)
{
  if (status)
    fits_report_error (stdout, status);	// print error report
}

int
write_fits (char *filename, long exposure, int w, int h, unsigned short *data)
{
  fitsfile *fptr = NULL;
  int status = 0;
  long naxes[2] = { w, h };

  if ((!strlen (filename))
      || ((strlen (filename) == 1) && (*filename == '!')))
    return 1;

  fits_clear_errmsg ();
  if (fits_create_file (&fptr, filename, &status))
    printerror (status);
  if (fits_create_img (fptr, USHORT_IMG, 2, naxes, &status))
    printerror (status);
  if (fits_update_key
      (fptr, TLONG, "EXPOSURE", &exposure, "Total exposure time", &status))
    printerror (status);
  if (fits_write_img (fptr, TUSHORT, 1, w * h, data, &status))
    printerror (status);
  if (fits_close_file (fptr, &status))
    printerror (status);

  return 0;
}


void
getmeandisp (unsigned short *img, int len, double *mean, double *disp)
{
  int i;
  double d;

  if (len < 1)
    return;
  if (len == 1)
    {
      *mean = *img;
      *disp = 0;
      return;
    }

  *mean = 0;
  i = len;
  while (i)
    *mean += img[--i];
  *mean /= len;

  *disp = 0;
  i = len;
  while (i)
    {
      d = *mean - img[--i];
      *disp += d * d;
    }
  *disp /= len - 1;
  *disp = sqrt (*disp);
}

// Specialni funkce pro regresi funkce z=Ax^2+Cx+F
// vektory predpoklada struct v[i] : i je pocet, v pointer na zacatek dat,
// zbytek je pro navratove hodnoty.
#define DIM 3
void
regr_q (double *vX, double *vZ, long i, double *rA, double *rB, double *rC)
{
  double A = 0, B = 0, C = 0, D = 0, N = 0, Q, V = 0, X = 0, Z =
    0, M[DIM][DIM + 1], _B, _C, _D, _Z;
  long a, b, c;

  // spocitame koeficienty
  for (a = 0; a < i; a++)
    {
      _D = vX[a];
      _C = _D * _D;
      _B = _D * _C;
      _Z = vZ[a];
      A += _C * _C;
      B += _B;
      C += _C;
      D += _D;
      N += 1;
      V += _C * _Z;
      X += _D * _Z;
      Z += _Z;
    }

  // naplnime matici
  M[0][0] = A;
  M[0][1] = B;
  M[0][2] = C;
  M[0][DIM] = V;
  M[1][0] = B;
  M[1][1] = C;
  M[1][2] = D;
  M[1][DIM] = X;
  M[2][0] = C;
  M[2][1] = D;
  M[2][2] = N;
  M[2][DIM] = Z;

  // udelame gaussovu eliminaci
  for (a = 0; a < (DIM - 1); a++)	// faze 1. - anulovat cleny pod diagonalou
    for (b = a + 1; b < DIM; b++)
      {				// takze tady odecitam od b. radku a. radek, podil v a. sloupci
	Q = -M[a][a] / M[b][a];
	for (c = 0; c < (DIM + 1); c++)
	  M[b][c] = Q * M[b][c] + M[a][c];
      }
  for (a = 0; a < (DIM - 1); a++)	// faze 2. - anulovat cleny nad diagonalou
    for (b = a + 1; b < DIM; b++)
      {				// tady odecitam od a. radku b. radek - podil clenu v b. sloupci
	Q = -M[b][b] / M[a][b];
	for (c = 0; c < (DIM + 1); c++)
	  M[a][c] = Q * M[a][c] + M[b][c];
      }

  *rA = M[0][DIM] / M[0][0];
  *rB = M[1][DIM] / M[1][1];
  *rC = M[2][DIM] / M[2][2];
}
