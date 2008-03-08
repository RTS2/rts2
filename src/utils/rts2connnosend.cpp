#include "rts2connnosend.h"

Rts2ConnNoSend::Rts2ConnNoSend (Rts2Block * in_master):Rts2Conn (in_master)
{
	setConnTimeout (-1);
}


Rts2ConnNoSend::Rts2ConnNoSend (int in_sock, Rts2Block * in_master):
Rts2Conn (in_sock, in_master)
{
	setConnTimeout (-1);
}


Rts2ConnNoSend::~Rts2ConnNoSend (void)
{
}


int
Rts2ConnNoSend::sendMsg (const char *msg)
{
	return 1;
}
