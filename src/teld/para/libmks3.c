#include "libmks3.h"

#include <stdio.h>
#include <math.h>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <ctype.h>

#define debugCharOut(c) fprintf(stderr,"%c",c)
#define debugByteOut(c) fprintf(stderr,"%02X",c)
#define debugWordOut(c) fprintf(stderr,"%04X",c)
#define debugDWordOut(c) fprintf(stderr,"%08lX",c)
#define debugStrOut(c) fprintf(stderr,"%s",c)

int hComm;
int _comstat = 2;

/* mks3comm.c*/
CWORD16
commDecodePacket (CWORD16 * pBuff, CWORD16 len, packet_t * pPack)
{
  CWORD16 checksum = 0;
  CWORD16 index;
  CWORD16 wantChecksum;
  if (len < COMM_OVERHEADLEN)
    {
/*	debugCharOut('L');*/
      return COMM_BADLEN;
    }
  for (index = 0; index < len - 2; index++)
    checksum += pBuff[index];
  wantChecksum = PACKETUINT (pBuff, len - 2);
  if (checksum != wantChecksum)
    debugStrOut ("Bad Checksum\r\n");

/*    printf("bad checksum\n");
    return COMM_BADCHECKSUM;  */
  pPack->packetLen = PACKETUINT (pBuff, 4);
  if (pPack->packetLen != len)
    {
/*	debugCharOut('X'); */
      return COMM_BADLEN;
    }
  pPack->unitId = PACKETUINT (pBuff, 0);
  pPack->axisId = pBuff[2];
  pPack->sequenceNum = pBuff[3];
  pPack->command = PACKETUINT (pBuff, 6);
  pPack->pData = pBuff;
  return COMM_OKPACKET;
}


/* user data needs to be already filled in */
CWORD16
commSendPack (packet_t * pPack)
{
  CWORD16 checksum = 0;
  CWORD16 index;
  CWORD16 outChar;
  pPack->pData[0] = PACKLO (pPack->unitId);
  pPack->pData[1] = PACKHI (pPack->unitId);
  pPack->pData[2] = pPack->axisId;
  pPack->pData[3] = pPack->sequenceNum;
  pPack->pData[4] = PACKLO (pPack->packetLen);
  pPack->pData[5] = PACKHI (pPack->packetLen);
  pPack->pData[6] = PACKLO (pPack->command);
  pPack->pData[7] = PACKHI (pPack->command);
  for (index = 0; index < pPack->packetLen - 2; index++)
    checksum += pPack->pData[index];
  pPack->pData[index++] = PACKLO (checksum);
  pPack->pData[index++] = PACKHI (checksum);
  for (index = 0; index < pPack->packetLen; index++)
    {
      outChar = pPack->pData[index];
      commPutChar (outChar);
      if (outChar == COMM_ENDPACKET)
	commPutChar (outChar);
    }
  commPutChar (COMM_ENDPACKET);
  commPutChar (COMM_ENDPACKET2);
  return COMM_OKPACKET;
}

CWORD16
commGetPacket (CWORD16 * packetBuff, CWORD16 * pReadLen, CWORD16 maxLen)
{
  CWORD16 packetLen;
  CWORD16 charIn;
  CWORD16 maybeEnd;
  *pReadLen = 0;
  if (!commCheckChar ())
    {
      return COMM_NOPACKET;
    }
  packetLen = 0;
  maybeEnd = 0;
  while (packetLen < maxLen)
    {
      charIn = commGetChar (200);

/*    debugByteOut(charIn); */
      if (charIn & 0x100)
	{
/*	    debugCharOut('T');*/
	  return COMM_TIMEOUT;
	}
      if (charIn & 0x200)
	{
/*	    debugCharOut('E');*/
	  return COMM_COMMERROR;
	}
      if (maybeEnd && (charIn != COMM_ENDPACKET))
	{
	  if (charIn != COMM_ENDPACKET2)
	    return COMM_BADCHAR;
	  *pReadLen = packetLen;
/*	    debugCharOut('P');*/
	  return COMM_OKPACKET;
	}

      else if ((charIn != COMM_ENDPACKET)
	       || (maybeEnd && (charIn == COMM_ENDPACKET)))
	{

/*      debugCharOut('C'); */
	  maybeEnd = 0;
	  packetBuff[packetLen] = charIn;
	  packetLen++;
	}

      else if (!maybeEnd)
	{

/*       debugCharOut('M'); */
	  maybeEnd = 1;
	}
    }
  return COMM_OVERRUN;
}

/* mks3cwin.c*/
struct termios tts_oldstate, tts_newstate;
int commPrintFlag = 0;
unsigned long mksCommStat;
unsigned long mksCommWord;

CWORD16
commInit (char *port)
{

  hComm = open (port, O_RDWR);

  if (hComm == -1)
    {
      fprintf (stderr, "No puedo abrir %s\n", port);
      return COMM_INITFAIL;
    }

  tcgetattr (hComm, &tts_oldstate);

  tts_newstate.c_iflag = IGNBRK | IGNPAR;
  tts_newstate.c_oflag = 0;	/*OPOST | CRDLY | BSDLY; */
  tts_newstate.c_cflag = CS8 | CLOCAL | CREAD | CSTOPB;
  tts_newstate.c_lflag = 0;

  tts_newstate.c_cc[VTIME] = 2;
  tts_newstate.c_cc[VMIN] = 0;

  cfsetospeed (&tts_newstate, B38400);
  cfsetispeed (&tts_newstate, B38400);

  tcflush (hComm, TCIOFLUSH);
  tcsetattr (hComm, TCSANOW, &tts_newstate);

  /*  bStat=BuildCommDCB("baud=38400 parity=N data=8 stop=1 xon=off odsr=off octs=off dtr=off rts=off idsr=off", &theDCB); */

  return MKS_OK;
}

CWORD16
commCheckChar ()
{
  return 1;
}

CWORD16
commGetChar (CWORD16 maxTimeout)
{				/* timeout in ticks (at 5.3KHz) */
  unsigned char getChar;
  unsigned long red;
  /*unsigned long err; */
  /*  ssize_t read(int fd, void *buf, size_t count); */
  red = read (hComm, &getChar, 1);
  /*bStat = ReadFile(hComm, &getChar, 1, &red, NULL); */
  if (red != 1)
    {
/*      err = GetLastError();*/
/*      debugDWordOut(err);*/
/*      ClearCommError(hComm, &mksCommWord, &mksCommStat);*/
/*      printf("@");*/
      return 0x100;
    }
/*    if (commPrintFlag) {
	debugByteOut(getChar);
	if (isascii(getChar) && isprint(getChar)) {
	    debugStrOut(": ");
	    debugCharOut(getChar);
	}
	debugStrOut("\n");
    } */

  /*  printf("(%x)", (int) getChar); */

  return (CWORD16) getChar;
}

CWORD16
commPutChar (CWORD16 rr)
{
  unsigned char putChar;
  /*unsigned long rote; */
  /*unsigned long err; */

/*    _getch();*/
  putChar = (unsigned char) rr;
  /*  if (commPrintFlag) { */
/*      debugByteOut(putChar);*/
/*      if (isascii(putChar) && isprint(putChar)) {*/
/*          debugStrOut(": ");*/
/*          debugCharOut(putChar);*/
/*      }*/
/*      debugStrOut("\n");*/
  /*  } */
  /*if (!WriteFile(hComm, &putChar, 1, &rote, NULL)) { */
  if (write (hComm, &putChar, 1) < 1)
    {
/*      err = GetLastError();*/
/*      debugStrOut("comm write error: ");*/
/*      debugDWordOut(err);*/
/*      debugStrOut("\n");*/
/*      ClearCommError(hComm, &mksCommWord, &mksCommStat);*/
    }
/*    printf("<%x>", (int) putChar);*/
  return COMM_OKPACKET;
}

void
makeStatPack (packet_t * pPack)
{
  pPack->packetLen = COMM_OVERHEADLEN;
  pPack->command = CMD_STATUS;
}

void
makeVersionPack (packet_t * pPack)
{
  pPack->packetLen = COMM_OVERHEADLEN;
  pPack->command = CMD_VERSION;
}

void
makeErasePack (packet_t * pPack, CWORD16 address)
{
  pPack->packetLen = COMM_OVERHEADLEN + CMD_FLASH_ERASE_DATA_LEN;
  pPack->command = CMD_FLASH_ERASE;
  pPack->pData[COMM_HEADERLEN + 0] = PACKHI (address);
}

void
makeProgramPacket (packet_t * pPack, CWORD16 start, CWORD16 end,
		   CWORD16 * pWData)
{
  unsigned len = end - start + 1;
  unsigned bIndex = 0;
  unsigned wIndex = 0;
  pPack->packetLen =
      COMM_OVERHEADLEN + CMD_FLASH_WRITE_DATA_OFFSET + 2 * len;
  pPack->command = CMD_FLASH_WRITE;
  pPack->pData[COMM_HEADERLEN + 0] = PACKLO (start);
  pPack->pData[COMM_HEADERLEN + 1] = PACKHI (start);
  pPack->pData[COMM_HEADERLEN + 2] = PACKLO (end);
  pPack->pData[COMM_HEADERLEN + 3] = PACKHI (end);
  for (wIndex = 0, bIndex = 0; wIndex < len; wIndex++, bIndex += 2)
    {
      pPack->pData[COMM_HEADERLEN + CMD_FLASH_WRITE_DATA_OFFSET +
		   bIndex] = PACKLO (pWData[wIndex]);
      pPack->pData[COMM_HEADERLEN + CMD_FLASH_WRITE_DATA_OFFSET +
		   bIndex + 1] = PACKHI (pWData[wIndex]);
    }
}

void
makeProgramBytePacket (packet_t * pPack, CWORD16 start, CWORD16 end,
		       unsigned char *pBData)
{
  unsigned len = end - start + 1;
  unsigned bIndex = 0;
  pPack->packetLen =
      COMM_OVERHEADLEN + CMD_FLASH_WRITE_DATA_OFFSET + 2 * len;
  pPack->command = CMD_FLASH_WRITE;
  pPack->pData[COMM_HEADERLEN + 0] = PACKLO (start);
  pPack->pData[COMM_HEADERLEN + 1] = PACKHI (start);
  pPack->pData[COMM_HEADERLEN + 2] = PACKLO (end);
  pPack->pData[COMM_HEADERLEN + 3] = PACKHI (end);
  for (bIndex = 0; bIndex < 2 * len; bIndex++)
    {
      pPack->pData[COMM_HEADERLEN + CMD_FLASH_WRITE_DATA_OFFSET +
		   bIndex] = pBData[bIndex];
    }
}

void
makeFlashReadPacket (packet_t * pPack, CWORD16 start, CWORD16 end)
{
  pPack->packetLen = COMM_OVERHEADLEN + CMD_FLASH_READ_DATA_LEN;
  pPack->command = CMD_FLASH_READ;
  pPack->pData[COMM_HEADERLEN + 0] = PACKLO (start);
  pPack->pData[COMM_HEADERLEN + 1] = PACKHI (start);
  pPack->pData[COMM_HEADERLEN + 2] = PACKLO (end);
  pPack->pData[COMM_HEADERLEN + 3] = PACKHI (end);
}

void
makeRamWritePacket (packet_t * pPack, CWORD16 start,
		    CWORD16 end, CWORD16 * pWData)
{
  unsigned len = end - start + 1;
  unsigned bIndex = 0;
  unsigned wIndex = 0;
  pPack->packetLen =
      COMM_OVERHEADLEN + CMD_RAM_WRITE_DATA_OFFSET + 2 * len;
  pPack->command = CMD_RAM_WRITE;
  pPack->pData[COMM_HEADERLEN + 0] = PACKLO (start);
  pPack->pData[COMM_HEADERLEN + 1] = PACKHI (start);
  pPack->pData[COMM_HEADERLEN + 2] = PACKLO (end);
  pPack->pData[COMM_HEADERLEN + 3] = PACKHI (end);
  for (wIndex = 0, bIndex = 0; wIndex < len; wIndex++, bIndex += 2)
    {
      pPack->pData[COMM_HEADERLEN + CMD_RAM_WRITE_DATA_OFFSET +
		   bIndex] = PACKLO (pWData[wIndex]);
      pPack->pData[COMM_HEADERLEN + CMD_RAM_WRITE_DATA_OFFSET + bIndex +
		   1] = PACKHI (pWData[wIndex]);
    }
}

void
makeRamReadPacket (packet_t * pPack, CWORD16 start, CWORD16 end)
{
  pPack->packetLen = COMM_OVERHEADLEN + CMD_RAM_READ_DATA_LEN;
  pPack->command = CMD_RAM_READ;
  pPack->pData[COMM_HEADERLEN + 0] = PACKLO (start);
  pPack->pData[COMM_HEADERLEN + 1] = PACKHI (start);
  pPack->pData[COMM_HEADERLEN + 2] = PACKLO (end);
  pPack->pData[COMM_HEADERLEN + 3] = PACKHI (end);
}

void
addCWORD32 (packet_t * pPack, int offset, CWORD32 val32)
{
  pPack->pData[COMM_HEADERLEN + offset] = PACKLO (val32);
  pPack->pData[COMM_HEADERLEN + offset + 1] = PACKHI (val32);
  pPack->pData[COMM_HEADERLEN + offset + 2] = PACKLO (val32 >> 16);
  pPack->pData[COMM_HEADERLEN + offset + 3] = PACKHI (val32 >> 16);
}

void
makeSetVal32Pack (packet_t * pPack, CWORD16 valId, CWORD32 val32)
{
  pPack->packetLen = COMM_OVERHEADLEN + CMD_SET_VAL32_DATA_LEN;
  pPack->command = CMD_SET_VAL32;
  pPack->pData[COMM_HEADERLEN + 0] = PACKLO (valId);
  pPack->pData[COMM_HEADERLEN + 1] = PACKHI (valId);
  addCWORD32 (pPack, 2, val32);
}

void
makeGetVal32Pack (packet_t * pPack, CWORD16 valId)
{
  pPack->packetLen = COMM_OVERHEADLEN + CMD_GET_VAL32_DATA_LEN;
  pPack->command = CMD_GET_VAL32;
  pPack->pData[COMM_HEADERLEN + 0] = PACKLO (valId);
  pPack->pData[COMM_HEADERLEN + 1] = PACKHI (valId);
}

void
makeSetVal16Pack (packet_t * pPack, CWORD16 valId, CWORD16 val16)
{
  pPack->packetLen = COMM_OVERHEADLEN + CMD_SET_VAL16_DATA_LEN;
  pPack->command = CMD_SET_VAL16;
  pPack->pData[COMM_HEADERLEN + 0] = PACKLO (valId);
  pPack->pData[COMM_HEADERLEN + 1] = PACKHI (valId);
  pPack->pData[COMM_HEADERLEN + 2] = PACKLO (val16);
  pPack->pData[COMM_HEADERLEN + 3] = PACKHI (val16);
}

void
makeGetVal16Pack (packet_t * pPack, CWORD16 valId)
{
  pPack->packetLen = COMM_OVERHEADLEN + CMD_GET_VAL16_DATA_LEN;
  pPack->command = CMD_GET_VAL16;
  pPack->pData[COMM_HEADERLEN + 0] = PACKLO (valId);
  pPack->pData[COMM_HEADERLEN + 1] = PACKHI (valId);
}

int
XCpacket (packet_t * pPack, packet_t * pRespPacket)
{
  /*int xx; */
  CWORD16 stat;
  CWORD16 len;

  tcflush (hComm, TCOFLUSH);

  /*PurgeComm(hComm, /-*PURGE_RXCLEAR  == *-/ 0); */
  stat = commSendPack (pPack);
/*    printf("send stat=%d\n", stat);*/
/*    debugStrOut("send stat=");*/
/*    debugWordOut(stat);*/
/*    debugStrOut("\n");*/
  stat = commGetPacket (pPack->pData, &len, COMM_MAX_PACKET);
/*    printf("get stat=%d, len=%d\n", stat, len);*/
/*    if (commPrintFlag) {
	for (xx = 0; xx < len; xx++) {
	    printf(":%02X", pPack->pData[xx]);
	    if (isascii(pPack->pData[xx]) && isprint(pPack->pData[xx]))
		printf(" %c\n", pPack->pData[xx]);

	    else
		printf(" .\n");
	}
    } */
  if (stat == COMM_OKPACKET)
    {
      stat = commDecodePacket (pPack->pData, len, pRespPacket);
/*      printf("decode stat=%d\n", stat);*/
      if (pRespPacket->command == CMD_ACK)
	{
/*          printf("Ack\n");*/
	}
      else if (pRespPacket->command == CMD_NACK)
	{
	  stat = PACKETUINT (pRespPacket->pData, COMM_HEADERLEN);
	  printf ("Nack: stat=%d\n", stat);
	}

      else
	{
	  fprintf (stderr, "packet command: %d\n", pRespPacket->command);
	  stat = COMM_COMMERROR;
	}
    }

//    else fprintf(stderr,"packet receive error: %d\n", stat);

  if (stat == COMM_OKPACKET)
    stat = MKS_OK;
  return stat;
}

/* mks3000.c*/
/*static CRITICAL_SECTION _MKSCritSection;*/
size_t
_MKS3PacketInit (packet_t * pPack, CWORD16 * pBuff, const MKS3Id Id)
{
  static size_t sequenceNum = 0;
  /*char messBuff[512]; */

/*  EnterCriticalSection(&_MKSCritSection);*/
  pPack->unitId = Id.unitId;
  pPack->axisId = Id.axisId;
  pPack->sequenceNum = sequenceNum;
  pPack->pData = pBuff;
  sequenceNum++;
  sequenceNum &= 0xFF;

/*  LeaveCriticalSection(&_MKSCritSection);*/
/*    sprintf(messBuff, "Tx sequence num=%d\n", pPack->sequenceNum);
    debugStrOut(messBuff); */
  return pPack->sequenceNum;
}

int
_MKSXCpacket (packet_t * pPack, packet_t * pRespPacket)
{
/*  EnterCriticalSection(&_MKSCritSection);*/
  int stat = XCpacket (pPack, pRespPacket);

  if (stat == MKS_OK && _comstat != 1)
    {
      printf ("XCpacket: connection reached\n");
      fflush (stdout);
      _comstat = 1;
    }
  if (stat != MKS_OK && _comstat != 0)
    {
      printf ("XCpacket: connection lost or unavailable\n");
      fflush (stdout);
      _comstat = 0;
    }

/*  LeaveCriticalSection(&_MKSCritSection);*/
  return stat;
}

CWORD16
_MKS3PacketCheck (packet_t * pPack, const MKS3Id id,
		  size_t sequenceNum, CWORD16 stat)
{
  CWORD16 retStat = MKS_OK;
  /*char messBuff[512]; */
  if (stat != MKS_OK)
    {
      retStat = stat;
      goto done;
    }
  if (pPack->command != CMD_ACK)
    {
      retStat = COMM_NACK;
      goto done;
    }
  if (id.unitId != 0 && pPack->axisId != id.axisId)
    {
      retStat = COMM_BADID;
      goto done;
    }
  if (id.unitId != 0 && pPack->unitId != id.unitId)
    {
      retStat = COMM_BADID;
      goto done;
    }
/*    sprintf(messBuff, "Rx sequence num=%d\n", pPack->sequenceNum);
    debugStrOut(messBuff);*/
  if (pPack->sequenceNum != sequenceNum)
    {
      retStat = COMM_BADSEQ;
      goto done;
    }
done:return retStat;
}

CWORD16
MKS3StatusGet (const MKS3Id id, CWORD16 * pStat)
{
  packet_t TxPacket;
  packet_t RxPacket;
  CWORD16 pBuff[COMM_MAX_PACKET];
  CWORD16 stat;
  size_t seqNum = _MKS3PacketInit (&TxPacket, pBuff, id);
  makeStatPack (&TxPacket);
  stat = _MKSXCpacket (&TxPacket, &RxPacket);
  stat = _MKS3PacketCheck (&RxPacket, id, seqNum, stat);
  if (stat != MKS_OK)
    return stat;
  *pStat = PACKETUINT (RxPacket.pData, COMM_HEADERLEN);
  return MKS_OK;
}

CWORD16
MKS3VersionGet (const MKS3Id id, CWORD16 * pMajor,
		CWORD16 * pMinor, CWORD16 * pBuild)
{
  packet_t TxPacket;
  packet_t RxPacket;
  CWORD16 pBuff[COMM_MAX_PACKET];
  CWORD16 stat;
  size_t seqNum = _MKS3PacketInit (&TxPacket, pBuff, id);
  makeVersionPack (&TxPacket);
  stat = _MKSXCpacket (&TxPacket, &RxPacket);
  stat = _MKS3PacketCheck (&RxPacket, id, seqNum, stat);
  if (stat != MKS_OK)
    return stat;
  *pMajor = PACKETUINT (RxPacket.pData, COMM_HEADERLEN);
  *pMinor = PACKETUINT (RxPacket.pData, COMM_HEADERLEN + 2);
  *pBuild = PACKETUINT (RxPacket.pData, COMM_HEADERLEN + 4);
  return MKS_OK;
}

CWORD16
MKS3MotorOff (const MKS3Id id)
{
  packet_t TxPacket;
  packet_t RxPacket;
  CWORD16 pBuff[COMM_MAX_PACKET];
  CWORD16 stat;
  size_t seqNum = _MKS3PacketInit (&TxPacket, pBuff, id);
  TxPacket.packetLen = COMM_OVERHEADLEN;
  TxPacket.command = CMD_MOTOR_OFF;
  stat = _MKSXCpacket (&TxPacket, &RxPacket);
  stat = _MKS3PacketCheck (&RxPacket, id, seqNum, stat);
  if (stat != MKS_OK)
    return stat;
  return MKS_OK;
}

CWORD16
MKS3MotorOn (const MKS3Id id)
{
  packet_t TxPacket;
  packet_t RxPacket;
  CWORD16 pBuff[COMM_MAX_PACKET];
  CWORD16 stat;
  size_t seqNum = _MKS3PacketInit (&TxPacket, pBuff, id);
  TxPacket.packetLen = COMM_OVERHEADLEN;
  TxPacket.command = CMD_MOTOR_ON;
  stat = _MKSXCpacket (&TxPacket, &RxPacket);
  stat = _MKS3PacketCheck (&RxPacket, id, seqNum, stat);
  if (stat != MKS_OK)
    return stat;
  return MKS_OK;
}

CWORD16
MKS3FlashErase (const MKS3Id id, CWORD16 address)
{
  packet_t TxPacket;
  packet_t RxPacket;
  CWORD16 pBuff[COMM_MAX_PACKET];
  CWORD16 stat;
  size_t seqNum = _MKS3PacketInit (&TxPacket, pBuff, id);
  makeErasePack (&TxPacket, address);
  stat = _MKSXCpacket (&TxPacket, &RxPacket);
  stat = _MKS3PacketCheck (&RxPacket, id, seqNum, stat);
  if (stat != MKS_OK)
    return stat;
  return MKS_OK;
}

CWORD16
MKS3FlashProgram (const MKS3Id id, CWORD16 startAdr,
		  CWORD16 endAdr, CWORD16 * pData)
{
  packet_t TxPacket;
  packet_t RxPacket;
  CWORD16 pBuff[COMM_MAX_PACKET];
  CWORD16 stat;
  size_t seqNum = _MKS3PacketInit (&TxPacket, pBuff, id);
  makeProgramPacket (&TxPacket, startAdr, endAdr, pData);
  stat = _MKSXCpacket (&TxPacket, &RxPacket);
  stat = _MKS3PacketCheck (&RxPacket, id, seqNum, stat);
  if (stat != MKS_OK)
    return stat;
  return MKS_OK;
}

CWORD16
MKS3FlashRead (const MKS3Id id, CWORD16 startAdr, CWORD16 endAdr,
	       CWORD16 * pData)
{
  packet_t TxPacket;
  packet_t RxPacket;
  CWORD16 pBuff[COMM_MAX_PACKET];
  CWORD16 stat;
  int xx = 0;
  size_t seqNum;
  seqNum = _MKS3PacketInit (&TxPacket, pBuff, id);
  makeFlashReadPacket (&TxPacket, startAdr, endAdr);
  stat = _MKSXCpacket (&TxPacket, &RxPacket);
  stat = _MKS3PacketCheck (&RxPacket, id, seqNum, stat);
  if (stat != MKS_OK)
    return stat;
  for (xx = 0; xx < endAdr - startAdr + 1; xx++)
    {
      pData[xx] = PACKETUINT (RxPacket.pData, COMM_HEADERLEN + xx * 2);
    }
  return MKS_OK;
}

CWORD16
MKS3RamWrite (const MKS3Id id, CWORD16 startAdr, CWORD16 endAdr,
	      CWORD16 * pData)
{
  packet_t TxPacket;
  packet_t RxPacket;
  CWORD16 pBuff[COMM_MAX_PACKET];
  CWORD16 stat;
  size_t seqNum = _MKS3PacketInit (&TxPacket, pBuff, id);
  makeRamWritePacket (&TxPacket, startAdr, endAdr, pData);
  stat = _MKSXCpacket (&TxPacket, &RxPacket);
  stat = _MKS3PacketCheck (&RxPacket, id, seqNum, stat);
  if (stat != MKS_OK)
    return stat;
  return MKS_OK;
}

CWORD16
MKS3RamRead (const MKS3Id id, CWORD16 startAdr, CWORD16 endAdr,
	     CWORD16 * pData)
{
  packet_t TxPacket;
  packet_t RxPacket;
  CWORD16 pBuff[COMM_MAX_PACKET];
  CWORD16 stat;
  int xx;
  size_t seqNum = _MKS3PacketInit (&TxPacket, pBuff, id);
  makeRamReadPacket (&TxPacket, startAdr, endAdr);
  stat = _MKSXCpacket (&TxPacket, &RxPacket);
  stat = _MKS3PacketCheck (&RxPacket, id, seqNum, stat);
  if (stat != MKS_OK)
    return stat;
  for (xx = 0; xx < endAdr - startAdr + 1; xx++)
    {
      pData[xx] = PACKETUINT (RxPacket.pData, COMM_HEADERLEN + xx * 2);
    }
  return MKS_OK;
}


/* ************************************************************************************ */
CWORD16
_MKS3DoGetVal16 (const MKS3Id id, CWORD16 valId, CWORD16 * val16)
{
  packet_t TxPacket;
  packet_t RxPacket;
  CWORD16 pBuff[COMM_MAX_PACKET];
  CWORD16 stat;
  size_t seqNum = _MKS3PacketInit (&TxPacket, pBuff, id);
  makeGetVal16Pack (&TxPacket, valId);
  stat = _MKSXCpacket (&TxPacket, &RxPacket);
  stat = _MKS3PacketCheck (&RxPacket, id, seqNum, stat);
  if (stat != MKS_OK)
    return stat;
  *val16 = PACKETUINT (RxPacket.pData, COMM_HEADERLEN);
  return MKS_OK;
}

CWORD16
_MKS3DoSetVal16 (const MKS3Id id, CWORD16 valId, CWORD16 val16)
{
  packet_t TxPacket;
  packet_t RxPacket;
  CWORD16 pBuff[COMM_MAX_PACKET];
  CWORD16 stat;
  size_t seqNum = _MKS3PacketInit (&TxPacket, pBuff, id);
  makeSetVal16Pack (&TxPacket, valId, val16);
  stat = _MKSXCpacket (&TxPacket, &RxPacket);
  stat = _MKS3PacketCheck (&RxPacket, id, seqNum, stat);
  if (stat != MKS_OK)
    return stat;
  return MKS_OK;
}

CWORD16
_MKS3DoGetVal32 (const MKS3Id id, CWORD16 valId, unsigned long *val32)
{
  packet_t TxPacket;
  packet_t RxPacket;
  CWORD16 pBuff[COMM_MAX_PACKET];
  CWORD16 stat;
  size_t seqNum = _MKS3PacketInit (&TxPacket, pBuff, id);
  makeGetVal32Pack (&TxPacket, valId);
  stat = _MKSXCpacket (&TxPacket, &RxPacket);
  stat = _MKS3PacketCheck (&RxPacket, id, seqNum, stat);
  if (stat != MKS_OK)
    return stat;
  *val32 = PACKETULONG (RxPacket.pData, COMM_HEADERLEN);
  return MKS_OK;
}

CWORD16
_MKS3DoSetVal32 (const MKS3Id id, CWORD16 valId, CWORD32 val32)
{
  packet_t TxPacket;
  packet_t RxPacket;
  CWORD16 pBuff[COMM_MAX_PACKET];
  CWORD16 stat;
  size_t seqNum = _MKS3PacketInit (&TxPacket, pBuff, id);
  makeSetVal32Pack (&TxPacket, valId, val32);
  stat = _MKSXCpacket (&TxPacket, &RxPacket);
  stat = _MKS3PacketCheck (&RxPacket, id, seqNum, stat);
  if (stat != MKS_OK)
    return stat;
  return MKS_OK;
}

CWORD16
MKS3PosAbort (const MKS3Id id)
{
  packet_t TxPacket;
  packet_t RxPacket;
  CWORD16 pBuff[COMM_MAX_PACKET];
  CWORD16 stat;
  size_t seqNum = _MKS3PacketInit (&TxPacket, pBuff, id);
  TxPacket.packetLen = COMM_OVERHEADLEN;
  TxPacket.command = CMD_DO_ABORT;
  stat = _MKSXCpacket (&TxPacket, &RxPacket);
  stat = _MKS3PacketCheck (&RxPacket, id, seqNum, stat);
  if (stat != MKS_OK)
    return stat;
  return MKS_OK;
}

CWORD16
MKS3ConstsUnitIdGet (const MKS3Id id, CWORD16 * pwId)
{
  packet_t TxPacket;
  packet_t RxPacket;
  CWORD16 pBuff[COMM_MAX_PACKET];
  CWORD16 stat;
  size_t seqNum = _MKS3PacketInit (&TxPacket, pBuff, id);
  TxPacket.packetLen = COMM_OVERHEADLEN;
  TxPacket.command = CMD_GET_ID;
  stat = _MKSXCpacket (&TxPacket, &RxPacket);
  stat = _MKS3PacketCheck (&RxPacket, id, seqNum, stat);
  if (stat != MKS_OK)
    return stat;

/* axis id is at offset COMM_HEADERLEN */
  *pwId = PACKETUINT (RxPacket.pData, COMM_HEADERLEN + 1);
  return MKS_OK;
}

CWORD16
MKS3ConstsUnitIdSet (const MKS3Id id, CWORD16 wId)
{
  packet_t TxPacket;
  packet_t RxPacket;
  CWORD16 pBuff[COMM_MAX_PACKET];
  CWORD16 stat;
  size_t seqNum = _MKS3PacketInit (&TxPacket, pBuff, id);
  makeGetVal16Pack (&TxPacket, wId);
  TxPacket.packetLen = COMM_OVERHEADLEN + CMD_SET_UNIT_ID_DATA_LEN;
  TxPacket.command = CMD_SET_UNIT_ID;
  TxPacket.pData[COMM_HEADERLEN + 0] = PACKLO (wId);
  TxPacket.pData[COMM_HEADERLEN + 1] = PACKHI (wId);
  stat = _MKSXCpacket (&TxPacket, &RxPacket);
  stat = _MKS3PacketCheck (&RxPacket, id, seqNum, stat);
  if (stat != MKS_OK)
    return stat;
  return MKS_OK;
}

CWORD16
MKS3ConstsStore (const MKS3Id id)
{
  packet_t TxPacket;
  packet_t RxPacket;
  CWORD16 pBuff[COMM_MAX_PACKET];
  CWORD16 stat;
  size_t seqNum = _MKS3PacketInit (&TxPacket, pBuff, id);
  TxPacket.packetLen = COMM_OVERHEADLEN;
  TxPacket.command = CMD_STORE_CONSTANTS;
  stat = _MKSXCpacket (&TxPacket, &RxPacket);
  stat = _MKS3PacketCheck (&RxPacket, id, seqNum, stat);
  if (stat != MKS_OK)
    return stat;
  return MKS_OK;
}

CWORD16
MKS3ConstsReload (const MKS3Id id)
{
  packet_t TxPacket;
  packet_t RxPacket;
  CWORD16 pBuff[COMM_MAX_PACKET];
  CWORD16 stat;
  size_t seqNum = _MKS3PacketInit (&TxPacket, pBuff, id);
  TxPacket.packetLen = COMM_OVERHEADLEN;
  TxPacket.command = CMD_RELOAD_CONSTANTS;
  stat = _MKSXCpacket (&TxPacket, &RxPacket);
  stat = _MKS3PacketCheck (&RxPacket, id, seqNum, stat);
  if (stat != MKS_OK)
    return stat;
  return MKS_OK;
}

/*****************************************************************/
CWORD16
MKS3MotorIndexFind (const MKS3Id id)
{
  packet_t TxPacket;
  packet_t RxPacket;
  CWORD16 pBuff[COMM_MAX_PACKET];
  CWORD16 stat;
  size_t seqNum = _MKS3PacketInit (&TxPacket, pBuff, id);
  TxPacket.packetLen = COMM_OVERHEADLEN;
  TxPacket.command = CMD_START_INDEX;
  stat = _MKSXCpacket (&TxPacket, &RxPacket);
  stat = _MKS3PacketCheck (&RxPacket, id, seqNum, stat);
  if (stat != MKS_OK)
    return stat;
  return MKS_OK;
}

CWORD16
MKS3MotorIndexMeasure (const MKS3Id id, CWORD16 * pIndex)
{
  CWORD16 stat = MKS3ConstsIndexAngleSet (id, 0xffff);	/* reset current value */
  if (stat != MKS_OK)
    return stat;
  stat = MKS3MotorIndexFind (id);
  if (stat != MKS_OK)
    return stat;
  while (1)
    {
      CWORD16 status;
      stat = MKS3StatusGet (id, &status);
      if (stat != MKS_OK)
	return stat;
      if (!(status & MOTOR_INDEXING))
	break;
    }
  stat = MKS3ConstsIndexAngleGet (id, pIndex);	/* get new value */
  if (stat != MKS_OK)
    return stat;
  return MKS_OK;
}

CWORD16
MKS3MotorVoltageGet (const MKS3Id id, long *pSumVoltage, long *pCount)
{
  CWORD16 stat = _MKS3DoGetVal32 (id, CMD_VAL32_SUM_VOLTAGE,
				  (unsigned long *) pSumVoltage);
  if (stat == MKS_OK)
    stat =
	_MKS3DoGetVal32 (id, CMD_VAL32_SUM_VOLTAGE_COUNT,
			 (unsigned long *) pCount);
  return stat;
}

CWORD16
MKS3HomeStart (const MKS3Id id)
{
  packet_t TxPacket;
  packet_t RxPacket;
  CWORD16 pBuff[COMM_MAX_PACKET];
  CWORD16 stat;
  size_t seqNum = _MKS3PacketInit (&TxPacket, pBuff, id);
  TxPacket.packetLen = COMM_OVERHEADLEN;
  TxPacket.command = CMD_START_HOME;
  stat = _MKSXCpacket (&TxPacket, &RxPacket);
  stat = _MKS3PacketCheck (&RxPacket, id, seqNum, stat);
  if (stat != MKS_OK)
    return stat;
  return MKS_OK;
}

CWORD16
MKS3HomePhaseGet (const MKS3Id id, CWORD16 * pPhase)
{
  return _MKS3DoGetVal16 (id, CMD_VAL16_HOME_PHASE, pPhase);
}

CWORD16
MKS3Home (const MKS3Id id, long offset)
{
  packet_t TxPacket;
  packet_t RxPacket;
  CWORD16 pBuff[COMM_MAX_PACKET];
  CWORD16 stat;
  size_t seqNum = _MKS3PacketInit (&TxPacket, pBuff, id);
  TxPacket.packetLen = COMM_OVERHEADLEN;
  TxPacket.command = CMD_START_HOME;
  stat = _MKSXCpacket (&TxPacket, &RxPacket);
  stat = _MKS3PacketCheck (&RxPacket, id, seqNum, stat);
  if (stat != MKS_OK)
    return stat;
  return MKS_OK;
}

CWORD16
MKS3PECTableSet (const MKS3Id id, CWORD16 * data)
{
  CWORD16 stat;
  CWORD16 ratio;
  size_t index = 0, rr;
  stat = MKS3ConstsPECRatioGet (id, &ratio);
  if (stat != MKS_OK)
    return stat;
  if (ratio * PEC_REV_COUNT > MAX_PEC_RATIO * PEC_REV_COUNT)
    return MAIN_BAD_PEC_LENGTH;
  for (rr = 0; rr < ratio; rr++)
    {
      stat =
	  MKS3FlashProgram (id, PEC_STORAGE_ADDR + index,
			    PEC_STORAGE_ADDR + index + PEC_REV_COUNT - 1,
			    data + index);
      if (stat != MKS_OK)
	return stat;
      index += PEC_REV_COUNT;
    }
  return MKS_OK;
}

CWORD16
MKS3PECTableGet (const MKS3Id id, CWORD16 * data)
{
  CWORD16 stat;
  CWORD16 ratio;
  size_t index = 0, rr;
  stat = MKS3ConstsPECRatioGet (id, &ratio);
  if (stat != MKS_OK)
    return stat;
  if (ratio * PEC_REV_COUNT > 16383)
    return MAIN_BAD_PEC_LENGTH;
  for (rr = 0; rr < ratio; rr++)
    {
      stat =
	  MKS3FlashRead (id, PEC_STORAGE_ADDR + index,
			 PEC_STORAGE_ADDR + index + PEC_REV_COUNT - 1,
			 data + index);
      if (stat != MKS_OK)
	return stat;
      index += PEC_REV_COUNT;
    }
  return MKS_OK;
}


CWORD16
MKS3UserSpaceWrite (const MKS3Id id, CWORD16 startAdr,
		    CWORD16 endAdr, CWORD16 * pData)
{
  CWORD16 stat;
  if (endAdr > 16383)
    return COMM_BADVALCODE;
  if (startAdr > 16383)
    return COMM_BADVALCODE;
  stat =
      MKS3FlashProgram (id, USER_FLASH_SECTOR + startAdr,
			USER_FLASH_SECTOR + endAdr, pData);
  if (stat != MKS_OK)
    return stat;
  return MKS_OK;
}

CWORD16
MKS3UserSpaceRead (const MKS3Id id, CWORD16 startAdr,
		   CWORD16 endAdr, CWORD16 * pData)
{
  CWORD16 stat;
  if (endAdr > 16383)
    return COMM_BADVALCODE;
  if (startAdr > 16383)
    return COMM_BADVALCODE;
  stat =
      MKS3FlashRead (id, USER_FLASH_SECTOR + startAdr,
		     USER_FLASH_SECTOR + endAdr, pData);
  if (stat != MKS_OK)
    return stat;
  return MKS_OK;
}

CWORD16
MKS3ObjTrackInit (const MKS3Id id, MKS3ObjTrackInfo * pTrackInfo)
{
  packet_t TxPacket;
  packet_t RxPacket;
  CWORD16 pBuff[COMM_MAX_PACKET];
  CWORD16 stat;
  long sampAccel, maxSpeedTicks;
  double interruptPeriod, clockRate;
  size_t seqNum = _MKS3PacketInit (&TxPacket, pBuff, id);
  TxPacket.packetLen = COMM_OVERHEADLEN;
  TxPacket.command = CMD_OBJ_TRACK_INIT;
  stat = _MKSXCpacket (&TxPacket, &RxPacket);
  stat = _MKS3PacketCheck (&RxPacket, id, seqNum, stat);
  if (stat != MKS_OK)
    return stat;
  sampAccel = PACKETULONG (RxPacket.pData, COMM_HEADERLEN);
  maxSpeedTicks = PACKETULONG (RxPacket.pData, COMM_HEADERLEN + 4);
  interruptPeriod = PACKETUINT (RxPacket.pData, COMM_HEADERLEN + 8);
  clockRate = RxPacket.pData[COMM_HEADERLEN + 10];
  pTrackInfo->sampleFreq = clockRate * 1000000 / interruptPeriod;
  pTrackInfo->absAccelTicksPerSecPerSec =
      sampAccel * pTrackInfo->sampleFreq * pTrackInfo->sampleFreq /
      (1 << 25);
  pTrackInfo->maxSpeedTicksPerSec =
      maxSpeedTicks * pTrackInfo->sampleFreq / (1 << 25);
  pTrackInfo->prevVelocityTicksPerSec = 0;
  pTrackInfo->prevPointTimeTicks = 0;
  stat = MKS3PosTargetGet (id, &pTrackInfo->prevPointPos);
  return stat;
}

/* compute PosVel(T), given conditions*/
void
_MKS3PosVel (double initVel, double finalVel, double absAccel,
	     double deltaT, long *pReachedDeltaPos, double *pReachedVel)
{
  double actAccel = (initVel < finalVel) ? absAccel : -absAccel;
  double accelTime = (finalVel - initVel) / actAccel;
  if (accelTime > deltaT)
    accelTime = deltaT;
  *pReachedVel = actAccel * accelTime + initVel;
  *pReachedDeltaPos =
      (long) (-0.5 * actAccel * accelTime * accelTime +
	      (actAccel * accelTime + initVel) * deltaT);
}

/* calc final position and velocity in continuous units, return true if it can actually reach final pos */
int
_MKS3FinalPosVelCalc (double initVel, double absAccel, double maxSpeed,
		      double endTimeSec, long endPos,
		      double *pFinalVel, long *pFinalPos)
{
  int bCanReach = 1;		/* bool */
  double actAccel = (initVel * endTimeSec < endPos) ? absAccel : -absAccel;
  double accelTime, finalVel;
  double discrim =
      actAccel * actAccel * endTimeSec * endTimeSec +
      2.0 * actAccel * (initVel * endTimeSec - endPos);
  if (discrim < 0)
    {				/* acceleration-limited, can't actually reach desired target pos, so see where we will go instead */
      bCanReach = 0;
      accelTime = endTimeSec;
    }

  else
    accelTime = endTimeSec - sqrt (discrim) / absAccel;
  finalVel = actAccel * accelTime + initVel;

  /* now check if it would break the speed limit */
  if (finalVel > maxSpeed || finalVel < -maxSpeed)
    {				/* would exceed max. speed, so re-compute */
      bCanReach = 0;
      if (finalVel < 0)
	maxSpeed = -maxSpeed;
      accelTime = (maxSpeed - initVel) / actAccel;
      finalVel = maxSpeed;
    }
  if (!bCanReach)		/* compute where it really will go */
    endPos =
	(long) (-0.5 * actAccel * accelTime * accelTime +
		(actAccel * accelTime + initVel) * endTimeSec);
  *pFinalVel = finalVel;
  *pFinalPos = endPos;
  return bCanReach;
}


/*      calc. velocity, given acceleration and current velocity or target velocity, depending on if it's passed the last control point or not*/
/* if it's passed the last control point, it's no longer accelerating, so use cur velocity & time,*/
/* otherwise, compute using acceleration, final velocity, and position from last control point time */
CWORD16
MKS3ObjTrackPointAdd (const MKS3Id id,
		      MKS3ObjTrackInfo * pTrackInfo,
		      double timeSec, long positionTicks,
		      MKS3ObjTrackStat * pAddStat)
{
  char bigBuff[1000];
  double deltaTsec;
  double finalVelTicksPerSec;
  long actualFinalDeltaPos, actualFinalPos, deltaPos;
  size_t seqNum;
  packet_t TxPacket;
  packet_t RxPacket;
  CWORD16 pBuff[COMM_MAX_PACKET];
  CWORD16 stat, curTimeTicks;
  CWORD16 pointStat, buffSpace, addedIndex;
  CWORD32 actualStartTimeTicks;
  long actualStartPos, actualStartVelocityTicks;
  long targetCreepVelocity;
  double actualAddedTimeSec;
  int canReach;
  pAddStat->status = OBJ_TRACK_STAT_FAILURE;
  pAddStat->actualPosition = 0;
  pAddStat->actualTime = 0;
  pAddStat->spaceLeft = 0;
  pAddStat->targetVelocityTicks = 0;
  pAddStat->index = -1;
  deltaTsec =
      timeSec - pTrackInfo->prevPointTimeTicks / pTrackInfo->sampleFreq;
  if (deltaTsec <= 0)
    {
      pAddStat->status = OBJ_TRACK_STAT_BAD_ORDER;
      return MAIN_BAD_POINT_ADD;
    }
  deltaPos = positionTicks - pTrackInfo->prevPointPos;
  canReach =
      _MKS3FinalPosVelCalc (pTrackInfo->prevVelocityTicksPerSec,
			    pTrackInfo->absAccelTicksPerSecPerSec,
			    pTrackInfo->maxSpeedTicksPerSec, deltaTsec,
			    deltaPos, &finalVelTicksPerSec,
			    &actualFinalDeltaPos);
  actualFinalPos = pTrackInfo->prevPointPos + actualFinalDeltaPos;
  curTimeTicks = (CWORD32) (timeSec * pTrackInfo->sampleFreq);
  seqNum = _MKS3PacketInit (&TxPacket, pBuff, id);
  TxPacket.packetLen = COMM_OVERHEADLEN + CMD_OBJ_TRACK_POINT_ADD_DATA_LEN;
  TxPacket.command = CMD_OBJ_TRACK_POINT_ADD;
  addCWORD32 (&TxPacket, 0, pTrackInfo->prevPointTimeTicks);
  addCWORD32 (&TxPacket, 4, curTimeTicks);
  targetCreepVelocity =
      (long) (finalVelTicksPerSec * (1 << 25) / pTrackInfo->sampleFreq);
  addCWORD32 (&TxPacket, 8, targetCreepVelocity);
  addCWORD32 (&TxPacket, 12, actualFinalPos);
  sprintf (bigBuff,
	   "MKS3ObjTrackPointAdd: adding: prevTimeTicks=%ld, curTimeTicks=%d, targVelTicks=%ld\n",
	   pTrackInfo->prevPointTimeTicks, curTimeTicks,
	   targetCreepVelocity);
  printf (bigBuff);
  stat = _MKSXCpacket (&TxPacket, &RxPacket);
  stat = _MKS3PacketCheck (&RxPacket, id, seqNum, stat);
  if (stat != MKS_OK)
    return stat;
  pointStat = RxPacket.pData[COMM_HEADERLEN];
  buffSpace = RxPacket.pData[COMM_HEADERLEN + 1];
  addedIndex = RxPacket.pData[COMM_HEADERLEN + 2];
  actualStartTimeTicks = PACKETULONG (RxPacket.pData, COMM_HEADERLEN + 3);
  actualStartPos = PACKETULONG (RxPacket.pData, COMM_HEADERLEN + 7);
  actualStartVelocityTicks =
      PACKETULONG (RxPacket.pData, COMM_HEADERLEN + 11);
  sprintf (bigBuff,
	   "MKS3ObjTrackPointAdd: returned stat=%d, buffSpace=%d, index=%d, actual time ticks=%ld, actual vel=%ld\n",
	   pointStat, buffSpace, addedIndex, actualStartTimeTicks,
	   actualStartVelocityTicks);
  printf (bigBuff);
  actualAddedTimeSec = actualStartTimeTicks / pTrackInfo->sampleFreq;
  if (pointStat == OBJ_TRACK_STAT_ADDED_LATE)
    {				/* need to recompute where it really will be based on actual start time, pos, & velocity. */
      double deltaTsec = timeSec - actualAddedTimeSec;
      double actualPrevVelocity =
	  actualStartVelocityTicks * pTrackInfo->sampleFreq / (1 << 25);
      _MKS3PosVel (actualPrevVelocity, finalVelTicksPerSec,
		   pTrackInfo->absAccelTicksPerSecPerSec, deltaTsec,
		   &actualFinalDeltaPos, &finalVelTicksPerSec);
      targetCreepVelocity =
	  (long) (finalVelTicksPerSec * (1 << 25) /
		  pTrackInfo->sampleFreq);
      actualFinalPos = actualStartPos + actualFinalDeltaPos;
    }
  if (pointStat == OBJ_TRACK_STAT_ADDED_LATE
      || pointStat == OBJ_TRACK_STAT_ADDED_OK)
    {
      pTrackInfo->prevVelocityTicksPerSec = finalVelTicksPerSec;
      pTrackInfo->prevPointPos = actualFinalPos;
      pTrackInfo->prevPointTimeTicks = curTimeTicks;
    }

  else
    stat = MAIN_BAD_POINT_ADD;
  pAddStat->status = pointStat;
  pAddStat->actualPosition = actualFinalPos;
  pAddStat->actualTime = actualAddedTimeSec;
  pAddStat->spaceLeft = buffSpace;
  pAddStat->targetVelocityTicks = targetCreepVelocity;
  pAddStat->index = addedIndex;
  return stat;
}

CWORD16
MKS3ObjTrackEnd (const MKS3Id id)
{
  packet_t TxPacket;
  packet_t RxPacket;
  CWORD16 pBuff[COMM_MAX_PACKET];
  CWORD16 stat;
  size_t seqNum = _MKS3PacketInit (&TxPacket, pBuff, id);
  TxPacket.packetLen = COMM_OVERHEADLEN;
  TxPacket.command = CMD_OBJ_TRACK_END;
  stat = _MKSXCpacket (&TxPacket, &RxPacket);
  stat = _MKS3PacketCheck (&RxPacket, id, seqNum, stat);
  if (stat != MKS_OK)
    return stat;
  return MKS_OK;
}

CWORD16
MKS3PosVelTimeGet (const MKS3Id id, long *pPos, long *pVel,
		   long *pTime, long *pIndex)
{
  packet_t TxPacket;
  packet_t RxPacket;
  CWORD16 pBuff[COMM_MAX_PACKET];
  CWORD16 stat;
  size_t seqNum = _MKS3PacketInit (&TxPacket, pBuff, id);
  TxPacket.packetLen = COMM_OVERHEADLEN;
  TxPacket.command = CMD_GET_POS_VEL_TIME;
  stat = _MKSXCpacket (&TxPacket, &RxPacket);
  stat = _MKS3PacketCheck (&RxPacket, id, seqNum, stat);
  if (stat != MKS_OK)
    return stat;
  *pTime = PACKETULONG (RxPacket.pData, COMM_HEADERLEN);
  *pPos = PACKETULONG (RxPacket.pData, COMM_HEADERLEN + 4);
  *pVel = PACKETULONG (RxPacket.pData, COMM_HEADERLEN + 8);
  *pIndex =
      (signed short) (PACKETUINT (RxPacket.pData, COMM_HEADERLEN + 12));
  return MKS_OK;
}

/********************************************************************/
/* diagnostic functions                                             */
CWORD16
MKS3DiagModeEnter (const MKS3Id id)
{
  packet_t TxPacket;
  packet_t RxPacket;
  CWORD16 pBuff[COMM_MAX_PACKET];
  CWORD16 stat;
  size_t seqNum = _MKS3PacketInit (&TxPacket, pBuff, id);
  TxPacket.packetLen = COMM_OVERHEADLEN;
  TxPacket.command = CMD_DIAGS_MODE_ENTER;
  stat = _MKSXCpacket (&TxPacket, &RxPacket);
  stat = _MKS3PacketCheck (&RxPacket, id, seqNum, stat);
  if (stat != MKS_OK)
    return stat;
  return MKS_OK;
}

CWORD16
MKS3DiagCurrentSet (const MKS3Id id, CWORD16 current)
{
  packet_t TxPacket;
  packet_t RxPacket;
  CWORD16 pBuff[COMM_MAX_PACKET];
  CWORD16 stat;
  size_t seqNum = _MKS3PacketInit (&TxPacket, pBuff, id);
  TxPacket.packetLen = COMM_OVERHEADLEN + CMD_DIAGS_CURRENT_SET_LEN;
  TxPacket.command = CMD_DIAGS_CURRENT_SET;
  TxPacket.pData[COMM_HEADERLEN + 0] = PACKLO (current);
  TxPacket.pData[COMM_HEADERLEN + 1] = PACKHI (current);
  stat = _MKSXCpacket (&TxPacket, &RxPacket);
  stat = _MKS3PacketCheck (&RxPacket, id, seqNum, stat);
  if (stat != MKS_OK)
    return stat;
  return MKS_OK;
}

CWORD16
MKS3DiagAngleSet (const MKS3Id id, CWORD16 angle)
{
  packet_t TxPacket;
  packet_t RxPacket;
  CWORD16 pBuff[COMM_MAX_PACKET];
  CWORD16 stat;
  size_t seqNum = _MKS3PacketInit (&TxPacket, pBuff, id);
  TxPacket.packetLen = COMM_OVERHEADLEN + CMD_DIAGS_ANGLE_SET_LEN;
  TxPacket.command = CMD_DIAGS_ANGLE_SET;
  TxPacket.pData[COMM_HEADERLEN + 0] = PACKLO (angle);
  TxPacket.pData[COMM_HEADERLEN + 1] = PACKHI (angle);
  stat = _MKSXCpacket (&TxPacket, &RxPacket);
  stat = _MKS3PacketCheck (&RxPacket, id, seqNum, stat);
  if (stat != MKS_OK)
    return stat;
  return MKS_OK;
}

CWORD16
MKS3DiagModeLeave (const MKS3Id id)
{
  packet_t TxPacket;
  packet_t RxPacket;
  CWORD16 pBuff[COMM_MAX_PACKET];
  CWORD16 stat;
  size_t seqNum = _MKS3PacketInit (&TxPacket, pBuff, id);
  TxPacket.packetLen = COMM_OVERHEADLEN;
  TxPacket.command = CMD_DIAGS_MODE_LEAVE;
  stat = _MKSXCpacket (&TxPacket, &RxPacket);
  stat = _MKS3PacketCheck (&RxPacket, id, seqNum, stat);
  if (stat != MKS_OK)
    return stat;
  return MKS_OK;
}
