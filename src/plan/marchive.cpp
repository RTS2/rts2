#include "../writers/rts2imagedb.h"
#include "../utilsdb/rts2appdb.h"
#include "../utils/rts2config.h"

#include <list>
#include <iostream>

class MArchive:public Rts2AppDb
{
private:
  std::list < const char *>imageNames;
public:
    MArchive (int argc, char **argv);
    virtual ~ MArchive (void);

  virtual int processArgs (const char *arg);

  virtual int run ();
};

MArchive::MArchive (int argc, char **argv):
Rts2AppDb (argc, argv)
{
}

MArchive::~MArchive (void)
{
  imageNames.clear ();
}

int
MArchive::processArgs (const char *arg)
{
  imageNames.push_back (arg);
  return 0;
}

int
MArchive::run ()
{
  std::list < const char *>::iterator image_name;

  Rts2Config *config;
  config = Rts2Config::instance ();

  Rts2ImageDb *image;

  for (image_name = imageNames.begin (); image_name != imageNames.end ();
       image_name++)
    {
      const char *name = *image_name;
      char *img_name = new char[strlen (name) + 1];
      strcpy (img_name, name);
      image = new Rts2ImageDb (img_name);
      image->saveImage ();
      std::cout << "Saved image " << name << std::endl;
      delete image;
      delete img_name;
    }
}

int
main (int argc, char **argv)
{
  MArchive *archive;
  int ret;
  archive = new MArchive (argc, argv);
  ret = archive->init ();
  if (ret)
    return 1;
  archive->run ();

  delete archive;
  return 0;
}
