#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <stdio.h>
#include <unistd.h>

#include "maps.h"

int _Tstat_handle, _Tctrl_handle;
struct T9_ctrl *Tctrl;
struct T9_stat *Tstat;

void
mk_maps ()			// server side
{
  mkdir (FILEPATH, 0755);

  {
    char a[sizeof (struct T9_ctrl)];
    _Tstat_handle = open (STATFILE, O_CREAT | O_RDWR, 0755);	// O_TRUNC ???
    write (_Tstat_handle, a, sizeof (struct T9_stat));
    close (_Tstat_handle);
  }

  _Tstat_handle = open (STATFILE, O_RDWR, 0755);
  Tstat =
    (struct T9_stat *) mmap (NULL, sizeof (struct T9_stat),
			     PROT_WRITE | PROT_READ, MAP_SHARED,
			     _Tstat_handle, 0);
  if (Tstat == MAP_FAILED)
    fprintf (stderr, "mmap %f failed (%s)\n", STATFILE, strerror (errno));
  else
    fprintf (stderr, "%s mapped to %p\n", STATFILE, Tstat);

  {
    struct T9_ctrl a;

    a.ra = 88.79275;
    a.dec = +7.40693;
    a.power = 0;

    _Tctrl_handle = open (CTRLFILE, O_CREAT | O_RDWR, 0755);	// O_TRUNC ???
    write (_Tctrl_handle, &a, sizeof (struct T9_ctrl));
    close (_Tctrl_handle);
  }

  _Tctrl_handle = open (CTRLFILE, O_RDWR, 0755);
  Tctrl =
    (struct T9_ctrl *) mmap (NULL, sizeof (struct T9_ctrl), PROT_READ,
			     MAP_SHARED, _Tctrl_handle, 0);
  if (Tctrl == MAP_FAILED)
    fprintf (stderr, "mmap %s failed: %s\n", CTRLFILE, strerror (errno));
  else
    fprintf (stderr, "%s mapped to %p\n", CTRLFILE, Tctrl);

  fflush (stderr);
}

void
use_maps ()			// client side
{
  _Tstat_handle = open (STATFILE, O_RDWR, 0755);
  Tstat =
    (struct T9_stat *) mmap (NULL, sizeof (struct T9_stat), PROT_READ,
			     MAP_SHARED, _Tstat_handle, 0);
  if (Tstat == MAP_FAILED)
    fprintf (stderr, "mmap %f failed (%s)\n", STATFILE, strerror (errno));
  else
    fprintf (stderr, "%s mapped to %p\n", STATFILE, Tstat);

  _Tctrl_handle = open (CTRLFILE, O_RDWR, 0755);
  Tctrl =
    (struct T9_ctrl *) mmap (NULL, sizeof (struct T9_ctrl),
			     PROT_WRITE | PROT_READ, MAP_SHARED,
			     _Tctrl_handle, 0);
  if (Tctrl == MAP_FAILED)
    fprintf (stderr, "mmap %s failed: %s\n", CTRLFILE, strerror (errno));
  else
    fprintf (stderr, "%s mapped to %p\n", CTRLFILE, Tctrl);

  fflush (stderr);
}
