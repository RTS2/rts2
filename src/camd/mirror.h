#ifndef __RTS2_MIRROR__
#define __RTS2_MIRROR__

#ifdef __cplusplus
extern "C"
{
#endif

  int mirror_open_dev (char *dev);

  int mirror_open ();

  int mirror_close ();

#ifdef __cplusplus
};
#endif

#endif /* ! __RTS2_MIRROR__ */
