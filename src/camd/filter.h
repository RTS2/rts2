#ifndef __RTS2_FILTER__
#define __RTS2_FILTER__

/*!
 * That class is used for filter devices.
 * It's directly attached to camera, so idependent filter devices can
 * be attached to independent cameras.
 *
 * @author petr
 */

class Rts2Filter
{
public:
  Rts2Filter (void);
    virtual ~ Rts2Filter (void);
  virtual int init (void);
  virtual int getFilterNum (void);
  virtual int setFilterNum (int new_filter);
};

#endif /* !__RTS2_FILTER__ */
