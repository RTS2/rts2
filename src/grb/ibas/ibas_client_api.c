#include <ibas_client.h>


IBC_DL ibc_api_dl_prev[IBC_API_DL_PREV_MAX];
int ibc_api_rxtx_sock = -1;

static unsigned ibc_api_ibas_ip4addr = 0x7f000001;
static unsigned short ibc_api_ibas_port = 1966;
static int *ibc_api_signalled = NULL;


int
ibc_api_init (unsigned ibas_ip4addr, unsigned short ibas_port,
	      unsigned short my_port, int *async_signalled)
{
  struct sockaddr_in sin;


  if (IBC_ASSUMED_PACKET_SIZE != sizeof (IBC_DL))
    return (IBC_BAD_ARCH);

  ibc_api_rxtx_sock = socket (PF_INET, SOCK_DGRAM, 0);
  if (ibc_api_rxtx_sock < 0)
    {
      ibc_api_rxtx_sock = -1;
      return (IBC_NO_SOCKET);
    }

  memset (&sin, 0, sizeof (sin));
  sin.sin_family = AF_INET;
  sin.sin_addr.s_addr = htonl (INADDR_ANY);
  sin.sin_port = htons (my_port);

  if (bind (ibc_api_rxtx_sock, (struct sockaddr *) &sin, sizeof (sin)))
    {
      close (ibc_api_rxtx_sock);
      ibc_api_rxtx_sock = -1;
      return (IBC_BIND_FAILED);
    }

  memset (&ibc_api_dl_prev, 0, sizeof (ibc_api_dl_prev));
  ibc_api_ibas_ip4addr = ibas_ip4addr;
  ibc_api_ibas_port = ibas_port;
  ibc_api_signalled = async_signalled;

  return (IBC_OK);
}


int
ibc_api_set_mode (int mode)
{
  int r;

  if (-1 == ibc_api_rxtx_sock)
    return (IBC_NOT_INIT);
  r = fcntl (ibc_api_rxtx_sock, F_GETFL);
  if (r < 0)
    return (IBC_FCNTL_FAILED);
  if (IBC_NONBLOCK_MODE == mode)
    {
      r |= O_NONBLOCK;
    }
  else
    {
      r &= ~O_NONBLOCK;
    }
  if (-1 == fcntl (ibc_api_rxtx_sock, F_SETFL, r))
    return (IBC_FCNTL_FAILED);
  return (IBC_OK);
}


int
ibc_api_listen (IBC_DL * dl)
{
  int r, sr, peerlen, i, found;
  struct sockaddr_in peeraddr;
  IBC_DL dl_copy;


  if (-1 == ibc_api_rxtx_sock)
    return (IBC_NOT_INIT);

  if (NULL == dl)
    return (IBC_NUL_PTR);

  for (;;)
    {
      peerlen = sizeof (peeraddr);
      memset (&peeraddr, 0, sizeof (peeraddr));
      r =
	recvfrom (ibc_api_rxtx_sock, dl, sizeof (*dl), 0,
		  (struct sockaddr *) (&peeraddr), &peerlen);
      sr = errno;
      if (-1 == r)		/* if interupted */
	{
	  if (NULL != ibc_api_signalled)
	    if (*ibc_api_signalled)
	      {
		r = IBC_SIGNALLED;	/* return when control variable is set (within signal) */
		*ibc_api_signalled = 0;	/* and clear it */
		break;
	      }
	  if (EAGAIN == sr)
	    {
	      r = IBC_AGAIN;
	      break;
	    }
	  continue;		/* otherwise ignore interrupt and keep waiting */
	}

      if (r != sizeof (*dl))
	continue;		/* foolproof checks */

      if (ibc_api_ibas_ip4addr != ntohl (peeraddr.sin_addr.s_addr))
	continue;
      if (ibc_api_ibas_port != ntohs (peeraddr.sin_port))
	continue;

      dl_copy = *dl;		/* make a copy for possible resent */
      r = ibc_ntoh_dl (dl);	/* convert to host byte order */
      if (IBC_OK != r)
	continue;		/* CRC error (most likely) */
      if (IBAS_ALERT_ID != dl->ID)
	continue;		/* are we getting garbage or genuine alert ? */
      if (IBAS_ALERT_TEST_FLAG_R_PING & dl->a.test_flag)
	return (IBC_OK);	/* do not reply to PING_REPLY packets */

      found = 0;
      for (i = 0; i < IBC_API_DL_PREV_MAX; i++)
	{
	  if (0 == memcmp (&(ibc_api_dl_prev[i]), dl, sizeof (*dl)))
	    {
	      found = 1;
	      break;
	    }
	}
      if (found)
	continue;

      sendto (ibc_api_rxtx_sock, &dl_copy, sizeof (dl_copy), 0,
	      (struct sockaddr *) (&peeraddr), sizeof (peeraddr));
      memcpy (&(ibc_api_dl_prev[0]), &(ibc_api_dl_prev[1]),
	      sizeof (IBC_DL) * (IBC_API_DL_PREV_MAX - 1));
      ibc_api_dl_prev[IBC_API_DL_PREV_MAX - 1] = *dl;
      return (IBC_OK);
    }

  return (r);
}


int
ibc_api_send_ping (int seqnum)
{
  IBC_DL dl;
  struct sockaddr_in to_addr;
  struct timeval mytv;
  int r;


  dl.ID = IBAS_ALERT_ID;
  dl.pid = getpid ();
  dl.seqnum = seqnum;
  dl.handle = 0;
  memset (&(dl.a), 0, sizeof (dl.a));
  dl.a.test_flag = IBAS_ALERT_TEST_FLAG_Q_PING | IBAS_ALERT_TEST_FLAG_TEST;
  gettimeofday (&mytv, NULL);
  dl.a.det_flags = mytv.tv_sec;
  dl.a.att_flags = mytv.tv_usec;

  if (IBC_OK != (r = ibc_hton_dl (&dl)))
    return (r);

  memset (&to_addr, 0, sizeof (to_addr));
  to_addr.sin_family = AF_INET;
  to_addr.sin_addr.s_addr = htonl (ibc_api_ibas_ip4addr);
  to_addr.sin_port = htons (ibc_api_ibas_port);

  if (sizeof (dl) !=
      sendto (ibc_api_rxtx_sock, &dl, sizeof (dl), 0,
	      (struct sockaddr *) (&to_addr), sizeof (to_addr)))
    {
      return (IBC_SENDTO_FAILED);
    }

  return (IBC_OK);
}


int
ibc_api_shutdown (void)
{
  if (-1 != ibc_api_rxtx_sock)
    {
      close (ibc_api_rxtx_sock);
      ibc_api_rxtx_sock = -1;
    }

  ibc_api_ibas_ip4addr = 0x7f000001;
  ibc_api_ibas_port = 1966;
  ibc_api_signalled = NULL;
  return (IBC_OK);
}


int
ibc_api_dump_ping_reply (IBC_DL * dl, unsigned ip4addr, unsigned short port)
{
  double tsend, tresp;
  struct timeval mytv;

  if (NULL == dl)
    return (IBC_NUL_PTR);
  if (0 == (IBAS_ALERT_TEST_FLAG_R_PING & dl->a.test_flag))
    return (IBC_BAD_ARG);

  tsend = dl->a.det_flags + (dl->a.att_flags) / 1000000.0;
  gettimeofday (&mytv, NULL);
  tresp = mytv.tv_sec + (mytv.tv_usec) / 1000000.0;

  printf
    ("PING_REPLY: %d octets from %d.%d.%d.%d/%d: time=%.3f ms seqnum=%d\n",
     sizeof (*dl), (int) ((ip4addr >> 24) & 0xff),
     (int) ((ip4addr >> 16) & 0xff), (int) ((ip4addr >> 8) & 0xff),
     (int) (ip4addr & 0xFF), (int) port, (tresp - tsend) * 1000.0,
     dl->seqnum);

  return (IBC_OK);
}


int
ibc_api_dump_alert (IBC_DL * dl, unsigned ip4addr, unsigned short port)
{
  char system_str_utc[99], buftime[IBAS_ALERT_TIME_SIZE + 1];
  char bufcomment[IBAS_ALERT_COMMENT_SIZE + 1];
  double system_tai, alert_tai, grb_tai;
  IBC_UTC_TIME system_utc, alert_utc, grb_utc;

  if (NULL == dl)
    return (IBC_NUL_PTR);
  if (IBAS_ALERT_TEST_FLAG_R_PING & dl->a.test_flag)
    return (IBC_BAD_ARG);

  system_str_utc[0] = 0;
  system_tai = 0.0;
  if (IBC_OK == ibc_current_utc (&system_utc))
    {
      if (IBC_OK != ibc_utc2str24 (system_utc, &(system_str_utc[0])))
	{
	  system_str_utc[0] = 0;
	}
      else
	{
	  system_str_utc[24] = 0;
	}
      if (IBC_OK != ibc_utc2tai2ert (&system_utc, &system_tai, NULL))
	system_tai = 0.0;
    }

  alert_tai = 0.0;
  if (IBC_OK == ibc_str242utc (dl->a.pkt_time, &alert_utc))
    {
      if (IBC_OK != ibc_utc2tai2ert (&alert_utc, &alert_tai, NULL))
	alert_tai = 0.0;
    }

  grb_tai = 0.0;
  if (IBC_OK == ibc_str242utc (dl->a.grb_time, &grb_utc))
    {
      if (IBC_OK != ibc_utc2tai2ert (&grb_utc, &grb_tai, NULL))
	grb_tai = 0.0;
    }

  printf ("\n\n  ======== IBAS alert received ========\n\n");
  printf ("dl.ID                      = 0x%08x\n", dl->ID);
  printf ("dl.pid                     = %d\n", dl->pid);
  printf ("dl.seqnum                  = %d\n", dl->seqnum);
  printf ("dl.handle                  = %d\n", dl->handle);
  printf ("dl.a.pkt_type              = %d\n", dl->a.pkt_type);
  printf ("dl.a.test_flag             = %d [odd number => TEST alert]\n",
	  dl->a.test_flag);
  printf ("dl.a.pkt_number            = %d\n", dl->a.pkt_number);
  strncpy (buftime, dl->a.pkt_time, IBAS_ALERT_TIME_SIZE);
  buftime[IBAS_ALERT_TIME_SIZE] = 0;
  printf ("dl.a.pkt_time              = %s [UTC (packet transmission) ]\n",
	  buftime);
  printf ("dl.a.alert_number          = %d\n", dl->a.alert_number);
  printf ("dl.a.alert_subnum          = %d\n", dl->a.alert_subnum);
  printf ("dl.a.nx_point_ra           = %.5f [J2000 DEG]\n",
	  dl->a.nx_point_ra);
  printf ("dl.a.nx_point_dec          = %.5f [J2000 DEG]\n",
	  dl->a.nx_point_dec);
  strncpy (buftime, dl->a.nx_point_time, IBAS_ALERT_TIME_SIZE);
  buftime[IBAS_ALERT_TIME_SIZE] = 0;
  printf ("dl.a.nx_point_time         = %s [UTC]\n", buftime);
  strncpy (buftime, dl->a.grb_time, IBAS_ALERT_TIME_SIZE);
  buftime[IBAS_ALERT_TIME_SIZE] = 0;
  printf ("dl.a.grb_time              = %s [UTC]\n", buftime);
  printf ("dl.a.grb_time_err          = %.5f [COBT seconds]\n",
	  dl->a.grb_time_err);
  printf ("dl.a.grb_ra                = %.5f [J2000 DEG]\n", dl->a.grb_ra);
  printf ("dl.a.grb_dec               = %.5f [J2000 DEG]\n", dl->a.grb_dec);
  printf ("dl.a.grb_pos_err           = %.5f [arcmin]\n", dl->a.grb_pos_err);
  printf ("dl.a.grb_sigma             = %.5f\n", dl->a.grb_sigma);
  printf ("dl.a.grb_timescale         = %.5f [COBT seconds]\n",
	  dl->a.grb_timescale);
  printf ("dl.a.point_ra              = %.5f [J2000 DEG]\n", dl->a.point_ra);
  printf ("dl.a.point_dec             = %.5f [J2000 DEG]\n", dl->a.point_dec);
  printf ("dl.a.det_flags             = %d\n", dl->a.det_flags);
  printf ("dl.a.att_flags             = %d\n", dl->a.att_flags);
  printf ("dl.a.mult_pos              = %d\n", dl->a.mult_pos);

  strncpy (bufcomment, dl->a.comment, IBAS_ALERT_COMMENT_SIZE);
  buftime[IBAS_ALERT_TIME_SIZE] = 0;
  printf ("dl.a.comment               = %s\n", bufcomment);

  printf ("dl.a.crc16                 = %02X%02X\n", dl->a.crc16[0],
	  dl->a.crc16[1]);

  printf ("\n");
  printf ("Reception time             : %s\n", system_str_utc);
  printf
    ("(Reception - transmission) : %.4f [seconds] (assuming clocks synced)\n",
     system_tai - alert_tai);
  printf
    ("(Reception - GRB event)    : %.4f [seconds] (assuming clocks synced)\n",
     system_tai - grb_tai);
  printf ("\n");

  return (IBC_OK);
}
