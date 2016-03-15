#include "conntcsng.h"

using namespace rts2teld;

ConnTCSNG::ConnTCSNG (rts2core::Block *_master, const char *_hostname, int _port, const char *_obsID, const char *_subID) : rts2core::ConnTCP (_master, _hostname, _port)
{
	obsID = _obsID;
	subID = _subID;
}

const char * ConnTCSNG::runCommand (const char *cmd, const char *req)
{
	char wbuf[200];
	size_t wlen = snprintf (wbuf, 200, "%s %s %d %s %s\n", obsID, subID, reqCount, cmd, req);

	init ();

	sendData (wbuf, wlen, false);
	receiveTillEnd (ngbuf, NGMAXSIZE, 3);

	close (sock);

	wlen = snprintf (wbuf, 200, "%s %s %d ", obsID, subID, reqCount);

	if (strncmp (wbuf, ngbuf, wlen) != 0)
	{
		throw rts2core::Error ("invalid reply");
	}

	while (isspace (wbuf[wlen]) && wbuf[wlen] != '\0')
		wlen++;
	
	reqCount++;

	return ngbuf + wlen;
}

const char * ConnTCSNG::request (const char *val)
{
	return runCommand ("REQUEST", val);
}

const char * ConnTCSNG::command (const char *cmd)
{
	return runCommand ("COMMAND", cmd);
}

double ConnTCSNG::getSexadecimalHours (const char *req)
{
	const char *ret = request (req);
	if (strlen (ret) < 6)
		throw rts2core::Error ("reply to sexadecimal request must be at least 6 characters long");

	int h,m;
	double sec;
	if (sscanf (ret, "%2d%2d%lf", &h, &m, &sec) != 3)
		throw rts2core::Error ("cannot parse sexadecimal reply");
	return 15 * (h + m / 60.0 + sec / 3600.0);
}

double ConnTCSNG::getSexadecimalTime (const char *req)
{
	const char *ret = request (req);
	if (strlen (ret) < 6)
		throw rts2core::Error ("reply to sexadecimal request must be at least 6 characters long");

	int h,m;
	double sec;
	if (sscanf (ret, "%d:%d:%lf", &h, &m, &sec) != 3)
		throw rts2core::Error ("cannot parse sexadecimal reply");
	return 15 * (h + m / 60.0 + sec / 3600.0);
}

double ConnTCSNG::getSexadecimalAngle (const char *req)
{
	const char *ret = request (req);
	if (strlen (ret) < 6)
		throw rts2core::Error ("reply to sexadecimal request must be at least 6 characters long");

	int h,m;
	double sec;
	if (sscanf (ret, "%3d%2d%lf", &h, &m, &sec) != 3)
		throw rts2core::Error ("cannot parse sexadecimal reply");
	return h + m / 60.0 + sec / 3600.0;
}
