/* 
 * Class representing FITS file.
 * Copyright (C) 2008 Petr Kubanek <petr@kubanek.net>
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

#include "expander.h"
#include "error.h"
#include "valuearray.h"
#include "iniparser.h"

#include <fitsio.h>

#include <sstream>
#include <list>

/** Defines for FitsFile flags. */
#define IMAGE_SAVE              0x01
#define IMAGE_NOT_SAVE          0x02
#define IMAGE_KEEP_DATA         0x04
#define IMAGE_DONT_DELETE_DATA  0x08
#define IMAGE_CANNOT_LOAD       0x10

// user defined flag
#define IMAGE_FLAG_USER1        0x20

namespace rts2image
{

/**
 * Class with table column informations. Used to temorary store
 * table data.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class ColumnData
{
	public:
		ColumnData (std::string _name, std::vector <double> _data);
		ColumnData (std::string _name, std::vector <int> _data, bool isBoolean);
		~ColumnData () { if (data) free (data); }

		std::string name;
		size_t len;
		const char *type;
		int ftype;
		void *data;
};

/**
 * Hold data which should be written to the FITS file.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class TableData:public std::list <ColumnData *>
{
	public:
		TableData (const char *_name, double _date)
		{
			name = std::string (_name);
			date = _date;
		}

		virtual ~TableData ()
		{
			for (std::list <ColumnData *>::iterator iter = begin (); iter != end (); iter++)
				delete *iter;
		}

		const char *getName () { return name.c_str (); }

		double getDate () { return date; }

		void addColumn (ColumnData *new_data) { push_back (new_data); }

	private:
		std::string name;
		double date;
};

/**
 * Class representing FITS file. This class represents FITS file. Usually you
 * will be looking for rts2image::Image class for image, or for Rts2FitsTable for
 * FITS table. This class only creates FITS file and manage keywords at primary
 * extension.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class FitsFile: public rts2core::Expander
{
	public:
		/**
		 * Creates file only in memory, do not write anything on disk.
		 */
		FitsFile ();

		/**
		 * Copy constructor.
		 *
		 * @param in_file  FitsFile to copy to this structure.
		 * Reference to fits file in original file will be deleted
		 * (NULLed).
		 */
		FitsFile (FitsFile * in_fitsfile);

		/**
		 * Create FITS file from filename.
		 *
		 * @param _filename Filename of the image.
		 */
		FitsFile (const char *_filename, bool _overwrite);

		/**
		 * Set only expansion date.
		 *
		 * @param _tv
		 */
		FitsFile (const struct timeval *_tv);
		
		/**
		 * Create FITS file from expand path. Uses given date for date related file expansion.
		 *
		 *
		 * @param _expression Input expression.
		 * @param _tv Timeval used for expansio of time-related keywords in expression.
		 */
		FitsFile (const char *_expression, const struct timeval *_tv, bool _overwrite);

		virtual ~FitsFile (void);

		virtual void openFile (const char *_filename = NULL, bool readOnly = false, bool _verbose = false);

		/**
		 * Return pointer to fitsfile structure.
		 *
		 * @return fitsfile pointer.
		 */
		fitsfile *getFitsFile () { return ffile; }

		/**
		 * Load given template file. Template file specifies values
		 * which should be written to the FITS headers, and general
		 * FITS file conventions.
		 *
		 * @param fn   template file path
		 */
		void loadTemplate (const char *fn);

		void setTemplate (rts2core::IniParser *temp) { templateFile = temp; }

		/**
		 * Return absolute filename. As filename is created in
		 * setFileName() call and it is check if it's absolute,
		 * the returned string is always absolute (e.g. begins with /).
		 *
		 * @return Filename of this FITS file.
		 * @see setFileName()
		 */
		const char *getFileName () { return fileName; }

		/**
		 * Return absolute filename.
		 */
		const char *getAbsoluteFileName () { return absoluteFileName; }

		/**
		 * Close file.
		 *
		 * @return -1 on error, 0 on success
		 */
		virtual int closeFile ();

		void setValue (const char *name, bool value, const char *comment);
		void setValue (const char *name, int value, const char *comment);
		void setValue (const char *name, long value, const char *comment);
		void setValue (const char *name, float value, const char *comment);
		void setValue (const char *name, double value, const char *comment);
		void setValue (const char *name, char value, const char *comment);
		void setValue (const char *name, const char *value, const char *comment);
		void setValue (const char *name, time_t * sec, long usec, const char *comment);

		// write rectangle in IRAF notation - e.g. as [x:y,w:h]
		void setValueRectange (const char *name, double x, double y, double w, double h, const char *comment);
		// that method is used to update DATE - creation date entry - for other file then ffile
		void setCreationDate (fitsfile * out_file = NULL);

		void getValue (const char *name, bool & value, bool required = false, char *comment = NULL);
		void getValue (const char *name, int &value, bool required = false, char *comment = NULL);
		void getValue (const char *name, long &value, bool required = false, char *comment = NULL);
		void getValue (const char *name, float &value, bool required = false, char *comment = NULL);
		void getValue (const char *name, double &value, bool required = false, char *comment = NULL);
		void getValue (const char *name, char &value, bool required = false, char *comment = NULL);
		void getValue (const char *name, char *value, int valLen, const char* defVal = NULL, bool required = false, char *comment = NULL);
		void getValue (const char *name, char **value, int valLen, bool required = false, char *comment = NULL);

		void getValueRa (const char *name, double &value, bool required = false, char *comment = NULL);
		void getValueDec (const char *name, double &value, bool required = false, char *comment = NULL);

		/**
		 * Get double value from image.
		 *
		 * @param name Value name.
		 * @return Value
		 * @throw KeyNotFound
		 */
		double getValue (const char *name);

		void getValues (const char *name, int *values, int num, bool required = false, int nstart = 1);
		void getValues (const char *name, long *values, int num, bool required = false, int nstart = 1);
		void getValues (const char *name, double *values, int num, bool required = false, int nstart = 1);
		void getValues (const char *name, char **values, int num, bool required = false, int nstart = 1);

		/**
		 * Expand FITS path.
		 *
		 * @param pathEx      Path expansion string.
		 * @param onlyAlNum   If true, replace non-path character in expansion
		 *
		 * @return Expanded path.
		 */
		std::string expandPath (std::string pathEx, bool onlyAlNum = true);

		/**
		 * Write header template to FITS headers. Current frame in
		 * multichannel/extensions image is used.
		 *
		 * @param hc     config section holding template to write
		 */
		void writeTemplate (rts2core::IniSection *hc);

		/**
		 * Add template values to already existing file.
		 *
		 * @param templ  template to add - PRIMARY will get to primary headers, CHANNELx will get to to extensions
		 */
		void addTemplate (rts2core::IniParser *templ);

		/**
		 * Move current HDU.
		 */
		void moveHDU (int hdu, int *hdutype = NULL);

		/**
		 * Return total number of HDUs.
		 */
		int getTotalHDUs ();

		/**
		 * Appends history string.
		 *
		 * @param history History keyword which will be appended.
		 *
		 * @throw ErrorSettingKey
		 */
		void writeHistory (const char *history);

		/**
		 * Append comment to FITS file.
		 *
		 * @param comment Comment which will be appended to FITS file comments.
		 *
		 * @throw ErrorSettingKey
		 */
		void writeComment (const char *comment);

		/**
		 * Create table extension from DoubleArray
		 */
		int writeArray (const char *extname, TableData *values);

		/**
		 * Return true if image shall be written to disk before it is closed.
		 *
		 * @return True if images shall be written.
		 */
		bool shouldSaveImage () { return (flags & IMAGE_SAVE); }

		/**
		 * Return explanation for fits errors. This method returns only
		 * explanation for the latest failed call. There is not a history
		 * of errors messages.
		 *
		 * @return Explanation for current fits_status.
		 */
		std::string getFitsErrors ();

	protected:
		int fits_status;
		int flags;

		// template - header config file
		rts2core::IniParser *templateFile;

		void setFileName (const char *_filename);

		/**
		 * @param _overwrite   if true, existing file will be overwritten
		 */
		virtual int createFile (bool _overwrite = false);
		int createFile (const char *_filename, bool _overwrite = false);
		int createFile (std::string _filename, bool _overwrite = false);

		void setFitsFile (fitsfile *_fitsfile) { ffile = _fitsfile; }

		int fitsStatusValue (const char *valname, const char *operation);

		void fitsStatusSetValue (const char *valname, bool required = true);
		void fitsStatusGetValue (const char *valname, bool required);

		bool isMemImage () { return memFile; }

	private:
		/**
		 * Pointer to fits file.
		 */
		fitsfile *ffile;

		char *fileName;
		char *absoluteFileName;

		bool memFile;

		size_t *memsize;
		void **imgbuf;
};

/**
 * Thrown where we cannot find header in the image.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class KeyNotFound:public rts2core::Error
{
	public:
		KeyNotFound (FitsFile *_image, const char *_header):rts2core::Error ()
		{
			std::ostringstream _os;
			_os << "keyword " << _header <<  " missing in file " << _image->getFileName () << ":" << _image->getFitsErrors ();
			setMsg (_os.str ());
		}
};


/**
 * Thrown when file cannot be opened.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class ErrorOpeningFitsFile: public rts2core::Error
{
	public:
		ErrorOpeningFitsFile (const char *filename):rts2core::Error ()
		{
			setMsg (std::string ("Cannot open file ") + filename);
		}
};

class ErrorSettingKey: public rts2core::Error
{
	public:
		ErrorSettingKey (FitsFile *ff, const char *valname):rts2core::Error ()
		{
			setMsg (std::string ("Cannot set key ") + valname + std::string (":") + ff->getFitsErrors ());
		}
};

};
