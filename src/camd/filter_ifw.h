#ifndef __RTS2_FILTER_IFW__
#define __RTS2_FILTER_IFW__

#include <unistd.h>

#include "filter.h"

/*! 
 * Class for OPTEC filter wheel.
 *
 * @author john, petr
 */
class Rts2FilterIfw:public Rts2Filter
{
  char *dev_file;
  char filter_buff[5];
  int dev_port;
  int writePort (char *buf, size_t len);
  int readPort (size_t len);
  int shutdown ();
public:
    Rts2FilterIfw (char *in_dev_file);
    virtual ~ Rts2FilterIfw (void);
  virtual int init (void);
  virtual int getFilterNum (void);
  virtual int setFilterNum (int new_filter);
};

#endif /* !__RTS2_FILTER_IFW__ */
