#include "sensorgpib.h"

#include "../utils/error.h"

namespace rts2sensord
{

class Lakeshore:public Gpib
{
	public:
		Lakeshore (int argc, char **argv);
		virtual ~ Lakeshore (void);

		virtual int info ();

	private:
		Rts2ValueFloat *tempA;
		Rts2ValueFloat *tempB;

	protected:
		virtual int init ();
		virtual int initValues ();

		virtual int setValue (Rts2Value * oldValue, Rts2Value * newValue);
};

}

using namespace rts2sensord;

Lakeshore::Lakeshore (int in_argc, char **in_argv):Gpib (in_argc, in_argv)
{
	createValue (tempA, "TEMP_A", "A channell temperature", true);
	createValue (tempB, "TEMP_B", "B channell temperature", true);
}


Lakeshore::~Lakeshore (void)
{
}

int Lakeshore::info ()
{
	try
	{
		readValue ("CRDG? A", tempA);
		readValue ("CRDG? B", tempB);
	}
	catch (rts2core::Error er)
	{
		logStream (MESSAGE_ERROR) << er << sendLog;
		return -1;
	}
	return Gpib::info ();
}

int Lakeshore::init ()
{
	return Gpib::init ();
}

int Lakeshore::initValues ()
{
	Rts2ValueString *model = new Rts2ValueString ("model");
	readValue ("*IDN?", model);
	addConstValue (model);
	return Gpib::initValues ();
}

int Lakeshore::setValue (Rts2Value * oldValue, Rts2Value * newValue)
{
/*	try
	{
	}
	catch (rts2core::Error er)
	{
		logStream (MESSAGE_ERROR) << er << sendLog;
		return -2;
	} */
	return Gpib::setValue (oldValue, newValue);
}

int main (int argc, char **argv)
{
	Lakeshore device = Lakeshore (argc, argv);
	return device.run ();
}
