#include "rts2loggerbase.h"

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

Rts2LoggerBase::Rts2LoggerBase ()
{
}

int
Rts2LoggerBase::readDevices (std::istream & is)
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

Rts2LogValName *
Rts2LoggerBase::getLogVal (const char *name)
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
Rts2LoggerBase::willConnect (Rts2Address * in_addr)
{
  if (getLogVal (in_addr->getName ()))
    return 1;
  return 0;
}

Rts2DevClient *
Rts2LoggerBase::createOtherType (Rts2Conn * conn, int other_device_type)
{
  Rts2LogValName *val = getLogVal (conn->getName ());
  return new Rts2DevClientLogger (conn, val->timeout, val->valueList);
}
