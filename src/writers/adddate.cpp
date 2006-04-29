#include "../writers/rts2appimage.h"
#include "../utils/rts2config.h"

#include <signal.h>
#include <iostream>

#include <list>

class Rts2AddDate:public Rts2AppImage
{
protected:
  virtual int processImage (Rts2Image * image)
  {
    int ret;
    time_t t;
      std::cout << "Processing " << image->getImageName () << "..";
      t = image->getExposureSec ();
      image->setValue ("DATE-OBS", &t, image->getExposureUsec (),
		       "date of observation");
      ret = image->saveImage ();
      std::cout << (ret ? "failed" : "OK") << std::endl;
      return 0;
  }
public:
    Rts2AddDate (int in_argc, char **in_argv):Rts2AppImage (in_argc, in_argv)
  {
    Rts2Config *config;
    config = Rts2Config::instance ();
  }
};

Rts2AddDate *app;

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
  app = new Rts2AddDate (argc, argv);
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
