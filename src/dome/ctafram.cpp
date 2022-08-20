/*
 * Driver for CTA FRAM controll.
 * Copyright (C) 2008-2010 Petr Kubanek <petr@kubanek.net>
 * Copyright (C) 2021 Ronan Cunniffe <ronan@cunniffe.net>
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

#include <open62541/client_config_default.h>
#include <open62541/client_highlevel.h>
#include <open62541/client_subscriptions.h>
#include <open62541/plugin/log_stdout.h>

#include <stdlib.h>

#include "dome.h"

// *FIXME*
#define EVENT_DEADBUT     RTS2_LOCAL_EVENT   + 1350
#define EVENT_PINGMOUNT   RTS2_LOCAL_EVENT + 1352 // 1351 is used (*FIXME*)

#define DEFAULT_DEADMAN_TIMEOUT_S   10
#define DEFAULT_COOLDOWN_LOCKOUT_S  1200 // Heat dissipation

// *FIXME* new opts
#define OPT_BATTERY                OPT_LOCAL + 401
#define OPT_Q1_NAME                OPT_LOCAL + 402
#define OPT_Q2_NAME                OPT_LOCAL + 403
#define OPT_Q3_NAME                OPT_LOCAL + 404
#define OPT_Q4_NAME                OPT_LOCAL + 406
#define OPT_Q5_NAME                OPT_LOCAL + 407
#define OPT_NO_POWER               OPT_LOCAL + 408
#define OPT_DONT_RESTART_OPENING   OPT_LOCAL + 409
#define OPT_COOLDOWN               OPT_LOCAL + 410


// Read-write parameters (ns=4)
#define DEADMAN_TIMEOUT                "ns=4;i=2"
#define TEMP_LIMIT                     "ns=4;i=3"
#define DEADMAN_BIT                    "ns=4;i=4"
#define Q1_BIT                         "ns=4;i=5"
#define Q2_BIT                         "ns=4;i=6"
#define Q3_BIT                         "ns=4;i=7"
#define Q4_BIT                         "ns=4;i=8"
#define Q5_BIT                         "ns=4;i=9"
#define SITE_LOCKDOWN                  "ns=4;i=10"
#define WEATHER_BRIDGE                 "ns=4;i=11"
#define STAT_STOP_ON                   "ns=4;i=12"
#define STAT_STOP_OFF                  "ns=4;i=13"
#define ROOF_TIMEOUT_LEFT              "ns=4;i=14"
#define ROOF_TIMEOUT_RIGHT             "ns=4;i=15"
#define ACCUM_TIMEOUT_VALUE            "ns=4;i=16"
#define CLOSE_ROOF_TIMEOUT_BRIDGE      "ns=4;i=17"
#define MOUNT_POWER_TOGGLE             "ns=4;i=19"
#define ERROR_RESET                    "ns=4;i=22"

/* These exist, but are not used by this driver
#define SECOND_SIREN_TONE              "ns=4;i=18"
#define REMOTE_REQUEST_LEFT_OPEN       "ns=4;i=20"
#define REMOTE_REQUEST_LEFT_CLOSE      "ns=4;i=21"
#define REMOTE_REQUEST_RIGHT_OPEN      "ns=4;i=23"
#define REMOTE_REQUEST_RIGHT_CLOSE     "ns=4;i=24"
#define SCARY_REMOTE                   "ns=4;i=25"// master enable of the 4 raw cmds.
*/

// Read-only parameters ("ns=5)"
#define OIL_LEVEL                      "ns=5;i=2"
#define CABINET_TEMP                   "ns=5;i=3"
#define CABINET_HEATER                 "ns=5;i=4"
#define CABINET_TEMP_LIMIT             "ns=5;i=5"
#define OIL_LEVEL_OUTOFLIMIT           "ns=5;i=6"
#define ACCUM_LOW_PRESSURE             "ns=5;i=7"
#define MOTOR_ON                       "ns=5;i=11"
#define SENS_OPEN_R                    "ns=5;i=12"
#define SENS_CLOSED_R                  "ns=5;i=13"
#define ROOF_RTIMEDOUT                 "ns=5;i=14"
#define ROOF_OPENING_R                 "ns=5;i=15"
#define ROOF_CLOSING_R                 "ns=5;i=16"
#define SENS_OPEN_L                    "ns=5;i=17"
#define SENS_CLOSED_L                  "ns=5;i=18"
#define ROOF_LTIMEDOUT                 "ns=5;i=19"
#define ROOF_OPENING_L                 "ns=5;i=20"
#define ROOF_CLOSING_L                 "ns=5;i=21"
#define ACCUM_TIMEDOUT                 "ns=5;i=22"
#define STOP_STATUS                    "ns=5;i=24"
#define REMOTE                         "ns=5;i=25"
#define INTERLOCK                      "ns=5;i=27"
#define LOCK_STATUS                    "ns=5;i=28"
#define E_STOP                         "ns=5;i=29"
#define WEATHER_OK                     "ns=5;i=31"
#define MAINS_OK                       "ns=5;i=32"
#define BATTERY_LOW                    "ns=5;i=34"
#define BATTERY_FAULT                  "ns=5;i=35"
#define SCARY_REMOTE                   "ns=5;i=37"

/* These exist, but are not used by this driver
#define TIMEOUT_L                      "ns=5;i=8"
#define TIMEOUT_R                      "ns=5;i=9"
#define TIMEOUT_ACCUM_VALUE            "ns=5;i=10"
#define BTN_DEADMAN                    "ns=5;i=23"
#define SWITCH_O/C                     "ns=5;i=26"
#define TIMEOUT_ACTIVE                 "ns=5;i=30"
#define BATTERY_ON                     "ns=5;i=33"
#define EXTHERNAL_BUTTON               "ns=5;i=36"
*/

#define DOOR_OPEN     0x0001
#define DOOR_CLOSED   0x0002
#define DOOR_OPENING  0x0004
#define DOOR_CLOSING  0x0008
#define DOOR_TIMEOUT  0x0010


namespace rts2dome
{


/**
 * Driver for CTA FRAM dome.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 * @author Ronan Cunniffe <ronan@cunniffe.net>
 * @author Sergey Karpov <karpov.sv@gmail.com>
 */

class CTA_Fram:public Dome
{
    public:
        CTA_Fram (int argc, char **argv);
        virtual ~CTA_Fram (void);

        virtual int info ();

        virtual void postEvent (rts2core::Event * event);

		virtual int commandAuthorized (rts2core::Connection *conn);

    protected:
        virtual int processOption (int in_opt);

        virtual int initHardware ();

        virtual int setValue (rts2core::Value *oldValue, rts2core::Value *newValue);

        virtual bool isGoodWeather ();

        virtual int startOpen ();
        virtual long isOpened ();
        virtual int endOpen ();

        virtual int startClose ();
        virtual long isClosed ();
        virtual int endClose ();

    private:
		std::string host;
		UA_Client *client;

        bool deadman_flag;

        // if true, error during closing was already reported
        bool closeErrorReported;

        char left_state_bits, right_state_bits;// limit switch bitfields

		int opcua_connect ();
        int opcua_set_byte(const char *ref, int value);
        int opcua_get_int(const char *ref, int &value);
        int opcua_get_bool(const char *ref, bool &value);
        int opcua_get_double(const char *ref, double &value);
        int opcua_set_bool(const char *ref, bool value);
        int opcua_set_float(const char *ref, float value);
        void updateRoofState ();
        int updateRoofHalf(rts2core::Value *door, char &old_state, char new_state,
                           const char *opening_node, const char *closing_node, const char *timeout_node);

        // writable, RTS2 internal
        rts2core::ValueInteger *cooldownSecs;

        // Read-write parameters (ns=4)
        // writable, PLC
        rts2core::ValueInteger *deadman_timeout;
        rts2core::ValueInteger *accum_timeout_value;
        rts2core::ValueInteger *roof_timeout_L;
        rts2core::ValueInteger *roof_timeout_R;
        rts2core::ValueBool *close_ign_timeout;
        rts2core::ValueInteger *cabinet_temp_limit;
        rts2core::ValueBool *ignore_bad_weather;
        //rts2core::ValueBool *software_estop; // *FIXME* TODO
        rts2core::ValueBool *mount_power_toggle;
        rts2core::ValueBool *reset_error;
        rts2core::ValueBool *Q1;
        rts2core::ValueBool *Q2;
        rts2core::ValueBool *Q3;
        rts2core::ValueBool *Q4;
        rts2core::ValueBool *Q5;

        const char *Q1_name;
        const char *Q2_name;
        const char *Q3_name;
        const char *Q4_name;
        const char *Q5_name;

        // read-only, PLC
        rts2core::ValueFloat *cabinet_temp; // *FIXME* TODO
        // rts2core::ValueBool *heater_setpoint; // *FIXME* TODO;
        rts2core::ValueFloat *oil_level; // *FIXME* TODO;
        rts2core::ValueBool *oil_level_ok;
        rts2core::ValueBool *accum_ok;
        rts2core::ValueBool *timeout_on_accum;
        rts2core::ValueBool *mains_ok;
        rts2core::ValueBool *battery_fault;
        rts2core::ValueBool *battery_low;

		/* Overview */
        rts2core::ValueBool *motor_on;
        rts2core::ValueString *right_roof;
        rts2core::ValueString *left_roof;
        rts2core::ValueString *limit_switches;

		/* End switches */
		rts2core::ValueBool *swOpenLeft;
		rts2core::ValueBool *swCloseLeft;
		rts2core::ValueBool *swCloseRight;
		rts2core::ValueBool *swOpenRight;

		/* Motors */
		rts2core::ValueBool *motOpenLeft;
		rts2core::ValueBool *motCloseLeft;
		rts2core::ValueBool *motOpenRight;
		rts2core::ValueBool *motCloseRight;

		/* Timeouts */
		rts2core::ValueBool *timeoLeft;
		rts2core::ValueBool *timeoRight;

        rts2core::ValueBool *cabinet_heater;
        rts2core::ValueBool *weather_ok;
        rts2core::ValueBool *stopped;
        rts2core::ValueString *stopped_by;
        rts2core::ValueBool *remote_mode;// false = local/manual
        rts2core::ValueBool *raw_remote;// scary mode (*no* safety - doors can collide)
        rts2core::ValueBool *site_lockdown;
};

}

using namespace rts2dome;

/* OPC-UA interface */

int CTA_Fram::opcua_get_bool (const char *ref, bool &value)
{
	opcua_connect ();

    UA_Variant *val = UA_Variant_new ();
    UA_StatusCode retval = UA_Client_readValueAttribute (client, UA_NODEID (ref), val);
	int ret = 0;

    if(retval == UA_STATUSCODE_GOOD && UA_Variant_isScalar (val))
	{
        if (val->type == &UA_TYPES[UA_TYPES_BOOLEAN])
            value = *(UA_Boolean *)val->data;
    }
	else
	{
		logStream (MESSAGE_ERROR) << "failed to get bool " << ref << " : " << UA_StatusCode_name (retval) << sendLog;
		ret = -1;
	}

    return ret;
}

int CTA_Fram::opcua_get_int (const char *ref, int &value)
{
	opcua_connect ();

    UA_Variant *val = UA_Variant_new ();
    UA_StatusCode retval = UA_Client_readValueAttribute (client, UA_NODEID (ref), val);
	int ret = 0;

    if(retval == UA_STATUSCODE_GOOD && UA_Variant_isScalar (val))
	{
        if (val->type == &UA_TYPES[UA_TYPES_BOOLEAN])
            value = *(UA_Boolean *)val->data;
        else if (val->type == &UA_TYPES[UA_TYPES_BYTE])
            value = *(UA_Byte *)val->data;
        else if (val->type == &UA_TYPES[UA_TYPES_INT16])
            value = *(UA_Int16 *)val->data;
        else if (val->type == &UA_TYPES[UA_TYPES_UINT16])
            value = *(UA_UInt16 *)val->data;
        else if (val->type == &UA_TYPES[UA_TYPES_INT32])
            value = *(UA_Int32 *)val->data;
        else if (val->type == &UA_TYPES[UA_TYPES_UINT32])
            value = *(UA_UInt32 *)val->data;
        else if (val->type == &UA_TYPES[UA_TYPES_INT64])
            value = *(UA_Int64 *)val->data;
        else if (val->type == &UA_TYPES[UA_TYPES_UINT64])
            value = *(UA_UInt64 *)val->data;
    }
	else
	{
		logStream (MESSAGE_ERROR) << "failed to get int " << ref << " : " << UA_StatusCode_name (retval) << sendLog;
		ret = -1;
	}

    return ret;
}

int CTA_Fram::opcua_get_double (const char *ref, double &value)
{
	opcua_connect ();

    UA_Variant *val = UA_Variant_new ();
    UA_StatusCode retval = UA_Client_readValueAttribute (client, UA_NODEID (ref), val);
	int ret = 0;

    if(retval == UA_STATUSCODE_GOOD && UA_Variant_isScalar (val))
	{
        if (val->type == &UA_TYPES[UA_TYPES_FLOAT])
            value = *(UA_Float *)val->data;
        else if (val->type == &UA_TYPES[UA_TYPES_DOUBLE])
            value = *(UA_Double *)val->data;
    }
	else
	{
		logStream (MESSAGE_ERROR) << "failed to get double " << ref << " : " << UA_StatusCode_name (retval) << sendLog;
		ret = -1;
	}

    return ret;
}

int CTA_Fram::opcua_set_byte (const char *ref, int value_in)
{
	opcua_connect ();

    UA_Byte value = value_in;
    UA_Variant *val = UA_Variant_new ();
	int ret = 0;

    UA_Variant_setScalarCopy (val, &value, &UA_TYPES[UA_TYPES_BYTE]);

    UA_StatusCode retval = UA_Client_writeValueAttribute (client, UA_NODEID (ref), val);

    if(retval != UA_STATUSCODE_GOOD) {
		logStream (MESSAGE_ERROR) << "failed to set byte " << ref << " : " << UA_StatusCode_name (retval) << sendLog;
		ret = -1;
    }

    return ret;
}

int CTA_Fram::opcua_set_bool (const char *ref, bool value_in)
{
	opcua_connect ();

    UA_Boolean value = value_in;
    UA_Variant *val = UA_Variant_new ();
	int ret = 0;

    UA_Variant_setScalarCopy(val, &value, &UA_TYPES[UA_TYPES_BOOLEAN]);

    UA_StatusCode retval = UA_Client_writeValueAttribute (client, UA_NODEID (ref), val);

    if(retval != UA_STATUSCODE_GOOD) {
		logStream (MESSAGE_ERROR) << "failed to set bool " << ref << " : " << UA_StatusCode_name (retval) << sendLog;
		ret = -1;
    }

    return ret;
}

int CTA_Fram::opcua_set_float (const char *ref, float value_in)
{
	opcua_connect ();

    UA_Float value = value_in;
    UA_Variant *val = UA_Variant_new ();
	int ret = 0;

    UA_Variant_setScalarCopy(val, &value, &UA_TYPES[UA_TYPES_FLOAT]);

    UA_StatusCode retval = UA_Client_writeValueAttribute (client, UA_NODEID (ref), val);

    if(retval != UA_STATUSCODE_GOOD) {
		logStream (MESSAGE_ERROR) << "failed to set float " << ref << " : " << UA_StatusCode_name (retval) << sendLog;
		ret = -1;
    }

    return ret;
}

int CTA_Fram::opcua_connect ()
{
	UA_StatusCode retval;
	UA_SessionState sstate;
	UA_SecureChannelState cstate;

	if (!client)
	{
		client = UA_Client_new ();

		UA_ClientConfig *config = UA_Client_getConfig(client);

		if (!getDebug ())
			/* Disable verbose logging to stdout, print only errors */
			config->logger = UA_Log_Stdout_withLevel(UA_LOGLEVEL_ERROR);

		UA_ClientConfig_setDefault(config);
	}

	UA_Client_getState(client, &cstate, &sstate, &retval);

	// printf("cstate = %d state = %d retval = %d\n", cstate, sstate, retval);

	if (cstate != UA_SECURECHANNELSTATE_OPEN || sstate != UA_SESSIONSTATE_ACTIVATED ||
		retval == UA_STATUSCODE_BADCONNECTIONCLOSED)
	{
		/* if already connected, this will return GOOD and do nothing, just logs a message */
		/* if the connection is closed/errored, the connection will be reset and then reconnected */
		retval = UA_Client_connect (client, host.c_str ());
	}

    if(retval != UA_STATUSCODE_GOOD) {
		logStream (MESSAGE_DEBUG) << "Error connecting to " << host << " : " << UA_StatusCode_name (retval) << sendLog;

		return -1;
    }

	return 0;
}

/* Really means "ok to open?" */
bool CTA_Fram::isGoodWeather ()
{
	if (mains_ok->getValueBool () == false)
	{
		valueError (mains_ok);
		setWeatherTimeout (60, "on battery");
		return false;
	} else {
		valueGood (mains_ok);
	}

	if (oil_level_ok->getValueBool () == false)
	{
		valueError (oil_level_ok);
		setWeatherTimeout (60, "oil level out of bounds");
		return false;
	} else {
		valueGood (oil_level_ok);
	}

	if (accum_ok->getValueBool() == false)
	{
		valueError (accum_ok);
		setWeatherTimeout (60, "accum pressure out of bounds");
		return false;
	} else {
		valueGood (accum_ok);
	}

	if (battery_fault->getValueBool ())
	{
		valueError (battery_fault);
		setWeatherTimeout (60, "failed battery");
		return false;
	} else {
		valueGood (battery_fault);
	}

	if (battery_low->getValueBool ())
	{
		valueError (battery_low);
		setWeatherTimeout (60, "low battery");
		return false;
	} else {
		valueGood (battery_low);
	}

	sendValueAll (accum_ok);
	sendValueAll (oil_level_ok);
	sendValueAll (mains_ok);
	sendValueAll (battery_fault);
	sendValueAll (battery_low);

	if (remote_mode->getValueBool () == false)
	{
		valueError (remote_mode);
		setWeatherTimeout (30, "in local mode");
		return false;
	} else {
		valueGood (remote_mode);
	}
	sendValueAll (remote_mode);

	return Dome::isGoodWeather ();
}

int CTA_Fram::startOpen ()
{
	closeErrorReported = false;

	// logStream (MESSAGE_DEBUG) << "starting to open." << sendLog;

	if (remote_mode->getValueBool () == false)
	{
		logStream(MESSAGE_WARNING) << "not opening: dome not in remote mode" << sendLog;
		return -1;
	}

	if (mains_ok->getValueBool () == false)
	{
		logStream(MESSAGE_WARNING) << "not opening: not on mains power" << sendLog;
		return -1;
	}

	if (battery_low->getValueBool() | battery_fault->getValueBool())
	{
		logStream(MESSAGE_WARNING) << "not opening: battery fault or low voltage" << sendLog;
		return -1;
	}

	if (timeout_on_accum->getValueBool())
	{
		logStream(MESSAGE_WARNING) << "not opening: accumulator fault (timeout during repress)" << sendLog;
		return -1;
	}

	if (oil_level_ok->getValueBool() == false)
	{
		logStream(MESSAGE_WARNING) << "not opening: oil level is out of limits" << sendLog;
		return -1;
	}

	if (stopped->getValueBool())
	{
		logStream(MESSAGE_WARNING) << "not opening: stopped condition is set" << sendLog;
		return -1;
	}
	if (site_lockdown->getValueBool())
	{
		logStream(MESSAGE_WARNING) << "not opening: site lockdown condition is set" << sendLog;
		return -1;
	}

	opcua_set_byte(DEADMAN_TIMEOUT, deadman_timeout->getValueInteger());

	opcua_get_bool(DEADMAN_BIT, deadman_flag);
	opcua_set_bool(DEADMAN_BIT, !deadman_flag);
	addTimer (deadman_timeout->getValueInteger () / 5.0, new rts2core::Event (EVENT_DEADBUT, this));
	return 0;
}


int CTA_Fram::startClose ()
{
	// we should have asked for an extra input in the dome logic, but we didn't....
	// Here we reduce the deadman timer to nearly zero, so the deadman event will be too late.
	// It detects that, and re-writes the timeout value to the official one. :-(

	// logStream (MESSAGE_INFO) << "Starting to close." << sendLog;

	// *FIXME* check we're actually open....
	opcua_set_byte (DEADMAN_TIMEOUT, 1);  // force the close.  When heartbeat timer expires, it will start closing

	setWeatherTimeout (cooldownSecs->getValueInteger (), "closed, timeout for opening (to allow dissipate motor heat)");

	return 0;
}


long CTA_Fram::isOpened ()
{
	// *FIXME* errorcheck
	updateRoofState();

	if (left_state_bits & right_state_bits & DOOR_OPEN)
	{
		return -2;
	}
	return 0;
}


long CTA_Fram::isClosed ()
{
	// *FIXME* errorcheck
	updateRoofState();

	if (left_state_bits & right_state_bits & DOOR_CLOSED)
	{
		return -2; // Magic code the base dome driver is looking for. :-|
	}
	return 0;
}


int CTA_Fram::endOpen ()
{
	return 0;
}


int CTA_Fram::endClose ()
{
	return 0;
}

int CTA_Fram::commandAuthorized (rts2core::Connection * conn)
{
	if (conn->isCommand ("toggle_mount"))
	{
		opcua_set_bool(MOUNT_POWER_TOGGLE, true);
		usleep (USEC_SEC / 2);
		opcua_set_bool(MOUNT_POWER_TOGGLE, false);

		logStream (MESSAGE_INFO) << "mount switch toggled on/off" << sendLog;

		return 0;
	}

	if (conn->isCommand ("reset_emergency"))
	{
		opcua_set_bool(ERROR_RESET, true);
		usleep (USEC_SEC / 2);
		opcua_set_bool(ERROR_RESET, false);

		logStream (MESSAGE_INFO) << "emergency reset switch toggled on/off" << sendLog;

		info ();

		return 0;
	}

	return Dome::commandAuthorized (conn);
}

int CTA_Fram::processOption (int in_opt)
{
	switch (in_opt)
	{
		case 'z':
			host = "opc.tcp://" + std::string(optarg) + ":4840";
			break;
		case OPT_Q1_NAME:
			Q1_name = optarg;
			break;
		case OPT_Q2_NAME:
			Q2_name = optarg;
			break;
		case OPT_Q3_NAME:
			Q3_name = optarg;
			break;
		case OPT_Q4_NAME:
			Q4_name = optarg;
			break;
		case OPT_Q5_NAME:
			Q5_name = optarg;
			break;
		case OPT_COOLDOWN: // *FIXME* need to make sure this is actually written the PLC
			cooldownSecs->setValueInteger (atoi (optarg));
			sendValueAll(cooldownSecs);
			break;
		default:
			return Dome::processOption (in_opt);
	}
	return 0;
}

void CTA_Fram::postEvent (rts2core::Event *event)
{
	switch (event->getType ())
	{
		case EVENT_DEADBUT:
			if (isGoodWeather () && ((getState () & DOME_DOME_MASK) == DOME_OPENED || (getState () & DOME_DOME_MASK) == DOME_OPENING))
			{
				opcua_set_bool(DEADMAN_BIT, deadman_flag);
				deadman_flag = !deadman_flag;
				addTimer (deadman_timeout->getValueInteger () / 5.0, event);
				return;
			} else {
				// To shut the dome promptly, startClose drops the PLC's timeout to 1s. Once the
				// dome starts to close, the heartbeat is killed by the OPEN | OPENING test above.
				// Once that fails, we can re-program the deadman timeout.  Yes, this is bad.
				opcua_set_byte(DEADMAN_TIMEOUT, deadman_timeout->getValueInteger());
			}
			break;
		case EVENT_PINGMOUNT:
			break;
	}
	Dome::postEvent (event);
}


CTA_Fram::CTA_Fram (int argc, char **argv):Dome (argc, argv)
{
	addOption ('z', NULL, 1, "PLC TCP/IP address");

	createValue(deadman_timeout, "deadman_timeout", "[s] max time before PLC assumes the PC is dead.", false, RTS2_VALUE_WRITABLE);
	deadman_timeout->setValueInteger(DEFAULT_DEADMAN_TIMEOUT_S);

	addOption (OPT_COOLDOWN, "cooldown", 1, "min. interval [s] between closing and opening the dome.");
	createValue (cooldownSecs, "cooldown_interval", "[s] interval between openings", false);
	cooldownSecs->setValueInteger(DEFAULT_COOLDOWN_LOCKOUT_S);
	sendValueAll(cooldownSecs);



	Q1 = NULL;
	Q2 = NULL;
	Q3 = NULL;
	Q4 = NULL;
	Q5 = NULL;

	Q1_name = "Q1_switch";
	Q2_name = "Q2_switch";
	Q3_name = "Q3_switch";
	Q4_name = "Q4_switch";
	Q5_name = "Q5_switch";

	addOption (OPT_Q1_NAME, "Q1-name", 1, "name of the Q1 switch");
	addOption (OPT_Q2_NAME, "Q2-name", 1, "name of the Q2 switch");
	addOption (OPT_Q3_NAME, "Q3-name", 1, "name of the Q3 switch");
	addOption (OPT_Q4_NAME, "Q4-name", 1, "name of the Q4 switch");
	addOption (OPT_Q5_NAME, "Q5-name", 1, "name of the Q5 switch");

	left_state_bits = right_state_bits = 0;

	client = NULL;
}


CTA_Fram::~CTA_Fram (void)
{
    UA_Client_disconnect (client);
    UA_Client_delete (client);
}


int CTA_Fram::info ()
{
	int retval = 0;

	updateRoofState ();
	if (retval < 0)
	{
		return retval;
	}

	// booleans first
	bool b;

	if (opcua_get_bool (Q1_BIT, b) == 0)
	{
		Q1->setValueBool (b);
		sendValueAll (Q1);
	}

	if (opcua_get_bool (Q2_BIT, b) == 0)
	{
		Q2->setValueBool (b);
		sendValueAll (Q2);
	}

	if (opcua_get_bool (Q3_BIT, b) == 0)
	{
		Q3->setValueBool (b);
		sendValueAll (Q3);
	}

	if (opcua_get_bool (Q4_BIT, b) == 0)
	{
		Q4->setValueBool (b);
		sendValueAll (Q4);
	}

	if (opcua_get_bool (Q5_BIT, b) == 0)
	{
		Q5->setValueBool (b);
		sendValueAll (Q5);
	}

	if (opcua_get_bool (CLOSE_ROOF_TIMEOUT_BRIDGE, b) == 0)
	{
		close_ign_timeout->setValueBool (b);
		sendValueAll (close_ign_timeout);
	}

	if (opcua_get_bool (WEATHER_BRIDGE, b) == 0)
	{
		ignore_bad_weather->setValueBool (b);
		sendValueAll (ignore_bad_weather);
	}

	opcua_get_bool(SITE_LOCKDOWN, b);
	site_lockdown->setValueBool(b);
	sendValueAll(site_lockdown);

	if (opcua_get_bool (ERROR_RESET, b) == 0)
	{
		reset_error->setValueBool (b);
		sendValueAll (reset_error);
	}

	if (opcua_get_bool (MOUNT_POWER_TOGGLE, b) == 0)
	{
		mount_power_toggle->setValueBool (b);
		sendValueAll (mount_power_toggle);
	}

	if (opcua_get_bool (OIL_LEVEL_OUTOFLIMIT, b) == 0)
	{
		oil_level_ok->setValueBool (!b); // NOTE: inverted logic
		sendValueAll (oil_level_ok);
	}

	if (opcua_get_bool (ACCUM_LOW_PRESSURE, b) == 0) // sensor will also detect overpressure
	{
		accum_ok->setValueBool (!b); // NOTE: inverted logic
		sendValueAll (accum_ok);
	}

	if (opcua_get_bool (ACCUM_TIMEDOUT, b) == 0)
	{
		timeout_on_accum->setValueBool (b);
		sendValueAll (timeout_on_accum);
	}

	if (opcua_get_bool (MAINS_OK, b) == 0)
	{
		mains_ok->setValueBool (b);
		sendValueAll (mains_ok);
	}

	if (opcua_get_bool (BATTERY_FAULT, b) == 0)
	{
		battery_fault->setValueBool (b);
		sendValueAll (battery_fault);
	}

	if (opcua_get_bool (BATTERY_LOW, b) == 0)
	{
		battery_low->setValueBool (b);
		sendValueAll (battery_low);
	}

	if (opcua_get_bool (WEATHER_OK, b) == 0)
	{
		weather_ok->setValueBool (b);
		sendValueAll (weather_ok);
	}

	if (opcua_get_bool (STOP_STATUS, b) == 0)
	{
		stopped->setValueBool (b);
		sendValueAll (stopped);
	}

	if (opcua_get_bool (SCARY_REMOTE, b) == 0)
	{
		raw_remote->setValueBool (b);
		sendValueAll (raw_remote);
	}

	if (opcua_get_bool (REMOTE, b) == 0)
	{
		remote_mode->setValueBool (b);
		sendValueAll(remote_mode);
	}

	if (opcua_get_bool (CABINET_HEATER, b) == 0)
	{
		cabinet_heater->setValueBool (b);
		sendValueAll (cabinet_heater);
	}

	// *FIXME* TODO
	// rts2core::ValueString *stopped_by;

	// bytes
	int i;

	if (opcua_get_int (ROOF_TIMEOUT_LEFT, i) == 0)
	{
		roof_timeout_L->setValueInteger (i);
		sendValueAll (roof_timeout_L);
	}

	if (opcua_get_int (ROOF_TIMEOUT_RIGHT, i) == 0)
	{
		roof_timeout_R->setValueInteger (i);
		sendValueAll (roof_timeout_R);
	}

	if (opcua_get_int (DEADMAN_TIMEOUT, i) == 0)
	{
		deadman_timeout->setValueInteger (i);
		sendValueAll (deadman_timeout);
	}

	if (opcua_get_int (ACCUM_TIMEOUT_VALUE, i) == 0)
	{
		accum_timeout_value->setValueInteger (i);
		sendValueAll (accum_timeout_value);
	}

	// floats
	double d;

	if (opcua_get_double (CABINET_TEMP, d) == 0)
	{
		cabinet_temp->setValueFloat (d);
		sendValueAll (cabinet_temp);
	}

	if (opcua_get_double (OIL_LEVEL, d) == 0)
	{
		oil_level->setValueFloat (d);
		sendValueAll (oil_level);
	}


	return Dome::info ();
}

int CTA_Fram::initHardware ()
{
	if (host.empty ())
	{
		logStream (MESSAGE_ERROR) << "You must specify PLC address/hostname (with -z option)." << sendLog;
		return -1;
	}

	// Initialize OPCUA connection
	logStream (MESSAGE_DEBUG) << "Connecting to " << host << sendLog;

	if (opcua_connect () != 0)
        return -1;

	setIdleInfoInterval (10);

	createValue(remote_mode, "remote_mode", "dome is under software control (not manual)", false);

	createValue(site_lockdown, "site_lockdown", "Site Lockdown", false); // TODO: better description

	// Timeouts
	createValue(accum_timeout_value,"accum_timeout_value", "[s] time limit for repress. of accum.", false, RTS2_VALUE_WRITABLE);
	createValue(roof_timeout_L, "dome_timeout_L", "[s] max allowed time for L half to move.", false, RTS2_VALUE_WRITABLE);
	createValue(roof_timeout_R, "dome_timeout_R", "[s] max allowed time for L half to move.", false, RTS2_VALUE_WRITABLE);
	createValue(close_ign_timeout, "close_ign_timeout", "If set, no timeout applies to closing the dome.", false, RTS2_VALUE_WRITABLE);

	//createValue(cooldown_lockout, "cooldown_lockout", "[s] interval after closing before dome will re-open.", false, RTS2_VALUE_WRITABLE);
	//sendValueAll(cooldown_lockout);

	createValue(weather_ok, "weather_is_ok", "True if weather is believed to be good.", false);
	createValue(ignore_bad_weather, "ignore_bad_weather", "Open even if weather_is_ok is false", false, RTS2_VALUE_WRITABLE);
	createValue(raw_remote, "raw_remote", "Engineering mode - should not be used!", false);

	createValue(limit_switches, "limit_switches", "'*' where door is detected (LO/LC/RC/RO)", false);
	createValue(right_roof, "right_roof", "right roof status", false);
	createValue(left_roof, "left_roof", "left roof status", false);
	createValue(stopped, "stopped", "if dome is in a stopped state.", false);
	createValue(stopped_by, "stopped_by", "The causes of dome stoppage, if any", false);
	createValue(motor_on, "motor_is_on", "self-explanatory, really...", false);

	createValue (swOpenLeft, "sw_open_left", "state of left open switch", false);
	createValue (swCloseLeft, "sw_close_left", "state of left close switch", false);
	createValue (swCloseRight, "sw_close_right", "state of right close switch", false);
	createValue (swOpenRight, "sw_open_right", "state of right open switch", false);

	createValue (motOpenLeft, "motor_open_left", "state of left opening motor", false);
	createValue (motCloseLeft, "motor_close_left", "state of left closing motor", false);
	createValue (motOpenRight, "motor_open_right", "state of right opening motor", false);
	createValue (motCloseRight, "motor_close_right", "state of right closing motor", false);

	createValue (timeoLeft, "timeo_left", "left timeout", false);
	createValue (timeoRight, "timeo_right", "right timeout", false);

	createValue(reset_error, "reset_error", "Reset error flag.", false, RTS2_VALUE_WRITABLE);

	// *FIXME* TODO
	//createValue(software_estop, "EMERGENCY_STOP", "Stop dome movement immediately.", false, RTS2_VALUE_WRITABLE);
	createValue(mount_power_toggle, "mount_power_toggle", "On -> Off, or Off -> On", false, RTS2_VALUE_WRITABLE);
	createValue(Q1, Q1_name, "Q1 output", false, RTS2_VALUE_WRITABLE);
	createValue(Q2, Q2_name, "Q2 output", false, RTS2_VALUE_WRITABLE);
	createValue(Q3, Q3_name, "Q3 output", false, RTS2_VALUE_WRITABLE);
	createValue(Q4, Q4_name, "Q4 output", false, RTS2_VALUE_WRITABLE);
	createValue(Q5, Q5_name, "Q5 output", false, RTS2_VALUE_WRITABLE);

	createValue(oil_level, "oil_level", "oil_level in cm", false); // *FIXME* TODO
	createValue(oil_level_ok, "oil_level_ok", "oil level is in range", false);
	createValue(accum_ok, "accum_ok", "accumulator pressure is in range", false);
	createValue(timeout_on_accum, "timeout_accum", "Accumulator did not reach pressure before timeout.", false);
	createValue(mains_ok, "mains_ok", "mains power is ok", false);
	createValue(battery_fault, "battery_fault", "cabinet UPS is indicating a battery fault", false);
	createValue(battery_low, "battery_low", "cabinet UPS is indicating low battery voltage", false);
	createValue(cabinet_temp, "cabinet_temp", "[C] Cabinet temperature.", false);
	createValue(cabinet_temp_limit, "heater_setpoint", "[C] Cabinet heater ON setpoint", false, RTS2_VALUE_WRITABLE);
	createValue(cabinet_heater, "cabinet_heater_on", "50W heater is running", false);

	right_state_bits = left_state_bits = 0;
	int ret = info ();
	if (ret)
		return ret;

	/* Guess initial state */
	if (swOpenLeft->getValueBool () == true && swOpenRight->getValueBool () == true)
	{
		maskState (DOME_DOME_MASK, DOME_OPENED, "initial dome state is opened");
	}
	else if (swCloseLeft->getValueBool () == true && swCloseRight->getValueBool () == true)
	{
		maskState (DOME_DOME_MASK, DOME_CLOSED, "initial dome state is closed");
	}
	else if (motOpenLeft->getValueBool () == true || motOpenRight->getValueBool () == true)
	{
		maskState (DOME_DOME_MASK, DOME_OPENING, "initial dome state is opening");
	}
	else if (motCloseLeft->getValueBool () == true || motCloseRight->getValueBool () == true)
	{
		maskState (DOME_DOME_MASK, DOME_CLOSING, "initial dome state is closing");
	}

	addTimer (1, new rts2core::Event (EVENT_DEADBUT, this));
	return 0;
}


int CTA_Fram::setValue (rts2core::Value *oldValue, rts2core::Value *newValue)
{
	// Booleans
	if (oldValue == Q1)
		return opcua_set_bool(Q1_BIT, ((rts2core::ValueBool *) newValue)->getValueBool());
	if (oldValue == Q2)
		return opcua_set_bool(Q2_BIT, ((rts2core::ValueBool *) newValue)->getValueBool());
	if (oldValue == Q3)
		return opcua_set_bool(Q3_BIT, ((rts2core::ValueBool *) newValue)->getValueBool());
	if (oldValue == Q4)
		return opcua_set_bool(Q4_BIT, ((rts2core::ValueBool *) newValue)->getValueBool());
	if (oldValue == Q5)
		return opcua_set_bool(Q5_BIT, ((rts2core::ValueBool *) newValue)->getValueBool());
	if (oldValue == reset_error)
		return opcua_set_bool(ERROR_RESET, ((rts2core::ValueBool *) newValue)->getValueBool());
	if (oldValue == mount_power_toggle)
		return opcua_set_bool(MOUNT_POWER_TOGGLE, ((rts2core::ValueBool *) newValue)->getValueBool());
	if (oldValue == ignore_bad_weather)
		return opcua_set_bool(WEATHER_BRIDGE, ((rts2core::ValueBool *) newValue)->getValueBool());
	// *FIXME* TODO
	//if (oldValue == software_estop)
	//return opcua_set_bool(, ((rts2core::ValueBool *) newValue)->getValueBool());
	if (oldValue == close_ign_timeout)
		return opcua_set_bool(CLOSE_ROOF_TIMEOUT_BRIDGE, ((rts2core::ValueBool *) newValue)->getValueBool());
	// Bytes
	if (oldValue == deadman_timeout)
		return opcua_set_byte(DEADMAN_TIMEOUT, ((rts2core::ValueInteger *) newValue)->getValueInteger());
	if (oldValue == cabinet_temp_limit)
		return opcua_set_byte(TEMP_LIMIT, ((rts2core::ValueInteger *) newValue)->getValueInteger());
	if (oldValue == accum_timeout_value)
		return opcua_set_byte(ACCUM_TIMEOUT_VALUE, ((rts2core::ValueInteger *) newValue)->getValueInteger());
	if (oldValue == roof_timeout_L)
		return opcua_set_byte(ROOF_TIMEOUT_LEFT, ((rts2core::ValueInteger *) newValue)->getValueInteger());
	if (oldValue == roof_timeout_R)
		return opcua_set_byte(ROOF_TIMEOUT_RIGHT, ((rts2core::ValueInteger *) newValue)->getValueInteger());

	return Dome::setValue (oldValue, newValue);
}


int CTA_Fram::updateRoofHalf(rts2core::Value *door, char &old_state, char new_state,
							 const char *opening_node, const char *closing_node, const char *timedout_node)
{
	bool timedout, opening, closing;

	// *FIXME* errorcheck
	opcua_get_bool(opening_node, opening);
	opcua_get_bool(closing_node, closing);
	opcua_get_bool(timedout_node, timedout);

	new_state += ((opening)? 4:0) + ((closing)? 8:0) + ((timedout)? 16:0);

	char multistr[41];
	switch (new_state)
	{
		case 1: door->setValueString("open"); break;
		case 2: door->setValueString("closed"); break;
		case 4: door->setValueString("opening"); break;
		case 8: door->setValueString("closing"); break;
		case 16: door->setValueString("timedout"); break;
		default:
			snprintf(multistr, 40, "multiple (");
			if (new_state & 0x0001)
				strncat(multistr, " open", 40);
			if (opening)
				strncat(multistr, " opening", 40);
			if (closing)
				strncat(multistr, " closing", 40);
			if (new_state & 0x0002)
				strncat(multistr, " closed", 40);
			if (timedout)
				strncat(multistr, " timedout", 40);
			strncat(multistr, " )", 40);
			door->setValueString(multistr);
	}
	if (timedout)
	{
		maskState (DEVICE_ERROR_MASK, DEVICE_ERROR_HW, "timeout");
		valueError(door);
		if (!(old_state & 0x0010))
		{
			logStream (MESSAGE_ERROR | MESSAGE_CRITICAL) << "Dome timeout." << sendLog;
		}
	} else {
		valueGood(door);
		maskState(DEVICE_ERROR_MASK, DEVICE_NO_ERROR, "timeout cleared");
	}

	return 0;
}

void CTA_Fram::updateRoofState ()
{
	bool b;
	opcua_get_bool(MOTOR_ON, b);
	motor_on->setValueBool(b);
	sendValueAll(motor_on);

	/* End switches */
	char limits[5] = "????";
	bool lopen, lclosed, ropen, rclosed;

	opcua_get_bool(SENS_OPEN_L, lopen);
	opcua_get_bool(SENS_CLOSED_L, lclosed);
	opcua_get_bool(SENS_OPEN_R, ropen);
	opcua_get_bool(SENS_CLOSED_R, rclosed);

	swOpenLeft->setValueBool (lopen);
	swCloseLeft->setValueBool (lclosed);

	sendValueAll (swOpenLeft);
	sendValueAll (swCloseLeft);

	swOpenRight->setValueBool (ropen);
	swCloseRight->setValueBool (rclosed);

	sendValueAll (swOpenRight);
	sendValueAll (swCloseRight);

	/* Motors */
	bool lopening, lclosing, ropening, rclosing;

	opcua_get_bool(ROOF_OPENING_L, lopening);
	opcua_get_bool(ROOF_CLOSING_L, lclosing);
	opcua_get_bool(ROOF_OPENING_R, ropening);
	opcua_get_bool(ROOF_CLOSING_R, rclosing);

	motOpenLeft->setValueBool (lopening);
	motCloseLeft->setValueBool (lclosing);

	sendValueAll (motOpenLeft);
	sendValueAll (motCloseLeft);

	motOpenRight->setValueBool (ropening);
	motCloseRight->setValueBool (rclosing);

	sendValueAll (motOpenRight);
	sendValueAll (motCloseRight);

	/* Timeouts */
	bool ltimeo, rtimeo;

	opcua_get_bool(ROOF_RTIMEDOUT, ltimeo);
	opcua_get_bool(ROOF_LTIMEDOUT, rtimeo);

	timeoLeft->setValueBool (ltimeo);
	timeoRight->setValueBool (rtimeo);

	sendValueAll (timeoLeft);
	sendValueAll (timeoRight);

	limits[0] = (lopen) ? '*' : '_';
	limits[1] = (lclosed) ? '*' : '_';
	limits[2] = (rclosed) ? '*' : '_';
	limits[3] = (ropen) ? '*' : '_';
	limits[4] = 0;

	limit_switches->setValueString(limits);
	sendValueAll(limit_switches);

	// Set the two bits we already know, the rest will be updated in the helper function
	char new_state = ((lopen)? 1:0) + ((lclosed)? 2:0);
	updateRoofHalf(left_roof, left_state_bits, new_state, ROOF_OPENING_L, ROOF_CLOSING_L, ROOF_LTIMEDOUT);
	if (new_state != left_state_bits)
	{
		logStream (MESSAGE_INFO) << "LH state changed to " << left_roof->getValueString() << sendLog;
	}
	left_state_bits = new_state;


	new_state = ((ropen)? 1:0) + ((rclosed)? 2:0);
	updateRoofHalf(right_roof, right_state_bits, new_state, ROOF_OPENING_R, ROOF_CLOSING_R, ROOF_RTIMEDOUT);

	if (new_state != right_state_bits)
	{
		logStream (MESSAGE_INFO) << "RH state changed to " << right_roof->getValueString() << sendLog;
	}
	right_state_bits = new_state;
}

int main (int argc, char **argv)
{
	CTA_Fram device (argc, argv);
	return device.run ();
}
