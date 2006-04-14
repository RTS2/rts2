#ifndef __RTS2_IMGSET__
#define __RTS2_IMGSET__

#include "../writers/rts2imagedb.h"

#include <iostream>
#include <vector>

class Rts2Obs;
class Rts2ImageDb;

/**
 * Class for accesing images.
 *
 * Compute statistics on images, allow easy access to images,..
 *
 * @author petr
 */
class Rts2ImgSet:public
  std::vector <
Rts2ImageDb * >
{
private:
  float
    img_alt;
  float
    img_az;
  float
    img_err;
  float
    img_err_ra;
  float
    img_err_dec;

  int
    count;
  int
    astro_count;

protected:
  int
  load (std::string in_where);
  void
  stat ();
public:
  Rts2ImgSet ();
  // abstract. subclasses needs to define that
  virtual int
  load () = 0;
  virtual ~
  Rts2ImgSet (void);
  void
  print (std::ostream & _os, int printImages);
  int
  getAverageErrors (double &eRa, double &eDec, double &eRad);

  friend
    std::ostream &
  operator << (std::ostream & _os, Rts2ImgSet & img_set);
};

class
  Rts2ImgSetTarget:
  public
  Rts2ImgSet
{
private:
  int
    tar_id;
public:
  Rts2ImgSetTarget (int in_tar_id);
  virtual int
  load ();
};

class
  Rts2ImgSetObs:
  public
  Rts2ImgSet
{
private:
  Rts2Obs *
    observation;
public:
  Rts2ImgSetObs (Rts2Obs * in_observation);
  virtual int
  load ();
};

class
  Rts2ImgSetPosition:
  public
  Rts2ImgSet
{
private:
  struct ln_equ_posn
    pos;
public:
  Rts2ImgSetPosition (struct ln_equ_posn *in_pos);
  virtual int
  load ();
};

std::ostream & operator << (std::ostream & _os, Rts2ImgSet & img_set);

#endif /* !__RTS2_IMGSET__ */
