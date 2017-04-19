/* 
 * Night reporting utility.
 * Copyright (C) 2005-2010 Petr Kubanek <petr@kubanek.net>
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

#include "imgdisplay.h"
#include "configuration.h"
#include "libnova_cpp.h"
#include "rts2format.h"
#include "rts2db/appdb.h"
#include "rts2db/labellist.h"
#include "rts2db/messagedb.h"
#include "rts2db/observationset.h"

#include "imgdisplay.h"

#include <time.h>
#include <iostream>
#include <iomanip>

#define STAT_NONE            0
#define STAT_COLLOCATE       1
#define STAT_SUPER           2

#define OPT_NOMESSAGES       OPT_LOCAL + 600
#define OPT_MESSAGEALL       OPT_LOCAL + 601
#define OPT_LABELSTAT        OPT_LOCAL + 602

/**
 * Produces night report output.
 *
 * @addgroup RTS2DbApps
 */
class NightReport:public rts2db::AppDb
{
	public:
		NightReport (int argc, char **argv);
		virtual ~ NightReport (void);
	protected:
		virtual void usage ();

		virtual int processOption (int in_opt);
		virtual int init ();

		virtual int doProcessing ();
	private:
		time_t t_from, t_to;
		struct ln_date *tm_night;
		int printImages;
		int printCounts;
		bool printStat;
		int statType;

		const char *imageFormat;

		long totalImages;
		long totalGoodImages;
		long totalObs;

		int observationsNights;

		void printObsList (time_t *t_end);
		void printStatistics ();
		void printLabelStat (time_t *t_start, time_t *t_end);
		void printFromTo (time_t *t_start, time_t *t_end, bool printEmpty);
		rts2db::ObservationSet *obs_set;

		std::vector <rts2db::ObservationSet *> allObs;

		rts2db::MessageSet messages;
		int messageMask;
		bool labelStat;
};

NightReport::NightReport (int in_argc, char **in_argv):rts2db::AppDb (in_argc, in_argv)
{
	t_from = 0;
	t_to = 0;
	tm_night = NULL;
	obs_set = NULL;
	printImages = 0;
	printCounts = 0;
	printStat = false;
	statType = STAT_NONE;

	imageFormat = NULL;

	observationsNights = 0;

	totalImages = 0;
	totalGoodImages = 0;
	totalObs = 0;

	messageMask = MESSAGE_REPORTIT;
	labelStat = false;

	addOption (OPT_LABELSTAT, "labelstat", 0, "print observations and skytime statistics by targets");
	addOption ('f', NULL, 1, "period start; default to current date - 24 hours. Date is in YYYY-MM-DD format.");
	addOption ('t', NULL, 1, "period end; default to from + 24 hours. Date is in YYYY-MM-DD format.");
	addOption ('n', NULL, 1, "report for night. Night date format is YYYY-MM-DD.");
	addOption ('N', NULL, 0, "print out all values as numbers, do not pretty format them");
	addOption ('l', NULL, 0, "print full image names");
	addOption ('i', NULL, 0, "print image listing");
	addOption ('I', NULL, 0, "print image summary row");
	addOption ('m', NULL, 2, "print counts listing; can be followed by format, txt for plain");
	addOption ('M', NULL, 0, "print counts summary row");
	addOption ('s', NULL, 0, "print night statistics");
	addOption ('S', NULL, 0, "print night statistics - % of time used for observation");
	addOption ('c', NULL, 0, "collocate statistics by nights, targets, ..");
	addOption ('P', NULL, 1, "print expression formed from image header");
	addOption (OPT_NOMESSAGES, "nomsg", 0, "do not print messages");
	addOption (OPT_MESSAGEALL, "allmsg", 0, "print all messages");
}

NightReport::~NightReport (void)
{
	delete tm_night;
	for (std::vector <rts2db::ObservationSet *>::iterator iter = allObs.begin (); iter != allObs.end (); iter++)
	{
		delete (*iter);
	}
	allObs.clear ();
}

void NightReport::usage ()
{
	std::cout << "\t" << getAppName () << " -n 2007-12-31" << std::endl
		<< "To print observations from 15th December 2007 to 18th December 2007:" << std::endl
		<< "\t" << getAppName () << " -f 2007-12-15 -t 2007-12-18" << std::endl
		<< "To print observations and images from the last night with observation record:" << std::endl
		<< "\t" << getAppName () << "-i" << std::endl;
}

int NightReport::processOption (int in_opt)
{
	switch (in_opt)
	{
		case 'f':
			return parseDate (optarg, &t_from);
		case 't':
			return parseDate (optarg, &t_to);
			break;
		case 'n':
			tm_night = new struct ln_date;
			return parseDate (optarg, tm_night);
		case 'N':
			std::cout << pureNumbers;	
			break;
		case 'l':
			printImages |= DISPLAY_FILENAME;
			break;
		case 'i':
			printImages |= DISPLAY_ALL;
			break;
		case 'I':
			printImages |= DISPLAY_SUMMARY;
			break;
		case 'm':
			if (optarg)
			{
				if (!strcmp (optarg, "txt"))
					printCounts = DISPLAY_SHORT;
				else
					printCounts |= DISPLAY_ALL;
			}
			else
			{
				printCounts |= DISPLAY_ALL;
			}
			break;
		case 'M':
			printCounts |= DISPLAY_SUMMARY;
			break;
		case 's':
			printStat = true;
			break;
		case 'S':
			statType = STAT_SUPER;
			break;
		case 'c':
			statType = STAT_COLLOCATE;
			break;
		case 'P':
			imageFormat = optarg;
			break;
		case OPT_NOMESSAGES:
			messageMask = 0;
			break;
		case OPT_MESSAGEALL:
			messageMask = 0x0f;
			break;
		case OPT_LABELSTAT:
			labelStat = true;
			break;
		default:
			return rts2db::AppDb::processOption (in_opt);
	}
	return 0;
}

int NightReport::init ()
{
	int ret;
	ret = rts2db::AppDb::init ();
	if (ret)
		return ret;

	if (tm_night == 0 && t_from == 0 && t_to == 0)
	{
		int lastobs = rts2db::lastObservationId ();
		if (lastobs > -1)
		{
			rts2db::Observation obs (lastobs);
			obs.load ();
			time_t ts = obs.getObsStart ();
			ts = rts2core::Configuration::instance ()->getNight (ts);
			Rts2Night night (ln_get_julian_from_timet (&ts), rts2core::Configuration::instance ()->getObserver ());
			t_from = *(night.getFrom ());
			t_to = *(night.getTo ());
		}
	}
	else if (tm_night)
	{
		// let's calculate time from..t_from will contains start of night
		// local 12:00 will be at ~ give time..
		Rts2Night night (tm_night, rts2core::Configuration::instance ()->getObserver ());
		t_from = *(night.getFrom ());
		t_to = *(night.getTo ());
	}
	else
	{
		if (t_from == 0)
			t_from = rts2core::Configuration::instance ()->getNight ();
		if (t_to == 0)
		  	t_to = rts2core::Configuration::instance ()->getNight () + 86400;
	}

	if (t_from >= t_to)
	{
		std::cerr << "Time from is greater than time to. This will not produce any results. Please either switch -f and -t options, or provide other values." << std::endl;
		return -1;
	}

	return 0;
}

void NightReport::printObsList (time_t *t_end)
{
	if (printImages || imageFormat)
		obs_set->printImages (printImages, imageFormat);
	if (printCounts)
		obs_set->printCounts (printCounts);
	if (statType == STAT_COLLOCATE)
		obs_set->collocate ();
	// mix with messages..
	messages.rewind ();
	obs_set->rewind ();
	while (true)	
	{
		double o_next = obs_set->getNextCtime ();
		double m_next = messages.getNextCtime ();
		if (std::isnan (o_next) && std::isnan (m_next))
			break;
		if (std::isnan (o_next))
		{
			messages.printUntil (*t_end + 1, std::cout);
			break;
		}
		if (std::isnan (m_next))
		{
			obs_set->printUntil (*t_end + 1, std::cout);
			break;
		}
		if (m_next < o_next)
			messages.printUntil (o_next, std::cout);
		else
			obs_set->printUntil (m_next, std::cout);
	}
}

void NightReport::printStatistics ()
{
	obs_set->printStatistics (std::cout);
}

void NightReport::printLabelStat (time_t *t_start, time_t *t_end)
{
	rts2db::LabelList labels;
	labels.load ();
	for (rts2db::LabelList::iterator iter = labels.begin (); iter != labels.end (); iter++)
	{
		std::cout << "Label " << iter->tid << " " << iter->text << " (" << iter->labid << "):" << std::endl;
		for (rts2db::ObservationSet::iterator oi = obs_set->begin (); oi != obs_set->end (); oi++)
		{
			if (oi->getTarget ()->hasLabel (iter->labid))
				std::cout << (*oi);
		}
		rts2db::ImageSetLabel isl (iter->labid, *t_start, *t_end);
		std::cout << "Observations: " << isl.getAllStat ().count << std::endl
			<< "Skytime: " << isl.getAllStat ().exposure << std::endl
			<< "--------------------------------------------" << std::endl;
	}
}

void NightReport::printFromTo (time_t *t_start, time_t * t_end, bool printEmpty)
{
	obs_set = new rts2db::ObservationSet ();
	obs_set->loadTime (t_start, t_end);

	messages.load (*t_start, *t_end, messageMask);

	if (!printEmpty && obs_set->empty ())
	{
		delete obs_set;
		return;
	}

	if (!obs_set->empty ())		 // used nights..
		observationsNights++;

	if (statType == STAT_SUPER)
	{
		obs_set->computeStatistics ();
		totalImages += obs_set->getNumberOfImages ();
		totalGoodImages += obs_set->getNumberOfGoodImages ();
		totalObs += obs_set->size ();
		allObs.push_back (obs_set);
	}
	else
	{
		// from which date to which..
		std::cout << "From " << Timestamp (*t_start) << " to " << Timestamp (*t_end) << std::endl;

		printObsList (t_end);

		if (printStat)
			printStatistics ();
		if (labelStat)
			printLabelStat (t_start, t_end);
		delete obs_set;
	}
}

int NightReport::doProcessing ()
{
	rts2core::Configuration::instance ();

	int totalNights = 0;

	if (statType != STAT_NONE && t_to - t_from > 86400)
	{
		time_t t_start = t_from;
		// iterate by nights
		for (time_t t_end = t_from + 86400; t_end <= t_to; t_start = t_end, t_end += 86400)
		{
			totalNights++;
			printFromTo (&t_start, &t_end, false);
		}
	}
	else
	{
		totalNights++;
		printFromTo (&t_from, &t_to, true);
	}

	// print number of nights
	if (statType == STAT_SUPER)
	{
		if (observationsNights == 0)
		{
			std::cerr << "Cannot find any observations from " << Timestamp (t_from)
				<< " to " << Timestamp (t_to) << std::endl;
			return -1;
		}
		std::cout << "From " << Timestamp (t_from) << " to " << Timestamp (t_to) << std::endl
			<< "Number of nights " << totalNights << " observations nights " << observationsNights
			<< " (" << Percentage (observationsNights, totalNights) << "%)" << std::endl;
		if (totalImages == 0)
		{
			std::cerr << "Cannot find any images from " << Timestamp (t_from)
				<< " to " << Timestamp (t_to) << std::endl;
		}
		else
		{
			std::cout << "Images " << totalImages << " good images " << totalGoodImages
				<< " (" << Percentage (totalGoodImages, totalImages) << "%)" << std::endl;

		}

		time_t t_start = t_from;

		std::cout << " Date                          Images      % of good" << std::endl
			<< "                    Observations   Good Images" << std::endl;

		for (std::vector <rts2db::ObservationSet *>::iterator iter = allObs.begin (); iter != allObs.end (); iter++)
		{
			std::cout << SEP << Timestamp (t_start)
				<< SEP << std::setw (5) << (*iter)->size ()
				<< SEP << std::setw (5) << (*iter)->getNumberOfImages ()
				<< SEP << std::setw (5) << (*iter)->getNumberOfGoodImages ()
				<< SEP << Percentage ((*iter)->getNumberOfGoodImages (), (*iter)->getNumberOfImages ())
				<< std::endl;
			t_start += 86400;
		}
	}

	return 0;
}

int main (int argc, char **argv)
{
	NightReport app = NightReport (argc, argv);
	return app.run ();
}
