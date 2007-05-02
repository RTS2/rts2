#include "filterd.h"

Rts2DevFilterd::Rts2DevFilterd (int in_argc, char **in_argv):
Rts2Device (in_argc, in_argv, DEVICE_TYPE_FW, "W0")
{
  createValue (filter, "filter", "used filter", false);

  filterType = NULL;
  serialNumber = NULL;
}

Rts2DevFilterd::~Rts2DevFilterd (void)
{
}

int
Rts2DevFilterd::initValues ()
{
  addConstValue ("type", filterType);
  addConstValue ("serial", serialNumber);

  return Rts2Device::initValues ();
}

Rts2DevConn *
Rts2DevFilterd::createConnection (int in_sock)
{
  return new Rts2DevConnFilter (in_sock, this);
}

int
Rts2DevFilterd::info ()
{
  filter->setValueInteger (getFilterNum ());
  return Rts2Device::info ();
}

int
Rts2DevFilterd::getFilterNum ()
{
  return -1;
}

int
Rts2DevFilterd::setFilterNum (int new_filter)
{
  return -1;
}

int
Rts2DevFilterd::setValue (Rts2Value * old_value, Rts2Value * new_value)
{
  if (old_value == filter)
    return setFilterNumMask (new_value->getValueInteger ()) == 0 ? 0 : -2;
  return Rts2Device::setValue (old_value, new_value);
}

int
Rts2DevFilterd::homeFilter ()
{
  return -1;
}

int
Rts2DevFilterd::setFilterNumMask (int new_filter)
{
  int ret;
  maskState (FILTERD_MASK, FILTERD_MOVE, "filter move started");
  ret = setFilterNum (new_filter);
  Rts2Device::infoAll ();
  if (ret == -1)
    {
      maskState (DEVICE_ERROR_MASK | FILTERD_MASK,
		 DEVICE_ERROR_HW | FILTERD_IDLE);
      return ret;
    }
  maskState (FILTERD_MASK, FILTERD_IDLE);
  return ret;
}

int
Rts2DevFilterd::setFilterNum (Rts2DevConnFilter * conn, int new_filter)
{
  int ret;
  ret = setFilterNumMask (new_filter);
  if (ret == -1)
    {
      conn->sendCommandEnd (DEVDEM_E_HW, "filter set failed");
    }
  return ret;
}

Rts2DevConnFilter::Rts2DevConnFilter (int in_sock, Rts2DevFilterd * in_master_device):Rts2DevConn (in_sock,
	     in_master_device)
{
  master = in_master_device;
}

int
Rts2DevConnFilter::commandAuthorized ()
{
  if (isCommand ("filter"))
    {
      int new_filter;
      if (paramNextInteger (&new_filter) || !paramEnd ())
	return -2;
      return master->setFilterNum (this, new_filter);
    }
  else if (isCommand ("home"))
    {
      if (!paramEnd ())
	return -2;
      return master->homeFilter ();
    }
  else if (isCommand ("help"))
    {
      send ("ready - is filter ready?");
      send ("info - information about camera");
      send ("filter <filter number> - set camera filter");
      send ("exit - exit from connection");
      send ("help - print, what you are reading just now");
      return 0;
    }
  return Rts2DevConn::commandAuthorized ();
}
