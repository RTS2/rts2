/* 
 * Simple GRB forwarder.
 * Copyright (C) 2005-2008 Petr Kubanek <petr@kubanek.net>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#include "block.h"
#include "connnosend.h"
#include "grbconst.h"
#include "grbd.h"
#include "rts2grbfw.h"

#include <errno.h>
#include <netdb.h>
#include <time.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <iostream>

/**
 * Simple GRBD forwarder.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class Rts2ConnFwGrb:public rts2core::ConnNoSend
{
	public:
		Rts2ConnFwGrb (char *in_gcn_hostname, int in_gcn_port, rts2core::Block * in_master);
		virtual ~ Rts2ConnFwGrb (void);
		virtual int idle ();
		virtual int init ();

		virtual int add (rts2core::Block *block);

		virtual void connectionError (int last_data_size);
		virtual int receive (rts2core::Block *block);

		int lastPacket ();
		double delta ();
		char *lastTarget ();
		void setLastTarget (char *in_last_target);
		int lastTargetTime ();

	private:
		long lbuf[SIZ_PKT];		 // local buffer - swaped for Linux
		long nbuf[SIZ_PKT];		 // network buffer
		struct timeval last_packet;
		double here_sod;		 // machine SOD (seconds after 0 GMT)
		double last_imalive_sod; // SOD of the previous imalive packet

		double deltaValue;
		char *last_target;

		// init listen (listening on given port) and call (try to connect to given
		// port; there must be GCN packet receiving running on oppoiste side) GCN
		// connection
		int init_listen ();
		int init_call ();

		double getPktSod ();

		// process various messages..
		int pr_imalive ();

		int gcn_port;
		char *gcn_hostname;
		int do_hete_test;

		int gcn_listen_sock;

		time_t nextTime;
};

double Rts2ConnFwGrb::getPktSod ()
{
	return lbuf[PKT_SOD] / 100.0;
}

int Rts2ConnFwGrb::pr_imalive ()
{
	deltaValue = here_sod - getPktSod ();
	logStream (MESSAGE_DEBUG)
		<< "Rts2ConnFwGrb::pr_imalive last packet SN="
		<< getPktSod () << " delta=" << deltaValue << " last_delta=" <<
		(getPktSod () - last_imalive_sod) << sendLog;
	last_imalive_sod = getPktSod ();
	return 0;
}

Rts2ConnFwGrb::Rts2ConnFwGrb (char *in_gcn_hostname, int in_gcn_port, rts2core::Block * in_master):rts2core::ConnNoSend (in_master)
{
	gcn_hostname = new char[strlen (in_gcn_hostname) + 1];
	strcpy (gcn_hostname, in_gcn_hostname);
	gcn_port = in_gcn_port;
	gcn_listen_sock = -1;

	time (&last_packet.tv_sec);
	last_packet.tv_sec -= 600;
	last_packet.tv_usec = 0;
	last_imalive_sod = -1;

	deltaValue = 0;
	last_target = NULL;

	setConnTimeout (90);
	time (&nextTime);
	nextTime += getConnTimeout ();
}

Rts2ConnFwGrb::~Rts2ConnFwGrb (void)
{
	delete gcn_hostname;
	if (last_target)
		delete last_target;
	if (gcn_listen_sock >= 0)
		close (gcn_listen_sock);
}

int Rts2ConnFwGrb::idle ()
{
	int ret;
	int err;
	socklen_t len = sizeof (err);

	time_t now;
	time (&now);

	switch (getConnState ())
	{
		case CONN_CONNECTING:
			ret = getsockopt (sock, SOL_SOCKET, SO_ERROR, &err, &len);
			if (ret)
			{
				logStream (MESSAGE_ERROR) << "Rts2ConnFwGrb::idle getsockopt " <<
					strerror (errno) << sendLog;
				connectionError (-1);
			}
			else if (err)
			{
				logStream (MESSAGE_ERROR) << "Rts2ConnFwGrb::idle getsockopt "
					<< strerror (err) << sendLog;
				connectionError (-1);
			}
			else
			{
				setConnState (CONN_CONNECTED);
			}
			// kill us when we were in conn_connecting state for to long
		RTS2_FALLTHRU;
		case CONN_BROKEN:
			if (nextTime < now)
			{
				ret = init ();
				if (ret)
				{
					time (&nextTime);
					nextTime += getConnTimeout ();
				}
			}
			break;
		case CONN_CONNECTED:
			if (last_packet.tv_sec + getConnTimeout () < now && nextTime < now)
				connectionError (-1);
			break;
		default:
			break;
	}
	// we don't like to get called upper code with timeouting stuff..
	return 0;
}

int Rts2ConnFwGrb::init_call ()
{
	struct addrinfo hints;
	struct addrinfo *info;
	int ret;

	hints.ai_flags = 0;
	hints.ai_family = PF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = 0;
	std::ostringstream _os;
	_os << gcn_port;
	ret = getaddrinfo (gcn_hostname, _os.str ().c_str (), &hints, &info);
	if (ret)
	{
		logStream (MESSAGE_ERROR) << "NetworkAddress::getAddress getaddrinfor: "
			<< gai_strerror (ret) << sendLog;
		freeaddrinfo (info);
		return -1;
	}
	sock = socket (info->ai_family, info->ai_socktype, info->ai_protocol);
	if (sock == -1)
	{
		freeaddrinfo (info);
		return -1;
	}
	ret = connect (sock, info->ai_addr, info->ai_addrlen);
	freeaddrinfo (info);
	time (&nextTime);
	nextTime += getConnTimeout ();
	if (ret == -1)
	{
		if (errno == EINPROGRESS)
		{
			setConnState (CONN_CONNECTING);
			return 0;
		}
		return -1;
	}
	setConnState (CONN_CONNECTED);
	return 0;
}

int Rts2ConnFwGrb::init_listen ()
{
	int ret;

	if (gcn_listen_sock >= 0)
	{
		close (gcn_listen_sock);
		gcn_listen_sock = -1;
	}

	connectionError (-1);

	gcn_listen_sock = socket (PF_INET, SOCK_STREAM, 0);
	if (gcn_listen_sock == -1)
	{
		logStream (MESSAGE_ERROR) << "Rts2ConnFwGrb::init_listen socket " <<
			strerror (errno) << sendLog;
		return -1;
	}
	const int so_reuseaddr = 1;
	setsockopt (gcn_listen_sock, SOL_SOCKET, SO_REUSEADDR, &so_reuseaddr,
		sizeof (so_reuseaddr));
	struct sockaddr_in server;
	server.sin_family = AF_INET;
	server.sin_port = htons (gcn_port);
	server.sin_addr.s_addr = htonl (INADDR_ANY);
	ret = bind (gcn_listen_sock, (struct sockaddr *) &server, sizeof (server));
	if (ret)
	{
		logStream (MESSAGE_ERROR) << "Rts2ConnFwGrb::init_listen bind: " <<
			strerror (errno) << sendLog;
		return -1;
	}
	ret = listen (gcn_listen_sock, 1);
	if (ret)
	{
		logStream (MESSAGE_ERROR) << "Rts2ConnFwGrb::init_listen listen: " <<
			strerror (errno) << sendLog;
		return -1;
	}
	setConnState (CONN_CONNECTED);
	time (&nextTime);
	nextTime += 2 * getConnTimeout ();
	return 0;
}

int Rts2ConnFwGrb::init ()
{
	if (!strcmp (gcn_hostname, "-"))
		return init_listen ();
	else
		return init_call ();
}

int Rts2ConnFwGrb::add (rts2core::Block *block)
{
	if (gcn_listen_sock >= 0)
	{
		block->addPollFD (gcn_listen_sock, POLLIN | POLLPRI);
		return 0;
	}
	return rts2core::Connection::add (block);
}

void Rts2ConnFwGrb::connectionError (__attribute__((unused)) int last_data_size)
{
	logStream (MESSAGE_DEBUG) << "Rts2ConnFwGrb::connectionError" << sendLog;
	if (sock > 0)
	{
		close (sock);
		sock = -1;
	}
	if (!isConnState (CONN_BROKEN))
	{
		time (&nextTime);
		sock = -1;
		setConnState (CONN_BROKEN);
	}
}

int Rts2ConnFwGrb::receive (rts2core::Block *block)
{
	int ret = 0;
	struct tm *t;
	if (gcn_listen_sock >= 0 && block->isForRead (gcn_listen_sock))
	{
		// try to accept connection..
		close (sock);			 // close previous connections..we support only one GCN connection
		sock = -1;
		struct sockaddr_in other_side;
		socklen_t addr_size = sizeof (struct sockaddr_in);
		sock =
			accept (gcn_listen_sock, (struct sockaddr *) &other_side, &addr_size);
		if (sock == -1)
		{
			// bad accept - strange
			logStream (MESSAGE_ERROR)
				<< "Rts2ConnFwGrb::receive accept on gcn_listen_sock: " <<
				strerror (errno) << sendLog;
			connectionError (-1);
		}
		// close listening socket..when we get connection
		close (gcn_listen_sock);
		gcn_listen_sock = -1;
		setConnState (CONN_CONNECTED);
		logStream (MESSAGE_INFO)
			<< "Rts2ConnFwGrb::receive accept gcn_listen_sock from "
			<< inet_ntoa (other_side.sin_addr) << " port " << ntohs (other_side.
			sin_port) <<
			sendLog;
	}
	else if (sock >= 0 && block->isForRead (sock))
	{
		// translate packages to linux..
		short *sp;				 // Ptr to a short; used for the swapping
		short pl, ph;			 // Low part & high part
		ret = read (sock, (char *) nbuf, sizeof (nbuf));
		if (ret == 0 && isConnState (CONN_CONNECTING))
		{
			setConnState (CONN_CONNECTED);
		}
		else if (ret <= 0)
		{
			connectionError (ret);
			return -1;
		}
		successfullRead ();
		gettimeofday (&last_packet, NULL);
		/* Immediately echo back the packet so GCN can monitor:
		 * (1) the actual receipt by the site, and
		 * (2) the roundtrip travel times.
		 * Everything except KILL's get echo-ed back.            */
		if (nbuf[PKT_TYPE] != TYPE_KILL_SOCKET)
		{
			write (sock, (char *) nbuf, sizeof (nbuf));
			successfullSend ();
		}
		swab (nbuf, lbuf, SIZ_PKT * sizeof (lbuf[0]));
		sp = (short *) lbuf;
		for (int i = 0; i < SIZ_PKT; i++)
		{
			pl = sp[2 * i];
			ph = sp[2 * i + 1];
			sp[2 * i] = ph;
			sp[2 * i + 1] = pl;
		}
		t = gmtime (&last_packet.tv_sec);
		here_sod =
			t->tm_hour * 3600 + t->tm_min * 60 + t->tm_sec +
			last_packet.tv_usec / USEC_SEC;

		// switch based on packet content
		switch (lbuf[PKT_TYPE])
		{
			case TYPE_IM_ALIVE:
				pr_imalive ();
				break;
			case TYPE_KILL_SOCKET:
				connectionError (-1);
				break;
			default:
				logStream (MESSAGE_ERROR) <<
					"Rts2ConnFwGrb::receive unknow packet type: " << lbuf[PKT_TYPE] <<
					sendLog;
				break;
		}
		// enable others to catch-up (FW connections will forward packet to their sockets)
		getMaster ()->postEvent (new rts2core::Event (RTS2_EVENT_GRB_PACKET, nbuf));
	}
	return ret;
}

// now the application
class Rts2AppFw:public rts2core::Block
{
	private:
		Rts2ConnFwGrb * gcncnn;
		char *gcn_host;
		int gcn_port;
		int do_hete_test;

		int forwardPort;

	protected:
		virtual rts2core::Connection * createClientConnection (__attribute__((unused)) char *in_deviceName)
		{
			return NULL;
		}
		virtual rts2core::Connection *createClientConnection (__attribute__((unused)) rts2core::NetworkAddress * in_addr)
		{
			return NULL;
		}

		virtual int processOption (int in_opt);
		virtual int init ();

	public:
		Rts2AppFw (int arcg, char **argv);
		virtual ~ Rts2AppFw (void);
		virtual int run ();
};

Rts2AppFw::Rts2AppFw (int in_argc, char **in_argv):rts2core::Block (in_argc, in_argv)
{
	gcncnn = NULL;
	gcn_host = NULL;
	gcn_port = -1;
	forwardPort = -1;

	addOption ('s', "gcn_host", 1, "GCN host name");
	addOption ('p', "gcn_port", 1, "GCN port");
	addOption ('f', "forward", 1, "forward incoming notices to that port");
}

Rts2AppFw::~Rts2AppFw (void)
{
	if (gcn_host)
		delete[]gcn_host;
}

int Rts2AppFw::processOption (int in_opt)
{
	switch (in_opt)
	{
		case 's':
			gcn_host = new char[strlen (optarg) + 1];
			strcpy (gcn_host, optarg);
			break;
		case 'p':
			gcn_port = atoi (optarg);
			break;
		case 'f':
			forwardPort = atoi (optarg);
			break;
		default:
			return rts2core::App::processOption (in_opt);
	}
	return 0;
}

int Rts2AppFw::init ()
{
	int ret;

	ret = rts2core::App::init ();
	if (ret)
		return ret;

	if (!gcn_host || !gcn_port)
	{
		logStream (MESSAGE_ERROR)
			<< "Rts2AppFw::init cannot init - please supply port and gcn hostname"
			<< sendLog;
		return -1;
	}

	// add connection..
	gcncnn = new Rts2ConnFwGrb (gcn_host, gcn_port, this);
	// wait till grb connection init..
	while (1)
	{
		ret = gcncnn->init ();
		if (!ret)
			break;
		logStream (MESSAGE_ERROR)
			<< "Rts2DevGrb::init cannot init conngrb, sleeping for 60 sec"
			<< sendLog;
		sleep (60);
	}
	addConnection (gcncnn);
	// add forward connection
	if (forwardPort > 0)
	{
		int ret2;
		Rts2GrbForwardConnection *forwardConnection;
		forwardConnection = new Rts2GrbForwardConnection (this, forwardPort);
		ret2 = forwardConnection->init ();
		if (ret2)
		{
			logStream (MESSAGE_ERROR)
				<< "Rts2DevGrb::init cannot create forward connection, ignoring ("
				<< ret2 << ")" << sendLog;
			delete forwardConnection;
			forwardConnection = NULL;
		}
		else
		{
			addConnection (forwardConnection);
		}
	}
	return ret;
}

int Rts2AppFw::run ()
{
	int ret;
	ret = init ();
	if (ret)
	{
		std::cerr << "Cannot init app, exiting";
		return ret;
	}
	while (!getEndLoop ())
	{
		oneRunLoop ();
	}
	return 0;
}

int main (int argc, char **argv)
{
	Rts2AppFw grb = Rts2AppFw (argc, argv);
	return grb.run ();
}
