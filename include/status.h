/* 
 * Basic constants.
 * Copyright (C) 2001-2010 Petr Kubanek <petr@kubanek.net>
 * Copyright (C) 2011 Petr Kubanek, Institute of Physics <kubanek@fzu.cz>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

/**
 * @file
 * Status constants.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */

#ifndef __RTS2__STATUS__
#define __RTS2__STATUS__

#include <inttypes.h>

typedef uint32_t rts2_status_t;

// mask used to communicate errors which occured on device
#define DEVICE_ERROR_MASK   0x000f0000

#define DEVICE_NO_ERROR     0x00000000
// this status is result of kill command, which occured
#define DEVICE_ERROR_KILL   0x00010000
// unspecified HW error occured
#define DEVICE_ERROR_HW     0x00020000
// device not ready..
#define DEVICE_NOT_READY    0x00040000

/**
 * Used to deal blocking functions.
 * BOP is for Block OPeration. BOP is used mainly in Rts2Command and
 * Rts2Value.
 *
 * @ingroup RTS2Block
 */
#define BOP_MASK            0x3f000000

/**
 * Block exposures.
 *
 * @ingroup RTS2Block
 */
#define BOP_EXPOSURE        0x01000000

/**
 * Block readout of CCD.
 *
 * @ingroup RTS2Block
 */
#define BOP_READOUT         0x02000000

/**
 * Block telescope movement.
 *
 * @ingroup RTS2Block
 */
#define BOP_TEL_MOVE        0x04000000

/**
 * Detector status issued before the detector takes an exposure.
 *
 * @ingroup RTS2Block
 */
#define BOP_WILL_EXPOSE     0x08000000

/**
 * Waiting for exposure trigger.
 *
 * @ingroup RTS2Block
 */
#define BOP_TRIG_EXPOSE     0x10000000

/**
 * Stop any move - emergency state.
 *
 * @ingroup RTS2Block
 */
#define STOP_MASK           0x40000000

/**
 * System can physicaly move devices.
 */
#define CAN_MOVE            0x00000000

/**
 * Stop any move.
 *
 * @ingroup RTS2Block
 */
#define STOP_EVERYTHING     0x40000000

/**
 * Mask for device weather voting.
 *
 * @ingroup RTS2Block
 */
#define WEATHER_MASK        0x80000000

/**
 * Allow operations as weather is acceptable,
 *
 * @ingroup RTS2Block
 */
#define GOOD_WEATHER        0x00000000

/**
 * Block observations because there is bad weather.
 *
 * @ingroup RTS2Block
 */
#define BAD_WEATHER         0x80000000

#define DEVICE_STATUS_MASK  0x00000fff

// weather reason mask
#define WR_MASK             0x00f00000

#define WR_RAIN             0x00100000
#define WR_WIND             0x00200000
#define WR_HUMIDITY         0x00400000
#define WR_CLOUD            0x00800000

/**
 * Miscelaneous mask.
 *
 * @ingroup RTS2Block
 */
#define DEVICE_MISC_MASK    0x0000f000

/**
 * State change was caused by command being run on current connection.
 * Currently only used to signal that the exposure was caused
 * by the connection.
 */
#define DEVICE_SC_CURR      0x00001000

// device need reload when it enter idle state - what should be reload is device specific
#define DEVICE_NEED_RELOAD  0x00002000

// device is starting up
#define DEVICE_STARTUP      0x00004000

// device is in shutdown
#define DEVICE_SHUTDOWN     0x00008000

#define DEVICE_IDLE         0x00000000

// Camera status
#define CAM_MASK_CHIP       0x0ff
#define CAM_MASK_EXPOSE     0x011
#define CAM_MASK_SHIFTING   0x020

#define CAM_NOEXPOSURE      0x000
#define CAM_EXPOSING        0x001
#define CAM_EXPOSING_NOIM   0x010

#define CAM_SHIFT           0x020

#define CAM_MASK_READING    0x002

#define CAM_NOTREADING      0x000
#define CAM_READING         0x002

#define CAM_MASK_FT         0x004
#define CAM_FT              0x004
#define CAM_NOFT            0x000

#define CAM_MASK_HAS_IMAGE  0x008
#define CAM_HAS_IMAGE       0x008
#define CAM_HASNOT_IMAGE    0x000

#define CAM_WORKING         (CAM_EXPOSING | CAM_EXPOSING_NOIM | CAM_READING)

#define CAM_MASK_SHUTTER    0x300

#define CAM_SHUT_CLEARED    0x000
#define CAM_SHUT_SET        0x100
#define CAM_SHUT_TRANS      0x200

// Photomer status
#define PHOT_MASK_INTEGRATE 0x001
#define PHOT_NOINTEGRATE    0x000
#define PHOT_INTEGRATE      0x001

#define PHOT_MASK_FILTER    0x002
#define PHOT_FILTER_IDLE    0x000
#define PHOT_FILTER_MOVE    0x002

// focuser status
#define FOC_MASK_FOCUSING   0x001
#define FOC_SLEEPING        0x000
#define FOC_FOCUSING        0x001

// rotator status
#define ROT_MASK_ROTATING   0x001
#define ROT_IDLE            0x000
#define ROT_ROTATING        0x001

// telescope status
#define TEL_MASK_MOVING     0x007
#define TEL_MASK_CUP_MOVING 0x00f

#define TEL_OBSERVING       0x000
#define TEL_MOVING          0x001
#define TEL_PARKED          0x002
#define TEL_PARKING         0x004

#define TEL_MASK_CUP        0x008
#define TEL_NO_WAIT_CUP     0x000
#define TEL_WAIT_CUP        0x008

#define TEL_MASK_TRACK      0x020

#define TEL_NOTRACK         0x000
#define TEL_TRACKING        0x020

#define TEL_MASK_CORRECTING 0x040
#define TEL_NOT_CORRECTING  0x000
#define TEL_CORRECTING      0x040

// when telescope need stop of observation - when it's aproaching limits etc.
#define TEL_MASK_NEED_STOP  0x080
#define TEL_NEED_STOP       0x080

// sensor performing command
#define SENSOR_INPROGRESS   0x001

// telescope movement dirs

#define DIR_NORTH           'n'
#define DIR_EAST            'e'
#define DIR_SOUTH           's'
#define DIR_WEST            'w'

// dome status

#define DOME_DOME_MASK      0x01f

#define DOME_UNKNOW         0x000
#define DOME_CLOSED         0x001
#define DOME_OPENING        0x002
#define DOME_OPENED         0x004
#define DOME_CLOSING        0x008
#define DOME_WAIT_CLOSING   0x010

#define DOME_CUP_MASK       0x0c0

#define DOME_CUP_MASK_MOVE  0x040
#define DOME_CUP_NOT_MOVE   0x000
#define DOME_CUP_MOVE       0x040

#define DOME_CUP_MASK_SYNC  0x080
#define DOME_CUP_NOT_SYNC   0x000
#define DOME_CUP_SYNC       0x080

#define MIRROR_MASK         0x01f
#define MIRROR_MASK_MOVE    0x010
#define MIRROR_MOVE         0x010
#define MIRROR_NOTMOVE      0x000

#define FILTERD_MASK        0x002
#define FILTERD_IDLE        0x000
#define FILTERD_MOVE        0x002

#define SERVERD_STATUS_MASK   0x00f

#define SERVERD_DAY         0x000
#define SERVERD_EVENING     0x001
#define SERVERD_DUSK        0x002
#define SERVERD_NIGHT       0x003
#define SERVERD_DAWN        0x004
#define SERVERD_MORNING     0x005

#define SERVERD_UNKNOW        13

#define SERVERD_ONOFF_MASK    0x030

#define SERVERD_ON            0x000
#define SERVERD_STANDBY       0x010

#define SERVERD_SOFT_OFF      0x020
// set when it is a real off state blocking all domes
#define SERVERD_HARD_OFF      0x030

// "executor" and related states..
#define EXEC_STATE_MASK     0x00f
#define EXEC_IDLE           0x000
#define EXEC_MOVE           0x001
#define EXEC_ACQUIRE        0x002
#define EXEC_ACQUIRE_WAIT   0x003
#define EXEC_OBSERVE        0x004
#define EXEC_LASTREAD       0x005

#define EXEC_MASK_END       0x010
#define EXEC_NOT_END        0x000
#define EXEC_END            0x010

#define EXEC_MASK_ACQ       0x060
#define EXEC_NOT_ACQ        0x000
#define EXEC_ACQ_OK         0x020
#define EXEC_ACQ_FAILED     0x040

#define EXEC_MASK_SCRIPT    0x080
#define EXEC_SCRIPT_RUNNING 0x080

#define IMGPROC_MASK_RUN    0x001
#define IMGPROC_IDLE        0x000
#define IMGPROC_RUN         0x001

// selector simulating..
#define SEL_SIMULATING      0x001
#define SEL_IDLE            0x000

// to send data

#define DEVDEM_DATA         0x080

// all OK
#define DEVDEM_OK           0

// Errors

#define DEVDEM_E_COMMAND    -1	 // invalid command
#define DEVDEM_E_PARAMSNUM  -2	 // invalid parameter number
#define DEVDEM_E_PARAMSVAL  -3	 // invalid parameter(s) value
#define DEVDEM_E_HW         -4	 // some HW failure
#define DEVDEM_E_SYSTEM     -5	 // some system error
#define DEVDEM_E_PRIORITY   -6	 // error by changing priority
#define DEVDEM_E_TIMEOUT    -7	 // connection has time-outed
#define DEVDEM_E_IGNORE     -8	 // command was ignored

#define DEVDEM_I_QUED       1	 // value change was qued

// Client errors goes together, intersection between devdem and plancomm clients
// must be empty.

								 //! invalid namespace
#define PLANCOMM_E_NAMESPACE -101
								 //! no route to host and various other problems. See errno for futhre details.
#define PLANCOMM_E_HOSTACCES -102
#define PLANCOMM_E_COMMAND   -103//! invalid command
#define PLANCOMM_E_PARAMSNUM -104//! invalid number of parameters to system command

// maximal sizes of some important strings
#define DEVICE_NAME_SIZE      50
#define CLIENT_LOGIN_SIZE     50
#define CLIENT_PASSWD_SIZE    50
#define DEVICE_URI_SIZE       80

// device types
#define DEVICE_TYPE_UNKNOW     0
#define DEVICE_TYPE_SERVERD    1
#define DEVICE_TYPE_MOUNT      2
#define DEVICE_TYPE_CCD        3
#define DEVICE_TYPE_DOME       4
#define DEVICE_TYPE_WEATHER    5
#define DEVICE_TYPE_ROTATOR    6
#define DEVICE_TYPE_PHOT       7
#define DEVICE_TYPE_PLAN       8
#define DEVICE_TYPE_GRB        9
#define DEVICE_TYPE_FOCUS     10
#define DEVICE_TYPE_MIRROR    11
#define DEVICE_TYPE_CUPOLA    12
#define DEVICE_TYPE_FW        13
#define DEVICE_TYPE_AUGERSH   14
#define DEVICE_TYPE_SENSOR    15

#define DEVICE_TYPE_EXECUTOR  20
#define DEVICE_TYPE_IMGPROC   21
#define DEVICE_TYPE_SELECTOR  22
#define DEVICE_TYPE_XMLRPC    23
#define DEVICE_TYPE_INDI      24
#define DEVICE_TYPE_LOGD      25
#define DEVICE_TYPE_SCRIPTOR  26
#define DEVICE_TYPE_BB        27

#endif	 /* __RTS2__STATUS__ */
