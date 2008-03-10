#ifndef __RTS2_GRBEXECCONN__
#define __RTS2_GRBEXECCONN__

#include "../utils/rts2connfork.h"

// executes GRB programm..
class Rts2GrbExecConn:public Rts2ConnFork
{
	private:
		char **argvs;
	public:
		Rts2GrbExecConn (Rts2Block * in_master, char *execFile, int in_tar_id,
			int in_grb_id, int in_grb_seqn, int in_grb_type,
			double in_grb_ra, double in_grb_dec, bool in_grb_is_grb,
			time_t * in_grb_date, float in_grb_errorbox, int grb_isnew);
		virtual ~ Rts2GrbExecConn (void);

		virtual int newProcess ();
};
#endif							 /* !__RTS2_GRBEXECCONN__ */
