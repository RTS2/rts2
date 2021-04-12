/**
 * Executor client for telescope and camera.
 * Copyright (C) 2005-2007 Petr Kubanek <petr@kubanek.net>
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

#include <limits.h>
#include <iostream>

#include "rts2script/connimgprocess.h"
#include "rts2script/execcli.h"
#include "rts2fits/image.h"
#include "rts2db/target.h"
#include "command.h"
#include "configuration.h"

using namespace rts2script;

// to add more debugging
// #define DEBUG_EXTRA

DevClientCameraExec::DevClientCameraExec (rts2core::Connection * _connection, rts2core::ValueString *_expandPath, std::string templateFile):rts2image::DevClientCameraImage (_connection, templateFile), DevScript (_connection)
{
	expandPathValue = _expandPath;
	expandPathString = std::string ("");
	expandOverwrite = false;
	waitForExposure = false;
	waitMetaData = false;
	imgCount = 0;
}

DevClientCameraExec::~DevClientCameraExec ()
{
	deleteScript ();
}

rts2image::Image * DevClientCameraExec::createImage (const struct timeval *expStart)
{
	exposureScript = getScript ();
	rts2image::Image *ret;
	if (expandPathString.length () > 0)
	{
		ret = new rts2image::Image (expandPathString.c_str (), getExposureNumber (), expStart, connection, expandOverwrite, writeConnection, writeRTS2Values);
		nullExpandPath ();
	}
	else if (expandPathValue)
	{
		ret = new rts2image::Image (expandPathValue->getValue (), getExposureNumber (), expStart, connection, expandOverwrite, writeConnection, writeRTS2Values);
	}
	else
	{
		ret = DevClientCameraImage::createImage (expStart);
	}
	return ret;
}

void DevClientCameraExec::postEvent (rts2core::Event * event)
{
	rts2image::Image *image;
	int type = event->getType ();
	switch (type)
	{
		case EVENT_KILL_ALL:
		case EVENT_SET_TARGET_KILL:
		case EVENT_SET_TARGET_KILL_NOT_CLEAR:
			waitForExposure = false;
			break;
		case EVENT_QUE_IMAGE:
		case EVENT_AFTER_COMMAND_FINISHED:
			image = (rts2image::Image *) event->getArg ();
			if (!strcmp (image->getCameraName (), getName ()))
				queImage (image, type == EVENT_AFTER_COMMAND_FINISHED ? false : true);
			break;
		case EVENT_COMMAND_OK:
			nextCommand ((rts2core::Command *) event->getArg ());
			break;
		case EVENT_COMMAND_FAILED:
			waitForExposure = false;
			nextCommand ();
			//deleteScript ();
			break;
	}
	DevScript::postEvent (event);
	rts2image::DevClientCameraImage::postEvent (event);
	// set target name on EVENT_OBSERVE
	switch (type)
	{
		case EVENT_SET_TARGET:
		case EVENT_SET_TARGET_NOT_CLEAR:
		case EVENT_SET_TARGET_KILL:
		case EVENT_SET_TARGET_KILL_NOT_CLEAR:
		case EVENT_OBSERVE:
			if (currentTarget && currentTarget->getTargetName ())
				queCommand (new rts2core::CommandChangeValue (getMaster (), "OBJECT", '=', std::string (currentTarget->getTargetName ())));
			break;
	}
	// must be done after processing trigger in parent
	if (type == EVENT_TRIGGERED && !waitForExposure)
		nextCommand ();
}

void DevClientCameraExec::startTarget (bool callScriptEnds)
{
	cmdConns.clear ();
	DevScript::startTarget (callScriptEnds);
}

int DevClientCameraExec::getNextCommand ()
{
	// there is only one continue which can bring us on beginning
	while (true)
	{
		int ret = getScript ()->nextCommand (*this, &nextComd, cmd_device);
		cmdConns.clear ();
		if (nextComd)
		{
			// send command to other device
			if (strcmp (getName (), cmd_device) != 0)
			{
				// special names, on two charactesrs, ending on X..
				if (strlen (cmd_device) == 2 && cmd_device[1] == 'X')
				{
					int dt = DEVICE_TYPE_UNKNOW;
					switch (cmd_device[0])
					{
						case 'C':
							dt = DEVICE_TYPE_CCD;
							break;
						case 'T':
							dt = DEVICE_TYPE_MOUNT;
							break;
						case 'F':
							dt = DEVICE_TYPE_FOCUS;
							break;
						case 'W':
							dt = DEVICE_TYPE_FW;
							break;
						case 'R':
							dt = DEVICE_TYPE_ROTATOR;
							break;
						case 'P':
							dt = DEVICE_TYPE_PHOT;
							break;
						case 'M':
							dt = DEVICE_TYPE_MIRROR;
							break;
					}

					connections_t::iterator iter = getMaster ()->getConnections ()->begin ();
					getMaster ()->getOpenConnectionType (dt, iter);
					while (iter != getMaster ()->getConnections ()->end ())
					{
						cmdConns.push_back (*iter);
						iter++;
						getMaster ()->getOpenConnectionType (dt, iter);
					}
				}
				else
				{
					rts2core::Connection *cmdC = getMaster ()->getOpenConnection (cmd_device);
					if (cmdC != NULL)
						cmdConns.push_back (cmdC);
				}

				if (cmdConns.size () == 0)
				{
					logStream (MESSAGE_ERROR)
						<< "cannot find device : " << cmd_device
						<< " (while running command " << nextComd->getText () << ")"
						<< sendLog;
					// only there try to get next command
					continue;
				}
			}
			nextComd->setOriginator (this);
			return ret;
		}
		return ret;
	}
}

bool DevClientCameraExec::canEndScript ()
{
	rts2core::Value *queExpNum = getConnection ()->getValue ("que_exp_num");
	if (queExpNum && queExpNum->getValueInteger () != 0)
		return false;
	if (isExposing ())
		return false;
	return DevScript::canEndScript ();
}

void DevClientCameraExec::nextCommand (rts2core::Command *triggerCommand)
{
	if (scriptKillCommand && triggerCommand == scriptKillCommand)
	{
		if (currentTarget == NULL && killTarget != NULL)
			DevScript::postEvent (new rts2core::Event (scriptKillcallScriptEnds ? EVENT_SET_TARGET : EVENT_SET_TARGET_NOT_CLEAR, killTarget));
		killTarget = NULL;
		scriptKillCommand = NULL;
		scriptKillcallScriptEnds = false;
	}
	int ret = 0;
	do
	{
		// if commands are just queued in, keep getting next commands
		ret = doNextCommand ();
#ifdef DEBUG_EXTRA
		logStream (MESSAGE_DEBUG) << "return from doNextCommand " << ret << sendLog;
#endif
	} while (ret > 0);
}

void DevClientCameraExec::queImage (rts2image::Image * image, bool run_after)
{
	// try immediately processing..
	std::string after_command;
	if (run_after && Configuration::instance ()->getString (getName (), "after_exposure_cmd", after_command) == 0)
	{
		int timeout = 60;
		std::string arg;
		Configuration::instance ()->getInteger (getName (), "after_exposure_cmd_timeout", timeout);
		rts2plan::ConnImgProcess *afterCommand = new rts2plan::ConnImgProcess (getMaster (), after_command.c_str (), image->getAbsoluteFileName (), timeout, EVENT_AFTER_COMMAND_FINISHED);
		Configuration::instance ()->getString (getName (), "after_exposure_cmd_arg", arg);
		afterCommand->addArg (image->expand (arg));
		int ret = afterCommand->init ();
		if (ret)
		{
			delete afterCommand;
		}
		else
		{
			getMaster ()->addConnection (afterCommand);
			logStream (MESSAGE_DEBUG) << "running " << afterCommand->getExePath () << sendLog;
			return;
		}
	}

	// find image processor with lowest que number..
	rts2core::Connection *minConn = getMaster ()->getMinConn ("queue_size");
	if (!minConn)
	{
		logStream (MESSAGE_WARNING) << "cannot find IMGP connection for image" << sendLog;
		return;
	}

	if (image->getImageType () != rts2image::IMGTYPE_FLAT && image->getImageType () != rts2image::IMGTYPE_DARK)
	{
		minConn->queCommand (new rts2image::CommandQueImage (getMaster (), image));
	}
	else
	{
		logStream (MESSAGE_DEBUG) << "ignored processing of flat or dark image " << image->getAbsoluteFileName () << sendLog;
	}
}

rts2image::imageProceRes DevClientCameraExec::processImage (rts2image::Image * image)
{
	int ret;
	// make sure script continues if it is waiting for metadata
	if (waitMetaData && !waitForExposure)
		nextCommand ();
	// try processing in script..
	if (exposureScript.get ())
	{
		ret = exposureScript->processImage (image);
		if (ret > 0)
		{
			return rts2image::IMAGE_KEEP_COPY;
		}
		else if (ret == 0)
		{
			return rts2image::IMAGE_DO_BASIC_PROCESSING;
		}
		// otherwise queue image processing
	}
	queImage (image);
	return rts2image::IMAGE_DO_BASIC_PROCESSING;
}

void DevClientCameraExec::idle ()
{
	DevScript::idle ();
	DevClientCameraImage::idle ();
	// when it is the first command in the script..
	if (getScript ().get () && getScript ()->getExecutedCount () == 0)
		nextCommand ();
}

void DevClientCameraExec::exposureStarted (bool expectImage)
{
	if (nextComd && (nextComd->getBopMask () & BOP_WHILE_STATE))
		nextCommand ();

	if (waitForExposure)
	{
		waitForExposure = false;
		if (nextComd && (nextComd->getBopMask () & BOP_TEL_MOVE))
			nextCommand ();
	}

	DevClientCameraImage::exposureStarted (expectImage);
}

void DevClientCameraExec::exposureEnd (bool expectImage)
{
	rts2core::Value *val = getConnection ()->getValue ("que_exp_num");
	// if script is running, inform it about end of exposure..
	if (exposureScript.get ())
		exposureScript->exposureEnd (expectImage);
	// if script is running and it does not have anything to do, end it
	if (getScript ().get ()
		&& !nextComd
		&& getScript ()->isLastCommand ()
		&& (!val || val->getValueInteger () == 0)
		&& !getMaster ()->commandOriginatorPending (this, NULL)
		)
	{
		deleteScript ();
		getMaster ()->postEvent (new rts2core::Event (EVENT_LAST_READOUT));
	}
	// execute value change, if we do not execute that during exposure
	if (strcmp (getName (), cmd_device) && nextComd && (!(nextComd->getBopMask () & BOP_WHILE_STATE)) && !isExposing () && val && val->getValueInteger () == 0)
		nextCommand ();

	// execute next command if it's null
	if (nextComd == NULL)
		nextCommand ();

	// send readout after we deal with next command - which can be filter move
	DevClientCameraImage::exposureEnd (expectImage);
}

void DevClientCameraExec::stateChanged (rts2core::ServerState * state)
{
	DevClientCameraImage::stateChanged (state);
	DevScript::stateChanged (state);
	if (nextComd && cmdConns.size () > 0 && !(state->getValue () & BOP_TEL_MOVE) && !(getConnection ()->getFullBopState () & BOP_TEL_MOVE))
		nextCommand ();
}

void DevClientCameraExec::exposureFailed (int status)
{
	// in case of an error..
	DevClientCameraImage::exposureFailed (status);
	logStream (MESSAGE_WARNING) << "detected exposure failure. Continuing with the script, script " << exposureScript.get () << sendLog;
	if (exposureScript.get ())
		exposureScript->exposureFailed ();
	clearImages ();
	nextCommand ();
}

void DevClientCameraExec::readoutEnd ()
{
	DevClientCameraImage::readoutEnd ();
	// already deleted script..try to query again for next one by sending EVENT_LAST_READOUT
	if (!haveScript ())
		getMaster ()->postEvent (new rts2core::Event (EVENT_SCRIPT_ENDED));
	nextCommand ();
}

int DevClientCameraExec::doNextCommand ()
{
	int ret = haveNextCommand (this);
#ifdef DEBUG_EXTRA
	logStream (MESSAGE_DEBUG) << "connection " << getName () << " DevClientCameraExec::nextComd haveNextCommand " << ret << sendLog;
#endif						 /* DEBUG_EXTRA */
	if (!ret)
		return -1;

#ifdef DEBUG_EXTRA
	logStream (MESSAGE_DEBUG) << "connection " << getName () << " next command " << nextComd->getText () << " bopmask " << nextComd->getBopMask () << sendLog;
#endif
	waitMetaData = false;

	if (nextComd->getBopMask () & BOP_EXPOSURE)
	{
		// if command cannot be executed when telescope is moving, do not execute it
		// before target was moved
		if (currentTarget && !currentTarget->wasMoved())
		{
#ifdef DEBUG_EXTRA
			logStream (MESSAGE_DEBUG) << "BOP_EXPOSURE target not moved" << sendLog;
#endif
			return -2;
		}
		// do not execute next exposure before all meta data of the current exposure are written
		if (waitForMetaData ())
		{
#ifdef DEBUG_EXTRA
			logStream (MESSAGE_DEBUG) << "BOP_EXPOSURE wait for metadata" << sendLog;
#endif
			waitMetaData = true;
		  	return -3;
		}
	}

	rts2core::Value *val;

#ifdef DEBUG_EXTRA
	logStream (MESSAGE_DEBUG) << "bopMask " << std::hex << nextComd->getBopMask () << sendLog;
#endif

	if (nextComd->getBopMask () & BOP_WHILE_STATE)
	{
		// if there are queued exposures, do not execute command
		val = getConnection ()->getValue ("que_exp_num");
		if (val && val->getValueInteger () != 0)
		{
#ifdef DEBUG_EXTRA
			logStream (MESSAGE_DEBUG) << "BOP_WHILE_STATE que_exp_num > 0" << sendLog;
#endif
			return -4;
		}
		// if there are commands in queue, do not execute command
		if (!connection->queEmptyForOriginator (this))
		{
#ifdef DEBUG_EXTRA
			logStream (MESSAGE_DEBUG) << "command queue is not empty " << sendLog;
#endif
			return -5;
		}
	}
	else if (nextComd->getBopMask () & BOP_TEL_MOVE)
	{
		// if there are queued exposures, do not execute command
		val = getConnection ()->getValue ("que_exp_num");
		if (val && val->getValueInteger () != 0)
		{
#ifdef DEBUG_EXTRA
			logStream (MESSAGE_DEBUG) << "BOP_TEL_MOVE que_exp_num > 0" << sendLog;
#endif
			return -6;
		}
		if (waitForExposure)
		{
#ifdef DEBUG_EXTRA
			logStream (MESSAGE_DEBUG) << "BOP_TEL_MOVE wait for exposure" << sendLog;
#endif
			return -7;
		}
	}
	else if (nextComd->getBopMask () & BOP_EXPOSURE)
	{
		if (waitForExposure)
		{
#ifdef DEBUG_EXTRA
			logStream (MESSAGE_DEBUG) << "BOP_EXPOSURE wait for exposure" << sendLog;
#endif
			return -8;
		}
	}

	// send command to other device
	if (cmdConns.size () > 0)
	{
		if (!(nextComd->getBopMask () & BOP_WHILE_STATE))
		{
			// if there are some commands in que, do not proceed, as they might change state of the device
			if (!connection->queEmptyForOriginator (this))
			{
#ifdef DEBUG_EXTRA
				logStream (MESSAGE_DEBUG) << "with cmdConn !BOP_WHILE_STATE queue not empty" << sendLog;
#endif
				return -9;
			}

			// do not execute if there are some exposures in queue
			val = getConnection ()->getValue ("que_exp_num");
			if (val && val->getValueInteger () > 0)
			{
#ifdef DEBUG_EXTRA
				logStream (MESSAGE_DEBUG) << "with cmdConn !BOP_WHILE_STATE que_exp_num > 0" << sendLog;
#endif
				return -10;
			}

			if ((getConnection ()->getState () & CAM_EXPOSING) || (getConnection ()->getBopState () & BOP_TEL_MOVE) || (getConnection ()->getFullBopState () & BOP_TEL_MOVE))
			{
#ifdef DEBUG_EXTRA
				logStream (MESSAGE_DEBUG) << "with cmdConn CAM_EXPOSING " << getConnection ()->getState () << " BOP_TEL_MOVE " << getConnection ()->getBopState () << " full state " << getConnection ()->getFullBopState () << sendLog;
#endif
				return -11;
			}

			nextComd->setBopMask (nextComd->getBopMask () & ~BOP_TEL_MOVE);
		}

		ret = 0;

		// execute command
		// when it returns, we can execute next command
		for (connections_t::iterator iter = cmdConns.begin (); iter != cmdConns.end (); iter++)
		{
#ifdef DEBUG_EXTRA
			logStream (MESSAGE_DEBUG) << "queueing command " << nextComd->getText () << " to " << (*iter)->getName () << sendLog;
#endif
			if (iter == cmdConns.begin ())
				ret += (*iter)->queCommand (nextComd);
			else
				ret += (*iter)->queCommand (new rts2core::Command (nextComd));
		}
		nextComd = NULL;
		waitForExposure = false;
		return ret;
	}

#ifdef DEBUG_EXTRA
	logStream (MESSAGE_DEBUG) << "for " << getName () << " queueing " << nextComd->getText () << sendLog;
#endif						 /* DEBUG_EXTRA */
	waitForExposure = nextComd->getBopMask () & BOP_EXPOSURE;
	ret = queCommand (nextComd);
	nextComd = NULL;			 // after command execute, it will be deleted
	if (waitForExposure)
		setTriggered ();
	return ret;
}

DevClientTelescopeExec::DevClientTelescopeExec (rts2core::Connection * _connection):DevClientTelescopeImage (_connection)
{
	currentTarget = NULL;
	cmdChng = NULL;
	fixedOffset.ra = 0;
	fixedOffset.dec = 0;
}

void DevClientTelescopeExec::postEvent (rts2core::Event * event)
{
	int ret;
	struct ln_equ_posn *offset;
	GuidingParams *gp;
	switch (event->getType ())
	{
		case EVENT_KILL_ALL:
			clearWait ();
			break;
		case EVENT_SET_TARGET:
		case EVENT_SET_TARGET_NOT_CLEAR:
			currentTarget = (rts2db::Target *) event->getArg ();
			break;
		case EVENT_SET_TARGET_KILL:
		case EVENT_SET_TARGET_KILL_NOT_CLEAR:
			clearWait ();
			currentTarget = (rts2db::Target *) event->getArg ();
			break;
		case EVENT_NEW_TARGET:
		case EVENT_CHANGE_TARGET:
			{
				struct ln_equ_posn *pos = (struct ln_equ_posn *) event->getArg ();
				queCommand (new rts2core::CommandMove (getMaster (), this, pos->ra, pos->dec), BOP_TEL_MOVE);
			}
			break;
		case EVENT_NEW_TARGET_ALTAZ:
		case EVENT_CHANGE_TARGET_ALTAZ:
			{
				struct ln_hrz_posn *hrz = (struct ln_hrz_posn *) event->getArg ();
				queCommand (new rts2core::CommandMoveAltAz (getMaster (), this, hrz->alt, hrz->az), BOP_TEL_MOVE);
			}
			break;
		case EVENT_SLEW_TO_TARGET:
		case EVENT_SLEW_TO_TARGET_NOW:
			if (currentTarget)
			{
				currentTarget->beforeMove ();
				ret = syncTarget (event->getType () == EVENT_SLEW_TO_TARGET_NOW, ((ValueInteger *) event->getArg ())->getValueInteger ());
				switch (ret)
				{
					case OBS_DONT_MOVE:
						getMaster ()->postEvent (new rts2core::Event (EVENT_OBSERVE, (void *) currentTarget));
						break;
					case OBS_MOVE:
						fixedOffset.ra = 0;
						fixedOffset.dec = 0;
					case OBS_MOVE_FIXED:
						queCommand (new rts2core::CommandScriptEnds (getMaster ()));
					case OBS_ALREADY_STARTED:
						break;
				}
			}
			break;
		case EVENT_TEL_SCRIPT_RESYNC:
			cmdChng = NULL;
			checkInterChange ();
			break;
		case EVENT_ENTER_WAIT:
			if (cmdChng)
			{
				queCommand (cmdChng);
				cmdChng = NULL;
			}
			else
			{
				ret = syncTarget ();
				if (ret == OBS_DONT_MOVE)
				{
					postEvent (new rts2core::Event (EVENT_MOVE_OK));
				}
			}
			break;
		case EVENT_MOVE_OK:
		case EVENT_CORRECTING_OK:
		case EVENT_MOVE_FAILED:
			break;
		case EVENT_ADD_FIXED_OFFSET:
			offset = (ln_equ_posn *) event->getArg ();
			// ra hold offset in HA - that increase on west
			// but we get offset in RA, which increase on east
			fixedOffset.ra += offset->ra;
			fixedOffset.dec += offset->dec;
			break;
		case EVENT_ACQUSITION_END:
			break;
		case EVENT_TEL_START_GUIDING:
			gp = (GuidingParams *) event->getArg ();
			queCommand (new rts2core::CommandStartGuide (getMaster (), gp->dir, gp->dist));
			break;
		case EVENT_TEL_STOP_GUIDING:
			queCommand (new rts2core::CommandStopGuideAll (getMaster ()));
			break;
	}
	DevClientTelescopeImage::postEvent (event);
}

int DevClientTelescopeExec::syncTarget (bool now, int plan_id)
{
	struct ln_equ_posn coord;
        std::string p1, p2;
	int ret;
	if (!currentTarget)
		return -1;
	getEqu (&coord);
	// startSlew fills coordinates, if needed..
	ret = currentTarget->startSlew (&coord, p1, p2, true, plan_id);
	if (std::isnan (coord.ra) || std::isnan (coord.dec))
	{
#ifdef DEBUG_EXTRA
		logStream (MESSAGE_DEBUG) << "DevClientTelescopeExec::syncTarget: ra, dec were NaN " << sendLog;
#endif
		currentTarget->moveEnded ();
		return 0;
	}
	int bopTel = now ? 0 : BOP_TEL_MOVE;
	switch (ret)
	{
		case OBS_MOVE:
			currentTarget->moveStarted ();
			queCommand (new rts2core::CommandMove (getMaster (), this, coord.ra, coord.dec), bopTel);
			break;
		case OBS_MOVE_UNMODELLED:
			currentTarget->moveStarted ();
			queCommand (new rts2core::CommandMoveUnmodelled (getMaster (), this, coord.ra, coord.dec), bopTel);
			break;
		case OBS_MOVE_MPEC:
			currentTarget->moveStarted ();
			queCommand (new rts2core::CommandMoveMpec (getMaster (), this, coord.ra, coord.dec, p1), bopTel);
			break;
		case OBS_MOVE_TLE:
			currentTarget->moveStarted ();
			queCommand (new rts2core::CommandMoveTle (getMaster (), this, coord.ra, coord.dec, p1, p2), bopTel);
			break;
		case OBS_MOVE_FIXED:
			currentTarget->moveStarted ();
			logStream (MESSAGE_DEBUG) << "DevClientTelescopeExec::syncTarget ha " << coord.ra << " dec " << coord.dec << " oha " << fixedOffset.ra << " odec " << fixedOffset.dec << sendLog;
			// we are ofsetting in HA, but offset is in RA - hence -
			queCommand (new	rts2core::CommandMoveFixed (getMaster (), this, coord.ra - fixedOffset.ra, coord.dec + fixedOffset.dec));
			break;
		case OBS_ALREADY_STARTED:
			currentTarget->moveStarted ();
			if (fixedOffset.ra != 0 || fixedOffset.dec != 0)
			{
			#ifdef DEBUG_EXTRA
				logStream (MESSAGE_DEBUG)<< "DevClientTelescopeExec::syncTarget resync offsets: ra " << fixedOffset.ra << " dec " << fixedOffset.dec << sendLog;
			#endif
				queCommand (new rts2core::CommandChange (this, fixedOffset.ra, fixedOffset.dec), bopTel);
				fixedOffset.ra = 0;
				fixedOffset.dec = 0;
				break;
			}
			queCommand (new rts2core::CommandResyncMove (getMaster (), this, coord.ra, coord.dec), bopTel);
			break;
		case OBS_DONT_MOVE:
			break;
	}
	return ret;
}

void DevClientTelescopeExec::checkInterChange ()
{
	int waitNum = 0;
	getMaster ()->postEvent (new rts2core::Event (EVENT_QUERY_WAIT, (void *) &waitNum));
	if (waitNum == 0)
		getMaster ()->postEvent (new rts2core::Event (EVENT_ENTER_WAIT));
}

void DevClientTelescopeExec::moveEnd ()
{
	if (moveWasCorrecting)
	{
		if (currentTarget)
			currentTarget->moveEnded ();
		getMaster ()->postEvent (new rts2core::Event (EVENT_CORRECTING_OK));
	}
	else
	{
		if (currentTarget)
			currentTarget->moveEnded ();
		getMaster ()->postEvent (new rts2core::Event (EVENT_MOVE_OK));
	}
	DevClientTelescopeImage::moveEnd ();
}

void DevClientTelescopeExec::moveFailed (int status)
{
	if (status == DEVDEM_E_IGNORE)
	{
		moveEnd ();
		return;
	}
	if (currentTarget && currentTarget->moveWasStarted ())
		currentTarget->moveFailed ();
	DevClientTelescopeImage::moveFailed (status);
	getMaster ()->postEvent (new rts2core::Event (EVENT_MOVE_FAILED, (void *) &status));
}
