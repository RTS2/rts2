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
  reqConn = in_conn;
  imgPath = new char[strlen (in_path) + 1];
  strcpy (imgPath, in_path);
  astrometryStat = NOT_ASTROMETRY;
}

Rts2ConnImgProcess::~Rts2ConnImgProcess (void)
{
  delete[]imgPath;
}

int
Rts2ConnImgProcess::newProcess ()
{
  int ret;
  Rts2Image *image;

  syslog (LOG_DEBUG, "Rts2ConnImgProcess::newProcess exe: %s img: %s (%i)",
	  exePath, imgPath, getpid ());

  image = new Rts2Image (imgPath);
  if (image->getType () == IMGTYPE_DARK)
    {
      astrometryStat = DARK;
      delete image;
      return 0;
    }
  delete image;

  if (exePath)
    {
      ret = execl (exePath, exePath, imgPath, (char *) NULL);
      if (ret)
	syslog (LOG_ERR, "Rts2ConnImgProcess::newProcess: %m");
    }
  return -2;
}

int
Rts2ConnImgProcess::processLine ()
{
  int ret;
  ret =
    sscanf (getCommand (), "%li %lf %lf (%lf,%lf)", &id, &ra, &dec, &ra_err,
	    &dec_err);
  syslog (LOG_DEBUG, "receive: %s sscanf: %i", getCommand (), ret);
  if (ret == 5)
    {
      astrometryStat = GET;
    }
  return -1;
}

int
Rts2ConnImgProcess::connectionError ()
{
  int ret;
  const char *telescopeName;
  int corr_mark;
  Rts2ImageDb *image;

  syslog (LOG_DEBUG, "Rts2ConnImgProcess::connectionError %m");

  image = new Rts2ImageDb (imgPath);
  switch (astrometryStat)
    {
    case NOT_ASTROMETRY:
    case TRASH:
      astrometryStat = TRASH;
      image->toTrash ();
      break;
    case GET:
      if (reqConn)
	reqConn->sendValue ("correct", image->getObsId (), image->getImgId (),
			    ra, dec, ra_err, dec_err);
      image->toArchive ();
      // send correction to telescope..
      telescopeName = image->getMountName ();
      ret = image->getValue ("MNT_MARK", corr_mark);
      if (ret)
	break;
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
      break;
    case DARK:
      image->toDark ();
      break;
    }
  delete image;
  return Rts2ConnFork::connectionError ();
}
