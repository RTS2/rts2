#include "nan.h"

#include "mirror.h"

Rts2DevMirror::Rts2DevMirror (int in_argc, char **in_argv):rts2core::Device (in_argc, in_argv, DEVICE_TYPE_MIRROR, "M0")
{
}

Rts2DevMirror::~Rts2DevMirror (void)
{

}

int Rts2DevMirror::idle ()
{
	int ret;
	ret = rts2core::Device::idle ();
	switch (getState () & MIRROR_MASK)
	{
		case MIRROR_A_B:
			ret = isOpened ();
			if (ret == -2)
				return endOpen ();
			break;
		case MIRROR_B_A:
			ret = isClosed ();
			if (ret == -2)
				return endClose ();
			break;
		default:
			return ret;
	}
	// set timeouts..
	setTimeoutMin (100000);
	return ret;
}

int Rts2DevMirror::startOpen (rts2core::Connection * conn)
{
	int ret;
	ret = startOpen ();
	if (ret)
	{
		conn->sendCommandEnd (DEVDEM_E_HW, "cannot open mirror");
		return -1;
	}
	return 0;
}

int Rts2DevMirror::startClose (rts2core::Connection * conn)
{
	int ret;
	ret = startClose ();
	if (ret)
	{
		conn->sendCommandEnd (DEVDEM_E_HW, "cannot close mirror");
		return -1;
	}
	return 0;
}

int Rts2DevMirror::commandAuthorized (rts2core::Connection * conn)
{
	if (conn->isCommand ("mirror"))
	{
		char *str_dir;
		if (conn->paramNextString (&str_dir) || !conn->paramEnd ())
			return -2;
		if (!strcasecmp (str_dir, "open"))
			return startOpen (conn);
		if (!strcasecmp (str_dir, "close"))
			return startClose (conn);
	}
	if (conn->isCommand ("set"))
	{
		char *str_dir;
		if (conn->paramNextString (&str_dir) || !conn->paramEnd () ||
			(strcasecmp (str_dir, "A") && strcasecmp (str_dir, "B")))
			return -2;
		if (!strcasecmp (str_dir, "A"))
		{
			if ((getState () & MIRROR_MASK) != MIRROR_A)
			{
				return startClose (conn);
			}
			else
			{
				conn->sendCommandEnd (DEVDEM_E_IGNORE, "already in A");
				return -1;
			}
		}
		else if (!strcasecmp (str_dir, "B"))
		{
			if ((getState () & MIRROR_MASK) != MIRROR_B)
			{
				return startOpen (conn);
			}
			else
			{
				conn->sendCommandEnd (DEVDEM_E_IGNORE, "already in B");
				return -1;
			}
		}
	}
	return rts2core::Device::commandAuthorized (conn);
}
