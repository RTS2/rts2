/* 
 * List of RTS2 XML-RPC methods.
 * Copyright (C) 2008 Petr Kubanek <petr@kubanek.net>
 * Copyright (C) 2010 Petr Kubanek <kubanek@fzu.cz> Institute of Physics, Czech Republic
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

#ifndef __RTS2_R2X__
#define __RTS2_R2X__

/**
 * Return session id for new user, if login is allowed.
 *
 * @param login     User login name.
 * @param password  Password for login.
 * @return SessionID
 */
#define R2X_LOGIN	              "rts2.login"

/**
 * Return master state.
 */
#define R2X_MASTER_STATE              "rts2.master.state"

#define R2X_MASTER_STATE_IS           "rts2.master.is_state"

/**
 * Return array with name of devices presented in the system.
 *
 * @return Array with names of devices presented in the system.
 */
#define R2X_DEVICES_LIST              "rts2.devices.list"

/**
 * Return array with name(s) of device(s) with given type.
 *
 * @return Array with name(s) fo devices mathing the type.
 */
#define R2X_DEVICE_GET_BY_TYPE        "rts2.device.get_by_type"

/**
 * Return type of the device.
 *
 * @param Device name.
 *
 * @return Device type string (camera, telescope, ..).
 */
#define R2X_DEVICE_TYPE               "rts2.device.type"

/**
 * Execute command on device.
 * @param device  Name of device which will receive the command.
 * @param command Command as passed to device, with all parameters.
 */
#define R2X_DEVICE_COMMAND            "rts2.device.command"

/**
 * Return device status. If you would like to get state of system (=centrald), specify as device name "centrald".
 *
 * @param Device name.
 *
 * @return two arguments - first is device state as string, second as its numeric representation.
 */
#define R2X_DEVICE_STATE              "rts2.device.state"

#define R2X_DEVICES_VALUES_LIST       "rts2.devices.values.list"

#define R2X_DEVICES_VALUES_PRETTYLIST "rts2.devices.values.prettylist"

/**
 * Return RTS2 variable. Two  parameters must be specified, with following meaning:
 *
 * @param device Name of device for which variable will be set.
 * @param var    Name of variable which will be set.
 */
#define R2X_VALUE_GET                 "rts2.value.get"

/**
 * Return pretty printed RTS2 variable. Two  parameters must be specified, with following meaning:
 *
 * @param device Name of device for which variable will be set.
 * @param var    Name of variable which will be set.
 */
#define R2X_VALUE_PRETTY              "rts2.value.pretty"

/**
 * Set RTS2 variable. Three parameters must be specified, with following meaning:
 *
 * @param device Name of device for which variable will be set.
 * @param var    Name of variable which will be set.
 * @param value  Value as string.
 */
#define R2X_VALUE_SET                 "rts2.value.set"

/**
 * Set all variables for a given device class. Please see rts2/include/status.h
 * for list of device types values (DEVICE_TYPE_xxxx defines).
 *
 * @param type  Type of the device as number. See rts2/include/status.h for assigned numbers.
 * @param var   Name of variable which will be set.
 * @param value New variable value.
 */
#define R2X_VALUE_BY_TYPE_SET         "rts2.value_by_type.set"

/**
 * Increment RTS2 variable. Three parameters must be specified, with following meaning:
 * @param device Name of device for which variable will be set.
 * @param var    Name of variable which will be set.
 * @param value  Value increment as string.
 */
#define R2X_VALUE_INC                 "rts2.value.inc"

/**
 * Retrieve messages from message buffer.
 *
 * @return Array with string - messages from buffer.
 */
#define R2X_MESSAGES_GET              "rts2.messages.get"

#define R2X_VALUES_LIST               "rts2.values.list"

/**
 * List all available targets.
 */
#define R2X_TARGETS_LIST              "rts2.targets.list"

/**
 * List targets by type.
 *
 * @param Array of types which will be listed.
 */
#define R2X_TARGETS_TYPE_LIST         "rts2.targets.type.list"


#define R2X_TARGETS_INFO              "rts2.targets.info"

/**
 * Return altitude of object during given night.
 *
 * @param tarid     Target id.
 * @param from      Computer time (seconds from 1970) of altitude information start.
 * @param to        Computer time (seconds from 1970) of altitude information end.
 * @param stepsize  Size of steps between altitudes in seconds.
 *
 * @return Array
 */
#define R2X_TARGET_ALTITUDE           "rts2.target.altitude"

/**
 * List observations for given target.
 *
 * @param tarId  Target id.
 */
#define R2X_TARGET_OBSERVATIONS_LIST         "rts2.target.observations.list"

/**
 * Return single observation.
 *
 * @param obsid  Observation ID.
 */
#define R2X_OBSERVATION_INFO                 "rts2.observation.info"

/**
 * List images which belongs to given observation.
 *
 * @param obsid  Observation ID.
 */
#define R2X_OBSERVATION_IMAGES_LIST          "rts2.observation.images.list"

/**
 * Provides observation statistics for given month.
 *
 * @param year   Year (ussually > 2000)
 * @param month  Month (1-12)
 */
#define R2X_OBSERVATIONS_MONTH               "rts2.observations.month"

#define R2X_TICKET_INFO               "rts2.tickets.info"

/**
 * List values which are recored to database.
 *
 * @return array with structure containing informations about each record:
 *    recvalid,device, value, from, to, recnums
 */
#define R2X_RECORDS_VALUES            "rts2.records.values"

/**
 * Retrieve records from database.
 *
 * @param id           Value to return
 * @param from         From this time
 * @param to           To this time
 */
#define R2X_RECORDS_GET               "rts2.records.get"

/**
 * Returns statistical values.
 *
 * @param Id           Value to return
 * @param From         From this time
 * @param To           To this time
 * @return Array of five values - middle time, average, minimal and maximal values, and number of records.
 */
#define R2X_RECORDS_AVERAGES          "rts2.records.averages"

/**
 * User login. Provides username and password, out true/false - true if login is OK
 */
#define R2X_USER_LOGIN                "rts2.user.login"

/**
 * New user. Provides all user parameters (login, email, password,..), out true/false - true if OK
 */
#define R2X_USER_TELMA_NEW            "rts2.user.telma.new"

/**
 * Update BB record about observatory.
 *
 * @param observatory  name of observatory which record will be updated
 * @param data Observarory data. Has of devices followed by variables.
 */
#define R2X_BB_UPDATE_OBSERVATORY     "rts2-bb.update-observatory"
#endif							 /* ! __RTS2_R2X__ */
