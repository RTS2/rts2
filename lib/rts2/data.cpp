/* 
 * Utilities for data connection.
 * Copyright (C) 2007,2010 Petr Kubanek <petr@kubanek.net>
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

#include "connection.h"
#include "data.h"

#include <strings.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>

using namespace rts2core;

int DataRead::readDataSize (Connection *conn)
{
	return conn->paramNextSSizeT (&binaryReadChunkSize);
}

int DataAbstractShared::removeClient (int segnum, int client_id, bool verbose)
{
	if (lockSegment (segnum))
		return -1;
	struct SharedDataSegment *sseg = getSegment (segnum);
	for (int s = 0; s < MAX_SHARED_CLIENTS; s++)
	{
		if (sseg->client_ids[s] == client_id)
		{
			sseg->client_ids[s] = 0;
			unlockSegment (segnum);
			return 0;
		}
	}
	unlockSegment (segnum);
	if (verbose)
		logStream (MESSAGE_ERROR) << "cannot find locked client with ID " << client_id << " to remove segment " << segnum << sendLog;
	return -1;
}

int DataAbstractShared::lockSegment (int segnum)
{
	struct sembuf so;
	so.sem_num = segnum;
	so.sem_op = -1;
	so.sem_flg = SEM_UNDO;
	if (semop (data->shared_sem, &so, 1))
	{
		logStream (MESSAGE_ERROR) << "cannot lock segment " << segnum << sendLog;
		return -1;
	}
	return 0;
}

int DataAbstractShared::unlockSegment (int segnum)
{
	struct sembuf so;
	so.sem_num = segnum;
	so.sem_op = 1;
	so.sem_flg = SEM_UNDO;
	if (semop (data->shared_sem, &so, 1))
	{
		logStream (MESSAGE_ERROR) << "cannot unlock segment " << segnum << sendLog;
		return -1;
	}
	return 0;
}

DataSharedRead::~DataSharedRead ()
{
}

int DataSharedRead::attach (int _shm_id)
{
	data = (struct SharedDataHeader *) shmat (_shm_id, NULL, 0);
	if (data == (void *) -1)
	{
		logStream (MESSAGE_ERROR) << "cannot attach to shared memory " << _shm_id << sendLog;
		return -1;
	}
	shm_id = _shm_id;
	return 0;
}


int DataSharedRead::confirmClient (int segnum, int client_id)
{
	if (lockSegment (segnum))
		return -1;
	// confirm there is already allocated client
	struct SharedDataSegment *sseg = getSegment (segnum);
	for (int s = 0; s < MAX_SHARED_CLIENTS; s++)
	{
		if (sseg->client_ids[s] == client_id)
		{
			unlockSegment (segnum);
			return 0;
		}
	}
	unlockSegment (segnum);
	logStream (MESSAGE_ERROR) << "cannot find empty client slot to lock segment " << segnum << sendLog;
	return -1;
}

DataWrite::DataWrite (int _channum, size_t *chansizes):DataAbstractWrite ()
{
	channum = _channum;
	binaryWriteDataSize = new size_t[channum];
	for (int i = 0; i < channum; i++)
		binaryWriteDataSize[i] = chansizes[i];
}

DataWrite::~DataWrite ()
{
	delete[] binaryWriteDataSize;
}

size_t DataWrite::getDataSize ()
{
	size_t ret = 0;
	for (int i = 0; i < channum; i++)
		ret += getChannelSize (i);
	return ret;
}

struct SharedDataHeader *DataSharedWrite::create (int numseg, size_t segsize)
{
	shm_id = shmget (IPC_PRIVATE, sizeof (struct SharedDataHeader) + numseg * (sizeof (struct SharedDataSegment) + segsize), 0666);
	if (shm_id < 0)
	{
		logStream (MESSAGE_ERROR) << "cannot create shared memory segment: " << strerror (errno) << sendLog;
		return NULL;
	}
	data = (struct SharedDataHeader *) shmat (shm_id, NULL, 0);
	if (data == (void *) -1)
	{
		shm_id = -1;
		logStream (MESSAGE_ERROR) << "cannot attach shared memory segment: " << strerror (errno) << sendLog;
		return NULL;
	}
	struct shmid_ds ds;
	if (shmctl (shm_id, IPC_STAT, &ds) < 0 || shmctl (shm_id, IPC_RMID, &ds) < 0)
	{
		shmdt (data);
		shm_id = -1;
		logStream (MESSAGE_ERROR) << "Cannot perform shmctl call: " << strerror (errno) << sendLog;
		return NULL;
	}
	// initalize shared data header
	data->nseg = numseg;
	// first set is for exclusive locks, second is to signal waiting locks
	data->shared_sem = semget (IPC_PRIVATE, numseg, 0666);
	if (data->shared_sem < 0)
	{
		logStream (MESSAGE_ERROR) << "cannot create shared semaphore " << strerror (errno) << sendLog;
		shmdt ((void *) data);
		return NULL;
	}

	struct SharedDataSegment *seg = getSegment (0);
	for (int i = 0; i < numseg; i++, seg++)
	{
		memset (seg->client_ids, 0, sizeof (int) * MAX_SHARED_CLIENTS);
		seg->size = segsize;
		seg->bytesSoFar = 0;
		seg->offset = sizeof (struct SharedDataHeader) + sizeof (struct SharedDataSegment) * numseg + i * segsize;

		struct sembuf so;
		so.sem_num = i;
		so.sem_op = 1;
		semop (data->shared_sem, &so, 1);
	}

	return data;
}

DataSharedWrite::~DataSharedWrite ()
{
	if (shm_id > 0)
	{
		semctl (data->shared_sem, IPC_RMID, 0);
		shmdt (data);
	}
}

size_t DataSharedWrite::getDataSize ()
{
	size_t ret = 0;
	for (std::map <int, struct SharedDataSegment *>::iterator iter = chan2seg.begin (); iter != chan2seg.end (); iter++)
		ret += getChannelSize (iter->first);
	return ret;
}

int DataSharedWrite::addClient (size_t segsize, int chan, int client)
{
	for (int i = 0; i < data->nseg; i++)
	{
		lockSegment (i);
		struct SharedDataSegment *sseg = getSegment (i);
		int s;
		for (s = 0; s < MAX_SHARED_CLIENTS; s++)
		{
			if (sseg->client_ids[s] > 0)
				break;
		}
		if (s == MAX_SHARED_CLIENTS)
		{
			sseg->bytesSoFar = 0;
			sseg->size = segsize;
			sseg->client_ids[0] = client;
			chan2seg[chan] = sseg; 
			unlockSegment (i);
			return i;
		}
		unlockSegment (i);
	}
	return -1;
}

void DataSharedWrite::endChannels ()
{
	for (std::map <int, struct SharedDataSegment *>::iterator iter = chan2seg.begin (); iter != chan2seg.end (); iter++)
	{
		iter->second->bytesSoFar = iter->second->size;
	}
}

DataChannels::~DataChannels ()
{
	for (DataChannels::iterator iter = begin (); iter != end (); iter++)
		delete (*iter);
}

void DataChannels::initFromConnection (Connection *conn)
{
	int data_type, channum;
	if (conn->paramNextInteger (&data_type) || conn->paramNextInteger (&channum))
		throw Error ("cannot read number of channels");
	for (int i = 0; i < channum; i++)
	{
		long chansize;
		if (conn->paramNextLong (&chansize))
			throw Error ("cannot parse channel size");
		push_back (new DataRead (chansize, data_type));
	}
	if (!conn->paramEnd ())
		throw Error ("too much parameters in PROTO_BINARY data header");
}

void DataChannels::initSharedFromConnection (Connection *conn, DataSharedRead *shm)
{
	int channum;
	if (conn->paramNextInteger (&channum))
		throw Error ("cannot read number of channels");
	for (int i = 0; i < channum; i++)
	{
		int seg;
		if (conn->paramNextInteger (&seg))
			throw Error ("cannot parse channel segment or size");
		push_back (new DataSharedRead (shm, seg));
	}
	if (!conn->paramEnd ())
		throw Error ("too much parameters in PROTO_SHARED command");
}

size_t DataChannels::getRestSize ()
{
	long ret = 0;
	for (DataChannels::iterator iter = begin (); iter != end (); iter++)
		ret += (*iter)->getRestSize ();
	return ret;	
}
