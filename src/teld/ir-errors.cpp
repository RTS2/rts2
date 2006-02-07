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
  // dump model
  int status = 0;
  double aoff, zoff, ae, an, npae, ca, flex;
  int recordcount;
  status = tpl_get ("POINTING.POINTINGPARAMS.AOFF", aoff, &status);
  status = tpl_get ("POINTING.POINTINGPARAMS.ZOFF", zoff, &status);
  status = tpl_get ("POINTING.POINTINGPARAMS.AE", ae, &status);
  status = tpl_get ("POINTING.POINTINGPARAMS.AN", an, &status);
  status = tpl_get ("POINTING.POINTINGPARAMS.NPAE", npae, &status);
  status = tpl_get ("POINTING.POINTINGPARAMS.CA", ca, &status);
  status = tpl_get ("POINTING.POINTINGPARAMS.FLEX", flex, &status);

  std::cout << "POINTING.POINTINGPARAMS.AOFF " << aoff << std::endl;
  std::cout << "POINTING.POINTINGPARAMS.ZOFF " << zoff << std::endl;
  std::cout << "POINTING.POINTINGPARAMS.AE " << ae << std::endl;
  std::cout << "POINTING.POINTINGPARAMS.AN " << an << std::endl;
  std::cout << "POINTING.POINTINGPARAMS.NPAE " << npae << std::endl;
  std::cout << "POINTING.POINTINGPARAMS.CA " << ca << std::endl;
  std::cout << "POINTING.POINTINGPARAMS.FLEX " << flex << std::endl;
  // dump offsets
  status = tpl_get ("AZ.OFFSET", aoff, &status);
  status = tpl_get ("ZD.OFFSET", zoff, &status);

  std::cout << "AZ.OFFSET " << aoff << std::endl;
  std::cout << "ZD.OFFSET " << zoff << std::endl;

  status =
    tpl_get ("POINTING.POINTINGPARAMS.RECORDCOUNT", recordcount, &status);

  std::cout << "POINTING.POINTINGPARAMS.RECORDCOUNT " << recordcount << std::
    endl;

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
