#ifndef __RTS2_IMGSETSTAT__
#define __RTS2_IMGSETSTAT__

#include <string>
#include <ostream>

/**
 * Holds statistics for given filters.
 */
class Rts2ImgSetStat
{
public:
  Rts2ImgSetStat ()
  {
    filter = std::string ("");
    img_alt = 0;
    img_az = 0;
    img_err = 0;
    img_err_ra = 0;
    img_err_dec = 0;
    count = 0;
    astro_count = 0;
  }
   ~Rts2ImgSetStat (void)
  {
  }
  std::string filter;
  float img_alt;
  float img_az;
  float img_err;
  float img_err_ra;
  float img_err_dec;
  int count;
  int astro_count;

  double exposure;

  void stat ();

  friend std::ostream & operator << (std::ostream & _os,
				     Rts2ImgSetStat & stat);
};

std::ostream & operator << (std::ostream & _os, Rts2ImgSetStat & stat);

#endif /* ! __RTS2_IMGSETSTAT__ */
