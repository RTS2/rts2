#include "sensorgpib.h"

struct keithG4Header
{
  // header
  char type1;
  char type2;
  char reserved;
  char instrset;
  int16_t bytecount;
};

class Rts2DevSensorKeithley487:public Rts2DevSensorGpib
{
private:
  Rts2ValueFloat * curr;
protected:
  virtual int init ();
  virtual int info ();
public:
    Rts2DevSensorKeithley487 (int in_argc, char **in_argv);
};

Rts2DevSensorKeithley487::Rts2DevSensorKeithley487 (int in_argc,
						    char **in_argv):
Rts2DevSensorGpib (in_argc, in_argv)
{
  setPad (23);

  createValue (curr, "CURRENT", "Measured current", true);
}

int
Rts2DevSensorKeithley487::init ()
{
  int ret;
  ret = Rts2DevSensorGpib::init ();
  if (ret)
    return ret;
  // binary for Intel
  return gpibWrite ("G4X");
}

int
Rts2DevSensorKeithley487::info ()
{
  int ret;
  struct
  {
    struct keithG4Header header;
    char value[4];
  } oneShot;

  ret = gpibWrite ("N1T5X");
  if (ret)
    return ret;
  ret = gpibRead (&oneShot, 10);
  if (ret)
    return ret;
  if (ibcnt != 10)
    {
      logStream (MESSAGE_ERROR) << "Read something else!" << ibcnt << sendLog;
      return -1;
    }

  curr->setValueFloat (*((float *) (&(oneShot.value))));

  return Rts2DevSensorGpib::info ();
}

int
main (int argc, char **argv)
{
  Rts2DevSensorKeithley487 device = Rts2DevSensorKeithley487 (argc, argv);
  return device.run ();
}
