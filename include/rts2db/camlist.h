#ifndef __RTS2_CAMLIST__
#define __RTS2_CAMLIST__

#include <list>
#include <string>

namespace rts2db
{

class CamList:public std::list < std::string >
{
	public:
		CamList ();
		virtual ~CamList ();
		
		int load ();
};

}
#endif	 /* ! __RTS2_CAMLIST__ */
