#ifndef __RTS2_GRBD__
#define __RTS2_GRBD__

#include "../utilsdb/rts2devicedb.h"
#include "rts2conngrb.h"

class Rts2ConnGrb;

class Rts2DevGrb:public Rts2DeviceDb
{
private:
  Rts2ConnGrb * gcncnn;
  char *gcn_host;
  int gcn_port;
public:
    Rts2DevGrb (int argc, char **argv);
    virtual ~ Rts2DevGrb ();
  virtual int processOption (int in_opt);
  virtual int init ();

  virtual int ready ()
  {

  }
  virtual int baseInfo ()
  {
    return 0;
  }
  virtual int info ()
  {
    return 0;
  }
  virtual int sendInfo (Rts2Conn * conn);

  int newGcnGrb (int grb_id, int grb_seqn, int grb_type, double grb_ra,
		 double grb_dec, int grb_is_grb, time_t * grb_date,
		 float grb_errorbox);
};

#endif /* __RTS2_GRBD__ */
