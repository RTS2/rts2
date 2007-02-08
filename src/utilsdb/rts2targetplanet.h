#ifndef __RTS2_TARGETPLANET__
#define __RTS2_TARGETPLANET__

#include "target.h"

typedef void (*get_equ_t) (double, struct ln_equ_posn *);

typedef struct planet_info_t
{
  char *name;
  get_equ_t function;
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
};

#endif /*! __RTS2_TARGETPLANET__ */
