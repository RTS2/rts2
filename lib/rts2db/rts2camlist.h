#ifndef __RTS2_CAMLIST__
#define __RTS2_CAMLIST__

#include <list>
#include <string>

class Rts2CamList:public std::list < std::string >
{
	public:
		Rts2CamList ();
		virtual ~Rts2CamList ();
		
		int load ();
};
#endif	 /* ! __RTS2_CAMLIST__ */
