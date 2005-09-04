#ifndef __RTS2_TELD_CPP__
#define __RTS2_TELD_CPP__

#include <libnova/libnova.h>
#include <sys/time.h>
#include <time.h>

#include "../utils/rts2block.h"
#include "../utils/rts2device.h"

// types of reset
// acquired from 

typedef enum
{ RESET_RESTART, RESET_WARM_START, RESET_COLD_START }
resetStates;

class Rts2DevTelescope:public Rts2Device
{
private:
  Rts2Conn * move_connection;
  int moveInfoCount;
  int moveInfoMax;
  int moveMark;
  int numCorr;
  int maxCorrNum;

  double locCorRa;
  double locCorDec;
  int locCorNum;

  // last errors
  double raCorr;
  double decCorr;
  double posErr;

  int knowPosition;
  double lastRa;
  double lastDec;
  struct ln_equ_posn lastTar;
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
  double searchRadius;
  double searchSpeed;		// in multiply of HA speed..eg 1 == 15 arcsec / sec
  virtual int isMovingFixed ()
  {
    return isMoving ();
  }
  virtual int isMoving ()
  {
    return -2;
  }
  virtual int isSearching ()
  {
    return -2;
  }
  virtual int isParking ()
  {
    return -2;
  }
  int move_fixed;
  virtual int processOption (int in_opt);
  virtual void cancelPriorityOperations ()
  {
    if ((getState (0) & TEL_MASK_SEARCHING) == TEL_SEARCH)
      {
	stopSearch ();
      }
    else
      {
	stopMove ();
      }
    clearStatesPriority ();
    Rts2Device::cancelPriorityOperations ();
  }
  resetStates nextReset;

  int getNumCorr ()
  {
    return numCorr;
  }
  // returns 0 when we need to 
  int setTarget (double tar_ra, double tar_dec);
  void unsetTarget ()
  {
    lastTar.ra = -1000;
    lastTar.dec = -1000;
  }
  double getMoveTargetSep ();
  double get_loc_sid_time ();
public:
  Rts2DevTelescope (int argc, char **argv);
  virtual int init ();
  virtual Rts2DevConn *createConnection (int in_sock, int conn_num);
  int checkMoves ();
  virtual int idle ();
  virtual int changeMasterState (int new_state);
  virtual int deleteConnection (Rts2Conn * in_conn)
  {
    if (in_conn == move_connection)
      move_connection = NULL;
    return Rts2Device::deleteConnection (in_conn);
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
  virtual int stopMove ();
  virtual int startMoveFixed (double tar_ha, double tar_dec)
  {
    return -1;
  }
  virtual int endMoveFixed ()
  {
    return -1;
  }
  virtual int startSearch ()
  {
    return -1;
  }
  virtual int stopSearch ();
  virtual int endSearch ();
  virtual int setTo (double set_ra, double set_dec)
  {
    return -1;
  }
  // isued for first correction which will be sended while telescope
  // moves to another target; can be used to set fixed offsets
  virtual int correctOffsets (double cor_ra, double cor_dec, double real_ra,
			      double real_dec)
  {
    return -1;
  }
  virtual int correct (double cor_ra, double cor_dec, double real_ra,
		       double real_dec)
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
  virtual int resetMount (resetStates reset_state)
  {
    nextReset = reset_state;
    return 0;
  }

  virtual int startDir (char *dir)
  {
    return -1;
  }
  virtual int stopDir (char *dir)
  {
    return -1;
  }

  // callback functions from telescope connection
  virtual int ready (Rts2Conn * conn);
  virtual int sendInfo (Rts2Conn * conn);
  virtual int sendBaseInfo (Rts2Conn * conn);

  int startMove (Rts2Conn * conn, double tar_ra, double tar_dec);
  int startMoveFixed (Rts2Conn * conn, double tar_ha, double tar_dec);
  int startSearch (Rts2Conn * conn, double radius, double in_searchSpeed);
  int startResyncMove (Rts2Conn * conn, double tar_ra, double tar_dec);
  int setTo (Rts2Conn * conn, double set_ra, double set_dec);
  int correct (Rts2Conn * conn, int cor_mark, double cor_ra, double cor_dec,
	       double real_ra, double real_dec);
  int startPark (Rts2Conn * conn);
  int change (Rts2Conn * conn, double chng_ra, double chng_dec);
  int saveModel (Rts2Conn * conn);
  int loadModel (Rts2Conn * conn);
  int stopWorm (Rts2Conn * conn);
  int startWorm (Rts2Conn * conn);
  int resetMount (Rts2Conn * conn, resetStates reset_state);
  virtual int grantPriority (Rts2Conn * conn);
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
