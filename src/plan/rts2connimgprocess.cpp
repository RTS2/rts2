#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include "rts2connimgprocess.h"
#include "../utils/rts2command.h"
#include "../utilsdb/rts2taruser.h"
#include "../utilsdb/rts2obs.h"

#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>

#include <sstream>

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

#ifdef DEBUG_EXTRA
  syslog (LOG_DEBUG, "Rts2ConnImgProcess::newProcess exe: %s img: %s (%i)",
	  exePath, imgPath, getpid ());
#endif

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
#ifdef DEBUG_EXTRA
  syslog (LOG_DEBUG, "receive: %s sscanf: %i", getCommand (), ret);
#endif
  if (ret == 5)
    {
      astrometryStat = GET;
      // inform others..
    }
  return -1;
}

int
Rts2ConnImgProcess::connectionError (int last_data_size)
{
  int ret;
  const char *telescopeName;
  int corr_mark;
  Rts2ImageDb *image;

  if (last_data_size < 0 && errno == EAGAIN)
    {
      syslog (LOG_DEBUG, "Rts2ConnImgProcess::connectionError %m");
      return 1;
    }

  image = new Rts2ImageDb (imgPath);
  switch (astrometryStat)
    {
    case NOT_ASTROMETRY:
    case TRASH:
      astrometryStat = TRASH;
      image->toTrash ();
      sendProcEndMail (image);
      break;
    case GET:
      if (reqConn)
	reqConn->sendValue ("correct", image->getObsId (), image->getImgId (),
			    ra, dec, ra_err, dec_err);
      image->setAstroResults (ra, dec, ra_err / 60.0, dec_err / 60.0);
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
      sendOKMail (image);
      sendProcEndMail (image);
      break;
    case DARK:
      image->toDark ();
      break;
    default:
      break;
    }
  if (astrometryStat == GET)
    master->postEvent (new Rts2Event (EVENT_OK_ASTROMETRY, (void *) image));
  else
    master->postEvent (new Rts2Event (EVENT_NOT_ASTROMETRY, (void *) image));
  delete image;
  return Rts2ConnFork::connectionError (last_data_size);
}

void
Rts2ConnImgProcess::sendOKMail (Rts2ImageDb * image)
{
  // is first such image..
  if (image->getOKCount () == 1)
    {
      Rts2TarUser tar_user =
	Rts2TarUser (image->getTargetId (), image->getTargetType ());
      std::string mails = tar_user.getUsers (SEND_ASTRO_OK);
      std::ostringstream subject;
      subject << "TARGET #"
	<< image->getTargetIdSel ()
	<< " (" << image->getTargetId ()
	<< ") GET ASTROMETRY (IMG_ID #" << image->getImgId () << ")";
      std::ostringstream os;
      os << image;
      sendMailTo (subject.str ().c_str (), os.str ().c_str (),
		  mails.c_str ());
    }
}

void
Rts2ConnImgProcess::sendProcEndMail (Rts2ImageDb * image)
{
  // last processed
  Rts2Obs observation = Rts2Obs (image->getObsId ());
  observation.checkUnprocessedImages ();
}
