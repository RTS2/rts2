
#include "XmlRpcClient.h"
#include "XmlRpcDispatch.h"
#include "XmlRpcSource.h"
#include "XmlRpcUtil.h"

#include <math.h>
#include <sys/timeb.h>

#if defined(_WINDOWS)
# include <winsock2.h>

# define USE_FTIME
# if defined(_MSC_VER)
#  define timeb _timeb
#  define ftime _ftime
# endif
#else
# include <sys/time.h>
#endif							 // _WINDOWS

using namespace XmlRpc;

XmlRpcDispatch::XmlRpcDispatch()
{
	_endTime = -1.0;
	_doClear = false;
	_inWork = false;
}

XmlRpcDispatch::~XmlRpcDispatch()
{
}

// Monitor this source for the specified events and call its event handler
// when the event occurs
void XmlRpcDispatch::addSource(XmlRpcSource* source, unsigned mask)
{
	_sources.push_back(MonitoredSource(source, mask));
}


// Stop monitoring this source. Does not close the source.
void XmlRpcDispatch::removeSource(XmlRpcSource* source)
{
	for (SourceList::iterator it=_sources.begin(); it!=_sources.end(); ++it)
		if (it->getSource() == source)
		{
			_sources.erase(it);
			break;
		}
}


// Modify the types of events to watch for on this source
void XmlRpcDispatch::setSourceEvents(XmlRpcSource* source, unsigned eventMask)
{
	for (SourceList::iterator it=_sources.begin(); it!=_sources.end(); ++it)
		if (it->getSource() == source)
		{
			it->getMask() = eventMask;
			return;
		}
	// if not found, add it
	addSource(source, eventMask);	
}

#define MAX_POLLS	200

// Watch current set of sources and process events
void XmlRpcDispatch::work(double timeout, XmlRpcClient *chunkWait)
{
	// Compute end time
	_endTime = (timeout < 0.0) ? -1.0 : (getTime() + timeout);
	_doClear = false;
	_inWork = true;

	if (chunkWait)
	{
		setSourceEvents(chunkWait, ReadableEvent | WritableEvent | Exception);
	}

	// Only work while there is something to monitor
	while (_sources.size() > 0)
	{

		// Construct the sets of descriptors we are interested in
		struct pollfd fds[MAX_POLLS];
		nfds_t nfds = 0;

	//	addToFd (fds, nfds);
	//	fds[nfds].fd = -1;

		// Check for events
		int nEvents;
		if (timeout < 0.0)
			nEvents = poll(fds, nfds, 0);
		else
		{
			struct timespec tv;
			tv.tv_sec = (int)floor(timeout);
			tv.tv_nsec = ((int)floor(1000000.0 * (timeout-floor(timeout)))) % 1000000;
			nEvents = ppoll(fds, nfds, &tv, NULL);
		}

		if (nEvents < 0)
		{
			XmlRpcUtil::error("Error in XmlRpcDispatch::work: error in select (%d).", nEvents);
			_inWork = false;
			return;
		}

	//	checkFd (fds, chunkWait);

		// Check whether to clear all sources
		if (_doClear)
		{
			SourceList closeList = _sources;
			_sources.clear();
			for (SourceList::iterator it=closeList.begin(); it!=closeList.end(); ++it)
			{
				XmlRpcSource *src = it->getSource();
				src->close();
			}

			_doClear = false;
		}

		// Check whether end time has passed
		if (0 <= _endTime && getTime() > _endTime)
			break;

		// if chunkWait and the connection received chunk..
		if (chunkWait && chunkWait->gotChunk ())
			break;
	}

	_inWork = false;
}

void XmlRpcDispatch::addToFd (void (*addFD) (int, short))
{
	SourceList::iterator it;
	for (it=_sources.begin(); it!=_sources.end(); ++it)
	{
		short events = 0;
		if (it->getMask() & ReadableEvent) events |= POLLIN | POLLPRI;
		if (it->getMask() & WritableEvent) events |= POLLOUT;
		if (it->getMask() & Exception)     events |= POLLRDHUP | POLLERR | POLLHUP | POLLNVAL;
		if (events != 0)
			addFD (it->getSource()->getfd(), events);
	}
}

void XmlRpcDispatch::checkFd (short (*getFDEvents) (int), XmlRpcSource *chunkWait)
{
	SourceList::iterator it;
	// Process events
	for (it=_sources.begin(); it != _sources.end(); )
	{
		SourceList::iterator thisIt = it++;
		XmlRpcSource* src = thisIt->getSource();
		int fd = src->getfd();
		short revents = getFDEvents(fd);
		if (revents == 0)
			continue;

		unsigned newMask = (unsigned) -1;
		// If you select on multiple event types this could be ambiguous
		try
		{
			if (revents & (POLLIN | POLLPRI))
				newMask &= (src == chunkWait) ? src->handleChunkEvent(ReadableEvent) : src->handleEvent(ReadableEvent);
		}
		catch (const XmlRpcAsynchronous &async)
		{
			XmlRpcUtil::log(3, "Asynchronous event while handling response.");
			// stop monitoring the source..
			thisIt->getMask() = 0;
			src->goAsync ();
		}

		if (revents & POLLOUT)
			newMask &= (src == chunkWait) ? src->handleChunkEvent(WritableEvent) : src->handleEvent(WritableEvent);
		if (revents & (POLLRDHUP | POLLERR | POLLHUP | POLLNVAL))
			newMask &= (src == chunkWait) ? src->handleChunkEvent(Exception) : src->handleEvent(Exception);

		if ( ! newMask)
		{
			// Stop monitoring this one
			_sources.erase(thisIt);
			if ( ! src->getKeepOpen())
				src->close();
		}
		else if (newMask != (unsigned) -1)
		{
			thisIt->getMask() = newMask;
		}
	}
}

// Exit from work routine. Presumably this will be called from
// one of the source event handlers.
void XmlRpcDispatch::exitWork()
{
	_endTime = 0.0;				 // Return from work asap
}

// Clear all sources from the monitored sources list
void XmlRpcDispatch::clear()
{
	if (_inWork)
		_doClear = true;		 // Finish reporting current events before clearing
	else
	{
		SourceList closeList = _sources;
		_sources.clear();
		for (SourceList::iterator it=closeList.begin(); it!=closeList.end(); ++it)
			it->getSource()->close();
	}
}

double XmlRpcDispatch::getTime()
{
	#ifdef USE_FTIME
	struct timeb  tbuff;

	ftime(&tbuff);
	return ((double) tbuff.time + ((double)tbuff.millitm / 1000.0) +
		((double) tbuff.timezone * 60));
	#else
	struct timeval  tv;
	struct timezone tz;

	gettimeofday(&tv, &tz);
	return (tv.tv_sec + tv.tv_usec / 1000000.0);
	#endif						 /* USE_FTIME */
}
