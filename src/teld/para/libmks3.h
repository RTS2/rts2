/**********************************************************

packet structure for MKS3000 commands:

note: uses SLIP-like protocol: if a 0 appears anywhere but the end, send another one.
      and for end-of-packet, send a non-zero char afterwards 0x5a afterwards
note: for 16-bit values, the low byte is first

offset    len(bytes)      purpose
------    ----------      -------
0         2               unit id (LO, HI)
2         1               axis id
3         1               packet sequence number
4         2               packet len (not including end char, but including checksum)
6         2               command code
8         packet_len-10    command data
.
.
n+8       2               checksum (of ENTIRE packet)

n+9       1               end char (0) [not counted as part of len]





COMMAND CODES
------------- */

#define CMD_ACK           1
#define CMD_NACK          2

#define CMD_STATUS        3

#define CMD_VERSION       4
#define CMD_VERSION_DATA_LEN  6
/* data:  WORD     ver Major */
/* data:  WORD     ver Minor */
/* data:  WORD     ver Build */

#define CMD_GET_ID        5

#define CMD_SET_UNIT_ID   6
#define CMD_SET_UNIT_ID_DATA_LEN  2
/* data:  WORD     unit id */

#define CMD_FLASH_ERASE   20
#define CMD_FLASH_ERASE_DATA_LEN  1
/* data: BYTE sector (high 3 bits) */

#define CMD_FLASH_WRITE   21
#define CMD_FLASH_WRITE_DATA_OFFSET 4
/* data:  WORD     start_address
          WORD     end_address
          BYTE     lo-order byte of first address
          BYTE     hi-order byte of first address
          .
          .
          .
          BYTE     byte_n_lo
          BYTE     byte_n_hi
*/

#define CMD_FLASH_READ  22
#define CMD_FLASH_READ_DATA_LEN 4
/* data:  WORD     start_address
          WORD     end_address
*/

#define CMD_RAM_WRITE   23
#define CMD_RAM_WRITE_DATA_OFFSET 4
/* data:  WORD     start_address
          WORD     end_address
          BYTE     lo-order byte of first address
          BYTE     hi-order byte of first address
          .
          .
          .
          BYTE     byte_n_lo
          BYTE     byte_n_hi
*/

#define CMD_RAM_READ  24
#define CMD_RAM_READ_DATA_LEN 4
/* data:  WORD     start_address
          WORD     end_address
*/

#define CMD_STORE_CONSTANTS 25
#define CMD_RELOAD_CONSTANTS 26

#define CMD_START_INDEX         100
#define CMD_START_HOME          101
#define CMD_DO_ABORT            110	/* controlled stop */
#define CMD_MOTOR_OFF           111	/* must not be slewing, sets voltage to 0 */
#define CMD_MOTOR_ON            112	/*  */

#define CMD_DIAGS_MODE_ENTER	120	/* enter diagnostics mode */

#define CMD_DIAGS_CURRENT_SET	121	/* set motor current */
#define CMD_DIAGS_CURRENT_SET_LEN	2	/*  WORD motor current */

#define CMD_DIAGS_ANGLE_SET		122	/* set motor angle in pseudo-degrees */
#define CMD_DIAGS_ANGLE_SET_LEN	2	/*  WORD motor angle */

#define CMD_DIAGS_MODE_LEAVE		123	/* leave diag mode */

#define CMD_OBJ_TRACK_INIT			130
#define CMD_OBJ_TRACK_INIT_RESP_LEN	11
/* data:  DWORD acceleration */
/* data:  DWORD max speed */
/* data:  WORD interrupt period: */
/* data:  BYTE clock rate in megahertz */

#define CMD_OBJ_TRACK_POINT_ADD		131
#define CMD_OBJ_TRACK_POINT_ADD_DATA_LEN	16
/* data: DWORD start_time */
/* data: DWORD end_time  */
/* data: DWORD target_creep_velocity  */
/* data: DWORD position  */

#define CMD_OBJ_TRACK_POINT_ADD_RESP_LEN	15
/* data:  BYTE status: */
#define OBJ_TRACK_STAT_FAILURE		1
#define OBJ_TRACK_STAT_ADDED_OK		2
#define OBJ_TRACK_STAT_ADDED_LATE 	3
#define OBJ_TRACK_STAT_TOO_LATE		4
#define OBJ_TRACK_STAT_NO_SPACE		5
#define OBJ_TRACK_STAT_BAD_ORDER		6
#define OBJ_TRACK_STAT_DUPE_POINT		7
/* data:  BYTE space left in buffer */
/* data:  BYTE index added at */
/* data:  DWORD current time (which is actual start time if added late) */
/* data:  DWORD current position */
/* data:  DWORD current velocity */

#define CMD_OBJ_TRACK_END			132

#define CMD_GET_POS_VEL_TIME			140
#define CMD_GET_POS_VEL_TIME_RESP_LEN	14
/* data: DWORD cur time */
/* data: DWORD cur position  */
/* data: DWORD cur velocity  (creep or slew) */
/* data: WORD  cur point index */

/*#define GET_PEC_TABLE           120 */
    /* data: WORD:     which revolution */
    /* data: 125 WORDS signed words (pec data) */

/* #define SET_PEC_TABLE           121 */
    /* data: PEC_RATIO * 200 signed words */

/*#define CMD_SET_BASERATE  101
  #define CMD_SET_BASERATE_DATA_LEN  4
*/
/* data:  DWORD     rate */

/*#define CMD_GET_BASERATE  102 */

#define CMD_GET_VAL16   200
#define CMD_GET_VAL16_DATA_LEN  2
/* data:  WORD     VAL_ID */

#define CMD_SET_VAL16   201
#define CMD_SET_VAL16_DATA_LEN  4
/* data:  WORD     VAL_ID */
/* data:  WORD     value */

#define CMD_VAL16_INDEX_ANGLE   1
#define CMD_VAL16_EMF_FACTOR    2
#define CMD_VAL16_PEC_RATIO     3
#define CMD_VAL16_LOGGING_INDEX 4
#define CMD_VAL16_BUTTON_CNT    5	/* get only */
#define CMD_VAL16_MAX_ERROR     6
#define CMD_VAL16_MIN_ERROR     7
#define CMD_VAL16_ERROR_LIMIT   8
#define CMD_VAL16_PEC_INDEX     9	/* get only */
#define CMD_VAL16_COMMAND_STATE 10	/* get only */
#define CMD_VAL16_SERVO_STATE   11	/* get only */
#define CMD_VAL16_HOMEDIR       12
#define CMD_VAL16_HOMESENSE1    13
#define CMD_VAL16_HOMESENSE2    14
#define CMD_VAL16_HOME_MODE     15
#define CMD_VAL16_MOTORSTAT     16	/* get only */
#define CMD_VAL16_HOME_SENSORS  17	/* get only */
#define CMD_VAL16_FOCUS_PULSE         18
#define CMD_VAL16_HOME2INDEX          19
#define CMD_VAL16_MOTOR_VOLT_MAX      20
#define CMD_VAL16_MOTOR_PROPORT_MAX   21
#define CMD_VAL16_HOME_PHASE          22	/* get only */
#define CMD_VAL16_BUTTON_CLICK        23	/* get only */
#define CMD_VAL16_BUTTON_DBL_CLICK    24	/* get only */
#define CMD_VAL16_JOY_INDEX           25	/* get only */
#define CMD_VAL16_

#define CMD_GET_VAL32   210
#define CMD_GET_VAL32_DATA_LEN  2
    /* data:  WORD     VAL_ID */

#define CMD_GET_VAL32_RESP_LEN 4
    /* data:  DWORD     value */

#define CMD_SET_VAL32   211
#define CMD_SET_VAL32_DATA_LEN  6
/* data:  WORD     VAL_ID */
/* data:  DWORD     value */

#define CMD_VAL32_BASERATE    1
#define CMD_VAL32_TARGPOS     2
#define CMD_VAL32_CURPOS      4	/* get only */
#define CMD_VAL32_BUTTON_POS  5	/* get only */
#define CMD_VAL32_SQRT_ACCEL  6
#define CMD_VAL32_SLEW_VEL    7
#define CMD_VAL32_MIN_POS_LIM 8
#define CMD_VAL32_MAX_POS_LIM 9
#define CMD_VAL32_ENCODER_POS 10
#define CMD_VAL32_RELPOS      11	/* set only */
#define CMD_VAL32_HOMEVEL_HI  12
#define CMD_VAL32_HOMEVEL_MED 13
#define CMD_VAL32_HOMEVEL_LO  14
#define CMD_VAL32_CREEP_VEL   15
#define CMD_VAL32_CONSTS_VEL_SLEW   16
#define CMD_VAL32_CONSTS_VEL_CREEP  17
#define CMD_VAL32_CONSTS_VEL_GUIDE  18
#define CMD_VAL32_PEC_CUT_SPEED     19
#define CMD_VAL32_SUM_VOLTAGE		  20	/* get only */
#define CMD_VAL32_SUM_VOLTAGE_COUNT 21	/* get only */
#define CMD_VAL32_TICK_TIME         22
#define CMD_VAL32_
typedef unsigned short CWORD16;
typedef unsigned long CWORD32;

/* status bits */

/* shared between F240 flash and AMD flash DO NOT CHANGE */
#define COMM_ERR      (0x1)
#define MOTOR_ERR     (0x2)
#define INTERNAL_FLASH (0x10)
#define FLASH_ERROR   (0x80)

/* AMD flash only */

#define MOTOR_HOMING      (0x0100)
#define MOTOR_SERVO       (0x0200)
#define MOTOR_INDEXING    (0x0400)
#define MOTOR_SLEWING     (0x0800)
#define MOTOR_HOMED       (0x1000)
#define MOTOR_JOYSTICKING (0x2000)
#define MOTOR_OFF         (0x4000)

#define MOTOR_MOVING (MOTOR_HOMING | MOTOR_SLEWING | MOTOR_JOYSTICKING)

/***********************************************/
/* servo and command states */

#define CON_STATE_ERR       1
#define CON_STATE_UN        2
#define CON_STATE_FIELDING  3
#define CON_STATE_FIELDED   4

#define SERVO_STATE_ERROR         1
#define SERVO_STATE_UN            2
#define SERVO_STATE_HOMING_1A_NOT 3
#define SERVO_STATE_HOMED_1A_NOT  4
#define SERVO_STATE_HOMING_1      5
#define SERVO_STATE_HOMED_1       6
#define SERVO_STATE_HOMING_1_NOT  7
#define SERVO_STATE_HOMED_1_NOT   8
#define SERVO_STATE_HOMING_1_P    9
#define SERVO_STATE_HOMED_1_P     10
#define SERVO_STATE_HOMING_2      11
#define SERVO_STATE_HOMED_2       12
#define SERVO_STATE_HOMING_X      13
#define SERVO_STATE_HOMED_X       14
#define SERVO_STATE_ABORTING      15
/* #define SERVO_STATE_ABORTED   10 */
#define SERVO_STATE_SLEW          16
#define SERVO_STATE_CREEP_STOP    17
#define SERVO_STATE_CREEP         18

/***********************************************************/

/* misc constants, types, and macros */

#define MAX_PEC_RATIO         20
#define PEC_REV_COUNT         125
#define PEC_STORAGE_ADDR         0x4000
#define USER_FLASH_SECTOR        0x8000

#define HOME_MODE_NONE      1
#define HOME_MODE_1_SENSOR  2
#define HOME_MODE_2_SENSORS 3
#define HOME_MODE_3_SENSORS 4
#define HOME_MODE_ENUM_MASK 0xff
#define HOME_MODE_IN_OUT_IN       0x100	/* now does (Out)-In-Out-In  7/21/01 MKS */
#define HOME_MODE_HOME_REQUIRED   0x200
#define HOME_MODE_FROM_JOYSTICK   0x400

#define COMM_MAX_PACKET  4000
#define COMM_ENDPACKET  0
#define COMM_ENDPACKET2 (0x5a)
#define COMM_OVERHEADLEN  10
#define COMM_HEADERLEN  8

#define PACKETUINT(array, offset) (array[offset] + (array[(offset)+1] << 8))

#define PACKETULONG(array, offset) (PACKETUINT(array, offset) + (((CWORD32)PACKETUINT(array, offset+2)) << 16))
/*#define PACKETULONG(array, offset) (array[offset] + (array[offset+1] << 8) + (array[offset+2] << 16) + (array[offset+3] << 24)) */
#define PACKLO(wval) ((CWORD16)((wval) & 0xff))
#define PACKHI(wval) ((CWORD16)( 0xff & ((wval) >> 8)))

/* data structures */
typedef struct
{
  CWORD16 unitId;
  CWORD16 axisId;
  CWORD16 sequenceNum;
  CWORD16 packetLen;		/* not including end of packet byte */
  CWORD16 command;
  CWORD16 *pData;		/* pointer to entire packet, from start, initialize before processing, each CWORD16 has value for one byte */
} packet_t;

/* fn prototypes */
/* shared comm functions */
CWORD16 commGetPacket (CWORD16 * packetBuff, CWORD16 * pReadLen,
		       CWORD16 maxLen);
CWORD16 commDecodePacket (CWORD16 * pBuff, CWORD16 len, packet_t * pPack);
CWORD16 commSendPack (packet_t * pPack);

/* implementation-dependent comm functions */
#ifdef __cplusplus
#define LINKAGE extern "C"
#else /*  */
#define LINKAGE
#endif /*  */
LINKAGE CWORD16 commInit (char *);
LINKAGE CWORD16 commCheckChar ();	/* returns 0 for no char, nonzero for char ready */

/* returns 0x100 for timeout, 0x200 for comm error */
LINKAGE CWORD16 commGetChar (CWORD16 maxTimeout);	/* timeout in ticks (at 5.3KHz) */
LINKAGE CWORD16 commPutChar (CWORD16 c);
LINKAGE void debugCharOut (CWORD16 c);
LINKAGE void debugByteOut (CWORD16 c);
LINKAGE void debugWordOut (CWORD16 c);
LINKAGE void debugDWordOut (CWORD32 c);
LINKAGE void debugStrOut (const char *pS);

/* return codes */

#define MKS_OK    0

#define COMM_BASE     1000
#define COMM_OKPACKET     (COMM_BASE    )
#define COMM_NOPACKET     (COMM_BASE + 1)
#define COMM_TIMEOUT      (COMM_BASE + 2)
#define COMM_COMMERROR    (COMM_BASE + 3)
#define COMM_BADCHAR      (COMM_BASE + 4)
#define COMM_OVERRUN      (COMM_BASE + 5)
#define COMM_BADCHECKSUM  (COMM_BASE + 6)
#define COMM_BADLEN       (COMM_BASE + 7)
#define COMM_BADCOMMAND   (COMM_BASE + 8)
#define COMM_INITFAIL     (COMM_BASE + 9)
#define COMM_NACK         (COMM_BASE + 10)
#define COMM_BADID        (COMM_BASE + 11)
#define COMM_BADSEQ       (COMM_BASE + 12)
#define COMM_BADVALCODE (COMM_BASE + 13)

#define MAIN_BASE     2000
#define MAIN_WRONG_UNIT     (MAIN_BASE + 1)
#define MAIN_BADMOTORINIT   (MAIN_BASE + 2)
#define MAIN_BADMOTORSTATE  (MAIN_BASE + 3)	/* e.g. attempt to slew before it's homed */
#define MAIN_BADSERVOSTATE  (MAIN_BASE + 4)	/* e.g. indexing before finding switch 1 */
#define MAIN_SERVOBUSY      (MAIN_BASE + 5)	/* e.g. indexing before finding switch 1 */
#define MAIN_BAD_PEC_LENGTH (MAIN_BASE + 6)
#define MAIN_AT_LIMIT       (MAIN_BASE + 7)	/* at min or max position limit */
#define MAIN_NOT_HOMED      (MAIN_BASE + 8)	/* system not homed but requires it */
#define MAIN_BAD_POINT_ADD	(MAIN_BASE + 9)	/* Object-Tracking point error */

#define FLASH_BASE             3000
#define FLASH_PROGERR         (FLASH_BASE + 1)
/* #define FLASH_ERASEERR (FLASH_BASE + 2) */
#define FLASH_TIMEOUT         (FLASH_BASE + 3)
#define FLASH_CANT_OPEN_FILE  (FLASH_BASE + 4)
#define FLASH_BAD_FILE        (FLASH_BASE + 5)
#define FLASH_FILE_READ_ERR   (FLASH_BASE + 6)
#define FLASH_BADVALID        (FLASH_BASE + 7)

#define MOTOR_BASE            4000
#define MOTOR_OK              (MOTOR_BASE )
#define MOTOR_OVERCURRENT     (MOTOR_BASE + 1)
#define MOTOR_POSERRORLIM     (MOTOR_BASE + 2)	/* maximum position error exceeded */
#define MOTOR_STILL_ON        (MOTOR_BASE + 3)	/* motor still slewing but command needs it stopped */
#define MOTOR_NOT_ON          (MOTOR_BASE + 4)	/* motor off */
#define MOTOR_STILL_MOVING    (MOTOR_BASE + 5)	/* motor still slewing but command needs it stopped */

/* mks3000win.c */
typedef struct
{

  /* MKS3Id(CWORD16 uID=0, CWORD16 xID=0): unitId(uID), axisId(xID) {} */
  CWORD16 unitId;
  CWORD16 axisId;
} MKS3Id;

typedef struct
{
  long prevPointPos;
  CWORD32 prevPointTimeTicks;
  double prevVelocityTicksPerSec;
  double absAccelTicksPerSecPerSec;	/* absolute value of acceleration */
  double maxSpeedTicksPerSec;	/* max. speed it can go */
  double sampleFreq;
} MKS3ObjTrackInfo;

typedef struct
{
  int status;
  long actualPosition;
  double actualTime;
  long targetVelocityTicks;
  int spaceLeft;
  int index;
} MKS3ObjTrackStat;

#define MKS3ConstsIndexAngleSet(id, wIndex) _MKS3DoSetVal16(id, CMD_VAL16_INDEX_ANGLE, wIndex)
#define MKS3ConstsIndexAngleGet(id, pIndex) _MKS3DoGetVal16(id, CMD_VAL16_INDEX_ANGLE, pIndex)
#define MKS3ConstsPECRatioGet(id, pwRatio) _MKS3DoGetVal16(id, CMD_VAL16_PEC_RATIO, pwRatio)
#define MKS3PosTargetGet(id, pPos) _MKS3DoGetVal32(id, CMD_VAL32_TARGPOS, (unsigned long *) (pPos))
#define MKS3Init(port) commInit(port)
#define MKS3DataIndexSet(id, index) _MKS3DoSetVal16(id, CMD_VAL16_LOGGING_INDEX, index)
#define MKS3RateSlewSet(id, dwRate) _MKS3DoSetVal32(id, CMD_VAL32_SLEW_VEL, dwRate)
#define MKS3RateSlewGet(id, pRate) _MKS3DoGetVal32(id, CMD_VAL32_SLEW_VEL, pRate)
#define MKS3RateCreepSet(id, dwRate) _MKS3DoSetVal32(id, CMD_VAL32_CREEP_VEL, dwRate)
#define MKS3RateCreepGet(id, pRate) _MKS3DoGetVal32(id, CMD_VAL32_CREEP_VEL, (unsigned long *)(pRate))
#define MKS3PosCurGet(id, pPos) _MKS3DoGetVal32(id, CMD_VAL32_CURPOS, (unsigned long *)(pPos))
#define MKS3PosTargetSet(id, dwPos) _MKS3DoSetVal32(id, CMD_VAL32_TARGPOS, dwPos)
#define MKS3PosRelativeSet(id, dwPos) _MKS3DoSetVal32(id, CMD_VAL32_RELPOS, dwPos)
#define MKS3PosEncoderGet(id, pPos) _MKS3DoGetVal32(id, CMD_VAL32_ENCODER_POS, (unsigned long *)(pPos))
#define MKS3StateControlGet(id, pState) _MKS3DoGetVal16(id, CMD_VAL16_COMMAND_STATE, pState)
#define MKS3StateServoGet(id, pState) _MKS3DoGetVal16(id, CMD_VAL16_SERVO_STATE, pState)
#define MKS3MotorStatusGet(id, pState) _MKS3DoGetVal16(id, CMD_VAL16_MOTORSTAT, pState)
#define MKS3FocusPulseSet(id, pulseLen) _MKS3DoSetVal16(id, CMD_VAL16_FOCUS_PULSE, pulseLen)
#define MKS3FocusPulseGet(id, pPulseLen) _MKS3DoGetVal16(id, CMD_VAL16_FOCUS_PULSE, (CWORD16 *)(pPulseLen))
#define MKS3ButtonClickCountGet(id, pCount) _MKS3DoGetVal16(id, CMD_VAL16_BUTTON_CLICK, pCount)
#define MKS3ButtonDblClickCountGet(id, pCount) _MKS3DoGetVal16(id, CMD_VAL16_BUTTON_DBL_CLICK, pCount)
#define MKS3ConstsAccelSet(id, dwCoef) _MKS3DoSetVal32(id, CMD_VAL32_SQRT_ACCEL, dwCoef)
#define MKS3ConstsAccelGet(id, pdwCoef) _MKS3DoGetVal32(id, CMD_VAL32_SQRT_ACCEL, pdwCoef)
#define MKS3ConstsMaxPosErrorSet(id, wCoef) _MKS3DoSetVal16(id, CMD_VAL16_ERROR_LIMIT, wCoef)
#define MKS3ConstsMaxPosErrorGet(id, pWCoef) _MKS3DoGetVal16(id, CMD_VAL16_ERROR_LIMIT, pWCoef)
#define MKS3ConstsEmfSet(id, wCoef) _MKS3DoSetVal16(id, CMD_VAL16_EMF_FACTOR, wCoef)
#define MKS3ConstsEmfGet(id, pWCoef) _MKS3DoGetVal16(id, CMD_VAL16_EMF_FACTOR, pWCoef)
#define MKS3ConstsRateBaseSet(id, dwRate) _MKS3DoSetVal32(id, CMD_VAL32_BASERATE, (unsigned long)(dwRate))
#define MKS3ConstsRateBaseGet(id, pRate) _MKS3DoGetVal32(id, CMD_VAL32_BASERATE, (unsigned long *)(pRate))
#define MKS3ConstsPECRatioSet(id, wRatio) _MKS3DoSetVal16(id, CMD_VAL16_PEC_RATIO, wRatio)
#define MKS3ConstsLimMinGet(id, pPos) _MKS3DoGetVal32(id, CMD_VAL32_MIN_POS_LIM, (unsigned long *)(pPos))
#define MKS3ConstsLimMinSet(id, dwPos) _MKS3DoSetVal32(id, CMD_VAL32_MIN_POS_LIM, (unsigned long)(dwPos))
#define MKS3ConstsLimMaxGet(id, pPos) _MKS3DoGetVal32(id, CMD_VAL32_MAX_POS_LIM, (unsigned long *)(pPos))
#define MKS3ConstsLimMaxSet(id, dwPos) _MKS3DoSetVal32(id, CMD_VAL32_MAX_POS_LIM, (unsigned long)(dwPos))
#define MKS3ConstsVelSlewGet(id, pPos) _MKS3DoGetVal32(id, CMD_VAL32_CONSTS_VEL_SLEW, (unsigned long *)(pPos))
#define MKS3ConstsVelSlewSet(id, dwPos) _MKS3DoSetVal32(id, CMD_VAL32_CONSTS_VEL_SLEW, (unsigned long)(dwPos))
#define MKS3ConstsVelCreepGet(id, pPos) _MKS3DoGetVal32(id, CMD_VAL32_CONSTS_VEL_CREEP, (unsigned long *)(pPos))
#define MKS3ConstsVelCreepSet(id, dwPos) _MKS3DoSetVal32(id, CMD_VAL32_CONSTS_VEL_CREEP, (unsigned long)(dwPos))
#define MKS3ConstsVelGuideGet(id, pPos) _MKS3DoGetVal32(id, CMD_VAL32_CONSTS_VEL_GUIDE, (unsigned long *)(pPos))
#define MKS3ConstsVelGuideSet(id, dwPos) _MKS3DoSetVal32(id, CMD_VAL32_CONSTS_VEL_GUIDE, (unsigned long)(dwPos))
#define MKS3ConstsMotorVoltMaxSet(id, wRatio) _MKS3DoSetVal16(id, CMD_VAL16_MOTOR_VOLT_MAX, wRatio)
#define MKS3ConstsMotorVoltMaxGet(id, pwRatio) _MKS3DoGetVal16(id, CMD_VAL16_MOTOR_VOLT_MAX, pwRatio)
#define MKS3ConstsMotorProportMaxSet(id, wRatio) _MKS3DoSetVal16(id, CMD_VAL16_MOTOR_PROPORT_MAX, wRatio)
#define MKS3ConstsMotorProportMaxGet(id, pwRatio) _MKS3DoGetVal16(id, CMD_VAL16_MOTOR_PROPORT_MAX, pwRatio)
#define MKS3ConstsHomeDirSet(id, wDir) _MKS3DoSetVal16(id, CMD_VAL16_HOMEDIR, wDir)
#define MKS3ConstsHomeDirGet(id, pwDir) _MKS3DoGetVal16(id, CMD_VAL16_HOMEDIR, pwDir)
#define MKS3ConstsHomeSense1Set(id, wSense) _MKS3DoSetVal16(id, CMD_VAL16_HOMESENSE1, wSense)
#define MKS3ConstsHomeSense1Get(id, pwSense) _MKS3DoGetVal16(id, CMD_VAL16_HOMESENSE1, pwSense)
#define MKS3ConstsHomeSense2Set(id, wSense) _MKS3DoSetVal16(id, CMD_VAL16_HOMESENSE2, wSense)
#define MKS3ConstsHomeSense2Get(id, pwSense) _MKS3DoGetVal16(id, CMD_VAL16_HOMESENSE2, pwSense)
#define MKS3ConstsHomeVelLoSet(id, dwVel) _MKS3DoSetVal32(id, CMD_VAL32_HOMEVEL_LO, dwVel)
#define MKS3ConstsHomeVelLoGet(id, pdwVel) _MKS3DoGetVal32(id, CMD_VAL32_HOMEVEL_LO, pdwVel)
#define MKS3ConstsHomeVelMedSet(id, dwVel) _MKS3DoSetVal32(id, CMD_VAL32_HOMEVEL_MED, dwVel)
#define MKS3ConstsHomeVelMedGet(id, pdwVel) _MKS3DoGetVal32(id, CMD_VAL32_HOMEVEL_MED, pdwVel)
#define MKS3ConstsHomeVelHiSet(id, dwVel) _MKS3DoSetVal32(id, CMD_VAL32_HOMEVEL_HI, dwVel)
#define MKS3ConstsHomeVelHiGet(id, pdwVel) _MKS3DoGetVal32(id, CMD_VAL32_HOMEVEL_HI, pdwVel)
#define MKS3ConstsHome2IndexSet(id, dwVel) _MKS3DoSetVal16(id, CMD_VAL16_HOME2INDEX, dwVel)
#define MKS3ConstsHome2IndexGet(id, pdwVel) _MKS3DoGetVal16(id, CMD_VAL16_HOME2INDEX, pdwVel)
#define MKS3ConstsHomeModeSet(id, dwVel) _MKS3DoSetVal16(id, CMD_VAL16_HOME_MODE, dwVel)
#define MKS3ConstsHomeModeGet(id, pdwVel) _MKS3DoGetVal16(id, CMD_VAL16_HOME_MODE, pdwVel)
#define MKS3ConstsPECCutSpeedSet(id, vel) _MKS3DoSetVal32(id, CMD_VAL32_PEC_CUT_SPEED, vel)
#define MKS3ConstsPECCutSpeedGet(id, pVel) _MKS3DoGetVal32(id, CMD_VAL32_PEC_CUT_SPEED, pVel)
#define MKS3UserSpaceErase(id) MKS3FlashErase(id, USER_FLASH_SECTOR)
#define MKS3JoyVelIndexGet(id, pIndex) _MKS3DoGetVal16(id, CMD_VAL16_JOY_INDEX, pIndex)
#define MKS3HomeSensorsGet(id, pSensors) _MKS3DoGetVal16(id, CMD_VAL16_HOME_SENSORS, pSensors)
#define MKS3PECIndexGet(id, pIndex) _MKS3DoGetVal16(id, CMD_VAL16_PEC_INDEX, pIndex)
#define MKS3PECDatumGet(id, index, pDatum) MKS3FlashRead(id, PEC_STORAGE_ADDR + (index), PEC_STORAGE_ADDR + (index), pDatum)
#define MKS3PECDatumSet(id, index, Datum) MKS3FlashProgram(id, PEC_STORAGE_ADDR + (index), PEC_STORAGE_ADDR + (index), &(Datum))
#define MKS3PECTableErase(id) MKS3FlashErase(id, PEC_STORAGE_ADDR)

CWORD16 _MKS3DoGetVal16 (const MKS3Id id, CWORD16 valId, CWORD16 * val16);
CWORD16 _MKS3DoSetVal16 (const MKS3Id id, CWORD16 valId, CWORD16 val16);
CWORD16 _MKS3DoGetVal32 (const MKS3Id id, CWORD16 valId,
			 unsigned long *val32);
CWORD16 _MKS3DoSetVal32 (const MKS3Id id, CWORD16 valId, CWORD32 val32);
CWORD16 MKS3StatusGet (const MKS3Id id, CWORD16 * pStat);
CWORD16 MKS3VersionGet (const MKS3Id id, CWORD16 * pMajor, CWORD16 * pMinor,
			CWORD16 * pBuild);
CWORD16 MKS3FlashErase (const MKS3Id id, CWORD16 address);
CWORD16 MKS3FlashProgram (const MKS3Id id, CWORD16 startAdr, CWORD16 endAdr,
			  CWORD16 * data);
CWORD16 MKS3FlashRead (const MKS3Id id, CWORD16 startAdr, CWORD16 endAdr,
		       CWORD16 * data);
CWORD16 MKS3coff16download (const MKS3Id id, const char *coffName);
CWORD16 MKS3RamRead (const MKS3Id id, CWORD16 startAdr, CWORD16 endAdr,
		     CWORD16 * data);
CWORD16 MKS3RamWrite (const MKS3Id id, CWORD16 startAdr, CWORD16 endAdr,
		      CWORD16 * data);
CWORD16 MKS3PosAbort (const MKS3Id id);
CWORD16 MKS3MotorOn (const MKS3Id id);
CWORD16 MKS3MotorOff (const MKS3Id id);
CWORD16 MKS3MotorVoltageGet (const MKS3Id id, long *pSumVoltage,
			     long *pCount);
CWORD16 MKS3PECTableSet (const MKS3Id id, CWORD16 * data);
CWORD16 MKS3PECTableGet (const MKS3Id id, CWORD16 * data);
CWORD16 MKS3UserSpaceWrite (const MKS3Id id, CWORD16 startAdr,
			    CWORD16 endAdr, CWORD16 * pData);
CWORD16 MKS3UserSpaceRead (const MKS3Id id, CWORD16 startAdr,
			   CWORD16 endAdr, CWORD16 * pData);
CWORD16 MKS3ConstsUnitIdGet (const MKS3Id id, CWORD16 * pwId);
CWORD16 MKS3ConstsUnitIdSet (const MKS3Id id, CWORD16 wId);
CWORD16 MKS3ConstsStore (const MKS3Id id);
CWORD16 MKS3ConstsReload (const MKS3Id id);
CWORD16 MKS3MotorIndexFind (const MKS3Id id);
CWORD16 MKS3MotorIndexMeasure (const MKS3Id id, CWORD16 * pIndex);
CWORD16 MKS3Home (const MKS3Id id, long offset);
CWORD16 MKS3HomePhaseGet (const MKS3Id id, CWORD16 * pPhase);
CWORD16 MKS3DiagModeEnter (const MKS3Id id);
CWORD16 MKS3DiagCurrentSet (const MKS3Id id, CWORD16 current);
CWORD16 MKS3DiagAngleSet (const MKS3Id id, CWORD16 angle);
CWORD16 MKS3DiagModeLeave (const MKS3Id id);
CWORD16 MKS3ObjTrackInit (const MKS3Id id, MKS3ObjTrackInfo * pTrackInfo);
CWORD16 MKS3ObjTrackPointAdd (const MKS3Id id,
			      MKS3ObjTrackInfo * pTrackInfo, double timeSec,
			      long position, MKS3ObjTrackStat * pAddStat);
CWORD16 MKS3ObjTrackEnd (const MKS3Id id);
CWORD16 MKS3PosVelTimeGet (const MKS3Id id, long *pPos, long *pVel,
			   long *pTime, long *pIndex);
