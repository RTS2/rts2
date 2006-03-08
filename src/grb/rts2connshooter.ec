#include "rts2connshooter.h"

#include <errno.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <unistd.h>
#include <sstream>

#include <libnova/libnova.h>

EXEC SQL include sqlca;

void
Rts2ConnShooter::getTimeTfromGPS (long GPSsec, long GPSusec, double & out_time)
{
  out_time = GPSsec + 100000 + GPSusec / USEC_SEC;
}

// is called when nbuf contains '\n'
int
Rts2ConnShooter::processAuger ()
{
  EXEC SQL BEGIN DECLARE SECTION;
  int db_auger_t3id;
  double db_auger_date;
  int db_auger_npixels;
  double db_auger_sdpphi;
  double db_auger_sdptheta;
  double db_auger_sdpangle;
  EXEC SQL END DECLARE SECTION;

  long gps_sec;
  long gps_usec;

  std::istringstream _is (nbuf);
  _is >> db_auger_t3id >> gps_sec >> gps_usec >> db_auger_npixels >> db_auger_sdpphi >> db_auger_sdptheta >> db_auger_sdpangle;
  if (_is.fail ())
    return -1;

  getTimeTfromGPS (gps_sec, gps_usec, db_auger_date);

  EXEC SQL INSERT INTO
    auger
  (
    auger_t3id,
    auger_date,
    auger_npixels,
    auger_sdpphi,
    auger_sdptheta,
    auger_sdpangle
  )
  VALUES
  (
    :db_auger_t3id,
    abstime (:db_auger_date),
    :db_auger_npixels,
    :db_auger_sdpphi,
    :db_auger_sdptheta,
    :db_auger_sdpangle
  );
  if (sqlca.sqlcode)
  {
    syslog (LOG_ERR, "Rts2ConnShooter::processAuger cannot add new value to db: %s (%li)", sqlca.sqlerrm.sqlerrmc, sqlca.sqlcode);
    EXEC SQL ROLLBACK;
    return -1;
  }
  EXEC SQL COMMIT;
  return 0;
}

Rts2ConnShooter::Rts2ConnShooter (int in_port, 
Rts2DevAugerShooter *in_master):Rts2ConnNoSend (in_master)
{
  master = in_master;
  port = in_port;
  auger_listen_socket = -1;

  time (&last_packet.tv_sec);
  last_packet.tv_sec -= 600;
  last_packet.tv_usec = 0;

  last_target_time = -1;

  setConnTimeout (-1);
}

Rts2ConnShooter::~Rts2ConnShooter (void)
{
  if (auger_listen_socket >= 0)
    close (auger_listen_socket);
}

int
Rts2ConnShooter::idle ()
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
          syslog (LOG_ERR, "Rts2ConnShooter::idle getsockopt %m");
	  connectionError (-1);
        }
      else if (err)
        {
          syslog (LOG_ERR, "Rts2ConnShooter::idle getsockopt %s",
                  strerror (err));
	  connectionError (-1);
        }
      else 
        {
          setConnState (CONN_CONNECTED);
	}
      break;
    case CONN_CONNECTED:
      // mayby handle connection error?
      break;
    default:
      break;
    }
  // we don't like to get called upper code with timeouting stuff..
  return 0;
}

int
Rts2ConnShooter::init_listen ()
{
  int ret;

  if (auger_listen_socket >= 0)
  {
    close (auger_listen_socket);
    auger_listen_socket = -1;
  }

  connectionError (-1);

  auger_listen_socket = socket (PF_INET, SOCK_STREAM, 0);
  if (auger_listen_socket == -1)
    {
      syslog (LOG_ERR, "Rts2ConnShooter::init_listen socket %m");
      return -1;
    }
  const int so_reuseaddr = 1;
  setsockopt (auger_listen_socket, SOL_SOCKET, SO_REUSEADDR, &so_reuseaddr, sizeof (so_reuseaddr));
  struct sockaddr_in server;
  server.sin_family = AF_INET;
  server.sin_port = htons (port);
  server.sin_addr.s_addr = htonl (INADDR_ANY);
  ret = bind (auger_listen_socket, (struct sockaddr *) &server, sizeof (server));
  if (ret)
    {
      syslog (LOG_ERR, "Rts2ConnShooter::init_listen bind: %m");
      return -1;
    }
  ret = listen (auger_listen_socket, 1);
  if (ret)
    {
      syslog (LOG_ERR, "Rts2ConnShooter::init_listen listen: %m");
      return -1;
    }
  setConnState (CONN_CONNECTED);
  return 0;
}

int
Rts2ConnShooter::init ()
{
  return init_listen ();
}

int
Rts2ConnShooter::add (fd_set * set)
{
  if (auger_listen_socket >= 0)
  {
    FD_SET (auger_listen_socket, set);
    return 0;
  }
  return Rts2Conn::add (set);
}

int
Rts2ConnShooter::connectionError (int last_data_size)
{
  syslog (LOG_DEBUG, "Rts2ConnShooter::connectionError");
  if (sock > 0)
  {
    close (sock);
    sock = -1;
  }
  if (!isConnState (CONN_BROKEN))
  {
    sock = -1;
    setConnState (CONN_BROKEN);
  }
  return -1;
}

int
Rts2ConnShooter::receive (fd_set *set)
{
  int ret = 0;
  if (auger_listen_socket >= 0 && FD_ISSET (auger_listen_socket, set))
  {
    // try to accept connection..
    close (sock); // close previous connections..we support only one GCN connection
    sock = -1;
    struct sockaddr_in other_side;
    socklen_t addr_size = sizeof (struct sockaddr_in);
    sock = accept (auger_listen_socket, (struct sockaddr *) &other_side, &addr_size);
    if (sock == -1)
    {
      // bad accept - strange
      syslog (LOG_ERR, "Rts2ConnShooter::receive accept on auger_listen_socket: %m");
      connectionError (-1);
    }
    // close listening socket..when we get connection
    close (auger_listen_socket);
    auger_listen_socket = -1;
    setConnState (CONN_CONNECTED);
    syslog (LOG_DEBUG, "Rts2ConnShooter::receive accept auger_listen_socket from %s %i",
      inet_ntoa (other_side.sin_addr), ntohs (other_side.sin_port));
  }
  else if (sock >= 0 && FD_ISSET (sock, set))
  {
    ret = read (sock, nbuf, sizeof (nbuf));
    if (ret == 0 && isConnState (CONN_CONNECTING))
    {
      setConnState (CONN_CONNECTED);
    }
    else if (ret <= 0)
    {
      connectionError (ret);
      return -1;
    }
    nbuf[ret] = '\0';
    processAuger ();
    syslog (LOG_DEBUG, "Rts2ConnShooter::receive date: %s", nbuf);
    successfullRead ();
    gettimeofday (&last_packet, NULL);
    // enable others to catch-up (FW connections will forward packet to their sockets)
    getMaster ()->postEvent (new Rts2Event (RTS2_EVENT_AUGER_SHOWER, nbuf));
  }
  return ret;
}

int
Rts2ConnShooter::lastPacket ()
{
  return last_packet.tv_sec;
}

double
Rts2ConnShooter::lastTargetTime ()
{
  return last_target_time;
}
