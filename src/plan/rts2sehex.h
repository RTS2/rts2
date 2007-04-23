#ifndef __RTS2_SEHEX__
#define __RTS2_SEHEX__

#include "rts2scriptblock.h"
#include <libnova/libnova.h>

class Rts2Path:public
std::vector < struct ln_equ_posn *>
{
private:
  Rts2Path::iterator
    top;
public:
  Rts2Path ()
  {
    top = begin ();
  }
  virtual ~
  Rts2Path (void);
  void
  addRaDec (double in_ra, double in_dec);
  void
  endPath ()
  {
    top = begin ();
  }
  bool
  getNext ()
  {
    if (top != end ())
      top++;
    return haveNext ();
  }
  bool
  haveNext ()
  {
    return (top != end ());
  }
  struct ln_equ_posn *
  getTop ()
  {
    return *top;
  }
  double
  getRa ()
  {
    return getTop ()->ra;
  }
  double
  getDec ()
  {
    return getTop ()->dec;
  }
};

class
  Rts2SEHex:
  public
  Rts2ScriptElementBlock
{
private:
  double
    ra_size;
  double
    dec_size;
  Rts2Path
    path;
  Rts2ScriptElementChange *
    changeEl;
  double
  getRa ();
  double
  getDec ();
protected:
  virtual
    bool
  endLoop ();
  virtual bool
  getNextLoop ();
public:
  Rts2SEHex (Rts2Script * in_script, double in_ra_size, double in_dec_size);
  virtual ~
  Rts2SEHex (void);

  virtual void
  beforeExecuting ();
};

#endif /* !__RTS2_SEHEX__ */
