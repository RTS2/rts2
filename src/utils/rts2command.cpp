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

Rts2Command::Rts2Command (Rts2Block * _owner)
{
	owner = _owner;
	text = NULL;
	bopMask = 0;
	originator = NULL;
}


Rts2Command::Rts2Command (Rts2Block * _owner, const char *_text)
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


int
Rts2Command::setCommand (const char *_text)
{
	text = new char[strlen (_text) + 1];
	strcpy (text, _text);
	return 0;
}


Rts2Command::~Rts2Command (void)
{
	delete[]text;
}


int
Rts2Command::send ()
{
	return connection->sendMsg (text);
}


int
Rts2Command::commandReturn (int status, Rts2Conn * conn)
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


int
Rts2Command::commandReturnOK (Rts2Conn * conn)
{
	if (originator)
		originator->postEvent (new Rts2Event (EVENT_COMMAND_OK, (void *) this));
	return -1;
}


int
Rts2Command::commandReturnQued (Rts2Conn * conn)
{
	if (originator)
		originator->postEvent (new Rts2Event (EVENT_COMMAND_OK, (void *) this));
	return commandReturnOK (conn);
}


int
Rts2Command::commandReturnFailed (int status, Rts2Conn * conn)
{
	if (originator)
		originator->
			postEvent (new Rts2Event (EVENT_COMMAND_FAILED, (void *) this));
	return -1;
}


void
Rts2Command::deleteConnection (Rts2Conn * conn)
{
	if (conn == originator)
		originator = NULL;
}


Rts2CommandSendKey::Rts2CommandSendKey (Rts2Block * _master, int _centrald_id, int _centrald_num, int _key)
:Rts2Command (_master)
{
	centrald_id = _centrald_id;
	centrald_num = _centrald_num;
	key = _key;
}


int
Rts2CommandSendKey::send ()
{
	char *command;
	asprintf (&command, "auth %i %i %i",
		centrald_id, centrald_num, key);
	setCommand (command);
	free (command);
	return Rts2Command::send ();
}


Rts2CommandAuthorize::Rts2CommandAuthorize (Rts2Block * _master, int centralId, int key)
:Rts2Command (_master)
{
	char *buf;
	asprintf (&buf, "authorize %i %i", centralId, key);
	setCommand (buf);
	free (buf);
}


Rts2CommandKey::Rts2CommandKey (Rts2Block * _master, const char *device_name)
:Rts2Command (_master)
{
	char *command;
	asprintf (&command, "key %s", device_name);
	setCommand (command);
	free (command);
}


Rts2CommandCameraSettings::Rts2CommandCameraSettings (Rts2DevClientCamera * _camera)
:Rts2Command (_camera->getMaster ())
{
}


Rts2CommandExposure::Rts2CommandExposure (Rts2Block * _master, Rts2DevClientCamera * _camera, int _bopMask)
:Rts2Command (_master)
{
	char *command;
								 //, (exp_type == EXP_LIGHT ? 1 : 0), exp_time);
	asprintf (&command, "expose");
	setCommand (command);
	free (command);
	camera = _camera;
	setBopMask (_bopMask);
}


int
Rts2CommandExposure::commandReturnFailed (int status, Rts2Conn * conn)
{
	camera->exposureFailed (status);
	return Rts2Command::commandReturnFailed (status, conn);
}


Rts2CommandReadout::Rts2CommandReadout (Rts2Block * _master):
Rts2Command (_master)
{
	setCommand ("readout");
}


void
Rts2CommandFilter::setCommandFilter (int filter)
{
	char *command;
	asprintf (&command, PROTO_SET_VALUE " filter = %i", filter);
	setCommand (command);
	free (command);
}


Rts2CommandFilter::Rts2CommandFilter (Rts2DevClientCamera * _camera, int filter)
:Rts2Command (_camera->getMaster ())
{
	camera = _camera;
	phot = NULL;
	filterCli = NULL;
	setCommandFilter (filter);
}


Rts2CommandFilter::Rts2CommandFilter (Rts2DevClientPhot * _phot, int filter)
:Rts2Command (_phot->getMaster ())
{
	camera = NULL;
	phot = _phot;
	filterCli = NULL;
	setCommandFilter (filter);
}


Rts2CommandFilter::Rts2CommandFilter (Rts2DevClientFilter * _filter, int filter)
:Rts2Command (_filter->getMaster ())
{
	camera = NULL;
	phot = NULL;
	filterCli = _filter;
	setCommandFilter (filter);
}


int
Rts2CommandFilter::commandReturnOK (Rts2Conn * conn)
{
	if (camera)
		camera->filterOK ();
	if (phot)
		phot->filterOK ();
	if (filterCli)
		filterCli->filterOK ();
	return Rts2Command::commandReturnOK (conn);
}


int
Rts2CommandFilter::commandReturnFailed (int status, Rts2Conn * conn)
{
	if (camera)
		camera->filterFailed (status);
	if (phot)
		phot->filterMoveFailed (status);
	if (filterCli)
		filterCli->filterMoveFailed (status);
	return Rts2Command::commandReturnFailed (status, conn);
}


Rts2CommandBox::Rts2CommandBox (Rts2DevClientCamera * _camera, int x, int y, int w, int h):
Rts2CommandCameraSettings (_camera)
{
	char *command;
	asprintf (&command, "box %i %i %i %i", x, y, w, h);
	setCommand (command);
	free (command);
}


Rts2CommandCenter::Rts2CommandCenter (Rts2DevClientCamera * _camera, int width = -1, int height = -1)
:Rts2CommandCameraSettings (_camera)
{
	char *command;
	asprintf (&command, "center %i %i", width, height);
	setCommand (command);
	free (command);
}


Rts2CommandChangeValue::Rts2CommandChangeValue (Rts2DevClient * _client, std::string _valName, char op, int _operand)
:Rts2Command (_client->getMaster ())
{
	char *command;
	asprintf (&command, PROTO_SET_VALUE " %s %c %i", _valName.c_str (), op,
		_operand);
	setCommand (command);
	free (command);
}


Rts2CommandChangeValue::Rts2CommandChangeValue (Rts2DevClient * _client, std::string _valName, char op, long _operand)
:Rts2Command (_client->getMaster ())
{
	char *command;
	asprintf (&command, PROTO_SET_VALUE " %s %c %li", _valName.c_str (), op,
		_operand);
	setCommand (command);
	free (command);
}


Rts2CommandChangeValue::Rts2CommandChangeValue (Rts2DevClient * _client, std::string _valName, char op, float _operand)
:Rts2Command (_client->getMaster ())
{
	char *command;
	asprintf (&command, PROTO_SET_VALUE " %s %c %f", _valName.c_str (), op,
		_operand);
	setCommand (command);
	free (command);
}


Rts2CommandChangeValue::Rts2CommandChangeValue (Rts2DevClient * _client, std::string _valName, char op, double _operand)
:Rts2Command (_client->getMaster ())
{
	char *command;
	asprintf (&command, PROTO_SET_VALUE " %s %c %f", _valName.c_str (), op,
		_operand);
	setCommand (command);
	free (command);
}


Rts2CommandChangeValue::Rts2CommandChangeValue (Rts2DevClient * _client,std::string _valName, char op, bool _operand)
:Rts2Command (_client->getMaster ())
{
	char *command;
	asprintf (&command, PROTO_SET_VALUE " %s %c %i", _valName.c_str (), op,
		_operand ? 1 : 0);
	setCommand (command);
	free (command);
}


Rts2CommandChangeValue::Rts2CommandChangeValue (Rts2DevClient * _client,std::string _valName, char op, int x, int y, int w, int h)
:Rts2Command (_client->getMaster ())
{
	char *command;
	asprintf (&command, PROTO_SET_VALUE " %s %c %i %i %i %i", _valName.c_str (), op,
		x, y, w, h);
	setCommand (command);
	free (command);
}


Rts2CommandChangeValue::Rts2CommandChangeValue (Rts2DevClient * _client, std::string _valName, char op, std::string _operand, bool raw)
:Rts2Command (_client->getMaster ())
{
	char *command;
	if (raw)
	{
		asprintf (&command, PROTO_SET_VALUE " %s %c %s", _valName.c_str (),
			op, _operand.c_str ());
	}
	else
	{
		asprintf (&command, PROTO_SET_VALUE " %s %c \"%s\"", _valName.c_str (),
			op, _operand.c_str ());
	}
	setCommand (command);
	free (command);
}


Rts2CommandMove::Rts2CommandMove (Rts2Block * _master, Rts2DevClientTelescope * _tel)
:Rts2Command (_master)
{
	tel = _tel;
}


Rts2CommandMove::Rts2CommandMove (Rts2Block * _master, Rts2DevClientTelescope * _tel, double ra, double dec)
:Rts2Command (_master)
{
	char *command;
	asprintf (&command, "move %lf %lf", ra, dec);
	setCommand (command);
	free (command);
	tel = _tel;
}


int
Rts2CommandMove::commandReturnFailed (int status, Rts2Conn * conn)
{
	tel->moveFailed (status);
	return Rts2Command::commandReturnFailed (status, conn);
}


Rts2CommandMoveUnmodelled::Rts2CommandMoveUnmodelled (Rts2Block * _master, Rts2DevClientTelescope * _tel, double ra, double dec)
:Rts2CommandMove (_master, _tel)
{
	char *command;
	asprintf (&command, "move_not_model %lf %lf", ra, dec);
	setCommand (command);
	free (command);
}


Rts2CommandMoveFixed::Rts2CommandMoveFixed (Rts2Block * _master, Rts2DevClientTelescope * _tel,
double ra, double dec)
:Rts2Command (_master)
{
	char *command;
	asprintf (&command, "fixed %lf %lf", ra, dec);
	setCommand (command);
	free (command);
	tel = _tel;
}


int
Rts2CommandMoveFixed::commandReturnFailed (int status, Rts2Conn * conn)
{
	tel->moveFailed (status);
	return Rts2Command::commandReturnFailed (status, conn);
}


Rts2CommandResyncMove::Rts2CommandResyncMove (Rts2Block * _master, Rts2DevClientTelescope * _tel, double ra, double dec)
:Rts2Command (_master)
{
	char *command;
	asprintf (&command, "resync %lf %lf", ra, dec);
	setCommand (command);
	free (command);
	tel = _tel;
}


int
Rts2CommandResyncMove::commandReturnFailed (int status, Rts2Conn * conn)
{
	tel->moveFailed (status);
	return Rts2Command::commandReturnFailed (status, conn);
}


Rts2CommandSearch::Rts2CommandSearch (Rts2DevClientTelescope * _tel, double searchRadius, double searchSpeed)
:Rts2Command (_tel->getMaster ())
{
	char *
		command;
	asprintf (&command, "search %lf %lf", searchRadius, searchSpeed);
	setCommand (command);
	free (command);
	tel = _tel;
}


int
Rts2CommandSearch::commandReturnFailed (int status, Rts2Conn * conn)
{
	tel->searchFailed (status);
	return Rts2Command::commandReturnFailed (status, conn);
}


Rts2CommandSearchStop::Rts2CommandSearchStop (Rts2DevClientTelescope * _tel)
:Rts2Command (_tel->getMaster (), "searchstop")
{
}


Rts2CommandChange::Rts2CommandChange (Rts2Block * _master, double ra, double dec)
:Rts2Command (_master)
{
	char *command;
	asprintf (&command, "change %lf %lf", ra, dec);
	setCommand (command);
	free (command);
	tel = NULL;
}


Rts2CommandChange::Rts2CommandChange (Rts2DevClientTelescope * _tel,
double ra, double dec)
:Rts2Command (_tel->getMaster ())
{
	char *command;
	asprintf (&command, "change %lf %lf", ra, dec);
	setCommand (command);
	free (command);
	tel = _tel;
}


Rts2CommandChange::Rts2CommandChange (Rts2CommandChange * _command, Rts2DevClientTelescope * _tel)
:Rts2Command (_command)
{
	tel = _tel;
}


int
Rts2CommandChange::commandReturnFailed (int status, Rts2Conn * conn)
{
	if (tel)
		tel->moveFailed (status);
	return Rts2Command::commandReturnFailed (status, conn);
}


Rts2CommandCorrect::Rts2CommandCorrect (Rts2Block * _master, int corr_mark, int corr_img, int img_id, double ra_corr, double dec_corr, double pos_err)
:Rts2Command (_master)
{
	char *command;
	asprintf (&command, "correct %i %i %i %lf %lf %lf", corr_mark, corr_img,
		img_id, ra_corr, dec_corr, pos_err);
	setCommand (command);
	free (command);
}


Rts2CommandStartGuide::Rts2CommandStartGuide (Rts2Block * _master, char dir, double dir_dist)
:Rts2Command (_master)
{
	char *command;
	asprintf (&command, "start_guide %c %lf", dir, dir_dist);
	setCommand (command);
	free (command);
}


Rts2CommandStopGuide::Rts2CommandStopGuide (Rts2Block * _master, char dir)
:Rts2Command (_master)
{
	char *command;
	asprintf (&command, "stop_guide %c", dir);
	setCommand (command);
	free (command);
}


Rts2CommandCupolaMove::Rts2CommandCupolaMove (Rts2DevClientCupola * _copula, double ra, double dec)
:Rts2Command (_copula->getMaster ())
{
	char *msg;
	copula = _copula;
	asprintf (&msg, "move %f %f", ra, dec);
	setCommand (msg);
	free (msg);
}


int
Rts2CommandCupolaMove::commandReturnFailed (int status, Rts2Conn * conn)
{
	if (copula)
		copula->syncFailed (status);
	return Rts2Command::commandReturnFailed (status, conn);
}


void
Rts2CommandChangeFocus::change (int _steps)
{
	char *msg;
	asprintf (&msg, "step %i", _steps);
	setCommand (msg);
	free (msg);
}


Rts2CommandChangeFocus::Rts2CommandChangeFocus (Rts2DevClientFocus * _focuser, int _steps)
:Rts2Command (_focuser->getMaster ())
{
	focuser = _focuser;
	camera = NULL;
	change (_steps);
}


Rts2CommandChangeFocus::Rts2CommandChangeFocus (Rts2DevClientCamera * _camera, int _steps)
:Rts2Command (_camera->getMaster ())
{
	focuser = NULL;
	camera = _camera;
	change (_steps);
}


int
Rts2CommandChangeFocus::commandReturnFailed (int status, Rts2Conn * conn)
{
	if (focuser)
		focuser->focusingFailed (status);
	if (camera)
		camera->focuserFailed (status);
	return Rts2Command::commandReturnFailed (status, conn);
}


void
Rts2CommandSetFocus::set (int _steps)
{
	char *msg;
	asprintf (&msg, "set %i", _steps);
	setCommand (msg);
	free (msg);
}


Rts2CommandSetFocus::Rts2CommandSetFocus (Rts2DevClientFocus * _focuser, int _steps)
:Rts2Command (_focuser->getMaster ())
{
	focuser = _focuser;
	camera = NULL;
	set (_steps);
}


Rts2CommandSetFocus::Rts2CommandSetFocus (Rts2DevClientCamera * _camera, int _steps)
:Rts2Command (_camera->getMaster ())
{
	focuser = NULL;
	camera = _camera;
	set (_steps);
}


int
Rts2CommandSetFocus::commandReturnFailed (int status, Rts2Conn * conn)
{
	if (focuser)
		focuser->focusingFailed (status);
	if (camera)
		camera->focuserFailed (status);
	return Rts2Command::commandReturnFailed (status, conn);
}


Rts2CommandMirror::Rts2CommandMirror (Rts2DevClientMirror * _mirror, int _pos)
:Rts2Command (_mirror->getMaster ())
{
	mirror = _mirror;
	char *
		msg;
	asprintf (&msg, "set %c", (_pos == 0 ? 'A' : 'B'));
	setCommand (msg);
	free (msg);
}


int
Rts2CommandMirror::commandReturnFailed (int status, Rts2Conn * conn)
{
	if (mirror)
		mirror->moveFailed (status);
	return Rts2Command::commandReturnFailed (status, conn);
}


Rts2CommandIntegrate::Rts2CommandIntegrate (Rts2DevClientPhot * _phot, int _filter, float _exp, int _count)
:Rts2Command (_phot->getMaster ())
{
	phot = _phot;
	char *
		msg;
	asprintf (&msg, "intfil %i %f %i", _filter, _exp, _count);
	setCommand (msg);
	free (msg);
}


Rts2CommandIntegrate::Rts2CommandIntegrate (Rts2DevClientPhot * _phot, float _exp, int _count)
:Rts2Command (_phot->getMaster ())
{
	phot = _phot;
	char *msg;
	asprintf (&msg, "integrate %f %i", _exp, _count);
	setCommand (msg);
	free (msg);
}


int
Rts2CommandIntegrate::commandReturnFailed (int status, Rts2Conn * conn)
{
	if (phot)
		phot->integrationFailed (status);
	return Rts2Command::commandReturnFailed (status, conn);
}


Rts2CommandExecNext::Rts2CommandExecNext (Rts2Block * _master, int next_id)
:Rts2Command (_master)
{
	char *command;
	asprintf (&command, "next %i", next_id);
	setCommand (command);
	free (command);
}


Rts2CommandExecNow::Rts2CommandExecNow (Rts2Block * _master, int now_id)
:Rts2Command (_master)
{
	char *command;
	asprintf (&command, "now %i", now_id);
	setCommand (command);
	free (command);
}


Rts2CommandExecGrb::Rts2CommandExecGrb (Rts2Block * _master, int grb_id)
:Rts2Command (_master)
{
	char *command;
	asprintf (&command, "grb %i", grb_id);
	setCommand (command);
	free (command);
}


Rts2CommandExecShower::Rts2CommandExecShower (Rts2Block * _master)
:Rts2Command (_master)
{
	setCommand ("shower");
}


Rts2CommandKillAll::Rts2CommandKillAll (Rts2Block *_master)
:Rts2Command (_master)
{
	setCommand ("killall");
}


Rts2CommandScriptEnds::Rts2CommandScriptEnds (Rts2Block *_master)
:Rts2Command (_master)
{
	setCommand ("script_ends");
}


Rts2CommandMessageMask::Rts2CommandMessageMask (Rts2Block * _master, int _mask)
:Rts2Command (_master)
{
	char *command;
	asprintf (&command, "message_mask %i", _mask);
	setCommand (command);
	free (command);
}


Rts2CommandInfo::Rts2CommandInfo (Rts2Block * _master)
:Rts2Command (_master)
{
	setCommand ("info");
}


int
Rts2CommandInfo::commandReturnOK (Rts2Conn * conn)
{
	if (connection && connection->getOtherDevClient ())
		connection->getOtherDevClient ()->infoOK ();
	return Rts2Command::commandReturnOK (conn);
}


int
Rts2CommandInfo::commandReturnFailed (int status, Rts2Conn * conn)
{
	if (connection && connection->getOtherDevClient ())
		connection->getOtherDevClient ()->infoFailed ();
	return Rts2Command::commandReturnFailed (status, conn);
}


Rts2CommandStatusInfo::Rts2CommandStatusInfo (Rts2Block * master, Rts2Conn * _control_conn)
:Rts2Command(master)
{
	control_conn = _control_conn;
	setCommand ("status_info");
}


int
Rts2CommandStatusInfo::commandReturnOK (Rts2Conn * conn)
{
	if (control_conn)
		control_conn->updateStatusWait (conn);
	return Rts2Command::commandReturnOK (conn);
}


int
Rts2CommandStatusInfo::commandReturnFailed (Rts2Conn * conn)
{
	if (control_conn)
		control_conn->updateStatusWait (conn);
	return Rts2Command::commandReturnOK (conn);
}


void
Rts2CommandStatusInfo::deleteConnection (Rts2Conn * conn)
{
	if (control_conn == conn)
		control_conn = NULL;
	Rts2Command::deleteConnection (conn);
}


Rts2CommandDeviceStatus::Rts2CommandDeviceStatus (Rts2Block * master, Rts2Conn * _control_conn)
:Rts2CommandStatusInfo (master, _control_conn)
{
	setCommand ("device_status");
}
