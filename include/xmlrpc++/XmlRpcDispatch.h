
#ifndef _XMLRPCDISPATCH_H_
#define _XMLRPCDISPATCH_H_
//
// XmlRpc++ Copyright (c) 2002-2003 by Chris Morley
//
#if defined(_MSC_VER)
# pragma warning(disable:4786)	 // identifier was truncated in debug info
#endif

#ifndef MAKEDEPEND
# include <list>
#endif

#include <malloc.h>
#include <sys/select.h>

namespace XmlRpc
{

	// An RPC source represents a file descriptor to monitor
	class XmlRpcSource;

	class XmlRpcClient;

	//! Thrown when execute should not return response - mark asynchronous connection,
	//  where response is received later.
	class XmlRpcAsynchronous
	{
		public:
			XmlRpcAsynchronous () {};
	};

	//! An object which monitors file descriptors for events and performs
	//! callbacks when interesting events happen.
	class XmlRpcDispatch
	{
		public:
			//! Constructor
			XmlRpcDispatch();
			~XmlRpcDispatch();

			//! Values indicating the type of events a source is interested in
			enum EventType
			{
								 //!< data available to read
				ReadableEvent = 1,
								 //!< connected/data can be written without blocking
				WritableEvent = 2,
				Exception     = 4//!< uh oh
			};

			//! Monitor this source for the event types specified by the event mask
			//! and call its event handler when any of the events occur.
			//!  @param source The source to monitor
			//!  @param eventMask Which event types to watch for. \see EventType
			void addSource(XmlRpcSource* source, unsigned eventMask);

			//! Stop monitoring this source.
			//!  @param source The source to stop monitoring
			void removeSource(XmlRpcSource* source);

			//! Modify the types of events to watch for on this source
			void setSourceEvents(XmlRpcSource* source, unsigned eventMask);

			//! Watch current set of sources and process events for the specified
			//! duration (in ms, -1 implies wait forever, or until exit is called)
			//!  @param chunkWait if not null, work until a chunk is available on the given connection
			void work(double msTime, XmlRpcClient *chunkWait = NULL);

			//! Add sockets to file descriptor set
			void addToFd (fd_set *inFd, fd_set *outFd, fd_set *excFd);

			void checkFd (fd_set *inFd, fd_set *outFd, fd_set *excFd);

			//! Exit from work routine
			void exitWork();

			//! Clear all sources from the monitored sources list. Sources are closed.
			void clear();

		protected:

			// helper
			double getTime();

			// A source to monitor and what to monitor it for
			struct MonitoredSource
			{
				MonitoredSource(XmlRpcSource* src, unsigned mask) : _src(src), _mask(mask) {}
				XmlRpcSource* getSource() const { return _src; }
				unsigned& getMask() { return _mask; }
				XmlRpcSource* _src;
				unsigned _mask;
			};

			// A list of sources to monitor
			typedef std::list< MonitoredSource > SourceList;

			// Sources being monitored
			SourceList _sources;

			// When work should stop (-1 implies wait forever, or until exit is called)
			double _endTime;

			bool _doClear;
			bool _inWork;

	};
}								 // namespace XmlRpc
#endif							 // _XMLRPCDISPATCH_H_
