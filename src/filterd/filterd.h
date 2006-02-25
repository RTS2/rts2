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
protected:
  char *filterType;
  char *serialNumber;

  int filter;

  virtual int getFilterNum (void);
  virtual int setFilterNum (int new_filter);
public:
    Rts2DevFilterd (int in_argc, char **in_argv);
    virtual ~ Rts2DevFilterd (void);

  virtual Rts2DevConn *createConnection (int in_sock, int conn_num);

  virtual int info ();

  virtual int sendInfo (Rts2Conn * conn);
  virtual int sendBaseInfo (Rts2Conn * conn);

  int setFilterNum (Rts2DevConnFilter * conn, int new_filter);
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
