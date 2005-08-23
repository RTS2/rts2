#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include "../utilsdb/rts2devicedb.h"
#include "status.h"
#include "rts2connimgprocess.h"

#include <glob.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include <unistd.h>
#include <iostream>
#include <stdio.h>

class Rts2ImageProc;

class Rts2DevConnImage:public Rts2DevConn
{
private:
  Rts2ImageProc * master;
protected:
  virtual int commandAuthorized ();
public:
    Rts2DevConnImage (int in_sock, Rts2ImageProc * in_master);
};

class Rts2ImageProc:public Rts2DeviceDb
{
private:
  std::list < Rts2ConnImgProcess * >imagesQue;
  Rts2ConnImgProcess *runningImage;
  int goodImages;
  int trashImages;
  int morningImages;
  int sendStop;			// if stop running astrometry with stop signal; it ussually doesn't work, so we will use FIFO
  char defaultImgProccess[2000];
  glob_t imageGlob;
  int globC;
  int reprocessingPossible;
public:
    Rts2ImageProc (int argc, char **argv);
    virtual ~ Rts2ImageProc (void);
  virtual Rts2DevConn *createConnection (int in_sock, int conn_num);

  virtual int init ();
  virtual int idle ();
  virtual int ready ()
  {
    return 0;
  }

  virtual int info ();
  virtual int baseInfo ();

  virtual int sendBaseInfo (Rts2Conn * conn);
  virtual int sendInfo (Rts2Conn * conn);

  virtual int changeMasterState (int new_state);

  virtual int deleteConnection (Rts2Conn * conn);

  int queImage (Rts2Conn * conn, const char *in_path);
  int doImage (Rts2Conn * conn, const char *in_path);
  int checkNotProcessed ();
  void changeRunning (Rts2ConnImgProcess * newImage);
};

Rts2DevConnImage::Rts2DevConnImage (int in_sock, Rts2ImageProc * in_master):
Rts2DevConn (in_sock, in_master)
{
  master = in_master;
}

int
Rts2DevConnImage::commandAuthorized ()
{
  if (isCommand ("que_image"))
    {
      char *in_imageName;
      if (paramNextString (&in_imageName) || !paramEnd ())
	return -2;
      return master->queImage (this, in_imageName);
    }
  if (isCommand ("do_image"))
    {
      char *in_imageName;
      if (paramNextString (&in_imageName) || !paramEnd ())
	return -2;
      return master->doImage (this, in_imageName);
    }
  return Rts2DevConn::commandAuthorized ();
}

Rts2ImageProc::Rts2ImageProc (int argc, char **argv):Rts2DeviceDb (argc, argv, DEVICE_TYPE_IMGPROC, 5561,
	      "IMGP")
{
  runningImage = NULL;

  goodImages = 0;
  trashImages = 0;
  morningImages = 0;

  imageGlob.gl_pathc = 0;
  imageGlob.gl_offs = 0;
  globC = 0;
  reprocessingPossible = 0;
}

Rts2ImageProc::~Rts2ImageProc (void)
{
  if (runningImage)
    delete runningImage;
  if (imageGlob.gl_pathc)
    globfree (&imageGlob);
}

Rts2DevConn *
Rts2ImageProc::createConnection (int in_sock, int conn_num)
{
  return new Rts2DevConnImage (in_sock, this);
}

int
Rts2ImageProc::init ()
{
  int ret;
  ret = Rts2DeviceDb::init ();
  if (ret)
    return ret;

  Rts2Config *config;
  config = Rts2Config::instance ();

  ret = config->getString ("imgproc", "astrometry", defaultImgProccess, 2000);

  if (ret)
    {
      syslog (LOG_ERR,
	      "Rts2ImageProc::init cannot get astrometry string, exiting!");
    }

  sendStop = 0;

  return ret;
}

int
Rts2ImageProc::idle ()
{
  std::list < Rts2ConnImgProcess * >::iterator img_iter;
  if (!runningImage && imagesQue.size () != 0)
    {
      img_iter = imagesQue.begin ();
      Rts2ConnImgProcess *newImage = *img_iter;
      imagesQue.erase (img_iter);
      changeRunning (newImage);
    }
  return Rts2Device::idle ();
}

int
Rts2ImageProc::info ()
{
  return 0;
}

int
Rts2ImageProc::baseInfo ()
{
  return 0;
}

int
Rts2ImageProc::sendBaseInfo (Rts2Conn * conn)
{
  return 0;
}

int
Rts2ImageProc::sendInfo (Rts2Conn * conn)
{
  conn->sendValue ("que_size",
		   (int) imagesQue.size () + (runningImage ? 1 : 0));
  conn->sendValue ("good_images", goodImages);
  conn->sendValue ("trash_images", trashImages);
  conn->sendValue ("morning_images", morningImages);
}

int
Rts2ImageProc::changeMasterState (int new_state)
{
  switch (new_state)
    {
    case SERVERD_DUSK:
    case SERVERD_DUSK | SERVERD_STANDBY_MASK:
    case SERVERD_NIGHT:
    case SERVERD_NIGHT | SERVERD_STANDBY_MASK:
    case SERVERD_DAWN:
    case SERVERD_DAWN | SERVERD_STANDBY_MASK:
      if (imageGlob.gl_pathc)
	{
	  globfree (&imageGlob);
	  imageGlob.gl_pathc = 0;
	  globC = 0;
	}
      reprocessingPossible = 0;
      break;
    default:
      reprocessingPossible = 1;
      if (!runningImage && imagesQue.size () == 0)
	checkNotProcessed ();
    }
}

int
Rts2ImageProc::deleteConnection (Rts2Conn * conn)
{
  std::list < Rts2ConnImgProcess * >::iterator img_iter;
  for (img_iter = imagesQue.begin (); img_iter != imagesQue.end ();
       img_iter++)
    {
      (*img_iter)->deleteConnection (conn);
      if (*img_iter == conn)
	{
	  imagesQue.erase (img_iter);
	}
    }
  if (runningImage)
    runningImage->deleteConnection (conn);
  if (conn == runningImage)
    {
      // que next image
      // Rts2Device::deleteConnection will delete runningImage
      switch (runningImage->getAstrometryStat ())
	{
	case GET:
	  goodImages++;
	  break;
	case TRASH:
	  trashImages++;
	  break;
	case MORNING:
	  morningImages++;
	  break;
	}
      runningImage = NULL;
      img_iter = imagesQue.begin ();
      if (img_iter != imagesQue.end ())
	{
	  imagesQue.erase (img_iter);
	  changeRunning (*img_iter);
	}
      else if (reprocessingPossible)
	{
	  if (globC < imageGlob.gl_pathc)
	    {
	      queImage (NULL, imageGlob.gl_pathv[globC]);
	      globC++;
	    }
	  else if (imageGlob.gl_pathc > 0)
	    {
	      globfree (&imageGlob);
	      imageGlob.gl_pathc = 0;
	    }
	}
    }
  return Rts2DeviceDb::deleteConnection (conn);
}

void
Rts2ImageProc::changeRunning (Rts2ConnImgProcess * newImage)
{
  int ret;
  if (runningImage)
    {
      if (sendStop)
	{
	  runningImage->stop ();
	  imagesQue.push_front (runningImage);
	}
      else
	{
	  imagesQue.push_front (newImage);
	  return;
	}
    }
  runningImage = newImage;
  ret = runningImage->init ();
  if (ret < 0)
    {
      delete runningImage;
      runningImage = NULL;
      return;
    }
  else if (ret == 0)
    {
      addConnection (runningImage);
    }
  infoAll ();
}

int
Rts2ImageProc::queImage (Rts2Conn * conn, const char *in_path)
{
  Rts2ConnImgProcess *newImageConn;
  newImageConn =
    new Rts2ConnImgProcess (this, conn, defaultImgProccess, in_path);
  if (runningImage)
    {
      imagesQue.push_back (newImageConn);
      if (conn)
	sendInfo (conn);
      return 0;
    }
  changeRunning (newImageConn);
  infoAll ();
  return 0;
}

int
Rts2ImageProc::doImage (Rts2Conn * conn, const char *in_path)
{
  Rts2ConnImgProcess *newImageConn;
  newImageConn =
    new Rts2ConnImgProcess (this, conn, defaultImgProccess, in_path);
  changeRunning (newImageConn);
  infoAll ();
  return 0;
}

int
Rts2ImageProc::checkNotProcessed ()
{
  char image_glob[250];
  int ret;

  Rts2Config *config;
  config = Rts2Config::instance ();

  ret = config->getString ("imgproc", "imageglob", image_glob, 250);
  if (ret)
    return ret;

  ret = glob (image_glob, 0, NULL, &imageGlob);
  if (ret)
    {
      globfree (&imageGlob);
      imageGlob.gl_pathc = 0;
      return -1;
    }

  globC = 0;

  // start files que..
  if (imageGlob.gl_pathc > 0)
    return queImage (NULL, imageGlob.gl_pathv[0]);
  return 0;
}

Rts2ImageProc *imgproc;

void
killSignal (int sig)
{
  if (imgproc)
    delete imgproc;
  exit (0);
}

int
main (int argc, char **argv)
{
  int ret;
  imgproc = new Rts2ImageProc (argc, argv);

  signal (SIGTERM, killSignal);
  signal (SIGINT, killSignal);

  ret = imgproc->init ();
  if (ret)
    {
      std::cerr << "cannot initialize image processor, exiting" << std::endl;
      exit (1);
    }
  imgproc->run ();
  delete imgproc;
  return 0;
}
