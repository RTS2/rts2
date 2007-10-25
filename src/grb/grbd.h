#ifndef __RTS2_GRBD__
#define __RTS2_GRBD__

#include "../utilsdb/rts2devicedb.h"
#include "rts2conngrb.h"
#include "rts2grbfw.h"

// when we get GRB packet..
#define RTS2_EVENT_GRB_PACKET  RTS2_LOCAL_EVENT + 600

class Rts2ConnGrb;

class Rts2DevGrb:public Rts2DeviceDb
{
	private:
		Rts2ConnGrb * gcncnn;
		char *gcn_host;
		int gcn_port;
		int do_hete_test;

		int forwardPort;

		char *addExe;
		int execFollowups;

		Rts2ValueTime *last_packet;
		Rts2ValueDouble *delta;
		Rts2ValueString *last_target;
		Rts2ValueTime *last_target_time;
		Rts2ValueInteger *execConnection;

	protected:
		virtual int processOption (int in_opt);
		virtual int reloadConfig ();
	public:
		Rts2DevGrb (int argc, char **argv);
		virtual ~ Rts2DevGrb ();
		virtual int init ();

		virtual int ready ()
		{
			return 0;
		}
		virtual int info ();
		virtual void postEvent (Rts2Event * event);

		int newGcnGrb (int tar_id);

		virtual int commandAuthorized (Rts2Conn * conn);
};
#endif							 /* __RTS2_GRBD__ */
