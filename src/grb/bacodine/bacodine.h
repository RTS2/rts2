#ifndef __RTS_GRB_BACODINE__
#define __RTS_GRB_BACODINE__

#ifdef __cplusplus
extern "C"
{
#endif

  extern void *receive_bacodine (void *arg);
  extern int open_conn (char *hostname, int port);
  extern int open_server (char *hostname, int port);

#ifdef __cplusplus
};
#endif

#endif /* __RTS_GRB_BACODINE__ */
