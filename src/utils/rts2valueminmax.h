#ifndef __RTS2_VALUE_MINMAX__
#define __RTS2_VALUE_MINMAX__

#include "rts2value.h"

/**
 * This class is double value with limits.
 */

class Rts2ValueDoubleMinMax:public Rts2ValueDouble
{
	private:
		double min;
		double max;
	public:
		Rts2ValueDoubleMinMax (std::string in_val_name);
		Rts2ValueDoubleMinMax (std::string in_val_name,
			std::string in_description, bool writeToFits =
			true, int32_t flags = 0);

		virtual int setValue (Rts2Conn * connection);
		virtual int doOpValue (char op, Rts2Value * old_value);
		virtual const char *getValue ();
		virtual const char *getDisplayValue ();
		virtual void setFromValue (Rts2Value * newValue);

		void copyMinMax (Rts2ValueDoubleMinMax * from)
		{
			min = from->getMin ();
			max = from->getMax ();
		}

		void setMin (double in_min)
		{
			min = in_min;
		}

		double getMin ()
		{
			return min;
		}

		void setMax (double in_max)
		{
			max = in_max;
		}

		double getMax ()
		{
			return max;
		}
};
#endif							 /* !__RTS2_VALUE_MINMAX__ */
