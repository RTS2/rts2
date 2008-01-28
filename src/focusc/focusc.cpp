/*!
 * Command-line fits writer.
 * $Id$
 *
 * Client to test camera deamon.
 *
 * @author petr
 */

#include "genfoc.h"

#include <vector>
#include <iostream>

class Rts2focusc:public Rts2GenFocClient
{
	protected:
		virtual Rts2GenFocCamera * createFocCamera (Rts2Conn * conn);
		virtual void help ();
	public:
		Rts2focusc (int argc, char **argv);
};

Rts2GenFocCamera *
Rts2focusc::createFocCamera (Rts2Conn * conn)
{
	return new Rts2GenFocCamera (conn, this);
}


void
Rts2focusc::help ()
{
	Rts2GenFocClient::help ();
	std::cout << "Examples:" << std::endl
		<<
		"\trts2-focusc -d C0 -d C1 -e 20 .. takes 20 sec exposures on devices C0 and C1"
		<< std::endl;
}


Rts2focusc::Rts2focusc (int in_argc, char **in_argv):
Rts2GenFocClient (in_argc, in_argv)
{
	autoSave = 1;

	addOption ('s', "secmod", 1, "exposure every UT sec");
}


int
main (int argc, char **argv)
{
	Rts2focusc client = Rts2focusc (argc, argv);
	return client.run ();
}
