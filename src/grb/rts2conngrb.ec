#include "rts2conngrb.h"

#include <errno.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <unistd.h>

#include <libnova/libnova.h>

EXEC SQL include sqlca;

double
Rts2ConnGrb::getPktSod ()
{
  return lbuf[PKT_SOD]/100.0;
}

void
Rts2ConnGrb::getTimeTfromTJD (long TJD, double SOD, time_t * in_time, int * usec)
{
  double JD = getJDfromTJD (TJD, SOD);
  ln_get_timet_from_julian (JD, in_time);
  if (usec)
    *usec = (int)((SOD - int (SOD)) * USEC_SEC);
}

int
Rts2ConnGrb::pr_imalive ()
{
  deltaValue = here_sod - getPktSod ();
  syslog (LOG_DEBUG, "Rts2ConnGrb::pr_imalive last packet SN=%f delta=%5.2f last_delta=%.1f",
    getPktSod (), deltaValue, getPktSod () - last_imalive_sod);
  last_imalive_sod = getPktSod ();
  return 0;
}

int
Rts2ConnGrb::pr_swift_point ()
{
  double roll;
  char *obs_name;
  float obstime;
  float merit;
  swiftLastRa  = lbuf[7]/10000.0;
  swiftLastDec = lbuf[8]/10000.0;
  roll = lbuf[9]/10000.0;
  getTimeTfromTJD (lbuf[5], lbuf[6]/100.0, &swiftLastPoint);
  obs_name = (char*) (&lbuf[BURST_URL]);
  obstime = lbuf[14]/100.0;
  merit = lbuf[15]/100.0;
  return addSwiftPoint (roll, obs_name, obstime, merit);
}

int
Rts2ConnGrb::pr_integral_point ()
{
  struct ln_equ_posn pos_int, pos_j2000;
  time_t t;
  pos_int.ra = lbuf[14]/10000.0;
  pos_int.dec = lbuf[15]/10000.0;
  // precess to J2000
  ln_get_equ_prec2 (&pos_int, ln_get_julian_from_sys (), JD2000, &pos_j2000);
  getTimeTfromTJD (lbuf[5], lbuf[6]/100.0, &t);
  return addIntegralPoint (pos_j2000.ra, pos_j2000.dec, &t);
}

int
Rts2ConnGrb::pr_hete ()
{
  int grb_id;
  int grb_seqn;
  int grb_type;
  double grb_ra;
  double grb_dec;
  int grb_is_grb = 1;
  time_t grb_date;
  float grb_errorbox;

  grb_id = ((lbuf[BURST_TRIG] & H_TRIGNUM_MASK) >> H_TRIGNUM_SHIFT);
  grb_seqn = ((lbuf[BURST_TRIG] & H_SEQNUM_MASK) >> H_SEQNUM_SHIFT);
  grb_type = lbuf[PKT_TYPE];

  grb_ra = lbuf[BURST_RA] / 10000.0;
  grb_dec = lbuf[BURST_DEC] / 10000.0;

  getTimeTfromTJD (lbuf[BURST_TJD], lbuf[BURST_SOD]/100.0, &grb_date);

  grb_errorbox = (lbuf[H_WXM_DIM_NSIG] >> 16) / 60.0;

  if (!do_hete_test
    && (grb_type == TYPE_HETE_TEST 
      || (lbuf[H_TRIG_FLAGS] & H_ART_TRIG)
    )
  )
  {
    syslog (LOG_DEBUG, "Rts2ConnGrb::pr_hete test packet");
    return 0;
  }

  if ((lbuf[H_TRIG_FLAGS] & H_DEF_NOT_GRB)
    || (lbuf[H_TRIG_FLAGS] & H_DEF_SGR)
    || (lbuf[H_TRIG_FLAGS] & H_DEF_XRB))
    grb_is_grb = 0;
  
  return addGcnPoint (grb_id, grb_seqn, grb_type, grb_ra, grb_dec, grb_is_grb, &grb_date, grb_errorbox);
}

int
Rts2ConnGrb::pr_integral ()
{
  int grb_id;
  int grb_seqn;
  int grb_type;
  struct ln_equ_posn pos_int, pos_j2000;
  int grb_is_grb = 1;
  time_t grb_date;
  float grb_errorbox;

  if (!do_hete_test
    && ((lbuf[12] & (1 << 31))
    )
  )
  {
    syslog (LOG_DEBUG, "Rts2ConnGrb::pr_integral test packet (%0lxi)", lbuf[12]);
    return 0;
  }

  grb_id = (lbuf[BURST_TRIG] & I_TRIGNUM_MASK) >> I_TRIGNUM_SHIFT;
  grb_seqn = (lbuf[BURST_TRIG] & I_SEQNUM_MASK) >> I_SEQNUM_SHIFT;
  grb_type = lbuf[PKT_TYPE];

  pos_int.ra = lbuf[BURST_RA]/10000.0;
  pos_int.dec = lbuf[BURST_DEC]/10000.0;

  ln_get_equ_prec2 (&pos_int, ln_get_julian_from_sys (), JD2000, &pos_j2000);

  getTimeTfromTJD (lbuf[BURST_TJD], lbuf[BURST_SOD]/100.0, &grb_date);

  grb_errorbox = (float) lbuf[BURST_ERROR]/60.0;

  return addGcnPoint (grb_id, grb_seqn, grb_type, pos_j2000.ra, pos_j2000.dec, grb_is_grb, &grb_date, grb_errorbox);
}

int
Rts2ConnGrb::pr_integral_spicas ()
{
  syslog (LOG_DEBUG, "INTEGRAL SPIACS");
  return 0;
}

int
Rts2ConnGrb::pr_swift_with_radec ()
{
  int grb_id;
  int grb_seqn;
  int grb_type;
  double grb_ra;
  double grb_dec;
  int grb_is_grb = 1;
  time_t grb_date;
  float grb_errorbox;

  grb_type = (int) (lbuf[PKT_TYPE]);
  grb_id = (lbuf[BURST_TRIG] >> S_TRIGNUM_SHIFT) & S_TRIGNUM_MASK;
  grb_seqn = (lbuf[BURST_TRIG] >> S_SEGNUM_SHIFT) & S_SEGNUM_MASK;
  grb_ra = lbuf[BURST_RA] / 10000.0;
  grb_dec = lbuf[BURST_DEC] / 10000.0;

  // we will set grb_is_grb to true in some special cases..
  switch (grb_type)
  {
    case TYPE_SWIFT_BAT_GRB_POS_ACK_SRC:
    case TYPE_SWIFT_BAT_GRB_LC_SRC:
    case TYPE_SWIFT_SCALEDMAP_SRC:
      if ((lbuf[TRIGGER_ID] & 0x00000002)
        && ((lbuf[TRIGGER_ID] & 0x00000020) == 0)
        && !(lbuf[TRIGGER_ID] & 0x00000100))
      {
	grb_is_grb = 1;
      }
      else
      {
	grb_is_grb = 0;
      }
      break;
  }

  getTimeTfromTJD (lbuf[BURST_TJD], lbuf[BURST_SOD]/100.0, &grb_date);
  switch (grb_type)
  {
    case TYPE_SWIFT_BAT_GRB_POS_ACK_SRC:
    case TYPE_SWIFT_XRT_POSITION_SRC:
    case TYPE_SWIFT_UVOT_POS_SRC:
      grb_errorbox = (float) 60 * lbuf[BURST_ERROR] / 10000.0;
    default:
      grb_errorbox = nan ("f");
  }
  return addGcnPoint (grb_id, grb_seqn, grb_type, grb_ra, grb_dec, grb_is_grb, &grb_date, grb_errorbox);
}

int
Rts2ConnGrb::pr_swift_without_radec ()
{
  // those messages have only sence, when they set grb_is_grb flag to false
  EXEC SQL BEGIN DECLARE SECTION;
  int d_grb_id;
  int d_grb_seqn;
  int d_grb_type;
  int d_grb_type_start;
  int d_grb_type_end;
  EXEC SQL END DECLARE SECTION;

  time_t grb_date;

  d_grb_type = (int)(lbuf[PKT_TYPE]);
  d_grb_id = (lbuf[BURST_TRIG] >> S_TRIGNUM_SHIFT) & S_TRIGNUM_MASK;
  d_grb_seqn = (lbuf[BURST_TRIG] >> S_SEGNUM_SHIFT) & S_SEGNUM_MASK;

  switch (d_grb_type)
    {
      case TYPE_SWIFT_BAT_GRB_ALERT_SRC:
        // get S/C coordinates to slew on
        getTimeTfromTJD (lbuf[BURST_TJD], lbuf[BURST_SOD]/100.0, &grb_date);
        // that's special in big errror-box
        // but as we specify last know ra/dec, we will slew to best location we know about burst
        // assume that swift will never spend more then three hours on one location, due to orbit parameters
	// as burst can happen during slew, we have to put in fabs - otherwise we will not respond to burst
	// catched during/before slew, but after pointdir notice was send
        if (fabs (grb_date - swiftLastPoint) < 3 * 3600)
          addGcnPoint (d_grb_id, d_grb_seqn, d_grb_type, swiftLastRa, swiftLastDec, 1, &grb_date, 60);
        break;
      case TYPE_SWIFT_BAT_GRB_POS_NACK_SRC:
        // update if not grb..
        getGrbBound (d_grb_type, d_grb_type_start, d_grb_type_end);
        EXEC SQL
        UPDATE
          grb
        SET
          grb_type = :d_grb_type,
          grb_seqn = :d_grb_seqn,
          grb_is_grb = false
        WHERE
            grb_id = :d_grb_id
          AND grb_type >= :d_grb_type_start
          AND grb_type <= :d_grb_type_end;
        if (sqlca.sqlcode)
        {
          syslog (LOG_DEBUG, "Rts2ConnGrb::pr_swift_without_radec sql errro: %s", sqlca.sqlerrm.sqlerrmc);
          EXEC SQL ROLLBACK;
        }
        else
        {
          syslog (LOG_DEBUG, "Rts2ConnGrb::pr_swift_without_radec grb_is_grb = false grb_id %i", d_grb_id);
          EXEC SQL COMMIT;
        }
        break;
  }

  return addGcnRaw (d_grb_id, d_grb_seqn, d_grb_type);
}

int
Rts2ConnGrb::addSwiftPoint (double roll, char * obs_name, float obstime, float merit)
{
  EXEC SQL BEGIN DECLARE SECTION;
  double d_swift_ra = swiftLastRa;
  double d_swift_dec = swiftLastDec;
  double d_swift_roll = roll;
  int d_swift_time = (int) swiftLastPoint;
  int d_swift_received = (int) last_packet.tv_sec;
  float d_swift_obstime = obstime;
  varchar d_swift_name[70];
  float d_swift_merit = merit;
  EXEC SQL END DECLARE SECTION;

  strcpy (d_swift_name.arr, obs_name);
  d_swift_name.len = strlen (obs_name);

  EXEC SQL
  INSERT INTO
    swift
  (
    swift_id,
    swift_ra,
    swift_dec,
    swift_roll,
    swift_time,
    swift_received,
    swift_name,
    swift_obstime,
    swift_merit
  ) VALUES (
    nextval ('point_id'),
    :d_swift_ra,
    :d_swift_dec,
    :d_swift_roll,
    abstime (:d_swift_time),
    abstime (:d_swift_received),
    :d_swift_name,
    :d_swift_obstime,
    :d_swift_merit
  );
  if (sqlca.sqlcode != 0)
  {
    syslog (LOG_ERR, "Rts2ConnGrb cannot insert swift: %s", sqlca.sqlerrm.sqlerrmc);
    EXEC SQL ROLLBACK;
    return -1;
  }
  EXEC SQL COMMIT;
  return 0;
}

int
Rts2ConnGrb::addIntegralPoint (double ra, double dec, const time_t *t)
{
  EXEC SQL BEGIN DECLARE SECTION;
  double d_integral_ra = ra;
  double d_integral_dec = dec;
  int d_integral_time = (int) *t;
  int d_integral_received = (int) last_packet.tv_sec;
  EXEC SQL END DECLARE SECTION;

  EXEC SQL
  INSERT INTO
    integral
  (
    integral_id,
    integral_ra,
    integral_dec,
    integral_time,
    integral_received
  ) VALUES (
    nextval ('point_id'),
    :d_integral_ra,
    :d_integral_dec,
    abstime (:d_integral_time),
    abstime (:d_integral_received)
  );
  if (sqlca.sqlcode != 0)
  {
    syslog (LOG_ERR, "Rts2ConnGrb cannot insert integral: %s", sqlca.sqlerrm.sqlerrmc);
    EXEC SQL ROLLBACK;
    return -1;
  }
  EXEC SQL COMMIT;
  return 0;
}

void
Rts2ConnGrb::getGrbBound (int grb_type, int &grb_start, int &grb_end)
{
  if (grb_type <= TYPE_SWIFT_BAT_TRANS)
  {
    if (grb_type >= TYPE_SWIFT_BAT_GRB_ALERT_SRC)
    {
      grb_start = TYPE_SWIFT_BAT_GRB_ALERT_SRC;
      grb_end = TYPE_SWIFT_BAT_TRANS;
    }
    else if (grb_type >= TYPE_MILAGRO_POS_SRC)
    {
      grb_start = TYPE_MILAGRO_POS_SRC;
      grb_end = TYPE_KONUS_LC_SRC;
    }
    else if (grb_type >= TYPE_INTEGRAL_POINTDIR_SRC)
    {
      grb_start = TYPE_INTEGRAL_POINTDIR_SRC;
      grb_end = TYPE_INTEGRAL_OFFLINE_SRC;
    }
    else if (grb_type >= TYPE_HETE_ALERT_SRC)
    {
      grb_start = TYPE_HETE_ALERT_SRC;
      grb_end = TYPE_GRB_CNTRPART_SRC;
    }
    else if (grb_type == TYPE_IPN_POS_SRC)
    {
      grb_start = TYPE_IPN_POS_SRC;
      grb_end = TYPE_IPN_POS_SRC;
    }
    else
    {
      // all fields counts - unknow type
      syslog (LOG_DEBUG, "Rts2ConnGrb::getGrbBound cannot get type for grb_type %i", grb_type);
      grb_start = 0;
      grb_end = 5000;
    }
  }
  else
  {
    // all fields counts - unknow type
    syslog (LOG_DEBUG, "Rts2ConnGrb::getGrbBound cannot get type for grb_type %i", grb_type);
    grb_start = 0;
    grb_end = 5000;
  }
}

int
Rts2ConnGrb::addGcnPoint (int grb_id, int grb_seqn, int grb_type, double grb_ra, double grb_dec, bool grb_is_grb, time_t *grb_date, float grb_errorbox)
{
  EXEC SQL BEGIN DECLARE SECTION;
  int d_tar_id;
  int d_grb_id = grb_id;
  int d_grb_seqn = grb_seqn;
  int d_grb_type = grb_type;
  double d_grb_ra = grb_ra;
  double d_grb_dec = grb_dec;
  bool d_grb_is_grb = grb_is_grb;
  long int d_grb_date = (int) *grb_date;
  long int d_grb_update = (int) last_packet.tv_sec;
  float d_grb_errorbox = grb_errorbox;
  int d_grb_errorbox_ind;
  // used to find correct grb - based on type
  int d_grb_type_start;
  int d_grb_type_end;
  // target stuff
  VARCHAR d_tar_name[150];
  VARCHAR d_tar_comment[2000];
  EXEC SQL END DECLARE SECTION;

  struct tm *grb_broken_time;

  int ret = 0;

  if (isnan (d_grb_errorbox))
  {
    d_grb_errorbox_ind = -1;
    d_grb_errorbox = 0;
  }
  else
  {
    d_grb_errorbox_ind = 0;
  }

  getGrbBound (grb_type, d_grb_type_start, d_grb_type_end);

  EXEC SQL
  SELECT
    tar_id
  INTO
    :d_tar_id
  FROM
    grb
  WHERE
      grb_id = :d_grb_id
    AND grb_type >= :d_grb_type_start
    AND grb_type <= :d_grb_type_end;

  if (sqlca.sqlcode == ECPG_NOT_FOUND)
  {
    // insert part..we do care about HETE burst without coordinates
    if (d_grb_ra < -300 && d_grb_dec < -300)
    {
      syslog (LOG_DEBUG, "Rts2ConnGrb::addGcnPoint HETE GRB without coords? %f %f", d_grb_ra, d_grb_dec);
      EXEC SQL ROLLBACK;
      return -1;
    }
    // generate new GRB details
    grb_broken_time = gmtime (grb_date);
    d_tar_name.len = sprintf (d_tar_name.arr, "GRB %02d%02d%06.3f GCN #%i", 
      grb_broken_time->tm_year % 100, grb_broken_time->tm_mon + 1, grb_broken_time->tm_mday +
      (grb_broken_time->tm_hour * 3600 + grb_broken_time->tm_min * 60 + grb_broken_time->tm_sec) / 86400.0,
      d_grb_id);
    d_tar_comment.len = sprintf (d_tar_comment.arr, "Generated by GRBD for event %d-%02d-%02dT%02d:%02d:%02d, GCN #%i, type %i",
      grb_broken_time->tm_year + 1900, grb_broken_time->tm_mon + 1, grb_broken_time->tm_mday,
      grb_broken_time->tm_hour, grb_broken_time->tm_min, grb_broken_time->tm_sec, d_grb_id, d_grb_type);
    EXEC SQL
    SELECT
      nextval ('grb_tar_id')
    INTO
      :d_tar_id;

    // insert new target
    EXEC SQL
    INSERT INTO
      targets
    (
      tar_id,
      type_id,
      tar_name,
      tar_ra,
      tar_dec,
      tar_enabled,
      tar_comment,
      tar_priority,
      tar_bonus,
      tar_bonus_time
    ) VALUES (
      :d_tar_id,
      'G',
      :d_tar_name,
      :d_grb_ra,
      :d_grb_dec,
      true,
      :d_tar_comment,
      100,
      100,
      NULL
    );
    if (sqlca.sqlcode)
    {
      syslog (LOG_ERR, "Rts2ConnGrb::addGcnPoint insert target error: %li %s", sqlca.sqlcode, sqlca.sqlerrm.sqlerrmc);
      EXEC SQL ROLLBACK;
    }
    // insert new grb packet
    EXEC SQL
    INSERT INTO
      grb
    (
      tar_id,
      grb_id,
      grb_seqn,
      grb_type,
      grb_ra,
      grb_dec,
      grb_is_grb,
      grb_date,
      grb_last_update,
      grb_errorbox
    ) VALUES (
      :d_tar_id,
      :d_grb_id,
      :d_grb_seqn,
      :d_grb_type,
      :d_grb_ra,
      :d_grb_dec,
      :d_grb_is_grb,
      abstime (:d_grb_date),
      abstime (:d_grb_update),
      :d_grb_errorbox :d_grb_errorbox_ind
    );
    if (sqlca.sqlcode)
    {
      syslog (LOG_ERR, "Rts2ConnGrb::addGcnPoint cannot insert new GCN : %li %s", sqlca.sqlcode, sqlca.sqlerrm.sqlerrmc);
      EXEC SQL ROLLBACK;
      ret = -1;
    }
    else
    {
      syslog (LOG_DEBUG, "Rts2ConnGrb::addGcnPoint grb created: tar_id: %i grb_id: %i grb_seqn: %i", d_tar_id, d_grb_id, d_grb_seqn);
      EXEC SQL COMMIT;
    }
  }
  else
  {
    // HETE burst have values -999 in some retraction notices..
    if (d_grb_ra > -300 && d_grb_dec > -300)
      {
	// update target informations..
	EXEC SQL
	UPDATE
	  targets
	SET
	  tar_ra = :d_grb_ra,
	  tar_dec = :d_grb_dec
	WHERE
	  tar_id = :d_tar_id;
	if (sqlca.sqlcode)
	{
	  syslog (LOG_ERR, "Rts2ConnGrb::addGcnPoint cannot update targets: %li %s", sqlca.sqlcode, sqlca.sqlerrm.sqlerrmc);
	  EXEC SQL ROLLBACK;
	}
      }
    // update grb informations..
    EXEC SQL
    UPDATE
      grb
    SET
      grb_seqn = :d_grb_seqn,
      grb_type = :d_grb_type,
      grb_ra = :d_grb_ra,
      grb_dec = :d_grb_dec,
      grb_is_grb = :d_grb_is_grb,
      grb_date = abstime (:d_grb_date),
      grb_last_update = abstime (:d_grb_update)
    WHERE
      tar_id = :d_tar_id;

    if (sqlca.sqlcode)
    {
      syslog (LOG_ERR, "Rts2ConnGrb::addGcnPoint cannot update GCN : %li %s", sqlca.sqlcode, sqlca.sqlerrm.sqlerrmc);
      EXEC SQL ROLLBACK;
      ret = -1;
    }
    else
    {
      EXEC SQL COMMIT;
    }

    if (d_grb_errorbox_ind == 0)
    {
      EXEC SQL
      UPDATE
        grb
      SET
        grb_errorbox = :d_grb_errorbox
      WHERE
        tar_id = :d_tar_id;
    }
    if (sqlca.sqlcode)
    {
      syslog (LOG_ERR, "Rts2ConnGrb::addGcnPoint cannot update GCN errorbox: %li %s", sqlca.sqlcode, sqlca.sqlerrm.sqlerrmc);
      EXEC SQL ROLLBACK;
      ret = -1;
    }
    else
    {
      syslog (LOG_DEBUG, "Rts2ConnGrb::addGcnPoint grb updated: tar_id: %i grb_id: %i grb_seqn: %i", d_tar_id, d_grb_id, d_grb_seqn);
      EXEC SQL COMMIT;
    }
  }

  addGcnRaw (grb_id, grb_seqn, grb_type);

  return master->newGcnGrb (d_tar_id);
}

int
Rts2ConnGrb::addGcnRaw (int grb_id, int grb_seqn, int grb_type)
{
  EXEC SQL BEGIN DECLARE SECTION;
  int d_grb_id = grb_id;
  int d_grb_seqn = grb_seqn;
  int d_grb_type = grb_type;
  long int d_grb_update = (int) last_packet.tv_sec;
  int d_grb_update_usec = (int) last_packet.tv_usec;
  long d_packet[40];
  EXEC SQL END DECLARE SECTION;

  // insert raw packet..
  memcpy (d_packet, lbuf, 40 * sizeof (long));

  EXEC SQL
  INSERT INTO
    grb_gcn
  (
    grb_id,
    grb_seqn,
    grb_type,
    grb_update,
    grb_update_usec,
    packet
  ) VALUES (
    :d_grb_id,
    :d_grb_seqn,
    :d_grb_type,
    abstime (:d_grb_update),
    :d_grb_update_usec,
    :d_packet
  );
  if (sqlca.sqlcode)
  {
    syslog (LOG_ERR, "Rts2ConnGrb::addGcnRaw cannot insert new gcn packet: %li %s", sqlca.sqlcode, sqlca.sqlerrm.sqlerrmc);
    EXEC SQL ROLLBACK;
    return -1;
  }
  else
  {
    EXEC SQL COMMIT;
    return 0;
  }
}

Rts2ConnGrb::Rts2ConnGrb (char *in_gcn_hostname, int in_gcn_port, int
in_do_hete_test, Rts2DevGrb *in_master):Rts2ConnNoSend (in_master)
{
  master = in_master;
  gcn_hostname = new char[strlen (in_gcn_hostname) + 1];
  strcpy (gcn_hostname, in_gcn_hostname);
  gcn_port = in_gcn_port;
  gcn_listen_sock = -1;

  time (&last_packet.tv_sec);
  last_packet.tv_sec -= 600;
  last_packet.tv_usec = 0;
  last_imalive_sod = -1;

  deltaValue = 0;
  last_target = NULL;
  last_target_time = -1;

  do_hete_test = in_do_hete_test;

  setConnTimeout (90);
  time (&nextTime);
  nextTime += getConnTimeout ();

  swiftLastPoint = 0;
  swiftLastRa = nan ("f");
  swiftLastDec = nan ("f");
}

Rts2ConnGrb::~Rts2ConnGrb (void)
{
  delete gcn_hostname;
  if (last_target)
    delete last_target;
  if (gcn_listen_sock >= 0)
    close (gcn_listen_sock);
}

int
Rts2ConnGrb::idle ()
{
  int ret;
  int err;
  socklen_t len = sizeof (err);

  time_t now;
  time (&now);

  switch (getConnState ())
    {
    case CONN_CONNECTING:
      ret = getsockopt (sock, SOL_SOCKET, SO_ERROR, &err, &len);
      if (ret)
        {
          syslog (LOG_ERR, "Rts2ConnGrb::idle getsockopt %m");
	  connectionError (-1);
        }
      else if (err)
        {
          syslog (LOG_ERR, "Rts2ConnGrb::idle getsockopt %s",
                  strerror (err));
	  connectionError (-1);
        }
      else 
        {
          setConnState (CONN_CONNECTED);
	}
      // kill us when we were in conn_connecting state for to long
    case CONN_BROKEN:
      if (nextTime < now)
        {
          ret = init ();
          if (ret)
            {
              time (&nextTime);
	      nextTime += getConnTimeout ();
            }
	}
      break;
    case CONN_CONNECTED:
      if (last_packet.tv_sec + getConnTimeout () < now
        && nextTime < now)
        connectionError (-1);
      break;
    default:
      break;
    }
  // we don't like to get called upper code with timeouting stuff..
  return 0;
}

int
Rts2ConnGrb::init_call ()
{
  char *s_port;
  struct addrinfo hints;
  struct addrinfo *info;
  int ret;

  hints.ai_flags = 0;
  hints.ai_family = PF_INET;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_protocol = 0;
  asprintf (&s_port, "%i", gcn_port);
  ret = getaddrinfo (gcn_hostname, s_port, &hints, &info);
  free (s_port);
  if (ret)
    {
      syslog (LOG_ERR, "Rts2Address::getAddress getaddrinfor: %s",
              gai_strerror (ret));
      freeaddrinfo (info);
      return -1;
    }
  sock = socket (info->ai_family, info->ai_socktype, info->ai_protocol);
  if (sock == -1)
  {
    freeaddrinfo (info);
    return -1;
  }
  ret = connect (sock, info->ai_addr, info->ai_addrlen);
  freeaddrinfo (info);
  time (&nextTime);
  nextTime += getConnTimeout ();
  if (ret == -1)
    {
      if (errno == EINPROGRESS)
        {
	  setConnState (CONN_CONNECTING);
          return 0;
        }
      return -1;
    }
  setConnState (CONN_CONNECTED);
  return 0;
}

int
Rts2ConnGrb::init_listen ()
{
  int ret;

  if (gcn_listen_sock >= 0)
  {
    close (gcn_listen_sock);
    gcn_listen_sock = -1;
  }

  connectionError (-1);

  gcn_listen_sock = socket (PF_INET, SOCK_STREAM, 0);
  if (gcn_listen_sock == -1)
    {
      syslog (LOG_ERR, "Rts2ConnGrb::init_listen socket %m");
      return -1;
    }
  const int so_reuseaddr = 1;
  setsockopt (gcn_listen_sock, SOL_SOCKET, SO_REUSEADDR, &so_reuseaddr, sizeof (so_reuseaddr));
  struct sockaddr_in server;
  server.sin_family = AF_INET;
  server.sin_port = htons (gcn_port);
  server.sin_addr.s_addr = htonl (INADDR_ANY);
  ret = bind (gcn_listen_sock, (struct sockaddr *) &server, sizeof (server));
  if (ret)
    {
      syslog (LOG_ERR, "Rts2ConnGrb::init_listen bind: %m");
      return -1;
    }
  ret = listen (gcn_listen_sock, 1);
  if (ret)
    {
      syslog (LOG_ERR, "Rts2ConnGrb::init_listen listen: %m");
      return -1;
    }
  setConnState (CONN_CONNECTED);
  time (&nextTime);
  nextTime += 2 * getConnTimeout ();
  return 0;
}

int
Rts2ConnGrb::init ()
{
  if (!strcmp (gcn_hostname, "-"))
    return init_listen ();
  else
    return init_call ();
}

int
Rts2ConnGrb::add (fd_set * set)
{
  if (gcn_listen_sock >= 0)
  {
    FD_SET (gcn_listen_sock, set);
    return 0;
  }
  return Rts2Conn::add (set);
}

int
Rts2ConnGrb::connectionError (int last_data_size)
{
  syslog (LOG_DEBUG, "Rts2ConnGrb::connectionError");
  if (sock > 0)
  {
    close (sock);
    sock = -1;
  }
  if (!isConnState (CONN_BROKEN))
  {
    time (&nextTime);
    sock = -1;
    setConnState (CONN_BROKEN);
  }
  return -1;
}

int
Rts2ConnGrb::receive (fd_set *set)
{
  int ret = 0;
  struct tm *t;
  if (gcn_listen_sock >= 0 && FD_ISSET (gcn_listen_sock, set))
  {
    // try to accept connection..
    close (sock); // close previous connections..we support only one GCN connection
    sock = -1;
    struct sockaddr_in other_side;
    socklen_t addr_size = sizeof (struct sockaddr_in);
    sock = accept (gcn_listen_sock, (struct sockaddr *) &other_side, &addr_size);
    if (sock == -1)
    {
      // bad accept - strange
      syslog (LOG_ERR, "Rts2ConnGrb::receive accept on gcn_listen_sock: %m");
      connectionError (-1);
    }
    // close listening socket..when we get connection
    close (gcn_listen_sock);
    gcn_listen_sock = -1;
    setConnState (CONN_CONNECTED);
    syslog (LOG_DEBUG, "Rts2ConnGrb::receive accept gcn_listen_sock from %s %i",
      inet_ntoa (other_side.sin_addr), ntohs (other_side.sin_port));
  }
  else if (sock >= 0 && FD_ISSET (sock, set))
  {
    // translate packages to linux..
    short *sp;			// Ptr to a short; used for the swapping
    short pl, ph;			// Low part & high part
    ret = read (sock, (char*) nbuf, sizeof (nbuf));
    if (ret == 0 && isConnState (CONN_CONNECTING))
    {
      setConnState (CONN_CONNECTED);
    }
    else if (ret <= 0)
    {
      connectionError (ret);
      return -1;
    }
    successfullRead ();
    gettimeofday (&last_packet, NULL);
    /* Immediately echo back the packet so GCN can monitor:
     * (1) the actual receipt by the site, and
     * (2) the roundtrip travel times.
     * Everything except KILL's get echo-ed back.            */
    if(nbuf[PKT_TYPE] != TYPE_KILL_SOCKET)
    {
      write (sock, (char *)nbuf, sizeof(nbuf)); 
      successfullSend ();
    }
    swab (nbuf, lbuf, SIZ_PKT * sizeof (lbuf[0]));
    sp = (short *)lbuf;
    for(int i=0; i<SIZ_PKT; i++)
      {
        pl = sp[2*i];
        ph = sp[2*i + 1];
        sp[2*i] = ph;
	sp[2*i + 1] = pl;
      }
    t = gmtime (&last_packet.tv_sec);
    here_sod = t->tm_hour*3600 + t->tm_min*60 + t->tm_sec + last_packet.tv_usec / USEC_SEC;

    // switch based on packet content
    switch (lbuf[PKT_TYPE])
    {
      case TYPE_IM_ALIVE:
        pr_imalive ();
        break;
      // pondtirs messages with history..
      case TYPE_INTEGRAL_POINTDIR_SRC:
        pr_integral_point ();
	break;
      case TYPE_SWIFT_POINTDIR_SRC:         // 83  // Swift Pointing Direction
	pr_swift_point();
	break;
      // hete, integral & swift GRB observations
      case TYPE_HETE_ALERT_SRC:
      case TYPE_HETE_UPDATE_SRC:
      case TYPE_HETE_FINAL_SRC:
      case TYPE_HETE_GNDANA_SRC:
      case TYPE_HETE_TEST:
      case TYPE_GRB_CNTRPART_SRC:
         pr_hete ();
	 break;
      case TYPE_INTEGRAL_WAKEUP_SRC:
      case TYPE_INTEGRAL_REFINED_SRC:
      case TYPE_INTEGRAL_OFFLINE_SRC:
         pr_integral ();
	 break;
      // integral spiacs
      case TYPE_INTEGRAL_SPIACS_SRC:
         pr_integral_spicas ();
	 break;
      case TYPE_SWIFT_BAT_GRB_POS_ACK_SRC:
      case TYPE_SWIFT_BAT_GRB_LC_SRC:
      case TYPE_SWIFT_FOM_2OBSAT_SRC:
      case TYPE_SWIFT_FOSC_2OBSAT_SRC:
      case TYPE_SWIFT_XRT_POSITION_SRC:
      case TYPE_SWIFT_XRT_SPECTRUM_SRC:
      case TYPE_SWIFT_XRT_IMAGE_SRC:
      case TYPE_SWIFT_XRT_LC_SRC:
      case TYPE_SWIFT_UVOT_FCHART_SRC:
      // processed messages
      case TYPE_SWIFT_BAT_GRB_LC_PROC_SRC:
      case TYPE_SWIFT_XRT_SPECTRUM_PROC_SRC:
      case TYPE_SWIFT_XRT_IMAGE_PROC_SRC:
      case TYPE_SWIFT_UVOT_FCHART_PROC_SRC:
      case TYPE_SWIFT_UVOT_POS_SRC:
      // transient
      case TYPE_SWIFT_BAT_TRANS:
        pr_swift_with_radec ();
	break;
      case TYPE_SWIFT_BAT_GRB_ALERT_SRC:
      case TYPE_SWIFT_BAT_GRB_POS_NACK_SRC:
      case TYPE_SWIFT_SCALEDMAP_SRC:
      case TYPE_SWIFT_XRT_CENTROID_SRC:
      case TYPE_SWIFT_UVOT_DBURST_SRC:
      // processed messages
      case TYPE_SWIFT_UVOT_DBURST_PROC_SRC:
        pr_swift_without_radec ();
	break;
      case TYPE_KILL_SOCKET:
        connectionError (-1);
        break;
      default:
        syslog (LOG_ERR, "Rts2ConnGrb::receive unknow packet type: %ld", lbuf[PKT_TYPE]);
	break;
    }
    // enable others to catch-up (FW connections will forward packet to their sockets)
    getMaster ()->postEvent (new Rts2Event (RTS2_EVENT_GRB_PACKET, nbuf));
  }
  return ret;
}

int
Rts2ConnGrb::lastPacket ()
{
  return last_packet.tv_sec;
}

double
Rts2ConnGrb::delta ()
{
  return deltaValue;
}

char*
Rts2ConnGrb::lastTarget ()
{
  return last_target;
}

void
Rts2ConnGrb::setLastTarget (char *in_last_target)
{
  if (last_target)
    delete last_target;
  last_target = new char[strlen (in_last_target) + 1];
  strcpy (last_target, in_last_target);
}

int
Rts2ConnGrb::lastTargetTime ()
{
  return last_target_time;
}
