/**
 * Used for Image-DB interaction.
 *
 * Build on Rts2Image, persistently update image information in
 * database.
 *
 * @author petr
 */

#ifndef __RTS2_IMAGEDB__
#define __RTS2_IMAGEDB__

#include "rts2image.h"

class Rts2ImageDb:public Rts2Image
{
private:
  void reportSqlError (char *msg);
  int updateObjectDB ();
  int updateDarkDB ();
  int updateFlatDB ();
  int updateDB ();
  int updateAstrometry ();
public:
    Rts2ImageDb (int in_epoch_id, int in_targetId,
		 Rts2DevClientCamera * camera, int in_obsId,
		 const struct timeval *exposureStart, int in_imgId);
    Rts2ImageDb (char *in_filename);
  virtual int saveImage ();
};

#endif /* ! __RTS2_IMAGEDB__ */
