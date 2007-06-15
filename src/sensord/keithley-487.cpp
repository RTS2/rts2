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
  Rts2ValueBool *sourceOn;
  Rts2ValueFloat *voltage;
protected:
    virtual int init ();
  virtual int info ();
  virtual int setValue (Rts2Value * old_value, Rts2Value * new_value);
public:
    Rts2DevSensorKeithley487 (int in_argc, char **in_argv);
};

Rts2DevSensorKeithley487::Rts2DevSensorKeithley487 (int in_argc,
						    char **in_argv):
Rts2DevSensorGpib (in_argc, in_argv)
{
  setPad (23);

  createValue (curr, "CURRENT", "Measured current", true);
  createValue (sourceOn, "ON", "If voltage source is switched on", true);
  createValue (voltage, "VOLTAGE", "Voltage level", true);

  voltage->setValueFloat (0);
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
Rts2DevSensorKeithley487::setValue (Rts2Value * old_value,
				    Rts2Value * new_value)
{
  char buf[50];
  if (old_value == sourceOn)
    {
      // operate..
      strcpy (buf, "O0X");
      if (((Rts2ValueBool *) new_value)->getValueBool ())
	buf[1] = '1';
      return gpibWrite (buf);
    }
  if (old_value == voltage)
    {
      snprintf (buf, 50, "V%fX", new_value->getValueFloat ());
      return gpibWrite (buf);
    }
  return Rts2DevSensorGpib::setValue (old_value, new_value);
}

int
main (int argc, char **argv)
{
  Rts2DevSensorKeithley487 device = Rts2DevSensorKeithley487 (argc, argv);
  return device.run ();
}
