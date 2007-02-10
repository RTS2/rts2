/*
 * saoprintblk.c
 */

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <stropts.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/signal.h>
#include <sys/errno.h>

char devname[16];


int total = 0;



void
sao_print_head ()
{
  printf ("\nSSBBCCC PP VHPPP  AAAA          \t\t\t\t      (decimal)\n");
  printf ("YYTTDDD TT NNPPP H1111AAAAAAAAAA\t\t\t\t      (address)\n");
  printf ("1010210 21 DD210 G32109876543210\n\n");
}

void
sao_print_command (cptr, num)
     u_int cptr[];
     int num;
{
  int ii, jj;

  for (ii = 0; ii < num; ii++)
    {
      if (total % 64 == 0)
	sao_print_head ();
      total++;
      for (jj = 31; jj >= 0; jj--)
	{
	  printf ("%1d", (cptr[ii] >> jj) & 1);
	}
      printf ("  = %0x", cptr[ii]);
      if ((cptr[ii] & 0x00034000) == 0x00034000)
	printf ("   Serial Skip  (PAT %2d)\t(%6d)\n",
		(cptr[ii] & 0x00003000) >> 12, cptr[ii] & 0x00000fff);
      else if ((cptr[ii] & 0x00024000) == 0x00024000)
	printf ("   Serial Add   (PAT %2d)\t(%6d)\n",
		(cptr[ii] & 0x00003000) >> 12, cptr[ii] & 0x00000fff);
      else if ((cptr[ii] & 0x00014000) == 0x00014000)
	printf ("   Serial Read  (PAT %2d)\t(%6d)\n",
		(cptr[ii] & 0x00003000) >> 12, cptr[ii] & 0x00000fff);
      else if ((cptr[ii] & 0x00084000) == 0x00084000)
	printf ("   HEND         (PAT %2d)\t(%6d)\n",
		(cptr[ii] & 0x00003000) >> 12, cptr[ii] & 0x00000fff);
      else if ((cptr[ii] & 0x00004000) == 0x00004000)
	printf ("   Serial No-op (PAT %2d)\t(%6d)\n",
		(cptr[ii] & 0x00003000) >> 12, cptr[ii] & 0x00000fff);
      else if ((cptr[ii] & 0x00030000) == 0x00030000)
	printf ("   Parallel Skip (use PT %1d)\t(%6d)\n",
		(cptr[ii] & 0x00c00000) >> 22, cptr[ii] & 0x00003fff);
      else if ((cptr[ii] & 0x00020000) == 0x00020000)
	printf ("   Parallel Add  (use PT %1d)\t(%6d)\n",
		(cptr[ii] & 0x00c00000) >> 22, cptr[ii] & 0x00003fff);
      else if ((cptr[ii] & 0x00010000) == 0x00010000)
	printf ("   Parallel Read (use PT %1d)\t(%6d)\n",
		(cptr[ii] & 0x00c00000) >> 22, cptr[ii] & 0x00003fff);
      else if ((cptr[ii] & 0x00100000) == 0x00100000)
	printf ("   VEND          (use PT %1d)\t(%6d)\n",
		(cptr[ii] & 0x00c00000) >> 22, cptr[ii] & 0x00003fff);
      else if ((cptr[ii] & 0xfff0ffff) != 0x00000000)
	printf ("   Parallel No-op(use PT %1d)\t(%6d)\n",
		(cptr[ii] & 0x00c00000) >> 22, cptr[ii] & 0x00003fff);
      else if ((cptr[ii] & 0x00000000) == 0x00000000)
	printf ("   Enable\n");
      else
	printf ("   Unknown\n");
    }
}
