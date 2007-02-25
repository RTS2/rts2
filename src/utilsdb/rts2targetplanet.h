#ifndef __RTS2_TARGETPLANET__
#define __RTS2_TARGETPLANET__

#include "target.h"

typedef void (*get_equ_t) (double, struct ln_equ_posn *);
typedef double (*get_double_val_t) (double);

typedef struct planet_info_t
{
  char *name;
  get_equ_t rst_func;
  get_double_val_t earth_func;
  get_double_val_t sun_func;
  get_double_val_t mag_func;
  get_double_val_t sdiam_func;
  get_double_val_t phase_func;
  get_double_val_t disk_func;
};

class TargetPlanet:public Target
{
private:
  planet_info_t * planet_info;
public:
  TargetPlanet (int tar_id, struct ln_lnlat_posn *in_obs);
    virtual ~ TargetPlanet (void);

  virtual int load ();
  virtual int getPosition (struct ln_equ_posn *pos, double JD);
  virtual int getRST (struct ln_rst_time *rst, double JD, double horizon);

  virtual int isContinues ();
  virtual void printExtra (std::ostream & _os, double JD);

  virtual double getEarthDistance (double JD);
  virtual double getSolarDistance (double JD);
  virtual double getMagnitude (double JD);
  virtual double getSDiam (double JD);
  double getPhase (double JD);
  double getDisk (double JD);
};

#endif /*! __RTS2_TARGETPLANET__ */
