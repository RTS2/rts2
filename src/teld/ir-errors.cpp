#include "ir.h"

class Rts2DevIrError:public Rts2DevTelescopeIr
{
  std::list < const char *>errList;
protected:
    virtual int processArgs (const char *arg);
public:
    Rts2DevIrError (int in_argc, char **in_argv):Rts2DevTelescopeIr (in_argc,
								     in_argv)
  {
  }
  virtual ~ Rts2DevIrError (void)
  {
  }
  virtual int run ();
};

int
Rts2DevIrError::processArgs (const char *arg)
{
  errList.push_back (arg);
  return 0;
}

int
Rts2DevIrError::run ()
{
  std::string desc;
  for (std::list < const char *>::iterator iter = errList.begin ();
       iter != errList.end (); iter++)
    {
      getError (atoi (*iter), desc);
      std::cout << desc << std::endl;
    }
  return 0;
}

int
main (int argc, char **argv)
{
  Rts2DevIrError *device = new Rts2DevIrError (argc, argv);

  int ret;
  ret = device->initOptions ();
  if (ret)
    {
      exit (ret);
    }
  ret = device->initDevice ();
  if (ret)
    {
      std::cerr << "Cannot initialize telescope bridge - exiting!" << std::
	endl;
      exit (ret);
    }
  ret = device->run ();
  delete device;

  return ret;
}
