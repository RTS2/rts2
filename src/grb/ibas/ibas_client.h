#ifndef	IBAS_CLIENT_H_INCLUDED
#define IBAS_CLIENT_H_INCLUDED

#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <time.h>
#include <string.h>
#ifndef IBC_ARCH_MACOSX_CC
#include <poll.h>
#endif
#include <math.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <signal.h>
#include <sys/time.h>
#ifdef IBC_ARCH_MACOSX_CC
#include <unistd.h>
#endif


#define	IBC_ASSUMED_PACKET_SIZE		(400)

#define	IBC_BLOCK_MODE			(0)
#define	IBC_NONBLOCK_MODE		(1)

#define	IBC_UNKNOWN_ENDIAN		(0)
#define	IBC_BIG_ENDIAN			(1)
#define	IBC_LITTLE_ENDIAN		(2)

#define	IBAS_ALERT_TEST_FLAG_NORMAL	(0)	/* genuine alert */
#define	IBAS_ALERT_TEST_FLAG_TEST	(1)	/* test alert */
#define	IBAS_ALERT_TEST_FLAG_Q_PING	(2)	/* ping request from the client */
#define	IBAS_ALERT_TEST_FLAG_R_PING	(4)	/* ping reply */

#define	IBAS_ALERT_ID			(1229078867)	/* ASCII: 'IBAS' */
#define	IBAS_ALERT_TIME_SIZE		(24)
#define	IBAS_ALERT_COMMENT_SIZE		(102)
#define	IBAS_ALERT_SPARE_SIZE		(100)

#define	CRC_BITS			(16)	/* number of bits in CRC-16 CCITT */
#define	BYTE_BITS			(8)	/* number of bits per char */


typedef unsigned char byte;
typedef unsigned short ushort2;

typedef union IBC_ENDIAN_DOUBLE_UNION
{
  unsigned char c[8];
  double d;
}
IBC_ENDIAN_DOUBLE;



typedef struct IBAS_ALERT_STRUCT
{
  short int pkt_type;
  short int test_flag;
  int pkt_number;
  char pkt_time[IBAS_ALERT_TIME_SIZE];	/* 24 chars - UTC */
  int alert_number;
  int alert_subnum;
  double nx_point_ra;		/* degrees J2000 */
  double nx_point_dec;		/* degrees J2000 */
  char nx_point_time[IBAS_ALERT_TIME_SIZE];	/* 24 chars - UTC */
  char grb_time[IBAS_ALERT_TIME_SIZE];	/* 24 chars - UTC */
  double grb_time_err;		/* seconds */
  double grb_ra;		/* degrees J2000 */
  double grb_dec;		/* degrees J2000 */
  double grb_pos_err;		/* arcmin */
  double grb_sigma;
  double grb_timescale;		/* seconds */
  double point_ra;		/* degrees J2000 */
  double point_dec;		/* degrees J2000 */
  int det_flags;
  int att_flags;
  int mult_pos;
  char comment[IBAS_ALERT_COMMENT_SIZE];	/* 102 - ASCIIZ */
  unsigned char spare[IBAS_ALERT_SPARE_SIZE];	/* 100 - undefined */
  unsigned char crc16[2];
}
IBAS_ALERT;


typedef struct IBC_DL_STRUCT
{
  int ID;			/* alert magic number */
  int pid;			/* ibasalertd process ID */
  int seqnum;			/* alert sequence counter (reset at restart) */
  int handle;			/* alert handle (in DL table) */
  IBAS_ALERT a;			/* output alert data */
}
IBC_DL;


#include <ibas_client_error.h>
#include <ibas_client_support.h>
#include <ibas_client_time.h>
#include <ibas_client_api.h>

#endif
