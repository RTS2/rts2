#include "../grbc.h"
#include "ibas_client.h"

#include <netdb.h>
#include <time.h>

void *
receive_iban (process_grb_event_t arg)
{
  int ibas_ip, ibas_port, my_port, r, pingseq, ping_signalled;
  char **p;
  struct hostent *hp;
  struct in_addr in;
  IBC_DL dl;

  if (NULL != (hp = gethostbyname ("129.194.67.222")))
    {
      p = hp->h_addr_list;
      if (NULL != p)
	if (0 != *p)
	  {
	    memcpy (&in.s_addr, *p, sizeof (in.s_addr));
	    ibas_ip = ntohl (in.s_addr);
	  }
    }

  ibas_port = 1966;
  my_port = 1944;

  if (IBC_OK !=
      (r = ibc_api_init (ibas_ip, ibas_port, my_port, &ping_signalled)))
    {
      printf ("ibc_api_init() failed, error code = %d\n", r);
      exit (r);
    }

  pingseq = 0;

  for (;;)
    {
      r = ibc_api_listen (&dl);
      if (IBC_SIGNALLED == r)
	{
	  r = ibc_api_send_ping (pingseq);
	  if (IBC_OK != r)
	    {
	      printf
		("WARNING: ibc_api_send_ping() failed, error code = %d\n", r);
	    }
	  else
	    {
	      printf ("PING_QUERY: packet sent out: seqnum = %d\n", pingseq);
	    }
	  pingseq++;
	  continue;
	}
      if (IBC_AGAIN == r)	/* only possible in NONBLOCK mode */
	{
	  ibc_sleep_msec (100, NULL);
	  continue;
	}
      if (IBC_OK != r)
	break;			/* ctrl-C breaks out of this loop */

      if (IBAS_ALERT_TEST_FLAG_R_PING & dl.a.test_flag)
	{
	  ibc_api_dump_ping_reply (&dl, ibas_ip, ibas_port);
	}
      else
	{
	  struct tm tm_time;
	  struct IBC_UTC_TIME_STRUCT ibc_time;
	  time_t date;

	  ibc_str242utc (dl.a.grb_time, &ibc_time);

	  tm_time.tm_sec = ibc_time.sec;	/* seconds */
	  tm_time.tm_min = ibc_time.min;	/* minutes */
	  tm_time.tm_hour = ibc_time.hour;	/* hours */
	  tm_time.tm_mday = ibc_time.day;	/* day of the month */
	  tm_time.tm_mon = ibc_time.month;	/* month */
	  tm_time.tm_year = ibc_time.year;	/* year */
	  tm_time.tm_wday = 0;	/* day of the week */
	  tm_time.tm_yday = 0;	/* day in the year */
	  tm_time.tm_isdst = 0;	/* daylight saving time */

	  date = mktime (&tm_time);

	  (process_grb_event_t *) arg (dl.ID, dl.seqnum, dl.a.grb_ra,
				       dl.a.grb_dec, &date);
	  ibc_api_dump_alert (&dl, ibas_ip, ibas_port);
	}
    }
  ibc_api_shutdown ();
  return NULL;
}
