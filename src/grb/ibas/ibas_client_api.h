#define	IBC_API_DL_PREV_MAX	(32)


extern IBC_DL ia_api_udp_dl_prev[IBC_API_DL_PREV_MAX];
extern int ia_api_udp_rxtx_sock;


int ibc_api_init (unsigned ibas_ip4addr, unsigned short ibas_port,
		  unsigned short my_port, int *async_signalled);
int ibc_api_set_mode (int mode);
int ibc_api_listen (IBC_DL * dl);
int ibc_api_send_ping (int seqnum);
int ibc_api_shutdown (void);
int ibc_api_dump_ping_reply (IBC_DL * dl, unsigned ip4addr,
			     unsigned short port);
int ibc_api_dump_alert (IBC_DL * dl, unsigned ip4addr, unsigned short port);
