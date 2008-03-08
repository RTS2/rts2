#include "../utils/mkpath.h"
#include "../utilsdb/rts2appdb.h"
#include "../writers/rts2imagedb.h"

#include <iostream>
#include <string.h>
#include <list>

class Rts2DeleteApp: public Rts2AppDb
{
  int findImages ();
  int deleteImage (const char *in_name);

  int dont_delete;
  std::list <const char *> imageNames;

  protected:
    virtual int processOption (int in_opt);
    virtual int processArgs (const char *in_arg);
  public:
    Rts2DeleteApp (int argc, char **argv);
    virtual ~Rts2DeleteApp (void);

    virtual int doProcessing ();
};

int
Rts2DeleteApp::findImages ()
{
  // find images in db and run delete on them..
  return 0;
}


int
Rts2DeleteApp::deleteImage (const char *in_name)
{
  int ret;
  Rts2ImageDb *image;
  std::cout << "Delete image " << in_name << "..";
  if (dont_delete)
  {
    ret = 0;
  }
  else
  {
    image = new Rts2ImageDb (in_name);
    ret = image->deleteImage ();
    delete image;
  }
  if (ret)
  {
    std::cout << "error (" << errno << " - " << strerror (errno) << ")" << std::endl;
  }
  else
  {
    std::cout << "OK" << std::endl;
  }
  return ret;
}


Rts2DeleteApp::Rts2DeleteApp (int in_argc, char **in_argv) : Rts2AppDb (in_argc, in_argv)
{
  addOption ('n', "notmod", 1, "don't delete anything, just show what will be done");
  dont_delete = 0;
}


Rts2DeleteApp::~Rts2DeleteApp (void)
{
  imageNames.clear ();
}


int
Rts2DeleteApp::processOption (int in_opt)
{
  switch (in_opt)
  {
    case 'n':
      dont_delete = 1;
      break;
    default:
      return Rts2AppDb::processOption (in_opt);
  }
  return 0;
}


int
Rts2DeleteApp::processArgs (const char *in_arg)
{
  imageNames.push_back (in_arg);
  return 0;
}


int
Rts2DeleteApp::doProcessing ()
{
  int ret;
  if (imageNames.size () != 0)
  {
    std::list <const char *>::iterator img_iter;
    ret = 0;
    for (img_iter = imageNames.begin (); img_iter != imageNames.end (); img_iter++)
    {
      const char *an_name = *img_iter;
      ret = deleteImage (an_name);
      if (ret)
        return ret;
    }
    return ret;
  }
  return findImages ();
}


int
main (int argc, char **argv)
{
  Rts2DeleteApp app = Rts2DeleteApp (argc, argv);
  return app.run ();
}
