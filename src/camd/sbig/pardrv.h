/*

	PARDRV.H

	Contains the function prototypes and enumerated constants
	for the Parallel Port driver.

	This supports the following devices:

		ST-7/8
		ST-5C/237 (PixCel255/237)
		Industrial ST-K
        TCE-1
        AO-7
        STV

    Version 3.10 - November 11, 2000

	(c)1995-2000 - Santa Barbara Instrument Group

*/
#ifndef _PARDRV_
#define _PARDRV_

/*

	SBIG Specific Code

*/
#define ENV_DOS 0
#define ENV_WIN 1
#define ENV_WIN32 2
#define ENV_WIN32S 3
#define ENV_WINNT 4
#define TARGET ENV_DOS

/*

	Enumerated Constants

	Note that the various constants are declared here as enums
	for ease of declaration but in the structures that use the
	enums unsigned shorts are used to force the various
	16 and 32 bit compilers to use 16 bits.

*/

/*

	Supported Camera Commands

	These are the commands supported by the driver.
	They are prefixed by CC_ to designate them as
	camera commands and avoid conflicts with other
	enums.

	Some of the commands are marked as SBIG use only
	and have been included to enhance testability
	of the driver for SBIG.

*/
typedef enum
{
  /*

     General Use Commands

   */
  /* 1 - 10 */
  CC_START_EXPOSURE = 1, CC_END_EXPOSURE, CC_READOUT_LINE, CC_DUMP_LINES,
  CC_SET_TEMPERATURE_REGULATION, CC_QUERY_TEMPERATURE_STATUS,
  CC_ACTIVATE_RELAY, CC_PULSE_OUT, CC_ESTABLISH_LINK, CC_GET_DRIVER_INFO,

  /* 11 - 20 */
  CC_GET_CCD_INFO, CC_QUERY_COMMAND_STATUS, CC_MISCELLANEOUS_CONTROL,
  CC_READ_SUBTRACT_LINE, CC_UPDATE_CLOCK, CC_READ_OFFSET, CC_OPEN_DRIVER,
  CC_CLOSE_DRIVER, CC_TX_SERIAL_BYTES, CC_GET_SERIAL_STATUS,
  /* 21 - 30 */
  CC_AO_TIP_TILT, CC_AO_SET_FOCUS, CC_AO_DELAY, CC_GET_TURBO_STATUS,
  CC_END_READOUT, CC_GET_US_TIMER, CC_OPEN_DEVICE, CC_CLOSE_DEVICE,
  CC_SET_IRQL, CC_GET_IRQL,
  /* 31 - 40 */
  CC_GET_LINE, CC_GET_LINK_STATUS, CC_GET_DRIVER_HANDLE,
  CC_SET_DRIVER_HANDLE,
  /*

     SBIG Use Only Commands

   */
  CC_SEND_BLOCK =
    90, CC_SEND_BYTE, CC_GET_BYTE, CC_SEND_AD, CC_GET_AD, CC_CLOCK_AD,
  CC_LAST_COMMAND
}
PAR_COMMAND;

/*

	Return Error Codes

	These are the error codes returned by the driver
	function.  They are prefixed with CE_ to designate
	them as camera errors.

*/
#ifndef CE_ERROR_BASE
#define CE_ERROR_BASE 1
#endif /*  */
typedef enum
{
  /* 0 - 10 */
  CE_NO_ERROR, CE_CAMERA_NOT_FOUND =
    CE_ERROR_BASE, CE_EXPOSURE_IN_PROGRESS, CE_NO_EXPOSURE_IN_PROGRESS,
  CE_UNKNOWN_COMMAND, CE_BAD_CAMERA_COMMAND, CE_BAD_PARAMETER,
  CE_TX_TIMEOUT, CE_RX_TIMEOUT, CE_NAK_RECEIVED, CE_CAN_RECEIVED,
  /* 11 - 20 */
  CE_UNKNOWN_RESPONSE, CE_BAD_LENGTH, CE_AD_TIMEOUT, CE_KBD_ESC,
  CE_CHECKSUM_ERROR, CE_EEPROM_ERROR, CE_SHUTTER_ERROR, CE_UNKNOWN_CAMERA,
  CE_DRIVER_NOT_FOUND, CE_DRIVER_NOT_OPEN,
  /* 21 - 30 */
  CE_DRIVER_NOT_CLOSED, CE_SHARE_ERROR, CE_TCE_NOT_FOUND, CE_AO_ERROR,
  CE_ECP_ERROR, CE_MEMORY_ERROR, CE_NEXT_ERROR
}
PAR_ERROR;

/*

	Camera Command State Codes

	These are the return status codes for the Query
	Command Status command.  Theyt are prefixed with
	CS_ to designate them as camera status.

*/
typedef enum
{ CS_IDLE, CS_IN_PROGRESS, CS_INTEGRATING, CS_INTEGRATION_COMPLETE
}
PAR_COMMAND_STATUS;

#define CS_PULSE_IN_ACTIVE 0x8000
#define CS_WAITING_FOR_TRIGGER 0x8000

/*
	Misc. Enumerated Constants

	ABG_STATE7 - Passed to Start Exposure Command
	MY_LOGICAL - General purpose type
	DRIVER_REQUEST - Used with Get Driver Info command
	CCD_REQUEST - Used with Imaging commands to specify CCD
	CCD_INFO_REQUEST - Used with Get CCD Info Command
	PORT - Used with Establish Link Command
	CAMERA_TYPE - Returned by Establish Link and Get CCD Info commands
	SHUTTER_COMMAND, SHUTTER_STATE7 - Used with Start Exposure
		and Miscellaneous Control Commands
	TEMPERATURE_REGULATION - Used with Enable Temperature Regulation
	LED_STATE - Used with the Miscellaneous Control Command
	FILTER_COMMAND, FILTER_STATE - Used with the Miscellaneous
		Control Command
	AD_SIZE, FILTER_TYPE - Used with the GetCCDInfo3 Command
	AO_FOCUS_COMMAND - Used with the AO Set Focus Command

*/
typedef enum
{ ABG_LOW7, ABG_CLK_LOW7, ABG_CLK_MED7, ABG_CLK_HI7 }
ABG_STATE7;
typedef unsigned short MY_LOGICAL;

#define FALSE 0
#define TRUE 1
typedef enum
{ DRIVER_STD, DRIVER_EXTENDED }
DRIVER_REQUEST;
typedef enum
{ CCD_IMAGING, CCD_TRACKING }
CCD_REQUEST;
typedef enum
{ CCD_INFO_IMAGING, CCD_INFO_TRACKING, CCD_INFO_EXTENDED,
  CCD_INFO_EXTENDED_5C
}
CCD_INFO_REQUEST;
typedef enum
{ ABG_NOT_PRESENT, ABG_PRESENT }
IMAGING_ABG;
typedef enum
{ BASE_ADDR, PORT0, PORT1, PORT2 }
PORT;
typedef enum
{ BR_AUTO, BR_9600, BR_19K, BR_38K, BR_57K, BR_115K }
PORT_RATE;
typedef enum
{ ST7_CAMERA =
    4, ST8_CAMERA, ST5C_CAMERA, TCE_CONTROLLER, ST237_CAMERA, STK_CAMERA,
  ST9_CAMERA, STV_CAMERA, ST10_CAMERA
}
CAMERA_TYPE;
typedef enum
{ SC_LEAVE_SHUTTER, SC_OPEN_SHUTTER, SC_CLOSE_SHUTTER, SC_INITIALIZE_SHUTTER
}
SHUTTER_COMMAND;
typedef enum
{ SS_OPEN, SS_CLOSED, SS_OPENING, SS_CLOSING }
SHUTTER_STATE7;
typedef enum
{ REGULATION_OFF, REGULATION_ON, REGULATION_OVERRIDE, REGULATION_FREEZE,
  REGULATION_UNFREEZE, REGULATION_ENABLE_AUTOFREEZE,
  REGULATION_DISABLE_AUTOFREEZE
}
TEMPERATURE_REGULATION;

#define REGULATION_FROZEN_MASK 0x8000
typedef enum
{ LED_OFF, LED_ON, LED_BLINK_LOW, LED_BLINK_HIGH }
LED_STATE;
typedef enum
{ FILTER_LEAVE, FILTER_SET_1, FILTER_SET_2, FILTER_SET_3, FILTER_SET_4,
  FILTER_SET_5, FILTER_STOP, FILTER_INIT
}
FILTER_COMMAND;
typedef enum
{ FS_MOVING, FS_AT_1, FS_AT_2, FS_AT_3, FS_AT_4, FS_AT_5, FS_UNKNOWN
}
FILTER_STATE;
typedef enum
{ AD_UNKNOWN, AD_12_BITS, AD_16_BITS }
AD_SIZE;
typedef enum
{ FW_UNKNOWN, FW_EXTERNAL, FW_VANE, FW_FILTER_WHEEL }
FILTER_TYPE;
typedef enum
{ AOF_HARD_CENTER, AOF_SOFT_CENTER, AOF_STEP_IN, AOF_STEP_OUT
}
AO_FOCUS_COMMAND;

/*

	General Purpose Flags

*/
#define END_SKIP_DELAY 0x8000	/* set in ccd parameter of EndExposure
				   command to skip synchronization
				   delay - Use this to increase the
				   rep rate when taking darks to later
				   be subtracted from SC_LEAVE_SHUTTER
				   exposures such as when tracking and
				   imaging */
#define EXP_WAIT_FOR_TRIGGER_IN 0x80000000	/* set in exposureTime to wait
						   for trigger in pulse */
#define EXP_SEND_TRIGGER_OUT    0x40000000	/* set in exposureTime to send
						   trigger out Y- */
#define EXP_LIGHT_CLEAR			0x20000000	/* set to do light clear of the
							   CCD (Kaiser only) */
#define EXP_TIME_MASK           0x00FFFFFF	/* mask with exposure time to
						   remove flags */

/*

	Defines

*/
#define MIN_ST7_EXPOSURE    11	/* Minimum exposure is 11/100ths second */

/*

	Command Parameter and Results Structs

	Make sure you set your compiler for byte structure alignment
	as that is how the driver was built.

*/
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
  unsigned short /* CCD_REQUEST */ ccd;
  unsigned short readoutMode;
  unsigned short pixelStart;
  unsigned short pixelLength;
}
ReadoutLineParams;
typedef struct
{
  unsigned short buffer;
  unsigned short pixelStart;
  unsigned short pixelLength;
}
GetLineParams;
typedef struct
{
  unsigned short /* CCD_REQUEST */ ccd;
  unsigned short readoutMode;
  unsigned short lineLength;
}
DumpLinesParams;
typedef struct
{
  unsigned short /* CCD_REQUEST */ ccd;
}
EndReadoutParams;
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
  unsigned short /* PORT */ port;
  unsigned short baseAddress;
}
EstablishLinkParams;
typedef struct
{
  unsigned short /* CAMERA_TYPE */ cameraType;
}
EstablishLinkResults;
typedef struct
{
  unsigned short /* DRIVER_REQUEST */ request;
}
GetDriverInfoParams;
typedef struct
{
  unsigned short version;
  char name[64];
  unsigned short maxRequest;
}
GetDriverInfoResults0;
typedef struct
{
  unsigned short /* CCD_INFO_REQUEST */ request;
}
GetCCDInfoParams;
typedef int READOUT_INFO;
typedef struct
{
  unsigned short firmwareVersion;
  unsigned short /* CAMERA_TYPE */ cameraType;
  char name[64];
  unsigned short readoutModes;
  struct
  {
    unsigned short mode;
    unsigned short width;
    unsigned short height;
    unsigned short gain;
    unsigned long pixelWidth;
    unsigned long pixelHeight;
  }
  readoutInfo[20];
}
GetCCDInfoResults0;
typedef struct
{
  unsigned short badColumns;
  unsigned short columns[4];
  unsigned short /* IMAGING_ABG */ imagingABG;
  char serialNumber[10];
}
GetCCDInfoResults2;
typedef struct
{
  unsigned short /* AD_SIZE */ adSize;
  unsigned short /* FILTER_TYPE */ filterType;
}
GetCCDInfoResults3;
typedef struct
{
  unsigned short command;
}
QueryCommandStatusParams;
typedef struct
{
  unsigned short status;
}
QueryCommandStatusResults;
typedef struct
{
  MY_LOGICAL fanEnable;
  unsigned short /* SHUTTER_COMMAND */ shutterCommand;
  unsigned short /* LED_STATE */ ledState;
}
MiscellaneousControlParams;
typedef struct
{
  unsigned short /* CCD_REQUEST */ ccd;
}
ReadOffsetParams;
typedef struct
{
  unsigned short offset;
}
ReadOffsetResults;
typedef struct
{
  unsigned short xDeflection;
  unsigned short yDeflection;
}
AOTipTiltParams;
typedef struct
{
  unsigned short /* AO_FOCUS_COMMAND */ focusCommand;
}
AOSetFocusParams;
typedef struct
{
  unsigned long delay;
}
AODelayParams;
typedef struct
{
  MY_LOGICAL turboDetected;
}
GetTurboStatusResults;
typedef struct
{
  unsigned short port;
}
OpenDeviceParams;
typedef struct
{
  unsigned short level;
}
SetIRQLParams;
typedef struct
{
  unsigned short level;
}
GetIRQLResults;
typedef struct
{
  MY_LOGICAL linkEstablished;
  unsigned short baseAddress;
  unsigned short /* CAMERA_TYPE */ cameraType;
  unsigned long comTotal;
  unsigned long comFailed;
}
GetLinkStatusResults;
typedef struct
{
  unsigned long count;
}
GetUSTimerResults;
typedef struct
{
  unsigned short port;
  unsigned short length;
  unsigned char *source;
}
SendBlockParams;
typedef struct
{
  unsigned short port;
  unsigned short data;
}
SendByteParams;
typedef struct
{
  unsigned short /* CCD_REQUEST */ ccd;
  unsigned short readoutMode;
  unsigned short pixelStart;
  unsigned short pixelLength;
}
ClockADParams;
typedef struct
{
  unsigned short outLength;
  unsigned char *outPtr;
  unsigned short inLength;
  unsigned char *inPtr;
}
SendSTVBlockParams;

/*

	TCE Defines

*/
/*

	Commands - These are the various commands the TCE supports.
			   They are all prefaced with TCEC_ so commands with
			   the same name will not conflict with the Universal
			   CPU or ST-7/8 commands.

*/
typedef enum
{ TCEC_NO_COMMAND, TCEC_GET_ENCODERS, TCEC_SYNC_ENCODERS,
  TCEC_RESET_ENCODERS, TCEC_GET_EXTENDED_ERROR, TCEC_GET_ACTIVITY_STATUS,
  TCEC_MOVE, TCEC_ABORT, TCEC_GOTO, TCEC_COMMUTATE, TCEC_INITIALIZE,
  TCEC_PARK, TCEC_ENABLE_COMMUTATION, TCEC_ACTIVATE_FOCUS,
  TCEC_ENABLE_PEC, TCEC_TRAIN_PEC, TCEC_OUTPUT_DEW, TCEC_TX_TO_EXP,
  TCEC_ENABLE_LIMIT, TCEC_SET_EXP_CONTROL, TCEC_GET_EXP_STATUS,
  TCEC_GET_RESULT_BUF, TCEC_LIMIT_STATUS, TCEC_WRITE_BLOCK,
  TCEC_READ_BLOCK, TCEC_GET_ROM_VERSION, TCEC_SET_COM_BAUD,
  TCEC_SET_MOTION_CONTROL, TCEC_GET_MOTION_CONTROL, TCEC_READ_THERMISTOR,
  TCEC_GET_MOTION_STATE, TCEC_SET_SIDEREAL_CLOCK, TCEC_GET_SIDEREAL_CLOCK,
  TCEC_LOOPBACK_EXP_TEST, TCEC_GET_KEY, TCEC_GET_KEYS,
  TCEC_GET_EEPROM_DATA, TCEC_GET_TCE_INFO, TCEC_SET_EEPROM_DATA,
  TCEC_OPEN_DRIVER =
    0x50, TCEC_CLOSE_DRIVER, TCEC_GET_DRIVER_INFO, TCEC_ESTABLISH_LINK,
  TCEC_HARDWARE_RESET,
  /*

     SBIG Use Only commands - These commands are for use by
     SBIG for production testing of the TCE.

   */
  TCEC_READ_EE_BLOCK =
    0x30, TCEC_WRITE_EE_BLOCK, TCEC_MICRO_OUT, TCEC_READ_R0, TCEC_WRITE_R0,
  TCEC_READ_GA, TCEC_WRITE_GA, TCEC_GET_EBS, TCEC_SEND_BLOCK = 0x40,
}
TCE_COMMAND;

/*

	These enumerated constants are used with the various
	TCE commands.

*/
/*
	Move and Goto speeds - Use with the TCEC_MOVE and TCEC_GOTO commands
*/
typedef enum
{ MOVE_STOP, MOVE_BASE, MOVE_LEAVE, MOVE_0R5X, MOVE_1X, MOVE_2X, MOVE_4X,
  MOVE_8X, MOVE_16X, MOVE_32X, MOVE_64X, MOVE_128X, MOVE_256X, MOVE_SLEW
}
MOVE_SPEED;

/*
	Move Directions
*/
typedef enum
{ MOVE_FORWARD, MOVE_REVERSE }
MOVE_DIRECTION;

/*
	Command Status Values - Returned by the TCEC_GET_ACTIVITY_STATUS cmd
*/
#define TCES_IDLE_STATUS	0	/* command is idle */
#define TCES_IN_PROGRESS	1	/* command is in progress */
#define TCES_DISABLED		0	/* feature is disabled */
#define TCES_ENABLED		1	/* feature is enabled */
#define TCES_EXTENDED_ERROR	99	/* this indicates an extended error occured
					   and the TCEC_GET_EXTENDED_ERROR command
					   should be called for further info */
typedef enum
{ PS_UNPARKED, PS_PARKED, PS_UNPARKING, PS_PARKING, PS_PARK_LOST,
  PS_INITIALIZING,
}
TCES_PARK_STATUS;
typedef enum
{ MS_BASE_RATE, MS_OTHER_RATE, }
TCES_MOVE_STATUS;

/*
	Extended Error Codes - Returned by the TCEC_GET_EXTENDED_ERROR cmd
*/
typedef enum
{ TCEEE_NO_ERROR, TCEEE_LIMIT_HIT, TCEEE_USER_ABORT, TCEEE_RA_TIMEOUT,
  TCEEE_DEC_TIMEOUT, TCEEE_COM_MOTION_TIMEOUT, TCEEE_RA_COM_ERROR,
  TCEE_DEC_COM_ERROR
}
TCE_EXTENDED_ERRORS;

/*

	Structs

	These struct type defines are used with the various commands.
	There are two types of structs, those ending with _DATA and
	those ending with _RESPONSE.

	The XX_DATA structs are used to feed data to the TCE for those
	commands that require data.

	The XX_RESPONSE structs are used to receive data back from the
	TCE in response to some command.

	Finally, the struct definitions start with TCE_ to avoid
	conflicts with any similarly names structs used with the
	Universal CPU and the center of the struct name echos
	the command the struct is associated with.

*/
typedef struct
{
  long rawRA;
  long rawDEC;
  long siderealRA;
  long siderealDEC;
  unsigned short isSynced;
}
TCEGetEncodersResults;
typedef struct
{
  long siderealRA;
  long siderealDEC;
}
TCESyncEncodersParams;
typedef struct
{
  unsigned short error;
}
TCEGetExtendedErrorResults;
typedef struct
{
  unsigned short command;
}
TCEGetActivityStatusParams;
typedef struct
{
  unsigned short command;
  unsigned short status;
}
TCEGetActivityStatusResults;
typedef struct
{
  unsigned short raRate;
  unsigned short decRate;
  unsigned short raDirection;
  unsigned short decDirection;
}
TCEMoveParams;
typedef struct
{
  long deltaRA;
  long deltaDEC;
  unsigned short rate;
}
TCEGotoParams;
typedef struct
{
  unsigned short raRatio;
  unsigned short decRatio;
  unsigned short raDirection;
  unsigned short decDirection;
}
TCEInitializeParams;
typedef struct
{
  unsigned short enable;
}
TCEEnableCommutationParams;
typedef struct
{
  unsigned short tPlus;
  unsigned short tMinus;
}
TCEActivateFocusParams;
typedef struct
{
  unsigned short enable;
}
TCEEnablePECParams;
typedef struct
{
  unsigned short power;
}
TCEOutputDewParams;
typedef struct
{
  unsigned char *dataPtr;
  unsigned short dataLen;
}
TCETxToEXPParams;
typedef struct
{
  unsigned short enable;
}
TCEEnableLimitParams;
typedef struct
{
  unsigned long baud;
  unsigned short control;
}
TCESetEXPControlParams;
typedef struct
{
  unsigned short errs;
}
TCEGetEXPStatusResults;
typedef struct
{
  unsigned short command;
}
TCEGetResultBufParams;
typedef struct
{
  unsigned short detected;
  unsigned short inLimit;
  long rawLimitRA;
  long rawLimitDEC;
  long siderealLimitRA;
  long siderealLimitDEC;
  long limitTime;
}
TCELimitStatusResults;
typedef struct
{
  unsigned short offset;
  unsigned short segment;
  unsigned char *dataPtr;
  unsigned short dataLen;
}
TCEWriteBlockParams;
typedef struct
{
  unsigned short offset;
  unsigned short segment;
  unsigned short length;
}
TCEReadBlockParams;
typedef struct
{
  unsigned short version;
}
TCEGetROMVersionResults;
typedef struct
{
  unsigned long baud;
}
TCESetCOMBaudParams;
typedef struct
{
  unsigned short permanent;
  unsigned short enableCOMTimeout;
  long raBase;
  long decBase;
  long acceleration;
  long maxRate;
}
TCESetMotionControlParams;
typedef struct
{
  unsigned short comTimeoutEnabled;
  long raBase;
  long decBase;
  long acceleration;
  long maxRate;
  unsigned short raRatio;
  unsigned short decRatio;
  unsigned short raDirection;
  unsigned short decDirection;
}
TCEGetMotionControlResults;
typedef struct
{
  unsigned short thermistor;
}
TCEReadThermistorResults;
typedef struct
{
  unsigned short park;
}
TCEParkParams;
typedef struct
{
  long raRate;
  long decRate;
  short deltaRA;
  short deltaDEC;
}
TCEGetMotionStateResults;
typedef struct
{
  long time;
}
TCEGetSiderealClockResults;
typedef struct
{
  long time;
}
TCESetSiderealClockParams;
typedef struct
{
  long baud;
}
TCELoopbackEXPTestParams;
typedef struct
{
  short command;
  short sent;
  short errors;
}
TCELoopbackEXPTestResults;
typedef struct
{
  unsigned short keyStatus;
  unsigned short keyHit;
}
TCEGetKeyResults;
typedef struct
{
  unsigned short number;
  unsigned short keyStatus;
  unsigned short keysHit[16];
}
TCEGetKeysResults;
typedef struct
{
  char serialNumber[10];
  unsigned char data[24];
}
TCEGetEEPROMDataResults;
typedef struct
{
  unsigned short version;
  unsigned short cpu;
  unsigned short firmwareVersion;
  unsigned short raROM;
  unsigned short decROM;
  unsigned short handPaddleROM;
  char name[32];
}
TCEGetTCEInfoResults;
typedef struct
{
  unsigned char data[24];
}
TCESetEEPROMDataParams;

/*

	SBIG Use Only Structs

	These are for use by SBIG in production testing of the TCE.

*/
typedef struct
{
  unsigned short address;
  unsigned char *dataPtr;
  unsigned short dataLen;
}
TCEWriteEEBlockParams;
typedef struct
{
  unsigned short address;
  unsigned short length;
}
TCEReadEEBlockParams;
typedef struct
{
  unsigned short value;
}
TCEWriteR0Params;
typedef struct
{
  unsigned short value;
}
TCEReadR0Results;
typedef struct
{
  short command;
  unsigned short raEB;
  unsigned short decEB;
}
TCEGetEBSResults;
typedef struct
{
  unsigned char motor;
  unsigned char data[5];
}
TCEMicroOutParams;
typedef struct
{
  unsigned short command;
  unsigned char result;
  unsigned char data[5];
}
TCEMicroOutResults;
typedef struct
{
  unsigned short value;
}
TCEWriteGAParams;
typedef struct
{
  unsigned short value;
}
TCEReadGAResults;

/*

	Function Prototypes

	This are the driver interface functions.

    ParDrvCommand() - Supports ST-7/8, ST-5C/237, PixCel255/237
                      STK and AO-7
    TCEDrvCommand() - Supports the TCE-1
    STVDrvCommand() - Supports the STV

	Each function takes a command parameter and pointers
	to parameters and results structs.

	The calling program needs to allocate the memory for
	the parameters and results structs and these routines
	read them and fill them in respectively.

*/
#if TARGET == ENV_DOS
#ifdef __cplusplus
extern "C" short ParDrvCommand (short command, void *Params, void *Results);
extern "C" short TCEDrvCommand (short command, void *Params, void *Results);
extern "C" short STVDrvCommand (short command, void *Params, void *Results);

#else /*  */
extern short ParDrvCommand (short command, void *Params, void *Results);
extern short TCEDrvCommand (short command, void *Params, void *Results);
extern short STVDrvCommand (short command, void *Params, void *Results);

#endif /*  */
#elif TARGET == ENV_WIN
#ifdef __cplusplus
"C" short __export ParDrvCommand (short command, void far * Params,
				  void far * Results);
"C" short __export TCEDrvCommand (short command, void far * Params,
				  void far * Results);
"C" short __export STVDrvCommand (short command, void far * Params,
				  void far * Results);

#else /*  */
short __export ParDrvCommand (short command, void far * Params,
			      void far * Results);
short __export TCEDrvCommand (short command, void far * Params,
			      void far * Results);
short __export STVDrvCommand (short command, void far * Params,
			      void far * Results);

#endif /*  */
#else /*  */
#ifdef __cplusplus
extern "C"
__declspec (dllexport)
     short ParDrvCommand (short command, void *Params, void *Results);
     extern "C" __declspec (dllexport)
     short TCEDrvCommand (short command, void *Params, void *Results);
     extern "C" __declspec (dllexport)
     short STVDrvCommand (short command, void *Params, void *Results);

#else /*  */
extern
__declspec (dllexport)
     short ParDrvCommand (short command, void *Params, void *Results);
     extern __declspec (dllexport)
     short TCEDrvCommand (short command, void *Params, void *Results);
     extern __declspec (dllexport)
     short STVDrvCommand (short command, void *Params, void *Results);

#endif /*  */
#endif /*  */

#endif /*  */
