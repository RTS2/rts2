#ifndef _PARMICRO_H
#define _PARMICRO_H

#ifndef CE_ERROR_BASE
#define CE_ERROR_BASE 1
#endif

typedef enum
{
  CE_NO_ERROR,			// 0
  CE_CAMERA_NOT_FOUND = CE_ERROR_BASE,
  CE_EXPOSURE_IN_PROGRESS,
  CE_NO_EXPOSURE_IN_PROGRESS,
  CE_UNKNOWN_COMMAND,		// 4
  CE_BAD_CAMERA_COMMAND,
  CE_BAD_PARAMETER,
  CE_TX_TIMEOUT,
  CE_RX_TIMEOUT,		// 8
  CE_NAK_RECEIVED,
  CE_CAN_RECEIVED,
  CE_UNKNOWN_RESPONSE,
  CE_BAD_LENGTH,		// c
  CE_AD_TIMEOUT,
  CE_KBD_ESC,
  CE_CHECKSUM_ERROR,
  CE_EEPROM_ERROR,		// 10
  CE_SHUTTER_ERROR,
  CE_UNKNOWN_CAMERA,
  CE_DRIVER_NOT_FOUND,
  CE_DRIVER_NOT_OPEN,		// 14
  CE_DRIVER_NOT_CLOSED,
  CE_SHARE_ERROR,
  CE_TCE_NOT_FOUND,
  CE_AO_ERROR,			// 18
  CE_ECP_ERROR,
  CE_MEMORY_ERROR,
  CE_NEXT_ERROR			// 1b
}
PAR_ERROR;

typedef unsigned short MY_LOGICAL;
#define FALSE 0
#define TRUE 1

typedef enum
{
  ST7_CAMERA = 4,
  ST8_CAMERA,
  ST5C_CAMERA,
  TCE_CONTROLLER,
  ST237_CAMERA,
  STK_CAMERA,
  ST9_CAMERA,
  STV_CAMERA,
  ST10_CAMERA
}
CAMERA_TYPE;

typedef enum
{
  MAIN_CCD = 0,
  TRACKING_CCD = 4
}
CCD_ID;

#define MIN_ST7_EXPOSURE    11	/* Minimum exposure is 11/100ths second */

typedef struct
{
  unsigned short /* CCD_REQUEST */ ccd;
  unsigned long exposureTime;
  unsigned short /* ABG_STATE7 */ abgState;
  unsigned short /* SHUTTER_COMMAND */ openShutter;
}
StartExposureParams;

typedef struct
{
  unsigned short /* CCD_REQUEST */ ccd;
}
EndExposureParams;

typedef struct
{
  unsigned short /* TEMPERATURE_REGULATION */ regulation;
  unsigned short ccdSetpoint;
}
SetTemperatureRegulationParams;

typedef struct
{
  MY_LOGICAL enabled;
  unsigned short ccdSetpoint;
  unsigned short power;
  unsigned short ccdThermistor;
  unsigned short ambientThermistor;
}
QueryTemperatureStatusResults;

typedef struct
{
  unsigned short tXPlus;
  unsigned short tXMinus;
  unsigned short tYPlus;
  unsigned short tYMinus;
}
ActivateRelayParams;

typedef struct
{
  unsigned short numberPulses;
  unsigned short pulseWidth;
  unsigned short pulsePeriod;
}
PulseOutParams;

typedef struct
{
  unsigned short dataLength;
  unsigned char data[256];
}
TXSerialBytesParams;

typedef struct
{
  unsigned short bytesSent;
}
TXSerialBytesResults;

typedef struct
{
  MY_LOGICAL clearToCOM;
}
GetSerialStatusResults;

typedef struct
{
  unsigned shutterCommand:2;	// SHUTTER_COMMAND
  unsigned ledState:2;		// LED_STATE
  unsigned fanEnable:1;
  unsigned reserved:3;
}
MiscellaneousControlParams;

typedef enum
{
  MC_START_EXPOSURE, MC_END_EXPOSURE, MC_REGULATE_TEMP, MC_TEMP_STATUS,
  MC_RELAY,
  MC_PULSE, MC_GET_VERSION, MC_EEPROM, MC_MISC_CONTROL, MC_STATUS,
  MC_SYSTEM_TEST, MC_TX_BYTES, MC_CONTROL_CCD, MC_REGULATE_TEMP2 = 0x82
}
MICRO_COMMAND;

typedef enum
{
  ECP_NORMAL, ECP_CLEAR, ECP_BIN1, ECP_BIN2, ECP_BIN3, ECP_FIFO_INPUT,
  ECP_INPUT
}
ECP_MODE;

#ifdef _PARMICRO_C
char MC_names[13][32] = { "START_EXPOSURE", "END_EXPOSURE", "REGULATE_TEMP",
  "TEMP_STATUS", "RELAY", "PULSE", "GET_VERSION", "EEPROM",
  "MISC_CONTROL",
  "STATUS", "SYSTEM_TEST", "TX_BYTES", "CONTROL_CCD"
};

char CE_names[32][32] = {
  "No error",			// 0
  "Camera not found",
  "Exposure in progress",	// 2
  "No exposure in progress",
  "Unknown command",		// 4
  "Bad camera command",
  "Bad parameter",		// 6
  "Transmit timeout",

  "Receive timeout",		// 8
  "NAK received",
  "CAN received",		// 0xa == 10
  "Unknown response",
  "Bad length",			// 0xc == 12
  "AD timeout",
  "Kbd esc",			// 0xe == 14
  "Checksum error",

  "EEPROM error",		// 0x10 == 16
  "Shutter error",
  "Unknown camera",		// 0x12 == 18
  "Driver not found",
  "Driver not open",		// 0x14 == 20
  "Driver not closed",
  "Share error",		// 0x16 == 22
  "TCE not found",

  "AO error",			// 0x18 == 24
  "ECP error",
  "Memory error",		// 0x1a == 26
  "Next error"
};
#else
extern char CE_names[32][32], MC_names[13][32];
#endif

typedef struct
{
  unsigned char model;		// Model as presented in EEPROM
  unsigned short vertBefore, vertImage, vertAfter;
  unsigned short horzBefore, horzImage, horzAfter;
  unsigned short pixelY, pixelX;
  unsigned char hasTrack;
  unsigned char chipName[16];
  unsigned char fullName[16];
}
CameraDescriptionType;

#ifdef _PARMICRO_C
CameraDescriptionType Cams[16] = {
  // As a tracking chip in st7/8/9 (artifical model ID)
  {0x0, 0, 165, 0, 0, 192, 0, 1600, 1375, 0, "TC-211", "ST4"},

  {0x1, 0, 0, 0, 0, 0, 0, 0, 0, 0, "?", "?"},
  {0x2, 0, 0, 0, 0, 0, 0, 0, 0, 0, "?", "?"},

  // Just for reference (does not have parport)
  {0x3, 0, 242, 0, 0, 375, 0, 2700, 2300, 0, "TC-241", "ST6"},

  {0x4, 4, 512, 4, 14, 768, 12, 900, 900, 1, "KAF040x", "ST7"},
  {0x5, 4, 1024, 4, 14, 1536, 12, 900, 900, 1, "KAF160x", "ST8"},

  // No readout values :(
  {0x6, 0, 240, 0, 0, 320, 0, 1000, 1000, 0, "TC-255", "ST5C"},

  // ?
  {0x7, 0, 0, 0, 0, 0, 0, 0, 0, 0, "?", "TCE"},

  // ?
  {0x8, 0, 495, 0, 0, 657, 0, 740, 740, 0, "TC-237", "ST237"},

  // ?
  {0x9, 0, 0, 0, 0, 0, 0, 900, 900, 0, "?", "STK"},

  {0xa, 4, 512, 4, 8, 512, 8, 2000, 2000, 1, "KAF0261", "ST9"},
  {0xb, 0, 495, 0, 0, 657, 0, 740, 740, 0, "TC-237", "STV"},
  {0xc, 0, 1472, 0, 0, 2184, 0, 680, 680, 1, "KAF320x", "ST10"},
  {0xd, 0, 1024, 0, 0, 1024, 0, 2400, 2400, 0, "KAF1001", "ST1001"},
  {0xf, 0, 0, 0, 0, 0, 0, 0, 0, 0, "?", "?"},
};
#else
CameraDescriptionType Cams[16];
#endif

typedef struct
{
  unsigned short testClocks, testMotor, test5800;
}
SystemTestParams;

typedef struct
{
  unsigned short firmwareVersion, cameraID;
}
GetVersionResults;

typedef struct
{				// s44
  unsigned int imagingState;	// 32 0
  unsigned int trackingState;	// 32 4
  unsigned int shutterStatus;	// 32 8
  unsigned int ledStatus;	// 32 c
  unsigned short xPlusActive;	// 16 10
  unsigned short xMinusActive;	// 16 12
  unsigned short yPlusActive;	// 16 14
  unsigned short yMinusActive;	// 16 16
  unsigned short fanEnabled;	// 16 18
  unsigned short CFW6Active;	// 16 1a
  unsigned short CFW6Input;	// 16 1c
  unsigned char shutterEdge;	// 8 1e <- ?
  unsigned int filterState;	// 32 20
  unsigned int adSize;		// 32 24
  unsigned int filterType;	// 32 28
}
StatusResults;

typedef struct
{
  unsigned short checksum;
  unsigned char version;
  unsigned char model;
  unsigned char abgType;
  unsigned char badColumns;
  unsigned short trackingOffset;
  unsigned short trackingGain;
  unsigned short imagingOffset;
  unsigned short imagingGain;
  unsigned short columns[4];
  unsigned char serialNumber[10];
}
EEPROMContents;

typedef struct
{
  unsigned char address, data;
  MY_LOGICAL write;
}
EEPROMParams;

typedef struct
{
  unsigned char data;
}
EEPROMResults;

typedef struct
{
  int regulation;
  unsigned short ccdSetpoint, preload;
}
MicroTemperatureRegulationParams;

typedef struct
{
  unsigned short freezeTE, lowerPGain;
}
MicroTemperatureSpecialParams;

// funkce v mario - inb/outb v <sys/io.h>
#define outportb(port, value) outb(value, port)
#define inportb(port) inb(port)
static __inline void
disable ()
{
  __asm__ __volatile__ ("cli");
}
static __inline void
enable ()
{
  __asm__ __volatile__ ("sti");
}

void begin_realtime ();
void end_realtime ();

// funkce v marmicro
void DisableLPTInterrupts (int);
void ForceMicroIdle ();
unsigned char CameraIn (unsigned char);
void CameraOut (unsigned char, int);
int MicroStat ();
unsigned char MicroIn (MY_LOGICAL);
void MicroOut (int);
double mSecCount ();
unsigned long TickCount ();
void RelayClick ();
void TimerDelay (unsigned long);
PAR_ERROR SendMicroBlock (unsigned char *, int);
PAR_ERROR ValGetMicroBlock (MICRO_COMMAND, CAMERA_TYPE, void *);
PAR_ERROR ValGetMicroAck ();
PAR_ERROR ValidateMicroResponse (MICRO_COMMAND, CAMERA_TYPE, void *, void *);
void BuildMicroCommand (MICRO_COMMAND, CAMERA_TYPE, void *, int *);
PAR_ERROR MicroCommand (MICRO_COMMAND, CAMERA_TYPE, void *, void *);
unsigned short CalculateEEPROMChecksum (EEPROMContents *);
PAR_ERROR GetEEPROM (CAMERA_TYPE, EEPROMContents *);
PAR_ERROR PutEEPROM (CAMERA_TYPE, EEPROMContents *);

// funkce v marccd
PAR_ERROR DigitizeRAWImagingLine (int, unsigned short *);
PAR_ERROR DigitizeImagingLine (int, int, int, int, int, int, unsigned short *,
			       int, int);
PAR_ERROR DigitizeImagingLineGK (int, int, int, int, int, int,
				 unsigned short *, int, int);
PAR_ERROR DigitizeTrackingLine (int, int, int, unsigned short *, int);
PAR_ERROR DigitizeST5CLine (int, int, int, unsigned short *, int);
PAR_ERROR DigitizeST237Line (int, int, int, unsigned short *, int);
PAR_ERROR DumpImagingLines (int, int, int);
PAR_ERROR DumpTrackingLines (int, int, int);
PAR_ERROR DumpST5CLines (int, int, int);
PAR_ERROR BlockClearPixels (int, int);
PAR_ERROR BlockClearTrackingPixels (int);
PAR_ERROR BlockClearST5CPixels (int);
PAR_ERROR ClearTrackingPixels (int);
PAR_ERROR ClearST5CPixels (int);
PAR_ERROR ClearImagingArray (int, int, int, int);
PAR_ERROR ClearImagingArrayGK (int, int, int, int);
PAR_ERROR ClearTrackingArray (int, int, int);
PAR_ERROR OffsetImagingArray (int, int, unsigned short *, int);
PAR_ERROR OffsetImagingArrayGK (int, int, unsigned short *, int);
PAR_ERROR OffsetTrackingArray (int, int, unsigned short *, int);
PAR_ERROR OffsetST5CArray (int, unsigned short *, int);
PAR_ERROR MeasureImagingBias (int, int);
PAR_ERROR MeasureImagingBiasGK (int, int);
PAR_ERROR MeasureTrackingBias (int);
PAR_ERROR MeasureST5CBias (int);
PAR_ERROR MeasureST237Bias (int);
PAR_ERROR ClockAD (int);
void IODelay (short i);
void cpy (char *, char *, int);
void scpy (char *, char *);
void clear (char *, int);
void ECPSetMode (ECP_MODE);
PAR_ERROR ECPDumpFifo ();
PAR_ERROR ECPGetPixel (unsigned short *);
PAR_ERROR SPPGetPixel (unsigned short *vidPtr, CCD_ID chip);
MY_LOGICAL ECPDetectHardware ();
PAR_ERROR ECPClear1Group ();
PAR_ERROR ECPWaitPCLK1 ();
void SetVdd (MY_LOGICAL raiseIt);

#ifndef _MARIO_C
extern int linux_strategy;
#endif

#ifndef _PARMICRO_C
// promenne v marmicro
extern unsigned char *CommandOutBuf;	// kde se VYRABEJI mikroprikazy
extern unsigned char *CommandInBuf;	// kam se PRIJIMAJI mikroodpovedi
extern unsigned char active_command;	// taky to nekde chteji...
extern unsigned char control_out;	// moje promenna, kterou uz nikdo
					// nepouziva
extern unsigned char imaging_clocks_out;	// moje promenna, pouziva ji i
						// parccd 
extern unsigned char romMSNtoID;	// ?
#endif

#ifndef _PARDRV_C
extern unsigned short baseAddress;
#endif

#endif
