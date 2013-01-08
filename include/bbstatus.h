/* 
 * Basic constants.
 * Copyright (C) 2013 Petr Kubanek <petr@kubanek.net>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

// observatory status
// 0x003F is observatory status bits, see SERVER_xxx status
#define BB_MOUNT_OK               0x0040
#define BB_FILTER_OK              0x0080
#define BB_FOCUSER_OK             0x0100
#define BB_CCDS_OK                0x0200
#define BB_DOME_OK                0x0400
#define BB_DOME_CLOSED            0x0800
#define BB_BAD_WEATHER            0x1000
