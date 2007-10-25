// helper file for C++ interface
#ifndef __RTS2_EDTINTERFACE__
#define __RTS2_EDTINTERFACE__

// values taken from http://www.bcbsr.com/sao/lsstdes.html
#define SAO_GAIN_HIGH 0x50300000
#define SAO_GAIN_LOW  0x50200000

#define SAO_PARALER_SP  0x46000006

#define SAO_SPLIT_ON  0x41000001
#define SAO_SPLIT_OFF 0x41000000

#define SAO_UNI_ON  0x43000001
#define SAO_UNI_OFF 0x43000000

#ifdef __cplusplus
extern "C"
{
	#endif

	#include "edtswap.h"
	#include "edtinc.h"
	#include "edtreg.h"
	#include "sdvlib.h"

	// define functions which are needed and don't have prototype
	int ccd_serial_write (PdvDev * fd, u_char * value, int count);

	void ccd_setdev (int dev);
	PdvDev *ccd_gopen (char *dvname, int unit);

	void ccd_picture_timeout (PdvDev * fd, int timeout);
	int ccd_set_serial_delay (PdvDev * fd, int delay);

	int sao_print_command (u_int cptr[], int num);

	#ifdef __cplusplus
};
#endif
#endif							 /* !__RTS2_EDTINTERFACE__ */
