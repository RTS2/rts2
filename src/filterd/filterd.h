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

class Rts2DevFilterd:public Rts2Device
{
private:
  /**
   * Set filter names from space separated argument list.
   *
   * @return -1 on error, otherwise 0.
   */
  int setFilters (char *filters);
  int setFilterNumMask (int new_filter);
protected:
  char *filterType;
  char *serialNumber;

  Rts2ValueSelection *filter;

  virtual int processOption (int in_opt);

  virtual int initValues ();

  virtual int getFilterNum (void);
  virtual int setFilterNum (int new_filter);

  virtual int setValue (Rts2Value * old_value, Rts2Value * new_value);

public:
    Rts2DevFilterd (int in_argc, char **in_argv);
    virtual ~ Rts2DevFilterd (void);

  virtual int info ();

  int setFilterNum (Rts2Conn * conn, int new_filter);

  virtual int homeFilter ();

  virtual int commandAuthorized (Rts2Conn * conn);
};

#endif /* !__RTS2_FILTERD__ */
