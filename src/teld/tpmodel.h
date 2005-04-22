#ifndef __RTS2_TPMODEL__
#define __RTS2_TPMODEL__

#include <libnova/libnova.h>

#ifdef __cplusplus
extern "C"
{
#endif

  double in180 (double x);
  double in360 (double x);

  int tpoint_correction (struct ln_equ_posn *mean_pos,
			 /* mean pos of the object to go              */
			 struct ln_equ_posn *proper_motion,
			 /* proper motion of the object: may be NULL  */
			 struct ln_lnlat_posn *obs,
			 /* observer location. if NULL: no refraction */
			 double JD,	/* time      */
			 double aux1, double aux2,	/* auxiliary reading */
			 struct ln_equ_posn *tel_pos,
			 /* return: coords corrected for the model */
			 int back	/* reverse the correction? */
    );

  void tpoint_apply_now (double *ra, double *dec, double aux1, double aux2,
			 int rev);

#ifdef __cplusplus
};
#endif

#endif /* __RTS2_TPMODEL__ */
