/* 
 * Interface for REMOTES units.
 * Copyright (C) 2015 Petr Kubanek <petr@kubanek.net> and Jan Strobl
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

#ifndef __RTS2_CONNREMOTES__
#define __RTS2_CONNREMOTES__

#include "ethernet.h"

// tokens:
#define	REMOTES_START_TOKEN	0xAA
#define	REMOTES_END_TOKEN	0x55
// block types:
#define	REMOTES_BT_REQUEST	0x01
#define	REMOTES_BT_RESPONSE	0x02
// command types:
#define	REMOTES_CT_DISCOVER	0x01
#define	REMOTES_CT_QUERY	0x02
#define	REMOTES_CT_READ		0x03
#define	REMOTES_CT_WRITE	0x04

namespace rts2core
{

typedef struct 
{
	unsigned char startToken;
	unsigned char blockType;
	unsigned char commandType;
	unsigned char serial;
	unsigned char status;
	unsigned short payloadLength;
	unsigned char headerChecksum;
	unsigned char payload[100];      // includes also PayloadChecksum and EndToken!

} __attribute__((packed)) RemotesCommandBlock;


/**
 * Class for communication with REMOTES units.
 * For details, see e.g. http://jakubec.cz/files/mjakubec-ibws-2012-04-23-introduction-to-remotes-poster.pdf .
 *
 * @author Jan Strobl
 */
class ConnREMOTES:public ConnEthernet
{
	public:
		/**
		 * Create new connection to remote server.
		 *
		 * @param _master	Reference to master holding this connection.
		 * @param _ethLocal 	Identifier (e.g. eth0) of local ethernet device, used for communication.
		 * @param _macRemote 	MAC adress of remote ethernet server, used for communication.
		 */
		ConnREMOTES (rts2core::Block *_master, const char *_ethLocal, const unsigned char *_macRemote);
		virtual ~ ConnREMOTES (void);

		/**
		 * Init connection to host given at constructor.
		 *
		 * @return -1 on error, 0 on success.
		 */
		virtual int init ();

		/**
		 * Set debugging flag. When debugging is enabled, all data send
		 * to and from the socket will be logged using standard RTS2
		 * logging with MESSAGE_DEBUG type.
		 */
		void setDebug (bool _debug = true) { debug = _debug; ConnEthernet::setDebug (_debug);}

		/**
		 * Read data.
		 *
		 * @param registerCode    register number, which will be read
		 * @param readBuf         buffer where read data will be stored
		 * @param readLen         length of the read buffer
		 *
		 * @return size of data read on success.
		 */
		int read (unsigned short registerCode, unsigned char *readBuf, int readLen);

		/**
		 * Write data.
		 *
		 * @param registerCode    register number, which will be changed
		 * @param writeBuf        buffer where data to be sent are stored; when NULL, do only read operation
		 * @param writeLen        length of the write buffer
		 * @param readBuf         buffer where read data (i.e. previous value) will be stored
		 * @param readLen         length of the read buffer
		 *
		 * @return size of data read on success.
		 */
		int write (unsigned short registerCode, unsigned char *writeBuf, int writeLen, unsigned char *readBuf, int readLen);

		/**
		 * Read 1 byte register.
		 *
		 * @param registerCode    register number, which will be read
		 * @param rByte           where read value will be stored
		 *
		 * @return 1 on success
		 */
		int read1b (unsigned short registerCode, unsigned char *rByte);

		/**
		 * Read 2 byte register.
		 *
		 * @param registerCode    register number, which will be read
		 * @param rShort           where read value will be stored
		 *
		 * @return 2 on success
		 */
		int read2b (unsigned short registerCode, unsigned short *rShort);

		/**
		 * Write to 1 byte register.
		 *
		 * @param registerCode    register number, which will be read
		 * @param wByte           value to be written to register
		 * @param rByte           where read (i.e. previous) value will be stored
		 *
		 * @return 1 on success
		 */
		int write1b (unsigned short registerCode, unsigned char wByte, unsigned char *rByte);

		/**
		 * Write to 2 byte register.
		 *
		 * @param registerCode    register number, which will be read
		 * @param wShort          value to be written to register
		 * @param rShort          where read (i.e. previous) value will be stored
		 *
		 * @return 2 on success
		 */
		int write2b (unsigned short registerCode, unsigned short wShort, unsigned short *rShort);


	protected:
		bool getDebug () { return debug; }

	private:
		unsigned char serialNum;
		RemotesCommandBlock remotesReadBuffer;
		RemotesCommandBlock remotesWriteBuffer;
		bool debug;

		/**
		 * Compute the checksum of string.
		 *
		 * @param checkStr	pointer to existing string
		 * @param checkLen	lenghth of string
		 * @param initValue	initial value, added to checksum
		 *
		 * @return computed checksum
		 */
		unsigned char computeChecksum (unsigned char *checkStr, int checkLen, unsigned char initValue = 0);

		/**
		 * Check the checksums, included in CommandBlock.
		 *
		 * @param remCB		checked CommandBlock
		 *
		 * @return 0 on success
		 */
		int checkChecksums (RemotesCommandBlock &remCB);

		/**
		 * Fill in checksums to reserved fields in CommandBlock. Also add the EndToken.
		 *
		 * @param remCB		CommandBlock to be checksummed
		 */
		void writeChecksums (RemotesCommandBlock &remCB);
};


};

#endif // !__RTS2_CONNREMOTES__
