#ifndef __RTS2_TELD_IR__
#define __RTS2_TELD_IR__

#include <errno.h>
#include <string.h>
#include <math.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <libnova/libnova.h>

#include "status.h"
#include "telescope.h"

#include <cstdio>
#include <cstdarg>
#include <opentpl/client.h>
#include <list>
#include <iostream>

using namespace OpenTPL;

class ErrorTime
{
  time_t etime;
  int error;
public:
    ErrorTime (int in_error);
  int clean (time_t now);
  int isError (int in_error);
};

class Rts2DevTelescopeIr:public Rts2DevTelescope
{
private:
  std::string * ir_ip;
  int ir_port;
  Client *tplc;
  time_t timeout;
  double cover;
  enum
  { OPENED, OPENING, CLOSING, CLOSED } cover_state;

  struct ln_equ_posn target;

  virtual int coverClose ();
  virtual int coverOpen ();

  int startMoveReal (double ra, double dec);

  void addError (int in_error);

  void checkErrors ();
  void checkCover ();
  void checkPower ();

    std::list < ErrorTime * >errorcodes;
  int irTracking;
  char *irConfig;
  bool makeModel;

protected:
    template < typename T > int tpl_get (const char *name, T & val,
					 int *status);
    template < typename T > int tpl_set (const char *name, T val,
					 int *status);
    template < typename T > int tpl_setw (const char *name, T val,
					  int *status);
  virtual int processOption (int in_opt);
public:
    Rts2DevTelescopeIr (int argc, char **argv);
    virtual ~ Rts2DevTelescopeIr (void);
  virtual int initDevice ();
  virtual int init ();
  virtual int idle ();
  virtual int ready ();
  virtual int baseInfo ();
  virtual int info ();
  virtual int startMove (double tar_ra, double tar_dec);
  virtual int isMoving ();
  virtual int endMove ();
  virtual int startPark ();
  virtual int isParking ();
  virtual int endPark ();
  virtual int stopMove ();
  virtual int correctOffsets (double cor_ra, double cor_dec, double real_ra,
			      double real_dec);
  virtual int correct (double cor_ra, double cor_dec, double real_ra,
		       double real_dec);
  virtual int change (double chng_ra, double chng_dec);
  virtual int saveModel ();
  virtual int loadModel ();
  virtual int stopWorm ();
  virtual int startWorm ();
  virtual int changeMasterState (int new_state);
  virtual int resetMount (resetStates reset_mount);

  virtual int getError (int in_error, std::string & desc);
};

#endif /* !__RTS2_TELD_IR__ */
