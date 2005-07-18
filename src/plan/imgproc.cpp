#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include "../utilsdb/rts2devicedb.h"
#include "status.h"
#include "rts2connimgprocess.h"

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

  virtual int sendInfo (Rts2Conn * conn);

  virtual int deleteConnection (Rts2Conn * conn);

  int queImage (Rts2Conn * conn, const char *in_path);
  int doImage (Rts2Conn * conn, const char *in_path);
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
}

Rts2ImageProc::~Rts2ImageProc (void)
{
  if (runningImage)
    delete runningImage;
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
Rts2ImageProc::sendInfo (Rts2Conn * conn)
{
  conn->sendValue ("que_size",
		   (int) imagesQue.size () + (runningImage ? 1 : 0));
  conn->sendValue ("good_images", goodImages);
  conn->sendValue ("trash_images", trashImages);
  conn->sendValue ("morning_images", morningImages);
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
  Rts2ConnImgProcess *newImage;
  newImage = new Rts2ConnImgProcess (this, conn, defaultImgProccess, in_path);
  if (runningImage)
    {
      imagesQue.push_back (newImage);
      sendInfo (conn);
      return 0;
    }
  changeRunning (newImage);
  infoAll ();
  return 0;
}

int
Rts2ImageProc::doImage (Rts2Conn * conn, const char *in_path)
{
  Rts2ConnImgProcess *newImage;
  newImage = new Rts2ConnImgProcess (this, conn, defaultImgProccess, in_path);
  changeRunning (newImage);
  infoAll ();
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
