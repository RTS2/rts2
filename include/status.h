/*! @file 
 * Status constants.
 * 
 * @author petr
 */

// Camera status

#define CAM_MASK_EXPOSE		0x01

#define CAM_NOEXPOSURE		0x00
#define CAM_EXPOSING		0x01

#define CAM_MASK_READING	0x02

#define CAM_NOTREADING		0x00
#define CAM_READING		0x02

#define CAM_MASK_DATA		0x04

#define CAM_NODATA		0x00
#define CAM_DATA		0x04

#define CAM_MASK_SHUTTER	0x30

#define CAM_SHUT_CLEARED	0x00
#define CAM_SHUT_SET		0x10
#define CAM_SHUT_TRANS		0x20

#define CAM_MASK_COOLING	0xC0

#define CAM_COOL_OFF		0x00
#define CAM_COOL_FAN		0x40
#define CAM_COOL_PWR		0x80
#define CAM_COOL_TEMP		0xC0


// telescope status
#define TEL_MASK_MOVING		0x01

#define TEL_STILL		0x00
#define TEL_MOVING		0x01

#define TEL_MASK_TRACK		0x02

#define TEL_NOTRACK		0x00
#define TEL_TRACKING		0x02

// dome status

#define DOME_READY		0x00

#define DOME_OPENED		0x00
#define DOME_OPENIGN		0x01
#define DOME_CLOSED		0x02
#define DOME_CLOSING		0x03

// to send data

#define DEVDEM_DATA		0x80

// Errors

#define DEVDEM_E_COMMAND	-1	// invalid command
#define DEVDEM_E_PARAMSNUM	-2	// invalid parameter number
#define DEVDEM_E_PARAMSVAL	-3	// invalid parameter(s) value
#define DEVDEM_E_HW		-4	// some HW failure
#define DEVDEM_E_SYSTEM		-5	// some system error
#define DEVDEM_E_PRIORITY	-6	// error by changing priority

// Client errors goes together, intersection between devdem and plancomm clients
// must be empty.

#define PLANCOMM_E_NAMESPACE	-101	//! invalid namespace
#define PLANCOMM_E_HOSTACCES	-102	//! no route to host and various other problems. See errno for futhre details.
#define PLANCOMM_E_COMMAND	-103	//! invalid command
#define PLANCOMM_E_PARAMSNUM	-104	//! invalid number of parameters to system command
