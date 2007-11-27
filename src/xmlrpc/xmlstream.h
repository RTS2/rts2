#ifndef __RTS2__XMLSTREAM__
#define __RTS2__XMLSTREAM__

#include <ostream>
#include "xmlrpc++/XmlRpc.h"
#include "../utils/infoval.h"

using namespace XmlRpc;

class XmlStream: public Rts2InfoValStream
{
	private:
		XmlRpcValue *val;

	public:
		XmlStream (XmlRpcValue *in_val): Rts2InfoValStream ()
		{
			val = in_val;
		}
		virtual void printInfoVal (const char *desc, int val)
		{
			val[desc] = val;
		}
};
#endif							 // !__RTS2__XMLSTREAM__
