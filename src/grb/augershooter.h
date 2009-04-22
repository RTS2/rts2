#ifndef __RTS2_AUGERSHOOTER__
#define __RTS2_AUGERSHOOTER__

#include "../utilsdb/rts2devicedb.h"
#include "connshooter.h"

#define RTS2_EVENT_AUGER_SHOWER   RTS2_LOCAL_EVENT + 700

namespace rts2too
{

class ConnShooter;

class AugerShooter:public Rts2DeviceDb
{
	private:
		ConnShooter * shootercnn;
		int port;
		Rts2ValueTime *lastAugerDate;
		Rts2ValueDouble *lastAugerRa;
		Rts2ValueDouble *lastAugerDec;
	protected:
		virtual int processOption (int in_opt);
	public:
		AugerShooter (int in_argc, char **in_argv);
		virtual ~ AugerShooter (void);

		virtual int ready ()
		{
			return 0;
		}

		virtual int init ();
		int newShower (double lastDate, double ra, double dec);
		bool wasSeen (double lastDate, double ra, double dec);
};

};
#endif							 /*! __RTS2_AUGERSHOOTER__ */
