#include "filterd.h"

class Rts2DevFilterdDummy:public Rts2DevFilterd
{
private:
  int filterNum;
  int filterSleep;
protected:
    virtual int getFilterNum (void)
  {
    return filterNum;
  }

  virtual int setFilterNum (int new_filter)
  {
    filterNum = new_filter;
    sleep (filterSleep);
    return 0;
  }

public:
Rts2DevFilterdDummy (int in_argc, char **in_argv):Rts2DevFilterd (in_argc,
		  in_argv)
  {
    filterNum = 0;
    filterSleep = 3;
    addOption ('s', "filter_sleep", 1, "how long wait for filter change");
  }

  virtual int processOption (int in_opt)
  {
    switch (in_opt)
      {
      case 's':
	filterSleep = atoi (optarg);
	break;
      default:
	return Rts2DevFilterd::processOption (in_opt);
      }
    return 0;
  }
};

int
main (int argc, char **argv)
{
  Rts2DevFilterdDummy device = Rts2DevFilterdDummy (argc, argv);
  return device.run ();
}
