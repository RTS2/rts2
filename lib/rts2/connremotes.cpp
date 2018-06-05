/* 
 * Interface for REMOTES units.
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

#include "connection/remotes.h"


using namespace rts2core;

ConnREMOTES::ConnREMOTES (rts2core::Block *_master, const char *_ethLocal, const unsigned char *_macRemote):ConnEthernet (_master, _ethLocal, _macRemote)
{
	debug = false;
}

ConnREMOTES::~ConnREMOTES ()
{
}

int ConnREMOTES::init ()
{
	serialNum = (unsigned char) 0;
	ConnEthernet::init ();

        return 0;
}


int ConnREMOTES::read (unsigned short registerCode, unsigned char *readBuf, int readLen)
{
	return write (registerCode, NULL, 0, readBuf, readLen);
}


int ConnREMOTES::write (unsigned short registerCode, unsigned char *writeBuf, int writeLen, unsigned char *readBuf, int readLen)
{
	int ret;
	unsigned short payloadLength, payloadDataSize, usVar;
	serialNum ++;

	remotesWriteBuffer.startToken = REMOTES_START_TOKEN;
	remotesWriteBuffer.blockType = REMOTES_BT_REQUEST;
	remotesWriteBuffer.serial = serialNum;
	remotesWriteBuffer.status = 0;
	usVar = htons (registerCode);

	memcpy (remotesWriteBuffer.payload, &usVar, 2);

	if (writeBuf==NULL)	// only "read" operation
	{
		remotesWriteBuffer.commandType = REMOTES_CT_READ;
		payloadLength = 2;
	}
	else
	{
		remotesWriteBuffer.commandType = REMOTES_CT_WRITE;
		payloadLength = 4 + writeLen;
		usVar =  htons ((unsigned short) writeLen);
		memcpy (remotesWriteBuffer.payload + 2, &usVar, 2);
		memcpy (remotesWriteBuffer.payload + 4, (const void *) writeBuf, writeLen);
	}

	remotesWriteBuffer.payloadLength = htons (payloadLength);

	writeChecksums (remotesWriteBuffer);

	// raw ethernet is unreliable, we have to deal with it
	for (int i=0; i<5; i++)
	{
		ret = ConnEthernet::writeRead ((const void *) &remotesWriteBuffer, 10 + payloadLength, (void *) &remotesReadBuffer, sizeof (RemotesCommandBlock));
		if (ret > 0)
			break;
	}
	if (ret <= 0)
	{
		logStream (MESSAGE_ERROR) << "ConnREMOTES::write writeRead permanent error: " << strerror (errno) << sendLog;
		return -1;
	}

	// now several checks of consistency and relateness of response...
	if (remotesReadBuffer.startToken != REMOTES_START_TOKEN || remotesReadBuffer.blockType != REMOTES_BT_RESPONSE)
	{
		logStream (MESSAGE_ERROR) << "ConnREMOTES::write : response with wrong tokens!" << sendLog;
		return -1;
	}

	if (remotesReadBuffer.serial != serialNum)
	{
		logStream (MESSAGE_ERROR) << "ConnREMOTES::write : response with wrong serial num: " << (int) remotesReadBuffer.serial << " instead of " << (int) serialNum << sendLog;
		return -1;
	}

	if (remotesReadBuffer.status != 0)
	{
		logStream (MESSAGE_ERROR) << "ConnREMOTES::write : response nonzero status: " << (int) remotesReadBuffer.status << sendLog;
		return -1;
	}

	payloadLength = ntohs (remotesReadBuffer.payloadLength);
	if (remotesReadBuffer.payload[payloadLength + 1] != REMOTES_END_TOKEN)
	{
		logStream (MESSAGE_ERROR) << "ConnREMOTES::write : EndToken not in place in response! " << sendLog;
		return -1;
	}

	if (checkChecksums (remotesReadBuffer) != 0)
	{
		logStream (MESSAGE_ERROR) << "ConnREMOTES::write : response checksums error!" << sendLog;
		return -1;
	}

	// also check the payload itself...
	if (remotesReadBuffer.payload[0] != 0)	// result of "command"
	{
		logStream (MESSAGE_ERROR) << "ConnREMOTES::write : response: nonzero result of command: " << (int) remotesReadBuffer.payload[0] << sendLog;
		return -1;
	}

	payloadDataSize = remotesReadBuffer.payload[1];
	payloadDataSize <<= 8;
	payloadDataSize |= remotesReadBuffer.payload[2];

	if (payloadDataSize + 3 > payloadLength)
	{
		logStream (MESSAGE_ERROR) << "ConnREMOTES::write : response data size in payload bigger than payloadLength?!? " << payloadDataSize << " vs " << (int) payloadLength << sendLog;
		return -1;
	}

	if (payloadDataSize > readLen)
	{
		logStream (MESSAGE_ERROR) << "ConnREMOTES::write : response longer than read buffer: " << (int) payloadDataSize << " > " << readLen << sendLog;
		return -1;
	}

	memcpy (readBuf, remotesReadBuffer.payload + 3, payloadDataSize);
	return payloadDataSize;
}


int ConnREMOTES::read1b (unsigned short registerCode, unsigned char *rByte)
{
	return read (registerCode, rByte, 1);
}

int ConnREMOTES::read2b (unsigned short registerCode, unsigned short *rShort)
{
	int ret;
	unsigned short usVar;
	ret = read (registerCode, (unsigned char *) &usVar, 2);
	if (ret != 2)
	{
		logStream (MESSAGE_ERROR) << "ConnREMOTES::read2b : response has bad length? " << ret << sendLog;
		return -1;
	}
	*rShort = ntohs (usVar);
	return ret;
}

int ConnREMOTES::write1b (unsigned short registerCode, unsigned char wByte, unsigned char *rByte)
{
	return write (registerCode, &wByte, 1, rByte, 1);
}

int ConnREMOTES::write2b (unsigned short registerCode, unsigned short wShort, unsigned short *rShort)
{
	int ret;
	unsigned short usVar;
	unsigned char buff[2];
	usVar = htons (wShort);
	memcpy (buff, &usVar, 2);
	ret = write (registerCode, buff, 2, (unsigned char *) &usVar, 2);
	if (ret != 2)
	{
		logStream (MESSAGE_ERROR) << "ConnREMOTES::write2b : response has bad length? " << ret << sendLog;
		return -1;
	}
	*rShort = ntohs (usVar);
	return ret;
}

unsigned char ConnREMOTES::computeChecksum (unsigned char *checkStr, int checkLen, unsigned char initValue)
{
	unsigned char val = initValue;

	for (int i = 0; i < checkLen; i++)
		val += checkStr[i];

	return (unsigned char)(~val + 1);
}

int ConnREMOTES::checkChecksums (RemotesCommandBlock &remCB)
{
	unsigned short payloadLength;
	payloadLength = ntohs (remCB.payloadLength);
	if (computeChecksum ((unsigned char *) &remCB, 7) != remCB.headerChecksum || computeChecksum ((unsigned char *) remCB.payload, payloadLength, REMOTES_END_TOKEN) != remCB.payload[payloadLength])
		return -1;

	return 0;
}

void ConnREMOTES::writeChecksums (RemotesCommandBlock &remCB)
{
	unsigned short payloadLength;
	payloadLength = ntohs (remCB.payloadLength);
	remCB.headerChecksum = computeChecksum ((unsigned char *) &remCB, 7);
	remCB.payload[payloadLength] = computeChecksum ((unsigned char *) remCB.payload, payloadLength, REMOTES_END_TOKEN);
	remCB.payload[payloadLength + 1] = REMOTES_END_TOKEN;
}



