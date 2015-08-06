/**
 * Copyright (C) 2015 Stanislav Vitek
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "multidev.h"
#include "sensord.h"
#include "apm-filter.h"
#include "apm-mirror.h"
#include "apm-relay.h"
#include "connection/apm.h"

using namespace rts2sensord;

int main (int argc, char **argv)
{
	rts2core::MultiDev md;
	rts2filterd::APMFilter *filter = new rts2filterd::APMFilter (argc, argv);
	md.push_back (filter);
	md.push_back (new APMRelay (argc, argv, "R1", filter));
	md.push_back (new APMMirror (argc, argv, "M1", filter));
	return md.run ();
}
