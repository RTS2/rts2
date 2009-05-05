#include "connshooter.h"
#include "../utils/libnova_cpp.h"

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

#define GPS_OFFSET  315964800

using namespace rts2too;

void
ConnShooter::getTimeTfromGPS (long GPSsec, long GPSusec, double &out_time)
{
  // we need to handle somehow better leap seconds, but that can wait
  out_time = GPSsec + GPS_OFFSET + GPSusec / USEC_SEC + 14.0;
}


// is called when nbuf contains '\n'
int
ConnShooter::processAuger ()
{
  EXEC SQL BEGIN DECLARE SECTION;
    //  int db_auger_t3id;
    double db_auger_date;
    int db_auger_npixels = 0;

    double db_auger_sdpphi;
    double db_auger_sdptheta;
    double db_auger_sdpangle = 0;

    double db_auger_ra;
    double db_auger_dec;
  EXEC SQL END DECLARE SECTION;

  int gap_ver;
  int gap_stations;
  double gap_theta;
  double gap_phi;
  double gap_energy;

  double gap_L;
  double gap_B;
  long gap_UTC;
  long gap_core;
  double gap_X;

  double gap_Y;
  double gap_S1000;
  double gap_dS1000;

  double gap_ctheta;
  double gap_cphi;
  double gap_dX;

  double gap_dY;
  double gap_comp;
  long gap_isT5;
  long gap_isT5p;
  long gap_isT5pp;

  long gap_isICR;
  long gap_isFd;
  double gap_geomfit;
  double gap_LDFfit;
  double gap_globfitchi2;

  double gap_globfitndof;
  double gap_LDFBeta;
  double gap_LDFGamma;
  double gap_R;
  long gap_OldId;

  long gps_sec;

  long gps_usec = 0;

  time_t now;

  // 1 3 34.6514 -105.751 0.476843
  // -70.1905 -7.69034 1153170055 10108491 -24303.2
  // 24092.6 2.66637 0.662263 157.832 -66.922
  // 34.5033 -106.143 1.90774 1.92494 66.7804
  // 145.449 1.11 1039 1 1
  // 1 0 4.23941e-05 0.927834 0.903655
  // 0 -3.75494 -0.447885 9180.13 2460068
  // 837205268

  std::istringstream _is (nbuf);
  _is
    >> gap_ver
    >> gap_stations
    >> gap_theta
    >> gap_phi
    >> gap_energy

    >> gap_L
    >> gap_B
    >> gap_UTC
    >> gap_core
    >> gap_X

    >> gap_Y
    >> gap_S1000
    >> gap_dS1000
    >> db_auger_ra
    >> db_auger_dec

    >> gap_ctheta
    >> gap_cphi
    >> db_auger_sdptheta
    >> db_auger_sdpphi
    >> gap_dX

    >> gap_dY
    >> gap_comp
    >> gap_isT5
    >> gap_isT5p
    >> gap_isT5pp

    >> gap_isICR
    >> gap_isFd
    >> gap_geomfit
    >> gap_LDFfit
    >> gap_globfitchi2

    >> gap_globfitndof
    >> gap_LDFBeta
    >> gap_LDFGamma
    >> gap_R
    >> gap_OldId

    >> gps_sec;

  if (_is.fail ())
  {
    logStream (MESSAGE_ERROR) << "ConnShooter::processAuger failed reading stream" << sendLog;
    return -1;
  }

  // valid shover and it's hibrid..

  getTimeTfromGPS (gps_sec, gps_usec, db_auger_date);

  time (&now);

  if ((!(gap_comp && gap_isT5 && gap_energy > minEnergy))
    || now - db_auger_date > maxTime
    || master->wasSeen (db_auger_date, db_auger_ra, db_auger_dec))
  {
    logStream (MESSAGE_INFO) << "ConnShooter::processAuger ignore (gap_comp "
      << gap_comp
      << " date " << LibnovaDateDouble (db_auger_date)
      << " gap_isT5 " << gap_isT5
      << " gap_energy " << gap_energy
      << " minEnergy " << minEnergy
      << " ra " << db_auger_ra
      << " dec " << db_auger_dec
      << ")" << sendLog;
    return -1;
  }

  EXEC SQL INSERT INTO
      auger
      (auger_t3id,
      auger_date,
      auger_npixels,
      auger_sdpphi,
      auger_sdptheta,
      auger_sdpangle,
      auger_ra,
      auger_dec)
    VALUES
      (nextval('auger_t3id'),
      (TIMESTAMP 'epoch' + :db_auger_date * INTERVAL '1 seconds'),
      :db_auger_npixels,
      :db_auger_sdpphi,
      :db_auger_sdptheta,
      :db_auger_sdpangle,
      :db_auger_ra,
      :db_auger_dec);
  if (sqlca.sqlcode)
  {
    logStream (MESSAGE_ERROR)
      << "ConnShooter::processAuger cannot add new value to db: "
      << sqlca.sqlerrm.sqlerrmc << " (" << sqlca.sqlcode << ")";
    EXEC SQL ROLLBACK;
    return -1;
  }
  EXEC SQL COMMIT;
  return master->newShower (db_auger_date, db_auger_ra, db_auger_dec);
}


ConnShooter::ConnShooter (int in_port, AugerShooter *in_master, double in_minEnergy, int in_maxTime):Rts2ConnNoSend (in_master)
{
  master = in_master;
  port = in_port;

  time (&last_packet.tv_sec);
  last_packet.tv_sec -= 600;
  last_packet.tv_usec = 0;

  last_target_time = -1;

  setConnTimeout (-1);

  minEnergy = in_minEnergy;
  maxTime = in_maxTime;
}


ConnShooter::~ConnShooter (void)
{
}


int
ConnShooter::idle ()
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
        logStream (MESSAGE_ERROR) << "ConnShooter::idle getsockopt " << strerror (errno) << sendLog;
        connectionError (-1);
      }
      else if (err)
      {
        logStream (MESSAGE_ERROR) << "ConnShooter::idle getsockopt " << strerror (err) << sendLog;
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
ConnShooter::init_listen ()
{
  int ret;

  connectionError (-1);

  sock = socket (AF_INET, SOCK_DGRAM, 0);
  if (sock == -1)
  {
    logStream (MESSAGE_ERROR) << "ConnShooter::init_listen socket " << strerror (errno) << sendLog;
    return -1;
  }
  struct sockaddr_in server;
  server.sin_family = AF_INET;
  server.sin_port = htons (port);
  server.sin_addr.s_addr = htonl (INADDR_ANY);

  ret = fcntl (sock, F_SETFL, O_NONBLOCK);
  if (ret)
  {
    logStream (MESSAGE_ERROR) << "ConnShooter::init_listen fcntl: " << strerror (errno) << sendLog;
    return -1;
  }

  ret =
    bind (sock, (struct sockaddr *) &server, sizeof (server));
  if (ret)
  {
    logStream (MESSAGE_ERROR) << "ConnShooter::init_listen bind: " << strerror (errno) << sendLog;
    return -1;
  }
  setConnState (CONN_CONNECTED);
  return 0;
}


int
ConnShooter::init ()
{
  return init_listen ();
}


void
ConnShooter::connectionError (int last_data_size)
{
  logStream (MESSAGE_DEBUG) << "ConnShooter::connectionError" << sendLog;
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
}


int
ConnShooter::receive (fd_set * set)
{
  int ret = 0;
  if (sock >= 0 && FD_ISSET (sock, set))
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
    logStream (MESSAGE_DEBUG) << "ConnShooter::receive data: " << nbuf << sendLog;
    successfullRead ();
    gettimeofday (&last_packet, NULL);
    // enable others to catch-up (FW connections will forward packet to their sockets)
    getMaster ()->postEvent (new Rts2Event (RTS2_EVENT_AUGER_SHOWER, nbuf));
  }
  return ret;
}


int
ConnShooter::lastPacket ()
{
  return last_packet.tv_sec;
}


double
ConnShooter::lastTargetTime ()
{
  return last_target_time;
}
