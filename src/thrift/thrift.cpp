/*
 * Thrift client.
 * Copyright (C) 2017 Petr Kubanek <petr@kubanek.net>
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

#include "ObservatoryService.h"

#include <thrift/transport/TSocket.h>
#include <thrift/transport/TBufferTransports.h>
#include <thrift/protocol/TBinaryProtocol.h>

using namespace apache::thrift;
using namespace apache::thrift::protocol;
using namespace apache::thrift::transport;

using namespace ::rts2;

int main (int argc, char **argv)
{
	boost::shared_ptr<TSocket> socket(new TSocket("localhost", 9093));
	boost::shared_ptr<TTransport> transport(new TBufferedTransport(socket));
	boost::shared_ptr<TProtocol> protocol(new TBinaryProtocol(transport));

	ObservatoryServiceClient client(protocol);
	transport->open();

	MountInfo mi;

	client.infoMount(mi);

	printf ("Mount RA: %.5f DEC %.5f\n", mi.TEL.ra, mi.TEL.dec);

	client.DomeLights(1);
	printf ("Lights on\n");

	sleep (10);

	client.DomeLights(0);
	printf ("Lights off\n");

	transport->close();

	return 0;
}
