#include "conntcsng.h"

using namespace rts2teld;

ConnTCSNG::ConnTCSNG (rts2core::Block *_master, const char *_hostname, int _port, const char *_obsID, const char *_subID) : rts2core::ConnTCP (_master, _hostname, _port)
{
	obsID = _obsID;
	subID = _subID;
}

const char * ConnTCSNG::request (const char *req, int refNum)
{
	char wbuf[200];
	size_t wlen = snprintf (wbuf, 200, "%s %s %d REQUEST %s\n", obsID, subID, refNum, req);

	init ();

	sendData (wbuf, wlen, false);
	receiveTillEnd (ngbuf, NGMAXSIZE, 3);

	close (sock);

	wlen = snprintf (wbuf, 200, "%s %s %d ", obsID, subID, refNum);

	if (strncmp (wbuf, ngbuf, wlen) != 0)
	{
		throw rts2core::Error ("invalid reply");
	}

	while (isspace (wbuf[wlen]) && wbuf[wlen] != '\0')
		wlen++;

	return ngbuf + wlen;
}

double ConnTCSNG::getSexadecimal (const char *req, int refnum)
{
	char *ret = request (req, refnum);
	if (strlen (ret) < 6)
		throw rts2core::Error ("reply to sexadecimal request must be at least 6 characters long");

	int h,m;
	double sec;
	if (sscanf (ret, "%2d%2d%f", &h, &m, &sec) != 3)
		throw rts2core::Error ("cannot parse sexadecimal reply");
	return h + m / 60.0 + sec / 3600.0;
}
