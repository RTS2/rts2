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

#ifndef __RTS2_COMMAND__
#define __RTS2_COMMAND__

#include "rts2block.h"

/**
 * @defgroup RTS2Command Command classes
 */

/** Send command again to device. @ingroup RTS2Command */
#define RTS2_COMMAND_REQUE      -5

/**
 * Miscelanus flag for sending command while exposure is in progress.
 */
#define BOP_WHILE_STATE         0x00001000

/**
 * Call-in-progress mask
 */
#define BOP_CIP_MASK            0x00006000

/**
 * Defines CIP (Command In Progress) states. Commands which waits on component or RTS2
 * to reach given state uses this to control their execution.
 */
enum cip_state_t
{
	CIP_NOT_CALLED = 0x00000000, //! The command does not use CIP.
	CIP_WAIT       = 0x00002000, //! The command is waiting for status update.
	CIP_RUN        = 0x00004000, //! The status update have run, but does not fullfill command request. Command is still waiting for correct state.
	CIP_RETURN     = 0x00006000	 //! Command is waiting for return from device status wait.
};

namespace rts2core
{

/**
 * Base class which represents commands send over network to other component.
 * This object is usually send through Rts2Conn::queCommand to connection,
 * which process it, wait for the other side to reply, pass return code to
 * Rts2Conn::commandReturn callback and delete it.
 *
 * @see Rts2Conn
 *
 * @ingroup RTS2Block
 * @ingroup RTS2Command
 */
class Rts2Command
{
	public:
		Rts2Command (Rts2Block * _owner);
		Rts2Command (Rts2Block * _owner, const char *_text);
		Rts2Command (Rts2Command * _command);
		Rts2Command (Rts2Command & _command);
		virtual ~ Rts2Command (void);

		/**
		 * Set command for this command object.
		 *
		 * @param _os Ostring stream which will be used to set command.
		 */
		void setCommand (std::ostringstream &_os);

		/**
		 * Set command for this command object.
		 *
		 * @param _text Command text.
		 */
		void setCommand (const char * _text);

		void setConnection (Rts2Conn * conn) { connection = conn; }

		Rts2Conn * getConnection () { return connection; }

		virtual int send ();
		int commandReturn (int status, Rts2Conn * conn);
		/**
		 * Returns command test.
		 */
		char * getText () { return text; }

		/**
		 * Set command Block of OPeration mask.
		 *
		 * @param _bopMask New BOP mask.
		 */
		void setBopMask (int _bopMask) { bopMask = _bopMask; }

		/**
		 * Return command BOP mask.
		 *
		 * @see Rts2Command::setBopMask
		 *
		 * @return Commmand BOP mask.
		 */
		int getBopMask () { return bopMask; }

		/**
		 * Set call originator.
		 *
		 * @param _originator Call originator. Call originator is issued
		 *   EVENT_COMMAND_OK or EVENT_COMMAND_FAILED event.
		 *
		 * @see Rts2Conn::queCommand
		 *
		 * @callergraph
		 */
		void setOriginator (Rts2Object * _originator) { originator = _originator; }

		/**
		 * Return true if testOriginator is originator
		 * of the command.
		 *
		 * @param testOriginator Test originator.
		 *
		 * @return True if testOriginator is command
		 * originator.
		 */
		bool isOriginator (Rts2Object * testOriginator) { return originator == testOriginator; }

		/**
		 * Returns status of info call, issued against central server.
		 *
		 * States are:
		 *  - status command was not issued
		 *  - status command was issued
		 *  - status command sucessfulle ended
		 *
		 * @return Status of info call.
		 */
		cip_state_t getStatusCallProgress () { return (cip_state_t) (bopMask & BOP_CIP_MASK); }

		/**
		 * Sets status of info call.
		 *
		 * @param call_progress Call progress.
		 *
		 * @see Rts2Command::getStatusCallProgress
		 */
		void setStatusCallProgress (cip_state_t call_progress)
		{
			bopMask = (bopMask & ~BOP_CIP_MASK) | (call_progress & BOP_CIP_MASK);
		}

		/**
		 * Called when command returns without error.
		 *
		 * This function is overwritten in childrens to react on
		 * command returning OK.
		 *
		 * @param conn Connection on which command returns.
		 *
		 * @return -1, @ref RTS2_COMMAND_KEEP or @ref RTS2_COMMAND_REQUE.
		 *
		 * @callgraph
		 */
		virtual int commandReturnOK (Rts2Conn * conn);

		/**
		 * Called when command returns with status indicating that it will be
		 * executed later.
		 *
		 * Is is called when device state will allow execution of such command. This
		 * function is overwritten in childrens to react on command which will be
		 * executed later.
		 *
		 * @param conn Connection on which command returns.
		 *
		 * @return -1, @ref RTS2_COMMAND_KEEP or @ref RTS2_COMMAND_REQUE.
		 *
		 * @callgraph
		 */
		virtual int commandReturnQued (Rts2Conn * conn);

		/**
		 * Called when command returns with error.
		 *
		 * This function is overwritten in childrens to react on
		 * command returning OK.
		 *
		 * @param conn Connection on which command returns.
		 *
		 * @return -1, @ref RTS2_COMMAND_KEEP or @ref RTS2_COMMAND_REQUE.
		 *
		 * @callgraph
		 */
		virtual int commandReturnFailed (int status, Rts2Conn * conn);

		/**
		 * Called to remove reference to deleted connection.
		 *
		 * @param conn Connection which will be removed.
		 */
		virtual void deleteConnection (Rts2Conn * conn);
	protected:
		Rts2Block * owner;
		Rts2Conn * connection;
		char * text;
	private:
		int bopMask;
		Rts2Object * originator;
};

/**
 * Command send to central daemon.
 *
 * @ingroup RTS2Command
 */
class Rts2CentraldCommand:public Rts2Command
{

	public:
		Rts2CentraldCommand (Rts2Block * _owner, char *_text)
			:Rts2Command (_owner, _text)
		{
		}
};

/**
 * Send and process authorization request.
 *
 * @ingroup RTS2Command
 */
class Rts2CommandSendKey:public Rts2Command
{
	private:
		int key;
		int centrald_id;
		int centrald_num;
	public:
		Rts2CommandSendKey (Rts2Block * _master, int _centrald_id, int _centrald_num, int _key);
		virtual int send ();

		virtual int commandReturnOK (Rts2Conn * conn)
		{
			connection->setConnState (CONN_AUTH_OK);
			return -1;
		}
		virtual int commandReturnFailed (int status, Rts2Conn * conn)
		{
			connection->setConnState (CONN_AUTH_FAILED);
			return -1;
		}
};

/**
 * Send authorization query to centrald daemon.
 *
 * @ingroup RTS2Command
 */
class Rts2CommandAuthorize:public Rts2Command
{
	public:
		Rts2CommandAuthorize (Rts2Block * _master, int centralId, int key);
		virtual int commandReturnFailed (int status, Rts2Conn * conn)
		{
			logStream (MESSAGE_ERROR) << "authentification failed for connection " << conn->getName ()
				<< " centrald num " << conn->getCentraldNum ()
				<< " centrald id " << conn->getCentraldId ()
				<< sendLog;
			return -1;
		}
};

/**
 * Send key request to centrald.
 *
 * @ingroup RTS2Command
 */
class Rts2CommandKey:public Rts2Command
{
	public:
		Rts2CommandKey (Rts2Block * _master, const char * device_name);
};

/**
 * Common class for all command, which changed camera settings.
 *
 * @ingroup RTS2Command
 */
class Rts2CommandCameraSettings:public Rts2Command
{
	public:
		Rts2CommandCameraSettings (Rts2DevClientCamera * camera);
};

/**
 * Start exposure on camera.
 *
 * @ingroup RTS2Command
 */
class Rts2CommandExposure:public Rts2Command
{
	private:
		Rts2DevClientCamera * camera;
	public:
		/**
		 * Send exposure command to device.
		 *
		 * @param _master Master block which controls exposure
		 * @param _camera Camera client for exposure
		 * @param _bopMask BOP mask for exposure command
		 */
		Rts2CommandExposure (Rts2Block * _master, Rts2DevClientCamera * _camera, int _bopMask);

		virtual int commandReturnFailed (int status, Rts2Conn * conn);
};

/**
 * Start data readout.
 *
 * @ingourp RTS2Command
 */
class Rts2CommandReadout:public Rts2Command
{
	public:
		Rts2CommandReadout (Rts2Block * _master);
};

/**
 * Set filter.
 *
 * @ingroup RTS2Command
 */
class Rts2CommandFilter:public Rts2Command
{
	private:
		Rts2DevClientCamera * camera;
		Rts2DevClientPhot * phot;
		Rts2DevClientFilter * filterCli;
		void setCommandFilter (int filter);
	public:
		Rts2CommandFilter (Rts2DevClientCamera * _camera, int filter);
		Rts2CommandFilter (Rts2DevClientPhot * _phot, int filter);
		Rts2CommandFilter (Rts2DevClientFilter * _filter, int filter);

		virtual int commandReturnOK (Rts2Conn * conn);
		virtual int commandReturnFailed (int status, Rts2Conn * conn);
};

class Rts2CommandBox:public Rts2CommandCameraSettings
{
	public:
		Rts2CommandBox (Rts2DevClientCamera * _camera, int x, int y, int w, int h);
};

class Rts2CommandCenter:public Rts2CommandCameraSettings
{
	public:
		Rts2CommandCenter (Rts2DevClientCamera * _camera, int width, int height);
};

/**
 * Issue command to change value, but do not send return status.
 *
 * @ingroup RTS2Command
 */
class Rts2CommandChangeValue:public Rts2Command
{
	public:
		Rts2CommandChangeValue (Rts2DevClient * _client, std::string _valName, char op, int _operand);
		Rts2CommandChangeValue (Rts2DevClient * _client, std::string _valName, char op, long int _operand);
		Rts2CommandChangeValue (Rts2DevClient * _client, std::string _valName, char op, float _operand);
		Rts2CommandChangeValue (Rts2DevClient * _client, std::string _valName, char op, double _operand);
		Rts2CommandChangeValue (Rts2DevClient * _client, std::string _valName, char op, double _operand1, double _operand2);
		Rts2CommandChangeValue (Rts2DevClient * _client, std::string _valName, char op, bool _operand);
		/**
		 * Change rectangle value.
		 */
		Rts2CommandChangeValue (Rts2DevClient * _client, std::string _valName, char op, int x, int y, int w, int h);
		/**
		 * Create command to change value from string.
		 *
		 * @param raw If true, string will be send without escaping.
		 */
		Rts2CommandChangeValue (Rts2DevClient * _client, std::string _valName, char op, std::string _operand, bool raw = false);
};

/**
 * Command for telescope movement.
 *
 * @ingroup RTS2Command
 */
class Rts2CommandMove:public Rts2Command
{
	public:
		Rts2CommandMove (Rts2Block * _master, Rts2DevClientTelescope * _tel, double ra, double dec);
		virtual int commandReturnFailed (int status, Rts2Conn * conn);
	protected:
		Rts2CommandMove (Rts2Block * _master, Rts2DevClientTelescope * _tel);
	private:
		Rts2DevClientTelescope *tel;
};

/**
 * Move telescope without modelling corrections.
 *
 * @ingroup RTS2Command
 */
class Rts2CommandMoveUnmodelled:public Rts2CommandMove
{
	public:
		Rts2CommandMoveUnmodelled (Rts2Block * _master, Rts2DevClientTelescope * _tel, double ra, double dec);
};

/**
 * Move telescope to fixed location.
 *
 * @ingroup RTS2Command
 */
class Rts2CommandMoveFixed:public Rts2CommandMove
{
	public:
		Rts2CommandMoveFixed (Rts2Block * _master, Rts2DevClientTelescope * _tel, double ra, double dec);
};

/**
 * Command for telescope movement in alt az.
 *
 * @ingroup RTS2Command
 */
class Rts2CommandMoveAltAz:public Rts2CommandMove
{
	public:
		Rts2CommandMoveAltAz (Rts2Block * _master, Rts2DevClientTelescope * _tel, double alt, double az);
};

class Rts2CommandResyncMove:public Rts2CommandMove
{
	public:
		Rts2CommandResyncMove (Rts2Block * _master, Rts2DevClientTelescope * _tel, double ra, double dec);
};

class Rts2CommandChange:public Rts2Command
{
	Rts2DevClientTelescope *tel;
	public:
		Rts2CommandChange (Rts2Block * _master, double ra, double dec);
		Rts2CommandChange (Rts2DevClientTelescope * _tel, double ra, double dec);
		Rts2CommandChange (Rts2CommandChange * _command, Rts2DevClientTelescope * _tel);
		virtual int commandReturnFailed (int status, Rts2Conn * conn);
};

class Rts2CommandCorrect:public Rts2Command
{
	public:
		Rts2CommandCorrect (Rts2Block * _master, int corr_mark, int corr_img, int img_id, double ra_corr, double dec_corr, double pos_err);
};

class Rts2CommandStartGuide:public Rts2Command
{
	public:
		Rts2CommandStartGuide (Rts2Block * _master, char dir, double dir_dist);
};

class Rts2CommandStopGuide:public Rts2Command
{
	public:
		Rts2CommandStopGuide (Rts2Block * _master, char dir);
};

class Rts2CommandStopGuideAll:public Rts2Command
{
	public:
		Rts2CommandStopGuideAll (Rts2Block * _master):Rts2Command (_master)
		{
			setCommand ("stop_guide_all");
		}
};

class Rts2CommandCupolaMove:public Rts2Command
{
	Rts2DevClientCupola * copula;
	public:
		Rts2CommandCupolaMove (Rts2DevClientCupola * _copula, double ra,
			double dec);
		virtual int commandReturnFailed (int status, Rts2Conn * conn);
};

class Rts2CommandCupolaNotMove:public Rts2Command
{
	Rts2DevClientCupola * copula;
	public:
		Rts2CommandCupolaNotMove (Rts2DevClientCupola * _copula);
		virtual int commandReturnFailed (int status, Rts2Conn * conn);
};

class Rts2CommandChangeFocus:public Rts2Command
{
	private:
		Rts2DevClientFocus * focuser;
		Rts2DevClientCamera * camera;
		void change (int _steps);
	public:
		Rts2CommandChangeFocus (Rts2DevClientFocus * _focuser, int _steps);
		Rts2CommandChangeFocus (Rts2DevClientCamera * _camera, int _steps);
		virtual int commandReturnFailed (int status, Rts2Conn * conn);
};

class Rts2CommandSetFocus:public Rts2Command
{
	private:
		Rts2DevClientFocus * focuser;
		Rts2DevClientCamera * camera;
		void set (int _steps);
	public:
		Rts2CommandSetFocus (Rts2DevClientFocus * _focuser, int _steps);
		Rts2CommandSetFocus (Rts2DevClientCamera * _camera, int _steps);
		virtual int commandReturnFailed (int status, Rts2Conn * conn);
};

class Rts2CommandMirror:public Rts2Command
{
	private:
		Rts2DevClientMirror * mirror;
	public:
		Rts2CommandMirror (Rts2DevClientMirror * _mirror, int _pos);
		virtual int commandReturnFailed (int status, Rts2Conn * conn);
};

class Rts2CommandIntegrate:public Rts2Command
{
	private:
		Rts2DevClientPhot * phot;
	public:
		Rts2CommandIntegrate (Rts2DevClientPhot * _phot, int _filter,
			float _exp, int _count);
		Rts2CommandIntegrate (Rts2DevClientPhot * _phot, float _exp,
			int _count);
		virtual int commandReturnFailed (int status, Rts2Conn * conn);
};

class Rts2CommandExecNext:public Rts2Command
{
	public:
		Rts2CommandExecNext (Rts2Block * _master, int next_id);
};

class Rts2CommandExecNow:public Rts2Command
{
	public:
		Rts2CommandExecNow (Rts2Block * _master, int now_id);
};

/**
 * Execute GRB observation.
 *
 * @ingroup RTS2Command
 */
class Rts2CommandExecGrb:public Rts2Command
{
	public:
		Rts2CommandExecGrb (Rts2Block * _master, int grb_id);
};

class Rts2CommandExecShower:public Rts2Command
{
	public:
		Rts2CommandExecShower (Rts2Block * _master);
};

class Rts2CommandKillAll:public Rts2Command
{
	public:
		Rts2CommandKillAll (Rts2Block * _master);
};

/**
 * Sends script end command.
 *
 * @ingroup RTS2Command
 */
class Rts2CommandScriptEnds:public Rts2Command
{
	public:
		Rts2CommandScriptEnds (Rts2Block * _master);
};

class Rts2CommandMessageMask:public Rts2Command
{
	public:
		Rts2CommandMessageMask (Rts2Block * _master, int _mask);
};

/**
 * Send info command to central server.
 *
 * @ingroup RTS2Command
 */
class Rts2CommandInfo:public Rts2Command
{
	public:
		Rts2CommandInfo (Rts2Block * _master);
		virtual int commandReturnOK (Rts2Conn * conn);
		virtual int commandReturnFailed (int status, Rts2Conn * conn);
};

/**
 * Send status info command.
 *
 * When this command return, device status is updated, so updateStatusWait from
 * control_conn is called.
 *
 * @msc
 *
 * Block, Centrald, Devices;
 *
 * Block->Centrald [label="status_info"];
 * Centrald->Devices [label="status_info"];
 * Devices->Centrald [label="S xxxx"];
 * Centrald->Block [label="S xxxx"];
 * Centrald->Block [label="OK"];
 * Devices->Centrald [label="OK"];
 *
 * @endmsc
 *
 * @ingroup RTS2Command
 */
class Rts2CommandStatusInfo:public Rts2Command
{
	private:
		Rts2Conn * control_conn;
	public:
		Rts2CommandStatusInfo (Rts2Block * master, Rts2Conn * _control_conn);
		virtual int commandReturnOK (Rts2Conn * conn);
		virtual int commandReturnFailed (Rts2Conn * conn);

		const char * getCentralName()
		{
			return control_conn->getName ();
		}

		virtual void deleteConnection (Rts2Conn * conn);
};

/**
 * Send device_status command instead of status_info command.
 *
 * @ingroup RTS2Command
 */
class Rts2CommandDeviceStatus:public Rts2CommandStatusInfo
{
	public:
		Rts2CommandDeviceStatus (Rts2Block * master, Rts2Conn * _control_conn);
};

}
#endif							 /* !__RTS2_COMMAND__ */
