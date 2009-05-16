#include "imgdisplay.h"
#include "../utilsdb/rts2appdb.h"
#include "../utilsdb/rts2obs.h"
#include "../utilsdb/observationset.h"

#include <list>
#include <iostream>

class Rts2ObsInfo:public Rts2AppDb
{
	private:
		rts2db::ObservationSet * obsset;
		enum
		{
			BASIC_INFO, EXT_INFO, IMAGES, IMAGES_ASTR_OK, IMAGES_TRASH,
			FLAT_IMAGES, DARK_IMAGES, IMAGES_QUE
		} action;
		int imageFlag;
		int displayFlats ();
		int displayDarks ();
		void printObsImages (Rts2Obs & obs);
	protected:
		virtual int processOption (int in_opt);
		virtual int processArgs (const char *arg);
	public:
		Rts2ObsInfo (int in_argc, char **in_argv);
		virtual ~ Rts2ObsInfo (void);
		virtual int doProcessing ();
};

Rts2ObsInfo::Rts2ObsInfo (int in_argc, char **in_argv):
Rts2AppDb (in_argc, in_argv)
{
	obsset = new rts2db::ObservationSet ();
	imageFlag = 0;
	addOption ('a', "action", 1,
		"i->images, a->with astrometry, t->trash images, q->que images (not yet processed)\n\tf->falts, d->darks");
	addOption ('l', "filenames", 0, "print filenames");
	addOption ('s', "short", 0, "short image listing");
	action = BASIC_INFO;
}


Rts2ObsInfo::~Rts2ObsInfo (void)
{
	delete obsset;
}


int
Rts2ObsInfo::processOption (int in_opt)
{
	switch (in_opt)
	{
		case 'a':
			switch (*optarg)
			{
				case 'i':
					action = IMAGES;
					break;
				case 'e':
					action = EXT_INFO;
					break;
				case 'a':
					action = IMAGES_ASTR_OK;
					imageFlag = (imageFlag & ~DISPLAY_MASK_ASTR) | DISPLAY_ASTR_OK;
					break;
				case 't':
					action = IMAGES_TRASH;
					imageFlag = (imageFlag & ~DISPLAY_MASK_ASTR) | DISPLAY_ASTR_TRASH;
					break;
				case 'q':
					action = IMAGES_QUE;
					imageFlag = (imageFlag & ~DISPLAY_MASK_ASTR) | DISPLAY_ASTR_QUE;
					break;
				case 'f':
					action = FLAT_IMAGES;
					imageFlag |= DISPLAY_SHORT;
					break;
				case 'd':
					action = DARK_IMAGES;
					imageFlag |= DISPLAY_SHORT;
					break;
				default:
					std::cerr << "Invalid action '" << *optarg << "'" << std::endl;
					return -1;
			}
			break;
		case 's':
			imageFlag |= DISPLAY_SHORT;
			break;
		case 'l':
			imageFlag |= DISPLAY_FILENAME;
			break;
		default:
			return Rts2AppDb::processOption (in_opt);
	}
	return 0;
}


int
Rts2ObsInfo::processArgs (const char *arg)
{
	int obs_id;
	obs_id = atoi (arg);
	Rts2Obs obs = Rts2Obs (obs_id);
	obsset->push_back (obs);
	return 0;
}


int
Rts2ObsInfo::displayFlats ()
{
	if (obsset->empty ())
	{
		delete obsset;
		obsset = new rts2db::ObservationSet (TYPE_FLAT, OBS_BIT_PROCESSED, true);
	}
	return 0;
}


int
Rts2ObsInfo::displayDarks ()
{
	if (obsset->empty ())
	{
		delete obsset;
		obsset = new rts2db::ObservationSet (TYPE_DARK, OBS_BIT_PROCESSED, true);
	}
	return 0;
}


void
Rts2ObsInfo::printObsImages (Rts2Obs & obs)
{
	if (!imageFlag || obs.loadImages ())
		return;
	obs.getImageSet ()->print (std::cout, imageFlag);
}


int
Rts2ObsInfo::doProcessing ()
{
	int ret;
	rts2db::ObservationSet::iterator iter;
	for (iter = obsset->begin (); iter != obsset->end (); iter++)
	{
		Rts2Obs obs = (Rts2Obs) * iter;
		ret = obs.load ();
		if (ret)
			return ret;
		switch (action)
		{
			case BASIC_INFO:
				if (!(imageFlag & DISPLAY_FILENAME))
					std::cout << obs << std::endl;
				printObsImages (obs);
				break;
			case EXT_INFO:
				std::cout << obs << std::endl;
			case IMAGES:
			case IMAGES_ASTR_OK:
			case IMAGES_TRASH:
			case IMAGES_QUE:
				printObsImages (obs);
				break;
			case FLAT_IMAGES:
				return displayFlats ();
			case DARK_IMAGES:
				return displayDarks ();
		}
	}
	return 0;
}


int
main (int argc, char **argv)
{
	Rts2ObsInfo app = Rts2ObsInfo (argc, argv);
	return app.run ();
}
