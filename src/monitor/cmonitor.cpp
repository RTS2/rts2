#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <iostream>
#include <fstream>
#include <iomanip>
#include <algorithm>

#include "../utils/rts2block.h"
#include "../utils/rts2client.h"
#include "../utils/rts2command.h"

using namespace std;

class Rts2CMonitorConnection:public Rts2ConnClient
{
private:
  void printStatus ();
public:
    Rts2CMonitorConnection (Rts2Block * in_master,
			    char *in_name):Rts2ConnClient (in_master, in_name)
  {
  }
  virtual int commandReturn (Rts2Command * command, int cmd_status);
};

void
Rts2CMonitorConnection::printStatus ()
{
  cout << "============================== \n\
  " << getName () << " status OK \n";
  // get values..
  std::vector < Rts2Value * >::iterator val_iter;
  for (val_iter = otherDevice->values.begin ();
       val_iter != otherDevice->values.end (); val_iter++)
    {
      Rts2Value *val = (*val_iter);
      char *buf;
      buf = val->getValue ();
      cout << fixed << setprecision (2);
      if (buf)
	cout << setw (15) << val->
	  getName () << " = " << setw (8) << buf << endl;
    }
}

int
Rts2CMonitorConnection::commandReturn (Rts2Command * command, int cmd_status)
{
  // if command failed..
  if (cmd_status)
    {
      endConnection ();
      // print some fake status..
      cout <<
	"===================================" << endl
	<< getName () << " FAILED! " << endl << endl
	<< command->getText () << "'returned : " << cmd_status << endl;
      return -1;
    }
  // command OK..
  if (!strcmp (command->getText (), "info"))
    {
      printStatus ();
    }
  return 0;
}

class CommandInfo:public Rts2Command
{

public:
  CommandInfo (Rts2Block * in_owner):Rts2Command (in_owner, "info")
  {
  }
  virtual int commandReturnOK ()
  {
    owner->queAll ("ready");
    owner->queAll ("base_info");
    owner->queAll ("info");
    return Rts2Command::commandReturnOK ();
  }
};

class Rts2CMonitor:public Rts2Client
{
protected:
  virtual Rts2ConnClient * createClientConnection (char *in_deviceName)
  {
    return new Rts2CMonitorConnection (this, in_deviceName);
  }

public:
    Rts2CMonitor (int argc, char **argv):Rts2Client (argc, argv)
  {

  }
  virtual int idle ();
  virtual int run ();
};

int
Rts2CMonitor::idle ()
{
  if (allQuesEmpty ())
    endRunLoop ();
  return Rts2Client::idle ();
}

int
Rts2CMonitor::run ()
{
  getCentraldConn ()->queCommand (new CommandInfo (this));
  return Rts2Client::run ();
}

int
main (int argc, char **argv)
{
  int ret;
  Rts2CMonitor *monitor;
  monitor = new Rts2CMonitor (argc, argv);
  ret = monitor->init ();
  if (ret)
    {
      cerr << "Cannot initialize Monitor\n";
      return 1;
    }
  monitor->run ();
  delete monitor;
  return 0;
}
