#include "connection/apm.h"

using namespace rts2core;

ConnAPM::ConnAPM (int _port, rts2core::Block *_master, const char *_hostname):ConnUDP (_port, _master, _hostname)
{
}

int ConnAPM::process (__attribute__ ((unused)) size_t len, __attribute__ ((unused)) struct sockaddr_in &from)
{
	return 0;
}
