#include "rts2imgset.h"
#include "rts2obs.h"

#include <iostream>
#include <iomanip>

#include "../utils/libnova_cpp.h"

Rts2ImgSet::Rts2ImgSet ()
{
  tar_id = 0;
  observation = NULL;
}

Rts2ImgSet::Rts2ImgSet (int in_tar_id)
{
  tar_id = in_tar_id;
  observation = NULL;
}

Rts2ImgSet::Rts2ImgSet (Rts2Obs *in_observation)
{
  tar_id = in_observation->getTargetId ();
  observation = in_observation;
}

Rts2ImgSet::~Rts2ImgSet (void)
{
  std::vector <Rts2ImageDb *>::iterator img_iter;
  for (img_iter = begin (); img_iter != end (); img_iter++)
  {
    delete *img_iter;
  }
  clear ();
}

int
Rts2ImgSet::loadTarget ()
{
  EXEC SQL BEGIN DECLARE SECTION;
  int d_tar_id = tar_id;
  int d_obs_id;
  int d_img_id;
  char d_obs_subtype;
  long d_img_date;
  int d_img_usec;
  float d_img_exposure;
  float d_img_temperature;
  VARCHAR d_img_filter[3];
  float d_img_alt;
  float d_img_az;
  VARCHAR d_camera_name[DEVICE_NAME_SIZE];
  VARCHAR d_mount_name[DEVICE_NAME_SIZE];
  bool d_delete_flag;
  int d_process_bitfield;
  double d_img_err_ra;
  double d_img_err_dec;
  double d_img_err;

  int d_img_temperature_ind;
  int d_img_err_ra_ind;
  int d_img_err_dec_ind;
  int d_img_err_ind;
  EXEC SQL END DECLARE SECTION;

  img_alt = 0;
  img_az  = 0;
  img_err = 0;
  img_err_ra  = 0;
  img_err_dec = 0;

  count = 0;
  astro_count = 0;

  EXEC SQL DECLARE cur_images_tar CURSOR FOR
  SELECT
    img_id,
    images.obs_id,
    images.obs_subtype,
    EXTRACT (EPOCH FROM img_date),
    img_usec,
    img_exposure,
    img_temperature,
    img_filter,
    img_alt,
    img_az,
    camera_name,
    mount_name,
    delete_flag,
    process_bitfield,
    img_err_ra,
    img_err_dec,
    img_err
  FROM
    images,
    observations
  WHERE
      tar_id = :d_tar_id
    AND observations.obs_id = images.obs_id
  ORDER BY
    img_date desc,
    img_id desc;

  EXEC SQL OPEN cur_images_tar;
  while (1)
  {
    EXEC SQL FETCH next FROM cur_images_tar INTO
      :d_img_id,
      :d_obs_id,
      :d_obs_subtype,
      :d_img_date,
      :d_img_usec,
      :d_img_exposure,
      :d_img_temperature :d_img_temperature_ind,
      :d_img_filter,
      :d_img_alt,
      :d_img_az,
      :d_camera_name,
      :d_mount_name,
      :d_delete_flag,
      :d_process_bitfield,
      :d_img_err_ra :d_img_err_ra_ind,
      :d_img_err_dec :d_img_err_dec_ind,
      :d_img_err :d_img_err_ind;
    if (sqlca.sqlcode)
      break;
    if (d_img_temperature_ind < 0)
      d_img_temperature = nan ("f");
    if (d_img_err_ra_ind < 0)
      d_img_err_ra = nan ("f");
    if (d_img_err_dec_ind < 0)
      d_img_err_dec = nan ("f");
    if (d_img_err_ind < 0)
      d_img_err = nan ("f");

    img_alt += d_img_alt;
    img_az  += d_img_az;
    if (!isnan (d_img_err))
    {
      img_err += d_img_err;
      img_err_ra  += d_img_err_ra;
      img_err_dec += d_img_err_dec;
      astro_count++;
    }
    count++;
      
    push_back (new Rts2ImageDb (tar_id, d_obs_id, d_img_id, d_obs_subtype,
      d_img_date, d_img_usec, d_img_exposure, d_img_temperature, d_img_filter.arr, d_img_alt, d_img_az,
      d_camera_name.arr, d_mount_name.arr, d_delete_flag, d_process_bitfield, d_img_err_ra,
      d_img_err_dec, d_img_err));

  }
  if (sqlca.sqlcode != ECPG_NOT_FOUND)
  {
    std::cerr << "Rts2ImgSet::loadTarget error in DB: " << sqlca.sqlerrm.sqlerrmc << std::endl;
    EXEC SQL CLOSE cur_images_tar;
    EXEC SQL ROLLBACK;
    return -1;
  }
  EXEC SQL CLOSE cur_images_tar;
  EXEC SQL COMMIT;

  return 0;
}

int
Rts2ImgSet::loadObs ()
{
  EXEC SQL BEGIN DECLARE SECTION;
  int d_obs_id;
  int d_img_id;
  char d_obs_subtype;
  long d_img_date;
  int d_img_usec;
  float d_img_exposure;
  float d_img_temperature;
  VARCHAR d_img_filter[3];
  float d_img_alt;
  float d_img_az;
  VARCHAR d_camera_name[DEVICE_NAME_SIZE];
  VARCHAR d_mount_name[DEVICE_NAME_SIZE];
  bool d_delete_flag;
  int d_process_bitfield;
  double d_img_err_ra;
  double d_img_err_dec;
  double d_img_err;

  int d_img_temperature_ind;
  int d_img_err_ra_ind;
  int d_img_err_dec_ind;
  int d_img_err_ind;
  EXEC SQL END DECLARE SECTION;

  d_obs_id = observation->getObsId ();
 
  img_alt = 0;
  img_az  = 0;
  img_err = 0;
  img_err_ra  = 0;
  img_err_dec = 0;

  count = 0;
  astro_count = 0;

  EXEC SQL DECLARE cur_images CURSOR FOR
  SELECT
    img_id,
    obs_subtype,
    EXTRACT (EPOCH FROM img_date),
    img_usec,
    img_exposure,
    img_temperature,
    img_filter,
    img_alt,
    img_az,
    camera_name,
    mount_name,
    delete_flag,
    process_bitfield,
    img_err_ra,
    img_err_dec,
    img_err
  FROM
    images
  WHERE
    obs_id = :d_obs_id
  ORDER BY
    img_id desc;

  EXEC SQL OPEN cur_images;
  while (1)
  {
    EXEC SQL FETCH next FROM cur_images INTO
      :d_img_id,
      :d_obs_subtype,
      :d_img_date,
      :d_img_usec,
      :d_img_exposure,
      :d_img_temperature :d_img_temperature_ind,
      :d_img_filter,
      :d_img_alt,
      :d_img_az,
      :d_camera_name,
      :d_mount_name,
      :d_delete_flag,
      :d_process_bitfield,
      :d_img_err_ra :d_img_err_ra_ind,
      :d_img_err_dec :d_img_err_dec_ind,
      :d_img_err :d_img_err_ind;
    if (sqlca.sqlcode)
      break;
    if (d_img_temperature_ind < 0)
      d_img_temperature = nan ("f");
    if (d_img_err_ra_ind < 0)
      d_img_err_ra = nan ("f");
    if (d_img_err_dec_ind < 0)
      d_img_err_dec = nan ("f");
    if (d_img_err_ind < 0)
      d_img_err = nan ("f");

    img_alt += d_img_alt;
    img_az  += d_img_az;
    if (!isnan (d_img_err))
    {
      img_err += d_img_err;
      img_err_ra  += d_img_err_ra;
      img_err_dec += d_img_err_dec;
      astro_count++;
    }
    count++;
      
    push_back (new Rts2ImageDb (tar_id, d_obs_id, d_img_id, d_obs_subtype,
      d_img_date, d_img_usec, d_img_exposure, d_img_temperature, d_img_filter.arr, d_img_alt, d_img_az,
      d_camera_name.arr, d_mount_name.arr, d_delete_flag, d_process_bitfield, d_img_err_ra,
      d_img_err_dec, d_img_err));

  }
  if (sqlca.sqlcode != ECPG_NOT_FOUND)
  {
    std::cerr << "Rts2ImgSet::loadObs error in DB: " << sqlca.sqlerrm.sqlerrmc << std::endl;
    EXEC SQL CLOSE cur_images;
    EXEC SQL ROLLBACK;
    return -1;
  }
  EXEC SQL CLOSE cur_images;
  EXEC SQL COMMIT;

  return 0;
}

int
Rts2ImgSet::load ()
{
  int ret;
  if (observation)
    ret = loadObs ();
  else
    ret = loadTarget ();

  // compute statistics

  img_alt /= count;
  img_az  /= count;
  if (astro_count > 0)
  {
    img_err /= astro_count;
    img_err_ra  /= astro_count;
    img_err_dec /= astro_count;
  }
  else
  {
    img_err = nan ("f");
    img_err_ra = nan ("f");
    img_err_dec = nan ("f");
  }

  return ret;
}

void
Rts2ImgSet::print (std::ostream &_os, int printImages)
{
  if (printImages & DISPLAY_ALL)
  {
    std::vector <Rts2ImageDb *>::iterator img_iter;
    if (empty ())
    {
      _os << "      " << "--- no images ---" << std::endl;
      return;
    }
    for (img_iter = begin (); img_iter != end (); img_iter++)
    {
      _os << "      " << *(*img_iter);
    }
  }
  if (printImages & DISPLAY_SUMMARY)
  {
    _os << "      " << *this << std::endl;
  }
}

std::ostream & operator << (std::ostream &_os, Rts2ImgSet &img_set)
{
  _os << "images:" << img_set.count;
  if (img_set.count == 0)
    return _os;
  int old_precision = _os.precision(0);
  _os << " with astrometry:" << img_set.astro_count
    << " (" << std::setw(3) << (100 * img_set.astro_count / img_set.count) << "%)";
  _os.precision(2);
  _os << " avg. alt:" << LibnovaDeg90 (img_set.img_alt)
    << " avg. err:" << LibnovaDegArcMin (img_set.img_err);
  _os.precision (old_precision);
  return _os;
}
