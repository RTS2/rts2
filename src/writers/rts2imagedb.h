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
#include "../utilsdb/target.h"

// process_bitfield content
#define ASTROMETRY_PROC	0x01
// when & -> image should be in archive, otherwise it's in trash|que
// depending on ASTROMETRY_PROC
#define ASTROMETRY_OK	0x02
#define DARK_OK		0x04
#define FLAT_OK		0x08

class Rts2ImageDb:public Rts2Image
{
private:
  void reportSqlError (char *msg);
  int updateObjectDB ();
  int updateDarkDB ();
  int updateFlatDB ();
  int updateDB ();
  int updateAstrometry ();

  int setDarkFromDb ();

  int processBitfiedl;
  void initDbImage ();
  int isCalibrationImage ()
  {
    return (getTargetType () == TYPE_CALIBRATION
	    || getTargetType () == TYPE_PHOTOMETRIC);
  }
  void updateCalibrationDb ();
public:
  Rts2ImageDb (Target * currTarget, Rts2DevClientCamera * camera,
	       const struct timeval *expStartd);
  Rts2ImageDb (const char *in_filename);
  virtual ~ Rts2ImageDb (void);

  virtual int toArchive ();
  virtual int toTrash ();

  virtual int saveImage ();
  virtual int deleteImage ();
};

#endif /* ! __RTS2_IMAGEDB__ */
