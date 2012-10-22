/* 
 * JSON value encoding.
 * Copyright (C) 2012 Petr Kubanek, Institute of Physics <kubanek@fzu.cz>
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

#include "rts2json/jsonvalue.h"

#include "valuearray.h"
#include "valuestat.h"
#include "valuerectangle.h"

using namespace rts2json;

void sendArrayValue (rts2core::Value *value, std::ostringstream &os)
{
	os << "[";
	switch (value->getValueBaseType ())
	{
		case RTS2_VALUE_INTEGER:
			for (std::vector <int>::iterator iter = ((rts2core::IntegerArray *) value)->valueBegin (); iter != ((rts2core::IntegerArray *) value)->valueEnd (); iter++)
			{
			  	if (iter != ((rts2core::IntegerArray *) value)->valueBegin ())
					os << ",";
				os << (*iter);
			}
			break;
		case RTS2_VALUE_STRING:
			for (std::vector <std::string>::iterator iter = ((rts2core::StringArray *) value)->valueBegin (); iter != ((rts2core::StringArray *) value)->valueEnd (); iter++)
			{
			  	if (iter != ((rts2core::StringArray *) value)->valueBegin ())
					os << ",";
				os << "\"" << (*iter) << "\"";
			}
			break;
		case RTS2_VALUE_DOUBLE:	
		case RTS2_VALUE_TIME:
			for (std::vector <double>::iterator iter = ((rts2core::DoubleArray *) value)->valueBegin (); iter != ((rts2core::DoubleArray *) value)->valueEnd (); iter++)
			{
				if (iter != ((rts2core::DoubleArray *) value)->valueBegin ())
					os << ",";
				os << JsonDouble (*iter);
			}
			break;
		case RTS2_VALUE_BOOL:
			for (std::vector <int>::iterator iter = ((rts2core::BoolArray *) value)->valueBegin (); iter != ((rts2core::BoolArray *) value)->valueEnd (); iter++)
			{
				if (iter != ((rts2core::BoolArray *) value)->valueBegin ())
					os << ",";
				os << ((*iter) ? "true":"false");
			}
			break;
		default:
			os << "\"" << value->getDisplayValue () << "\"";
			break;
	}
	os << "]";
}

void sendStatValue (rts2core::Value *value, std::ostringstream &os)
{
	os << "[";
	switch (value->getValueBaseType ())
	{
		case RTS2_VALUE_DOUBLE:
			{
				rts2core::ValueDoubleStat *vds = (rts2core::ValueDoubleStat *) value;
				os << vds->getNumMes () << ","
					<< JsonDouble (vds->getValueDouble ()) << ","
					<< JsonDouble (vds->getMode ()) << ","
					<< JsonDouble (vds->getStdev ()) << ","
					<< JsonDouble (vds->getMin ()) << ","
					<< JsonDouble (vds->getMax ());
			}
			break;
		default:
			os << "\"" << value->getDisplayValue () << "\"";
			break;
	}
	os << "]";
}

void sendRectangleValue (rts2core::Value *value, std::ostringstream &os)
{
	rts2core::ValueRectangle *r = (rts2core::ValueRectangle *) value;
	os << "[" << r->getXInt () << "," << r->getYInt () << "," << r->getWidthInt () << "," << r->getHeightInt () << "]";
}

void sendValue (rts2core::Value *value, std::ostringstream &os)
{
	switch (value->getValueBaseType ())
	{
		case RTS2_VALUE_STRING:
			os << "\"" << value->getValue () << "\"";
			break;
		case RTS2_VALUE_DOUBLE:
		case RTS2_VALUE_FLOAT:
		case RTS2_VALUE_TIME:
			os << JsonDouble (value->getValueDouble ());
			break;
		case RTS2_VALUE_INTEGER:
		case RTS2_VALUE_LONGINT:
		case RTS2_VALUE_SELECTION:
		case RTS2_VALUE_BOOL:
			os << value->getValue ();
			break;
		case RTS2_VALUE_RADEC:
			os << "{\"ra\":" << JsonDouble (((rts2core::ValueRaDec *) value)->getRa ()) << ",\"dec\":" << JsonDouble (((rts2core::ValueRaDec *) value)->getDec ()) << "}";
			break;
		case RTS2_VALUE_ALTAZ:
			os << "{\"alt\":" << JsonDouble (((rts2core::ValueAltAz *) value)->getAlt ()) << ",\"az\":" << JsonDouble (((rts2core::ValueAltAz *) value)->getAz ()) << "}";
			break;
		default:
			os << "\"" << value->getDisplayValue () << "\"";
			break;
	}
}

// encode value to JSON
void jsonValue (rts2core::Value *value, bool extended, std::ostringstream & os)
{
	os << "\"" << value->getName () << "\":";
	if (extended)
		os << "[" << value->getFlags () << ",";
  	if (value->getValueExtType() & RTS2_VALUE_ARRAY)
	{
	  	sendArrayValue (value, os);
	}
	else switch (value->getValueExtType())
	{
		case RTS2_VALUE_STAT:
			sendStatValue (value, os);
			break;
		case RTS2_VALUE_RECTANGLE:
			sendRectangleValue (value, os);
			break;
		default:
	  		sendValue (value, os);
			break;
	}
	if (extended)
		os << "," << value->isError () << "," << value->isWarning () << ",\"" << value->getDescription () << "\"]";
}
