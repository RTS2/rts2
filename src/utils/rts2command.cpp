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

#include "rts2command.h"

using namespace std;
using namespace rts2core;

Rts2Command::Rts2Command (rts2core::Block * _owner)
{
	owner = _owner;
	text = NULL;
	bopMask = 0;
	originator = NULL;
}

Rts2Command::Rts2Command (rts2core::Block * _owner, const char *_text)
{
	owner = _owner;
	setCommand (_text);
	bopMask = 0;
	originator = NULL;
}

Rts2Command::Rts2Command (Rts2Command * _command)
{
	owner = _command->owner;
	connection = _command->connection;
	setCommand (_command->getText ());
	bopMask = _command->getBopMask ();
	originator = NULL;
}

Rts2Command::Rts2Command (Rts2Command & _command)
{
	owner = _command.owner;
	connection = _command.connection;
	setCommand (_command.getText ());
	bopMask = _command.getBopMask ();
	originator = NULL;
}

void Rts2Command::setCommand (std::ostringstream &_os)
{
	setCommand (_os.str().c_str());
}

void Rts2Command::setCommand (const char *_text)
{
	text = new char[strlen (_text) + 1];
	strcpy (text, _text);
}

Rts2Command::~Rts2Command (void)
{
	delete[]text;
}

int Rts2Command::send ()
{
	return connection->sendMsg (text);
}

int Rts2Command::commandReturn (int status, Rts2Conn * conn)
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

int Rts2Command::commandReturnOK (Rts2Conn * conn)
{
	if (originator)
		originator->postEvent (new Rts2Event (EVENT_COMMAND_OK, (void *) this));
	return -1;
}

int Rts2Command::commandReturnQued (Rts2Conn * conn)
{
	if (originator)
		originator->postEvent (new Rts2Event (EVENT_COMMAND_OK, (void *) this));
	return commandReturnOK (conn);
}

int Rts2Command::commandReturnFailed (int status, Rts2Conn * conn)
{
	if (originator)
		originator->
			postEvent (new Rts2Event (EVENT_COMMAND_FAILED, (void *) this));
	return -1;
}

void Rts2Command::deleteConnection (Rts2Conn * conn)
{
	if (conn == originator)
		originator = NULL;
}

Rts2CommandSendKey::Rts2CommandSendKey (rts2core::Block * _master, int _centrald_id, int _centrald_num, int _key):Rts2Command (_master)
{
	centrald_id = _centrald_id;
	centrald_num = _centrald_num;
	key = _key;
}

int Rts2CommandSendKey::send ()
{
	std::ostringstream _os;
	_os << "auth " << centrald_id
		<< " " << centrald_num
		<< " " << key;
	setCommand (_os);
	return Rts2Command::send ();
}

Rts2CommandAuthorize::Rts2CommandAuthorize (rts2core::Block * _master, int centralId, int key):Rts2Command (_master)
{
	std::ostringstream _os;
	_os << "authorize " << centralId << " " << key;
	setCommand (_os);
}

Rts2CommandKey::Rts2CommandKey (rts2core::Block * _master, const char *device_name):Rts2Command (_master)
{
 	std::ostringstream _os;
	_os << "key " << device_name;
	setCommand (_os);
}

Rts2CommandCameraSettings::Rts2CommandCameraSettings (Rts2DevClientCamera * _camera):Rts2Command (_camera->getMaster ())
{
}

Rts2CommandExposure::Rts2CommandExposure (rts2core::Block * _master, Rts2DevClientCamera * _camera, int _bopMask):Rts2Command (_master)
{
	setCommand ("expose");
	camera = _camera;
	setBopMask (_bopMask);
}

int Rts2CommandExposure::commandReturnFailed (int status, Rts2Conn * conn)
{
	camera->exposureFailed (status);
	return Rts2Command::commandReturnFailed (status, conn);
}

Rts2CommandReadout::Rts2CommandReadout (rts2core::Block * _master):Rts2Command (_master)
{
	setCommand ("readout");
}

void Rts2CommandFilter::setCommandFilter (int filter)
{
	std::ostringstream _os;
	_os << PROTO_SET_VALUE " filter = " << filter;
	setCommand (_os);
}

Rts2CommandFilter::Rts2CommandFilter (Rts2DevClientCamera * _camera, int filter):Rts2Command (_camera->getMaster ())
{
	camera = _camera;
	phot = NULL;
	filterCli = NULL;
	setCommandFilter (filter);
}

Rts2CommandFilter::Rts2CommandFilter (Rts2DevClientPhot * _phot, int filter):Rts2Command (_phot->getMaster ())
{
	camera = NULL;
	phot = _phot;
	filterCli = NULL;
	setCommandFilter (filter);
}

Rts2CommandFilter::Rts2CommandFilter (Rts2DevClientFilter * _filter, int filter):Rts2Command (_filter->getMaster ())
{
	camera = NULL;
	phot = NULL;
	filterCli = _filter;
	setCommandFilter (filter);
}

int Rts2CommandFilter::commandReturnOK (Rts2Conn * conn)
{
	if (camera)
		camera->filterOK ();
	if (phot)
		phot->filterOK ();
	if (filterCli)
		filterCli->filterOK ();
	return Rts2Command::commandReturnOK (conn);
}

int Rts2CommandFilter::commandReturnFailed (int status, Rts2Conn * conn)
{
	if (camera)
		camera->filterFailed (status);
	if (phot)
		phot->filterMoveFailed (status);
	if (filterCli)
		filterCli->filterMoveFailed (status);
	return Rts2Command::commandReturnFailed (status, conn);
}

Rts2CommandBox::Rts2CommandBox (Rts2DevClientCamera * _camera, int x, int y, int w, int h):Rts2CommandCameraSettings (_camera)
{
	std::ostringstream _os;
	_os << "box " << x << " " << y << " " << w << " " << h;
	setCommand (_os);
}

Rts2CommandCenter::Rts2CommandCenter (Rts2DevClientCamera * _camera, int width = -1, int height = -1):Rts2CommandCameraSettings (_camera)
{
	std::ostringstream _os;
	_os << "center " << width << " " << height;
	setCommand (_os);
}

Rts2CommandChangeValue::Rts2CommandChangeValue (Rts2DevClient * _client, std::string _valName, char op, int _operand):Rts2Command (_client->getMaster ())
{
	std::ostringstream _os;
	_os << PROTO_SET_VALUE " " <<  _valName << " " << op << " " << _operand;
	setCommand (_os);
}

Rts2CommandChangeValue::Rts2CommandChangeValue (Rts2DevClient * _client, std::string _valName, char op, long _operand):Rts2Command (_client->getMaster ())
{
	std::ostringstream _os;
	_os << PROTO_SET_VALUE " " << _valName << " " << op << " " << _operand;
	setCommand (_os);
}

Rts2CommandChangeValue::Rts2CommandChangeValue (Rts2DevClient * _client, std::string _valName, char op, float _operand):Rts2Command (_client->getMaster ())
{
	std::ostringstream _os;
	_os << PROTO_SET_VALUE " " << _valName << " " << op << " " << _operand;
	setCommand (_os);
}

Rts2CommandChangeValue::Rts2CommandChangeValue (Rts2DevClient * _client, std::string _valName, char op, double _operand):Rts2Command (_client->getMaster ())
{
	std::ostringstream _os;
	_os << PROTO_SET_VALUE " " << _valName << " " << op << " " << _operand;
	setCommand (_os);
}

Rts2CommandChangeValue::Rts2CommandChangeValue (Rts2DevClient * _client, std::string _valName, char op, double _operand1, double _operand2):Rts2Command (_client->getMaster ())
{
	std::ostringstream _os;
	_os << PROTO_SET_VALUE " " << _valName << " " << op
		<< " " << _operand1 << " " << _operand2;
	setCommand (_os);
}

Rts2CommandChangeValue::Rts2CommandChangeValue (Rts2DevClient * _client,std::string _valName, char op, bool _operand):Rts2Command (_client->getMaster ())
{
	std::ostringstream _os;
	_os << PROTO_SET_VALUE " " << _valName << " " << op << " " 
		<< (_operand ? 1 : 0);
	setCommand (_os);
}

Rts2CommandChangeValue::Rts2CommandChangeValue (Rts2DevClient * _client,std::string _valName, char op, int x, int y, int w, int h):Rts2Command (_client->getMaster ())
{
	std::ostringstream _os;
	_os << PROTO_SET_VALUE " " << _valName << " " << op
		<< " " << x << " " << y << " " << w << " " << h;
	setCommand (_os);
}

Rts2CommandChangeValue::Rts2CommandChangeValue (Rts2DevClient * _client, std::string _valName, char op, std::string _operand, bool raw):Rts2Command (_client->getMaster ())
{
	std::ostringstream _os;
	if (raw)
	{
		_os << PROTO_SET_VALUE " " << _valName << " " << op
			<< " " << _operand;
	}
	else
	{
		_os << PROTO_SET_VALUE " " << _valName << " " << op
			<< " \"" << _operand << "\"";
	}
	setCommand (_os);
}

Rts2CommandMove::Rts2CommandMove (rts2core::Block * _master, Rts2DevClientTelescope * _tel):Rts2Command (_master)
{
	tel = _tel;
}

Rts2CommandMove::Rts2CommandMove (rts2core::Block * _master, Rts2DevClientTelescope * _tel, double ra, double dec):Rts2Command (_master)
{
	std::ostringstream _os;
	_os << "move " << ra << " " << dec;
	setCommand (_os);
	tel = _tel;
}

int Rts2CommandMove::commandReturnFailed (int status, Rts2Conn * conn)
{
	tel->moveFailed (status);
	return Rts2Command::commandReturnFailed (status, conn);
}

Rts2CommandMoveUnmodelled::Rts2CommandMoveUnmodelled (rts2core::Block * _master, Rts2DevClientTelescope * _tel, double ra, double dec):Rts2CommandMove (_master, _tel)
{
	std::ostringstream _os;
	_os << "move_not_model " << ra << " " << dec;
	setCommand (_os);
}

Rts2CommandMoveFixed::Rts2CommandMoveFixed (rts2core::Block * _master, Rts2DevClientTelescope * _tel, double ra, double dec):Rts2CommandMove (_master,_tel)
{
	std::ostringstream _os;
	_os << "fixed " << ra << " " << dec;
	setCommand (_os);
}

Rts2CommandMoveAltAz::Rts2CommandMoveAltAz (rts2core::Block * _master, Rts2DevClientTelescope * _tel, double alt, double az):Rts2CommandMove (_master, _tel)
{
	std::ostringstream _os;
	_os << "altaz " << alt << " " << az;
	setCommand (_os);
}

Rts2CommandResyncMove::Rts2CommandResyncMove (rts2core::Block * _master, Rts2DevClientTelescope * _tel, double ra, double dec):Rts2CommandMove (_master, _tel)
{
	std::ostringstream _os;
	_os << "resync " << ra << " " << dec;
	setCommand (_os);
}

Rts2CommandChange::Rts2CommandChange (rts2core::Block * _master, double ra, double dec):Rts2Command (_master)
{
	std::ostringstream _os;
	_os << "change " << ra << " " << dec;
	setCommand (_os);
	tel = NULL;
}

Rts2CommandChange::Rts2CommandChange (Rts2DevClientTelescope * _tel, double ra, double dec):Rts2Command (_tel->getMaster ())
{
	std::ostringstream _os;
	_os << "change " << ra << " " << dec;
	setCommand (_os);
	tel = _tel;
}

Rts2CommandChange::Rts2CommandChange (Rts2CommandChange * _command, Rts2DevClientTelescope * _tel):Rts2Command (_command)
{
	tel = _tel;
}

int Rts2CommandChange::commandReturnFailed (int status, Rts2Conn * conn)
{
	if (tel)
		tel->moveFailed (status);
	return Rts2Command::commandReturnFailed (status, conn);
}

Rts2CommandCorrect::Rts2CommandCorrect (rts2core::Block * _master, int corr_mark, int corr_img, int img_id, double ra_corr, double dec_corr, double pos_err):Rts2Command (_master)
{
 	std::ostringstream _os;
	_os << "correct " << corr_mark << " " << corr_img
		<< " " << img_id << " " << ra_corr
		<< " " << dec_corr << " " << pos_err;
	setCommand (_os);
}

Rts2CommandStartGuide::Rts2CommandStartGuide (rts2core::Block * _master, char dir, double dir_dist):Rts2Command (_master)
{
	std::ostringstream _os;
	_os << "start_guide " << dir << " " << dir_dist;
	setCommand (_os);
}

Rts2CommandStopGuide::Rts2CommandStopGuide (rts2core::Block * _master, char dir):Rts2Command (_master)
{
	std::ostringstream _os;
	_os << "stop_guide " << dir;
	setCommand (_os);
}

Rts2CommandCupolaMove::Rts2CommandCupolaMove (Rts2DevClientCupola * _copula, double ra, double dec):Rts2Command (_copula->getMaster ())
{
	std::ostringstream _os;
	copula = _copula;
	_os << "move " << ra << " " << dec;
	setCommand (_os);
}

int Rts2CommandCupolaMove::commandReturnFailed (int status, Rts2Conn * conn)
{
	if (copula)
		copula->syncFailed (status);
	return Rts2Command::commandReturnFailed (status, conn);
}

Rts2CommandCupolaNotMove::Rts2CommandCupolaNotMove (Rts2DevClientCupola * _copula):Rts2Command (_copula->getMaster ())
{
	std::ostringstream _os;
	copula = _copula;
	_os << "stop" ;
	setCommand (_os);
}

int Rts2CommandCupolaNotMove::commandReturnFailed (int status, Rts2Conn * conn)
{
	if (copula)
		copula->notMoveFailed (status);
	return Rts2Command::commandReturnFailed (status, conn);
}

void Rts2CommandChangeFocus::change (int _steps)
{
	std::ostringstream _os;
	_os << "X FOC_TOFFS += " << _steps;
	setCommand (_os);
}

Rts2CommandChangeFocus::Rts2CommandChangeFocus (Rts2DevClientFocus * _focuser, int _steps):Rts2Command (_focuser->getMaster ())
{
	focuser = _focuser;
	camera = NULL;
	change (_steps);
}

Rts2CommandChangeFocus::Rts2CommandChangeFocus (Rts2DevClientCamera * _camera, int _steps):Rts2Command (_camera->getMaster ())
{
	focuser = NULL;
	camera = _camera;
	change (_steps);
}

int Rts2CommandChangeFocus::commandReturnFailed (int status, Rts2Conn * conn)
{
	if (focuser)
		focuser->focusingFailed (status);
	if (camera)
		camera->focuserFailed (status);
	return Rts2Command::commandReturnFailed (status, conn);
}

void Rts2CommandSetFocus::set (int _steps)
{
	std::ostringstream _os;
	_os << "set " << _steps;
	setCommand (_os);
}

Rts2CommandSetFocus::Rts2CommandSetFocus (Rts2DevClientFocus * _focuser, int _steps):Rts2Command (_focuser->getMaster ())
{
	focuser = _focuser;
	camera = NULL;
	set (_steps);
}

Rts2CommandSetFocus::Rts2CommandSetFocus (Rts2DevClientCamera * _camera, int _steps):Rts2Command (_camera->getMaster ())
{
	focuser = NULL;
	camera = _camera;
	set (_steps);
}

int Rts2CommandSetFocus::commandReturnFailed (int status, Rts2Conn * conn)
{
	if (focuser)
		focuser->focusingFailed (status);
	if (camera)
		camera->focuserFailed (status);
	return Rts2Command::commandReturnFailed (status, conn);
}

Rts2CommandMirror::Rts2CommandMirror (Rts2DevClientMirror * _mirror, int _pos):Rts2Command (_mirror->getMaster ())
{
	mirror = _mirror;
	std::ostringstream _os;
	_os << "set " << (_pos == 0 ? "A" : "B");
	setCommand (_os);
}

int Rts2CommandMirror::commandReturnFailed (int status, Rts2Conn * conn)
{
	if (mirror)
		mirror->moveFailed (status);
	return Rts2Command::commandReturnFailed (status, conn);
}

Rts2CommandIntegrate::Rts2CommandIntegrate (Rts2DevClientPhot * _phot, int _filter, float _exp, int _count):Rts2Command (_phot->getMaster ())
{
	phot = _phot;
	std::ostringstream _os;
	_os << "intfil " << _filter << " " << _exp << " " <<_count;
	setCommand (_os);
}

Rts2CommandIntegrate::Rts2CommandIntegrate (Rts2DevClientPhot * _phot, float _exp, int _count):Rts2Command (_phot->getMaster ())
{
	phot = _phot;
	std::ostringstream _os;
	_os << "integrate " << _exp << " " << _count;
	setCommand (_os);
}

int Rts2CommandIntegrate::commandReturnFailed (int status, Rts2Conn * conn)
{
	if (phot)
		phot->integrationFailed (status);
	return Rts2Command::commandReturnFailed (status, conn);
}

Rts2CommandExecNext::Rts2CommandExecNext (rts2core::Block * _master, int next_id):Rts2Command (_master)
{
	std::ostringstream _os;
	_os << "next " << next_id;
	setCommand (_os);
}

Rts2CommandExecNow::Rts2CommandExecNow (rts2core::Block * _master, int now_id):Rts2Command (_master)
{
	std::ostringstream _os;
	_os << "now " << now_id;
	setCommand (_os);
}

Rts2CommandExecGrb::Rts2CommandExecGrb (rts2core::Block * _master, int grb_id):Rts2Command (_master)
{
	std::ostringstream _os;
	_os << "grb " << grb_id;
	setCommand (_os);
}

Rts2CommandExecShower::Rts2CommandExecShower (rts2core::Block * _master):Rts2Command (_master)
{
	setCommand ("shower");
}

Rts2CommandKillAll::Rts2CommandKillAll (rts2core::Block *_master):Rts2Command (_master)
{
	setCommand ("killall");
}

Rts2CommandScriptEnds::Rts2CommandScriptEnds (rts2core::Block *_master):Rts2Command (_master)
{
	setCommand ("script_ends");
}

Rts2CommandMessageMask::Rts2CommandMessageMask (rts2core::Block * _master, int _mask):Rts2Command (_master)
{
	std::ostringstream _os;
	_os << "message_mask " << _mask;
	setCommand (_os);
}

Rts2CommandInfo::Rts2CommandInfo (rts2core::Block * _master):Rts2Command (_master)
{
	setCommand ("info");
}

int Rts2CommandInfo::commandReturnOK (Rts2Conn * conn)
{
	if (connection && connection->getOtherDevClient ())
		connection->getOtherDevClient ()->infoOK ();
	return Rts2Command::commandReturnOK (conn);
}

int Rts2CommandInfo::commandReturnFailed (int status, Rts2Conn * conn)
{
	if (connection && connection->getOtherDevClient ())
		connection->getOtherDevClient ()->infoFailed ();
	return Rts2Command::commandReturnFailed (status, conn);
}

Rts2CommandStatusInfo::Rts2CommandStatusInfo (rts2core::Block * master, Rts2Conn * _control_conn):Rts2Command(master)
{
	control_conn = _control_conn;
	setCommand ("status_info");
}

int Rts2CommandStatusInfo::commandReturnOK (Rts2Conn * conn)
{
	if (control_conn)
		control_conn->updateStatusWait (conn);
	return Rts2Command::commandReturnOK (conn);
}

int Rts2CommandStatusInfo::commandReturnFailed (Rts2Conn * conn)
{
	if (control_conn)
		control_conn->updateStatusWait (conn);
	return Rts2Command::commandReturnOK (conn);
}

void Rts2CommandStatusInfo::deleteConnection (Rts2Conn * conn)
{
	if (control_conn == conn)
		control_conn = NULL;
	Rts2Command::deleteConnection (conn);
}

Rts2CommandDeviceStatus::Rts2CommandDeviceStatus (rts2core::Block * master, Rts2Conn * _control_conn):Rts2CommandStatusInfo (master, _control_conn)
{
	setCommand ("device_status");
}

Rts2CommandObservation::Rts2CommandObservation (rts2core::Block * master, int tar_id, int obs_id):Rts2Command (master)
{
	std::ostringstream _os;
	_os << "observation " << tar_id << " " << obs_id;
	setCommand (_os);
}
