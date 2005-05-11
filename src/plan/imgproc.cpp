#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include "../utils/rts2device.h"
#include "status.h"

#include <iostreams>
#include <stdio.h>

/*
 * This class expect that images are stored in CENTRAL repository,
 * accesible throught NFS/other network sharing to all machines on
 * which imgproc runs.
 *
 * Hence passing full image path will be sufficient for finding it.
 */
class Image
{

}

/*
 * "Connection" which reads output of image processor
 */
class Rts2ConnImgProcess:public Rts2Conn
{
  FILE *pfile;
  Rts2Conn *reqConn;

public:
    Rts2ConnImgProcess (Rts2Block * in_master, Rts2Conn * reqConn);
    virtual ~ Rts2ConnImgProcess (void);

  virtual int receive (fd_set * set);
}

class Rts2DevConnImage:public Rts2DevConn
{
protected:
  virtual int commandAuthorized ();
public:
    Rts2DevConnImage (int in_sock, Rts2Block * in_master);
};

class Rts2ImageProc:public Rts2Device
{
private:
  std::vector < ImageQue * >imagesQue;
public:
  Rts2ImageProc (int argc, char **argv);
  virtual Rts2Conn *createConnection (int in_sock, int conn_num);

  virtual int ready ()
  {
    return 0;
  }

  virtual int info ();
  virtual int baseInfo ();

  virtual int sendInfo (Rts2Conn * conn);
};

Rts2ConnImgProcess::Rts2ConnImgProcess (Rts2Block * in_master):Rts2Conn
  (in_master)
{
  pfile = popen ();
  if (!pfile)
    return;
  sock = fileno (pfile);
}

Rts2ConnImgProcess::~Rts2ConnImgProcess (void)
{
  fclose (pfile);
  sock = -1;
}

int
Rts2ConnImgProcess::receive (fd_set * set)
{
  if (conn_state == CONN_DELETE)
    return -1;
  if ((sock >= 0) && FD_ISSET (sock, set))
    {
      int ret;
      long id;
      double ra, dec, ra_err, dec_err;
      ret =
	fscanf (pfile, "%li %lf %lf (%lf,%lf)", &id, &ra, &dec, &ra_err,
		&dec_err);
      if (ret == EOF)
	{
	  conn_state = CONN_DELETE;
	  return -1;
	}
      if (ret == 5 & reqConn)
	{
	  reqConn->sendValue ("img_ra", ra);
	  reqConn->sendValue ("img_dec", dec);
	  reqConn->sendValue ("img_ra_err", ra_err);
	  reqConn->sendValue ("img_dec_err", dec_err);
	}
      return 1;
    }
}

Rts2DevConnImage::Rts2DevConnImage (int in_sock, Rts2Block * in_master):Rts2DevConn (in_sock,
	     in_master)
{
}

Rts2ImageProc::Rts2ImageProc (int argc, char **argv):
Rts2Device (argc, argv, DEVICE_TYPE_IMGPROC, 5561, "IMGP")
{
}

Rts2Conn *
Rts2ImageProc::createConnection (int in_sock, int conn_num)
{
  return new Rts2ConnImage (in_sock);
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
  // sendValue ( // send size of que
}

Rts2ImageProc *imgproc;

int
main (int argc, char **argv)
{
  int ret;
  imgproc = new (argc, argv);
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
