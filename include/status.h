/*! @file Status constant.
 * @author petr
 */

// Camera status

#define CAM_READY 		0x00

#define CAM_MASK_EXPOSE		0x03

#define CAM_NOEXPOSE		0x00
#define CAM_EXPOSE		0x01
#define CAM_READING		0x02
#define CAM_READED		0x03

#define CAM_MASK_COOLING	0x0C

#define CAM_COOL_OF		0x00
#define CAM_FAN_ON		0x04
#define CAM_COOL_PWR		0x08
#define CAM_COOL_TEMP		0x0C

#define CAM_MASK_SHUTTER	0x30

#define CAM_SHUT_CLEARED	0x00
#define CAM_SHUT_SET		0x10
#define CAM_SHUT_TRANS		0x20

#define CAM_MASK_KIND		0xC0

#define CAM_CAM			0x00
#define CAM_CHIP		0x40

// telescope status

#define TEL_READY		0x00
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
