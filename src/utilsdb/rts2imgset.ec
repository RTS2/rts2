#include "rts2imgset.h"
#include "rts2obs.h"

#include <iostream>

Rts2ImgSet::Rts2ImgSet ()
{
  observation = NULL;
}

Rts2ImgSet::Rts2ImgSet (Rts2Obs *in_observation)
{
  observation = in_observation;
}

int
Rts2ImgSet::load ()
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

  int d_img_err_ra_ind;
  int d_img_err_dec_ind;
  int d_img_err_ind;
  EXEC SQL END DECLARE SECTION;

  int tar_id;
  int count = 0;

  if (observation)
  {
    d_obs_id = observation->getObsId ();
    tar_id = observation->getTargetId ();
  }
  else
  {
    d_obs_id = 0;
    tar_id = 0;
  }

  img_alt = 0;
  img_az  = 0;
  img_err = 0;
  img_err_ra  = 0;
  img_err_dec = 0;

  count = 0;

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
      :d_img_temperature,
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
    if (! (d_process_bitfield & ASTROMETRY_OK))
    {
      d_img_err_ra = nan ("f");
      d_img_err_dec = nan ("f");
      d_img_err = nan ("f");
    }

    img_alt += d_img_alt;
    img_az  += d_img_az;
    img_err += d_img_err;
    img_err_ra  += d_img_err_ra;
    img_err_dec += d_img_err_dec;
    count++;
      
    push_back (new Rts2ImageDb (tar_id, d_obs_id, d_img_id, d_obs_subtype,
      d_img_date, d_img_usec, d_img_exposure, d_img_temperature, d_img_filter.arr, d_img_alt, d_img_az,
      d_camera_name.arr, d_mount_name.arr, d_delete_flag, d_process_bitfield, d_img_err_ra,
      d_img_err_dec, d_img_err));

  }
  if (sqlca.sqlcode != ECPG_NOT_FOUND)
  {
    std::cerr << "Rts2Obs::loadImages error in DB: " << sqlca.sqlerrm.sqlerrmc << std::endl;
    EXEC SQL CLOSE cur_images;
    EXEC SQL ROLLBACK;
    return -1;
  }
  EXEC SQL CLOSE cur_images;
  EXEC SQL COMMIT;

  img_alt /= count;
  img_az  /= count;
  img_err /= count;
  img_err_ra  /= count;
  img_err_dec /= count;

  return 0;
}

std::ostream & operator << (std::ostream &_os, Rts2ImgSet &img_set)
{
  _os << "Number of images:" << img_set.size ()
    << " avg. alt:" << img_set.img_alt
    << " avg. err:" << img_set.img_err;
  return _os;
}
