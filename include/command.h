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

#include "block.h"

/**
 * @defgroup RTS2Command RTS2 commands
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
 * Info command. @ingroup RTS2Command
 *
 * Forces device to refresh its values and send updated values over connection
 * before the command is finished.
 */
#define COMMAND_INFO            "info"


/**
 * Move command. @ingroup RTS2Command
 *
 * This command should work only on telescope (mount) devices. Its two
 * parameters, RA and DEC, is the target RA DEC of the sky position telescope
 * should move on. An error can be reported immediately after the command is send.
 *
 * RA and DEC parameters can be specified as float number, or as float numbers
 * separated by ":". In the first case it is assumed command is in degrees, in
 * the second the command is assuemed to be in sexadecimal format, with the
 * highest unit being always degrees.
 *
 * @subsection Examples
 *
 * To move the telescope to 1:20 in RA, -85:21:00 in DEC, use one of the following:
 *  - move 20.00 -85:21:00
 *  - move 1:20 -85:21
 *  - move 1:20:00 -85:21
 *  - move 20 -85.35
 */
#define COMMAND_TELD_MOVE       "move"

/**
 * Move with proper motion data (in arcsec/year)
 * move_pm 1:20:35 -85:21:14 0.211 0.356
 */
#define COMMAND_TELD_MOVE_PM    "move_pm"

#define COMMAND_TELD_MOVE_EPOCH  "move_epoch"

/**
 * Move to position in given epoch, with proper motions (arcsec/year).
 * Proper motions are defined in arcseconds.
 */
#define COMMAND_TELD_MOVE_EPOCH_PM  "move_epoch_pm"

/**
 * Move to HA DEC coordinates.
 */
#define COMMAND_TELD_HADEC      "hadec"

/**
 * Move telescope to altaz coordinates.
 */
#define COMMAND_TELD_ALTAZ      "altaz"

/**
 * Move telescope to altaz, unset horizon check.
 */
#define COMMAND_TELD_ALTAZ_NC   "nc_altaz"

/**
 * Move to MPEC one line element position. @ingroup RTS2Command
 */
#define COMMAND_TELD_MOVE_MPEC  "move_mpec"

/**
 * Move to TLE two line elements. @ingroup RTS2Command
 */
#define COMMAND_TELD_MOVE_TLE   "move_tle"

/**
 * Park telescope.
 */
#define COMMAND_TELD_PARK       "park"

/**
 * Peek telescope movement. @ingroup RTS2Command
 *
 * @subsection Examples
 *
 * To preview movement to 1:20 in RA, -85:21:00 in DEC, use one of the following:
 *  - peek 20.00 -85:21:00
 *  - peek 1:20 -85:21
 *  - peek 1:20:00 -85:21
 *  - peek 20 -85.35
 */
#define COMMAND_TELD_PEEK       "peek"

/**
 * Download new DUT1 data.
 */
#define COMMAND_TELD_GETDUT1    "getdut1"

/**
 * Updates weather data.
 */
#define COMMAND_TELD_WEATHER    "weather"

/**
 * Send client location of the latest camera image. @ingroup RTS2Command
 *
 * This command is used when camera produces data directly written on the disk.
 * Client can decide after receiving the data, if it would like to e.g. add
 * something to file header.
 */
#define COMMAND_DATA_IN_FITS    "fits_data"

/**
 * Send by recipient connction to update image statistics. Can be used only
 * for FITS transfere data.
 */
#define COMMAND_FITS_STAT       "fits_statistics"

/**
 * Start exposure on camera.
 */
#define COMMAND_CCD_EXPOSURE    "expose"


/**
 * Start exposure on camera, do not check for blocking states.
 */
#define COMMAND_CCD_EXPOSURE_NO_CHECKS  "expose_no_checks"

/**
 * Shift-store sequence.
 */
#define COMMAND_CCD_SHIFTSTORE  "shiftstore"

/**
 * Open dome/cover/...
 */
#define COMMAND_OPEN       "open"

#define COMMAND_CLOSE      "close"

#define COMMAND_STOP       "stop"

/**
 * Cupola azimuth move. @ingroup RTS2Command
 *
 * @subsection Examples
 *
 * To move the cupola slit to face azimuth 90 degrees, use one of the following:
 *  - az 90
 *  - az 90:00:00
 *  - az 90.000
 */
#define COMMAND_CUPOLA_AZ       "az"

#define COMMAND_CUPOLA_MOVE     "move"
#define COMMAND_CUPOLA_SYNCTEL  "synctel"
#define COMMAND_CUPOLA_PARK     "park"

/**
 * Distribution of the new parallactic angle. @ingroup RTS2Command
 *
 * Updates devices about parallactic angle, calculated at their master device.
 * The only parameter so far is new value of parallactic angle.
 */
#define COMMAND_PARALLACTIC_UPDATE   "pa_update"

/**
 * Start autorotating (rotate max/min, without raising BLOCK_EXPOSURE)
 */
#define COMMAND_ROTATOR_AUTO         "autorotate"

#define COMMAND_ROTATOR_AUTOSTOP     "autostop"

/**
 * Park derotators.
 */
#define COMMAND_ROTATOR_PARK         "park"

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
 * This object is usually send through Connection::queCommand to connection,
 * which process it, wait for the other side to reply, pass return code to
 * Connection::commandReturn callback and delete it.
 *
 * @see Connection
 *
 * @ingroup RTS2Block
 * @ingroup RTS2Command
 */
class Command
{
	public:
		Command (Block * _owner);
		Command (Block * _owner, const char *_text);
		Command (Command * _command);
		Command (Command & _command);
		virtual ~ Command (void);

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

		void setConnection (Connection * conn) { connection = conn; }

		Connection * getConnection () { return connection; }

		virtual int send ();
		int commandReturn (int status, Connection * conn);

		/**
		 * Returns command text.
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
		 * @see Command::setBopMask
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
		 * @see Connection::queCommand
		 *
		 * @callergraph
		 */
		void setOriginator (Object * _originator) { originator = _originator; }

		/**
		 * Return true if testOriginator is originator
		 * of the command.
		 *
		 * @param testOriginator Test originator.
		 *
		 * @return True if testOriginator is command
		 * originator.
		 */
		bool isOriginator (Object * testOriginator) { return originator == testOriginator; }

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
		 * @see Command::getStatusCallProgress
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
		virtual int commandReturnOK (Connection * conn);

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
		virtual int commandReturnQued (Connection * conn);

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
		virtual int commandReturnFailed (int status, Connection * conn);

		/**
		 * Called to remove reference to deleted connection.
		 *
		 * @param conn Connection which will be removed.
		 */
		virtual void deleteConnection (Connection * conn);
	protected:
		Block * owner;
		Connection * connection;
		char * text;
	private:
		int bopMask;
		Object * originator;
};

/**
 * Command send to central daemon.
 *
 * @ingroup RTS2Command
 */
class Rts2CentraldCommand:public Command
{

	public:
		Rts2CentraldCommand (Block * _owner, char *_text)
			:Command (_owner, _text)
		{
		}
};

/**
 * Send and process authorization request.
 *
 * @ingroup RTS2Command
 */
class CommandSendKey:public Command
{
	private:
		int key;
		int centrald_id;
		int centrald_num;
	public:
		CommandSendKey (Block * _master, int _centrald_id, int _centrald_num, int _key);
		virtual int send ();

		virtual int commandReturnOK (Connection * conn)
		{
			connection->setConnState (CONN_AUTH_OK);
			return -1;
		}
		virtual int commandReturnFailed (int status, Connection * conn)
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
class CommandAuthorize:public Command
{
	public:
		CommandAuthorize (Block * _master, int centralId, int key);
		virtual int commandReturnFailed (int status, Connection * conn)
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
class CommandKey:public Command
{
	public:
		CommandKey (Block * _master, const char * device_name);
};

/**
 * Common class for all command, which changed camera settings.
 *
 * @ingroup RTS2Command
 */
class CommandCameraSettings:public Command
{
	public:
		CommandCameraSettings (DevClientCamera * camera);
};

/**
 * Start exposure on camera.
 *
 * @ingroup RTS2Command
 */
class CommandExposure:public Command
{
	public:
		/**
		 * Send exposure command to device.
		 *
		 * @param _master Master block which controls exposure
		 * @param _camera Camera client for exposure
		 * @param _bopMask BOP mask for exposure command
		 */
		CommandExposure (Block * _master, DevClientCamera * _camera, int _bopMask);

		virtual int commandReturnOK (Connection *conn);
		virtual int commandReturnFailed (int status, Connection * conn);
	private:
		DevClientCamera * camera;
};

/**
 * Start data readout.
 *
 * @ingourp RTS2Command
 */
class CommandReadout:public Command
{
	public:
		CommandReadout (Block * _master);
};

class CommandShiftStart: public Command
{
	public:
		CommandShiftStart (Block * _master, float expTime, int _bopMask);
};

class CommandShiftProgress: public Command
{
	public:
		CommandShiftProgress (Block * _master, int shift, float expTime, int _bopMask);
};

class CommandShiftEnd: public Command
{
	public:
		CommandShiftEnd (Block * _master, int shift, float expTime, int _bopMask);
};


/**
 * Sends image statistics.
 */
class CommandFitsStat:public Command
{
	public:
		CommandFitsStat (Block * _master, double average, double min, double max, double sum, double mode);
};

/**
 * Set filter.
 *
 * @ingroup RTS2Command
 */
class CommandFilter:public Command
{
	private:
		DevClientCamera * camera;
		DevClientPhot * phot;
		DevClientFilter * filterCli;
		void setCommandFilter (int filter);
	public:
		CommandFilter (DevClientCamera * _camera, int filter);
		CommandFilter (DevClientPhot * _phot, int filter);
		CommandFilter (DevClientFilter * _filter, int filter);

		virtual int commandReturnOK (Connection * conn);
		virtual int commandReturnFailed (int status, Connection * conn);
};

class CommandBox:public CommandCameraSettings
{
	public:
		CommandBox (DevClientCamera * _camera, int x, int y, int w, int h);
};

class CommandCenter:public CommandCameraSettings
{
	public:
		CommandCenter (DevClientCamera * _camera, int width, int height);
};

/**
 * Issue command to change value, but do not send return status.
 *
 * @ingroup RTS2Command
 */
class CommandChangeValue:public Command
{
	public:
		CommandChangeValue (Block * _master, std::string _valName, char op, int _operand);
		CommandChangeValue (Block * _master, std::string _valName, char op, long int _operand);
		CommandChangeValue (Block * _master, std::string _valName, char op, float _operand);
		CommandChangeValue (Block * _master, std::string _valName, char op, double _operand);
		CommandChangeValue (Block * _master, std::string _valName, char op, double _operand1, double _operand2);
		CommandChangeValue (Block * _master, std::string _valName, char op, int _operand1, int _operand2, int _operand3);
		CommandChangeValue (Block * _master, std::string _valName, char op, bool _operand);
		/**
		 * Change rectangle value.
		 */
		CommandChangeValue (Block * _master, std::string _valName, char op, int x, int y, int w, int h);
		/**
		 * Create command to change value from string.
		 *
		 * @param raw If true, string will be send without escaping.
		 */
		CommandChangeValue (Block * _master, std::string _valName, char op, std::string _operand, bool raw = false);
};

/**
 * Command for telescope movement.
 *
 * @ingroup RTS2Command
 */
class CommandMove:public Command
{
	public:
		CommandMove (Block * _master, DevClientTelescope * _tel, double ra, double dec);
		virtual int commandReturnFailed (int status, Connection * conn);
	protected:
		CommandMove (Block * _master, DevClientTelescope * _tel);
	private:
		DevClientTelescope *tel;
};

/**
 * Move telescope without modelling corrections.
 *
 * @ingroup RTS2Command
 */
class CommandMoveUnmodelled:public CommandMove
{
	public:
		CommandMoveUnmodelled (Block * _master, DevClientTelescope * _tel, double ra, double dec);
};

/**
 * Move telescope to MPEC one-line.
 *
 * @ingroup RTS2Command
 */
class CommandMoveMpec:public CommandMove
{
	public:
		CommandMoveMpec (Block * _master, DevClientTelescope * _tel, double ra, double dec, std::string mpec_oneline);
};

/**
 * Move telescope to MPEC one-line.
 *
 * @ingroup RTS2Command
 */
class CommandMoveTle:public CommandMove
{
	public:
		CommandMoveTle (Block * _master, DevClientTelescope * _tel, double ra, double dec, std::string tel1, std::string tle2);
};

/**
 * Move telescope to fixed location.
 *
 * @ingroup RTS2Command
 */
class CommandMoveFixed:public CommandMove
{
	public:
		CommandMoveFixed (Block * _master, DevClientTelescope * _tel, double ra, double dec);
};

/**
 * Command for telescope movement in alt az.
 *
 * @ingroup RTS2Command
 */
class CommandMoveAltAz:public CommandMove
{
	public:
		CommandMoveAltAz (Block * _master, DevClientTelescope * _tel, double alt, double az);
};

class CommandResyncMove:public CommandMove
{
	public:
		CommandResyncMove (Block * _master, DevClientTelescope * _tel, double ra, double dec);
};

class CommandChange:public Command
{
	DevClientTelescope *tel;
	public:
		CommandChange (Block * _master, double ra, double dec);
		CommandChange (DevClientTelescope * _tel, double ra, double dec);
		CommandChange (CommandChange * _command, DevClientTelescope * _tel);
		virtual int commandReturnFailed (int status, Connection * conn);
};

class CommandCorrect:public Command
{
	public:
		CommandCorrect (Block * _master, int corr_mark, int corr_img, int corr_obs, int img_id, int obs_id, double ra_corr, double dec_corr, double pos_err);
};

class CommandStartGuide:public Command
{
	public:
		CommandStartGuide (Block * _master, char dir, double dir_dist);
};

class CommandStopGuide:public Command
{
	public:
		CommandStopGuide (Block * _master, char dir);
};

class CommandStopGuideAll:public Command
{
	public:
		CommandStopGuideAll (Block * _master):Command (_master)
		{
			setCommand ("stop_guide_all");
		}
};

class CommandWeather:public Command
{
	public:
		CommandWeather (Block *_master, double temp, double hum, double pres);
};

class CommandCupolaSyncTel:public Command
{
	public:
		CommandCupolaSyncTel (Block * _master, double ra, double dec);
};

class CommandCupolaNotMove:public Command
{
	DevClientCupola * copula;
	public:
		CommandCupolaNotMove (DevClientCupola * _copula);
		virtual int commandReturnFailed (int status, Connection * conn);
};

class CommandChangeFocus:public Command
{
	private:
		DevClientFocus * focuser;
		DevClientCamera * camera;
		void change (int _steps);
	public:
		CommandChangeFocus (DevClientFocus * _focuser, int _steps);
		CommandChangeFocus (DevClientCamera * _camera, int _steps);
		virtual int commandReturnFailed (int status, Connection * conn);
};

class CommandSetFocus:public Command
{
	private:
		DevClientFocus * focuser;
		DevClientCamera * camera;
		void set (int _steps);
	public:
		CommandSetFocus (DevClientFocus * _focuser, int _steps);
		CommandSetFocus (DevClientCamera * _camera, int _steps);
		virtual int commandReturnFailed (int status, Connection * conn);
};

class CommandMirror:public Command
{
	private:
		DevClientMirror * mirror;
	public:
		CommandMirror (DevClientMirror * _mirror, int _pos);
		virtual int commandReturnFailed (int status, Connection * conn);
};

class CommandIntegrate:public Command
{
	private:
		DevClientPhot * phot;
	public:
		CommandIntegrate (DevClientPhot * _phot, int _filter,
			float _exp, int _count);
		CommandIntegrate (DevClientPhot * _phot, float _exp,
			int _count);
		virtual int commandReturnFailed (int status, Connection * conn);
};

class CommandExecNext:public Command
{
	public:
		CommandExecNext (Block * _master, int next_id);
};

class CommandExecNextPlan:public Command
{
	public:
		CommandExecNextPlan (Block * _master, int next_plan_id);
};

class CommandExecNow:public Command
{
	public:
		CommandExecNow (Block * _master, int now_id);
};

/**
 * Execute GRB observation.
 *
 * @ingroup RTS2Command
 */
class CommandExecGrb:public Command
{
	public:
		CommandExecGrb (Block * _master, int _grb_id);

		int getGrbID () { return grb_id; }
	private:
		int grb_id;
};

class CommandQueueNow:public Command
{
	public:
		CommandQueueNow (Block * _master, const char *queue, int tar_id);
};

/**
 * Queue target only once, make sure it does not create multiple entries into queue.
 *
 * @author Petr Kubanek, Institute of Physics <kubanek@fzu.cz>
 */
class CommandQueueNowOnce:public Command
{
	public:
		CommandQueueNowOnce (Block * _master, const char *queue, int tar_id);
};

class CommandQueueAt:public Command
{
	public:
		CommandQueueAt (Block * _master, const char *queue, int tar_id, double t_start, double t_end);
};

class CommandExecShower:public Command
{
	public:
		CommandExecShower (Block * _master);
};

class CommandKillAll:public Command
{
	public:
		CommandKillAll (Block * _master);
};

class CommandKillAllWithoutScriptEnds:public Command
{
	public:
		CommandKillAllWithoutScriptEnds (Block * _master);
};

/**
 * Sends script end command.
 *
 * @ingroup RTS2Command
 */
class CommandScriptEnds:public Command
{
	public:
		CommandScriptEnds (Block * _master);
};

class CommandMessageMask:public Command
{
	public:
		CommandMessageMask (Block * _master, int _mask);
};

/**
 * Send info command to central server.
 *
 * @ingroup RTS2Command
 */
class CommandInfo:public Command
{
	public:
		CommandInfo (Block * _master);
		virtual int commandReturnOK (Connection * conn);
		virtual int commandReturnFailed (int status, Connection * conn);
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
class CommandStatusInfo:public Command
{
	private:
		Connection * control_conn;
	public:
		CommandStatusInfo (Block * master, Connection * _control_conn);
		virtual int commandReturnOK (Connection * conn);
		virtual int commandReturnFailed (int status, Connection * conn);

		const char * getCentralName()
		{
			return control_conn->getName ();
		}

		virtual void deleteConnection (Connection * conn);
};

/**
 * Send device_status command instead of status_info command.
 *
 * @ingroup RTS2Command
 */
class CommandDeviceStatus:public CommandStatusInfo
{
	public:
		CommandDeviceStatus (Block * master, Connection * _control_conn);
};

/**
 * Update parallactic angle (PA) tracking. Parameters of the command are:
 *
 * <ol>
 *   <li>time when PA was calculated</li>
 *   <li>calculated PA</li>
 *   <li>rate of PA change (in degrees/hour)</li>
 *   <li>telescope altitude</li>
 *   <li>telescope azimuth</li>
 * </ol>
 */
class CommandParallacticAngle:public Command
{
	public:
		CommandParallacticAngle (Block * _master, double reftime, double pa, double rate, double altitude, double azimuth);
};

class CommandMoveAz:public Command
{
	public:
		CommandMoveAz (Block * _master, double az);
};

}
#endif							 /* !__RTS2_COMMAND__ */
