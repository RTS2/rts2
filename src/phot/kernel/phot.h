
/***************************************************************************\

    Copyright (c) 2004 Petr Kubánek

    All rights reserved.

    Permission is hereby granted, free of charge, to any person obtaining a
    copy of this software and associated documentation files (the
    "Software"), to deal in the Software without restriction, including
    without limitation the rights to use, copy, modify, merge, publish,
    distribute, and/or sell copies of the Software, and to permit persons
    to whom the Software is furnished to do so, provided that the above
    copyright notice(s) and this permission notice appear in all copies of
    the Software and that both the above copyright notice(s) and this
    permission notice appear in supporting documentation.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
    OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
    MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT
    OF THIRD PARTY RIGHTS. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
    HOLDERS INCLUDED IN THIS NOTICE BE LIABLE FOR ANY CLAIM, OR ANY SPECIAL
    INDIRECT OR CONSEQUENTIAL DAMAGES, OR ANY DAMAGES WHATSOEVER RESULTING
    FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT,
    NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION
    WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

    Except as contained in this notice, the name of a copyright holder
    shall not be used in advertising or otherwise to promote the sale, use
    or other dealings in this Software without prior written authorization
    of the copyright holder.

\***************************************************************************/

/*
 * phot.h - defines IO and other stuff for photometer operations.
 *
 */

#ifndef _PHOT_H_
#define _PHOT_H

/*
 * Status field - integrating/not integrating
 */

#define PHOT_S_INTEGRATING	0x01
#define PHOT_S_FILTERMOVE	0x02
#define PHOT_S_INTEGRATING_ONCE	0x04

#define PHOT_CMD_RESET		'A'	//0x00
#define PHOT_CMD_INTEGRATE_ONCE	'B'	//0x01
#define PHOT_CMD_INTEGRATE	'C'	//0x02
#define PHOT_CMD_STOP_INTEGRATE	'D'	//0x03
#define PHOT_CMD_MOVEFILTER	'E'	//0x04
#define PHOT_CMD_INTEGR_ENABLED 'F'

#endif /* _PHOT_H_ */
