#include "target_auger.h"
#include "../utils/timestamp.h"
#include "../utils/infoval.h"

TargetAuger::TargetAuger (int in_tar_id, struct ln_lnlat_posn * in_obs):Target (in_tar_id, in_obs)
{

}

TargetAuger::~TargetAuger (void)
{

}

int
TargetAuger::load ()
{
  EXEC SQL BEGIN DECLARE SECTION;
  int d_auger_t3id;
  long d_auger_date;
  int d_auger_npixels;
  double d_auger_sdpphi;
  double d_auger_sdptheta;
  double d_auger_sdpangle;
  EXEC SQL END DECLARE SECTION;

  EXEC SQL
  SELECT
    auger_t3id,
    EXTRACT (EPOCH FROM auger_date),
    auger_npixels,
    auger_sdpphi,
    auger_sdptheta,
    auger_sdpangle
  INTO
    :d_auger_t3id,
    :d_auger_date,
    :d_auger_npixels,
    :d_auger_sdpphi,
    :d_auger_sdptheta,
    :d_auger_sdpangle
  FROM
    auger
  WHERE
    auger_t3id = (SELECT MAX(auger_t3id) FROM auger);

  if (sqlca.sqlcode)
  {
    EXEC SQL ROLLBACK;
    return -1;
  }
  EXEC SQL ROLLBACK;
  
  t3id = d_auger_t3id;
  date = d_auger_date;
  npixels = d_auger_npixels;
  sdpphi = d_auger_sdpphi;
  sdptheta = d_auger_sdptheta;
  sdpangle = d_auger_sdpangle;

  return Target::load ();
}

int
TargetAuger::getPosition (struct ln_equ_posn *pos, double JD)
{
  // samehow calculate auger position
  return 0;
}

float
TargetAuger::getBonus (double JD)
{
  time_t jd_date;
  ln_get_timet_from_julian (JD, &jd_date);
  // shower too old - observe for 30 minutes..
  if (jd_date > date + 1800)
    return 0;
  // else return value from DB
  return Target::getBonus (JD);
}

void
TargetAuger::printExtra (std::ostream & _os)
{
  _os
    << InfoVal<int> ("T3ID", t3id)
    << InfoVal<Timestamp> ("DATE", Timestamp(date))
    << InfoVal<int> ("NPIXELS", npixels)
    << InfoVal<double> ("SDP_PHI", sdpphi)
    << InfoVal<double> ("SDP_THETA", sdptheta)
    << InfoVal<double> ("SDP_ANGLE", sdpangle);
}
