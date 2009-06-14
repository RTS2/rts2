
#include "rts2grbexecconn.h"

#include <unistd.h>
#include "../utils/utilsfunc.h"

Rts2GrbExecConn::Rts2GrbExecConn (Rts2Block * in_master, char *execFile,
int in_tar_id, int in_grb_id,
int in_grb_seqn, int in_grb_type,
double in_grb_ra, double in_grb_dec,
bool in_grb_is_grb, time_t * in_grb_date,
float in_grb_errorbox, int in_grb_isnew):
rts2core::ConnFork (in_master, execFile, false)
{
	argvs = new char *[12];
	// defaults..
	argvs[0] = execFile;
	// pass real arguments..
	fillIn (&argvs[1], in_tar_id);
	fillIn (&argvs[2], in_grb_id);
	fillIn (&argvs[3], in_grb_seqn);
	fillIn (&argvs[4], in_grb_type);
	fillIn (&argvs[5], in_grb_ra);
	fillIn (&argvs[6], in_grb_dec);
	fillIn (&argvs[7], (in_grb_is_grb ? 1 : 0));
	fillIn (&argvs[8], *in_grb_date);
	fillIn (&argvs[9], in_grb_errorbox);
	fillIn (&argvs[10], in_grb_isnew);
	argvs[11] = NULL;
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
