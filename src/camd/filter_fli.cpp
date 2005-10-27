/**
 * Copyright (C) 2005 Petr Kubanek <petr@kubanek.net>
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

#include "filter_fli.h"

#include <string.h>

Rts2FilterFli::Rts2FilterFli (char *in_deviceName, flidomain_t in_domain)
{
  deviceName = new char[strlen (in_deviceName) + 1];
  strcpy (deviceName, in_deviceName);
  domain = (in_domain & 0x00ff) | FLIDEVICE_FILTERWHEEL;
  dev = 0;
}

Rts2FilterFli::~Rts2FilterFli (void)
{
  delete[]deviceName;
  if (dev)
    FLIClose (dev);
}

int
Rts2FilterFli::init (void)
{
  LIBFLIAPI ret;

  ret = FLIOpen (&dev, deviceName, domain);
  if (ret)
    return -1;

  ret = FLIGetFilterCount (dev, &filter_count);
  if (ret)
    return -1;

  return 0;
}

int
Rts2FilterFli::getFilterNum (void)
{
  long ret_f;
  LIBFLIAPI ret;
  ret = FLIGetFilterPos (dev, &ret_f);
  if (ret)
    return -1;
  return (int) ret_f;
}

int
Rts2FilterFli::setFilterNum (int new_filter)
{
  LIBFLIAPI ret;
  if (new_filter < 0 || new_filter >= filter_count)
    return -1;

  ret = FLISetFilterPos (dev, new_filter);
  if (ret)
    return -1;
  return 0;
}
