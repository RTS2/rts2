#ifndef __RTS2_SENSOR_GPIB__
#define __RTS2_SENSOR_GPIB__

#include "sensord.h"

#include <gpib/ib.h>

class Rts2DevSensorGpib:public Rts2DevSensor
{
	private:
		int minor;
		int pad;

		int gpib_dev;
	protected:
		int gpibWrite (const char *buf);
		int gpibRead (void *buf, int blen);
		int gpibWriteRead (char *buf, char *val, int blen = 50);

		int gpibWaitSRQ ();

		virtual int processOption (int in_opt);
		virtual int init ();

		void setPad (int in_pad)
		{
			pad = in_pad;
		}
	public:
		Rts2DevSensorGpib (int in_argc, char **in_argv);
		virtual ~ Rts2DevSensorGpib (void);
};
#endif							 /* !__RTS2_SENSOR_GPIB__ */
