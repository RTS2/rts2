#include "imgdisplay.h"
#include "rts2imgset.h"
#include "rts2obs.h"

#include <iostream>
#include <iomanip>
#include <sstream>

#include "../utils/libnova_cpp.h"

Rts2ImgSet::Rts2ImgSet ()
{
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
Rts2ImgSet::load (std::string in_where)
{
  EXEC SQL BEGIN DECLARE SECTION;
  char *stmp_c;

  int d_tar_id;
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

  int d_epoch_id;
  EXEC SQL END DECLARE SECTION;

  img_alt = 0;
  img_az  = 0;
  img_err = 0;
  img_err_ra  = 0;
  img_err_dec = 0;

  count = 0;
  astro_count = 0;

  asprintf (&stmp_c,
  "SELECT "
    "tar_id,"
    "img_id,"
    "images.obs_id,"
    "obs_subtype,"
    "EXTRACT (EPOCH FROM img_date),"
    "img_usec,"
    "img_exposure,"
    "img_temperature,"
    "img_filter,"
    "img_alt,"
    "img_az,"
    "camera_name,"
    "mount_name,"
    "delete_flag,"
    "process_bitfield,"
    "img_err_ra,"
    "img_err_dec,"
    "img_err,"
    "epoch_id"
  " FROM "
    "images,"
    "observations"
  " WHERE "
    " images.obs_id = observations.obs_id AND %s "
  " ORDER BY "
    "img_id DESC;", in_where.c_str());

  EXEC SQL PREPARE cur_images_stmp FROM :stmp_c;

  EXEC SQL DECLARE cur_images CURSOR FOR cur_images_stmp;

  EXEC SQL OPEN cur_images;
  while (1)
  {
    EXEC SQL FETCH next FROM cur_images INTO
      :d_tar_id,
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
      :d_img_err :d_img_err_ind,
      :d_epoch_id;
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

    d_img_filter.arr[d_img_filter.len] = '\0';
      
    push_back (new Rts2ImageDb (d_tar_id, d_obs_id, d_img_id, d_obs_subtype,
      d_img_date, d_img_usec, d_img_exposure, d_img_temperature, d_img_filter.arr, d_img_alt, d_img_az,
      d_camera_name.arr, d_mount_name.arr, d_delete_flag, d_process_bitfield, d_img_err_ra,
      d_img_err_dec, d_img_err, d_epoch_id));

  }
  free (stmp_c);
  if (sqlca.sqlcode != ECPG_NOT_FOUND)
  {
    std::cerr << "Rts2ImgSet::load error in DB: " << sqlca.sqlerrm.sqlerrmc << std::endl;
    EXEC SQL CLOSE cur_images;
    EXEC SQL ROLLBACK;
    return -1;
  }
  EXEC SQL CLOSE cur_images;
  EXEC SQL COMMIT;

  stat ();

  return 0;
}

void
Rts2ImgSet::stat ()
{
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
}

void
Rts2ImgSet::print (std::ostream &_os, int printImages)
{
  if (printImages & DISPLAY_ALL)
  {
    std::vector <Rts2ImageDb *>::iterator img_iter;
    if (empty () && !(printImages & DISPLAY_SHORT))
    {
      _os << "      " << "--- no images ---" << std::endl;
      return;
    }
    for (img_iter = begin (); img_iter != end (); img_iter++)
    {
      Rts2ImageDb *image = (*img_iter);
      if ((printImages & DISPLAY_ASTR_OK) && !image->haveOKAstrometry ())
	continue;
      if ((printImages & DISPLAY_ASTR_TRASH) && (image->haveOKAstrometry () || !image->isProcessed ()))
	continue;
      if ((printImages & DISPLAY_ASTR_QUE) && image->isProcessed ())
	continue;
      image->print (_os, printImages);
    }
  }
  if (printImages & DISPLAY_SUMMARY)
  {
    _os << "      " << *this << std::endl;
  }
}

int
Rts2ImgSet::getAverageErrors (double &eRa, double &eDec, double &eRad)
{
  double tRa;
  double tDec;
  double tRad;

  eRa = 0;
  eDec = 0;
  eRad = 0;

  int aNum = 0;

  std::vector <Rts2ImageDb *>::iterator img_iter;
  for (img_iter = begin (); img_iter != end (); img_iter++)
  {
    if (((*img_iter)->getError (tRa, tDec, tRad)) == 0)
    {
      eRa += tRa;
      eDec += tDec;
      eRad += tRad;
      aNum++;
    }
  }
  if (aNum > 0)
  {
    eRa /= aNum;
    eDec /= aNum;
    eRad /= aNum;
  }
  return aNum;
}

Rts2ImgSetTarget::Rts2ImgSetTarget (int in_tar_id)
{
  tar_id = in_tar_id;
}

int
Rts2ImgSetTarget::load ()
{
  std::ostringstream os;
  os << "tar_id = " << tar_id;
  return Rts2ImgSet::load (os.str());
}


Rts2ImgSetObs::Rts2ImgSetObs (Rts2Obs *in_observation)
{
  observation = in_observation;
}

int
Rts2ImgSetObs::load ()
{
  std::ostringstream os;
  os << "observations.obs_id = " << observation->getObsId ();
  return Rts2ImgSet::load (os.str());
}

Rts2ImgSetPosition::Rts2ImgSetPosition (struct ln_equ_posn * in_pos)
{
  pos = *in_pos;
}

int
Rts2ImgSetPosition::load ()
{
  std::ostringstream os;
  os << "isinwcs (" << pos.ra 
    << ", " << pos.dec
    << ", astrometry)";
  return Rts2ImgSet::load (os.str ());
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
