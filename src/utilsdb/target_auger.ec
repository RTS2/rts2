#include "target_auger.h"
#include "../utils/timestamp.h"
#include "../utils/infoval.h"

TargetAuger::TargetAuger (int in_tar_id, struct ln_lnlat_posn * in_obs):ConstTarget (in_tar_id, in_obs)
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
  double d_auger_date;
  int d_auger_npixels;
  double d_auger_ra;
  double d_auger_dec;
  EXEC SQL END DECLARE SECTION;

  struct ln_equ_posn pos;
  struct ln_hrz_posn hrz;
  double JD = ln_get_julian_from_sys ();

  EXEC SQL DECLARE cur_auger CURSOR FOR
    SELECT
      auger_t3id,
      EXTRACT (EPOCH FROM auger_date),
      auger_npixels,
      auger_ra,
      auger_dec
    FROM
      auger
    ORDER BY
      auger_date desc;

  EXEC SQL OPEN cur_auger;
  while (1)
    {
      EXEC SQL FETCH next FROM cur_auger INTO
        :d_auger_t3id,
        :d_auger_date,
        :d_auger_npixels,
        :d_auger_ra,
        :d_auger_dec;
      if (sqlca.sqlcode)
	break;
      pos.ra = d_auger_ra;
      pos.dec = d_auger_dec;
      ln_get_hrz_from_equ (&pos, observer, JD, &hrz);
      if (hrz.alt > 10)
      {
        t3id = d_auger_t3id;
        auger_date = d_auger_date;
        npixels = d_auger_npixels;
        setPosition (d_auger_ra, d_auger_dec);
        EXEC SQL CLOSE cur_auger;
	EXEC SQL COMMIT;
        return Target::load ();
      }
    }
  logMsgDb ("TargetAuger::load");
  EXEC SQL CLOSE cur_auger;
  EXEC SQL ROLLBACK;
  auger_date = 0;
  return -1;
}

float
TargetAuger::getBonus (double JD)
{
  time_t jd_date;
  ln_get_timet_from_julian (JD, &jd_date);
  // shower too old - observe for 30 minutes..
  if (jd_date > auger_date + 1800)
    return 1;
  // else return value from DB
  return ConstTarget::getBonus (JD);
}

void
TargetAuger::printExtra (std::ostream & _os)
{
  _os
    << InfoVal<int> ("T3ID", t3id)
    << InfoVal<Timestamp> ("DATE", Timestamp(auger_date))
    << InfoVal<int> ("NPIXELS", npixels);
}
