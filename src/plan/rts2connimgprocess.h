#ifndef __RTS2CONNIMGPROCESS__
#define __RTS2CONNIMGPROCESS__

#include "../utils/rts2connfork.h"
#include "../writers/rts2imagedb.h"

#define EVENT_OK_ASTROMETRY	RTS2_LOCAL_EVENT + 200
#define EVENT_NOT_ASTROMETRY	RTS2_LOCAL_EVENT + 201

typedef enum
{ NOT_ASTROMETRY, TRASH, GET, MORNING, DARK, FLAT } astrometry_stat_t;

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
class Rts2ConnImgProcess:public Rts2ConnFork
{
private:
  Rts2Conn * reqConn;

  char *imgPath;

  astrometry_stat_t astrometryStat;
  long id;
  double ra, dec, ra_err, dec_err;

  void sendOKMail (Rts2ImageDb * image);
  void sendProcEndMail (Rts2ImageDb * image);

protected:
    virtual int connectionError (int last_data_size);

public:
    Rts2ConnImgProcess (Rts2Block * in_master, Rts2Conn * in_conn,
			const char *in_exe, const char *in_path);
    virtual ~ Rts2ConnImgProcess (void);

  virtual int newProcess ();
  virtual int processLine ();

  void deleteConnection (Rts2Conn * conn)
  {
    if (conn == reqConn)
      reqConn = NULL;
  }

  astrometry_stat_t getAstrometryStat ()
  {
    return astrometryStat;
  }
};

#endif /* !__RTS2CONNIMGPROCESS__ */
