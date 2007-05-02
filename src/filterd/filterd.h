#ifndef __RTS2_FILTERD__
#define __RTS2_FILTERD__

#include "../utils/rts2device.h"

/*!
 * That class is used for filter devices.
 * It's directly attached to camera, so idependent filter devices can
 * be attached to independent cameras.
 *
 * @author petr
 */

class Rts2DevConnFilter;

class Rts2DevFilterd:public Rts2Device
{
private:
  int setFilterNumMask (int new_filter);
protected:
  char *filterType;
  char *serialNumber;

  Rts2ValueInteger *filter;

  virtual int getFilterNum (void);
  virtual int setFilterNum (int new_filter);

  virtual int setValue (Rts2Value * old_value, Rts2Value * new_value);

public:
    Rts2DevFilterd (int in_argc, char **in_argv);
    virtual ~ Rts2DevFilterd (void);

  virtual int initValues ();

  virtual Rts2DevConn *createConnection (int in_sock);

  virtual int info ();

  int setFilterNum (Rts2DevConnFilter * conn, int new_filter);

  virtual int homeFilter ();
};

class Rts2DevConnFilter:public Rts2DevConn
{
private:
  Rts2DevFilterd * master;
protected:
  virtual int commandAuthorized ();
public:
    Rts2DevConnFilter (int in_sock, Rts2DevFilterd * in_master_device);
};

#endif /* !__RTS2_FILTERD__ */
