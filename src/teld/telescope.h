#ifndef __RTS2_TELD_CPP__
#define __RTS2_TELD_CPP__

#include <sys/time.h>
#include <time.h>

#include "../utils/rts2block.h"
#include "../utils/rts2device.h"

class Rts2DevTelescope:public Rts2Device
{
private:
  Rts2Conn * move_connection;
  int moveMark;
  int numCorr;
  int maxCorrNum;
protected:
  char *device_file;
  char telType[64];
  char telSerialNumber[64];
  double telRa;
  double telDec;
  double telSiderealTime;
  double telLocalTime;
  int telFlip;
  double telAxis[2];
  double telLongtitude;
  double telLatitude;
  double telAltitude;
  double telParkDec;
  virtual int isMovingFixed ()
  {
    return isMoving ();
  }
  virtual int isMoving ()
  {
    return -2;
  }
  virtual int isParking ()
  {
    return -2;
  }
  int move_fixed;
public:
  Rts2DevTelescope (int argc, char **argv);
  virtual int processOption (int in_opt);
  virtual int init ();
  virtual Rts2Conn *createConnection (int in_sock, int conn_num);
  int checkMoves ();
  virtual int idle ();
  virtual int changeMasterState (int new_state);
  virtual void deleteConnection (Rts2Conn * in_conn)
  {
    if (in_conn == move_connection)
      move_connection = NULL;
  }

  // callback functions for Camera alone
  virtual int ready ()
  {
    return -1;
  }
  virtual int info ()
  {
    return -1;
  }
  virtual int baseInfo ()
  {
    return -1;
  }
  virtual int startMove (double tar_ra, double tar_dec)
  {
    return -1;
  }
  virtual int endMove ()
  {
    return -1;
  }
  virtual int startMoveFixed (double tar_ha, double tar_dec)
  {
    return -1;
  }
  virtual int endMoveFixed ()
  {
    return -1;
  }
  virtual int setTo (double set_ra, double set_dec)
  {
    return -1;
  }
  virtual int correct (double cor_ra, double cor_dec)
  {
    return -1;
  }
  virtual int startPark ()
  {
    return -1;
  }
  virtual int endPark ()
  {
    return -1;
  }
  virtual int change (double chng_ra, double chng_dec)
  {
    return -1;
  }
  virtual int saveModel ()
  {
    return -1;
  }
  virtual int loadModel ()
  {
    return -1;
  }
  virtual int stopWorm ()
  {
    return -1;
  }
  virtual int startWorm ()
  {
    return -1;
  }

  // callback functions from telescope connection
  int ready (Rts2Conn * conn);
  int info (Rts2Conn * conn);
  int baseInfo (Rts2Conn * conn);

  int startMove (Rts2Conn * conn, double tar_ra, double tar_dec);
  int startMoveFixed (Rts2Conn * conn, double tar_ha, double tar_dec);
  int setTo (Rts2Conn * conn, double set_ra, double set_dec);
  int correct (Rts2Conn * conn, int cor_mark, double cor_ra, double cor_dec);
  int startPark (Rts2Conn * conn);
  int change (Rts2Conn * conn, double chng_ra, double chng_dec);
  int saveModel (Rts2Conn * conn);
  int loadModel (Rts2Conn * conn);
  int stopWorm (Rts2Conn * conn);
  int startWorm (Rts2Conn * conn);
};

class Rts2DevConnTelescope:public Rts2DevConn
{
private:
  Rts2DevTelescope * master;
protected:
  virtual int commandAuthorized ();
public:
    Rts2DevConnTelescope (int in_sock, Rts2DevTelescope * in_master_device);
};

#endif /* !__RTS2_TELD_CPP__ */
