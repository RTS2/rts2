/* 
 * List of RTS2 XML-RPC methods.
 * Copyright (C) 2008 Petr Kubanek <petr@kubanek.net>
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

#define R2X_DEVICES_LIST              "rts2.devices.list"
#define R2X_DEVICES_VALUES_LIST       "rts2.devices.values.list"

/**
 * Set RTS2 variable. Three parameter must be specified,w ith following meaning:
 *
 * @param device Name of device for which variable will be set.
 * @param var    Name of variable which will be set.
 * @param value  Value as string.
 */
#define R2X_VALUE_SET                 "rts2.value.set"

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
 * List observations for given target.
 *
 * @param tarId  Target id.
 */
#define R2X_TARGET_OBSERVATIONS_LIST         "rts2.target.observations.list"


/**
 * List images which belongs to given observation.
 *
 * @param obsid  Observation ID.
 */
#define R2X_OBSERVATION_IMAGES_LIST          "rts2.observation.images.list"

#define R2X_MESSAGES_GET              "rts2.messages.get"

#define R2X_TICKET_INFO               "rts2.tickets.info"

/**
 * User login. Provides username and password, out true/false - true if login is OK
 */
#define R2X_USER_LOGIN                "rts2.user.login"

/**
 * New user. Provides all user parameters (login, email, password,..), out true/false - true if OK
 */
#define R2X_USER_TELMA_NEW            "rts2.user.telma.new"
#endif							 /* ! __RTS2_R2X__ */
