#include "ir.h"

class Rts2DevIrError:public Rts2DevTelescopeIr
{
  std::list < const char *>errList;
  enum
  { NO_OP, CAL, RESET } op;
protected:
    virtual int processOption (int in_opt);
  virtual int processArgs (const char *arg);
public:
    Rts2DevIrError (int in_argc, char **in_argv);
    virtual ~ Rts2DevIrError (void)
  {
  }
  virtual int run ();
};

Rts2DevIrError::Rts2DevIrError (int in_argc, char **in_argv):
Rts2DevTelescopeIr (in_argc, in_argv)
{
  op = NO_OP;
  addOption ('C', "calculate", 0, "Calculate model");
  addOption ('R', "reset_model", 0, "Reset model counts");
}

int
Rts2DevIrError::processOption (int in_opt)
{
  switch (in_opt)
    {
    case 'C':
      op = CAL;
      break;
    case 'R':
      op = RESET;
      break;
    default:
      return Rts2DevTelescopeIr::processOption (in_opt);
    }
  return 0;
}

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
  int status = 0;
  double fparam;

  switch (op)
    {
    case NO_OP:
      break;
    case CAL:
      fparam = 2;
      status = tpl_set ("POINTING.POINTINGPARAMS.CALCULATE", fparam, &status);
      break;
    case RESET:
      fparam = 0;
      status =
	tpl_set ("POINTING.POINTINGPARAMS.RECORDCOUNT", fparam, &status);
      break;
    }

  // dump model
  double aoff, zoff, ae, an, npae, ca, flex;
  int recordcount;
  std::string dumpfile;

  status = tpl_get ("POINTING.POINTINGPARAMS.DUMPFILE", dumpfile, &status);
  status = tpl_get ("POINTING.POINTINGPARAMS.AOFF", aoff, &status);
  status = tpl_get ("POINTING.POINTINGPARAMS.ZOFF", zoff, &status);
  status = tpl_get ("POINTING.POINTINGPARAMS.AE", ae, &status);
  status = tpl_get ("POINTING.POINTINGPARAMS.AN", an, &status);
  status = tpl_get ("POINTING.POINTINGPARAMS.NPAE", npae, &status);
  status = tpl_get ("POINTING.POINTINGPARAMS.CA", ca, &status);
  status = tpl_get ("POINTING.POINTINGPARAMS.FLEX", flex, &status);

  std::cout << "POINTING.POINTINGPARAMS.DUMPFILE " << dumpfile << std::endl;
  std::cout.precision (20);
  std::cout << "AOFF = " << aoff << std::endl;
  std::cout << "ZOFF = " << zoff << std::endl;
  std::cout << "AE = " << ae << std::endl;
  std::cout << "AN = " << an << std::endl;
  std::cout << "NPAE = " << npae << std::endl;
  std::cout << "CA = " << ca << std::endl;
  std::cout << "FLEX = " << flex << std::endl;
  // dump offsets
  status = tpl_get ("AZ.OFFSET", aoff, &status);
  status = tpl_get ("ZD.OFFSET", zoff, &status);

  std::cout << "AZ.OFFSET " << aoff << std::endl;
  std::cout << "ZD.OFFSET " << zoff << std::endl;

  status =
    tpl_get ("POINTING.POINTINGPARAMS.RECORDCOUNT", recordcount, &status);

  std::cout << "POINTING.POINTINGPARAMS.RECORDCOUNT " << recordcount << std::
    endl;

  status = tpl_get ("POINTING.POINTINGPARAMS.CALCULATE", fparam, &status);

  std::cout << "POINTING.POINTINGPARAMS.CALCULATE " << fparam << std::endl;

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
