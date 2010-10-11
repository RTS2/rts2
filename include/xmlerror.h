/* 
 * Various errors generated while parsing XML documents.
 * Copyright (C) 2009-2010 Petr Kubanek <petr@kubanek.net>
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

#ifndef __RTS2_XMLERROR__
#define __RTS2_XMLERROR__

/**
 * Error in Xml - either we cannot load it, or something is wrong in its
 * structure.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class XmlError
{
	public:
		XmlError () {}

		XmlError (std::string _desc) { desc = _desc; }

		void setDescription (std::ostringstream &_os) { desc = _os.str (); }

		friend std::ostream & operator << (std::ostream &_os, XmlError & _err)
		{
			_os << _err.desc << std::endl;
			return _os;
		}
	private:
		std::string desc;
};


/**
 * Error thrown when we find a missing attribute.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class XmlMissingAttribute: public XmlError
{
	public:
		XmlMissingAttribute (xmlNodePtr _node, std::string attr_name):XmlError ()
		{
			std::ostringstream _os;
			_os << "cannot find attribute " << attr_name << " in node " << xmlGetNodePath (_node) << " on line " << xmlGetLineNo (_node);
			setDescription (_os);
		}
};

/**
 * Error thrown when on unknow attribute.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class XmlUnexpectedAttribute: public XmlError
{
	public:
		XmlUnexpectedAttribute (xmlNodePtr _node, std::string attr_name):XmlError ()
		{
			std::ostringstream _os;
			_os << "unexpacted attribute " << attr_name << " in node " << xmlGetNodePath (_node) << " on line " << xmlGetLineNo (_node);
			setDescription (_os);
		}
};

/**
 * Error thrown when elemant is missing in sequence.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class XmlMissingElement: public XmlError
{
	public:
		XmlMissingElement (xmlNodePtr _node, std::string _name):XmlError ()
		{
			std::ostringstream _os;
			_os << "cannot find element " << _name << " in node " << xmlGetNodePath (_node) << " at line " << xmlGetLineNo (_node);
			setDescription (_os);
		}
};

/**
 * Error thrown when unexpected node is encoutered.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class XmlUnexpectedNode: public XmlError
{
	public:
		XmlUnexpectedNode (xmlNodePtr _node):XmlError ()
		{
			std::ostringstream _os;
			_os << "unexpected node " << xmlGetNodePath (_node) << " at line " << xmlGetLineNo (_node);
			setDescription (_os);
		}
};

/**
 * Error thrown when empty node, which should have content, is encoutered.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class XmlEmptyNode: public XmlError
{
	public:
		XmlEmptyNode (xmlNodePtr _node):XmlError ()
		{
			std::ostringstream _os;
			_os << "empty node " << xmlGetNodePath (_node) << " an line " << xmlGetLineNo (_node);
			setDescription (_os);
		}
};

#endif /* !__RTS2_XMLERROR__ */
