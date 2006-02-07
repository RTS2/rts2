#ifndef __RTS2_FILTER_FLI__
#define __RTS2_FILTER_FLI__

#include "filter.h"

#include "libfli.h"

class Rts2FilterFli:public Rts2Filter
{
private:
  flidev_t dev;
  flidomain_t domain;

  long filter_count;

public:
    Rts2FilterFli (flidomain_t in_domain);
    virtual ~ Rts2FilterFli (void);
  virtual int init (void);
  virtual int getFilterNum (void);
  virtual int setFilterNum (int new_filter);
};

#endif /* !__RTS2_FILTER_FLI__ */
