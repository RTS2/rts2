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

#include "rts2fits/fitsfile.h"

#include "block.h"
#include "utilsfunc.h"
#include "radecparser.h"

#include "rts2-config.h"

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

FitsFile::FitsFile ():rts2core::Expander ()
{
  	memFile = true;
	memsize = NULL;
	imgbuf = NULL;
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

	memFile = _fitsfile->memFile;
	memsize = _fitsfile->memsize;
	imgbuf = _fitsfile->imgbuf;

	_fitsfile->memsize = NULL;
	_fitsfile->imgbuf = NULL;

	setFileName (_fitsfile->getFileName ());

	fits_status = _fitsfile->fits_status;
	templateFile = NULL;
}

FitsFile::FitsFile (const char *_fileName, bool _overwrite):rts2core::Expander ()
{
	memFile = true;
	memsize = NULL;
	imgbuf = NULL;
	fileName = NULL;
	absoluteFileName = NULL;
	fits_status = 0;

	templateFile = NULL;

	createFile (_fileName, _overwrite);
}

FitsFile::FitsFile (const struct timeval *_tv):rts2core::Expander (_tv)
{
	memFile = true;
	memsize = NULL;
	imgbuf = NULL;
	ffile = NULL;
	fileName = NULL;
	absoluteFileName = NULL;
	fits_status = 0;
	templateFile = NULL;
}

FitsFile::FitsFile (const char *_expression, const struct timeval *_tv, bool _overwrite):rts2core::Expander (_tv)
{
	memFile = true;
	memsize = NULL;
	imgbuf = NULL;
	fileName = NULL;
	absoluteFileName = NULL;
	fits_status = 0;
	templateFile = NULL;

	createFile (expandPath (_expression), _overwrite);
}

FitsFile::~FitsFile (void)
{
	closeFile ();

	if (imgbuf)
		free (*imgbuf);
	delete imgbuf;
	delete memsize;

	if (fileName != absoluteFileName)
		delete[] absoluteFileName;
	delete[] fileName;
}

void FitsFile::openFile (const char *_fileName, bool readOnly, bool _verbose)
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

	memFile = false;
}

int FitsFile::closeFile ()
{
	if (getFitsFile ())
	{
		if (memFile)
		{
			memFile = false;
			// only save file if its path was specified
			if (getFileName ())
			{
				fitsfile *ofptr = getFitsFile ();
				if (createFile ())
					return -1;
				fits_copy_file (ofptr, getFitsFile (), 1, 1, 1, &fits_status);
				if (fits_status)
				{
					logStream (MESSAGE_ERROR) << "fits_copy_file: " << getFitsErrors () << sendLog;
					fits_close_file (ofptr, &fits_status);
					return -1;
				}
				fits_close_file (ofptr, &fits_status);
			}
			else
			{
				// memfile MUST be closed before its memory is freed
				fits_close_file (getFitsFile (), &fits_status);
				setFitsFile (NULL);
			}
			free (*imgbuf);
			delete imgbuf;
			delete memsize;

			imgbuf = NULL;
			memsize = NULL;
		}
		if (getFitsFile ())
		{
			fits_close_file (getFitsFile (), &fits_status);
			if (fits_status)
			{
				logStream (MESSAGE_ERROR) << "while saving fits file " << getFileName () << ": " << getFitsErrors () << sendLog;
			}
		}
		flags &= ~IMAGE_SAVE;

		setFitsFile (NULL);
	}
	return 0;
}

std::string FitsFile::getFitsErrors ()
{
	std::ostringstream os;
	char buf[200];
	char errmsg[81];

	fits_get_errstatus (fits_status, buf);
	fits_read_errmsg (errmsg);
	os << "file " << getFileName () << " " << buf << " message: " << errmsg;
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

int FitsFile::createFile (bool _overwrite)
{
	fits_status = 0;
	ffile = NULL;

	if (memFile)
	{
		memsize = new size_t;
		*memsize = 2880;
		imgbuf = new void*;
		*imgbuf = malloc (*memsize);
		fits_create_memfile (&ffile, imgbuf, memsize, 10 * (*memsize), realloc, &fits_status);
		if (fits_status)
		{
			logStream (MESSAGE_ERROR) << "FitsFile::createImage memimage " << getFitsErrors () << sendLog;
			return -1;
		}

		return 0;
	}
	int ret;
	// make path for us..
	ret = mkpath (getFileName (), 0777);
	if (ret)
		return -1;

	if (_overwrite)
	{
		ret = unlink (getFileName ());
		if (ret && errno != ENOENT)
		{
			logStream (MESSAGE_ERROR) << "Fitsfile::createImage cannot unlink existing file: " << strerror (errno) << sendLog;
			return -1;
		}
	}

	fits_create_file (&ffile, getFileName (), &fits_status);

	if (fits_status)
	{
		logStream (MESSAGE_ERROR) << "FitsFile::createImage " << getFitsErrors () << sendLog;
		return -1;
	}

	return 0;
}

int FitsFile::createFile (const char *_fileName, bool _overwrite)
{
	setFileName (_fileName);
	return createFile (_overwrite);
}

int FitsFile::createFile (std::string _fileName, bool _overwrite)
{
	setFileName (_fileName.c_str ());
	return createFile (_overwrite);
}

void FitsFile::setValue (const char *name, bool value, const char *comment)
{
	if (!getFitsFile ())
	{
		if (flags & IMAGE_NOT_SAVE)
			return;
		openFile ();
	}
	int i_val = value ? 1 : 0;
	fits_update_key (getFitsFile (), TLOGICAL, (char *) name, &i_val, (char *) comment, &fits_status);
	flags |= IMAGE_SAVE;
	return fitsStatusSetValue (name, true);
}

void FitsFile::setValue (const char *name, int value, const char *comment)
{
	if (!getFitsFile ())
	{
		if (flags & IMAGE_NOT_SAVE)
			return;
		openFile ();
	}
	fits_update_key (getFitsFile (), TINT, (char *) name, &value, (char *) comment, &fits_status);
	flags |= IMAGE_SAVE;
	fitsStatusSetValue (name, true);
}

void FitsFile::setValue (const char *name, long value, const char *comment)
{
	if (!getFitsFile ())
	{
		if (flags & IMAGE_NOT_SAVE)
			return;
		openFile ();
	}
	fits_update_key (getFitsFile (), TLONG, (char *) name, &value, (char *) comment, &fits_status);
	flags |= IMAGE_SAVE;
	fitsStatusSetValue (name);
}

void FitsFile::setValue (const char *name, float value, const char *comment)
{
	float val = value;
	if (!getFitsFile ())
	{
		if (flags & IMAGE_NOT_SAVE)
			return;
		openFile ();
	}
	if (isnan (val) || isinf (val))
		val = FLOATNULLVALUE;
	fits_update_key (getFitsFile (), TFLOAT, (char *) name, &val, (char *) comment, &fits_status);
	flags |= IMAGE_SAVE;
	fitsStatusSetValue (name);
}

void FitsFile::setValue (const char *name, double value, const char *comment)
{
	double val = value;
	if (!getFitsFile ())
	{
		if (flags & IMAGE_NOT_SAVE)
			return;
		openFile ();
	}
	if (isnan (val) || isinf (val))
		val = DOUBLENULLVALUE;
	fits_update_key (getFitsFile (), TDOUBLE, (char *) name, &val, (char *) comment, &fits_status);
	flags |= IMAGE_SAVE;
	fitsStatusSetValue (name);
}

void FitsFile::setValue (const char *name, char value, const char *comment)
{
	char val[2];
	if (!getFitsFile ())
	{
		if (flags & IMAGE_NOT_SAVE)
			return;
		openFile ();
	}
	val[0] = value;
	val[1] = '\0';
	fits_update_key (getFitsFile (), TSTRING, (char *) name, (void *) val, (char *) comment, &fits_status);
	flags |= IMAGE_SAVE;
	fitsStatusSetValue (name);
}

void FitsFile::setValue (const char *name, const char *value, const char *comment)
{
	// we will not save null values
	if (!value)
		return;
	if (!getFitsFile ())
	{
		if (flags & IMAGE_NOT_SAVE)
			return;
		openFile ();
	}
	fits_update_key_longstr (getFitsFile (), (char *) name, (char *) value, (char *) comment, &fits_status);
	flags |= IMAGE_SAVE;
	fitsStatusSetValue (name);
}

void FitsFile::setValue (const char *name, time_t * sec, suseconds_t usec, const char *comment)
{
	char buf[25];
	getDateObs (*sec, usec, buf);
	setValue (name, buf, comment);
}

void FitsFile::setValueRectange (const char *name, double x, double y, double w, double h, const char *comment)
{
	std::ostringstream os;
	os << "[" << x << ":" << y << "," << w << ":" << h << "]";
	setValue (name, os.str ().c_str (), comment);
}

void FitsFile::setCreationDate (fitsfile * out_file)
{
	fitsfile *curr_ffile = getFitsFile ();

	if (out_file)
	{
		setFitsFile (out_file);
	}

	struct timeval now;
	gettimeofday (&now, NULL);
	setValue ("DATE", &(now.tv_sec), now.tv_usec, "creation date");

	if (out_file)
	{
		setFitsFile (curr_ffile);
	}
}

void FitsFile::getValue (const char *name, bool & value, bool required, char *comment)
{
	if (!getFitsFile ())
		openFile (NULL, false, true);

	int i_val;
	fits_read_key (getFitsFile (), TLOGICAL, (char *) name, (void *) &i_val, comment, &fits_status);
	value = i_val == TRUE;
	fitsStatusGetValue (name, required);
}

void FitsFile::getValue (const char *name, int &value, bool required, char *comment)
{
	if (!getFitsFile ())
		openFile (NULL, false, true);

	fits_read_key (getFitsFile (), TINT, (char *) name, (void *) &value, comment, &fits_status);
	fitsStatusGetValue (name, required);
}

void FitsFile::getValue (const char *name, long &value, bool required, char *comment)
{
	if (!getFitsFile ())
		openFile (NULL, false, true);
	
	fits_read_key (getFitsFile (), TLONG, (char *) name, (void *) &value, comment,
		&fits_status);
	fitsStatusGetValue (name, required);
}

void FitsFile::getValue (const char *name, float &value, bool required, char *comment)
{
	if (!getFitsFile ())
		openFile (NULL, false, true);
	
	fits_read_key (getFitsFile (), TFLOAT, (char *) name, (void *) &value, comment,	&fits_status);
	fitsStatusGetValue (name, required);
}

void FitsFile::getValue (const char *name, double &value, bool required, char *comment)
{
	if (!getFitsFile ())
		openFile (NULL, false, true);
	
	fits_read_key (getFitsFile (), TDOUBLE, (char *) name, (void *) &value, comment, &fits_status);
	fitsStatusGetValue (name, required);
}

void FitsFile::getValue (const char *name, char &value, bool required, char *comment)
{
	static char val[FLEN_VALUE];
	if (!getFitsFile ())
		openFile (NULL, false, true);

	fits_read_key (getFitsFile (), TSTRING, (char *) name, (void *) val, comment,
		&fits_status);
	value = *val;
	fitsStatusGetValue (name, required);
}

void FitsFile::getValue (const char *name, char *value, int valLen, const char* defVal, bool required, char *comment)
{
	try
	{
		static char val[FLEN_VALUE];
		if (!getFitsFile ())
			openFile (NULL, false, true);
	
                fits_status = 0;
		fits_read_key (getFitsFile (), TSTRING, (char *) name, (void *) val, comment, &fits_status);
		strncpy (value, val, valLen);
		value[valLen - 1] = '\0';
		if (required)
		{
			fitsStatusGetValue (name, true);
		}
		else
		{
			fits_status = 0;
			return;
		}
	}
	catch (rts2core::Error &er)
	{
		if (defVal)
		{
			strncpy (value, defVal, valLen);
			return;
		}
		if (required)
			throw (er);
	}
}

void FitsFile::getValue (const char *name, char **value, int valLen, bool required, char *comment)
{
	if (!getFitsFile ())
		openFile (NULL, false, true);

	fits_read_key_longstr (getFitsFile (), (char *) name, value, comment, &fits_status);
	fitsStatusGetValue (name, required);
}

void FitsFile::getValueRa (const char *name, double &value, bool required, char *comment)
{
	char val[100];
	val[0] = '\0';
	getValue (name, val, 100, NULL, required, comment);

	double mul = 1;
	value = parseDMS (val, &mul);
	if (mul != 1)
		value *= 15.0;
}

void FitsFile::getValueDec (const char *name, double &value, bool required, char *comment)
{
	char val[100];
	val[0] = '\0';
	getValue (name, val, 100, NULL, required, comment);

	double mul = 1;
	value = parseDMS (val, &mul);
}

double FitsFile::getValue (const char *name)
{
	if (!getFitsFile ())
		openFile (NULL, false, true);

	double ret;
	fits_read_key_dbl (getFitsFile (), (char *) name, &ret, NULL, &fits_status);
	if (fits_status != 0)
	{
		fits_status = 0;
		throw KeyNotFound (this, name);
	}
	return ret;
}

void FitsFile::getValues (const char *name, int *values, int num, bool required, int nstart)
{
	if (!getFitsFile ())
		throw ErrorOpeningFitsFile (getFileName ());

	int nfound;
	fits_read_keys_log (getFitsFile (), (char *) name, nstart, num, values, &nfound,
		&fits_status);
	fitsStatusGetValue (name, required);
}

void FitsFile::getValues (const char *name, long *values, int num, bool required, int nstart)
{
	if (!getFitsFile ())
		throw ErrorOpeningFitsFile (getFileName ());

	int nfound;
	fits_read_keys_lng (getFitsFile (), (char *) name, nstart, num, values, &nfound,
		&fits_status);
	fitsStatusGetValue (name, required);
}

void FitsFile::getValues (const char *name, double *values, int num, bool required, int nstart)
{
	if (!getFitsFile ())
		throw ErrorOpeningFitsFile (getFileName ());

	int nfound;
	fits_read_keys_dbl (getFitsFile (), (char *) name, nstart, num, values, &nfound,
		&fits_status);
	fitsStatusGetValue (name, required);
}

void FitsFile::getValues (const char *name, char **values, int num, bool required, int nstart)
{
	if (!getFitsFile ())
		throw ErrorOpeningFitsFile (getFileName ());
	
	int nfound;
	fits_read_keys_str (getFitsFile (), (char *) name, nstart, num, values, &nfound, &fits_status);
	fitsStatusGetValue (name, required);
}

std::string FitsFile::expandPath (std::string pathEx, bool onlyAlNum)
{
	std::string ret = expand (pathEx, onlyAlNum);
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

void FitsFile::writeTemplate (rts2core::IniSection *hc)
{
	for (rts2core::IniSection::iterator iter = hc->begin (); iter != hc->end (); iter++)
	{
		std::string v = iter->getValue ();
		std::string com;
		// find comment start..
		size_t cb = v.find ('/');
		if (cb != std::string::npos)
		{
			com = v.substr (cb + 1);
			v = v.substr (0, cb - 1);
		}
		// expand v and com strings..
		v = expand (v);
		com = expand (com);
		// value type based on suffix
		std::string suff = iter->getSuffix ();
		if (suff == "i")
		{
			long dl = atol (v.c_str ());
			setValue (iter->getValueName ().c_str (), dl, com.c_str ());
		}
		else if (suff == "d")
		{
			double dv = atof (v.c_str ());
			setValue (iter->getValueName ().c_str (), dv, com.c_str ());
		}
		else
		{
			setValue (iter->getValueName ().c_str (), v.c_str (), com.c_str ());
		}
	}
}

void FitsFile::addTemplate (rts2core::IniParser *templ)
{
	int hdutype;
	moveHDU (1, &hdutype);
	rts2core::IniSection *hc = templ->getSection ("PRIMARY", false);
	if (hc)
		writeTemplate (hc);

	for (int i = 1; i < getTotalHDUs (); i++)
	{
		moveHDU (i + 1, &hdutype);
		int chan = -1;
		getValue ("CHANNEL", chan);
		if (chan > 0)
		{
			std::ostringstream channame;
			channame << "CHANNEL" << chan;
			hc = templ->getSection (channame.str ().c_str (), false);
			if (hc)
				writeTemplate (hc);
		}
	}
}

void FitsFile::appendFITS (const char *afile, int index)
{
	if (!getFitsFile ())
		openFile ();

	fitsfile *affile;
	fits_open_diskfile (&affile, afile, READONLY, &fits_status);
	if (fits_status)
		throw ErrorOpeningFitsFile (afile);
	appendFITS (affile, afile, index);
	fits_close_file (affile, &fits_status);
}

void FitsFile::appendFITS (fitsfile *affile, const char *ename, int index)
{
	fits_movabs_hdu (affile, index, NULL, &fits_status);
	if (fits_status)
	{
		fits_close_file (affile, &fits_status);
		throw rts2core::Error ("cannot move to requested HDU");
	}
	fits_copy_hdu (affile, getFitsFile (), 1, &fits_status);
	if (fits_status)
	{
		fits_close_file (affile, &fits_status);
		throw rts2core::Error ("cannot copy HDU");
	}
	moveHDU (getTotalHDUs ());
	if (ename)
		setValue ("EXTNAME", ename, "extension name");
}

int FitsFile::getTotalHDUs ()
{
	fits_status = 0;
	int tothdu;
	fits_get_num_hdus (getFitsFile (), &tothdu, &fits_status);
	if (fits_status)
	{
	  	logStream (MESSAGE_ERROR) << "cannot retrieve total number of HDUs: " << getFitsErrors () << sendLog;
		return -1;
	}
	return tothdu;
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

void FitsFile::loadTemplate (const char *fn)
{
	templateFile = new rts2core::IniParser ();

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
