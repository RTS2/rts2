#ifndef __RTS2_GRBFW__
#define __RTS2_GRBFW__

#include "../utils/rts2block.h"
#include "../utils/rts2connnosend.h"

/**
 * Defines forward connection.
 *
 * Enables GRBd to forward incoming GCN packets to other destinations.
 *
 * @author petr
 */
class Rts2GrbForwardConnection:public Rts2ConnNoSend
{
	private:
		int forwardPort;
	public:
		Rts2GrbForwardConnection (Rts2Block * in_master, int in_forwardPort);
		virtual int init ();
		// span new GRBFw connection
		virtual int receive (fd_set * set);
};

class Rts2GrbForwardClientConn:public Rts2ConnNoSend
{
	private:
		void forwardPacket (int32_t *nbuf);
	public:
		Rts2GrbForwardClientConn (int in_sock, Rts2Block * in_master);
		virtual void postEvent (Rts2Event * event);

		virtual int receive (fd_set * set);
};
#endif							 /* !__RTS2_GRBFW__ */
