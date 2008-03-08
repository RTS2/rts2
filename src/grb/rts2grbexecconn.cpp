#include "rts2grbexecconn.h"

#include <unistd.h>

Rts2GrbExecConn::Rts2GrbExecConn (Rts2Block * in_master, char *execFile,
int in_tar_id, int in_grb_id,
int in_grb_seqn, int in_grb_type,
double in_grb_ra, double in_grb_dec,
bool in_grb_is_grb, time_t * in_grb_date,
float in_grb_errorbox):
Rts2ConnFork (in_master, execFile)
{
	argvs = new char *[11];
	// defaults..
	argvs[0] = execFile;
	// pass real arguments..
	asprintf (&argvs[1], "%i", in_tar_id);
	asprintf (&argvs[2], "%i", in_grb_id);
	asprintf (&argvs[3], "%i", in_grb_seqn);
	asprintf (&argvs[4], "%i", in_grb_type);
	asprintf (&argvs[5], "%f", in_grb_ra);
	asprintf (&argvs[6], "%f", in_grb_dec);
	asprintf (&argvs[7], "%i", (in_grb_is_grb ? 1 : 0));
	asprintf (&argvs[8], "%li", *in_grb_date);
	asprintf (&argvs[9], "%f", in_grb_errorbox);
	argvs[10] = NULL;
}


Rts2GrbExecConn::~Rts2GrbExecConn (void)
{
	for (int i = 1; i < 10; i++)
		free (argvs[i]);
	delete[]argvs;
}


int
Rts2GrbExecConn::newProcess ()
{
	if (exePath)
	{
		// if that execute, we will never call return..
		execv (exePath, argvs);
	}
	return -2;
}
