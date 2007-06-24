#include "../utils/rts2client.h"
#include "../utils/rts2displayvalue.h"

/**
 * This class logs given values to given file.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class Rts2DevClientLogger:public Rts2DevClient
{
private:
  std::list < Rts2Value * >logValues;
  std::list < std::string > logNames;

  time_t nextInfoCall;
  time_t numberSec;

  void fillLogValues ();
public:
    Rts2DevClientLogger (Rts2Conn * in_conn, double in_numberSec,
			 std::list < std::string > &in_logNames);

  virtual void infoOK ();
  virtual void infoFailed ();

  virtual void idle ();
};

Rts2DevClientLogger::Rts2DevClientLogger (Rts2Conn * in_conn,
					  double in_numberSec,
					  std::list < std::string >
					  &in_logNames):
Rts2DevClient (in_conn)
{
  time (&nextInfoCall);
  numberSec = (time_t) in_numberSec;
  logNames = in_logNames;
}

void
Rts2DevClientLogger::fillLogValues ()
{
  for (std::list < std::string >::iterator iter = logNames.begin ();
       iter != logNames.end (); iter++)
    {
      Rts2Value *val = getValue ((*iter).c_str ());
      if (val)
	{
	  logValues.push_back (val);
	}
      else
	{
	  std::
	    cerr << "Cannot find value '" << *iter << "' of device '" <<
	    getName () << "', exiting." << std::endl;
	  exit (EXIT_FAILURE);
	}
    }
}

void
Rts2DevClientLogger::infoOK ()
{
  if (logValues.empty ())
    fillLogValues ();
  std::cout << getName ();
  for (std::list < Rts2Value * >::iterator iter = logValues.begin ();
       iter != logValues.end (); iter++)
    {
      std::cout << " " << getDisplayValue (*iter);
    }
  std::cout << std::endl;
}

void
Rts2DevClientLogger::infoFailed ()
{
  std::cout << "info failed" << std::endl;
}

void
Rts2DevClientLogger::idle ()
{
  time_t now;
  time (&now);
  if (nextInfoCall < now)
    {
      queCommand (new Rts2CommandInfo (getMaster ()));
      nextInfoCall = now + numberSec;
    }
}

/**
 * Holds informations about which device should log which values at which time interval.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class Rts2LogValName
{
public:
  std::string devName;
  double timeout;
    std::list < std::string > valueList;

    Rts2LogValName (std::string in_devName, double in_timeout,
		    std::list < std::string > in_valueList)
  {
    devName = in_devName;
    timeout = in_timeout;
    valueList = in_valueList;
  }
};

class Rts2Logger:public Rts2Client
{
private:
  std::list < Rts2LogValName > devicesNames;
  int readDevices (std::istream & is);
    std::istream * inputStream;

  Rts2LogValName *getLogVal (const char *name);

protected:
    virtual int processOption (int in_opt);
  virtual int init ();
  virtual int willConnect (Rts2Address * in_addr);
public:
    Rts2Logger (int in_argc, char **in_argv);

  virtual Rts2DevClient *createOtherType (Rts2Conn * conn,
					  int other_device_type);
};

Rts2Logger::Rts2Logger (int in_argc, char **in_argv):
Rts2Client (in_argc, in_argv)
{
  setTimeout (USEC_SEC);
  inputStream = NULL;

  addOption ('f', NULL, 1,
	     "specify input file, which holds information which values should be logged for which devices");
}

int
Rts2Logger::readDevices (std::istream & is)
{
  while (!is.eof () && !is.fail ())
    {
      std::string devName;
      double timeout;
      std::string values;
      std::list < std::string > valueList;
      valueList.clear ();

      is >> devName;
      if (is.eof ())
	return 0;

      is >> timeout;

      if (is.eof () || is.fail ())
	{
	  std::
	    cerr << "Cannot read device name or timeout, please correct line "
	    << std::endl;
	  return -1;
	}
      getline (is, values);
      // split values..
      std::istringstream is2 (values);
      while (true)
	{
	  std::string value;
	  is2 >> value;
	  if (is2.fail ())
	    break;
	  valueList.push_back (std::string (value));
	}
      if (valueList.empty ())
	{
	  std::
	    cerr << "Value list for device " << devName <<
	    " is empty, please correct that." << std::endl;
	  return -1;
	}
      devicesNames.push_back (Rts2LogValName (devName, timeout, valueList));
    }
  return 0;
}

int
Rts2Logger::processOption (int in_opt)
{
  int ret;
  switch (in_opt)
    {
    case 'f':
      inputStream = new std::ifstream (optarg);
      ret = readDevices (*inputStream);
      delete inputStream;
      return ret;
    default:
      return Rts2Client::processOption (in_opt);
    }
  return 0;
}

int
Rts2Logger::init ()
{
  int ret;
  ret = Rts2Client::init ();
  if (ret)
    return ret;
  if (!inputStream)
    ret = readDevices (std::cin);
  return ret;
}

Rts2LogValName *
Rts2Logger::getLogVal (const char *name)
{
  for (std::list < Rts2LogValName >::iterator iter = devicesNames.begin ();
       iter != devicesNames.end (); iter++)
    {
      if ((*iter).devName == name)
	return &(*iter);
    }
  return NULL;
}

int
Rts2Logger::willConnect (Rts2Address * in_addr)
{
  if (getLogVal (in_addr->getName ()))
    return 1;
  return 0;
}

Rts2DevClient *
Rts2Logger::createOtherType (Rts2Conn * conn, int other_device_type)
{
  Rts2LogValName *val = getLogVal (conn->getName ());
  return new Rts2DevClientLogger (conn, val->timeout, val->valueList);
}

int
main (int argc, char **argv)
{
  Rts2Logger app = Rts2Logger (argc, argv);
  return app.run ();
}
