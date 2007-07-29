/*! @file 
 * Status constants.
 * 
 * @author petr
 */

#ifndef __RTS__STATUS__
#define __RTS__STATUS__

// mask used to communicate errors which occured on device
#define DEVICE_ERROR_MASK	0x00ff0000

// used to deal blocking functions
// BOP is for Block OPeration
#define BOP_MASK		0xff000000

#define BOP_EXPOSURE		0x01000000
#define BOP_READOUT		0x02000000
#define BOP_TEL_MOVE		0x04000000

#define DEVICE_NO_ERROR		0x00000000
// this status is result of kill command, which occured
#define DEVICE_ERROR_KILL	0x00010000
// unspecified HW error occured
#define DEVICE_ERROR_HW		0x00020000
// device not ready..
#define DEVICE_NOT_READY	0x00040000

#define DEVICE_STATUS_MASK	0x0000ffff

#define DEVICE_IDLE		0x00000000

// Camera status
#define CAM_MASK_CHIP		0x0f
#define CAM_MASK_EXPOSE		0x01

#define CAM_NOEXPOSURE		0x00
#define CAM_EXPOSING		0x01

#define CAM_MASK_READING	0x02

#define CAM_NOTREADING		0x00
#define CAM_READING		0x02

#define CAM_MASK_DATA		0x04

#define CAM_NODATA		0x00
#define CAM_DATA		0x04

#define CAM_MASK_FOCUSING	0x8000

#define CAM_NOFOCUSING		0x0000
#define CAM_FOCUSING		0x8000

#define CAM_MASK_SHUTTER	0x3000

#define CAM_SHUT_CLEARED	0x0000
#define CAM_SHUT_SET		0x1000
#define CAM_SHUT_TRANS		0x2000

#define CAM_MASK_COOLING	0xC000

#define CAM_COOL_OFF		0x0000
#define CAM_COOL_FAN		0x4000
#define CAM_COOL_PWR		0x8000
#define CAM_COOL_TEMP		0xC000

// Photomer status
#define PHOT_MASK_INTEGRATE	0x01
#define PHOT_NOINTEGRATE	0x00
#define PHOT_INTEGRATE		0x01

#define PHOT_MASK_FILTER	0x02
#define PHOT_FILTER_IDLE	0x00
#define PHOT_FILTER_MOVE	0x02

// focuser status
#define FOC_MASK_FOCUSING	0x01
#define FOC_SLEEPING		0x00
#define FOC_FOCUSING		0x01

// telescope status
#define TEL_MASK_MOVING		0x07
#define TEL_MASK_COP_MOVING	0x0f

#define TEL_OBSERVING		0x00
#define TEL_MOVING		0x01
#define TEL_PARKED		0x02
#define TEL_PARKING		0x04

#define TEL_MASK_COP		0x08
#define TEL_NO_WAIT_COP		0x00
#define TEL_WAIT_COP		0x08

#define TEL_MASK_SEARCHING	0x10
#define TEL_NOSEARCH		0x00
#define TEL_SEARCH		0x10

#define TEL_MASK_TRACK		0x20

#define TEL_NOTRACK		0x00
#define TEL_TRACKING		0x20

#define TEL_MASK_CORRECTING	0x40
#define TEL_NOT_CORRECTING	0x00
#define TEL_CORRECTING		0x40

#define TEL_GUIDE_MASK		0x0f00

#define TEL_NOGUIDE		0x0000

#define TEL_GUIDE_NORTH		0x0100
#define TEL_GUIDE_EAST		0x0200
#define TEL_GUIDE_SOUTH		0x0400
#define TEL_GUIDE_WEST		0x0800

// when telescope need stop of observation - when it's aproaching limits etc.
#define TEL_MASK_NEED_STOP	0x1000
#define TEL_NEED_STOP		0x1000

// telescope movement dirs

#define DIR_NORTH	'n'
#define DIR_EAST	'e'
#define DIR_SOUTH	's'
#define DIR_WEST	'w'

// dome status

#define DOME_DOME_MASK		0x0f

#define DOME_UNKNOW		0x00
#define DOME_CLOSED		0x01
#define DOME_OPENING		0x02
#define DOME_OPENED		0x04
#define DOME_CLOSING		0x08

#define DOME_WEATHER_MASK	0x30
#define DOME_WEATHER_OK		0x10
#define DOME_WEATHER_BAD	0x20
#define DOME_WEATHER_UNKNOW	0x30

#define DOME_COP_MASK		0xc0

#define DOME_COP_MASK_MOVE	0x40
#define DOME_COP_NOT_MOVE	0x00
#define DOME_COP_MOVE		0x40

#define DOME_COP_MASK_SYNC	0x80
#define DOME_COP_NOT_SYNC	0x00
#define DOME_COP_SYNC		0x80

#define MIRROR_MASK		0x1f
#define MIRROR_MASK_MOVE	0x10
#define MIRROR_MOVE		0x10
#define MIRROR_NOTMOVE		0x00
#define MIRROR_UNKNOW		0x00
#define MIRROR_A		0x01
#define MIRROR_A_B		0x12
#define MIRROR_B		0x03
#define MIRROR_B_A		0x14

#define FILTERD_MASK		0x02
#define FILTERD_IDLE		0x00
#define FILTERD_MOVE		0x02

#define SERVERD_DAY		0
#define SERVERD_EVENING		1
#define SERVERD_DUSK		2
#define SERVERD_NIGHT		3
#define SERVERD_DAWN		4
#define SERVERD_MORNING		5

#define SERVERD_OFF		11
#define SERVERD_UNKNOW		12

#define SERVERD_STATUS_MASK	0x0f
#define SERVERD_STANDBY_MASK	0x30

#define SERVERD_STANDBY		0x10

// "executor" and related states..
#define EXEC_STATE_MASK		0x0f
#define EXEC_IDLE		0x00
#define EXEC_MOVE		0x01
#define EXEC_ACQUIRE		0x02
#define EXEC_ACQUIRE_WAIT	0x03
#define EXEC_OBSERVE		0x04
#define EXEC_LASTREAD		0x05

#define EXEC_MASK_END		0x10
#define EXEC_NOT_END		0x00
#define EXEC_END		0x10

#define EXEC_MASK_ACQ		0x60
#define EXEC_NOT_ACQ		0x00
#define EXEC_ACQ_OK		0x20
#define EXEC_ACQ_FAILED		0x40

#define IMGPROC_MASK_RUN	0x01
#define IMGPROC_IDLE		0x00
#define IMGPROC_RUN		0x01

// to send data

#define DEVDEM_DATA		0x80

// Errors

#define DEVDEM_E_COMMAND	-1	// invalid command
#define DEVDEM_E_PARAMSNUM	-2	// invalid parameter number
#define DEVDEM_E_PARAMSVAL	-3	// invalid parameter(s) value
#define DEVDEM_E_HW		-4	// some HW failure
#define DEVDEM_E_SYSTEM		-5	// some system error
#define DEVDEM_E_PRIORITY	-6	// error by changing priority
#define DEVDEM_E_TIMEOUT	-7	// connection has time-outed
#define DEVDEM_E_IGNORE         -8	// command mind as unnecessary..

#define DEVDEM_I_QUED		1	// value change was qued

// Client errors goes together, intersection between devdem and plancomm clients
// must be empty.

#define PLANCOMM_E_NAMESPACE	-101	//! invalid namespace
#define PLANCOMM_E_HOSTACCES	-102	//! no route to host and various other problems. See errno for futhre details.
#define PLANCOMM_E_COMMAND	-103	//! invalid command
#define PLANCOMM_E_PARAMSNUM	-104	//! invalid number of parameters to system command

// maximal number of devices
#define MAX_DEVICE		10

// maximal sizes of some important strings
#define DEVICE_NAME_SIZE	50
#define CLIENT_LOGIN_SIZE	50
#define CLIENT_PASSWD_SIZE	50
#define DEVICE_URI_SIZE		80


// device types
#define DEVICE_TYPE_UNKNOW	0
#define DEVICE_TYPE_SERVERD	1
#define DEVICE_TYPE_MOUNT	2
#define DEVICE_TYPE_CCD		3
#define DEVICE_TYPE_DOME	4
#define DEVICE_TYPE_WEATHER	5
#define DEVICE_TYPE_ARCH	6
#define DEVICE_TYPE_PHOT	7
#define DEVICE_TYPE_PLAN	8
#define DEVICE_TYPE_GRB		9
#define DEVICE_TYPE_FOCUS	10
#define DEVICE_TYPE_MIRROR	11
#define DEVICE_TYPE_COPULA	12
#define DEVICE_TYPE_FW		13
#define DEVICE_TYPE_AUGERSH	14
#define DEVICE_TYPE_SENSOR	15

#define DEVICE_TYPE_EXECUTOR    20
#define DEVICE_TYPE_IMGPROC	21
#define DEVICE_TYPE_SELECTOR    22
#define DEVICE_TYPE_SOAP	23
#define DEVICE_TYPE_INDI	24
#define DEVICE_TYPE_LOGD	25

// and more to come..
// #define DEVICE_TYPE_

// default serverd port
#define SERVERD_PORT		5557

#endif /* __RTS__STATUS__ */
