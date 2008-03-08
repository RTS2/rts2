#include "SkyBoTBinding.nsmap"
#include "soapSkyBoTBindingProxy.h"

#include <sstream>

#include "rts2skybottarget.h"

using namespace std;

Rts2SkyBoTTarget::Rts2SkyBoTTarget (const char *in_name):
EllTarget ()
{
	setTargetName (in_name);
}


int
Rts2SkyBoTTarget::load ()
{
	SkyBoTBinding *bind = new SkyBoTBinding();
	skybot__skybotresolverRequest *skybot_req = new skybot__skybotresolverRequest ();
	skybot__skybotresolverResponse skybot_res;

	skybot_req->name = std::string(getTargetName());

	bind->skybot__skybotresolver (skybot_req, skybot_res);
	if (bind->soap->error != SOAP_OK)
	{
		cerr << "Cannot get coordinates for " << getTargetName () << endl;
		soap_print_fault (bind->soap, stderr);
		delete skybot_req;
		delete bind;
		return -1;
	}
	cout << "Skybot response for '" << getTargetName () << "': " << endl
		<< "***************************" << endl
		<< skybot_res.result << endl << "***************************" << endl;

	delete skybot_req;
	delete bind;

	return 0;
}
