/*
 * Command classes.
 * Copyright (C) 2003-2007 Petr Kubanek <petr@kubanek.net>
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

#include <iostream>
#include <fstream>

#include "command.h"

using namespace std;
using namespace rts2core;

Command::Command (Block * _owner)
{
	owner = _owner;
	text = NULL;
	bopMask = 0;
	originator = NULL;
}

Command::Command (Block * _owner, const char *_text)
{
	owner = _owner;
	text = NULL;
	setCommand (_text);
	bopMask = 0;
	originator = NULL;
}

Command::Command (Command * _command)
{
	owner = _command->owner;
	text = NULL;
	connection = _command->connection;
	setCommand (_command->getText ());
	bopMask = _command->getBopMask ();
	originator = NULL;
}

Command::Command (Command & _command)
{
	owner = _command.owner;
	text = NULL;
	connection = _command.connection;
	setCommand (_command.getText ());
	bopMask = _command.getBopMask ();
	originator = NULL;
}

void Command::setCommand (std::ostringstream &_os)
{
	setCommand (_os.str().c_str());
}

void Command::setCommand (const char *_text)
{
	delete[] text;
	text = new char[strlen (_text) + 1];
	strcpy (text, _text);
}

Command::~Command (void)
{
	delete[] text;
}

int Command::send ()
{
	return connection->sendMsg (text);
}

int Command::commandReturn (int status, Connection * conn)
{
	#ifdef DEBUG_EXTRA
	logStream(MESSAGE_DEBUG) << "commandReturn status: " << status << " connection: " << conn->getName () << " command: " << text << sendLog;
	#endif
	switch (status)
	{
		case 0:
			return commandReturnOK (conn);
		case 1:
			return commandReturnQued (conn);
		default:
			return commandReturnFailed (status, conn);
	}
}

int Command::commandReturnOK (__attribute__ ((unused)) Connection * conn)
{
	if (originator)
		originator->postEvent (new Event (EVENT_COMMAND_OK, (void *) this));
	return -1;
}

int Command::commandReturnQued (Connection * conn)
{
	if (originator)
		originator->postEvent (new Event (EVENT_COMMAND_OK, (void *) this));
	return commandReturnOK (conn);
}

int Command::commandReturnFailed (__attribute__ ((unused)) int status, __attribute__ ((unused)) Connection * conn)
{
	if (originator)
		originator->postEvent (new Event (EVENT_COMMAND_FAILED, (void *) this));
	return -1;
}

void Command::deleteConnection (Connection * conn)
{
	if (conn == originator)
		originator = NULL;
}

CommandSendKey::CommandSendKey (Block * _master, int _centrald_id, int _centrald_num, int _key):Command (_master)
{
	centrald_id = _centrald_id;
	centrald_num = _centrald_num;
	key = _key;
}

int CommandSendKey::send ()
{
	std::ostringstream _os;
	_os << "auth " << centrald_id
		<< " " << centrald_num
		<< " " << key;
	setCommand (_os);
	return Command::send ();
}

CommandAuthorize::CommandAuthorize (Block * _master, int centralId, int key):Command (_master)
{
	std::ostringstream _os;
	_os << "authorize " << centralId << " " << key;
	setCommand (_os);
}

CommandKey::CommandKey (Block * _master, const char *device_name):Command (_master)
{
 	std::ostringstream _os;
	_os << "key " << device_name;
	setCommand (_os);
}

CommandCameraSettings::CommandCameraSettings (DevClientCamera * _camera):Command (_camera->getMaster ())
{
}

CommandExposure::CommandExposure (Block * _master, DevClientCamera * _camera, int _bopMask):Command (_master)
{
	setCommand (COMMAND_CCD_EXPOSURE);
	camera = _camera;
	setBopMask (_bopMask);
}

int CommandExposure::commandReturnOK (Connection *conn)
{
	camera->exposureCommandOK ();
	return Command::commandReturnOK (conn);
}

int CommandExposure::commandReturnFailed (int status, Connection * conn)
{
	camera->exposureFailed (status);
	return Command::commandReturnFailed (status, conn);
}

CommandReadout::CommandReadout (Block * _master):Command (_master)
{
	setCommand ("readout");
}

CommandShiftStart::CommandShiftStart (Block * _master, float expTime, int _bopMask):Command (_master)
{
	std::ostringstream _os;
	_os << COMMAND_CCD_SHIFTSTORE << " start " << expTime;
	setCommand (_os);
	setBopMask (_bopMask);
}

CommandShiftProgress::CommandShiftProgress (Block * _master, int shift, float expTime, int _bopMask):Command (_master)
{
	std::ostringstream _os;
	_os << COMMAND_CCD_SHIFTSTORE << " shift " << shift << " " << expTime;
	setCommand (_os);
	setBopMask (_bopMask);
}

CommandShiftEnd::CommandShiftEnd (Block * _master, int shift, float expTime, int _bopMask):Command (_master)
{
	std::ostringstream _os;
	_os << COMMAND_CCD_SHIFTSTORE << " end " << shift << " " << expTime;
	setCommand (_os);
	setBopMask (_bopMask);
}

CommandFitsStat::CommandFitsStat (Block * _master, double average, double min, double max, double sum, double mode):Command (_master)
{
	std::ostringstream _os;
	_os << COMMAND_FITS_STAT << " " << std::fixed << average << " " << min << " " << max << " " << sum << " " << mode;
	setCommand (_os);
}

void CommandFilter::setCommandFilter (int filter)
{
	std::ostringstream _os;
	_os << PROTO_SET_VALUE " filter = " << filter;
	setCommand (_os);
}

CommandFilter::CommandFilter (DevClientCamera * _camera, int filter):Command (_camera->getMaster ())
{
	camera = _camera;
	phot = NULL;
	filterCli = NULL;
	setCommandFilter (filter);
}

CommandFilter::CommandFilter (DevClientPhot * _phot, int filter):Command (_phot->getMaster ())
{
	camera = NULL;
	phot = _phot;
	filterCli = NULL;
	setCommandFilter (filter);
}

CommandFilter::CommandFilter (DevClientFilter * _filter, int filter):Command (_filter->getMaster ())
{
	camera = NULL;
	phot = NULL;
	filterCli = _filter;
	setCommandFilter (filter);
}

int CommandFilter::commandReturnOK (Connection * conn)
{
	if (camera)
		camera->filterOK ();
	if (phot)
		phot->filterOK ();
	if (filterCli)
		filterCli->filterOK ();
	return Command::commandReturnOK (conn);
}

int CommandFilter::commandReturnFailed (int status, Connection * conn)
{
	if (camera)
		camera->filterFailed (status);
	if (phot)
		phot->filterMoveFailed (status);
	if (filterCli)
		filterCli->filterMoveFailed (status);
	return Command::commandReturnFailed (status, conn);
}

CommandBox::CommandBox (DevClientCamera * _camera, int x, int y, int w, int h):CommandCameraSettings (_camera)
{
	std::ostringstream _os;
	_os << "box " << x << " " << y << " " << w << " " << h;
	setCommand (_os);
}

CommandCenter::CommandCenter (DevClientCamera * _camera, int width = -1, int height = -1):CommandCameraSettings (_camera)
{
	std::ostringstream _os;
	_os << "center " << width << " " << height;
	setCommand (_os);
}

CommandChangeValue::CommandChangeValue (Block * _master, std::string _valName, char op, int _operand):Command (_master)
{
	std::ostringstream _os;
	_os << PROTO_SET_VALUE " " <<  _valName << " " << op << " " << _operand;
	setCommand (_os);
}

CommandChangeValue::CommandChangeValue (Block * _master, std::string _valName, char op, long _operand):Command (_master)
{
	std::ostringstream _os;
	_os << PROTO_SET_VALUE " " << _valName << " " << op << " " << _operand;
	setCommand (_os);
}

CommandChangeValue::CommandChangeValue (Block * _master, std::string _valName, char op, float _operand):Command (_master)
{
	std::ostringstream _os;
	_os << PROTO_SET_VALUE " " << _valName << " " << op << " " << std::fixed << _operand;
	setCommand (_os);
}

CommandChangeValue::CommandChangeValue (Block * _master, std::string _valName, char op, double _operand):Command (_master)
{
	std::ostringstream _os;
	_os << PROTO_SET_VALUE " " << _valName << " " << op << " " << std::fixed << _operand;
	setCommand (_os);
}

CommandChangeValue::CommandChangeValue (Block * _master, std::string _valName, char op, double _operand1, double _operand2):Command (_master)
{
	std::ostringstream _os;
	_os << PROTO_SET_VALUE " " << _valName << " " << op << " " << std::fixed << _operand1 << " " << _operand2;
	setCommand (_os);
}

CommandChangeValue::CommandChangeValue (Block * _master, std::string _valName, char op, int _operand1, int _operand2, int _operand3):Command (_master)
{
	std::ostringstream _os;
	_os << PROTO_SET_VALUE " " << _valName << " " << op << " " << _operand1 << " " << _operand2 << " " << _operand3;
	setCommand (_os);
}

CommandChangeValue::CommandChangeValue (Block * _master,std::string _valName, char op, bool _operand):Command (_master)
{
	std::ostringstream _os;
	_os << PROTO_SET_VALUE " " << _valName << " " << op << " " << (_operand ? 1 : 0);
	setCommand (_os);
}

CommandChangeValue::CommandChangeValue (Block * _master,std::string _valName, char op, int x, int y, int w, int h):Command (_master)
{
	std::ostringstream _os;
	_os << PROTO_SET_VALUE " " << _valName << " " << op << " " << x << " " << y << " " << w << " " << h;
	setCommand (_os);
}

CommandChangeValue::CommandChangeValue (Block * _master, std::string _valName, char op, std::string _operand, bool raw):Command (_master)
{
	std::ostringstream _os;
	if (raw)
	{
		_os << PROTO_SET_VALUE " " << _valName << " " << op << " " << _operand;
	}
	else
	{
		_os << PROTO_SET_VALUE " " << _valName << " " << op << " \"" << _operand << "\"";
	}
	setCommand (_os);
}

CommandMove::CommandMove (Block * _master, DevClientTelescope * _tel):Command (_master)
{
	tel = _tel;
}

CommandMove::CommandMove (Block * _master, DevClientTelescope * _tel, double ra, double dec):Command (_master)
{
	std::ostringstream _os;
	_os << COMMAND_TELD_MOVE " " << std::fixed << ra << " " << dec;
	setCommand (_os);
	tel = _tel;
}

int CommandMove::commandReturnFailed (int status, Connection * conn)
{
	tel->moveFailed (status);
	return Command::commandReturnFailed (status, conn);
}

CommandMoveUnmodelled::CommandMoveUnmodelled (Block * _master, DevClientTelescope * _tel, double ra, double dec):CommandMove (_master, _tel)
{
	std::ostringstream _os;
	_os << "move_not_model " << std::fixed << ra << " " << dec;
	setCommand (_os);
}

CommandMoveMpec::CommandMoveMpec (Block * _master, DevClientTelescope * _tel, __attribute__ ((unused)) double ra, __attribute__ ((unused)) double dec, std::string mpec_oneline):CommandMove (_master, _tel)
{
	std::ostringstream _os;
	_os << COMMAND_TELD_MOVE_MPEC " \"" << mpec_oneline << "\"";
	setCommand (_os);
}

CommandMoveTle::CommandMoveTle (Block * _master, DevClientTelescope * _tel, __attribute__ ((unused)) double ra, __attribute__ ((unused)) double dec, std::string tle1, std::string tle2):CommandMove (_master, _tel)
{
	std::ostringstream _os;
	_os << COMMAND_TELD_MOVE_TLE " \"" << tle1 << "\" \"" << tle2 << "\"";
	setCommand (_os);
}

CommandMoveFixed::CommandMoveFixed (Block * _master, DevClientTelescope * _tel, double ra, double dec):CommandMove (_master,_tel)
{
	std::ostringstream _os;
	_os << "fixed " << std::fixed << ra << " " << dec;
	setCommand (_os);
}

CommandMoveAltAz::CommandMoveAltAz (Block * _master, DevClientTelescope * _tel, double alt, double az):CommandMove (_master, _tel)
{
	std::ostringstream _os;
	_os << COMMAND_TELD_ALTAZ " " << std::fixed << alt << " " << az;
	setCommand (_os);
}

CommandResyncMove::CommandResyncMove (Block * _master, DevClientTelescope * _tel, double ra, double dec):CommandMove (_master, _tel)
{
	std::ostringstream _os;
	_os << "resync " << std::fixed << ra << " " << dec;
	setCommand (_os);
}

CommandChange::CommandChange (Block * _master, double ra, double dec):Command (_master)
{
	std::ostringstream _os;
	_os << "change " << std::fixed << ra << " " << dec;
	setCommand (_os);
	tel = NULL;
}

CommandChange::CommandChange (DevClientTelescope * _tel, double ra, double dec):Command (_tel->getMaster ())
{
	std::ostringstream _os;
	_os << "change " << std::fixed << ra << " " << dec;
	setCommand (_os);
	tel = _tel;
}

CommandChange::CommandChange (CommandChange * _command, DevClientTelescope * _tel):Command (_command)
{
	tel = _tel;
}

int CommandChange::commandReturnFailed (int status, Connection * conn)
{
	if (tel)
		tel->moveFailed (status);
	return Command::commandReturnFailed (status, conn);
}

CommandCorrect::CommandCorrect (Block * _master, int corr_mark, int corr_img, int img_id, double ra_corr, double dec_corr, double pos_err):Command (_master)
{
 	std::ostringstream _os;
	_os << "correct " << corr_mark << " " << corr_img
		<< " " << img_id << " " << std::fixed << ra_corr
		<< " " << dec_corr << " " << pos_err;
	setCommand (_os);
}

CommandStartGuide::CommandStartGuide (Block * _master, char dir, double dir_dist):Command (_master)
{
	std::ostringstream _os;
	_os << "start_guide " << dir << " " << dir_dist;
	setCommand (_os);
}

CommandStopGuide::CommandStopGuide (Block * _master, char dir):Command (_master)
{
	std::ostringstream _os;
	_os << "stop_guide " << dir;
	setCommand (_os);
}

CommandWeather::CommandWeather (Block *_master, double temp, double hum, double pres):Command (_master)
{
	std::ostringstream _os;
	_os << COMMAND_TELD_WEATHER << " " << temp << " " << hum << " " << pres;
	setCommand (_os);
}

CommandCupolaSyncTel::CommandCupolaSyncTel (Block * _master, double ra, double dec):Command (_master)
{
	std::ostringstream _os;
	_os << COMMAND_CUPOLA_SYNCTEL " " << std::fixed << ra << " " << dec;
	setCommand (_os);
}

CommandCupolaNotMove::CommandCupolaNotMove (DevClientCupola * _copula):Command (_copula->getMaster ())
{
	std::ostringstream _os;
	copula = _copula;
	_os << "stop" ;
	setCommand (_os);
}

int CommandCupolaNotMove::commandReturnFailed (int status, Connection * conn)
{
	if (copula)
		copula->notMoveFailed (status);
	return Command::commandReturnFailed (status, conn);
}

void CommandChangeFocus::change (int _steps)
{
	std::ostringstream _os;
	_os << "X FOC_TOFFS += " << _steps;
	setCommand (_os);
}

CommandChangeFocus::CommandChangeFocus (DevClientFocus * _focuser, int _steps):Command (_focuser->getMaster ())
{
	focuser = _focuser;
	camera = NULL;
	change (_steps);
}

CommandChangeFocus::CommandChangeFocus (DevClientCamera * _camera, int _steps):Command (_camera->getMaster ())
{
	focuser = NULL;
	camera = _camera;
	change (_steps);
}

int CommandChangeFocus::commandReturnFailed (int status, Connection * conn)
{
	if (focuser)
		focuser->focusingFailed (status);
	if (camera)
		camera->focuserFailed (status);
	return Command::commandReturnFailed (status, conn);
}

void CommandSetFocus::set (int _steps)
{
	std::ostringstream _os;
	_os << "set " << _steps;
	setCommand (_os);
}

CommandSetFocus::CommandSetFocus (DevClientFocus * _focuser, int _steps):Command (_focuser->getMaster ())
{
	focuser = _focuser;
	camera = NULL;
	set (_steps);
}

CommandSetFocus::CommandSetFocus (DevClientCamera * _camera, int _steps):Command (_camera->getMaster ())
{
	focuser = NULL;
	camera = _camera;
	set (_steps);
}

int CommandSetFocus::commandReturnFailed (int status, Connection * conn)
{
	if (focuser)
		focuser->focusingFailed (status);
	if (camera)
		camera->focuserFailed (status);
	return Command::commandReturnFailed (status, conn);
}

CommandMirror::CommandMirror (DevClientMirror * _mirror, int _pos):Command (_mirror->getMaster ())
{
	mirror = _mirror;
	std::ostringstream _os;
	_os << "set " << (_pos == 0 ? "A" : "B");
	setCommand (_os);
}

int CommandMirror::commandReturnFailed (int status, Connection * conn)
{
	if (mirror)
		mirror->moveFailed (status);
	return Command::commandReturnFailed (status, conn);
}

CommandIntegrate::CommandIntegrate (DevClientPhot * _phot, int _filter, float _exp, int _count):Command (_phot->getMaster ())
{
	phot = _phot;
	std::ostringstream _os;
	_os << "intfil " << _filter << " " << std::fixed << _exp << " " <<_count;
	setCommand (_os);
}

CommandIntegrate::CommandIntegrate (DevClientPhot * _phot, float _exp, int _count):Command (_phot->getMaster ())
{
	phot = _phot;
	std::ostringstream _os;
	_os << "integrate " << std::fixed << _exp << " " << _count;
	setCommand (_os);
}

int CommandIntegrate::commandReturnFailed (int status, Connection * conn)
{
	if (phot)
		phot->integrationFailed (status);
	return Command::commandReturnFailed (status, conn);
}

CommandExecNext::CommandExecNext (Block * _master, int next_id):Command (_master)
{
	std::ostringstream _os;
	_os << "next " << next_id;
	setCommand (_os);
}

CommandExecNextPlan::CommandExecNextPlan (Block * _master, int next_plan_id):Command (_master)
{
	std::ostringstream _os;
	_os << "next_plan " << next_plan_id;
	setCommand (_os);
}

CommandExecNow::CommandExecNow (Block * _master, int now_id):Command (_master)
{
	std::ostringstream _os;
	_os << "now " << now_id;
	setCommand (_os);
}

CommandExecGrb::CommandExecGrb (Block * _master, int _grb_id):Command (_master)
{
	std::ostringstream _os;
	grb_id = _grb_id;
	_os << "grb " << grb_id;
	setCommand (_os);
}

CommandQueueNow::CommandQueueNow (Block *_master, const char *queue, int tar_id):Command (_master)
{
	std::ostringstream _os;
	_os << "now " << queue << " " << tar_id;
	setCommand (_os);
}

CommandQueueNowOnce::CommandQueueNowOnce (Block *_master, const char *queue, int tar_id):Command (_master)
{
	std::ostringstream _os;
	_os << "now_once " << queue << " " << tar_id;
	setCommand (_os);
}

CommandQueueAt::CommandQueueAt (Block * _master, const char *queue, int tar_id, double t_start, double t_end):Command (_master)
{
	std::ostringstream _os;
	_os << "queue_at " << queue << " " << tar_id << " " << std::fixed << t_start << " " << std::fixed << t_end;
	setCommand (_os);
}

CommandExecShower::CommandExecShower (Block * _master):Command (_master)
{
	setCommand ("shower");
}

CommandKillAll::CommandKillAll (Block *_master):Command (_master)
{
	setCommand ("killall");
}

CommandKillAllWithoutScriptEnds::CommandKillAllWithoutScriptEnds (Block *_master):Command (_master)
{
	setCommand ("killall_wse");
}

CommandScriptEnds::CommandScriptEnds (Block *_master):Command (_master)
{
	setCommand ("script_ends");
}

CommandMessageMask::CommandMessageMask (Block * _master, int _mask):Command (_master)
{
	std::ostringstream _os;
	_os << "message_mask " << _mask;
	setCommand (_os);
}

CommandInfo::CommandInfo (Block * _master):Command (_master)
{
	setCommand (COMMAND_INFO);
}

int CommandInfo::commandReturnOK (Connection * conn)
{
	if (connection && connection->getOtherDevClient ())
		connection->getOtherDevClient ()->infoOK ();
	return Command::commandReturnOK (conn);
}

int CommandInfo::commandReturnFailed (int status, Connection * conn)
{
	if (connection && connection->getOtherDevClient ())
		connection->getOtherDevClient ()->infoFailed ();
	return Command::commandReturnFailed (status, conn);
}

CommandStatusInfo::CommandStatusInfo (Block * master, Connection * _control_conn):Command(master)
{
	control_conn = _control_conn;
	setCommand ("status_info");
}

int CommandStatusInfo::commandReturnOK (Connection * conn)
{
	if (control_conn)
		control_conn->updateStatusWait (conn);
	return Command::commandReturnOK (conn);
}

int CommandStatusInfo::commandReturnFailed (__attribute__ ((unused)) int status, Connection * conn)
{
	if (control_conn)
		control_conn->updateStatusWait (conn);
	return Command::commandReturnOK (conn);
}

void CommandStatusInfo::deleteConnection (Connection * conn)
{
	if (control_conn == conn)
		control_conn = NULL;
	Command::deleteConnection (conn);
}

CommandDeviceStatus::CommandDeviceStatus (Block * master, Connection * _control_conn):CommandStatusInfo (master, _control_conn)
{
	setCommand ("device_status");
}

CommandParallacticAngle::CommandParallacticAngle (Block * _master, double reftime, double pa, double rate, double altitude, double azimuth):Command (_master)
{
	std::ostringstream _os;
	_os << COMMAND_PARALLACTIC_UPDATE " " << std::fixed << reftime << " " << pa << " " << rate << " " << altitude << " " << azimuth;
	setCommand (_os);
}

CommandMoveAz::CommandMoveAz (Block *_master, double az):Command (_master)
{
	std::ostringstream _os;
	_os << COMMAND_CUPOLA_AZ " " << std::fixed << az;
	setCommand (_os);
}
