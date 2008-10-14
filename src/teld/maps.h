#ifndef _MAPS_H
#define _MAPS_H

#include "telescope_info.h"

#define FILEPATH "/dev/tel"
#define STATFILE "/dev/tel/stat"
#define CTRLFILE "/dev/tel/ctrl"

enum
{
	TEL_POINT,
	// Sidereal mode, telescope points to an object. Is allowed to give commands,
	// if it somehow gets to a dangerous position, it will be
	// stopped, if it will need rehoming, will recieve it
	TEL_SLEWING,
	// Slewing to a position: after recieving position change. Cannot give telescope
	// commands apart of killing of the slew (then recycle and
	// slew from "point" state
	TEL_STOP,
	// Motors are off: before doing anything (if requested) is needed to switch them on
	TEL_HOMING,
	// one of tel axes does homing: nothing can we do until it reaches homing
	TEL_TIMEOUT,
	// Connection to telescope has not been reached: no commands until we'll
	// know what's up
	//    TEL_JOYSTICK// Somebody touches joystick at the telescope?
}


TEL_STATE;

struct T9_ctrl
{
	double ra;
	double dec;
	int power;
};

#define T9_stat telescope_info
/*struct T9_stat
{
  double ra, dec;
  long int c0, c1;
  TEL_STATE state;
};*/

extern struct T9_ctrl *Tctrl;
extern struct T9_stat *Tstat;
#endif							 /* _MAPS_H */
