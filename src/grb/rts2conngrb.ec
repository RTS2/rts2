#include "rts2conngrb.h"

#include <errno.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <unistd.h>

EXEC SQL include sqlca;

double
Rts2ConnGrb::getPktSod ()
{
  return lbuf[PKT_SOD]/100.0;
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
  double ra;
  double dec;
  double roll;
  time_t t;
  char *name;
  float obstime;
  float merit;
  ra  = lbuf[7]/10000.0;
  dec = lbuf[8]/10000.0;
  roll = lbuf[9]/10000.0;
  ln_get_timet_from_julian (getJDfromTJD (lbuf[5], lbuf[6]/100.0), &t);
  name = (char*) (&lbuf[BURST_URL]);
  obstime = lbuf[14]/100.0;
  merit = lbuf[15]/100.0;
  return addSwiftPoint (ra, dec, roll, &t, name, obstime, merit);
}

int
Rts2ConnGrb::pr_integral_point ()
{
  double ra;
  double dec;
  time_t t;
  ra = lbuf[14]/10000.0;
  dec = lbuf[14]/10000.0;
  ln_get_timet_from_julian (getJDfromTJD (lbuf[5], lbuf[6]/100.0), &t);
  return addIntegralPoint (ra, dec, &t);
}

int
Rts2ConnGrb::addSwiftPoint (double ra, double dec, double roll, const time_t *t, 
  char * name, float obstime, float merit)
{
  EXEC SQL BEGIN DECLARE SECTION;
  double d_swift_ra = ra;
  double d_swift_dec = dec;
  double d_swift_roll = roll;
  int d_swift_time = (int) *t;
  int d_swift_received = (int) last_packet.tv_sec;
  float d_swift_obstime = obstime;
  varchar d_swift_name[70];
  float d_swift_merit = merit;
  EXEC SQL END DECLARE SECTION;

  strcpy (d_swift_name.arr, name);
  d_swift_name.len = strlen (name);

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
    syslog (LOG_ERR, "Rts2ConnGrb cannot insert integral: %s", sqlca.sqlerrm.sqlerrmc);
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
    syslog (LOG_ERR, "Rts2ConnGrb cannot insert swift: %s", sqlca.sqlerrm.sqlerrmc);
    EXEC SQL ROLLBACK;
    return -1;
  }
  EXEC SQL COMMIT;
  return 0;
}

Rts2ConnGrb::Rts2ConnGrb (char *in_gcn_hostname, int in_gcn_port, Rts2Device *in_master):Rts2Conn (in_master)
{
  gcn_hostname = new char[strlen (in_gcn_hostname) + 1];
  strcpy (gcn_hostname, in_gcn_hostname);
  gcn_port = in_gcn_port;
  last_packet.tv_sec = 0;
  last_packet.tv_usec = 0;
  last_imalive_sod = -1;

  deltaValue = 0;
  last_target = NULL;
  last_target_time = -1;
}

Rts2ConnGrb::~Rts2ConnGrb (void)
{
  delete gcn_hostname;
  if (last_target)
    delete last_target;
}

int
Rts2ConnGrb::idle ()
{
  switch (conn_state)
    {
    case CONN_CONNECTING:
      int err;
      int ret;
      socklen_t len = sizeof (err);

      ret = getsockopt (sock, SOL_SOCKET, SO_ERROR, &err, &len);
      if (ret)
        {
          syslog (LOG_ERR, "Rts2ConnGrb::idle getsockopt %m");
          conn_state = CONN_DELETE;
          break;
        }
      if (err)
        {
          syslog (LOG_ERR, "Rts2ConnGrb::idle getsockopt %s",
                  strerror (err));
          conn_state = CONN_DELETE;
          break;
        }
      break;
    }
  return Rts2Conn::idle ();
}

int
Rts2ConnGrb::init ()
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
    return -1;
    freeaddrinfo (info);
  }
  ret = connect (sock, info->ai_addr, info->ai_addrlen);
  freeaddrinfo (info);
  if (ret == -1)
    {
     
      if (errno = EINPROGRESS)
        {
          conn_state = CONN_CONNECTING;
          return 0;
        }
      return -1;
    }
  return 0;
}

int
Rts2ConnGrb::receive (fd_set *set)
{
  int ret = 0;
  struct tm *t;
  if (conn_state == CONN_DELETE)
    return -1;
  if (sock >= 0 && FD_ISSET (sock, set))
  {
    // translate packages to linux..
    short *sp;			// Ptr to a short; used for the swapping
    short pl, ph;			// Low part & high part
    ret = read (sock, (char*) nbuf, sizeof (nbuf));
    gettimeofday (&last_packet, NULL);
    if (ret < 0)
    {
      conn_state = CONN_DELETE;
      return -1;
    }
    /* Immediately echo back the packet so GCN can monitor:
     * (1) the actual receipt by the site, and
     * (2) the roundtrip travel times.
     * Everything except KILL's get echo-ed back.            */
    if(nbuf[PKT_TYPE] != TYPE_KILL_SOCKET)
      write (sock, (char *)nbuf, sizeof(nbuf)); 
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
      case TYPE_INTEGRAL_POINTDIR_SRC:
        pr_integral_point ();
	break;
      case TYPE_SWIFT_POINTDIR_SRC:         // 83  // Swift Pointing Direction
	pr_swift_point();
	break;
      case TYPE_KILL_SOCKET:
        shutdown (sock, SHUT_RDWR);
	close (sock);
	conn_state = CONN_CONNECTING;
	do
	{
          ret = init ();
	} while (ret != 0);
        break;
      default:
        syslog (LOG_ERR, "Rts2ConnGrb::receive unknow packet type: %d", lbuf[PKT_TYPE]);
	break;
    }
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
