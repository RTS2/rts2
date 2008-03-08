#include "rts2displayvalue.h"
#include <sstream>
#include <iomanip>

std::string
getDisplayValue (Rts2Value * value)
{
	std::ostringstream _os;
	const char *tmp_val;
	switch (value->getValueDisplayType ())
	{
		case RTS2_DT_RA:
			_os << LibnovaRa (value->getValueDouble ());
			break;
		case RTS2_DT_DEC:
			_os << LibnovaDeg90 (value->getValueDouble ());
			break;
		case RTS2_DT_DEGREES:
			_os << LibnovaDeg (value->getValueDouble ());
			break;
		case RTS2_DT_DEG_DIST:
			_os << LibnovaDegDist (value->getValueDouble ());
			break;
		case RTS2_DT_PERCENTS:
			_os << std::setw (6) << value->getValueDouble () << "%";
			break;
		case RTS2_DT_HEX:
			_os << std::setw(8) << std::hex << value->getValueInteger ();
			break;
		default:
			tmp_val = value->getDisplayValue ();
			if (tmp_val)
				return std::string (tmp_val);
			return std::string ("");
	}
	return _os.str ();
}
