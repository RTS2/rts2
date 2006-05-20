#include "../utilsdb/rts2appdb.h"
#include "../utilsdb/rts2obs.h"
#include "../utilsdb/rts2obsset.h"

#include <list>
#include <iostream>

class Rts2ObsInfo:public Rts2AppDb
{
private:
  Rts2ObsSet * obsset;
  enum
  { BASIC_INFO, EXT_INFO, IMAGES, IMAGES_ASTR_OK, IMAGES_TRASH,
      IMAGES_QUE } action;
protected:
    virtual int processOption (int in_opt);
  virtual int processArgs (const char *arg);
public:
    Rts2ObsInfo (int in_argc, char **in_argv);
    virtual ~ Rts2ObsInfo (void);
  virtual int run ();
};

Rts2ObsInfo::Rts2ObsInfo (int in_argc, char **in_argv):
Rts2AppDb (in_argc, in_argv)
{
  obsset = new Rts2ObsSet ();
  addOption ('a', "action", 1,
	     "i->images, a->with astrometry, t->trash images, q->que images (not yet processed)");
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
	  break;
	case 't':
	  action = IMAGES_TRASH;
	  break;
	case 'q':
	  action = IMAGES_QUE;
	  break;
	default:
	  std::cerr << "Invalid action '" << *optarg << "'" << std::endl;
	  return -1;
	}
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
Rts2ObsInfo::run ()
{
  int ret;
  Rts2ObsSet::iterator iter;
  for (iter = obsset->begin (); iter != obsset->end (); iter++)
    {
      Rts2Obs obs = (Rts2Obs) * iter;
      ret = obs.load ();
      if (ret)
	return ret;
      switch (action)
	{
	case BASIC_INFO:
	  std::cout << obs << std::endl;
	  break;
	case EXT_INFO:
	  std::cout << obs << std::endl;
	case IMAGES:
	  ret = obs.loadImages ();
	  if (ret)
	    return ret;
	  obs.getImageSet ()->print (std::cout, DISPLAY_ALL);
	  break;
	case IMAGES_ASTR_OK:
	  break;
	case IMAGES_TRASH:
	  break;
	case IMAGES_QUE:
	  break;
	}
    }
  return 0;
}

Rts2ObsInfo *app;

int
main (int argc, char **argv)
{
  int ret;
  app = new Rts2ObsInfo (argc, argv);
  ret = app->init ();
  if (ret)
    return ret;
  ret = app->run ();
  delete app;
  return ret;
}
