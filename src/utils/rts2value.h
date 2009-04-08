/* 
 * Various value classes.
 * Copyright (C) 2003-2008 Petr Kubanek <petr@kubanek.net>
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

#ifndef __RTS2_VALUE__
#define __RTS2_VALUE__

#include <limits.h>
#include <string.h>
#include <string>
#include <math.h>
#include <time.h>
#include <vector>
#include <iostream>

#include <libnova/libnova.h>

/**
 * @file Various value classes.
 *
 * @defgroup RTS2Value RTS2 Values
 */

/**
 * Value is string (character array).
 * @ingroup RTS2Value
 */
#define RTS2_VALUE_STRING             0x00000001

/**
 * Value is integer number. 
 * @ingroup RTS2Value
 */
#define RTS2_VALUE_INTEGER            0x00000002

/**
 * Value is time (date and time, distributed as double precission floating point
 * number, representing number of seconds from 1.1.1970) 
 * @ingroup RTS2Value
 */
#define RTS2_VALUE_TIME               0x00000003

/**
 * Value is double precission floating point number.
 * @ingroup RTS2Value
 */
#define RTS2_VALUE_DOUBLE             0x00000004

/**
 * Value is single precission floating point number.
 * @ingroup RTS2Value
 */
#define RTS2_VALUE_FLOAT              0x00000005

/**
 * Value is boolean value (true or false).
 * @ingroup RTS2Value
 */
#define RTS2_VALUE_BOOL               0x00000006

/**
 * Value is selection value. Ussuall represnetation is integer number, but string representation is provided as well.
 * @ingroup RTS2Value
 */
#define RTS2_VALUE_SELECTION          0x00000007

/**
 * Value is long integer value.
 * @ingroup RTS2Value
 */
#define RTS2_VALUE_LONGINT            0x00000008

/**
 * Value is RA DEC.
 * @ingroup RTS2Value
 */
#define RTS2_VALUE_RADEC              0x00000009


/**
 * Value is Alt Az
 * @ingroup RTS2Value
 */
#define RTS2_VALUE_ALTAZ              0x0000000A

/**
 * Value have statistics nature (include mean, average, min and max values and number of measurements taken for value).
 * @ingroup RTS2Value
 */
#define RTS2_VALUE_STAT               0x00000010

/**
 * Value is min-max value, which puts boundaries on minimal and maximal values.
 * @ingroup RTS2Value
 */
#define RTS2_VALUE_MMAX               0x00000020
/**
 * Value holds rectangle - 4 values of same type (X,Y,w,h).
 * @ingroup RTS2Value
 */
#define RTS2_VALUE_RECTANGLE          0x00000030

/**
 * Value is an array of strings.
 * @ingroup RTS2Value
 */
#define RTS2_VALUE_ARRAY              0x00000040

/**
 * Base type mask.
 * @ingroup RTS2Value
 */
#define RTS2_BASE_TYPE                0x0000000f

#define RTS2_VALUE_MASK               0x000000ff

#define RTS2_EXT_TYPE                 0x000000f0

/**
 * When this bit is set, value is writen to FITS file.
 * @ingroup RTS2Value
 */
#define RTS2_VALUE_FITS               0x00000100

/**
 * When this bit is set, value is written to FITS file
 * with device name used as prefix.
 * @ingroup RTS2Value
 */
#define RTS2_VALUE_DEVPREFIX          0x00000200

/**
 * When this bit is set, value was changed from last valueChanged reset.
 * @ingroup RTS2Value
 */
#define RTS2_VALUE_CHANGED            0x00000400

/**
 * When set, writer will add to FITS header key with name <value_key>.CHANGED, That FITS key will be true,
 * if value changed during exposure. It will be false if value do not changed during exposure.
 * @ingroup RTS2Value
 */
#define RTS2_VWHEN_RECORD_CHANGE      0x00000800

#define RTS2_VWHEN_MASK               0x0000f000

#define RTS2_VWHEN_BEFORE_EXP         0x00000000
#define RTS2_VWHEN_AFTER_START        0x00001000
#define RTS2_VWHEN_BEFORE_END         0x00002000
#define RTS2_VWHEN_AFTER_END          0x00003000

#define RTS2_TYPE_MASK                0x00ff0000
#define RTS2_DT_RA                    0x00010000
#define RTS2_DT_DEC                   0x00020000
#define RTS2_DT_DEGREES               0x00030000
#define RTS2_DT_DEG_DIST              0x00040000
#define RTS2_DT_PERCENTS              0x00050000
#define RTS2_DT_ROTANG                0x00060000
#define RTS2_DT_HEX                   0x00070000
#define RTS2_DT_BYTESIZE              0x00080000

#define RTS2_VALUE_INFOTIME           "infotime"

/**
 * Script value, when we will display it, we might look for scriptPosition and
 * scriptLen, which will show current script position.
 */
#define RTS2_DT_SCRIPT                0x00100000


/**
 * If set. value is read-write. When not set, value is read only.
 */
#define RTS2_VALUE_WRITABLE           0x01000000

#define VALUE_BUF_LEN                 200

// BOP mask is taken from status.h, and occupied highest byte (0xff000000)

#include <status.h>

class Rts2Conn;

/**
 * Holds values send over TCP/IP.
 * Values which are in RTS2 system belongs to one component. They are
 * distributed with metainformations over TCP/IP network, and stored in
 * receiving devices.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 *
 * @ingroup RTS2Block
 * @ingroup RTS2Value
 */
class Rts2Value
{
	private:
		std::string valueName;
		std::string description;
	protected:
		char buf[VALUE_BUF_LEN];
		int32_t rts2Type;

		void setValueFlags (int32_t flags)
		{
			rts2Type |= (RTS2_TYPE_MASK | RTS2_VWHEN_MASK | RTS2_VWHEN_RECORD_CHANGE | RTS2_VALUE_DEVPREFIX) & flags;
		}

		/**
		 * Send values meta infomration. @see sendMetaInfo adds to it actual value.
		 */
		virtual int sendTypeMetaInfo (Rts2Conn * connection);
	public:
		/**
		 * Create value. RA and DEC names will be composed by suffixing
		 * in_val_name with RA and DEC strings.
		 */
		Rts2Value (std::string _val_name);

		/**
		 * Create value. RA and DEC names will be composed by suffixing
		 * in_val_name with RA and DEC strings.
		 */
		Rts2Value (std::string _val_name, std::string _description, bool writeToFits = true, int32_t flags = 0);
		virtual ~ Rts2Value (void)
		{
		}

		int isValue (const char *in_val_name)
		{
			return !strcmp (in_val_name, valueName.c_str ());
		}

		std::string getName ()
		{
			return valueName;
		}

		/**
		 * Sets value from connection.
		 *
		 * @param connection Connection which contains command to set value.
		 *
		 * @return -2 on error, 0 on success.
		 */
		virtual int setValue (Rts2Conn * connection) = 0;

		/**
		 * Set value from string.
		 *
		 * @param
		 *
		 * @return -1 when value cannot be set from string.
		 */
		virtual int setValueCharArr (const char *_value) = 0;

		/**
		 * Return value as string.
		 *
		 * @return String which contains value.
		 */
		virtual const char *getValue () = 0;

		/**
		 * Set value from other value.
		 *
		 * @param newValue Value which content will be copied.
		 */
		virtual void setFromValue (Rts2Value * newValue) = 0;

		/**
		 * Returns true, if value is equal to other value.
		 *
		 * @return True if value is equal to other value, otherwise false.
		 */
		virtual bool isEqual (Rts2Value *other_value) = 0;

		int setValueString (std::string in_value)
		{
			return setValueCharArr (in_value.c_str ());
		}

		virtual int setValueInteger (int in_value)
		{
			return -1;
		}

		/**
		 * Performs operation on value.
		 *
		 * @param op Operator character. Currently are supoprted =, + and -.
		 * @param old_value Operand. Ussually any value of any type.
		 *
		 * @return -1 on error, 0 on success.
		 */
		virtual int doOpValue (char op, Rts2Value * old_value);

		/**
		 * Return value as displayed in rts2-mon and other
		 * applications.
		 *
		 * @return String with human-readable value.
		 */
		virtual const char *getDisplayValue ()
		{
			return getValue ();
		}

		int32_t getValueWriteFlags ()
		{
			return rts2Type & RTS2_VWHEN_MASK;
		}

		bool writeWhenChanged ()
		{
			return rts2Type & RTS2_VWHEN_RECORD_CHANGE;
		}

		int32_t getValueDisplayType ()
		{
			return rts2Type & RTS2_TYPE_MASK;
		}
		int32_t getBopMask ()
		{
			return rts2Type & BOP_MASK;
		}
		virtual double getValueDouble ()
		{
			return nan ("f");
		}
		virtual float getValueFloat ()
		{
			return nan ("f");
		}
		virtual int getValueInteger ()
		{
			return -1;
		}
		virtual long int getValueLong ()
		{
			return getValueInteger ();
		}

		std::string getDescription ()
		{
			return description;
		}

		const int getValueType ()
		{
			return rts2Type & RTS2_VALUE_MASK;
		}

		const int getValueExtType ()
		{
			return rts2Type & RTS2_EXT_TYPE;
		}

		/**
		 * Return value base type. This will be different from
		 * value type for composite types (min-max, rectangles,..)
		 *
		 * @return Value base type.
		 */
		const int getValueBaseType ()
		{
			return rts2Type & RTS2_BASE_TYPE;
		}

		void setWriteToFits ()
		{
			rts2Type |= RTS2_VALUE_FITS;
		}

		bool getWriteToFits ()
		{
			return (rts2Type & RTS2_VALUE_FITS);
		}

		bool prefixWithDevice ()
		{
			return (rts2Type & RTS2_VALUE_DEVPREFIX);
		}

		int32_t getFlags ()
		{
			return rts2Type;
		}

		/**
		 * Sends value metainformations, including description.
		 *
		 * @param connection Connection over which value will be send.
		 * @return -1 on error, 0 on success.
		 */
		int sendMetaInfo (Rts2Conn * connection);

		/**
		 * Sends value over given connection.
		 *
		 * @param connection Connection on which value will be send.
		 */
		virtual void send (Rts2Conn * connection);

		/**
		 * Reset value change bit, so changes will be recorded from now on.
		 *
		 * @see wasChanged()
		 */
		virtual void resetValueChanged ()
		{
			rts2Type &= ~RTS2_VALUE_CHANGED;
		}

		/**
		 * Return true if value was changed from last call of resetValueChanged().
		 *
		 * @see resetValueChanged()
		 */
		bool wasChanged ()
		{
			return rts2Type & RTS2_VALUE_CHANGED;
		}

		/**
		 * Set value change flag.
		 */
		void changed ()
		{
			rts2Type |= RTS2_VALUE_CHANGED;
		}
};

/**
 * Class which holds string value.
 *
 * @ingroup RTS2Value
 */
class Rts2ValueString:public Rts2Value
{
	private:
		std::string value;
	public:
		Rts2ValueString (std::string in_val_name);
		Rts2ValueString (std::string in_val_name, std::string in_description,
			bool writeToFits = true, int32_t flags = 0);
		virtual ~ Rts2ValueString (void)
		{
		}
		virtual int setValue (Rts2Conn * connection);
		virtual int setValueCharArr (const char *in_value);
		virtual int setValueInteger (int in_value);
		virtual const char *getValue ();
		virtual void send (Rts2Conn * connection);
		virtual void setFromValue (Rts2Value * newValue);
		virtual bool isEqual (Rts2Value *other_value);
};

/**
 * Class which holds integre value.
 *
 * @ingroup RTS2Value
 */
class Rts2ValueInteger:public Rts2Value
{
	private:
		int value;
	public:
		Rts2ValueInteger (std::string in_val_name);
		Rts2ValueInteger (std::string in_val_name, std::string in_description,
			bool writeToFits = true, int32_t flags = 0);
		virtual int setValue (Rts2Conn * connection);
		virtual int setValueCharArr (const char *in_value);
		/**
		 * Returns -1 on error
		 *
		 */
		virtual int setValueInteger (int in_value)
		{
			if (value != in_value)
			{
				changed ();
				value = in_value;
			}
			return 0;
		}
		virtual int doOpValue (char op, Rts2Value * old_value);
		virtual const char *getValue ();
		virtual double getValueDouble ()
		{
			return value;
		}
		virtual float getValueFloat ()
		{
			return value;
		}
		virtual int getValueInteger ()
		{
			return value;
		}
		int inc ()
		{
			return value++;
		}
		int dec ()
		{
			return value--;
		}
		virtual void setFromValue (Rts2Value * newValue);
		virtual bool isEqual (Rts2Value *other_value);
};

/**
 * Class which holds double value.
 *
 * @ingroup RTS2Value
 */
class Rts2ValueDouble:public Rts2Value
{
	protected:
		double value;
	public:
		Rts2ValueDouble (std::string in_val_name);
		Rts2ValueDouble (std::string in_val_name, std::string in_description,
			bool writeToFits = true, int32_t flags = 0);
		virtual int setValue (Rts2Conn * connection);
		virtual int setValueCharArr (const char *in_value);
		virtual int setValueInteger (int in_value);
		virtual int doOpValue (char op, Rts2Value * old_value);
		void setValueDouble (double in_value)
		{
			if (value != in_value)
			{
				changed ();
				value = in_value;
			}
		}
		virtual const char *getValue ();
		virtual const char *getDisplayValue ();
		virtual double getValueDouble ()
		{
			return value;
		}
		virtual float getValueFloat ()
		{
			return value;
		}
		virtual int getValueInteger ()
		{
			if (isnan (value))
				return -1;
			return (int) value;
		}
		virtual long int getValueLong ()
		{
			if (isnan (value))
				return -1;
			return (long int) value;
		}
		virtual void setFromValue (Rts2Value * newValue);
		virtual bool isEqual (Rts2Value *other_value);
};

/**
 * Class which holds time value.
 *
 * @ingroup RTS2Value
 */
class Rts2ValueTime:public Rts2ValueDouble
{
	public:
		Rts2ValueTime (std::string in_val_name);
		Rts2ValueTime (std::string in_val_name, std::string in_description,
			bool writeToFits = true, int32_t flags = 0);
		void setValueTime (time_t in_value)
		{
			Rts2ValueDouble::setValueDouble (in_value);
		}
		virtual const char *getDisplayValue ();

		/**
		 * Set time value to current date and time.
		 */
		void setNow ();

		/**
		 * Convert double ctime to struct tm and fraction of seconds.
		 */
		void getStructTm (struct tm *tm_s, long *usec);

		/**
		 * Get timeval.
		 *
		 * @param tv Returned timeval value.
		 */
		void getValueTime (struct timeval &tv);
};

/**
 * Class which holds float value.
 *
 * @ingroup RTS2Value
 */
class Rts2ValueFloat:public Rts2Value
{
	private:
		float value;
	public:
		Rts2ValueFloat (std::string in_val_name);
		Rts2ValueFloat (std::string in_val_name, std::string in_description,
			bool writeToFits = true, int32_t flags = 0);
		virtual int setValue (Rts2Conn * connection);
		virtual int setValueCharArr (const char *in_value);
		virtual int setValueInteger (int in_value);
		virtual int doOpValue (char op, Rts2Value * old_value);
		void setValueDouble (double in_value)
		{
			if (value != in_value)
			{
				changed ();
				value = (float) in_value;
			}
		}
		void setValueFloat (float in_value)
		{
			if (value != in_value)
			{
				changed ();
				value = in_value;
			}
		}
		virtual const char *getValue ();
		virtual const char *getDisplayValue ();
		virtual double getValueDouble ()
		{
			return value;
		}
		virtual float getValueFloat ()
		{
			return value;
		}
		virtual int getValueInteger ()
		{
			if (isnan (value))
				return -1;
			return (int) value;
		}
		virtual void setFromValue (Rts2Value * newValue);
		virtual bool isEqual (Rts2Value *other_value);
};

/**
 * Class which holds boolean value.
 *
 * @ingroup RTS2Value
 */
class Rts2ValueBool:public Rts2ValueInteger
{
	// value - 2 means unknow, 0 is false, 1 is true
	public:
		Rts2ValueBool (std::string in_val_name);
		Rts2ValueBool (std::string in_val_name, std::string in_description,
			bool writeToFits = true, int32_t flags = 0);
		virtual int setValue (Rts2Conn * connection);
		virtual int setValueCharArr (const char *in_value);

		void setValueBool (bool in_bool)
		{
			if (in_bool != getValueInteger ())
				changed ();
			setValueInteger (in_bool);
		}

		bool getValueBool ()
		{
			return getValueInteger () == 1 ? true : false;
		}
		virtual const char *getDisplayValue ();
};

/**
 * Abstract class, it's descandants cann be added as data to SelVal.
 *
 * @see SelVal
 */
class Rts2SelData
{
	public:
		/**
		 * Empty SelData constructor.
		 */
		Rts2SelData ()
		{
		}

		/**
		 * Empty SelData destructor.
		 */
		virtual ~Rts2SelData ()
		{
		}
};

/**
 * Holds selection value.
 *
 * @ingroup RTS2Value
 */
class SelVal
{
	public:
		std::string name;
		Rts2SelData * data;

		/**
		 * Create new selection value.
		 *
		 * @param in_name Selection value name.
		 * @param in_data Selection value optional data.
		 */
		SelVal (std::string in_name, Rts2SelData *in_data = NULL)
		{
			name = in_name;
			data = in_data;
		}
};

/**
 * This value adds as meta-information allowed content in strings.
 * It can be used for named selection list (think of enums..).
 *
 * @ingroup RTS2Value
 */
class Rts2ValueSelection:public Rts2ValueInteger
{
	private:
		// holds variables bag..
		std::vector < SelVal > values;
		void deleteValues ();

	protected:
		virtual int sendTypeMetaInfo (Rts2Conn * connection);

	public:
		Rts2ValueSelection (std::string in_val_name);
		Rts2ValueSelection (std::string in_val_name, std::string in_description, bool writeToFits = false, int32_t flags = 0);
		virtual ~Rts2ValueSelection (void);

		virtual int setValue (Rts2Conn * connection);
		virtual int setValueCharArr (const char *in_value);
		virtual int doOpValue (char op, Rts2Value * old_value);

		int getSelIndex (std::string in_val);

		virtual const char *getDisplayValue ()
		{
			return getSelName ().c_str ();
		}

		virtual int setValueInteger (int in_value)
		{
			if (in_value < 0 || (size_t) in_value >= values.size ())
				return -1;
			return Rts2ValueInteger::setValueInteger (in_value);
		}

		int setSelIndex (char *selVal)
		{
			int i = getSelIndex (std::string (selVal));
			if (i < 0)
				return -1;
			setValueInteger (i);
			return 0;
		}
		void copySel (Rts2ValueSelection * sel);

		/**
		 * Adds selection value.
		 *
		 * @param sel_name String identifing new selection value.
		 */
		void addSelVal (const char *sel_name, Rts2SelData *data = NULL)
		{
			addSelVal (std::string (sel_name), data);
		}

		void addSelVal (std::string sel_name, Rts2SelData *data = NULL)
		{
			addSelVal (SelVal (sel_name, data));
		}

		void addSelVal (SelVal val)
		{
			values.push_back (val);
		}

		void addSelVals (const char **vals);

		std::string getSelName ()
		{
			return getSelName (getValueInteger ());
		}

		std::string getSelName (int index)
		{
			const SelVal *val = getSelVal (index);
			if (val == NULL)
				return std::string ("UNK");
			return val->name;
		}

		Rts2SelData *getData ()
		{
			return getData (getValueInteger ());
		}

		Rts2SelData *getData (int index)
		{
			const SelVal *val = getSelVal (index);
			if (val == NULL)
				return NULL;
			return val->data;
		}

		const SelVal* getSelVal ()
		{
			return getSelVal (getValueInteger ());
		}

		const SelVal* getSelVal (int index)
		{
			if (index < 0 || (unsigned int) index >= values.size ())
				return NULL;
			// find it
			std::vector <SelVal>::iterator iter = selBegin ();
			for (int i = 0; iter != selEnd () && i < index; iter++, i++)
			{
			}
			return &(*iter);
		}

		std::vector < SelVal >::iterator selBegin ()
		{
			return values.begin ();
		}

		std::vector < SelVal >::iterator selEnd ()
		{
			return values.end ();
		}

		int selSize ()
		{
			return values.size ();
		}

		void duplicateSelVals (Rts2ValueSelection * otherValue);
};

/**
 * Class for long int value.
 *
 * @ingroup RTS2Value
 */
class Rts2ValueLong:public Rts2Value
{
	private:
		long int value;
	public:
		Rts2ValueLong (std::string in_val_name);
		Rts2ValueLong (std::string in_val_name, std::string in_description,
			bool writeToFits = true, int32_t flags = 0);
		virtual int setValue (Rts2Conn * connection);
		virtual int setValueCharArr (const char *in_value);
		virtual int setValueInteger (int in_value);
		virtual int doOpValue (char op, Rts2Value * old_value);

		virtual const char *getValue ();
		virtual double getValueDouble ()
		{
			return value;
		}
		virtual float getValueFloat ()
		{
			return value;
		}
		virtual int getValueInteger ()
		{
			return (int) value;
		}
		virtual long int getValueLong ()
		{
			return value;
		}
		long inc ()
		{
			return value++;
		}
		long dec ()
		{
			return value--;
		}
		int setValueLong (long in_value)
		{
			if (value != in_value)
			{
				changed ();
				value = in_value;
			}
			return 0;
		}
		virtual void setFromValue (Rts2Value * newValue);
		virtual bool isEqual (Rts2Value *other_value);
};

/**
 * Class for RADEC informations.
 *
 * @ingroup RTS2Value
 */
class Rts2ValueRaDec: public Rts2Value
{
	private:
		double ra;
		double decl;

	public:
		Rts2ValueRaDec (std::string in_val_name);
		Rts2ValueRaDec (std::string in_val_name, std::string in_description,
			bool writeToFits = true, int32_t flags = 0);
		virtual int setValue (Rts2Conn * connection);

		/**
		 * Set value from string, uses parsing from
		 * input stream, so it recognized various forms of the value.
		 *
		 * @param in_value String represenation of RA and DEC.
		 */
		virtual int setValueCharArr (const char *in_value);

		/**
		 * Set RA and DEC values from two doubles.
		 *
		 * @param in_ra RA value.
		 * @param in_dec DEC value.
		 *
		 * @return 0 on sucess.
		 */
		int setValueRaDec (double in_ra, double in_dec)
		{
			setRa (in_ra);
			setDec (in_dec);
			return 0;
		}


		/**
		 * Increments RA and DEC values.
		 *
		 * @param _ra RA increment.
		 * @param _dec Dec increment.
		 *
		 * @return 0 on success.
		 */
		int incValueRaDec (double _ra, double _dec)
		{
			setRa (getRa () + _ra);
			setDec (getDec () + _dec);
			return 0;
		}

		/**
		 * Sets RA.
		 */
		void setRa (double in_ra)
		{
			if (ra != in_ra)
			{
				ra = in_ra;
				changed ();
			}
		}

		/**
		 * Sets DEC.
		 */
		void setDec (double in_dec)
		{
			if (decl != in_dec)
			{
				decl = in_dec;
				changed ();
			}
		}

		virtual int doOpValue (char op, Rts2Value * old_value);

		virtual const char *getValue ();
		virtual double getValueDouble ()
		{
			return nan("f");
		}
		virtual float getValueFloat ()
		{
			return nan("f");
		}
		virtual int getValueInteger ()
		{
			return INT_MAX;
		}
		virtual long int getValueLong ()
		{
			return INT_MAX;
		}
		long inc ()
		{
			return INT_MAX;
		}
		long dec ()
		{
			return INT_MAX;
		}

		/**
		 * Return RA in degrees.
		 *
		 * @return RA value in degrees.
		 */
		double getRa ()
		{
			return ra;
		}

		/**
		 * Return DEC in degrees.
		 *
		 * @return DEC value in degrees.
		 */
		double getDec ()
		{
			return decl;
		}

		virtual void setFromValue (Rts2Value * newValue);
		virtual bool isEqual (Rts2Value *other_value);
};


/**
 * Class for Alt-Az informations.
 *
 * @ingroup RTS2Value
 */
class Rts2ValueAltAz: public Rts2Value
{
	private:
		double alt;
		double az;

	public:
		Rts2ValueAltAz (std::string in_val_name);
		Rts2ValueAltAz (std::string in_val_name, std::string in_description,
			bool writeToFits = true, int32_t flags = 0);
		virtual int setValue (Rts2Conn * connection);

		/**
		 * Set value from string, uses parsing from
		 * input stream, so it recognized various forms of the value.
		 *
		 * @param in_value String represenation of altitude and azimuth.
		 */
		virtual int setValueCharArr (const char *in_value);

		/**
		 * Set altitude and azimuth values from two doubles.
		 *
		 * @param in_alt Altitude value.
		 * @param in_az Azimuth value.
		 *
		 * @return 0 on sucess.
		 */
		int setValueAltAz (double in_alt, double in_az)
		{
			setAlt (in_alt);
			setAz (in_az);
			return 0;
		}

		/**
		 * Sets altitude.
		 */
		void setAlt (double in_alt)
		{
			if (alt != in_alt)
			{
				alt = in_alt;
				changed ();
			}
		}

		/**
		 * Sets azimuth.
		 */
		void setAz (double in_az)
		{
			if (az != in_az)
			{
				az = in_az;
				changed ();
			}
		}

		virtual int doOpValue (char op, Rts2Value * old_value);

		virtual const char *getValue ();
		virtual double getValueDouble ()
		{
			return nan("f");
		}
		virtual float getValueFloat ()
		{
			return nan("f");
		}
		virtual int getValueInteger ()
		{
			return INT_MAX;
		}
		virtual long int getValueLong ()
		{
			return INT_MAX;
		}
		long inc ()
		{
			return INT_MAX;
		}
		long dec ()
		{
			return INT_MAX;
		}

		/**
		 * Return altitude in degrees.
		 *
		 * @return Altitude value in degrees.
		 */
		double getAlt ()
		{
			return alt;
		}

		/**
		 * Return azimuth in degrees.
		 *
		 * @return Azimuth value in degrees.
		 */
		double getAz ()
		{
			return az;
		}

		virtual void setFromValue (Rts2Value * newValue);
		virtual bool isEqual (Rts2Value *other_value);
};


/**
 * Creates value from meta information.
 *
 * @param rts2Type Value flags.
 * @param name Value name.
 * @param desc Value description.
 */
Rts2Value *newValue (int rts2Type, std::string name, std::string desc);
#endif							 /* !__RTS2_VALUE__ */
