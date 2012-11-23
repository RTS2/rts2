#include <iostream>
#include <fstream>
#include <iomanip>
#include <algorithm>

#include "block.h"
#include "client.h"
#include "command.h"

using namespace std;

class Rts2CMonitorConnection:public rts2core::ConnClient
{
	private:
		void printStatus ();
	public:
		Rts2CMonitorConnection (rts2core::Block * _master, int _centrald_num, char *_name):rts2core::ConnClient (_master, _centrald_num, _name)
		{
		}
		virtual void commandReturn (rts2core::Command * command, int cmd_status);
};

void Rts2CMonitorConnection::printStatus ()
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

void Rts2CMonitorConnection::commandReturn (rts2core::Command * cmd, int cmd_status)
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

class CommandInfo:public rts2core::Command
{

	public:
		CommandInfo (rts2core::Block * in_owner):rts2core::Command (in_owner, "info")
		{
		}
		virtual int commandReturnOK (rts2core::Connection * conn)
		{
			owner->queAll ("base_info");
			owner->queAll ("info");
			return rts2core::Command::commandReturnOK (conn);
		}
};

class Rts2CMonitor:public rts2core::Client
{
	protected:
		virtual rts2core::ConnClient * createClientConnection (char *_deviceName);

	public:
		Rts2CMonitor (int in_argc, char **in_argv):rts2core::Client (in_argc, in_argv, "cmonitor") {}
		virtual int idle ();
		virtual int run ();
};

rts2core::ConnClient * Rts2CMonitor::createClientConnection (char *_deviceName)
{
	rts2core::NetworkAddress *addr = findAddress (_deviceName);
	if (addr == NULL)
		return NULL;
	return new Rts2CMonitorConnection (this, addr->getCentraldNum (), _deviceName);
}

int Rts2CMonitor::idle ()
{
	if (commandQueEmpty ())
		endRunLoop ();
	return rts2core::Client::idle ();
}

int Rts2CMonitor::run ()
{
	rts2core::connections_t::iterator iter;
	for (iter = getCentraldConns ()->begin (); iter != getCentraldConns ()->end (); iter++)
	{
		(*iter)->queCommand (new CommandInfo (this));
	}
	return rts2core::Client::run ();
}
 
int main (int argc, char **argv)
{
	Rts2CMonitor monitor = Rts2CMonitor (argc, argv);
	return monitor.run ();
}
