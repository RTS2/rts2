#include "rts2imagedb.h"
#include "../utils/timestamp.h"
#include "../utils/libnova_cpp.h"

#include <iomanip>
#include <libnova/airmass.h>

#define DISPLAY_OBS		0x04

EXEC SQL include sqlca;

void
Rts2ImageDb::reportSqlError (char *msg)
{
  syslog (LOG_DEBUG, "SQL error %li %s (in %s)\n", sqlca.sqlcode,
  sqlca.sqlerrm.sqlerrmc, msg);
}

int
Rts2ImageDb::updateObjectDB ()
{
  EXEC SQL BEGIN DECLARE SECTION;
  int d_img_id = imgId;
  int d_obs_id = obsId;
  char d_obs_subtype = 'S';
  int d_img_date = getExposureSec ();
  int d_img_usec = getExposureUsec ();
  float d_img_temperature = -500;
  int d_img_temperature_ind;
  float d_img_exposure = -1;
  float d_img_alt = -100;
  float d_img_az = -100;
  int d_epoch_id = epochId;
  int d_med_id = 0;
  int d_proccess_bitfield = processBitfiedl;
  VARCHAR d_mount_name[8];
  VARCHAR d_camera_name[8];
  VARCHAR d_img_filter[3];
  EXEC SQL END DECLARE SECTION;

  char tmp_filter[4] = "UNK";

  strncpy (d_mount_name.arr, getMountName (), 8);
  d_mount_name.len = strlen (getMountName ());

  strncpy (d_camera_name.arr, getCameraName (), 8);
  d_camera_name.len = strlen (getCameraName ());

  getValue ("FILTER", tmp_filter, 4);

  d_img_filter.len = strlen (tmp_filter) > 3 ? 3 : strlen (tmp_filter);
  strncpy (d_img_filter.arr, tmp_filter, d_img_filter.len);

  d_img_temperature_ind = getValue ("CCD_TEMP", d_img_temperature);
  if (isnan (d_img_temperature))
  {
    d_img_temperature = 100;
    d_img_temperature_ind = -1;
  }
  d_img_exposure = getExposureLength ();
  getValue ("ALT", d_img_alt);
  getValue ("AZ", d_img_az);

  EXEC SQL
  INSERT INTO
    images
  (
    img_id,
    obs_id,
    obs_subtype,
    mount_name,
    camera_name,
    img_temperature,
    img_exposure,
    img_filter,
    img_alt,
    img_az,
    img_date,
    img_usec,
    epoch_id,
    med_id,
    process_bitfield
  )
  VALUES
  (
    :d_img_id,
    :d_obs_id,
    :d_obs_subtype,
    :d_mount_name,
    :d_camera_name,
    :d_img_temperature :d_img_temperature_ind,
    :d_img_exposure,
    :d_img_filter,
    :d_img_alt,
    :d_img_az,
    abstime (:d_img_date),
    :d_img_usec,
    :d_epoch_id,
    :d_med_id,
    :d_proccess_bitfield
  );
  // data found..do just update
  if (sqlca.sqlcode != 0)
  {
    printf ("Cannot insert new image, triing update (error: %li %s)\n",
    sqlca.sqlcode, sqlca.sqlerrm.sqlerrmc);
    EXEC SQL ROLLBACK;
    EXEC SQL
    UPDATE
      images
    SET
      img_date = abstime (:d_img_date),
      img_usec = :d_img_usec,
      epoch_id = :d_epoch_id,
      med_id   = :d_med_id,
      process_bitfield = :d_proccess_bitfield
    WHERE
        img_id = :d_img_id
      AND obs_id = :d_obs_id;
    // still error.. return -1
    if (sqlca.sqlcode != 0)
    {
      reportSqlError ("image OBJECT update");
      EXEC SQL ROLLBACK;
      return -1;
    }
  }
  EXEC SQL COMMIT;
  return updateAstrometry ();
}

int
Rts2ImageDb::updateDarkDB ()
{
  EXEC SQL BEGIN DECLARE SECTION;
  int d_img_id = imgId;
  int d_obs_id = obsId;
  int d_dark_date = getExposureSec ();
  int d_dark_usec = getExposureUsec ();
  float d_dark_exposure;
  float d_dark_temperature = 100;
  int d_dark_temperature_ind;
  float d_dark_mean = getAverage ();
  int d_epoch_id = epochId;
  VARCHAR d_camera_name[8];
  EXEC SQL END DECLARE SECTION;

  d_dark_temperature_ind = getValue ("CCD_TEMP", d_dark_temperature);
  d_dark_exposure = getExposureLength ();

  strncpy (d_camera_name.arr, cameraName, 8);
  d_camera_name.len = strlen (cameraName);

  EXEC SQL
  INSERT INTO
    darks
  (
    img_id,
    obs_id,
    dark_date,
    dark_usec,
    dark_exposure,
    dark_temperature,
    dark_mean,
    epoch_id,
    camera_name
  ) VALUES (
    :d_img_id,
    :d_obs_id,
    abstime (:d_dark_date),
    :d_dark_usec,
    :d_dark_exposure,
    :d_dark_temperature :d_dark_temperature_ind,
    :d_dark_mean,
    :d_epoch_id,
    :d_camera_name
  );
  if (sqlca.sqlcode)
  {
    reportSqlError ("image DARK update");
    EXEC SQL ROLLBACK;
    return -1;
  }
  EXEC SQL COMMIT;
  return 0;
}

int
Rts2ImageDb::updateFlatDB ()
{
  // flat images aren't stored in DB - we store only finished flats
  return 0;
}

int
Rts2ImageDb::updateDB ()
{
  switch (getType ())
  {
    case IMGTYPE_OBJECT:
      return updateObjectDB ();
    case IMGTYPE_DARK:
      return updateDarkDB ();
    case IMGTYPE_FLAT:
      return updateFlatDB ();
    default:
      break;
  }
  syslog (LOG_DEBUG, "unknow image type: %i", imageType);
  return -1;
}

int
Rts2ImageDb::updateAstrometry ()
{
  EXEC SQL BEGIN DECLARE SECTION;
  int d_obs_id = obsId;
  int d_img_id = imgId;

  double d_img_err_ra;
  double d_img_err_dec;
  double d_img_err;

  VARCHAR s_astrometry[2000];
  EXEC SQL END DECLARE SECTION;

  long a_naxis[2];
  char *ctype[2];
  double crpix[2];
  double crval[2];
  double cdelt[2];
  double crota[2];
  double equinox;
  double epoch;

  int ret;

  ctype[0] = (char *) malloc (9);
  ctype[1] = (char *) malloc (9);

  ret = getValues ("NAXIS", a_naxis, 2);
  if (ret)
    return -1;
  ret = getValues ("CTYPE", (char **) &ctype, 2);
  if (ret)
    return -1;
  ret = getValues ("CRPIX", crpix, 2);
  if (ret)
    return -1;
  ret = getValues ("CRVAL", crval, 2);
  if (ret)
    return -1;
  ret = getValues ("CDELT", cdelt, 2);
  if (ret)
    return -1;
  ret = getValues ("CROTA", crota, 2);
  if (ret)
    return -1;
  ret = getValue ("EQUINOX", equinox);
  if (ret)
    return -1;
  ret = getValue ("EPOCH", epoch);
  if (ret)
    return -1;

  d_img_err_ra = ra_err;
  d_img_err_dec = dec_err;
  d_img_err = getAstrometryErr ();

  snprintf (s_astrometry.arr, 2000,
    "NAXIS1 %ld NAXIS2 %ld CTYPE1 %s CTYPE2 %s CRPIX1 %f CRPIX2 %f "
    "CRVAL1 %f CRVAL2 %f CDELT1 %f CDELT2 %f CROTA %f EQUINOX %f EPOCH %f",
    a_naxis[0], a_naxis[1], ctype[0], ctype[1], crpix[0], crpix[1],
    crval[0], crval[1], cdelt[0], cdelt[1], crota[0], equinox, epoch);
  s_astrometry.len = strlen (s_astrometry.arr);

  EXEC SQL UPDATE
    images
  SET
    astrometry  = :s_astrometry,
    img_err_ra  = :d_img_err_ra,
    img_err_dec = :d_img_err_dec,
    img_err     = :d_img_err,
    process_bitfield = process_bitfield | 1 | 2
  WHERE
      obs_id = :d_obs_id
    AND img_id = :d_img_id;
  EXEC SQL COMMIT;
  if (sqlca.sqlcode != 0)
  {
    reportSqlError ("astrometry update");
    return -1;
  }
  processBitfiedl |= ASTROMETRY_PROC | ASTROMETRY_OK;
  return 0;  
}

int
Rts2ImageDb::setDarkFromDb ()
{
  EXEC SQL BEGIN DECLARE SECTION;
  VARCHAR d_camera_name[8];
  int d_date = getExposureSec ();
  VARCHAR d_dark_name[250];
  double d_img_temperature = nan("f");
  double d_img_exposure = nan("f");
  EXEC SQL END DECLARE SECTION;

  if (!cameraName)
    return -1;

  if (!(getType () == IMGTYPE_OBJECT
  || getType () == IMGTYPE_FLAT))
    return 0;
    
  strncpy (d_camera_name.arr, cameraName, 8);
  d_camera_name.len = strlen (cameraName);

  getValue ("CCD_TEMP", d_img_temperature);
  d_img_exposure = getExposureLength ();

  EXEC SQL DECLARE dark_cursor CURSOR FOR
    SELECT
      dark_name (obs_id, img_id, dark_date, dark_usec, epoch_id, camera_name)
    FROM
      darks
    WHERE
        camera_name = :d_camera_name
      AND dark_exposure = :d_img_exposure
      AND abs (dark_temperature - :d_img_temperature) < 2
      AND abs (EXTRACT (EPOCH FROM dark_date) - :d_date) < 60000
    ORDER BY
      dark_date DESC;
  EXEC SQL OPEN dark_cursor;
  EXEC SQL FETCH next FROM dark_cursor INTO
    :d_dark_name;
  if (sqlca.sqlcode)
  {
    syslog (LOG_DEBUG, "Rts2ImageDb::setDarkFromDb SQL error: %s (%li)",
      sqlca.sqlerrm.sqlerrmc, sqlca.sqlcode);
    EXEC SQL CLOSE dark_cursor;
    EXEC SQL ROLLBACK;
    return -1;
  }
  EXEC SQL CLOSE dark_cursor;
  EXEC SQL COMMIT;
  setValue ("DARK", d_dark_name.arr, "dark image full path");
  processBitfiedl |= DARK_OK;

  return 0;
}

void
Rts2ImageDb::initDbImage ()
{
  processBitfiedl = 0;
  getValue ("PROC", processBitfiedl);
  filter = NULL;
}

int
Rts2ImageDb::isCalibrationImage ()
{
  return (getTargetType () == TYPE_CALIBRATION
	  || getTargetType () == TYPE_PHOTOMETRIC);
}

void
Rts2ImageDb::updateCalibrationDb ()
{
  EXEC SQL BEGIN DECLARE SECTION;
  int db_air_last_image;
  float db_airmass;
  int db_obs_id = getObsId ();
  int db_img_id = getImgId ();
  EXEC SQL END DECLARE SECTION;

  double img_alt;

  int ret;

  // we have calibration image processed..update airmass_cal_images table
  if (isCalibrationImage ())
  {
    ret = getValue ("ALT", img_alt);
    if (!ret)
    {
      db_airmass = ln_get_airmass (img_alt, 750.0);
      db_air_last_image = getExposureSec ();
      // delete - unset our old references (if any exists..)
      EXEC SQL
      UPDATE
        airmass_cal_images
      SET
        air_last_image = abstime (0),
	obs_id = NULL,
	img_id = NULL
      WHERE
          obs_id = :db_obs_id
	AND img_id = :db_img_id;

      if (sqlca.sqlcode && sqlca.sqlcode != ECPG_NOT_FOUND)
      {
        reportSqlError ("Rts2Image::updateCalibrationDb unseting airmass_cal_images");
	EXEC SQL ROLLBACK;
      }
      else
      {
        EXEC SQL COMMIT;
      }
      // if not processed, or processed and not failed..
      if (!(processBitfiedl & ASTROMETRY_PROC)
	|| (processBitfiedl & ASTROMETRY_OK))
      {
	EXEC SQL
	UPDATE
	  airmass_cal_images
	SET
	  air_last_image = abstime (:db_air_last_image),
	  obs_id = :db_obs_id,
	  img_id = :db_img_id
	WHERE
	    air_airmass_start <= :db_airmass
	  AND :db_airmass < air_airmass_end
	  AND air_last_image <= abstime (:db_air_last_image);

	if (sqlca.sqlcode)
	{
	  reportSqlError ("Rts2Image::toArchive updating airmass_cal_images");
	  EXEC SQL ROLLBACK;
	}
	else
	{
	  EXEC SQL COMMIT;
	}
      }
    }
  }
}

Rts2ImageDb::Rts2ImageDb (Target *currTarget, Rts2DevClientCamera * camera, const struct timeval *expStart) : 
  Rts2Image (currTarget, camera, expStart)
{
  initDbImage ();
}

Rts2ImageDb::Rts2ImageDb (const char *in_filename) : Rts2Image (in_filename)
{
  initDbImage ();
}

Rts2ImageDb::Rts2ImageDb (int in_obs_id, int in_img_id) : Rts2Image ()
{
  initDbImage ();
  // fill in filter
  // fill in exposureStart
  // fill in average (for darks..)
  // fill in target_id
  // fill in epochId
  // fill in camera_name
  // fill in mount_name
  // fill in obsId
  // fill in imgId
  // fill in ra_err
  // fill in dec_err
  // fill in pos_astr (from astrometry)
  // fill in imageType
  // fill in imageName
}

Rts2ImageDb::Rts2ImageDb (int in_tar_id, int in_obs_id, int in_img_id, char in_obs_subtype, long in_img_date, int in_img_usec, 
    float in_img_exposure, float in_img_temperature, const char *in_img_filter, float in_img_alt, float in_img_az, const char *in_camera_name,
    const char *in_mount_name, bool in_delete_flag, int in_process_bitfield, double in_img_err_ra, double in_img_err_dec,
    double in_img_err) : Rts2Image (in_img_date, in_img_usec, in_img_exposure)
{
  targetId = in_tar_id;
  targetIdSel = in_tar_id;
  targetType = in_obs_subtype;
  targetName = NULL;
  obsId = in_obs_id;
  imgId = in_img_id;
  cameraName = new char[strlen (in_camera_name) + 1];
  strcpy (cameraName, in_camera_name);
  mountName = new char[strlen (in_mount_name) + 1];
  strcpy (mountName, in_mount_name);
  focName = NULL;
  // construct image name..

  imageType = IMGTYPE_UNKNOW;
  ra_err = in_img_err_ra;
  dec_err = in_img_err_dec;
  img_err = in_img_err;

  // TODO fill that..
  pos_astr.ra = nan ("f");
  pos_astr.dec = nan ("f");
  processBitfiedl = in_process_bitfield;

  filter = new char[strlen(in_img_filter) + 1];
  strcpy (filter, in_img_filter);
}

Rts2ImageDb::~Rts2ImageDb ()
{
  delete[] filter;
}

int
Rts2ImageDb::toArchive ()
{
  int ret;

  processBitfiedl |= ASTROMETRY_OK | ASTROMETRY_PROC;

  ret = Rts2Image::toArchive ();
  if (ret)
  {
    processBitfiedl |= IMG_ERR;
    return ret;
  }

  return 0;
}

int
Rts2ImageDb::toTrash ()
{
  int ret; 

  processBitfiedl &= (~ASTROMETRY_OK);
  processBitfiedl |= ASTROMETRY_PROC;

  ret = Rts2Image::toTrash ();
  if (ret)
    return ret;

  return 0;
}

// write changes of image to DB..

int
Rts2ImageDb::saveImage ()
{
  if (!shouldSaveImage())
    return 0;
  updateDB ();
  setDarkFromDb ();
  updateCalibrationDb ();
  setValue ("PROC", processBitfiedl, "procesing status; info in DB");
  return Rts2Image::saveImage ();
}

int
Rts2ImageDb::deleteImage ()
{
  EXEC SQL BEGIN DECLARE SECTION;
  int d_img_id = imgId;
  int d_obs_id = obsId;
  EXEC SQL END DECLARE SECTION;

  if (getType () == IMGTYPE_OBJECT)
  {
    EXEC SQL
    DELETE FROM
      images
    WHERE
	img_id = :d_img_id
      AND obs_id = :d_obs_id;
  }
  else if (getType () == IMGTYPE_DARK)
  {
    EXEC SQL
    DELETE FROM
      darks
    WHERE
	img_id = :d_img_id
      AND obs_id = :d_obs_id;
  }
  if (sqlca.sqlcode)
  {
     reportSqlError ("Rts2ImageDb::deleteImage");
     EXEC SQL ROLLBACK;
  }
  else
  {
     EXEC SQL COMMIT;
  }
  return Rts2Image::deleteImage ();
}

int
Rts2ImageDb::getOKCount ()
{
  EXEC SQL BEGIN DECLARE SECTION;
  int db_obs_id = getObsId ();
  int db_count = 0;
  EXEC SQL END DECLARE SECTION;

  EXEC SQL
  SELECT
    count (*)
  INTO
    :db_count
  FROM
    images
  WHERE
      obs_id = :db_obs_id
    AND ((process_bitfield & 2) = 2);
  EXEC SQL ROLLBACK;

  return db_count;
}

void
Rts2ImageDb::print (std::ostream &_os, int in_flags)
{
  std::ios_base::fmtflags old_settings = _os.flags ();
  int old_precision = _os.precision (2);

  if (in_flags & DISPLAY_OBS)
    _os
      << std::setw(5) << getObsId () << " | ";

  _os 
    << std::setw(5) << getCameraName () << " | "
    << std::setw(4) << getImgId () << " | "
    << Timestamp (getExposureSec () + (double) getExposureUsec () / USEC_SEC) << " | "
    << std::setw(3) << getFilter () << " | "
    << std::setw(8) << getExposureLength () << "' | "
    << LibnovaDegArcMin (ra_err) << " | " 
    << LibnovaDegArcMin (dec_err) << " | "
    << LibnovaDegArcMin (img_err)
    << std::endl;

  _os.flags (old_settings);
  _os.precision (old_precision);
}

std::ostream & operator << (std::ostream &_os, Rts2ImageDb &img_db)
{
  img_db.print (_os);
  return _os;
}
