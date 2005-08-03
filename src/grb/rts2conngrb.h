#ifndef __RTS2_GRBCONN__
#define __RTS2_GRBCONN__

#include "../utils/rts2connnosend.h"

#include "grbconst.h"
#include "grbd.h"

class Rts2DevGrb;

class Rts2ConnGrb:public Rts2ConnNoSend
{
private:
  Rts2DevGrb * master;

  long lbuf[SIZ_PKT];		// local buffer - swaped for Linux 
  long nbuf[SIZ_PKT];		// network buffer
  struct timeval last_packet;
  double here_sod;		// machine SOD (seconds after 0 GMT)
  double last_imalive_sod;	// SOD of the previous imalive packet

  double deltaValue;
  char *last_target;
  int last_target_time;

  // init listen (listening on given port) and call (try to connect to given
  // port; there must be GCN packet receiving running on oppoiste side) GCN
  // connection
  int init_listen ();
  int init_call ();

  // utility functions..
  double getPktSod ();

  void getTimeTfromTJD (long TJD, double SOD, time_t * in_time, int *usec =
			NULL);

  double getJDfromTJD (long TJD, double SOD)
  {
    return TJD + 2440000.5 + SOD / 86400.0;
  }
  // process various messages..
  int pr_imalive ();
  int pr_swift_point ();	// swift pointing.
  int pr_integral_point ();	// integral pointing
  // burst messages
  int pr_hete ();
  int pr_integral ();
  int pr_integral_spicas ();
  int pr_swift_with_radec ();
  int pr_swift_without_radec ();

  // GRB db stuff
  int addSwiftPoint (double roll, char *name, float obstime, float merit);
  int addIntegralPoint (double ra, double dec, const time_t * t);

  void getGrbBound (int grb_type, int &grb_start, int &grb_end);
  int addGcnPoint (int grb_id, int grb_seqn, int grb_type, double grb_ra,
		   double grb_dec, bool grb_is_grb, time_t * grb_date,
		   float grb_errorbox);
  int addGcnRaw (int grb_id, int grb_seqn, int grb_type);

  int gcn_port;
  char *gcn_hostname;
  int do_hete_test;

  int gcn_listen_sock;

  time_t swiftLastPoint;
  double swiftLastRa;
  double swiftLastDec;

  time_t nextTime;
public:
  Rts2ConnGrb (char *in_gcn_hostname, int in_gcn_port, int in_do_hete_test,
	       Rts2DevGrb * in_master);
  virtual ~ Rts2ConnGrb (void);
  virtual int idle ();
  virtual int init ();

  virtual int add (fd_set * set);

  virtual int connectionError ();
  virtual int receive (fd_set * set);

  int lastPacket ();
  double delta ();
  char *lastTarget ();
  void setLastTarget (char *in_last_target);
  int lastTargetTime ();
};

#endif /* !__RTS2_GRBCONN__ */
