/**
 * STA Reflex constants, definitions, ..
 * Copyright (C) 2011 Petr Kubanek, Institute of Physics <kubanek@fzu.cz>
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

#ifndef __RTS2_REFLEX_H__
#define __RTS2_REFLEX_H__


#define FIXED_BOARD_COUNT          4
#define MAX_DAUGHTER_COUNT         8
#define TOTAL_BOARD_COUNT         12
#define PARAM_COUNT               64

#define SYSTEM_CONTROL_ADDR       0x10000000
#define SYSTEM_STATUS_ADDR        0x00000000
#define ERROR_SYSTEM_STATUS_ADDR  0x40000000

// Board type values
#define BT_NONE                   0x00
#define BT_BPX6                   0x10
#define BT_CLIF                   0x20
#define BT_PA                     0x30
#define BT_PB                     0x40
#define BT_AD8X120                0x50
#define BT_AD8X100                0x51
#define BT_DRIVER                 0x60
#define BT_BIAS                   0x70
#define BT_HS                     0x80

// System status registers, starting at address 0x00000000
enum SYSTEM_STATUS_FIELDS
{
	ERROR_CODE,
	ERROR_SRC,
	ERROR_LINE,
	STATUS_INDEX,
	POWER,
	BOARD_TYPE_BP,
	BOARD_TYPE_IF,
	BOARD_TYPE_PA,
	BOARD_TYPE_PB,
	BOARD_TYPE_D1,
	BOARD_TYPE_D2,
	BOARD_TYPE_D3,
	BOARD_TYPE_D4,
	BOARD_TYPE_D5,
	BOARD_TYPE_D6,
	BOARD_TYPE_D7,
	BOARD_TYPE_D8,
	ROMID_BP,
	ROMID_IF,
	ROMID_PA,
	ROMID_PB,
	ROMID_D1,
	ROMID_D2,
	ROMID_D3,
	ROMID_D4,
	ROMID_D5,
	ROMID_D6,
	ROMID_D7,
	ROMID_D8,
	BUILD_BP,
	BUILD_IF,
	BUILD_PA,
	BUILD_PB,
	BUILD_D1,
	BUILD_D2,
	BUILD_D3,
	BUILD_D4,
	BUILD_D5,
	BUILD_D6,
	BUILD_D7,
	BUILD_D8,
	FLAGS_BP,
	FLAGS_IF,
	FLAGS_PA,
	FLAGS_PB,
	FLAGS_D1,
	FLAGS_D2,
	FLAGS_D3,
	FLAGS_D4,
	FLAGS_D5,
	FLAGS_D6,
	FLAGS_D7,
	FLAGS_D8,
	SYSTEM_STATUS_SIZE
};

#endif // !__RTS2_REFLEX_H__
