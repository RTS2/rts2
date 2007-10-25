#ifndef __RTS2_INFOVAL__
#define __RTS2_INFOVAL__

#include <ostream>
#include <iomanip>

#include "libnova_cpp.h"
#include "timestamp.h"

template < class val > class InfoVal
{
	private:
		const char *desc;
		val value;

		template < class v > friend std::ostream & operator<< (std::ostream & _os,
			InfoVal < v > _val);
	public:
		InfoVal (const char *in_desc, val in_value)
		{
			desc = in_desc;
			value = in_value;
		}

};

template < class v >
std::ostream & operator << (std::ostream & _os, InfoVal < v > _val)
{
	_os << std::left << std::setw (20) << _val.desc << std::right << _val.value
		<< std::endl;
	return _os;
}
#endif
