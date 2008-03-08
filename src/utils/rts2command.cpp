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

Rts2Command::Rts2Command (Rts2Block * in_owner)
{
	owner = in_owner;
	text = NULL;
	bopMask = 0;
	originator = NULL;
}


Rts2Command::Rts2Command (Rts2Block * in_owner, char *in_text)
{
	owner = in_owner;
	setCommand (in_text);
	bopMask = 0;
	originator = NULL;
}


Rts2Command::Rts2Command (Rts2Command * in_command)
{
	owner = in_command->owner;
	connection = in_command->connection;
	setCommand (in_command->getText ());
	bopMask = in_command->getBopMask ();
	originator = NULL;
}


int
Rts2Command::setCommand (char *in_text)
{
	text = new char[strlen (in_text) + 1];
	strcpy (text, in_text);
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


Rts2CommandSendKey::Rts2CommandSendKey (Rts2Block * in_master, int in_key):Rts2Command
(in_master)
{
	key = in_key;
}


int
Rts2CommandSendKey::send ()
{
	char *command;
	asprintf (&command, "auth %i %i",
		owner->getCentraldConn ()->getCentraldId (), key);
	setCommand (command);
	free (command);
	return Rts2Command::send ();
}


Rts2CommandAuthorize::Rts2CommandAuthorize (Rts2Block * in_master, int centralId, int key)
:Rts2Command (in_master)
{
	char *buf;
	asprintf (&buf, "authorize %i %i", centralId, key);
	setCommand (buf);
	free (buf);
}


Rts2CommandKey::Rts2CommandKey (Rts2Block * in_master, const char *device_name):Rts2Command
(in_master)
{
	char *command;
	asprintf (&command, "key %s", device_name);
	setCommand (command);
	free (command);
}


Rts2CommandCameraSettings::Rts2CommandCameraSettings (Rts2DevClientCamera * in_camera):Rts2Command (in_camera->
getMaster
())
{
}


Rts2CommandExposure::Rts2CommandExposure (Rts2Block * in_master, Rts2DevClientCamera * in_camera, int in_bopMask):
Rts2Command (in_master)
{
	char *command;
								 //, (exp_type == EXP_LIGHT ? 1 : 0), exp_time);
	asprintf (&command, "expose");
	setCommand (command);
	free (command);
	camera = in_camera;
	setBopMask (in_bopMask);
}


int
Rts2CommandExposure::commandReturnFailed (int status, Rts2Conn * conn)
{
	camera->exposureFailed (status);
	return Rts2Command::commandReturnFailed (status, conn);
}


Rts2CommandReadout::Rts2CommandReadout (Rts2Block * in_master):
Rts2Command (in_master)
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


Rts2CommandFilter::Rts2CommandFilter (Rts2DevClientCamera * in_camera, int filter):
Rts2Command (in_camera->getMaster ())
{
	camera = in_camera;
	phot = NULL;
	filterCli = NULL;
	setCommandFilter (filter);
}


Rts2CommandFilter::Rts2CommandFilter (Rts2DevClientPhot * in_phot, int filter):
Rts2Command (in_phot->getMaster ())
{
	camera = NULL;
	phot = in_phot;
	filterCli = NULL;
	setCommandFilter (filter);
}


Rts2CommandFilter::Rts2CommandFilter (Rts2DevClientFilter * in_filter, int filter):
Rts2Command (in_filter->getMaster ())
{
	camera = NULL;
	phot = NULL;
	filterCli = in_filter;
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


Rts2CommandBox::Rts2CommandBox (Rts2DevClientCamera * in_camera, int x, int y, int w, int h):
Rts2CommandCameraSettings (in_camera)
{
	char *command;
	asprintf (&command, "box %i %i %i %i", x, y, w, h);
	setCommand (command);
	free (command);
}


Rts2CommandCenter::Rts2CommandCenter (Rts2DevClientCamera * in_camera, int width = -1, int height = -1):
Rts2CommandCameraSettings (in_camera)
{
	char *command;
	asprintf (&command, "center %i %i", width, height);
	setCommand (command);
	free (command);
}


Rts2CommandChangeValue::Rts2CommandChangeValue (Rts2DevClient * in_client, std::string in_valName, char op, int in_operand):
Rts2Command (in_client->getMaster ())
{
	char *command;
	asprintf (&command, PROTO_SET_VALUE " %s %c %i", in_valName.c_str (), op,
		in_operand);
	setCommand (command);
	free (command);
}


Rts2CommandChangeValue::Rts2CommandChangeValue (Rts2DevClient * in_client, std::string in_valName, char op, long in_operand):
Rts2Command (in_client->getMaster ())
{
	char *command;
	asprintf (&command, PROTO_SET_VALUE " %s %c %li", in_valName.c_str (), op,
		in_operand);
	setCommand (command);
	free (command);
}


Rts2CommandChangeValue::Rts2CommandChangeValue (Rts2DevClient * in_client, std::string in_valName, char op, float in_operand):
Rts2Command (in_client->getMaster ())
{
	char *command;
	asprintf (&command, PROTO_SET_VALUE " %s %c %f", in_valName.c_str (), op,
		in_operand);
	setCommand (command);
	free (command);
}


Rts2CommandChangeValue::Rts2CommandChangeValue (Rts2DevClient * in_client, std::string in_valName, char op, double in_operand):
Rts2Command (in_client->getMaster ())
{
	char *command;
	asprintf (&command, PROTO_SET_VALUE " %s %c %f", in_valName.c_str (), op,
		in_operand);
	setCommand (command);
	free (command);
}


Rts2CommandChangeValue::Rts2CommandChangeValue (Rts2DevClient * in_client,std::string in_valName, char op, bool in_operand):
Rts2Command (in_client->getMaster ())
{
	char *command;
	asprintf (&command, PROTO_SET_VALUE " %s %c %i", in_valName.c_str (), op,
		in_operand ? 2 : 1);
	setCommand (command);
	free (command);
}


Rts2CommandChangeValue::Rts2CommandChangeValue (Rts2DevClient * in_client, std::string in_valName, char op, std::string in_operand, bool raw):
Rts2Command (in_client->getMaster ())
{
	char *command;
	if (raw)
	{
		asprintf (&command, PROTO_SET_VALUE " %s %c %s", in_valName.c_str (),
			op, in_operand.c_str ());
	}
	else
	{
		asprintf (&command, PROTO_SET_VALUE " %s %c \"%s\"", in_valName.c_str (),
			op, in_operand.c_str ());
	}
	setCommand (command);
	free (command);
}


Rts2CommandMove::Rts2CommandMove (Rts2Block * in_master, Rts2DevClientTelescope * in_tel):
Rts2Command (in_master)
{
	tel = in_tel;
}


Rts2CommandMove::Rts2CommandMove (Rts2Block * in_master,
Rts2DevClientTelescope * in_tel, double ra,
double dec):
Rts2Command (in_master)
{
	char *command;
	asprintf (&command, "move %lf %lf", ra, dec);
	setCommand (command);
	free (command);
	tel = in_tel;
}


int
Rts2CommandMove::commandReturnFailed (int status, Rts2Conn * conn)
{
	tel->moveFailed (status);
	return Rts2Command::commandReturnFailed (status, conn);
}


Rts2CommandMoveUnmodelled::Rts2CommandMoveUnmodelled (Rts2Block * in_master, Rts2DevClientTelescope * in_tel, double ra, double dec):
Rts2CommandMove (in_master, in_tel)
{
	char *command;
	asprintf (&command, "move_not_model %lf %lf", ra, dec);
	setCommand (command);
	free (command);
}


Rts2CommandMoveFixed::Rts2CommandMoveFixed (Rts2Block * in_master,
Rts2DevClientTelescope * in_tel,
double ra, double dec):
Rts2Command (in_master)
{
	char *command;
	asprintf (&command, "fixed %lf %lf", ra, dec);
	setCommand (command);
	free (command);
	tel = in_tel;
}


int
Rts2CommandMoveFixed::commandReturnFailed (int status, Rts2Conn * conn)
{
	tel->moveFailed (status);
	return Rts2Command::commandReturnFailed (status, conn);
}


Rts2CommandResyncMove::Rts2CommandResyncMove (Rts2Block * in_master, Rts2DevClientTelescope * in_tel, double ra, double dec):
Rts2Command (in_master)
{
	char *command;
	asprintf (&command, "resync %lf %lf", ra, dec);
	setCommand (command);
	free (command);
	tel = in_tel;
}


int
Rts2CommandResyncMove::commandReturnFailed (int status, Rts2Conn * conn)
{
	tel->moveFailed (status);
	return Rts2Command::commandReturnFailed (status, conn);
}


Rts2CommandSearch::Rts2CommandSearch (Rts2DevClientTelescope * in_tel, double searchRadius, double searchSpeed):Rts2Command (in_tel->
getMaster
())
{
	char *
		command;
	asprintf (&command, "search %lf %lf", searchRadius, searchSpeed);
	setCommand (command);
	free (command);
	tel = in_tel;
}


int
Rts2CommandSearch::commandReturnFailed (int status, Rts2Conn * conn)
{
	tel->searchFailed (status);
	return Rts2Command::commandReturnFailed (status, conn);
}


Rts2CommandSearchStop::Rts2CommandSearchStop (Rts2DevClientTelescope * in_tel):Rts2Command (in_tel->getMaster (),
"searchstop")
{
}


Rts2CommandChange::Rts2CommandChange (Rts2Block * in_master, double ra, double dec):
Rts2Command (in_master)
{
	char *command;
	asprintf (&command, "change %lf %lf", ra, dec);
	setCommand (command);
	free (command);
	tel = NULL;
}


Rts2CommandChange::Rts2CommandChange (Rts2DevClientTelescope * in_tel,
double ra, double dec):
Rts2Command (in_tel->getMaster ())
{
	char *command;
	asprintf (&command, "change %lf %lf", ra, dec);
	setCommand (command);
	free (command);
	tel = in_tel;
}


Rts2CommandChange::Rts2CommandChange (Rts2CommandChange * in_command, Rts2DevClientTelescope * in_tel):Rts2Command
(in_command)
{
	tel = in_tel;
}


int
Rts2CommandChange::commandReturnFailed (int status, Rts2Conn * conn)
{
	if (tel)
		tel->moveFailed (status);
	return Rts2Command::commandReturnFailed (status, conn);
}


Rts2CommandCorrect::Rts2CommandCorrect (Rts2Block * in_master, int corr_mark,
int corr_img, int img_id, double ra_corr, double dec_corr, double pos_err):
Rts2Command (in_master)
{
	char *command;
	asprintf (&command, "correct %i %i %i %lf %lf %lf", corr_mark, corr_img,
		img_id, ra_corr, dec_corr, pos_err);
	setCommand (command);
	free (command);
}


Rts2CommandStartGuide::Rts2CommandStartGuide (Rts2Block * in_master, char dir,
double dir_dist):
Rts2Command (in_master)
{
	char *command;
	asprintf (&command, "start_guide %c %lf", dir, dir_dist);
	setCommand (command);
	free (command);
}


Rts2CommandStopGuide::Rts2CommandStopGuide (Rts2Block * in_master, char dir):
Rts2Command (in_master)
{
	char *command;
	asprintf (&command, "stop_guide %c", dir);
	setCommand (command);
	free (command);
}


Rts2CommandCupolaMove::Rts2CommandCupolaMove (Rts2DevClientCupola * in_copula,
double ra, double dec):
Rts2Command (in_copula->getMaster ())
{
	char *msg;
	copula = in_copula;
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
Rts2CommandChangeFocus::change (int in_steps)
{
	char *msg;
	asprintf (&msg, "step %i", in_steps);
	setCommand (msg);
	free (msg);
}


Rts2CommandChangeFocus::Rts2CommandChangeFocus (Rts2DevClientFocus *
in_focuser, int in_steps):
Rts2Command (in_focuser->getMaster ())
{
	focuser = in_focuser;
	camera = NULL;
	change (in_steps);
}


Rts2CommandChangeFocus::Rts2CommandChangeFocus (Rts2DevClientCamera *
in_camera, int in_steps):
Rts2Command (in_camera->getMaster ())
{
	focuser = NULL;
	camera = in_camera;
	change (in_steps);
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
Rts2CommandSetFocus::set (int in_steps)
{
	char *msg;
	asprintf (&msg, "set %i", in_steps);
	setCommand (msg);
	free (msg);
}


Rts2CommandSetFocus::Rts2CommandSetFocus (Rts2DevClientFocus * in_focuser,
int in_steps):
Rts2Command (in_focuser->getMaster ())
{
	focuser = in_focuser;
	camera = NULL;
	set (in_steps);
}


Rts2CommandSetFocus::Rts2CommandSetFocus (Rts2DevClientCamera * in_camera,
int in_steps):
Rts2Command (in_camera->getMaster ())
{
	focuser = NULL;
	camera = in_camera;
	set (in_steps);
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


Rts2CommandMirror::Rts2CommandMirror (Rts2DevClientMirror * in_mirror, int in_pos):Rts2Command (in_mirror->
getMaster
())
{
	mirror = in_mirror;
	char *
		msg;
	asprintf (&msg, "set %c", (in_pos == 0 ? 'A' : 'B'));
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


Rts2CommandIntegrate::Rts2CommandIntegrate (Rts2DevClientPhot * in_phot, int in_filter, float in_exp, int in_count):Rts2Command (in_phot->
getMaster
())
{
	phot = in_phot;
	char *
		msg;
	asprintf (&msg, "intfil %i %f %i", in_filter, in_exp, in_count);
	setCommand (msg);
	free (msg);
}


Rts2CommandIntegrate::Rts2CommandIntegrate (Rts2DevClientPhot * in_phot,
float in_exp, int in_count):
Rts2Command (in_phot->getMaster ())
{
	phot = in_phot;
	char *msg;
	asprintf (&msg, "integrate %f %i", in_exp, in_count);
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


Rts2CommandExecNext::Rts2CommandExecNext (Rts2Block * in_master, int next_id):
Rts2Command (in_master)
{
	char *command;
	asprintf (&command, "next %i", next_id);
	setCommand (command);
	free (command);
}


Rts2CommandExecNow::Rts2CommandExecNow (Rts2Block * in_master, int now_id):
Rts2Command (in_master)
{
	char *command;
	asprintf (&command, "now %i", now_id);
	setCommand (command);
	free (command);
}


Rts2CommandExecGrb::Rts2CommandExecGrb (Rts2Block * in_master, int grb_id):
Rts2Command (in_master)
{
	char *command;
	asprintf (&command, "grb %i", grb_id);
	setCommand (command);
	free (command);
}


Rts2CommandExecShower::Rts2CommandExecShower (Rts2Block * in_master):
Rts2Command (in_master)
{
	setCommand ("shower");
}


Rts2CommandKillAll::Rts2CommandKillAll (Rts2Block * in_master):Rts2Command
(in_master)
{
	setCommand ("killall");
}


Rts2CommandScriptEnds::Rts2CommandScriptEnds (Rts2Block * in_master):Rts2Command
(in_master)
{
	setCommand ("script_ends");
}


Rts2CommandMessageMask::Rts2CommandMessageMask (Rts2Block * in_master,
int in_mask):
Rts2Command (in_master)
{
	char *command;
	asprintf (&command, "message_mask %i", in_mask);
	setCommand (command);
	free (command);
}


Rts2CommandInfo::Rts2CommandInfo (Rts2Block * in_master):Rts2Command
(in_master)
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


Rts2CommandStatusInfo::Rts2CommandStatusInfo (Rts2Block * master, Rts2Conn * in_control_conn)
:Rts2Command(master)
{
	control_conn = in_control_conn;
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


Rts2CommandDeviceStatus::Rts2CommandDeviceStatus (Rts2Block * master, Rts2Conn * in_control_conn)
:Rts2CommandStatusInfo (master, in_control_conn)
{
	setCommand ("device_status");
}
