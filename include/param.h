#ifndef __RTS_PARAM__
#define __RTS_PARAM__

#include <stdlib.h>

struct param_status
{
  char *param_processing;
  char *param_argv;
  size_t argz_len;
};

extern int param_init (struct param_status **params, char *line, char sep);
extern int param_is_empty (struct param_status *params);
extern int param_get_length (struct param_status *params);
extern int param_next_char (struct param_status *params, char *ret);
extern int param_next_string (struct param_status *params, char **ret);
extern int param_next_string_copy (struct param_status *params, char *ret,
				   size_t size);
extern int param_next_integer (struct param_status *params, int *ret);
extern int param_next_time_t (struct param_status *params, time_t * ret);
extern int param_next_float (struct param_status *params, float *ret);
extern int param_next_double (struct param_status *params, double *ret);
extern int param_next_hmsdec (struct param_status *params, double *ret);
extern int param_next_ip_address (struct param_status *params,
				  char **hostname, unsigned int *port);
extern void param_done (struct param_status *params);

#endif /* __RTS_PARAM__ */
