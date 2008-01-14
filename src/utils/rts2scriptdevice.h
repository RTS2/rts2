#ifndef __RTS2_SCRIPTDEVICE__
#define __RTS2_SCRIPTDEVICE__

#include "rts2device.h"

class Rts2ScriptDevice:public Rts2Device
{
	private:
		Rts2ValueInteger * scriptRepCount;
		Rts2ValueString *runningScript;
		Rts2ValueString *scriptComment;
		Rts2ValueInteger *scriptNumber;
		Rts2ValueInteger *scriptPosition;
		Rts2ValueInteger *scriptLen;
	protected:
		virtual int setValue (Rts2Value * old_value, Rts2Value * new_value);
	public:
		Rts2ScriptDevice (int in_argc, char **in_argv, int in_device_type,
			char *default_name);
};
#endif							 /* !__RTS2_SCRIPTDEVICE__ */
