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
					const char *in_path):
Rts2Conn (in_master)
{
  path = new char[strlen (in_path) + 1];
  strcpy (path, in_path);
  reqConn = in_conn;
  image = new Rts2ImageDb (path);
  imgproc_pid = 0;
  astrometryStat = NOT_ASTROMETRY;
}

Rts2ConnImgProcess::~Rts2ConnImgProcess (void)
{
  if (astrometryStat == TRASH || astrometryStat == NOT_ASTROMETRY)
    {
      image->toTrash ();
    }
  delete[]path;
  delete image;
  if (imgproc_pid)
    kill (SIGINT, imgproc_pid);
  sock = -1;
}

int
Rts2ConnImgProcess::send (char *msg)
{
  // we don't send anything to imgproc connection!      
  return 1;
}

int
Rts2ConnImgProcess::idle ()
{
  // do not call idle - it will end us :(
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

int
Rts2ConnImgProcess::run ()
{
  int ret;
  if (imgproc_pid)
    {
      // continue
      kill (imgproc_pid, SIGCONT);
      return 1;
    }
  int filedes[2];
  ret = pipe (filedes);
  if (ret)
    {
      syslog (LOG_ERR,
	      "Rts2ConnImgProcess::run cannot create pipe for process: %m");
      return -1;
    }
  imgproc_pid = fork ();
  if (imgproc_pid == -1)
    {
      syslog (LOG_ERR, "Rts2ConnImgProcess::run cannot fork: %m");
      return -1;
    }
  else if (imgproc_pid)		// parent
    {
      sock = filedes[0];
      close (filedes[1]);
      fcntl (sock, F_SETFL, O_NONBLOCK);
      return 0;
    }
  // child
  close (filedes[0]);
  dup2 (filedes[1], 1);
  ret =
    execl ("/etc/rts2/img_process", "/etc/rts2/img_process", path,
	   (char *) NULL);
}

void
Rts2ConnImgProcess::stop ()
{
  if (imgproc_pid)
    kill (imgproc_pid, SIGSTOP);
}
