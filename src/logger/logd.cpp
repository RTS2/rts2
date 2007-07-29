#include "rts2loggerbase.h"
#include "../utils/rts2device.h"

class Rts2Logd:public Rts2Device, public Rts2LoggerBase
{
private:
  Rts2ValueString * logConfig;
  int setLogConfig (const char *new_config);
protected:
    virtual int setValue (Rts2Value * old_value, Rts2Value * new_value);
  virtual int processArgs (const char *arg);
  virtual int willConnect (Rts2Address * in_addr);
public:
    Rts2Logd (int in_argc, char **in_argv);
  virtual Rts2DevClient *createOtherType (Rts2Conn * conn,
					  int other_device_type);
};

Rts2Logd::Rts2Logd (int in_argc, char **in_argv):
Rts2Device (in_argc, in_argv, DEVICE_TYPE_LOGD, "LOGD")
{
  setTimeout (USEC_SEC);
  createValue (logConfig, "log_config", "logging configuration file", false);
}

int
Rts2Logd::setLogConfig (const char *new_config)
{
  std::ifstream * istream = new std::ifstream (new_config);
  int ret = readDevices (*istream);
  delete istream;
  if (ret)
    return ret;
  logConfig->setValueString (new_config);
  return ret;
}

int
Rts2Logd::setValue (Rts2Value * old_value, Rts2Value * new_value)
{
  if (old_value == logConfig)
    {
      return (setLogConfig (new_value->getValue ()) == 0) ? 0 : -2;
    }
  return Rts2Device::setValue (old_value, new_value);
}

int
Rts2Logd::processArgs (const char *arg)
{
  return setLogConfig (arg);
}

int
Rts2Logd::willConnect (Rts2Address * in_addr)
{
  return Rts2LoggerBase::willConnect (in_addr);
}

Rts2DevClient *
Rts2Logd::createOtherType (Rts2Conn * conn, int other_device_type)
{
  return Rts2LoggerBase::createOtherType (conn, other_device_type);
}

int
main (int argc, char **argv)
{
  Rts2Logd logd = Rts2Logd (argc, argv);
  return logd.run ();
}
