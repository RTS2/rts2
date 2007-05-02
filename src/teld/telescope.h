#ifndef __RTS2_TELD_CPP__
#define __RTS2_TELD_CPP__

#include <libnova/libnova.h>
#include <sys/time.h>
#include <time.h>

#include "../utils/rts2block.h"
#include "../utils/rts2device.h"

// types of corrections
#define COR_ABERATION	0x01
#define COR_PRECESSION	0x02
#define COR_REFRACTION	0x04
// if we will use model corrections..
#define COR_MODEL	0x08

// types of reset
// acquired from 

typedef enum
{ RESET_RESTART, RESET_WARM_START, RESET_COLD_START, RESET_INIT_START }
resetStates;

class Rts2TelModel;

class Rts2DevTelescope:public Rts2Device
{
private:
  Rts2Conn * move_connection;
  int moveInfoCount;
  int moveInfoMax;
  Rts2ValueInteger *moveMark;
  Rts2ValueInteger *numCorr;
  int maxCorrNum;

  double locCorRa;
  double locCorDec;
  int locCorNum;

  // last errors
  Rts2ValueDouble *raCorr;
  Rts2ValueDouble *decCorr;
  Rts2ValueDouble *posErr;

  Rts2ValueDouble *sepLimit;
  // if correction is greater than that limit, it will preform correction
  // immediately, independet of the camera exposures.  That is good to put NF
  // to position on WF-NF setups.
  Rts2ValueDouble *minGood;

  Rts2ValueBool *quickEnabled;

  Rts2ValueInteger *knowPosition;
  Rts2ValueDouble *rasc;
  Rts2ValueDouble *desc;
  Rts2ValueDouble *lastRa;
  Rts2ValueDouble *lastDec;
  Rts2ValueDouble *lastTarRa;
  Rts2ValueDouble *lastTarDec;

  Rts2ValueDouble *targetRa;
  Rts2ValueDouble *targetDec;

  struct ln_equ_posn lastTar;

  void checkMoves ();
  void checkGuiding ();

  struct timeval dir_timeouts[4];

  char *modelFile;
  Rts2TelModel *model;

  char *modelFile0;
  Rts2TelModel *model0;

  bool standbyPark;

  void applyAberation (struct ln_equ_posn *pos, double JD);
  void applyPrecession (struct ln_equ_posn *pos, double JD);
  void applyRefraction (struct ln_equ_posn *pos, double JD);

  void dontKnowPosition ();

protected:
    Rts2ValueInteger * corrections;

  void applyModel (struct ln_equ_posn *pos, struct ln_equ_posn *model_change,
		   int flip, double JD);
  void applyCorrections (struct ln_equ_posn *pos, double JD);
  // apply corrections (at system time)
  void applyCorrections (double &tar_ra, double &tar_dec);

  virtual int willConnect (Rts2Address * in_addr);
  char *device_file;
  char telType[64];
  Rts2ValueDouble *telRa;
  Rts2ValueDouble *telDec;
  Rts2ValueDouble *telAlt;
  Rts2ValueDouble *telAz;
  Rts2ValueDouble *telSiderealTime;
  Rts2ValueDouble *telLocalTime;
  Rts2ValueInteger *telFlip;
  Rts2ValueDouble *ax1;
  Rts2ValueDouble *ax2;
  Rts2ValueDouble *telLongtitude;
  Rts2ValueDouble *telLatitude;
  Rts2ValueDouble *telAltitude;
  double telParkDec;
  Rts2ValueDouble *telGuidingSpeed;	// in multiply of sidereal speed..eg 1 == 15 arcsec/sec
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
    if ((getState () & TEL_MASK_SEARCHING) == TEL_SEARCH)
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
    return numCorr->getValueInteger ();
  }
  // returns 0 when we need to 
  int setTarget (double tar_ra, double tar_dec);
  void getTarget (struct ln_equ_posn *out_tar)
  {
    out_tar->ra = lastTar.ra;
    out_tar->dec = lastTar.dec;
  }
  void getTargetWithCor (struct ln_equ_posn *out_tar)
  {
    getTarget (out_tar);
    out_tar->ra += locCorRa;
    out_tar->dec += locCorDec;
  }
  void applyLocCorr (struct ln_equ_posn *tar)
  {
    tar->ra += locCorRa;
    tar->dec += locCorDec;
  }
  void getTargetCorrected (struct ln_equ_posn *out_tar)
  {
    getTargetCorrected (out_tar, ln_get_julian_from_sys ());
  }
  void getTargetCorrected (struct ln_equ_posn *out_tar, double JD);
  void unsetTarget ()
  {
    lastTar.ra = -1000;
    lastTar.dec = -1000;
  }
  double getMoveTargetSep ();
  void getTargetAltAz (struct ln_hrz_posn *hrz);
  void getTargetAltAz (struct ln_hrz_posn *hrz, double jd);
  virtual double getLocSidTime ();
  double getLocSidTime (double JD);
  double getSidTime (double JD);
  double getLstDeg (double JD);

  virtual bool isBellowResolution (double ra_off, double dec_off)
  {
    return (ra_off == 0 && dec_off == 0);
  }

  void needStop ()
  {
    maskState (TEL_MASK_NEED_STOP, TEL_NEED_STOP);
  }

  virtual int setValue (Rts2Value * old_value, Rts2Value * new_value);
public:
  Rts2DevTelescope (int argc, char **argv);
  virtual ~ Rts2DevTelescope (void);
  virtual int init ();
  virtual int initValues ();
  virtual int idle ();
  virtual void postEvent (Rts2Event * event);
  virtual int changeMasterState (int new_state);
  virtual Rts2DevClient *createOtherType (Rts2Conn * conn,
					  int other_device_type);
  virtual int deleteConnection (Rts2Conn * in_conn)
  {
    if (in_conn == move_connection)
      move_connection = NULL;
    return Rts2Device::deleteConnection (in_conn);
  }

  // callback functions for telescope alone
  virtual int startMove (double tar_ra, double tar_dec)
  {
    return -1;
  }
  virtual int endMove ();
  virtual int stopMove ();
  virtual int startMoveFixed (double tar_ha, double tar_dec);
  virtual int endMoveFixed ();
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
    locCorRa = cor_ra;
    locCorDec = cor_dec;
    knowPosition->setValueInteger (1);
    return 0;
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
    // ::change (Rts2Conn * conn.. calls setTarget with new target coordinates
    return startMove (lastTar.ra, lastTar.dec);
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
  double getLatitude ()
  {
    return telLatitude->getValueDouble ();
  }

  virtual int startGuide (char dir, double dir_dist);
  virtual int stopGuide (char dir);
  virtual int stopGuideAll ();

  virtual int getAltAz ();

  // callback functions from telescope connection
  virtual int info ();

  virtual int scriptEnds ();

  int startMove (Rts2Conn * conn, double tar_ra, double tar_dec,
		 bool onlyCorrect);
  int startMoveFixed (Rts2Conn * conn, double tar_ha, double tar_dec,
		      bool onlyCorrect);
  int startSearch (Rts2Conn * conn, double radius, double in_searchSpeed);
  int startResyncMove (Rts2Conn * conn, double tar_ra, double tar_dec,
		       bool onlyCorrect);
  int setTo (Rts2Conn * conn, double set_ra, double set_dec);
  int correct (Rts2Conn * conn, int cor_mark, double cor_ra, double cor_dec,
	       struct ln_equ_posn *realPos);
  int startPark (Rts2Conn * conn);
  int change (Rts2Conn * conn, double chng_ra, double chng_dec);
  int saveModel (Rts2Conn * conn);
  int loadModel (Rts2Conn * conn);
  int stopWorm (Rts2Conn * conn);
  int startWorm (Rts2Conn * conn);
  int resetMount (Rts2Conn * conn, resetStates reset_state);
  virtual int getFlip ();
  virtual int grantPriority (Rts2Conn * conn);

  void modelOff ()
  {
    corrections->setValueInteger (corrections->
				  getValueInteger () & ~COR_MODEL);
  }

  void modelOn ()
  {
    corrections->setValueInteger (corrections->
				  getValueInteger () | COR_MODEL);
  }

  bool isModelOn ()
  {
    return (corrections->getValueInteger () & COR_MODEL);
  }

  virtual int commandAuthorized (Rts2Conn * conn);
};

#endif /* !__RTS2_TELD_CPP__ */
