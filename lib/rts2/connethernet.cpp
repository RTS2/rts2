/* 
 * Simple connection using RAW ethernet (IEEE 802.3).
 * Copyright (C) 2015 Petr Kubanek <petr@kubanek.net> and Jan Strobl.
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

#include "connection/ethernet.h"


using namespace rts2core;

ConnEthernet::ConnEthernet (rts2core::Block *_master, const char *_ethLocal, const unsigned char *_macRemote):ConnNoSend (_master)
{
	strcpy (ethLocal, _ethLocal);
	memcpy (macRemote, _macRemote, 6);
	debug = false;
}

ConnEthernet::~ConnEthernet ()
{
	close (sockE);
	free (ethInBuffer);
	free (ethOutBuffer);
}

int ConnEthernet::init ()
{
	// check, if the MACs were really set
	if (strlen (ethLocal) == 0 || memcmp (macRemote, "\0\0\0\0\0\0", 6) == 0)
	{
		logStream (MESSAGE_ERROR) << "ConnEthernet::init : eth or MAC not configured" << sendLog;
		return -1;
	}

	// let's start with opening the socket...
	sockE = socket (AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
        if (sockE == -1)
	{
		logStream (MESSAGE_ERROR) << "ConnEthernet::writeRead socket: " << strerror (errno) << sendLog;
		return -1;
	}

	// bind socket to just our network adapter
	if (setsockopt(sockE, SOL_SOCKET, SO_BINDTODEVICE, ethLocal, strlen(ethLocal)) == -1)	{
		logStream (MESSAGE_ERROR) << "ConnEthernet::writeRead setsockopt: " << strerror (errno) << sendLog;
		close (sockE);
		return -1;
	}

	// get MAC of local interface
	struct ifreq ifr;
	memset(&ifr, 0, sizeof(struct ifreq));
	strcpy(ifr.ifr_name, ethLocal);
	if (ioctl(sockE, SIOCGIFHWADDR, &ifr) == -1) {
		logStream (MESSAGE_ERROR) << "ConnEthernet::init ioctl SIOCGIFHWADDR:" << strerror (errno) << "... maybe need of root privileges?" << sendLog;
		return -1;
	}
	memcpy(macLocal, ifr.ifr_hwaddr.sa_data, 6);

	// buffer for ethernet frame
	ethOutBuffer = (void*)malloc(ETH_FRAME_LEN);
	ethInBuffer = (void*)malloc(ETH_FRAME_LEN);

	// pointer to ethenet header
	ethOutHead = (unsigned char*)ethOutBuffer;

	// userdata in ethernet frame
	ethOutData = (unsigned char*)ethOutBuffer + 14;
	ethInData = (unsigned char*)ethInBuffer + 14;

	// another pointer to ethernet header
	eh = (struct ethhdr *)ethOutHead;

	// setup RAW ethernet communication
	socket_address.sll_family   = PF_PACKET;
	// we don't use a protocoll above ethernet layer -> just use anything here
	socket_address.sll_protocol = htons(ETH_P_IP);
	// index of the network device
	socket_address.sll_ifindex  = 2;
	// ARP hardware identifier is ethernet
	socket_address.sll_hatype   = ARPHRD_ETHER;
	// target is another host
	socket_address.sll_pkttype  = PACKET_OTHERHOST;
	// address length
	socket_address.sll_halen    = ETH_ALEN;		
	// destination MAC - not used for packet construction in RAW mode, but...
	memcpy (socket_address.sll_addr, macRemote, 6);
	socket_address.sll_addr[6]  = 0x00;/*not used*/
	socket_address.sll_addr[7]  = 0x00;/*not used*/

	// set the frame header
	memcpy (eh->h_dest, macRemote, 6);
	memcpy (eh->h_source, macLocal, 6);
	eh->h_proto = 0xffff;

        return 0;
}

int ConnEthernet::writeRead (const void* wbuf, int wlen, void *rbuf, int rlen, int wtime)
{
	int ret;
	int packetLength;
	struct timeval readTout;
	time_t readToutDline;
	fd_set read_set;

        if (wlen > ETH_FRAME_LEN - 14)
	{
		logStream (MESSAGE_ERROR) << "ConnEthernet::writeRead - too long wbuf!" << sendLog;
		return -1;
	}

	// empty read buffer in socket (remove accumulated unrelated traffic)
	while (recvfrom (sockE, ethInBuffer, ETH_FRAME_LEN, MSG_DONTWAIT, NULL, NULL) > 0) {}

	// zero buffers for clean usage
	memset (ethOutData, 0, ETH_FRAME_LEN - 14);
	memset (ethInBuffer, 0, ETH_FRAME_LEN);

	memcpy (ethOutData, wbuf, wlen);

	// value of ETH_FRAME_LEN should safely work, but still, we want to spare traffic and load
	if (wlen < 46)
		packetLength = 46+14;
	else
		packetLength = wlen+14;

	if (debug)
	{
		LogStream ls = logStream (MESSAGE_DEBUG);
		ls << "send ";
		ls.logArrAsHex ((char *)ethOutBuffer, packetLength);
		ls << sendLog;
	}

	// OK, finally, send it!
	ret = sendto (sockE, ethOutBuffer, packetLength, 0, (struct sockaddr*)&socket_address, sizeof(socket_address));
	if (ret == -1) 
	{
		logStream (MESSAGE_ERROR) << "ConnEthernet::writeRead sendto: " << strerror (errno) << sendLog;
		return -1;
	}

	// and now take care of response...
	readTout.tv_sec = wtime;
	readTout.tv_usec = 0;
	readToutDline = time (0) + wtime;

	// docasna testovaci nahrada za select:
	usleep (100000);

	while (true)
	{
		FD_ZERO (&read_set);
		FD_SET (sockE, &read_set);

		// docasna testovaci nahrada za select:
		usleep (10000);

		/*ret = select (FD_SETSIZE, &read_set, NULL, NULL, &readTout);
		if (ret < 0)
		{
			logStream (MESSAGE_ERROR) << "ConnEthernet::writeRead : error calling select function : " << strerror (errno) << sendLog;
			return -1;
		}
		else if (ret == 0 || time (0) > readToutDline)
		{
			logStream (MESSAGE_ERROR) << "ConnEthernet::writeRead : zero response or timeout reached : " << strerror (errno) << sendLog;
			return -1;
		}*/
		if (time (0) > readToutDline)
		{
			logStream (MESSAGE_ERROR) << "ConnEthernet::writeRead : timeout reached (without select!) : " << strerror (errno) << sendLog;
			return -1;
		}


		//packetLength = recvfrom (sockE, ethInBuffer, ETH_FRAME_LEN, 0, NULL, NULL);
		packetLength = recvfrom (sockE, ethInBuffer, ETH_FRAME_LEN, MSG_DONTWAIT, NULL, NULL);
		if (packetLength == -1)
		{
			logStream (MESSAGE_ERROR) << "ConnEthernet::writeRead recvfrom: " << strerror (errno) << sendLog;
			return -1;
		}

		//if (memcmp ((unsigned char*)ethInBuffer + 6, macRemote, 6) == 0 && memcmp ((unsigned char*)ethInBuffer, macLocal, 6) == 0)
		if (packetLength > 0 && memcmp ((unsigned char*)ethInBuffer + 6, macRemote, 6) == 0 && memcmp ((unsigned char*)ethInBuffer, macLocal, 6) == 0)
		{
			// we have our response!
			break;
		}

		// false alert, let's try it again...
		memset (ethInBuffer, 0, packetLength);
	}

	if (debug)
	{
		LogStream ls = logStream (MESSAGE_DEBUG);
		ls << "recv ";
		ls.logArrAsHex ((char *)ethInBuffer, packetLength);
		ls << sendLog;
	}

	if (packetLength - 14 > rlen)
	{
			logStream (MESSAGE_ERROR) << "ConnEthernet::writeRead - recieved data are longer than rbuf buffer!" << sendLog;
			return -1;
	}

	memcpy (rbuf, ethInData, packetLength - 14);

	return packetLength - 14;
}


