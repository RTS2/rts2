#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include "rts2connimgprocess.h"
#include "../utils/rts2command.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>

Rts2ConnImgProcess::Rts2ConnImgProcess (Rts2Block * in_master,
					Rts2Conn * in_conn,
					const char *in_exe,
					const char *in_path):
Rts2ConnFork (in_master, in_exe)
{
  path = new char[strlen (in_path) + 1];
  strcpy (path, in_path);
  reqConn = in_conn;
  image = new Rts2ImageDb (path);
  astrometryStat = NOT_ASTROMETRY;
}

Rts2ConnImgProcess::~Rts2ConnImgProcess (void)
{
  if (astrometryStat == NOT_ASTROMETRY)
    {
      image->toTrash ();
    }
  delete[]path;
  delete image;
  sock = -1;
}

int
Rts2ConnImgProcess::newProcess ()
{
  int ret;
  syslog (LOG_DEBUG, "Rts2ConnImgProcess::newProcess exe: %s img: %s",
	  exePath, path);
  if (exePath)
    {
      ret = execl (exePath, exePath, path, (char *) NULL);
      if (ret)
	syslog (LOG_ERR, "Rts2ConnImgProcess::newProcess: %m");
    }
  return -2;
}

int
Rts2ConnImgProcess::processLine ()
{
  int ret;
  long id;
  double ra, dec, ra_err, dec_err;
  const char *telescopeName;
  int corr_mark;
  ret =
    sscanf (getCommand (), "%li %lf %lf (%lf,%lf)", &id, &ra, &dec, &ra_err,
	    &dec_err);
  syslog (LOG_DEBUG, "receive: %s sscanf: %i", getCommand (), ret);
  if (ret == 5 && reqConn)
    {
      reqConn->sendValue ("correct", image->getObsId (), image->getImgId (),
			  ra, dec, ra_err, dec_err);
      image->toArchive ();
      astrometryStat = GET;
      // send correction to telescope..
      telescopeName = image->getMountName ();
      ret = image->getValue ("MNT_MARK", corr_mark);
      if (ret)
	return -1;
      if (telescopeName)
	{
	  Rts2Conn *telConn;
	  telConn = master->findName (telescopeName);
	  // correction error should be in degrees
	  if (telConn)
	    telConn->
	      queCommand (new
			  Rts2CommandCorrect (master, corr_mark, ra, dec,
					      ra_err / 60.0, dec_err / 60.0));
	}
    }
  return -1;
}
