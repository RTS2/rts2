/* 
 * WebSocket daemon.
 * Copyright (C) 2016 Petr Kubanek <petr@kubanek.net>
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

#include "rts2-config.h"
#include "wsd.h"

#ifdef RTS2_HAVE_PGSQL
#include "rts2db/devicedb.h"
#else
#include "device.h"
#endif

int max_poll_elements;

struct lws_pollfd *pollfds;
int *fd_lookup;
int count_pollfds;

int callback_dumb_increment (struct lws *wsi, enum lws_callback_reasons reason, void *user, void *in, size_t len)
{
	unsigned char buf[LWS_PRE + 512];
	struct per_session_data__dumb_increment *pss = (struct per_session_data__dumb_increment *)user;
	unsigned char *p = &buf[LWS_PRE];
	int n, m;

	switch (reason) {

	case LWS_CALLBACK_ESTABLISHED:
		pss->number = 0;
		break;

	case LWS_CALLBACK_SERVER_WRITEABLE:
		n = sprintf((char *)p, "%d", pss->number++);
		m = lws_write(wsi, p, n, LWS_WRITE_TEXT);
		if (m < n)
		{
			logStream (MESSAGE_ERROR) << "ERROR " << n << " writing to di socket" << sendLog;
			return -1;
		}
		break;

	case LWS_CALLBACK_RECEIVE:
		if (len < 6)
			break;
		if (strcmp((const char *)in, "reset\n") == 0)
			pss->number = 0;
		if (strcmp((const char *)in, "closeme\n") == 0) {
			logStream (MESSAGE_INFO) << "dumb_inc: closing as requested" << sendLog;
			lws_close_reason(wsi, LWS_CLOSE_STATUS_GOINGAWAY, (unsigned char *)"seeya", 5);
			return -1;
		}
		break;
	/*
	 * this just demonstrates how to handle
	 * LWS_CALLBACK_WS_PEER_INITIATED_CLOSE and extract the peer's close
	 * code and auxiliary data.  You can just not handle it if you don't
	 * have a use for this.
	 */
	case LWS_CALLBACK_WS_PEER_INITIATED_CLOSE:
		logStream (MESSAGE_INFO) << "LWS_CALLBACK_WS_PEER_INITIATED_CLOSE: len " << len << sendLog;
		for (n = 0; n < (int)len; n++)
			logStream (MESSAGE_INFO) << " " << n << ": " << ((unsigned char*) in)[n] << sendLog;
		break;

	default:
		break;
	}

	return 0;
}

static struct lws_protocols protocols[] = {
	/* first protocol must always be HTTP handler */
	{
		"http-only",		/* name */
		callback_http,		/* callback */
		sizeof (struct per_session_data__http),	/* per_session_data_size */
		0			/* max frame size / rx buffer */
	},
	{
		"dumb-increment-protocol",
		callback_dumb_increment,
		sizeof(struct per_session_data__dumb_increment),
		10
	}
};

/**
 * Websocket access daemon.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
#ifdef RTS2_HAVE_PGSQL
class WsD:public rts2db::DeviceDb
#else
class WsD:public rts2core::Device
#endif
{
	public:
		WsD (int argc, char **argv);
		virtual ~WsD ();

	protected:
		virtual int processOption (int opt);
		virtual int initHardware ();
#ifndef RTS2_HAVE_PGSQL
		virtual int willConnect (rts2core::NetworkAddress * _addr);
#endif

	private:
		struct lws_context_creation_info info;
		struct lws_context *context;
};

#ifdef RTS2_HAVE_PGSQL
WsD::WsD (int argc, char **argv):rts2db::DeviceDb (argc, argv, DEVICE_TYPE_HTTPD, "WSD")
#else
WsD::WsD (int argc, char **argv):rts2core::Device (argc, argv, DEVICE_TYPE_HTTPD, "WSD")
#endif
{
	memset (&info, 0, sizeof info);
	info.port = 8888;

	context = NULL;

	addOption ('p', NULL, 1, "websocket port. Default to 8888");
}

WsD::~WsD()
{
	lws_context_destroy (context);
}

int WsD::processOption (int opt)
{
	switch (opt)
	{
		case 'p':
			info.port = atoi (optarg);
			break;
		default:
#ifdef RTS2_HAVE_PGSQL
			return DeviceDb::processOption (opt);
#else
			return Device::processOption (opt);
#endif
	}
	return 0;
}

int WsD::initHardware ()
{
	info.protocols = protocols;

	info.gid = -1;
	info.uid = -1;
	info.max_http_header_pool = 16;
	info.options = LWS_SERVER_OPTION_ALLOW_NON_SSL_ON_SSL_PORT | LWS_SERVER_OPTION_VALIDATE_UTF8;
	info.extensions = NULL;
	info.timeout_secs = 5;
	info.ssl_cipher_list = "ECDHE-ECDSA-AES256-GCM-SHA384:"
			       "ECDHE-RSA-AES256-GCM-SHA384:"
			       "DHE-RSA-AES256-GCM-SHA384:"
			       "ECDHE-RSA-AES256-SHA384:"
			       "HIGH:!aNULL:!eNULL:!EXPORT:"
			       "!DES:!MD5:!PSK:!RC4:!HMAC_SHA1:"
			       "!SHA1:!DHE-RSA-AES128-GCM-SHA256:"
			       "!DHE-RSA-AES128-SHA256:"
			       "!AES128-GCM-SHA256:"
			       "!AES128-SHA256:"
			       "!DHE-RSA-AES256-SHA256:"
			       "!AES256-GCM-SHA384:"
			       "!AES256-SHA256";
	context = lws_create_context(&info);
	if (context == NULL)
	{
		logStream (MESSAGE_ERROR) << "cannot create libwebsocket context" << sendLog;
		return -1;
	}

	return 0;
}

#ifndef RTS2_HAVE_PGSQL
int WsD::willConnect (rts2core::NetworkAddress *_addr)
{
	if (_addr->getType () < getDeviceType ()
		|| (_addr->getType () == getDeviceType ()
		&& strcmp (_addr->getName (), getDeviceName ()) < 0))
		return 1;
	return 0;
}
#endif

int main (int argc, char **argv)
{
	WsD device (argc, argv);
	return device.run ();
}
