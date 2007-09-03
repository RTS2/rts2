#include "sensord.h"

class Rts2DevSensorDummy:public Rts2DevSensor
{
private:
  Rts2ValueDoubleStat * statTest;
  Rts2ValueDoubleMinMax *minMaxTest;
public:
    Rts2DevSensorDummy (int in_argc, char **in_argv):Rts2DevSensor (in_argc,
								    in_argv)
  {
    createValue (statTest, "test_stat", "test stat value", true);
    createValue (minMaxTest, "test_minmax", "test minmax value", true);
  }

  virtual int setValue (Rts2Value * old_value, Rts2Value * newValue)
  {
    if (old_value == minMaxTest)
      return 0;
    return Rts2DevSensor::setValue (old_value, newValue);
  }

  virtual int commandAuthorized (Rts2Conn * conn)
  {
    if (conn->isCommand ("add"))
      {
	double aval;
	if (conn->paramNextDouble (&aval) || !conn->paramEnd ())
	  return -2;
	statTest->addValue (aval);
	statTest->calculate ();
	infoAll ();
	return 0;
      }
    return Rts2DevSensor::commandAuthorized (conn);
  }
};

int
main (int argc, char **argv)
{
  Rts2DevSensorDummy device = Rts2DevSensorDummy (argc, argv);
  return device.run ();
}
