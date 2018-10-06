/*
 * Centrald - RTS2 coordinator
 * Copyright (C) 2003-2013 Petr Kubanek <petr@kubanek.net>
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

#include "centrald.h"
#include "libnova_cpp.h"
#include "command.h"
#include "timestamp.h"
#include "centralstate.h"

using namespace rts2centrald;

void ConnCentrald::setState (rts2_status_t in_value, char *msg)
{
	rts2core::Connection::setState (in_value, msg);
	// distribute weather updates..
	if (serverState->maskValueChanged (WEATHER_MASK))
		master->weatherChanged (getName (), msg);
	if (serverState->maskValueChanged (STOP_MASK))
	  	master->stopChanged (getName (), msg);
	if (serverState->maskValueChanged (BOP_MASK))
		master->bopMaskChanged ();
	if (serverState->maskValueChanged (DEVICE_BLOCK_OPEN) || serverState->maskValueChanged (DEVICE_BLOCK_CLOSE))
		master->openCloseChanged (getName (), msg);
}

ConnCentrald::ConnCentrald (int in_sock, Centrald * in_master, int in_centrald_id):rts2core::Connection (in_sock, in_master)
{
	master = in_master;
	setCentraldId (in_centrald_id);
	messageMask = 0x00;

	statusCommandRunning = 0;
}

ConnCentrald::~ConnCentrald (void)
{
}

int ConnCentrald::sendDeviceKey ()
{
	int dev_key;
	dev_key = random ();
	if (dev_key == 0)
		dev_key = 1;
	std::ostringstream _os;
	// device number could change..device names don't
	char *dev_name;
	rts2core::Connection *dev;
	if (paramNextString (&dev_name) || !paramEnd ())
		return -2;
	// find device, set it authorization key
	dev = master->findName (dev_name);
	if (!dev)
	{
		sendCommandEnd (DEVDEM_E_SYSTEM, "cannot find device with name");
		return -1;
	}
	setKey (dev_key);
	_os << "authorization_key " << dev_name << " " << getKey ();
	sendMsg (_os);
	return 0;
}

int ConnCentrald::sendMessage (Message & msg)
{
	if (msg.passMask (messageMask))
		return rts2core::Connection::sendMessage (msg);
	return -1;
}

int ConnCentrald::sendInfo ()
{
	if (!paramEnd ())
		return -2;

	connections_t::iterator iter;
	for (iter = master->getConnections ()->begin (); iter != master->getConnections ()->end (); iter++)
	{
		ConnCentrald *conn = (ConnCentrald *) * iter;
		conn->sendConnectedInfo (this);
	}
	((Centrald *)getMaster ())->sendInfo (this);
	return 0;
}

int ConnCentrald::sendConnectedInfo (rts2core::Connection * conn)
{
	std::ostringstream _os;
	int ret = -1;

	switch (getType ())
	{
		case CLIENT_SERVER:
			_os << "client "
				<< getCentraldId () << " "
				<< login << " "
				<< getName ();
			ret = conn->sendMsg (_os);
			break;
		case DEVICE_SERVER:
			_os << "device "
				<< getCentraldNum () << " "
				<< getCentraldId () << " "
				<< getName () << " "
				<< hostname << " "
				<< port << " "
				<< device_type;
			ret = conn->sendMsg (_os);
			break;
		default:
			break;
	}
	return ret;
}

void ConnCentrald::updateStatusWait (rts2core::Connection * conn)
{
	if (conn)
	{
		if (getMaster ()->commandOriginatorPending (this, conn))
			return;
	}
	else
	{
		if (statusCommandRunning == 0)
			return;
		if (getMaster ()->commandOriginatorPending (this, NULL))
			return;
	}

	master->sendBopMessage (master->getState (), master->getStateForConnection (this), this);
	sendCommandEnd (DEVDEM_OK, "OK");
	statusCommandRunning--;
}

int ConnCentrald::commandDevice ()
{
	if (isCommand ("authorize"))
	{
		int client;
		int dev_key;
		if (paramNextInteger (&client) || paramNextInteger (&dev_key) || !paramEnd ())
			return -2;

		if (client < 0)
		{
			logStream (MESSAGE_ERROR) << "invalid client ID requested for authentification: " << client << " from " << getName () << sendLog;
			return -2;
		}

		rts2core::Connection *conn = master->getConnection (client);

		// client vanished when we processed data..
		if (conn == NULL)
		{
			sendCommandEnd (DEVDEM_E_SYSTEM, "client vanished during auth sequence");
			return -1;
		}

		if (conn->getKey () == 0)
		{
			sendAValue ("authorization_failed", client);
			sendCommandEnd (DEVDEM_E_SYSTEM, "client didn't ask for authorization");
			return -1;
		}

		if (conn->getKey () != dev_key)
		{
			sendAValue ("authorization_failed", client);
			sendCommandEnd (DEVDEM_E_SYSTEM, "invalid authorization key");
			return -1;
		}

		sendAValue ("authorization_ok", client);
		sendInfo ();

		return 0;
	}
	if (isCommand ("key"))
	{
		return sendDeviceKey ();
	}
	if (isCommand ("info"))
	{
		master->info ();
		return sendInfo ();
	}
	if (isCommand ("on"))
	{
		return master->changeStateOn (getName ());
	}
	if (isCommand ("standby"))
	{
		return master->changeStateStandby (getName ());
	}
	if (isCommand ("off"))
	{
		return master->changeStateHardOff (getName ());
	}
	if (isCommand ("soft_off"))
	{
		return master->changeStateSoftOff (getName ());
	}
	if (isCommand ("weather_update"))
	{
		return (master->weatherUpdate (this));
	}
	return rts2core::Connection::command ();
}

int ConnCentrald::sendStatusInfo ()
{
	std::ostringstream _os;
	_os << PROTO_STATUS " " << master->getState ();
	return sendMsg (_os);
}

int ConnCentrald::sendAValue (const char *val_name, int value)
{
	std::ostringstream _os;
	_os << PROTO_AUTH " " << val_name << " " << value;
	return sendMsg (_os);
}

int ConnCentrald::commandClient ()
{
	if (isCommand ("password"))
	{
		char *passwd;
		if (paramNextString (&passwd) || !paramEnd ())
			return -2;

		if (login == passwd)
		{
			std::ostringstream _os;
			authorized = 1;
			_os << "logged_as " << getCentraldId ();
			sendMsg (_os);
			sendStatusInfo ();
			return 0;
		}
		else
		{
			sleep (5);			 // wait some time to prevent repeat attack
			sendCommandEnd (DEVDEM_E_SYSTEM, "invalid login or password");
			return -1;
		}
	}
	if (authorized)
	{
		if (isCommand ("info"))
		{
			master->info ();
			return sendInfo ();
		}
		if (isCommand ("key"))
		{
			return sendDeviceKey ();
		}
		if (isCommand ("on"))
		{
			return master->changeStateOn (login.c_str ());
		}
		if (isCommand ("standby"))
		{
			return master->changeStateStandby (login.c_str ());
		}
		if (isCommand ("off"))
		{
			return master->changeStateHardOff (login.c_str ());
		}
		if (isCommand ("soft_off"))
		{
			return master->changeStateSoftOff (login.c_str ());
		}
		if (isCommand (COMMAND_OPEN))
		{
			if (!paramEnd ())
				return -2;
			return master->startOpen () ? DEVDEM_E_PARAMSVAL : DEVDEM_OK;
		}
		if (isCommand (COMMAND_CLOSE))
		{
			if (!paramEnd ())
				return -2;
			return master->startClose () ? DEVDEM_E_PARAMSVAL : DEVDEM_OK;
		}
	}
	return rts2core::Connection::command ();
}

int ConnCentrald::command ()
{
	if (isCommand ("login"))
	{
		if (getType () == NOT_DEFINED_SERVER)
		{
			char *in_login;
			char *in_name;

			srandom (time (NULL));

			if (paramNextString (&in_login) || paramNextString (&in_name) || !paramEnd ())
				return -2;

			login = std::string (in_login);
			setName (getCentraldId (), in_name);

			setType (CLIENT_SERVER);
			master->connAdded (this);
			return 0;
		}
		else
		{
			sendCommandEnd (DEVDEM_E_COMMAND, "cannot switch server type to CLIENT_SERVER");
			return -1;
		}
	}
	else if (isCommand ("register"))
	{
		if (getType () == NOT_DEFINED_SERVER)
		{
			int centraldNum;
			char *reg_device;
			char *in_hostname;

			if (paramNextInteger (&centraldNum)
				|| paramNextString (&reg_device)
				|| paramNextInteger (&device_type)
				|| paramNextString (&in_hostname)
				|| paramNextInteger (&port)
				|| !paramEnd ())
				return -2;

			if (rts2core::Connection *c = master->findName (reg_device))
			{
				std::ostringstream _os;
				_os << "name " << reg_device << " already registered with id " << c->getCentraldId ();
				sendCommandEnd (DEVDEM_E_SYSTEM, _os.str ().c_str ());
				return -1;
			}

			setName (centraldNum, reg_device);
			strncpy (hostname, in_hostname, HOST_NAME_MAX);

			setType (DEVICE_SERVER);
			sendStatusInfo ();

			sendAValue ("registered_as", getCentraldId ());
			master->connAdded (this);
			sendInfo ();
			return 0;
		}
		else
		{
			sendCommandEnd (DEVDEM_E_COMMAND,
				"cannot switch server type to DEVICE_SERVER");
			return -1;
		}
	}
	else if (isCommand ("message_mask"))
	{
		int newMask;
		if (paramNextInteger (&newMask) || !paramEnd ())
			return -2;
		messageMask = newMask;
		return 0;
	}
	else if (getType () == DEVICE_SERVER)
		return commandDevice ();
	else if (getType () == CLIENT_SERVER)
		return commandClient ();
	// else
	return rts2core::Connection::command ();
}

Centrald::Centrald (int argc, char **argv):Daemon (argc, argv, SERVERD_HARD_OFF | BAD_WEATHER)
{
	connNum = 0;

	configFile = NULL;
	logFileSource = LOGFILE_DEF;
	fileLog = NULL;

	createValue (morning_off, "morning_off", "switch to off in the morning", false, RTS2_VALUE_WRITABLE);
	createValue (morning_standby, "morning_standby", "switch to standby in the morning", false, RTS2_VALUE_WRITABLE);

	createValue (openClose, "open_close", "open/close operation in progress", false);
	openClose->addSelVal ("none");
	openClose->addSelVal ("opening");
	openClose->addSelVal ("closing");

	createValue (openCloseTimeout, "open_close_timeout", "timeout for open/close device", false);
	createValue (openCloseLast, "open_close_last", "name of opening/closing device", false);
	createValue (failedClose, "close_failed", "device(s) failed to close", false);

	createValue (requiredDevices, "required_devices", "devices necessary to automatically switch system to on state", false, RTS2_VALUE_WRITABLE);
	createValue (badWeatherDevices, "bad_weather_devices", "devices reporting bad weather or required and not present", false);

	createValue (openSequence, "open_sequence", "opening sequence (reverse for closing)", false, RTS2_VALUE_WRITABLE);

	createValue (badWeatherReason, "bad_weather_reason", "why system was switched to bad weather", false);
	createValue (badWeatherDevice, "bad_weather_device", "device reporting as the first bad weather", false);

	createValue (nextStateChange, "next_state_change", "time of next state change", false);
	createValue (nextState, "next_state", "next server state", false);
	nextState->addSelVal ("day");
	nextState->addSelVal ("evening");
	nextState->addSelVal ("dusk");
	nextState->addSelVal ("night");
	nextState->addSelVal ("dawn");
	nextState->addSelVal ("morning");

	createConstValue (observerLng, "longitude", "observatory longitude", false, RTS2_DT_DEGREES);
	createConstValue (observerLat, "latitude", "observatory latitude", false, RTS2_DT_DEC);

	createValue (nightHorizon, "night_horizon", "observatory night horizon", false, RTS2_DT_DEC | RTS2_VALUE_WRITABLE);
	createValue (dayHorizon, "day_horizon", "observatory day horizon", false, RTS2_DT_DEC | RTS2_VALUE_WRITABLE);

	createValue (eveningTime, "evening_time", "time needed to cool down cameras", false, RTS2_VALUE_WRITABLE);
	createValue (morningTime, "morning_time", "time needed to heat up cameras", false, RTS2_VALUE_WRITABLE);

	createValue (nightBeginning, "night_beginning", "beginning of the next night", false);
	createValue (nightEnding, "night_ending", "ending of the current or next night", false);

	createValue (darkBeginning, "dark_beginning", "beginning of the next dark period", false);
	createValue (darkEnding, "dark_ending", "ending of the current or next dark period", false);

	createValue (switchedStandby, "switched_standby", "times when the system was switched to standby", false);

	createValue (lastOn, "last_night_on", "time when system was last switched to on in ready night state", false);

	createValue (sunAlt, "sun_alt", "Sun altitude", false, RTS2_DT_DEC);
	createValue (sunAz, "sun_az", "Sun azimuth", false, RTS2_DT_DEGREES);

	createValue (sunRise, "sun_rise", "sun rise", false);
	createValue (sunSet, "sun_set", "sun set", false);

	createValue (moonAlt, "moon_alt", "[deg] moon altitude", false, RTS2_DT_DEC);
	createValue (moonAz, "moon_az", "[deg] moon azimuth", false, RTS2_DT_DEGREES);

	createValue (lunarPhase, "lunar_phase", "[deg] lunar phase angle (sun-earth-moon)", false, RTS2_DT_DEGREES);
	createValue (lunarLimb, "lunar_limb", "[deg] lunar bright limb", false, RTS2_DT_DEGREES);

	createValue (moonRise, "moon_rise", "Moon rise", false);
	createValue (moonSet, "moon_set", "Moon set", false);

	addOption (OPT_CONFIG, "config", 1, "configuration file");
	addOption (OPT_LOGFILE, "logfile", 1, "log file (put '-' to log to stderr");

	setIdleInfoInterval (300);
}

Centrald::~Centrald (void)
{
	if (fileLog)
	{
		fileLog->close ();
		delete fileLog;
	}
	// do not report any priority changes
	priority_client = -2;
}

void Centrald::openLog ()
{
	if (fileLog)
	{
		fileLog->close ();
		delete fileLog;
	}
	if (logFile == std::string ("-"))
	{
		fileLog = NULL;
		return;
	}
	fileLog = new std::ofstream ();
	fileLog->open (logFile.c_str (), std::ios_base::out | std::ios_base::app);
}

int Centrald::reloadConfig ()
{
	int ret;
	Configuration *config = Configuration::instance ();
	ret = config->loadFile (configFile);
	if (ret)
		return ret;

	if (logFileSource != LOGFILE_ARG)
	{
		config->getString ("centrald", "logfile", logFile, RTS2_LOG_FILE);
		logFileSource = LOGFILE_CNF;
	}

	openLog ();

	observer = config->getObserver ();

	observerLng->setValueDouble (observer->lng);
	observerLat->setValueDouble (observer->lat);

	requiredDevices->setValueArray (config->observatoryRequiredDevices ());
	openSequence->setValueArray (config->openSequence ());

	nightHorizon->setValueDouble (config->getDoubleDefault ("observatory", "night_horizon", -10));

	double t_h = config->getDoubleDefault ("observatory", "day_horizon", 0);
	if (t_h < nightHorizon->getValueDouble ())
	{
		t_h = 0;
		if (t_h < nightHorizon->getValueDouble ())
			t_h = nightHorizon->getValueDouble () + 1;
		logStream (MESSAGE_ERROR) << "day_horizon must be higher then night_horizon, setting it to " << t_h << sendLog;
	}
	dayHorizon->setValueDouble (t_h);

	eveningTime->setValueInteger (config->getIntegerDefault ("observatory", "evening_time", 7200));
	morningTime->setValueInteger (config->getIntegerDefault ("observatory", "morning_time", 1800));

	next_event_time = 0;

	constInfoAll ();

	return 0;
}

int Centrald::processOption (int in_opt)
{
	switch (in_opt)
	{
		case OPT_CONFIG:
			configFile = optarg;
			break;
		case OPT_LOGFILE:
			logFile = std::string (optarg);
			logFileSource = LOGFILE_ARG;
			break;
		default:
			return Daemon::processOption (in_opt);
	}
	return 0;
}

int Centrald::init ()
{
	int ret;
	setPort (atoi (RTS2_CENTRALD_PORT));
	ret = Daemon::init ();
	if (ret)
		return ret;

	srandom (time (NULL));

	ret = reloadConfig ();
	if (ret)
		return ret;

	// only set morning_off and morning_standby values at firts config load
	morning_off->setValueBool (Configuration::instance ()->getBoolean ("centrald", "morning_off", true));
	morning_standby->setValueBool (Configuration::instance ()->getBoolean ("centrald", "morning_standby", true));

	centraldConnRunning (NULL);

#ifndef RTS2_HAVE_FLOCK
	// reopen..
	ret = checkLockFile ();
	if (ret)
		return ret;
#endif
	return lockFile ();
}

int Centrald::initValues ()
{
	time_t curr_time;

	rts2_status_t call_state;

	curr_time = time (NULL);

	next_event (observer, &curr_time, &call_state, &next_event_type,
		&next_event_time, nightHorizon->getValueDouble (),
		dayHorizon->getValueDouble (), eveningTime->getValueInteger (),
		morningTime->getValueInteger ());

	Configuration *config = Configuration::instance ();

	if (config->getBoolean ("centrald", "reboot_on", false))
	{
		maskCentralState (SERVERD_ONOFF_MASK, 0, "switched on centrald reboot");
	}
	else
	{
		maskCentralState (SERVERD_ONOFF_MASK, SERVERD_HARD_OFF, "switched on centrald reboot");
	}

	nextStateChange->setValueTime (next_event_time);
	nextState->setValueInteger (next_event_type);

	next_event_time = 0;

	return Daemon::initValues ();
}

void Centrald::connectionRemoved (rts2core::Connection * conn)
{
	// update weather
	weatherChanged (conn->getName (), "connection removed");
	stopChanged (conn->getName (), "connection removed");
	// make sure we will change BOP mask..
	bopMaskChanged ();
	// and make sure we aren't the last who block status info
	for (connections_t::iterator iter = getConnections ()->begin (); iter != getConnections ()->end (); iter++)
	{
		(*iter)->updateStatusWait (NULL);
		std::ostringstream os;
		switch (conn->getType ())
		{
			case CLIENT_SERVER:
				os << "delete_client " << conn->getCentraldId ();
				(*iter)->sendMsg (os.str ());
				break;
			case DEVICE_SERVER:
				os << "delete_device " << conn->getCentraldId () << " " << conn->getName ();
				(*iter)->sendMsg (os.str ());
				break;
			default:
				break;
		}
	}
}

int Centrald::setValue (rts2core::Value *old_value, rts2core::Value *new_value)
{
	if (old_value == dayHorizon)
	{
		if (new_value->getValueDouble () < nightHorizon->getValueDouble ())
			return -2;
	}
	else if (old_value == nightHorizon)
	{
		if (new_value->getValueDouble () > dayHorizon->getValueDouble ())
			return -2;
	}
	return rts2core::Daemon::setValue (old_value, new_value);
}

void Centrald::stateChanged (rts2_status_t new_state, rts2_status_t old_state, const char *description, rts2core::Connection *commandedConn)
{
	Daemon::stateChanged (new_state, old_state, description, commandedConn);
	if ((getState () & ~BOP_MASK) != (old_state & ~BOP_MASK))
	{
		logStream (MESSAGE_INFO) << "State changed from " << rts2core::CentralState::getString (old_state)
			<< " to " << rts2core::CentralState::getString (getState ())
			<< " description " << description
			<< sendLog;
		sendStatusMessage (getState (), description);
	}

	// open if needed
	if ((new_state & WEATHER_MASK) == GOOD_WEATHER && (new_state & SERVERD_ONOFF_MASK) == SERVERD_ON)
	{
		switch (new_state & SERVERD_STATUS_MASK)
		{
			case SERVERD_DUSK:
			case SERVERD_NIGHT:
			case SERVERD_DAWN:
				startOpen ();
				break;
			default:
				startClose ();
				break;
		}
	}
	else
	{
		startClose ();
	}
}

rts2core::Connection * Centrald::createConnection (int in_sock)
{
	connNum++;
	return new ConnCentrald (in_sock, this, connNum);
}

void Centrald::connAdded (ConnCentrald * added)
{
	connections_t::iterator iter;
	for (iter = getConnections ()->begin (); iter != getConnections ()->end (); iter++)
	{
		rts2core::Connection *conn = *iter;
		added->sendConnectedInfo (conn);
	}
}

rts2core::Connection * Centrald::getConnection (int conn_num)
{
	connections_t::iterator iter;
	for (iter = getConnections ()->begin (); iter != getConnections ()->end (); iter++)
	{
		ConnCentrald *conn = (ConnCentrald *) * iter;
		if (conn->getCentraldId () == conn_num)
			return conn;
	}
	return NULL;
}

int Centrald::changeState (rts2_status_t new_state, const char *user)
{
	logStream (MESSAGE_INFO) << "State switched to " << rts2core::CentralState::getString (new_state) << " by " << user << sendLog;
	maskCentralState (SERVERD_ONOFF_MASK | SERVERD_STATUS_MASK, new_state, user);
	return 0;
}

int Centrald::info ()
{
	struct ln_equ_posn pos, parallax;
	struct ln_rst_time rst;
	struct ln_hrz_posn hrz;
	double JD = ln_get_julian_from_sys ();

	ln_get_solar_equ_coords (JD, &pos);
	ln_get_parallax (&pos, ln_get_earth_solar_dist (JD), observer, 1700, JD, &parallax);
	pos.ra += parallax.ra;
	pos.dec += parallax.dec;
	ln_get_hrz_from_equ (&pos, observer, JD, &hrz);

	sunAlt->setValueDouble (hrz.alt);
	sunAz->setValueDouble (hrz.az);

	ln_get_lunar_equ_coords (JD, &pos);
	ln_get_parallax (&pos, ln_get_earth_solar_dist (JD), observer, 1700, JD, &parallax);
	pos.ra += parallax.ra;
	pos.dec += parallax.dec;
	ln_get_hrz_from_equ (&pos, observer, JD, &hrz);

	moonAlt->setValueDouble (hrz.alt);
	moonAz->setValueDouble (hrz.az);

	ln_get_solar_rst (JD, observer, &rst);

	sunRise->setValueDouble (timetFromJD (rst.rise));
	sunSet->setValueDouble (timetFromJD (rst.set));

	lunarPhase->setValueDouble (ln_get_lunar_phase (JD));
	lunarLimb->setValueDouble (ln_get_lunar_bright_limb (JD));

	ln_get_lunar_rst (JD, observer, &rst);

	moonRise->setValueDouble (timetFromJD (rst.rise));
	moonSet->setValueDouble (timetFromJD (rst.set));

	return Daemon::info ();
}

int Centrald::idle ()
{
	time_t curr_time;

	rts2_status_t call_state;

	curr_time = time (NULL);

	if (curr_time < next_event_time)
		return Daemon::idle ();

	next_event (observer, &curr_time, &call_state, &next_event_type,
		&next_event_time, nightHorizon->getValueDouble (),
		dayHorizon->getValueDouble (), eveningTime->getValueInteger (),
		morningTime->getValueInteger ());

	if (getState () != call_state)
	{
		nextStateChange->setValueTime (next_event_time);
		nextState->setValueInteger (next_event_type);

		if (next_event_type == SERVERD_DUSK)
		{
			switchedStandby->clear ();
			sendValueAll (switchedStandby);
		}

		// update state only if state isn't OFF or SOFT_OFF
		if ((getState () & SERVERD_STATUS_MASK) == SERVERD_MORNING && (call_state & SERVERD_STATUS_MASK) == SERVERD_DAY)
		{
			if (morning_off->getValueBool ())
				maskCentralState (SERVERD_ONOFF_MASK, SERVERD_HARD_OFF, "switched off on day by idle routine");
			else if (morning_standby->getValueBool () && !((getState () & SERVERD_ONOFF_MASK) == SERVERD_HARD_OFF || (getState () & SERVERD_ONOFF_MASK) == SERVERD_SOFT_OFF))
				maskCentralState (SERVERD_ONOFF_MASK, SERVERD_STANDBY, "switched to standby on day by idle routine");
		}

		maskCentralState (SERVERD_STATUS_MASK, call_state, "by idle routine");

		time_t nstart, nstop;

		getNight (curr_time, observer, nightHorizon->getValueDouble (), nstart, nstop);

		nightBeginning->setValueTime (nstart);
		nightEnding->setValueTime (nstop);

		getNight (curr_time, observer, 0, nstart, nstop);

		darkBeginning->setValueTime (nstart);
		darkEnding->setValueTime (nstop);

		// send update about next state transits..
		infoAll ();
	}
	return Daemon::idle ();
}

void Centrald::deviceReady (rts2core::Connection * conn)
{
	Daemon::deviceReady (conn);
	// check again for weather state..
	weatherChanged (conn->getName (), "device ready");
	stopChanged (conn->getName (), "device ready");
}

void Centrald::postEvent (rts2core::Event * event)
{
	switch (event->getType ())
	{
		case EVENT_CLOSE_TIMEOUT:
			if (openClose->getValueInteger () == 2)
			{
				failedClose->addValue (openCloseLast->getValueString ());
				sendValueAll (failedClose);
				doClose ();
			}
			break;
	}
	Daemon::postEvent (event);
}

void Centrald::sendMessage (messageType_t in_messageType, const char *in_messageString)
{
	Message msg = Message ("centrald", in_messageType, in_messageString);
	Daemon::sendMessage (in_messageType, in_messageString);
	processMessage (msg);
}

void Centrald::message (Message & msg)
{
	processMessage (msg);
}

void Centrald::processMessage (Message & msg)
{
	// log it
	if (fileLog)
	{
		(*fileLog) << msg << std::endl;
	}
	else
	{
		std::cerr << msg << std::endl;
	}

	// and send it to all
	sendMessageAll (msg);
}

void Centrald::signaledHUP ()
{
	reloadConfig ();
	Daemon::signaledHUP ();
}

void Centrald::weatherChanged (const char * device, const char * msg)
{
	// state of the required devices
	std::vector <std::string> failedArr;
	std::vector <std::string>::iterator namIter;
	for (namIter = requiredDevices->valueBegin (); namIter != requiredDevices->valueEnd (); namIter++)
		failedArr.push_back (*namIter);

	connections_t::iterator iter;
	// check if some connection block weather
	for (iter = getConnections ()->begin (); iter != getConnections ()->end (); iter ++)
	{
		// if connection is required..
		namIter = std::find (failedArr.begin (), failedArr.end (), std::string ((*iter)->getName ()));
		if (namIter != failedArr.end ())
		{
			if ((*iter)->isGoodWeather () == true && (*iter)->isConnState (CONN_CONNECTED))
				failedArr.erase (namIter);
			// otherwise, connection name will not be erased from
			// failedArr, so failedArr size will be larger then 0,
			// so newWeather will be set to false in size check - few lines
			// bellow.
		}
		else  if ((*iter)->isGoodWeather () == false)
		{
			failedArr.push_back ((*iter)->getName ());
		}
		if (!strcmp ((*iter)->getName (), device))
		{
			// device which causes bad weather..
			if ((*iter)->isGoodWeather () == false && strlen (badWeatherReason->getValue ()) == 0)
			{
				if (msg == NULL)
					msg = "NULL";
				badWeatherReason->setValueCharArr (msg);
				badWeatherDevice->setValueCharArr (device);
				logStream (MESSAGE_INFO) << "received bad weather from " << device << " claiming that " << device << ": '" << msg << "'" << sendLog;
			}
		}
	}
	badWeatherDevices->setValueArray (failedArr);
	sendValueAll (badWeatherDevices);

	setWeatherState (failedArr.size () > 0 ? false : true, "weather state update from weatherChanged");
	if (failedArr.size () > 0)
	{
		rts2core::LogStream ls = logStream (MESSAGE_DEBUG);
		ls << "failed devices:";
		for (namIter = failedArr.begin (); namIter != failedArr.end (); namIter++)
			ls << " " << (*namIter);
		ls << sendLog;
	}
	else
	{
		badWeatherReason->setValueCharArr ("");
		badWeatherDevice->setValueCharArr ("");
	}
	sendValueAll (badWeatherReason);
	sendValueAll (badWeatherDevice);
}

int Centrald::weatherUpdate (rts2core::Connection *conn)
{
	char *w_device;
	char *w_update;
	if (conn->paramNextString (&w_device) || conn->paramNextString (&w_update) || !conn->paramEnd ())
		return -2;

	if (!strcmp (w_device, badWeatherDevice->getValue ()))
	{
		badWeatherReason->setValueCharArr (w_update);
		sendValueAll (badWeatherReason);
	}
	return 0;
}

void Centrald::stopChanged (const char * device, const char * msg)
{
	// state of the required devices
	std::vector <std::string> failedArr;
	std::vector <std::string>::iterator namIter;
	for (namIter = requiredDevices->valueBegin (); namIter != requiredDevices->valueEnd (); namIter++)
		failedArr.push_back (*namIter);

	connections_t::iterator iter;
	// check if some connection block weather
	for (iter = getConnections ()->begin (); iter != getConnections ()->end (); iter ++)
	{
		// if connection is required..
		namIter = std::find (failedArr.begin (), failedArr.end (), std::string ((*iter)->getName ()));
		if (namIter != failedArr.end ())
		{
			if ((*iter)->canMove () == true && (*iter)->isConnState (CONN_CONNECTED))
				failedArr.erase (namIter);
			// otherwise, connection name will not be erased from
			// failedArr, so failedArr size will be larger then 0,
			// so newWeather will be set to false in size check - few lines
			// bellow.
		}
		else  if ((*iter)->canMove () == false)
		{
			failedArr.push_back ((*iter)->getName ());
		}
	}

	setStopState (failedArr.size () > 0 ? true : false, "stop state update from stopChanged");
	if (failedArr.size () > 0)
	{
		rts2core::LogStream ls = logStream (MESSAGE_DEBUG);
		ls << "devices stopping telescope:";
		for (namIter = failedArr.begin (); namIter != failedArr.end (); namIter++)
			ls << " " << (*namIter);
		ls << sendLog;
	}
}

void Centrald::bopMaskChanged ()
{
	int bopState = 0;
	connections_t::iterator iter;
	maskState (BOP_MASK, bopState, "changed BOP state");
	for (iter = getConnections ()->begin (); iter != getConnections ()->end (); iter++)
	{
		bopState |= (*iter)->getBopState ();
		if ((*iter)->getType () == DEVICE_SERVER)
			sendBopMessage (getState (), getStateForConnection (*iter), *iter);
	}
	sendStatusMessage (getState ());
}

void Centrald::openCloseChanged (const char *device, const char *msg)
{
	switch (openClose->getValueInteger ())
	{
		// opening
		case 1:
			doOpen ();
			break;
		// closing
		case 2:
			doClose ();
			break;
	}
}

int Centrald::statusInfo (rts2core::Connection * conn)
{
	ConnCentrald *c_conn = (ConnCentrald *) conn;
	int s_count = 0;
	// update system status
	connections_t::iterator iter;
	for (iter = getConnections ()->begin (); iter != getConnections ()->end (); iter++)
	{
		ConnCentrald *test_conn = (ConnCentrald *) * iter;
		// do not request status from client connections
		if (test_conn != conn && test_conn->getType () != CLIENT_SERVER)
		{
			if (conn->getType () == DEVICE_SERVER)
			{
				if (Configuration::instance ()->blockDevice (conn->getName (), test_conn->getName ()) == false)
					continue;
			}
			rts2core::CommandStatusInfo *cs = new rts2core::CommandStatusInfo (this, c_conn);
			cs->setOriginator (conn);
			test_conn->queCommand (cs);
			s_count++;
		}
	}
	// command was not send at all..
	if (s_count == 0)
	{
		return 0;
	}

	c_conn->statusCommandSend ();

	// indicate command pending, we will send command end once we will get reply from all devices
	return -1;
}

int Centrald::getStateForConnection (rts2core::Connection * conn)
{
	if (conn->getType () != DEVICE_SERVER)
		return getState ();
	int sta = getState ();
	// get rid of BOP mask
	sta &= ~BOP_MASK & ~DEVICE_ERROR_MASK;
	connections_t::iterator iter;
	// cretae BOP mask for device
	for (iter = getConnections ()->begin (); iter != getConnections ()->end (); iter++)
	{
		rts2core::Connection *test_conn = *iter;
		if (Configuration::instance ()->blockDevice (conn->getName (), test_conn->getName ()) == false)
			continue;
		sta |= test_conn->getBopState ();
	}
	return sta;
}

int Centrald::startOpen ()
{
	openClose->setValueInteger (1);
	return doOpen ();
}

int Centrald::startClose ()
{
	openClose->setValueInteger (2);
	return doClose ();
}

int Centrald::doOpen ()
{
	for (std::vector <std::string>::iterator iter = openSequence->valueBegin (); iter != openSequence->valueEnd (); iter++)
	{
		// first device which blocks opening shall be opened
		rts2core::Connection *dconn = getOpenConnection (iter->c_str ());
		if (dconn == NULL)
		{
			logStream (MESSAGE_ERROR) << "missing " << *iter << " from opening sequence, stopping opening" << sendLog;
			openClose->setValueInteger (0);
			updateOpenClose ();
			return -1;
		}
		else if ((dconn->getState () & DEVICE_BLOCK_OPEN))
		{
			dconn->queCommand (new rts2core::Command (this, COMMAND_OPEN));
			if (*iter != openCloseLast->getValueString ())
			{
				openCloseTimeout->setValueDouble (getNow () + 180);
				openCloseLast->setValueCharArr (dconn->getName ());
				updateOpenClose ();
			}
			return 0;
		}
	}
	updateOpenClose (true);
	return -1;
}

int Centrald::doClose ()
{
	if (openSequence->size () < 1)
		return 0;
	std::vector <std::string>::iterator iter = openSequence->valueEnd ();
	do
	{
		iter--;
		// first device which blocks opening shall be opened
		rts2core::Connection *dconn = getOpenConnection (iter->c_str ());
		if (dconn == NULL)
		{
			logStream (MESSAGE_ERROR) << "missing " << *iter << " from close sequence, ignoring" << sendLog;
		}
		else if (!(failedClose->isPresent (*iter)) && (dconn->getState () & DEVICE_BLOCK_CLOSE))
		{
			dconn->queCommand (new rts2core::Command (this, COMMAND_CLOSE));
			if (*iter != openCloseLast->getValueString ())
			{
				openCloseTimeout->setValueDouble (getNow () + 180);
				openCloseLast->setValueCharArr (dconn->getName ());
				updateOpenClose ();
				addTimer (180, new rts2core::Event (EVENT_CLOSE_TIMEOUT));
			}
			return 0;
		}
	}
	while (iter != openSequence->valueBegin ());

	updateOpenClose (true);
	failedClose->clear ();
	sendValueAll (failedClose);
	return -1;
}

void Centrald::updateOpenClose (bool clear)
{
	if (clear)
	{
		openClose->setValueInteger (0);
		openCloseLast->setValueCharArr ("");
		openCloseTimeout->setValueDouble (NAN);
	}

	sendValueAll (openClose);
	sendValueAll (openCloseLast);
	sendValueAll (openCloseTimeout);
}

void Centrald::maskCentralState (rts2_status_t state_mask, rts2_status_t new_state, const char *description, double start, double end, Connection *commandedConn)
{
	if (!(getState () & SERVERD_STANDBY))
	{
		switch (getState () & SERVERD_STATUS_MASK)
		{
			case SERVERD_DUSK:
			case SERVERD_NIGHT:
			case SERVERD_DAWN:
				switchedStandby->addValue (getNow ());
				sendValueAll (switchedStandby);
				break;
		}
	}
	maskState (state_mask, new_state, description, start, end, commandedConn);
	if (state_mask & SERVERD_STATUS_MASK)
	{
		if ((new_state & SERVERD_STATUS_MASK) == SERVERD_NIGHT && !(new_state & SERVERD_STANDBY))
			lastOn->setValueDouble (getNow ());
		else
			lastOn->setValueDouble (NAN);
		sendValueAll (lastOn);
	}
}

int main (int argc, char **argv)
{
	Centrald centrald (argc, argv);
	return centrald.run ();
}
