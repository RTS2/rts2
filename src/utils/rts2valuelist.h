#ifndef __RTS2_VALUELIST__
#define __RTS2_VALUELIST__

#include <vector>

#include "rts2value.h"

/**
 * Represent set of Rts2Values. It's used to store values which shall
 * be reseted when new script starts etc..
 */
class Rts2ValueVector:public
  std::vector <
Rts2Value * >
{
public:
  Rts2ValueVector ()
  {
  }
  ~
  Rts2ValueVector (void)
  {
    for (Rts2ValueVector::iterator iter = begin (); iter != end (); iter++)
      delete *
	iter;
  }
};

/**
 * Holds state condition for value.
 */
class
  Rts2CondValue
{
private:
  Rts2Value *
    value;
  int
    stateCondtion;
public:
  Rts2CondValue (Rts2Value * in_value, int in_stateCondtion)
  {
    value = in_value;
    stateCondtion = in_stateCondtion;
  }
  ~
  Rts2CondValue (void)
  {
    delete
      value;
  }
  int
  getStateCondition ()
  {
    return stateCondtion;
  }
  Rts2Value *
  getValue ()
  {
    return value;
  }
};

class
  Rts2CondValueVector:
  public
  std::vector <
Rts2CondValue * >
{
public:
  Rts2CondValueVector ()
  {

  }
  ~
  Rts2CondValueVector (void)
  {
    for (Rts2CondValueVector::iterator iter = begin (); iter != end ();
	 iter++)
      delete *
	iter;
  }
};

#endif /* !__RTS2_VALUELIST__ */
