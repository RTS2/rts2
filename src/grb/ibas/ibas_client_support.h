ushort2 ibc_crc_16 (ushort2 crc, byte * p, int nbytes);
int ibc_endian_test_double (void);
int ibc_ntoh_double (double *x);
int ibc_hton_double (double *x);
int ibc_hton_alert (IBAS_ALERT * a);
int ibc_ntoh_alert (IBAS_ALERT * a);
int ibc_hton_dl (IBC_DL * dl);
int ibc_ntoh_dl (IBC_DL * dl);
int ibc_sleep_msec (int ms, int *ctrlvar);
