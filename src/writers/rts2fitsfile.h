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

#include "../utils/expander.h"
#include "../utils/error.h"
#include "../utils/valuearray.h"

#include <fitsio.h>

#include <sstream>
#include <list>

/** Defines for FitsFile flags. */
#define IMAGE_SAVE              0x01
#define IMAGE_NOT_SAVE          0x02
#define IMAGE_KEEP_DATA         0x04
#define IMAGE_DONT_DELETE_DATA  0x08
#define IMAGE_CANNOT_LOAD       0x10

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
		~ColumnData () { if (data) free (data); }

		std::string name;
		size_t len;
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
 * will be looking for Rts2Image class for Rts2Image, or for Rts2FitsTable for
 * FITS table. This class only creates FITS file and manage keywords at primary
 * extension.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class Rts2FitsFile: public rts2core::Expander
{
	public:
		/**
		 * Creates file only in memory, do not write anything on disk.
		 */
		Rts2FitsFile ();

		/**
		 * Copy constructor.
		 *
		 * @param in_file  Rts2FitsFile to copy to this structure.
		 * Reference to fits file in original file will be deleted
		 * (NULLed).
		 */
		Rts2FitsFile (Rts2FitsFile * in_fitsfile);

		/**
		 * Create FITS file from filename.
		 *
		 * @param _filename Filename of the image.
		 */
		Rts2FitsFile (const char *_filename);

		/**
		 * Set only expansion date.
		 *
		 * @param _tv
		 */
		Rts2FitsFile (const struct timeval *_tv);
		
		/**
		 * Create FITS file from expand path. Uses given date for date related file expansion.
		 *
		 *
		 * @param _expression Input expression.
		 * @param _tv Timeval used for expansio of time-related keywords in expression.
		 */
		Rts2FitsFile (const char *_expression, const struct timeval *_tv);

		virtual ~Rts2FitsFile (void);

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

		/**
		 * Expand FITS path.
		 *
		 * @param pathEx Path expansion string.
		 *
		 * @return Expanded path.
		 */
		std::string expandPath (std::string pathEx) { return expand (pathEx); }

		/**
		 * Move current HDU.
		 */
		void moveHDU (int hdu, int *hdutype = NULL);

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

	protected:
		int fits_status;
		int flags;

		/**
		 * Return explanation for fits errors. This method returns only
		 * explanation for the latest failed call. There is not a history
		 * of errors messages.
		 *
		 * @return Explanation for current fits_status.
		 */
		std::string getFitsErrors ();

		void setFileName (const char *_filename);

		virtual int createFile ();
		int createFile (const char *_filename);
		int createFile (std::string _filename);

		void openFile (const char *_filename = NULL, bool readOnly = false);

		/**
		 * Return pointer to fitsfile structure.
		 *
		 * @return fitsfile pointer.
		 */
		fitsfile *getFitsFile () { return ffile; }

		void setFitsFile (fitsfile *_fitsfile) { ffile = _fitsfile; }

		int fitsStatusValue (const char *valname, const char *operation, bool required);

		void fitsStatusSetValue (const char *valname, bool required = true);
		void fitsStatusGetValue (const char *valname, bool required);

	private:
		/**
		 * Pointer to fits file.
		 */
		fitsfile *ffile;

		char *fileName;
		char *absoluteFileName;
};


namespace rts2image
{

/**
 * Thrown where we cannot find header in the image.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class KeyNotFound:public rts2core::Error
{
	public:
		KeyNotFound (Rts2FitsFile *_image, const char *_header):rts2core::Error ()
		{
			std::ostringstream _os;
			_os << "keyword " << _header <<  " missing in file " << _image->getFileName ();
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
		ErrorSettingKey (const char *valname):rts2core::Error ()
		{
			setMsg (std::string ("Cannot set key ") + valname);
		}
};

};
