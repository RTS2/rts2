#ifndef __RTS2_FOCUSING__
#define __RTS2_FOCUSING__

#ifdef __cplusplus
extern "C"
{
#endif

  extern int write_fits (char *filename, long exposure, int w, int h,
			 unsigned short *data);
  extern void getmeandisp (unsigned short *img, int len, double *mean,
			   double *disp);
  extern void regr_q (double *vX, double *vZ, long i, double *rA, double *rB,
		      double *rC);

#ifdef __cplusplus
};
#endif

#endif /* !__RTS2_FOCUSING__ */
