#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include "../utils/rts2config.h"

#include <getopt.h>
#include <libnova/libnova.h>
#include <iostream>
#include <fstream>
#include <stdlib.h>

EXEC SQL include sqlca;

using namespace std;

void
printDate (char *text, double JD)
{
  struct ln_date     date;

  ln_get_date (JD, &date);
  cout << text << date.days << "/" << date.months << "/" << date.years
     << " " << date.hours << ":" << date.minutes << ":" <<
     date.seconds << "\n";
}

int
getInfo (int tar_id, struct ln_lnlat_posn *pos, double JD)
{
}

int
main (int argc, char **argv)
{
  struct ln_lnlat_posn pos;
  double JD;
  int c;

  JD = ln_get_julian_from_sys ();
  
  if (read_config (CONFIG_FILE) == -1)
  {
     cerr << 
      "Cannot read configuration file " CONFIG_FILE ", exiting.";
     exit (1); 
  }

  pos.lat = get_double_default ("latitude", 0);
  pos.lng = get_double_default ("longtitude", 0);

  struct option long_option[] = {
     {"target_id", 1, 0, 't'},
     {"grb_id", 1, 0, 'g'},
     {"help", 1, 0, 'h'},
     {0, 0, 0, 0}
  };

  db_connect ();
  // get the options..
  while (1)
  {
    c = getopt_long (argc, argv, "t:g:h", long_option, NULL);
    
    if (c == -1)
      break;

    switch (c)
    {
      case 'h':
         cout << "Put multiple -t, followed by target id, on command line\n";
	 break;
      case 't':
         int tar;
	 tar = atoi (optarg);
	 getInfo (tar, &pos, JD);
         break;
    }
  }
  db_disconnect ();
}
