#define _MICRO_C
#define ST237_SUPPORT

#include <sys/time.h>
#include <sys/io.h>
#include <unistd.h>
#include <stdio.h>
#include "urvc.h"

unsigned char CommandOutBuf[50];	// kde se VYRABEJI mikroprikazy
unsigned char CommandInBuf[50];	// kam se PRIJIMAJI mikroodpovedi
unsigned char control_out;	// moje promenna, kterou uz nikdo nepouziva
unsigned char imaging_clocks_out = 0;	// moje promenna, pouziva ji i parccd 
unsigned char romMSNtoID = 4;	// ?

int IO_LOG = 1;

int
MicroStat ()
{
  unsigned char val;

  CameraOut (0x30, (control_out | 2) & 0xff);
  val = CameraIn (0x30);
  if (control_out & 1)
    val = val & 0x10;
  else
    val = !(val & 0x10);

  return val;
}

unsigned char
MicroIn (MY_LOGICAL ackIt)
{
  unsigned char val;

  CameraOut (0x30, (control_out | 2));
  val = CameraIn (0x30) & 0xf;

  if (ackIt)
    CameraOut (0x30, (control_out ^ 1));

  return val;
}

void
MicroOut (int val)
{
  CameraOut (0x20, val);
  CameraOut (0x30, (control_out ^ 1));
}

// Tiky stareho XT, 18.3x za sekundu 
unsigned long
TickCount ()
{
  return (unsigned long) (mSecCount () / 54.64);
}

PAR_ERROR
SendMicroBlock (unsigned char *p, int len)
{
  PAR_ERROR err = CE_NO_ERROR;
  short i;
  unsigned long t0;

  t0 = TickCount ();
  CameraOut (0x30, 4);
  // i = 0;

  // while(i <= len)
  for (i = 0; i <= len;)
    {
      if (!MicroStat ())
	{
	  if (TickCount () - t0 <= 5)
	    continue;
	  else
	    {
	      err = CE_TX_TIMEOUT;
	      CameraOut (0x30, 0);
	      break;
	    }
	}
      if (i == len)
	break;
      if (i == 1)
	CameraOut (0x30, control_out & 0xfb);
      MicroOut (*(p++));
      i++;
      t0 = TickCount ();
    }
  return err;
}

PAR_ERROR
ValGetMicroBlock (MICRO_COMMAND command, CAMERA_TYPE camera, void *dataPtr)
{
  PAR_ERROR err = CE_NO_ERROR;

  short rx_len = 0;
  short cmp_len = 0;
  short packet_len = 0;
  short state;
  unsigned long ticks;
  unsigned char *p = NULL;
  unsigned char c;
  unsigned char start = 0;
  unsigned char romMSN;
  unsigned short u;
  unsigned short u1;
  QueryTemperatureStatusResults *qtsr;	// sizeof=10
  GetVersionResults *gvr;	// Tohle prvni!
  StatusResults *sr;
  EEPROMResults *eepr;
  TXSerialBytesResults *txbr;	// sizeof=2

  state = 0;

  // Prvni faze... 
  switch (command)		// goto ( *(eax * 4 + 0x80579b0));
    {
    case MC_TEMP_STATUS:
      cmp_len = 10;
      break;
    case MC_GET_VERSION:
      cmp_len = 4;
      break;
    case MC_EEPROM:
      cmp_len = 2;
      break;
    case MC_STATUS:
      if ((camera == ST5C_CAMERA) || (camera == ST237_CAMERA)
	  || (camera == ST237A_CAMERA))
	cmp_len = 4;
      else
	cmp_len = 6;
      break;
    case MC_TX_BYTES:		// TX_SERIAL_BYTES (jaxem na to prisel??? -m.)
      cmp_len = 2;
      break;
    default:
      err = 4;
      break;
    }

  // Druha faze - natazeni informaci z kamery
  ticks = TickCount ();
  do
    if (MicroStat () && (state <= 4))
      {
	c = MicroIn (1);
	switch (state)
	  {
	  case 0:
	    if ((c == 0xa) || (c == 1))
	      {
		start = c << 4;
		state++;
		rx_len = 1;
	      }
	    else
	      err = CE_UNKNOWN_RESPONSE;
	    break;
	  case 1:
	    start += c;
	    if (start == 0xa5)
	      {
		state++;
		p = CommandInBuf;
		rx_len++;
		break;
	      }
	    if (start == 0x18)
	      {
		err = CE_CAN_RECEIVED;
		break;
	      }
	    if (start == 0x15)
	      {
		err = CE_NAK_RECEIVED;
		break;
	      }
	    err = CE_UNKNOWN_RESPONSE;
	    break;
	  case 2:
	    *(p++) = c;
	    rx_len++;
	    if (c == command)
	      {
		state++;
		break;
	      }
	    err = CE_UNKNOWN_RESPONSE;
	    break;
	  case 3:
	    *(p++) = c;
	    rx_len++;
	    packet_len = 2 * c;
	    if (cmp_len == packet_len)
	      {
		state++;
		break;
	      }
	    err = CE_BAD_LENGTH;
	    break;
	  case 4:
	    *(p++) = c;
	    rx_len++;
	    if (rx_len == (packet_len + 4))
	      state++;
	    break;
	  }			// konec switche
	ticks = TickCount ();
      }				// konec ifu
    else if (TickCount () - ticks > 5)
      err = CE_RX_TIMEOUT;
  while ((state <= 4) && (err == CE_NO_ERROR));

  // Treti faze - ulozeni ziskanych dat do patricnych struktur.  
  p = CommandInBuf + 2;

  if (err == 0)
    switch (command)
      {
      case MC_TEMP_STATUS:	// TEMP_STATUS DELKA = 10
	qtsr = (QueryTemperatureStatusResults *) dataPtr;
	u = *(p++) << 4;
	u += *(p++);
	qtsr->enabled = u;	// &1;
	u = *(p++) << 4;
	u += *(p++);
	qtsr->ccdSetpoint = u << 4;
	u = *(p++) << 4;
	u += *(p++);
	qtsr->ccdThermistor = u << 4;
	u = *(p++) << 4;
	u += *(p++);
	qtsr->ambientThermistor = u << 4;
	u = *(p++) << 4;
	u += *(p++);
	qtsr->power = u;
	break;
      case MC_GET_VERSION:	// Odpoved na verzi DELKA = 4
	gvr = (GetVersionResults *) dataPtr;
	u = *(p++);
	romMSN = u;		// rom je asi 8b
	if ((romMSN == 2) || (romMSN > 4))
	  gvr->cameraID = 0;
	else
	  gvr->cameraID = romMSN * 4 + romMSNtoID;
	u += (unsigned short) (*(p++)) << 8;
	u += *(p++) << 4;
	u += *(p++);
	gvr->firmwareVersion = u;	// as 16b
	break;
      case MC_EEPROM:		// EEPROM Result, DELKA = 2
	eepr = (EEPROMResults *) dataPtr;
	u = *(p++) << 4;
	u += *(p++);
	eepr->data = u;
	break;
      case MC_TX_BYTES:	// TX_BYTES delka = 2
	txbr = (TXSerialBytesResults *) dataPtr;
	u = *(p++) << 4;
	u += *(p++);
	txbr->bytesSent = u;
	break;
      case MC_STATUS:

	sr = (StatusResults *) dataPtr;
	u = *(p++) << 4;
	u += *(p++);
	u1 = *(p++) << 4;
	u1 += *(p++);

	sr->imagingState = u & 3;

//              printf("CAMERA:%d\n", camera);

#if defined(ST5C_SUPPORT) || defined(ST237_SUPPORT)
	if ((camera == ST5C_CAMERA) || (camera == ST237_CAMERA)
	    || (camera == ST237A_CAMERA))
	  {
	    sr->trackingState = 0;
	    sr->shutterStatus = 0;
	    sr->ledStatus = 0;
	    sr->xPlusActive = u1 & 1;
	    sr->xMinusActive = (u1 >> 1) & 1;
	    sr->yPlusActive = (u1 >> 2) & 1;
	    sr->yMinusActive = (u1 >> 3) & 1;
	    if ((camera == ST237_CAMERA) || (camera == ST237A_CAMERA))
	      sr->fanEnabled = (u1 >> 4) & 1;
	    else
	      sr->fanEnabled = 0;
	    sr->CFW6Active = 0;
	    sr->CFW6Input = 0;
	    sr->shutterEdge = 0;
	    sr->filterState = (u >> 2) & 7;
	    if (camera == ST5C_CAMERA)
	      sr->adSize = 2;
	    else
	      sr->adSize = 1;
	    if (u & 0x40)
	      {
		sr->filterType = 0;
		break;
	      }
	    if (u & 0x20)
	      sr->filterType = 3;
	    else
	      sr->filterType = 2;
	    break;
	  }
	else
	  {
#endif
	    sr->trackingState = (u >> 2) & 3;
	    sr->shutterStatus = (u >> 4) & 3;
	    sr->ledStatus = (u >> 6) & 3;
	    sr->xPlusActive = (u1 >> 4) & 1;
	    sr->xMinusActive = (u1 >> 5) & 1;
	    sr->yPlusActive = (u1 >> 6) & 1;
	    sr->yMinusActive = (u1 >> 7) & 1;
	    sr->fanEnabled = u1 & 1;
	    sr->CFW6Active = (u1 >> 1) & 1;
	    sr->CFW6Input = (u1 >> 2) & 1;

	    u = *(p++) << 4;
	    u += *(p++);
	    sr->shutterEdge = u;
	    sr->filterState = 0;
	    sr->adSize = 0;
	    sr->filterType = 1;
	    break;
#if defined(ST5C_SUPPORT) ||  defined(ST237_SUPPORT)
	  }
#endif
      default:
	err = 4;
	break;

      }
  return err;
}

PAR_ERROR
ValGetMicroAck ()
{
  unsigned long ticks;
  PAR_ERROR err = CE_NO_ERROR;
  short state = 0;
  unsigned short r = 0;

  state = 0;
  ticks = TickCount ();

  while (err == CE_NO_ERROR)
    {
      if (!MicroStat ())
	{
	  if (TickCount () - ticks <= 5)
	    continue;
	  else
	    {
	      err = CE_RX_TIMEOUT;
	      break;
	    }
	}
      if (state == 0)
	{
	  r = MicroIn (1) << 4;
	  state++;
	  continue;
	}
      if (state == 1)
	{
	  r += MicroIn (0);
	  state++;
	  continue;
	}
      if (state == 2)
	{
	  switch (r)
	    {
	    case 0x06:
	      break;		// ACK
	    case 0x15:
	      err = CE_NAK_RECEIVED;
	      break;
	    case 0x18:
	      err = CE_CAN_RECEIVED;
	      break;
	    default:
	      err = CE_UNKNOWN_RESPONSE;
	      break;
	    }
	  break;
	}
    }
  return err;
}

PAR_ERROR
ValidateMicroResponse (MICRO_COMMAND command, CAMERA_TYPE camera,
		       void *dataPtr, void *txDataPtr)
{
  PAR_ERROR result = CE_NO_ERROR;
  //ActivateRelayParams *arp;

  MicroOut (0);

  if (command <= 0x82)
    switch (command)
      {
      case MC_START_EXPOSURE:
      case MC_END_EXPOSURE:
      case MC_REGULATE_TEMP:
      case MC_REGULATE_TEMP2:
      case MC_PULSE:
      case MC_SYSTEM_TEST:
      case MC_CONTROL_CCD:
      case MC_MISC_CONTROL:
	result = ValGetMicroAck ();
	break;
      case MC_RELAY:
	result = ValGetMicroAck ();
	if (result != 0)
	  break;

	//arp = (ActivateRelayParams *) txDataPtr;
	//if((arp->tXPlus != 0) || (arp->tXMinus != 0)
	//   || (arp->tYPlus != 0) || (arp->tYMinus != 0))
	//    RelayClick();
	break;

      case MC_TEMP_STATUS:
      case MC_GET_VERSION:
      case MC_TX_BYTES:
      case MC_STATUS:
	result = ValGetMicroBlock (command, camera, dataPtr);
	break;

      case MC_EEPROM:		// Jiste to neni, ale dava to logiku zapis se
	// jen potvrdi, cteni posila bajty...
	if (((EEPROMParams *) txDataPtr)->write == 0)
	  result = ValGetMicroBlock (command, camera, dataPtr);
	else
	  result = ValGetMicroAck ();
	break;
      }

  if (result)
    CameraOut (0x30, 0);

  return result;
}

void
BuildMicroCommand (MICRO_COMMAND command, CAMERA_TYPE camera, void *dataPtr,
		   int *lenp)
{
  unsigned char *p;
  unsigned char *p1;
  unsigned short len = 0;
  unsigned short u /* , u1 */ ;
  unsigned char flags = 0;
  short i;
  StartExposureParams *sep;
  EndExposureParams *eep;
  MicroTemperatureRegulationParams *mtrp;
  MicroTemperatureSpecialParams *mtsp;
  ActivateRelayParams *arp;
  PulseOutParams *pop;
  EEPROMParams *eepp;
  SystemTestParams *stp;
  MiscellaneousControlParams *mcp;
  MY_LOGICAL relays16x;
  unsigned long maxExposure;
  unsigned long exposureTime;

  p = CommandOutBuf;
  *(p++) = 0xa;			// p[0]=0xa;
  *(p++) = 5;			// p[1]=0x5
  *(p++) = command & 0x7f;	// Tohle vypada rozhodne zvlastne 
  // nemely by to bejt jen pulbajty? -> &0xf

  if (command <= 0x82)
    switch (command)
      {
      case MC_START_EXPOSURE:

	len = 8;
	*(p++) = len / 2;

	sep = (StartExposureParams *) dataPtr;

#ifdef DEBUG
	fprintf (stderr, "ccd=%d, exp=%d, shu=%d ", sep->ccd,
		 sep->exposureTime, sep->openShutter);
#endif
	if (sep->ccd == 1)
	  maxExposure = 0xffff;
	else
	  maxExposure = 0xffffff;

	exposureTime = sep->exposureTime & 0xffffff;

	if ((camera == ST7_CAMERA)
	    || (camera == ST8_CAMERA) || (camera == ST9_CAMERA))
	  {
	    if (sep->openShutter == 1)
	      {
		if (exposureTime <= MIN_ST7_EXPOSURE)
		  exposureTime = 0;
		else
		  exposureTime -= MIN_ST7_EXPOSURE;
	      }
	  }

	if (exposureTime > maxExposure)
	  exposureTime = maxExposure;

	u = exposureTime >> 16;
	*(p++) = u >> 4 & 0xf;	// p[+1] = 6. pulbajt exptime
	*(p++) = u & 0xf;	// p[+2] = 5. pulbajt exptime

	u = exposureTime;
	*(p++) = (u >> 12) & 0xf;	// p[+3] = 4. pulbajt exptime
	*(p++) = (u >> 8) & 0xf;	// p[+4] = 3. pulbajt exptime
	*(p++) = (u >> 4) & 0xf;	// p[+5] = 2. pulbajt exptime
	*(p++) = u & 0xf;	// p[+6] = 1. pulbajt exptime

	if (sep->abgState > 3)
	  flags = 0;
	else
	  flags = sep->abgState;

	if ((camera == ST7_CAMERA)
	    || (camera == ST8_CAMERA)
	    || (camera == STK_CAMERA) || (camera == ST9_CAMERA))
	  {
	    flags = flags + (sep->openShutter & 3) * 4;
	    flags = flags + ((sep->ccd & 1) << 4);
	    if (sep->exposureTime & 0x40000000)
	      flags += 0x20;
	  }

	*(p++) = flags >> 4;	// p[+7]=flags - horni pulbajt
	*(p++) = flags & 0xf;	// p[+8]=flags - dolni pubajt

	break;

      case MC_END_EXPOSURE:

	if (camera == ST237_CAMERA || camera == ST237A_CAMERA)
	  {
	    len = 0;
	  }
	else
	  {
	    len = 2;
	    *(p++) = len / 2;

	    eep = (EndExposureParams *) dataPtr;
	    *(p++) = 0;
	    *(p++) = eep->ccd & 1;
	  }
	break;

      case MC_REGULATE_TEMP:

	len = 6;		// delka=6
	*(p++) = len / 2;	// p[3]=delka/2 (==3)
	mtrp = (MicroTemperatureRegulationParams *) dataPtr;
#ifdef DEBUG
	fprintf (stderr, "reg=%d, setpoint=%d, preload=%d ",
		 mtrp->regulation, mtrp->ccdSetpoint, mtrp->preload);
#endif
	*(p++) = 0;
	*(p++) = mtrp->regulation;

	u = mtrp->ccdSetpoint;
	if (mtrp->regulation == 1)
	  u >>= 4;
	if (u > 0xff)
	  u = 0xff;
	*(p++) = u >> 4 & 0xf;
	*(p++) = u & 0xf;

	u = mtrp->preload;
	if (u > 0xff)
	  u = 0xff;
	*(p++) = u >> 4 & 0xf;
	*(p++) = u & 0xf;
	break;

      case MC_REGULATE_TEMP2:

	len = 6;
	*(p++) = len >> 1;

	mtsp = (MicroTemperatureSpecialParams *) dataPtr;
	flags = 0x10;
	if (mtsp->freezeTE)
	  flags = flags | 8;
	if (mtsp->lowerPGain)
	  flags = flags | 4;

	*(p++) = flags >> 4 & 0xf;
	*(p++) = flags & 0xf;
	*(p++) = 0;
	*(p++) = 0;
	*(p++) = 0;
	*(p++) = 0;

	break;

      case MC_RELAY:		// Never tested

	len = 10;
	*(p++) = len / 2;

	arp = (ActivateRelayParams *) dataPtr;
	if ((arp->tXPlus > 0xff) || (arp->tXMinus > 0xff)
	    || (arp->tYPlus > 0xff) || (arp->tYMinus > 0xff))
	  {
	    arp->tXPlus >>= 4;
	    arp->tXMinus >>= 4;
	    arp->tYPlus >>= 4;
	    arp->tYMinus >>= 4;
	    relays16x = 1;
	  }
	else
	  relays16x = 0;

	u = arp->tXPlus;
	if (u > 0xff)
	  u = 0xff;
	*(p++) = u >> 4 & 0xf;
	*(p++) = u & 0xf;
	u = arp->tXMinus;
	if (u > 0xff)
	  u = 0xff;
	*(p++) = u >> 4 & 0xf;
	*(p++) = u & 0xf;
	u = arp->tYPlus;
	if (u > 0xff)
	  u = 0xff;
	*(p++) = u >> 4 & 0xf;
	*(p++) = u & 0xf;
	u = arp->tYMinus;
	if (u > 0xff)
	  u = 0xff;
	*(p++) = u >> 4 & 0xf;
	*(p++) = u & 0xf;
	*(p++) = 0;
	*(p++) = relays16x;	// no, tohle fakt nevimmm...
	// *(p++) = cl; // XXX: BYLO TAM TOHLE!

	break;

      case MC_PULSE:

	len = 10;
	*(p++) = len / 2;

	pop = (PulseOutParams *) dataPtr;

	p1 = p;

	u = pop->numberPulses;
	if (u > 0xff)
	  u = 0xff;
	*(p++) = (u >> 4) & 0xf;
	*(p++) = u & 0xf;

	if (pop->pulseWidth == 0)
	  {
	    *(p++) = 0;
	    *(p++) = 0;
	    *(p++) = 0;
	    *(p++) = 0;
	    *(p++) = 0;
	    *(p++) = 0;
	    *(p++) = 0;
	    u = (pop->pulsePeriod > 7) ? 7 : pop->pulsePeriod;
	    *(p++) = u;
	  }
	else
	  {
	    u = pop->pulseWidth - 9;
	    *(p++) = (u >> 12) & 0xf;
	    *(p++) = (u >> 8) & 0xf;
	    *(p++) = (u >> 4) & 0xf;
	    *(p++) = u & 0xf;

	    u = pop->pulsePeriod - u - 38;
	    *(p++) = (u >> 12) & 0xf;
	    *(p++) = (u >> 8) & 0xf;
	    *(p++) = (u >> 4) & 0xf;
	    *(p++) = u & 0xf;
	  }

	break;

      case MC_EEPROM:

	len = 4;
	*(p++) = len / 2;
	eepp = dataPtr;
	u = eepp->address & 0x7f;
	if (eepp->write == 0)
	  u = u + 0x80;
	*(p++) = (u >> 4) & 0xf;
	*(p++) = u & 0xf;
	u = eepp->data;
	*(p++) = (u >> 4) & 0xf;
	*(p++) = u & 0xf;
	break;

      case MC_MISC_CONTROL:

	len = 2;
	*(p++) = len / 2;
	mcp = (MiscellaneousControlParams *) dataPtr;

	// Native (st[789]):
	// typedef struct 
	// {
	// unsigned shutterCommand:2;
	// unsigned ledState:2;
	// unsigned fanEnable:1;
	// unsigned reserved:3;
	// } MiscellaneousControlParams;

	switch (camera)
	  {
#ifdef ST5C_SUPPORT
	  case ST5C_CAMERA:
	    flags = 0;
	    break;
#endif
#ifdef ST237_SUPPORT
	  case ST237_CAMERA:
	  case ST237A_CAMERA:
	    if (mcp->fanEnable)
	      flags = 0x10;
	    else
	      flags = 0;
	    break;
#endif
	  default:
	    flags = (mcp->shutterCommand & 3) | ((mcp->ledState & 3) << 2);
	    if (mcp->fanEnable)
	      flags |= 0x10;
	    break;
	  }
	*(p++) = flags >> 4;
	*(p++) = flags & 0xf;
	break;

      case MC_CONTROL_CCD:

	len = 4;
	*(p++) = len / 2;

	flags = 1;

	*(p++) = (flags >> 4) & 0xf;
	*(p++) = flags & 0xf;
	*(p++) = 0;
	*(p++) = 0;
	break;

      case MC_SYSTEM_TEST:

	len = 4;
	*(p++) = len / 2;

	stp = (SystemTestParams *) dataPtr;

	flags = 0;
	if (stp->testClocks)
	  flags = flags | 1;
	if (stp->testMotor)
	  flags = flags | 2;
	if (stp->test5800)
	  flags = flags | 4;

	*(p++) = (flags >> 4) & 0xf;
	*(p++) = flags & 0xf;

	*(p++) = 0;
	*(p++) = 0;
	break;

      case MC_TX_BYTES:

	*(p++) = 0;

	len = ((TXSerialBytesParams *) dataPtr)->dataLength;
	*(p++) = (len >> 4) & 0xf;
	*(p++) = len & 0xf;

	p1 = ((TXSerialBytesParams *) dataPtr)->data;

	for (i = len; i > 0; i--)
	  {
	    u = *(p1++);
	    *(p++) = (u >> 4) & 0xf;
	    *(p++) = u & 0xf;
	  }

	len = 2 * (len + 1);
	break;

      case MC_GET_VERSION:
      case MC_STATUS:
      case MC_TEMP_STATUS:

	len = 0;
	*(p++) = 0;
	break;
      }

  *lenp = len + 4;

}

PAR_ERROR
MicroCommand (MICRO_COMMAND command, CAMERA_TYPE camera,
	      void *txDataPtr, void *rxDataPtr)
{
  PAR_ERROR err = CE_NO_ERROR;
  int len;

  BuildMicroCommand (command, camera, txDataPtr, &len);

  if (!(err = SendMicroBlock (CommandOutBuf, len)))
    err = ValidateMicroResponse (command, camera, rxDataPtr, txDataPtr);

  return err;
}

unsigned short
CalculateEEPROMChecksum (EEPROMContents * eePtr)
{
  unsigned short sum = 0;
  short i;
  unsigned char *p;

  p = ((unsigned char *) eePtr) + 2;
  for (i = 2; i < 0x20; i++)
    sum += *(p++);
  return 0xffff - sum;
}

/* Navrh, ktery by mohl usetrit par nesmyslnych cteni po parportu...
 
PAR_ERROR GetEEPROMByte(CAMERA_TYPE camera, char *byte, int byte_address)
{
    static / global EEPROM_Checksum_OK;

    if(EEPROM_Checcksum_OK)
    	GetEEPROM(); // ktera ho nastavi...

    else
	{
    	eep.address = byte_address;
	if((res = MicroCommand(MC_EEPROM, camera, &eep, &eer)))
	    break;
	*(p++) = eer.data;
	}	
}
 */

PAR_ERROR
GetEEPROM (CAMERA_TYPE camera, EEPROMContents * eePtr)
{
  short i;
  unsigned char *p;
  PAR_ERROR res = CE_NO_ERROR;
  EEPROMParams eep;
  EEPROMResults eer;
  unsigned short cs;

  if (camera == ST237A_CAMERA)
    {
      eePtr->version = 1;
      eePtr->model = 8;
      eePtr->abgType = 0;
      eePtr->badColumns = 0;
      eePtr->trackingOffset = 0;
      eePtr->trackingGain = 0;
      eePtr->imagingOffset = 0;
      eePtr->imagingGain = 560;
      eePtr->columns[0] = 0;
      eePtr->columns[1] = 0;
      eePtr->columns[2] = 0;
      eePtr->columns[3] = 0;
      eePtr->serialNumber[0] = 0x2d;
      eePtr->serialNumber[1] = 0;
      eePtr->checksum = CalculateEEPROMChecksum (eePtr);
      return res;
    }

  p = (unsigned char *) eePtr;
  eep.write = 0;		// WRITE OFF
  eep.data = 0;

  for (i = 0; i <= 0x1f; i++)
    {
      eep.address = i;
      if ((res = MicroCommand (MC_EEPROM, camera, &eep, &eer)))
	break;
      *(p++) = eer.data;
    }

  cs = CalculateEEPROMChecksum (eePtr);

//    if(res)
//    {
  if (eePtr->checksum != cs)
    {
      res = CE_CHECKSUM_ERROR;
    }
  else if (eePtr->version == 0)
    {
      res = CE_EEPROM_ERROR;
    }
  else if (eePtr->version > 1)
    {
      eePtr->version = 1;
      res = CE_EEPROM_ERROR;
    }
//    }
  // else init = 1; // nenacetla se EEPROM

/*    if(init)
    {
	eePtr->version = 1;
	eePtr->model = 4;
	eePtr->abgType = 0;
	eePtr->badColumns = 0;
	eePtr->trackingOffset = 0;
	eePtr->trackingGain = 304;
	eePtr->imagingOffset = 0;
	eePtr->imagingGain = 560;
	eePtr->columns[0] = 0;
	eePtr->columns[1] = 0;
	eePtr->columns[2] = 0;
	eePtr->columns[3] = 0;
	eePtr->serialNumber[0] = 0x2d;
	eePtr->serialNumber[1] = 0;
	eePtr->checksum = CalculateEEPROMChecksum(eePtr);
    }
*/
#ifdef DEBUG
  IO_LOG = IO_LOG_bak;
#endif
  return res;
}

PAR_ERROR
PutEEPROM (CAMERA_TYPE camera, EEPROMContents * eePtr)
{
  short i;
  unsigned char *p;
  PAR_ERROR res = CE_NO_ERROR;
  EEPROMParams eep;
  EEPROMResults eer;
#ifdef DEBUG
  int IO_LOG_bak;
  IO_LOG_bak = IO_LOG;
  IO_LOG = 0;
  if (IO_LOG_bak)
    fprintf (stderr, "GetEEPROM\n");
#endif

  p = (unsigned char *) eePtr;

  eep.write = 1;		// zapis
  eePtr->version = 1;		// Nastavi natvrdo verzi...

  eePtr->checksum = CalculateEEPROMChecksum (eePtr);

  for (i = 0; i < 0x20; i++)
    {
      eep.address = i;
      eep.data = *(p++);
      if ((res = MicroCommand (MC_EEPROM, camera, &eep, &eer)))
	break;
    }

#ifdef DEBUG
  IO_LOG = IO_LOG_bak;
#endif
  return res;
}
