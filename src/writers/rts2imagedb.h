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
#include "../utilsdb/rts2taruser.h"

// process_bitfield content
#define ASTROMETRY_PROC	0x01
// when & -> image should be in archive, otherwise it's in trash|que
// depending on ASTROMETRY_PROC
#define ASTROMETRY_OK	0x02
#define DARK_OK		0x04
#define FLAT_OK		0x08
// some error durring image operations occured, information in DB is unrealiable
#define IMG_ERR		0x8000

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
  inline int isCalibrationImage ();
  void updateCalibrationDb ();

  char *filter;

public:
    Rts2ImageDb (Target * currTarget, Rts2DevClientCamera * camera,
		 const struct timeval *expStartd);
    Rts2ImageDb (const char *in_filename);
  //! Construct image directly from DB (eg. retrieve all missing parameters)
    Rts2ImageDb (int in_obs_id, int in_img_id);
  //! Construcy image from one database row..
    Rts2ImageDb (int in_tar_id, int in_obs_id, int in_img_id,
		 char in_obs_subtype, long in_img_date, int in_img_usec,
		 float in_img_exposure, float in_img_temperature,
		 const char *in_img_filter, float in_img_alt, float in_img_az,
		 const char *in_camera_name, const char *in_mount_name,
		 bool in_delete_flag, int in_process_bitfield,
		 double in_img_err_ra, double in_img_err_dec,
		 double in_img_err, int in_epoch_id);
    virtual ~ Rts2ImageDb (void);

  virtual int toArchive ();
  virtual int toTrash ();

  virtual int saveImage ();
  virtual int deleteImage ();

  const char *getFilter ()
  {
    return filter;
  }

  int getOKCount ();

  bool haveOKAstrometry ()
  {
    return (processBitfiedl & ASTROMETRY_OK);
  }

  bool isProcessed ()
  {
    return (processBitfiedl & ASTROMETRY_PROC);
  }

  friend std::ostream & operator << (std::ostream & _os,
				     Rts2ImageDb & img_db);

  virtual void printFileName (std::ostream & _os);

  virtual void getFileName (std::string & out_filename);

  void print (std::ostream & _os, int in_flags = 0);
};

std::ostream & operator << (std::ostream & _os, Rts2ImageDb & img_db);

#endif /* ! __RTS2_IMAGEDB__ */
