#include "rts2devcliphot.h"

EXEC SQL include sqlca;

Rts2DevClientPhotExec::Rts2DevClientPhotExec (Rts2Conn * in_connection):Rts2DevClientPhot (in_connection),
  Rts2DevScript
  (in_connection)
{
  minFlux = 20;
}

Rts2DevClientPhotExec::~Rts2DevClientPhotExec ()
{
  deleteScript ();
}

void
Rts2DevClientPhotExec::integrationEnd ()
{
  nextCommand ();
  Rts2DevClientPhot::integrationEnd ();
}

void
Rts2DevClientPhotExec::integrationFailed (int status)
{
  nextCommand ();
  Rts2DevClientPhot::integrationFailed (status);
}

void
Rts2DevClientPhotExec::addCount (int count, float exp, int is_ov)
{
  EXEC SQL BEGIN DECLARE SECTION;
  int d_obs_id;
  int d_count_value;
  int d_count_date;
  int d_count_usec;
  float d_count_exposure;
  char d_count_filter;
  double d_count_ra;
  double d_count_dec;
  VARCHAR d_counter_name[8];
  EXEC SQL END DECLARE SECTION;
  if ((connection->getState (0) & PHOT_MASK_INTEGRATE) != PHOT_INTEGRATE)
    {
      // we are in searching mode..
      if (waitScript == WAIT_SEARCH)
	{
	  syslog (LOG_DEBUG,
		  "Rts2DevClientPhotExec::addCount WAIT_SEARCH %i %f %i",
		  count, exp, is_ov);
	  if (is_ov || (count / exp) > minFlux)
	    searchSucess ();
	}
      return;
    }

  if (!currentTarget)
    return;

  struct ln_equ_posn actRaDec;
  struct timeval now;

  if (is_ov)
    {
      syslog (LOG_DEBUG, "Rts2DevClientPhotExec::addCount is_ov");
      return;
    }

  gettimeofday (&now, NULL);

  actRaDec.ra = -1000;
  actRaDec.dec = -1000;

  getMaster ()->
    postEvent (new Rts2Event (EVENT_GET_RADEC, (void *) &actRaDec));

  d_obs_id = currentTarget->getObsId ();
  d_count_date = now.tv_sec;
  d_count_usec = now.tv_usec;
  d_count_value = count;
  d_count_exposure = exp;
  d_count_filter = getValueInteger ("filter") + '0';
  d_count_ra = actRaDec.ra;
  d_count_dec = actRaDec.dec;

  strncpy (d_counter_name.arr, connection->getName (), 8);
  d_counter_name.len = strlen (connection->getName ());
  if (d_counter_name.len > 8)
    d_counter_name.len = 8;

  EXEC SQL
  INSERT INTO
    counts 
  (
    obs_id,
    count_date,
    count_usec,
    count_value,
    count_exposure,
    count_filter,
    count_ra,
    count_dec,
    counter_name
  )
  VALUES
  (
    :d_obs_id,
    abstime (:d_count_date),
    :d_count_usec,
    :d_count_value,
    :d_count_exposure,
    :d_count_filter,
    :d_count_ra,
    :d_count_dec,
    :d_counter_name
  );

  if (sqlca.sqlcode != 0)
    {
      syslog (LOG_ERR, "Rts2DevClientPhotExec::addCount db error %s",
	      sqlca.sqlerrm.sqlerrmc);
      EXEC SQL ROLLBACK;
    }
  else
    {
      EXEC SQL COMMIT;
    }
}

int
Rts2DevClientPhotExec::getNextCommand ()
{
  return script->nextCommand (this, &nextComd, cmd_device);
}

void
Rts2DevClientPhotExec::postEvent (Rts2Event * event)
{
  Rts2DevScript::postEvent (new Rts2Event (event));
  Rts2DevClientPhot::postEvent (event);
}

void
Rts2DevClientPhotExec::nextCommand ()
{
  int ret;
  ret = haveNextCommand ();
  if (!ret)
    return;

  connection->queCommand (nextComd);
  nextComd = NULL;		// after command execute, it will be deleted
  blockMove = 1;		// as we run a script..
  if (currentTarget)
    currentTarget->startObservation ();
}

void
Rts2DevClientPhotExec::filterOK ()
{
  nextCommand ();
}

void
Rts2DevClientPhotExec::filterFailed (int status)
{
  deleteScript ();
}
