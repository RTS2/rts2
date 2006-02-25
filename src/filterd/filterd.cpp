#include "filterd.h"

Rts2DevFilterd::Rts2DevFilterd (int in_argc, char **in_argv):
Rts2Device (in_argc, in_argv, DEVICE_TYPE_FW, 5550, "W0")
{
  filter = -1;
  filterType = NULL;
  serialNumber = NULL;
}

Rts2DevFilterd::~Rts2DevFilterd (void)
{
}

Rts2DevConn *
Rts2DevFilterd::createConnection (int in_sock, int conn_num)
{
  return new Rts2DevConnFilter (in_sock, this);
}

int
Rts2DevFilterd::info ()
{
  filter = getFilterNum ();
  return 0;
}

int
Rts2DevFilterd::sendInfo (Rts2Conn * conn)
{
  conn->sendValue ("filter", filter);
  return 0;
}

int
Rts2DevFilterd::sendBaseInfo (Rts2Conn * conn)
{
  conn->sendValue ("type", filterType);
  conn->sendValue ("serial", serialNumber);
  return 0;
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
Rts2DevFilterd::setFilterNum (Rts2DevConnFilter * conn, int new_filter)
{
  int ret;
  ret = setFilterNum (new_filter);
  Rts2Device::info (conn);
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
