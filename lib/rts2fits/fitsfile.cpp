/* 
 * Class representing FITS file.
 * Copyright (C) 2011 Petr Kubanek, Institute of Physics <kubanek@fzu.cz>
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

#include "fitsfile.h"

#include "app.h"
#include "utilsfunc.h"

#include "config.h"

#include <errno.h>
#include <string.h>
#include <iomanip>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

using namespace rts2image;

ColumnData::ColumnData (std::string _name, std::vector <double> _data)
{
	name = _name;
	len = _data.size ();
	type = "D";
	ftype = TDOUBLE;
	if (len == 0)
	{
		data = NULL;
		return;
	}
	data = malloc (sizeof (double) * len);
	double *tp = (double *) data;
	for (std::vector <double>::iterator iter = _data.begin (); iter != _data.end (); tp++, iter++)
		*tp = *iter;
}

ColumnData::ColumnData (std::string _name, std::vector <int> _data, bool isBoolean)
{
	name = _name;
	len = _data.size ();
	if (isBoolean)
	{
		type = "L";
		ftype = TLOGICAL;
	}
	else
	{
		type = "J";
		ftype = TINT;
	}
	if (len == 0)
	{
		data = NULL;
		return;
	}
	data = malloc (sizeof (int) * len);
	int *tp = (int *) data;
	for (std::vector <int>::iterator iter = _data.begin (); iter != _data.end (); tp++, iter++)
		*tp = *iter;
}

std::string FitsFile::getFitsErrors ()
{
	std::ostringstream os;
	char buf[200];
	char errmsg[81];

	fits_get_errstatus (fits_status, buf);
	fits_read_errmsg (errmsg);
	os << " file " << getFileName () << " " << buf << " message: " << errmsg;
	return os.str ();
}

void FitsFile::setFileName (const char *_fileName)
{
	if (fileName != absoluteFileName)
		delete[] absoluteFileName;
	delete[] fileName;

	if (_fileName == NULL)
	{
		fileName = NULL;
		absoluteFileName = NULL;
		return;
	}

	fileName = new char[strlen (_fileName) + 1];
	strcpy (fileName, _fileName);

	// not an absolute filename..
	if (fileName[0] != '/')
	{
		char path[1000];
		char *p = getcwd (path, 1000);
		if (p == NULL)
		{
			logStream (MESSAGE_ERROR) << "too long cwd" << sendLog;
			return;
		}
		absoluteFileName = new char[strlen (path) + strlen(_fileName) + 2];
		strcpy (absoluteFileName, path);
		int l = strlen (path);
		if (l == 0)
		{
			absoluteFileName[0] = '/';
			l++;
		}
		// last char isn't /
		else if (absoluteFileName[l - 1] != '/')
		{
			absoluteFileName[l] = '/';
			l++;
		}
		strcpy (absoluteFileName + l, fileName);
	}
	else
	{
		absoluteFileName = fileName;
	}
}

int FitsFile::createFile ()
{
	fits_status = 0;
	ffile = NULL;

	int ret;
	// make path for us..
	ret = mkpath (getFileName (), 0777);
	if (ret)
		return -1;

	fits_create_file (&ffile, getFileName (), &fits_status);

	if (fits_status)
	{
		logStream (MESSAGE_ERROR) << "FitsFile::createImage " << getFitsErrors () << sendLog;
		return -1;
	}

	return 0;
}

int FitsFile::createFile (const char *_fileName)
{
	setFileName (_fileName);
	return createFile ();
}

int FitsFile::createFile (std::string _fileName)
{
	setFileName (_fileName.c_str ());
	return createFile ();
}

void FitsFile::openFile (const char *_fileName, bool readOnly)
{
	closeFile ();

	fits_status = 0;

	if (_fileName && _fileName != getFileName ())
		setFileName (_fileName);

	if (getFileName () == NULL)
	 	throw ErrorOpeningFitsFile ("");

	#ifdef DEBUG_EXTRA
	logStream (MESSAGE_DEBUG) << "Opening " << getFileName () << " ffile " << getFitsFile () << sendLog;
	#endif						 /* DEBUG_EXTRA */

	fits_open_diskfile (&ffile, getFileName (), readOnly ? READONLY : READWRITE, &fits_status);
	if (fits_status)
	{
		if (!(flags & IMAGE_CANNOT_LOAD))
		{
			logStream (MESSAGE_ERROR) << "while opening fits file: " << getFitsErrors () <<	sendLog;
			flags |= IMAGE_CANNOT_LOAD;
		}
		throw ErrorOpeningFitsFile (getFileName ());
	}
}

int FitsFile::closeFile ()
{
	if (getFitsFile ())
	{
		fits_close_file (getFitsFile (), &fits_status);
		flags &= ~IMAGE_SAVE;
		setFitsFile (NULL);
	}
	return 0;
}

std::string FitsFile::expandPath (std::string pathEx)
{
	std::string ret = expand (pathEx);
	if (num_pos >= 0)
	{
		for (int n = 1; n < INT_MAX; n++)
		{
			std::ostringstream os;
			// construct new path..
			os.fill (num_fill);
			os << ret.substr (0, num_pos) << std::setw (num_lenght) << n << ret.substr (num_pos);
			struct stat s;
			if (stat (os.str ().c_str (), &s))
			{
				if (errno == ENOENT)
					return os.str ();
				throw rts2core::Error (std::string ("error when checking for ") + os.str() + ":" + strerror (errno));
			}
		}
		throw rts2core::Error ("too many files with name matching " + ret);
	}
	return ret;
}

void FitsFile::moveHDU (int hdu, int *hdutype)
{
	fits_movabs_hdu (getFitsFile (), hdu, hdutype, &fits_status);
	if (fits_status)
	{
		logStream (MESSAGE_ERROR) << "cannot move HDU to " << hdu << ": " << getFitsErrors () << sendLog;
	}
}

int FitsFile::fitsStatusValue (const char *valname, const char *operation)
{
	int ret = 0;
	if (fits_status)
	{
		ret = -1;
	}
	fits_status = 0;
	return ret;
}

void FitsFile::fitsStatusSetValue (const char *valname, bool required)
{
	int ret = fitsStatusValue (valname, "SetValue");
	if (ret && required)
		throw ErrorSettingKey (this, valname);
}

void FitsFile::fitsStatusGetValue (const char *valname, bool required)
{
	int ret = fitsStatusValue (valname, "GetValue");
	if (ret && required)
		throw KeyNotFound (this, valname);
}

FitsFile::FitsFile ():rts2core::Expander ()
{
	ffile = NULL;
	fileName = NULL;
	absoluteFileName = NULL;
	fits_status = 0;
	templateFile = NULL;
}

FitsFile::FitsFile (FitsFile * _fitsfile):rts2core::Expander (_fitsfile)
{
	fileName = NULL;
	absoluteFileName = NULL;

	setFitsFile (_fitsfile->getFitsFile ());
	_fitsfile->setFitsFile (NULL);

	setFileName (_fitsfile->getFileName ());

	fits_status = _fitsfile->fits_status;
	templateFile = NULL;
}

FitsFile::FitsFile (const char *_fileName):rts2core::Expander ()
{
	fileName = NULL;
	absoluteFileName = NULL;
	fits_status = 0;

	templateFile = NULL;

	createFile (_fileName);
}

FitsFile::FitsFile (const struct timeval *_tv):rts2core::Expander (_tv)
{
	ffile = NULL;
	fileName = NULL;
	absoluteFileName = NULL;
	fits_status = 0;
	templateFile = NULL;
}

FitsFile::FitsFile (const char *_expression, const struct timeval *_tv):rts2core::Expander (_tv)
{
	fileName = NULL;
	absoluteFileName = NULL;
	fits_status = 0;
	templateFile = NULL;

	createFile (expandPath (_expression));
}

FitsFile::~FitsFile (void)
{
	closeFile ();

	if (fileName != absoluteFileName)
		delete[] absoluteFileName;
	delete[] fileName;
}

void FitsFile::loadTemlate (const char *fn)
{
	templateFile = new Rts2ConfigRaw ();

	int ret = templateFile->loadFile (fn);
	if (ret)
	{
		logStream (MESSAGE_ERROR) << "cannot load template file " << fn << sendLog;
		delete templateFile;
		templateFile = NULL;
	}
}

void FitsFile::writeHistory (const char *history)
{
	fits_write_history (ffile, (char *) history, &fits_status);
	fitsStatusSetValue ("history", true);
}

void FitsFile::writeComment (const char *comment)
{
	fits_write_comment (ffile, (char *) comment, &fits_status);
	fitsStatusSetValue ("comment", true);
}

int FitsFile::writeArray (const char *extname, TableData *values)
{
	char *cols[values->size ()];
	const char *types[values->size ()];
	const char *units[values->size ()];

	size_t maxsize = 0;
	
	int i = 0;
	std::list <ColumnData *>::iterator iter;

	for (iter = values->begin (); iter != values->end (); iter++, i++)
	{
		cols[i] = new char[(*iter)->name.length () + 1];
		strcpy (cols[i], (*iter)->name.c_str ());
		// replace .
		for (char *c = cols[i]; *c; c++)
			if (*c == '.')
				*c = '_';
		types[i] = (*iter)->type;
		units[i] = "A";
		if ((*iter)->len > maxsize)
			maxsize = (*iter)->len;
	}

	fits_create_tbl (ffile, BINARY_TBL, maxsize, values->size (), (char **) cols, (char **) types, (char **) units, (char *) extname, &fits_status);

	if (fits_status)
	{
		logStream (MESSAGE_ERROR) << "FitsFile::writeArray creating table " << getFitsErrors () << sendLog;
		return -1;
	}

	i = 1;

	for (iter = values->begin (); iter != values->end (); iter++, i++)
	{
		if ((*iter)->data)
			fits_write_col (ffile, (*iter)->ftype, i, 1, 1, (*iter)->len, (*iter)->data, &fits_status);
	}

	for (i = 0; i < (int) (values->size ()); i++)
		delete[] cols[i];

	if (fits_status)
	{
		logStream (MESSAGE_ERROR) << "FitsFile::writeArray " << getFitsErrors () << sendLog;
		return -1;
	}
	return 0;
}
