#ifndef __RTS2_LOGSTREAM__
#define __RTS2_LOGSTREAM__

#include <sstream>

#include <message.h>

class Rts2App;

class Rts2LogStream
{
	private:
		Rts2App * masterApp;
		messageType_t messageType;
		std::ostringstream ls;
	public:

		Rts2LogStream (Rts2App * in_master, messageType_t in_type)
		{
			masterApp = in_master;
			messageType = in_type;
			ls.setf (std::ios_base::fixed, std::ios_base::floatfield);
			ls.precision (6);
		}

		Rts2LogStream (Rts2LogStream & in_logStream)
		{
			masterApp = in_logStream.masterApp;
			messageType = in_logStream.messageType;
			ls.setf (std::ios_base::fixed, std::ios_base::floatfield);
			ls.precision (6);
		}

		Rts2LogStream & operator << (Rts2LogStream & (*func) (Rts2LogStream &))
		{
			return func (*this);
		}

		template < typename _charT > Rts2LogStream & operator << (_charT value)
		{
			ls << value;
			return *this;
		}

		inline void sendLog ();
};

Rts2LogStream & sendLog (Rts2LogStream & _ls);
#endif							 /* ! __RTS2_LOGSTREAM__ */
