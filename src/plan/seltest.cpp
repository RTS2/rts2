#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include "../utilsdb/rts2appdb.h"
#include "../utils/rts2config.h"
#include "rts2selector.h"

#include <iostream>

class SelectorApp:public Rts2AppDb
{
	public:
		SelectorApp (int argc, char **argv);
		virtual ~ SelectorApp (void);
		virtual int doProcessing ();
};

SelectorApp::SelectorApp (int in_argc, char **in_argv):Rts2AppDb (in_argc, in_argv)
{
}

SelectorApp::~SelectorApp (void)
{
}

int SelectorApp::doProcessing ()
{
	int next_tar;

	Rts2Config *config;
	struct ln_lnlat_posn *observer;

	rts2plan::Selector *sel;

	Target *tar;

	config = Rts2Config::instance ();
	observer = config->getObserver ();

	sel = new rts2plan::Selector (observer);

	next_tar = sel->selectNextNight ();

	std::cout << "Next target:" << next_tar << std::endl;

	tar = createTarget (next_tar, observer);
	if (tar)
	{
		std::cout << *tar << std::endl;
	}
	else
	{
		std::cout << "cannot create target" << std::endl;
	}

	delete tar;

	delete sel;
	return 0;
}

int main (int argc, char **argv)
{
	SelectorApp selApp = SelectorApp (argc, argv);
	return selApp.run ();
}
