#define _GNU_SOURCE

#ifndef __RTS_PARAM__
#define __RTS_PARAM__

struct param_status
{
  char *param_processing;
  char *param_argv;
  size_t argz_len;
};

int param_init (struct param_status **params, char *line, char sep);
int param_is_empty (struct param_status *params);
int param_get_length (struct param_status *params);
int param_next_string (struct param_status *params, char **ret);
int param_next_integer (struct param_status *params, int *ret);
int param_next_time_t (struct param_status *params, time_t * ret);
int param_next_float (struct param_status *params, float *ret);
int param_next_hmsdec (struct param_status *params, double *ret);
int param_next_ip_address (struct param_status *params, char **hostname,
			   unsigned int *port);
void param_done (struct param_status *params);

#endif /* __RTS_PARAM__ */
