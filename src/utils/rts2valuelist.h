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
class Rts2CondValue
{
private:
  Rts2Value * value;
  int
    save;
  int
    stateCondition;
public:
  Rts2CondValue (Rts2Value * in_value, int in_stateCondition,
		 bool in_save_value)
  {
    value = in_value;
    save = in_save_value ? 0x01 : 0x00;
    stateCondition = in_stateCondition;
  }
  ~Rts2CondValue (void)
  {
    delete value;
  }
  int
  getStateCondition ()
  {
    return stateCondition;
  }
  bool queValueChange (int state)
  {
    return (getStateCondition () & state);
  }
  /**
   * Returns true if value should be saved before it will be changed.
   */
  bool saveValue ()
  {
    return save & 0x01;
  }
  /**
   * Returns true if value needs to be saved.
   */
  bool needSaveValue ()
  {
    return save == 0x01;
  }
  void
  setValueSave ()
  {
    save |= 0x02;
  }
  void
  clearValueSave ()
  {
    save &= ~0x02;
  }
  void
  setIgnoreSave ()
  {
    save |= 0x04;
  }
  bool
  ignoreLoad ()
  {
    return save & 0x04;
  }
  void
  clearIgnoreSave ()
  {
    save &= ~0x04;
  }
  // mark that next operation is value load from que..
  void
  loadFromQue ()
  {
    save |= 0x08;
  }
  void
  clearLoadedFromQue ()
  {
    save &= ~0x08;
  }
  bool
  loadedFromQue ()
  {
    return save & 0x08;
  }
  Rts2Value *
  getValue ()
  {
    return value;
  }
};

class Rts2CondValueVector:
public std::vector < Rts2CondValue * >
{
public:
  Rts2CondValueVector ()
  {

  }
  ~Rts2CondValueVector (void)
  {
    for (Rts2CondValueVector::iterator iter = begin (); iter != end ();
	 iter++)
      delete *
	iter;
  }
};

/**
 * Holds value changes which cannot be handled by device immediately.
 */
class Rts2ValueQue
{
private:
  char
    operation;
  Rts2CondValue *
    old_value;
  Rts2Value *
    new_value;
public:
  Rts2ValueQue (Rts2CondValue * in_old_value, char in_operation,
		Rts2Value * in_new_value)
  {
    old_value = in_old_value;
    operation = in_operation;
    new_value = in_new_value;
  }
  ~Rts2ValueQue (void)
  {
  }
  int
  getStateCondition ()
  {
    return old_value->getStateCondition ();
  }
  bool queValueChange (int state)
  {
    return (getStateCondition () & state);
  }
  Rts2CondValue *
  getCondValue ()
  {
    return old_value;
  }
  Rts2Value *
  getOldValue ()
  {
    return old_value->getValue ();
  }
  char
  getOperation ()
  {
    return operation;
  }
  Rts2Value *
  getNewValue ()
  {
    return new_value;
  }
};

class Rts2ValueQueVector:
public std::vector < Rts2ValueQue * >
{
public:
  Rts2ValueQueVector ()
  {
  }
  ~Rts2ValueQueVector (void)
  {
    for (Rts2ValueQueVector::iterator iter = begin (); iter != end (); iter++)
      delete *
	iter;
  }
};

#endif /* !__RTS2_VALUELIST__ */
