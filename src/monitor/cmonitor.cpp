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
		Rts2CMonitorConnection (Rts2Block * _master, int _centrald_num, char *_name)
		:Rts2ConnClient (_master, _centrald_num, _name)
		{
		}
		virtual void commandReturn (rts2core::Rts2Command * command, int cmd_status);
};

void
Rts2CMonitorConnection::printStatus ()
{
	cout << "============================== \n\
  " << getName () << " status OK \n";
	// get values..
	rts2core::ValueVector::iterator val_iter;
	for (val_iter = valueBegin ();
		val_iter != valueEnd (); val_iter++)
	{
		rts2core::Value *val = (*val_iter);
		const char *val_buf;
		val_buf = val->getValue ();
		cout << fixed << setprecision (2);
		if (val_buf)
			cout << setw (15) << val->
				getName () << " = " << setw (8) << val_buf << endl;
	}
}


void
Rts2CMonitorConnection::commandReturn (rts2core::Rts2Command * cmd, int cmd_status)
{
	// if command failed..
	if (cmd_status)
	{
		endConnection ();
		// print some fake status..
		cout <<
			"===================================" << endl
			<< getName () << " FAILED! " << endl << endl
			<< cmd->getText () << "'returned : " << cmd_status << endl;
		return;
	}
	// command OK..
	if (!strcmp (cmd->getText (), "info"))
	{
		printStatus ();
	}
}


class CommandInfo:public rts2core::Rts2Command
{

	public:
		CommandInfo (Rts2Block * in_owner):rts2core::Rts2Command (in_owner, "info")
		{
		}
		virtual int commandReturnOK (Rts2Conn * conn)
		{
			owner->queAll ("base_info");
			owner->queAll ("info");
			return rts2core::Rts2Command::commandReturnOK (conn);
		}
};

class Rts2CMonitor:public Rts2Client
{
	protected:
		virtual Rts2ConnClient * createClientConnection (char *_deviceName);

	public:
		Rts2CMonitor (int in_argc, char **in_argv):Rts2Client (in_argc, in_argv)
		{

		}
		virtual int idle ();
		virtual int run ();
};


Rts2ConnClient *
Rts2CMonitor::createClientConnection (char *_deviceName)
{
	Rts2Address *addr = findAddress (_deviceName);
	if (addr == NULL)
		return NULL;
	return new Rts2CMonitorConnection (this, addr->getCentraldNum (), _deviceName);
}


int
Rts2CMonitor::idle ()
{
	if (commandQueEmpty ())
		endRunLoop ();
	return Rts2Client::idle ();
}


int
Rts2CMonitor::run ()
{
	connections_t::iterator iter;
	for (iter = getCentraldConns ()->begin (); iter != getCentraldConns ()->end (); iter++)
	{
		(*iter)->queCommand (new CommandInfo (this));
	}
	return Rts2Client::run ();
}


int
main (int argc, char **argv)
{
	Rts2CMonitor monitor = Rts2CMonitor (argc, argv);
	return monitor.run ();
}
