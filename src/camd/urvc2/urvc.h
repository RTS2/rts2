#ifndef _PARMICRO_H
#define _PARMICRO_H

#ifdef __cplusplus
extern "C"
{
	#endif

	#define IODELAYCONST 0xa

	#ifndef CE_ERROR_BASE
	#define CE_ERROR_BASE 1
	#endif

	#define FAN_ON 1
	#define FAN_OFF 0

	// Particular functions (from modules)
	typedef enum
	{
		CE_NO_ERROR,
		CE_CAMERA_NOT_FOUND = CE_ERROR_BASE,
		CE_EXPOSURE_IN_PROGRESS,
		CE_NO_EXPOSURE_IN_PROGRESS,
		CE_UNKNOWN_COMMAND,
		CE_BAD_CAMERA_COMMAND,
		CE_BAD_PARAMETER,
		CE_TX_TIMEOUT,
		CE_RX_TIMEOUT,
		CE_NAK_RECEIVED,
		CE_CAN_RECEIVED,
		CE_UNKNOWN_RESPONSE,
		CE_BAD_LENGTH,
		CE_AD_TIMEOUT,
		CE_KBD_ESC,
		CE_CHECKSUM_ERROR,
		CE_EEPROM_ERROR,
		CE_SHUTTER_ERROR,
		CE_UNKNOWN_CAMERA,
		CE_DRIVER_NOT_FOUND,
		CE_DRIVER_NOT_OPEN,
		CE_DRIVER_NOT_CLOSED,
		CE_SHARE_ERROR,
		CE_TCE_NOT_FOUND,
		CE_AO_ERROR,
		CE_ECP_ERROR,
		CE_MEMORY_ERROR,
		CE_NEXT_ERROR
	}
	PAR_ERROR;

	typedef unsigned short MY_LOGICAL;
	#define FALSE 0
	#define TRUE 1

	typedef enum
	{
		ST7_CAMERA = 4,
		ST8_CAMERA,				 // 5
		ST5C_CAMERA,			 // 6
		TCE_CONTROLLER,			 // 7
		ST237_CAMERA,			 // 8
		STK_CAMERA,				 // 9
		ST9_CAMERA,				 // 10
		STV_CAMERA,				 // 11
		ST10_CAMERA,			 // 12
		ST237A_CAMERA = 16		 // 16
	}
	CAMERA_TYPE;

	typedef enum
	{
		MAIN_CCD = 0,
		TRACKING_CCD = 4
	}
	CCD_ID;

	void CameraPulse (int a);
	PAR_ERROR CameraReady ();

	PAR_ERROR DigitizeLine_st237 (int left, int len, int horzBefore, int bin,
		unsigned short *dest, int clearWidth);
	PAR_ERROR DigitizeLine_st7 (int left, int len, int horzBefore, int bin,
		unsigned short *dest, int clearWidth);
	PAR_ERROR DigitizeLine_trk (int left, int len, int horzBefore, int bin,
		unsigned short *dest, int clearWidth);

	PAR_ERROR DumpLines_st237 (int, int, int);
	PAR_ERROR DumpLines_st7 (int, int, int);
	PAR_ERROR DumpLines_trk (int, int, int);

	PAR_ERROR DumpPixels_st237 (int);
	PAR_ERROR DumpPixels_st7 (int);
	PAR_ERROR DumpPixels_trk (int);

	PAR_ERROR ClearArray_st237 (int, int, int);
	PAR_ERROR ClearArray_st7 (int, int, int);
	PAR_ERROR ClearArray_trk (int, int, int);

	// The open camera structure
	typedef struct
	{
		PAR_ERROR (*DigitizeLine) (int, int, int, int, unsigned short *, int);
		PAR_ERROR (*DumpPixels) (int);
		PAR_ERROR (*DumpLines) (int, int, int);
		PAR_ERROR (*ClearArray) (int, int, int);

		int ccd;
		int cameraID;

		int horzImage, vertImage;
		int horzTotal, vertTotal;
		int horzBefore, vertBefore;
		int horzAfter, vertAfter;
	} CAMERA;

								 /* Minimum exposure is 11/100ths second */
	#define MIN_ST7_EXPOSURE    11

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
								 // SHUTTER_COMMAND
		unsigned shutterCommand:2;
		unsigned ledState:2;	 // LED_STATE
		unsigned fanEnable:1;
		unsigned reserved:3;
	}
	MiscellaneousControlParams;

	typedef enum
	{
		MC_START_EXPOSURE, MC_END_EXPOSURE, MC_REGULATE_TEMP, MC_TEMP_STATUS,
		MC_RELAY, MC_PULSE, MC_GET_VERSION, MC_EEPROM,
		MC_MISC_CONTROL, MC_STATUS, MC_SYSTEM_TEST, MC_TX_BYTES,
		MC_CONTROL_CCD, MC_REGULATE_TEMP2 = 0x82
	}
	MICRO_COMMAND;

	typedef enum
	{
		ECP_NORMAL, ECP_CLEAR, ECP_BIN1, ECP_BIN2, ECP_BIN3, ECP_FIFO_INPUT,
		ECP_INPUT
	}
	ECP_MODE;

	#ifdef _MICRO_C
	char MC_names[13][32] =
	{
		"START_EXPOSURE", "END_EXPOSURE", "REGULATE_TEMP",
		"TEMP_STATUS", "RELAY", "PULSE", "GET_VERSION", "EEPROM",
		"MISC_CONTROL",
		"STATUS", "SYSTEM_TEST", "TX_BYTES", "CONTROL_CCD"
	};

	char CE_names[32][32] =
	{
		"No error",				 // 0
		"Camera not found",
		"Exposure in progress",	 // 2
		"No exposure in progress",
		"Unknown command",		 // 4
		"Bad camera command",
		"Bad parameter",		 // 6
		"Transmit timeout",

		"Receive timeout",		 // 8
		"NAK received",
		"CAN received",			 // 0xa == 10
		"Unknown response",
		"Bad length",			 // 0xc == 12
		"AD timeout",
		"Kbd esc",				 // 0xe == 14
		"Checksum error",

		"EEPROM error",			 // 0x10 == 16
		"Shutter error",
		"Unknown camera",		 // 0x12 == 18
		"Driver not found",
		"Driver not open",		 // 0x14 == 20
		"Driver not closed",
		"Share error",			 // 0x16 == 22
		"TCE not found",

		"AO error",				 // 0x18 == 24
		"ECP error",
		"Memory error",			 // 0x1a == 26
		"Next error"
	};
	#else
	extern char CE_names[32][32], MC_names[13][32];
	#endif

	typedef struct
	{
		unsigned char model;	 // Model as presented in EEPROM
		unsigned short vertBefore, vertImage, vertAfter;
		unsigned short horzDummy, horzBefore, horzImage, horzAfter;
		unsigned short pixelY, pixelX;
		unsigned char hasTrack;
		unsigned char chipName[16];
		unsigned char fullName[16];
	}
	CameraDescriptionType;

	#ifdef _MICRO_C
	CameraDescriptionType Cams[16] =
	{
		// As a tracking chip in st7/8/9 (artifical model ID)
		// square image area 2640x2640 um
		{0x0, 0, 165, 0, 6, 0, 192, 12, 1600, 1375, 0, "TC-211", "ST4"},
		{0x1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, "?", "?"},
		{0x2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, "?", "?"},
		// Just for reference (does not have parport)
		//{0x3,  0,  244, 0,  3, 12,  376,  2, 2700, 2300, 0,  "TC-241",    "ST6"},
		{0x3, 0, 244, 0, 6, 24, 753, 3, 2700, 1150, 0, "TC-241", "ST6"},

		{0x4, 4, 512, 4, 10, 4, 768, 12, 900, 900, 1, "KAF040x", "ST7"},
		{0x5, 4, 1024, 4, 10, 4, 1536, 12, 900, 900, 1, "KAF160x", "ST8"},
		// No readout values for st5
		{0x6, 0, 240, 0, 0, 0, 320, 0, 1000, 1000, 0, "TC-255", "ST5C"},
		{0x7, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, "?", "?"},
		//  {0x8,  4,  496, 0,  4, 22,  658,  0,  740,  740, 0, "TC-237B",  "ST237"},
		{0x8, 3, 496, 1, 4, 22, 658, 0, 740, 740, 0, "TC-237B", "ST237"},
		// ?
		{0x9, 0, 0, 0, 0, 0, 0, 0, 900, 900, 0, "?", "STK"},

		{0xa, 4, 512, 4, 4, 4, 512, 8, 2000, 2000, 1, "KAF0261", "ST9"},
		{0x8, 3, 496, 1, 4, 23, 658, 0, 740, 740, 0, "TC-237B", "STv"},
		{0xc, 34, 1472, 0, 12, 34, 2184, 34, 680, 680, 1, "KAF320x", "ST10"},
		{0xd, 4, 1024, 4, 4, 4, 1024, 4, 2400, 2400, 0, "KAF1001", "ST1001"},
		{0xf, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, "?", "?"},
	};
	#else
	extern CameraDescriptionType Cams[16];
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
	{							 // s44
								 // 32 0
		unsigned int imagingState;
								 // 32 4
		unsigned int trackingState;
								 // 32 8
		unsigned int shutterStatus;
		unsigned int ledStatus;	 // 32 c
								 // 16 10
		unsigned short xPlusActive;
								 // 16 12
		unsigned short xMinusActive;
								 // 16 14
		unsigned short yPlusActive;
								 // 16 16
		unsigned short yMinusActive;
								 // 16 18
		unsigned short fanEnabled;
								 // 16 1a
		unsigned short CFW6Active;
		unsigned short CFW6Input;// 16 1c
								 // 8 1e <- ?
		unsigned char shutterEdge;
		unsigned int filterState;// 32 20
		unsigned int adSize;	 // 32 24
		unsigned int filterType; // 32 28
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
	static __inline void disable ()
	{
		__asm__ __volatile__ ("cli");
	}
	static __inline void enable ()
	{
		__asm__ __volatile__ ("sti");
	}

	void begin_realtime ();
	void end_realtime ();
	void CameraInit (int base);

	// funkce v marmicro
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
	PAR_ERROR ValidateMicroResponse (MICRO_COMMAND, CAMERA_TYPE, void *,
		void *);
	void BuildMicroCommand (MICRO_COMMAND, CAMERA_TYPE, void *, int *);
	PAR_ERROR MicroCommand (MICRO_COMMAND, CAMERA_TYPE, void *, void *);
	unsigned short CalculateEEPROMChecksum (EEPROMContents *);
	PAR_ERROR GetEEPROM (CAMERA_TYPE, EEPROMContents *);
	PAR_ERROR PutEEPROM (CAMERA_TYPE, EEPROMContents *);

	// funkce v marccd
	PAR_ERROR DigitizeRAWImagingLine (int, unsigned short *);
	PAR_ERROR DigitizeImagingLine (int, int, int, int, int, int,
		unsigned short *, int, int);
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

	PAR_ERROR CCDExpose (CAMERA * C, int Time, int Shutter);
	int CCDImagingState (CAMERA * C);
	PAR_ERROR OpenCCD (int CCD_num, CAMERA ** cptr);
	void CloseCCD (CAMERA * C);
	PAR_ERROR CCDReadout (unsigned short *buffer, CAMERA * C, int tx, int ty,
		int tw, int th, int bin);

	#ifndef _IO_C
	extern int linux_strategy;
	#endif

	#ifndef _MICRO_C
	// promenne v marmicro
								 // kde se VYRABEJI mikroprikazy
	extern unsigned char *CommandOutBuf;
								 // kam se PRIJIMAJI mikroodpovedi
	extern unsigned char *CommandInBuf;
								 // taky to nekde chteji...
	extern unsigned char active_command;
								 // moje promenna, kterou uz nikdo
	extern unsigned char control_out;
	// nepouziva
								 // moje promenna, pouziva ji i
	extern unsigned char imaging_clocks_out;
	// parccd
								 // ?
	extern unsigned char romMSNtoID;
	#endif

	unsigned int ccd_c2ad (double t);
	double ccd_ad2c (unsigned int ad);
	double ambient_ad2c (unsigned int ad);
	int getbaseaddr (int parport_num);

	#ifdef __cplusplus
};
#endif
#endif							 /* !_PARMICRO_H */
