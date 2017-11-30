/*
 * OSX compatibility code
 * Copyright (C) 2017 Sergey Karpov <karpov.sv@gmail.com>
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

#ifndef __RST2_COMPAT_OSX_COMPAT_H__
#define __RST2_COMPAT_OSX_COMPAT_H__

#ifdef __APPLE__

#define ppoll(fds, nfds, tv, extra) poll(fds, nfds, (tv)->tv_sec*1000 + (tv)->tv_nsec/1000000)

#ifndef POLLRDHUP
#define POLLRDHUP 0x2000
#endif

#define syncfs(fd) fsync(fd)

#include <signal.h>

/* Should return non-zero if thread is still running and zero if it is successfully joined */
#define pthread_tryjoin_np(thread, val) ((pthread_kill(thread, 0) != 0) ? pthread_join(thread, val) : 1)

/* endian.h */
#include <libkern/OSByteOrder.h>
#define htobe16(x) OSSwapHostToBigInt16(x)
#define htole16(x) OSSwapHostToLittleInt16(x)
#define be16toh(x) OSSwapBigToHostInt16(x)
#define le16toh(x) OSSwapLittleToHostInt16(x)
#define htobe32(x) OSSwapHostToBigInt32(x)
#define htole32(x) OSSwapHostToLittleInt32(x)
#define be32toh(x) OSSwapBigToHostInt32(x)
#define le32toh(x) OSSwapLittleToHostInt32(x)
#define htobe64(x) OSSwapHostToBigInt64(x)
#define htole64(x) OSSwapHostToLittleInt64(x)
#define be64toh(x) OSSwapBigToHostInt64(x)
#define le64toh(x) OSSwapLittleToHostInt64(x)

#endif /* __APPLE__ */

#endif /* __RST2_COMPAT_OSX_COMPAT_H__ */
