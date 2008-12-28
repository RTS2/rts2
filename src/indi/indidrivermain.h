#ifndef __INDI_DRIVERMAIN__
#define __INDI_DRIVERMAIN__

#ifdef __cplusplus
extern "C" {
#endif

extern void IDProcessParams (int ac, char *av[]);
extern void IDIncVerbosity ();
extern int IDMain ();

#ifdef __cplusplus
}
#endif

#endif // !__INDI_DRIVERMAIN__ 
