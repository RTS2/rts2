#ifndef __RTS2_TCF__
#define __RTS2_TCF__

#ifdef __cplusplus
extern "C"
{
#endif

  void set_focuser_port (char *in_focuser_port);
  int input_timeout (int desc, unsigned int seconds);
  int tcf_set_manual ();
  int tcf_step_out (int num, int direction);
  int tcf_get_pos (int *position);
  int tcf_set_center ();
  int tcf_set_sleep ();
  int tcf_set_wakeup ();
  int tcf_set_auto ();

#ifdef __cplusplus
};
#endif

#endif /* !__RTS2_TCF__ */
