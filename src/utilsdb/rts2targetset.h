#ifndef __RTS2_TARGETSET__
#define __RTS2_TARGETSET__

#include <libnova/libnova.h>
#include <list>
#include <ostream>

#include "target.h"

class Rts2TargetSet:public
  std::list <
Target * >
{
private:
  void
  load (std::string in_where, std::string order_by);
public:
  Rts2TargetSet (struct ln_equ_posn *pos, double radius);
  virtual ~
  Rts2TargetSet (void);
};

std::ostream & operator << (std::ostream & _os, Rts2TargetSet & tar_set);

#endif /* !__RTS2_TARGETSET__ */
