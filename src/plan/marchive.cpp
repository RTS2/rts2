#include "../writers/rts2appdbimage.h"
#include "../utils/rts2config.h"

#include <signal.h>
#include <iostream>

#include <list>

class Rts2MoveArchive:public Rts2AppDbImage
{
protected:
  virtual int processImage (Rts2ImageDb * image)
  {
    int ret;
    double val;
      std::cout << "Processing " << image->getImageName () << "..";
      ret = image->getValue ("CRVAL1", val);
    if (!ret)
        ret = image->toArchive ();
      std::cout << (ret ? "failed (not arhive?)" : "archive") << std::endl;
      return 0;
  }
public:
    Rts2MoveArchive (int argc, char **argv):Rts2AppDbImage (argc, argv)
  {
    Rts2Config *config;
    config = Rts2Config::instance ();
  }
};

Rts2MoveArchive *app;

void
killSignal (int sig)
{
  if (app)
    delete app;
}

int
main (int argc, char **argv)
{
  int ret;
  app = new Rts2MoveArchive (argc, argv);
  signal (SIGTERM, killSignal);
  signal (SIGINT, killSignal);
  ret = app->init ();
  if (ret)
    {
      delete app;
      return 0;
    }
  ret = app->run ();
  delete app;
  return ret;
}
