#ifndef __RTS2CONNIMGPROCESS__
#define __RTS2CONNIMGPROCESS__

#include "../utils/rts2block.h"
#include "../writers/rts2image.h"
#include "../writers/rts2imagedb.h"

typedef enum
{ NOT_ASTROMETRY, TRASH, GET, MORNING } astrometry_stat_t;

/*
 * "Connection" which reads output of image processor
 *
 * This class expect that images are stored in CENTRAL repository,
 * accesible throught NFS/other network sharing to all machines on
 * which imgproc runs.
 *
 * Hence passing full image path will be sufficient for finding
 * it.
 */
class Rts2ConnImgProcess:public Rts2Conn
{
  char *path;
  Rts2Conn *reqConn;

  pid_t imgproc_pid;
  Rts2Image *image;

  astrometry_stat_t astrometryStat;
protected:
    virtual int send (char *msg);

public:
    Rts2ConnImgProcess (Rts2Block * in_master, Rts2Conn * in_conn,
			const char *in_path);
    virtual ~ Rts2ConnImgProcess (void);

  virtual int idle ();
  virtual int processLine ();

  void deleteConnection (Rts2Conn * conn)
  {
    if (conn == reqConn)
      reqConn = NULL;
  }
  int run ();
  void stop ();
  virtual void childReturned (pid_t child_pid)
  {
    if (child_pid == imgproc_pid)
      endConnection ();
  }
  astrometry_stat_t getAstrometryStat ()
  {
    return astrometryStat;
  }
};

#endif /* !__RTS2CONNIMGPROCESS__ */
