#include <ibas_client.h>


	/* precomputed table for CRC-16 CCITT algorithm for packet CRC calculation */

static ushort2 ibc_crc_16_tab[1 << BYTE_BITS] =
  { 0x0000, 0x1021, 0x2042, 0x3063, 0x4084, 0x50a5, 0x60c6, 0x70e7,
  0x8108, 0x9129, 0xa14a, 0xb16b, 0xc18c, 0xd1ad, 0xe1ce, 0xf1ef,
  0x1231, 0x0210, 0x3273, 0x2252, 0x52b5, 0x4294, 0x72f7, 0x62d6,
  0x9339, 0x8318, 0xb37b, 0xa35a, 0xd3bd, 0xc39c, 0xf3ff, 0xe3de,
  0x2462, 0x3443, 0x0420, 0x1401, 0x64e6, 0x74c7, 0x44a4, 0x5485,
  0xa56a, 0xb54b, 0x8528, 0x9509, 0xe5ee, 0xf5cf, 0xc5ac, 0xd58d,
  0x3653, 0x2672, 0x1611, 0x0630, 0x76d7, 0x66f6, 0x5695, 0x46b4,
  0xb75b, 0xa77a, 0x9719, 0x8738, 0xf7df, 0xe7fe, 0xd79d, 0xc7bc,
  0x48c4, 0x58e5, 0x6886, 0x78a7, 0x0840, 0x1861, 0x2802, 0x3823,
  0xc9cc, 0xd9ed, 0xe98e, 0xf9af, 0x8948, 0x9969, 0xa90a, 0xb92b,
  0x5af5, 0x4ad4, 0x7ab7, 0x6a96, 0x1a71, 0x0a50, 0x3a33, 0x2a12,
  0xdbfd, 0xcbdc, 0xfbbf, 0xeb9e, 0x9b79, 0x8b58, 0xbb3b, 0xab1a,
  0x6ca6, 0x7c87, 0x4ce4, 0x5cc5, 0x2c22, 0x3c03, 0x0c60, 0x1c41,
  0xedae, 0xfd8f, 0xcdec, 0xddcd, 0xad2a, 0xbd0b, 0x8d68, 0x9d49,
  0x7e97, 0x6eb6, 0x5ed5, 0x4ef4, 0x3e13, 0x2e32, 0x1e51, 0x0e70,
  0xff9f, 0xefbe, 0xdfdd, 0xcffc, 0xbf1b, 0xaf3a, 0x9f59, 0x8f78,
  0x9188, 0x81a9, 0xb1ca, 0xa1eb, 0xd10c, 0xc12d, 0xf14e, 0xe16f,
  0x1080, 0x00a1, 0x30c2, 0x20e3, 0x5004, 0x4025, 0x7046, 0x6067,
  0x83b9, 0x9398, 0xa3fb, 0xb3da, 0xc33d, 0xd31c, 0xe37f, 0xf35e,
  0x02b1, 0x1290, 0x22f3, 0x32d2, 0x4235, 0x5214, 0x6277, 0x7256,
  0xb5ea, 0xa5cb, 0x95a8, 0x8589, 0xf56e, 0xe54f, 0xd52c, 0xc50d,
  0x34e2, 0x24c3, 0x14a0, 0x0481, 0x7466, 0x6447, 0x5424, 0x4405,
  0xa7db, 0xb7fa, 0x8799, 0x97b8, 0xe75f, 0xf77e, 0xc71d, 0xd73c,
  0x26d3, 0x36f2, 0x0691, 0x16b0, 0x6657, 0x7676, 0x4615, 0x5634,
  0xd94c, 0xc96d, 0xf90e, 0xe92f, 0x99c8, 0x89e9, 0xb98a, 0xa9ab,
  0x5844, 0x4865, 0x7806, 0x6827, 0x18c0, 0x08e1, 0x3882, 0x28a3,
  0xcb7d, 0xdb5c, 0xeb3f, 0xfb1e, 0x8bf9, 0x9bd8, 0xabbb, 0xbb9a,
  0x4a75, 0x5a54, 0x6a37, 0x7a16, 0x0af1, 0x1ad0, 0x2ab3, 0x3a92,
  0xfd2e, 0xed0f, 0xdd6c, 0xcd4d, 0xbdaa, 0xad8b, 0x9de8, 0x8dc9,
  0x7c26, 0x6c07, 0x5c64, 0x4c45, 0x3ca2, 0x2c83, 0x1ce0, 0x0cc1,
  0xef1f, 0xff3e, 0xcf5d, 0xdf7c, 0xaf9b, 0xbfba, 0x8fd9, 0x9ff8,
  0x6e17, 0x7e36, 0x4e55, 0x5e74, 0x2e93, 0x3eb2, 0x0ed1, 0x1ef0
};

	/* TM packet CRC calculation algorithm (crc should be initially set to 0xffff) */

ushort2
ibc_crc_16 (ushort2 crc, byte * p, int nbytes)
{
  for (; nbytes > 0; nbytes--)
    crc =
      (crc << BYTE_BITS) ^ ibc_crc_16_tab[(crc >> (CRC_BITS - BYTE_BITS)) ^
					  *p++];
  return (crc);
}


int
ibc_endian_test_double (void)
{
  static IBC_ENDIAN_DOUBLE ed_big = { {0x3f, 0xf3, 0xc0, 0x83, 0x12, 0x6e, 0x97, 0x8d} };	/* 1.2345 */
  static IBC_ENDIAN_DOUBLE ed_little = { {0x8d, 0x97, 0x6e, 0x12, 0x83, 0xc0, 0xf3, 0x3f} };	/* 1.2345 */
  IBC_ENDIAN_DOUBLE ed_test;

  ed_test.d = 1.2345;
  if (0 == memcmp (&(ed_big.c[0]), &(ed_test.c[0]), 8))
    return (IBC_BIG_ENDIAN);
  if (0 == memcmp (&(ed_little.c[0]), &(ed_test.c[0]), 8))
    return (IBC_LITTLE_ENDIAN);
  return (IBC_UNKNOWN_ENDIAN);
}


int
ibc_ntoh_double (double *x)
{
  unsigned char s[8], d[8];
  int i;

  if ((NULL != x) && (IBC_LITTLE_ENDIAN == ibc_endian_test_double ()))
    {
      memcpy (&(s[0]), x, 8);
      for (i = 0; i < 8; i++)
	d[7 - i] = s[i];
      memcpy (x, &(d[0]), 8);
    }
  return (IBC_OK);
}


int
ibc_hton_double (double *x)
{
  unsigned char s[8], d[8];
  int i;

  if ((NULL != x) && (IBC_LITTLE_ENDIAN == ibc_endian_test_double ()))
    {
      memcpy (&(s[0]), x, 8);
      for (i = 0; i < 8; i++)
	d[7 - i] = s[i];
      memcpy (x, &(d[0]), 8);
    }
  return (IBC_OK);
}


int
ibc_hton_alert (IBAS_ALERT * a)
{
  if (NULL == a)
    return (IBC_NUL_PTR);

  a->pkt_type = htons (a->pkt_type);
  a->test_flag = htons (a->test_flag);
  a->pkt_number = htonl (a->pkt_number);
  a->alert_number = htonl (a->alert_number);
  a->alert_subnum = htonl (a->alert_subnum);
  ibc_hton_double (&(a->nx_point_ra));
  ibc_hton_double (&(a->nx_point_dec));
  ibc_hton_double (&(a->grb_time_err));
  ibc_hton_double (&(a->grb_ra));
  ibc_hton_double (&(a->grb_dec));
  ibc_hton_double (&(a->grb_pos_err));
  ibc_hton_double (&(a->grb_sigma));
  ibc_hton_double (&(a->grb_timescale));
  ibc_hton_double (&(a->point_ra));
  ibc_hton_double (&(a->point_dec));
  a->det_flags = htonl (a->det_flags);
  a->att_flags = htonl (a->att_flags);
  a->mult_pos = htonl (a->mult_pos);

  return (IBC_OK);
}


int
ibc_ntoh_alert (IBAS_ALERT * a)
{
  if (NULL == a)
    return (IBC_NUL_PTR);

  a->pkt_type = ntohs (a->pkt_type);
  a->test_flag = ntohs (a->test_flag);
  a->pkt_number = ntohl (a->pkt_number);
  a->alert_number = ntohl (a->alert_number);
  a->alert_subnum = ntohl (a->alert_subnum);
  ibc_ntoh_double (&(a->nx_point_ra));
  ibc_ntoh_double (&(a->nx_point_dec));
  ibc_ntoh_double (&(a->grb_time_err));
  ibc_ntoh_double (&(a->grb_ra));
  ibc_ntoh_double (&(a->grb_dec));
  ibc_ntoh_double (&(a->grb_pos_err));
  ibc_ntoh_double (&(a->grb_sigma));
  ibc_ntoh_double (&(a->grb_timescale));
  ibc_ntoh_double (&(a->point_ra));
  ibc_ntoh_double (&(a->point_dec));
  a->det_flags = ntohl (a->det_flags);
  a->att_flags = ntohl (a->att_flags);
  a->mult_pos = ntohl (a->mult_pos);

  return (IBC_OK);
}


int
ibc_hton_dl (IBC_DL * dl)
{
  ushort2 crc;

  if (NULL == dl)
    return (IBC_NUL_PTR);
  dl->ID = htonl (dl->ID);
  dl->pid = htonl (dl->pid);
  dl->seqnum = htonl (dl->seqnum);
  dl->handle = htonl (dl->handle);
  ibc_hton_alert (&(dl->a));

  crc = ibc_crc_16 (0xffff, (void *) dl, sizeof (*dl) - 2);
  dl->a.crc16[0] = crc >> 8;
  dl->a.crc16[1] = crc & 0xff;

  return (IBC_OK);
}


int
ibc_ntoh_dl (IBC_DL * dl)
{
  if (NULL == dl)
    return (IBC_NUL_PTR);

  if (ibc_crc_16 (0xffff, (void *) dl, sizeof (*dl)))
    return (IBC_BAD_CRC);

  dl->ID = ntohl (dl->ID);
  dl->pid = ntohl (dl->pid);
  dl->seqnum = ntohl (dl->seqnum);
  dl->handle = ntohl (dl->handle);
  return (ibc_ntoh_alert (&(dl->a)));
}


int
ibc_sleep_msec (int ms, int *ctrlvar)
{
#ifdef IBC_ARCH_MACOSX_CC
  usleep (1000 * ms);
#else
  struct timespec rqtp;

  rqtp.tv_sec = ms / 1000;
  rqtp.tv_nsec = (ms % 1000) * 1000000;
  nanosleep (&rqtp, NULL);
#endif
  return (IBC_OK);
}
