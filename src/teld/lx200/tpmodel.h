#ifndef TPMODEL_H
#define TPMODEL_H

#include <libnova.h>

int tpoint_correction (struct ln_equ_posn *mean_pos,	/* mean pos of the object to go */
		       struct ln_equ_posn *proper_motion,	/* proper motion of the object: may be NULL */
		       struct ln_lnlat_posn *obs,	/* observer location. if NULL: no refraction */
		       double JD,	/* time */
		       struct ln_equ_posn *tel_pos	/* return: coords corrected for the model */
  );

void tpoint_apply_now (double *ra, double *dec);

#endif /* TPMODEL_H */
