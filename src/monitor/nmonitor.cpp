#include <curses.h>
#include <libnova/libnova.h>
#include <getopt.h>
#include <panel.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <pthread.h>
#include <limits.h>

#include <iostream>
#include <fstream>

#include "../utils/rts2block.h"
#include "../utils/rts2client.h"
#include "../utils/rts2command.h"

#include "../writers/rts2image.h"
#include "../writers/rts2devcliimg.h"

#define LINE_SIZE	13
#define COL_SIZE	25

#define CLR_OK		1
#define CLR_TEXT	2
#define CLR_PRIORITY	3
#define CLR_WARNING	4
#define CLR_FAILURE	5
#define CLR_STATUS      6

#define CMD_BUF_LEN    100

#define EVENT_PRINT		RTS2_LOCAL_EVENT + 1
#define EVENT_PRINT_SHORT	RTS2_LOCAL_EVENT + 2
#define EVENT_PRINT_FULL	RTS2_LOCAL_EVENT + 3
#define EVENT_PRINT_MESSAGES	RTS2_LOCAL_EVENT + 4

class Rts2CNMonConn:public Rts2ConnClient
{
private:
  WINDOW * window;
protected:
  virtual void print ();

public:
    Rts2CNMonConn (Rts2Block * in_master,
		   char *in_name):Rts2ConnClient (in_master, in_name)
  {
    window = NULL;
  }
  virtual ~ Rts2CNMonConn (void)
  {
    if (window)
      {
	wclear (window);
	wrefresh (window);
	delwin (window);
      }
  }

  virtual void postEvent (Rts2Event * event)
  {
    switch (event->getType ())
      {
      case EVENT_PRINT:
	event->setArg ((void *) window);
	break;
      }
    Rts2ConnClient::postEvent (event);
  }

  int hasWindow ()
  {
    return window != NULL;
  }

  void setWindow (WINDOW * in_window)
  {
    if (hasWindow ())
      delwin (window);
    window = in_window;
  }

  virtual int commandReturn (Rts2Command * cmd, int in_status)
  {
    print ();
    wrefresh (window);
    return 0;
  }

  virtual int command ()
  {
    // for immediate updates of values..
    int ret;
    ret = Rts2ConnClient::command ();
    print ();
    wrefresh (window);
    return ret;
  }

  virtual int setState (char *in_state_name, int in_value)
  {
    int ret;
    ret = Rts2ConnClient::setState (in_state_name, in_value);
    print ();
    wrefresh (window);
    return ret;
  }
};

class MonWindow
{
private:
  int printFormat;
  int statusBegin;
protected:
  int printStatus (WINDOW * window, Rts2Conn * connection);

  virtual void print (WINDOW * wnd) = 0;
  virtual void printOneLine (WINDOW * wnd) = 0;
public:
    MonWindow ()
  {
    printFormat = EVENT_PRINT_FULL;
    statusBegin = 1;
  }

  void postEvent (Rts2Event * event);
  void printTimeDiff (WINDOW * window, int row, char *text, double diff);

  void setStatusBegin (int in_status_begin)
  {
    statusBegin = in_status_begin;
  }
};

int
MonWindow::printStatus (WINDOW * window, Rts2Conn * connection)
{
  int nrow;
  nrow = statusBegin;
  for (int i = 0; i < MAX_STATE; i++)
    {
      Rts2ServerState *state = connection->getStateObject (i);
      if (state)
	{
	  mvwprintw (window, nrow, 0, "%10s: %-5i", state->name,
		     state->getValue ());
	  nrow++;
	}
    }
  return 0;
}

void
MonWindow::postEvent (Rts2Event * event)
{
  WINDOW *window;
  switch (event->getType ())
    {
    case EVENT_PRINT:
      window = (WINDOW *) event->getArg ();
      switch (printFormat)
	{
	case EVENT_PRINT_SHORT:
	  printOneLine (window);
	  break;
	case EVENT_PRINT_FULL:
	  print (window);
	  break;
	case EVENT_PRINT_MESSAGES:
	  break;
	}
      break;
    case EVENT_PRINT_SHORT:
    case EVENT_PRINT_FULL:
    case EVENT_PRINT_MESSAGES:
      printFormat = event->getType ();
    }
}

void
MonWindow::printTimeDiff (WINDOW * window, int row, char *text, double diff)
{
  char time_buf[50];
  struct ln_hms hms;
  time_buf[0] = '\0';
  double t = fabs (diff);
  if (t > 86400)
    {
      sprintf (time_buf, "%li days ", (long) (t / 86400));
      t = (long) t % 86400;
    }
  // that will delete sign..
  ln_deg_to_hms (360.0 * ((double) fabs (t) / 86400.0), &hms);
  sprintf (time_buf, "%s%c%i:%02i:%04.1f", time_buf, diff > 0 ? '+' : '-',
	   hms.hours, hms.minutes, hms.seconds);
  mvwprintw (window, row, 1, "%s:%s", text, time_buf);
}

class Rts2NMTelescope:public Rts2DevClientTelescopeImage, public MonWindow
{
protected:
  virtual void print (WINDOW * wnd);
  virtual void printOneLine (WINDOW * wnd);
public:
    Rts2NMTelescope (Rts2CNMonConn *
		     in_connection):Rts2DevClientTelescopeImage
    (in_connection)
  {
    setStatusBegin (9);
  }
  virtual void postEvent (Rts2Event * event)
  {
    MonWindow::postEvent (event);
    Rts2DevClientTelescopeImage::postEvent (event);
  }
};

void
Rts2NMTelescope::print (WINDOW * wnd)
{
  struct ln_hrz_posn altaz;
  struct ln_hms hms;
  double lst;

  struct ln_equ_posn tar, tel;
  struct ln_lnlat_posn obs;

  getEquTar (&tar);
  getEquTel (&tel);
  getAltAz (&altaz);
  getObs (&obs);

  lst = getLocalSiderealDeg ();

  mvwprintw (wnd, 1, 1, "Typ: %-10s", getValueChar ("type"));
  mvwprintw (wnd, 2, 1, "R+D/f: %07.3f%+06.3f/%c",
	     tar.ra, tar.dec, getValueInteger ("flip") ? 'f' : 'n');
  if (getValueInteger ("know_position"))
    mvwprintw (wnd, 3, 1, "Err: %.2f %.2f", getValueDouble ("ra_corr") * 60.0,
	       getValueDouble ("dec_corr") * 60.0);
  else
    mvwprintw (wnd, 3, 1, "R+D/f: %07.3f%+06.3f/%c",
	       tel.ra, tel.dec, getValueInteger ("flip") ? 'f' : 'n');
  mvwprintw (wnd, 4, 1, "Az/Al/D: %03.0f %+02.0f %s", altaz.az, altaz.alt,
	     ln_hrz_to_nswe (&altaz));

  mvwprintw (wnd, 5, 1, "x/y: %.0f %.0f", getValueDouble ("axis0_counts"),
	     getValueDouble ("axis1_counts"));
  mvwprintw (wnd, 6, 1, "Lon/Lat: %+03.3f %+03.3f",
	     getValueDouble ("longtitude"), getValueDouble ("latitude"));

  ln_rad_to_hms (ln_deg_to_rad (lst), &hms);
  mvwprintw (wnd, 7, 1, "Lsid: %07.3f (%02i:%02i:%02.1f)",
	     getValueDouble ("siderealtime"), hms.hours, hms.minutes,
	     hms.seconds);

  mvwprintw (wnd, 8, 1, "Corr: %i Exec: %i",
	     getValueInteger ("correction_mark"),
	     getValueInteger ("num_corr"));

  printStatus (wnd, connection);
}

void
Rts2NMTelescope::printOneLine (WINDOW * wnd)
{
  struct ln_hrz_posn hrz;
  getAltAz (&hrz);
  mvwprintw (wnd, 0, 0,
	     "Ra %07.3f Dec %+06.3f Flip %c Alt %f Az %f Status %i",
	     getValueDouble ("ra_tel"), getValueDouble ("dec_tel"),
	     getValueInteger ("flip") ? 'f' : 'n', hrz.alt, hrz.az,
	     getStatus (0));
}

class Rts2NMCamera:public Rts2DevClientCamera, public MonWindow
{
protected:
  virtual void print (WINDOW * wnd);
  virtual void printOneLine (WINDOW * wnd);
public:
    Rts2NMCamera (Rts2CNMonConn *
		  in_connection):Rts2DevClientCamera (in_connection)
  {
    setStatusBegin (8);
  }
  virtual void postEvent (Rts2Event * event)
  {
    MonWindow::postEvent (event);
    Rts2DevClientCamera::postEvent (event);
  }
};

void
Rts2NMCamera::print (WINDOW * wnd)
{
  mvwprintw (wnd, 1, 1, "Typ: %-10s", getValueChar ("type"));
  mvwprintw (wnd, 2, 1, "Ser: %-10s", getValueChar ("serial"));
  mvwprintw (wnd, 3, 1, "Exp: %.2f %i Gain: %.2f",
	     getValueDouble ("exposure"), getValueInteger ("shutter"),
	     getValueDouble ("gain"));
/*  if (info->chip_info)
    mvwprintw (wnd, 3, 1, "Siz: [%ix%i]", info->chip_info[0].width,
	       info->chip_info[0].height);
  else
    mvwprintw (wnd, 3, 1, "Siz: Unknow"); */
  mvwprintw (wnd, 4, 1, "S/A: %+05.1f %+05.1f oC",
	     getValueDouble ("temperature_setpoint"),
	     getValueDouble ("air_temperature"));
  mvwprintw (wnd, 5, 1, "CCD: %+05.1f oC",
	     getValueDouble ("ccd_temperature"));
  mvwprintw (wnd, 6, 1, "CPo: %03.0f Foc: %i",
	     getValueDouble ("cooling_power") / 10.0,
	     getValueInteger ("focpos"));
  mvwprintw (wnd, 7, 1, "Fan: %s Fil: %i",
	     getValueDouble ("fan") ? "on " : "off",
	     getValueInteger ("filter"));

  printStatus (wnd, connection);
}

void
Rts2NMCamera::printOneLine (WINDOW * wnd)
{
  mvwprintw (wnd, 0, 0,
	     "Exp: %.2f CCD %+05.1f oC Air %+05.1f oC Set %+05.1f Status ",
	     getValueDouble ("exposure"), getValueDouble ("ccd_temperature"),
	     getValueDouble ("air_temperature"),
	     getValueDouble ("temperature_setpoint"), getStatus (0));
}

class Rts2NMFocus:public Rts2DevClientFocus, public MonWindow
{
protected:
  virtual void print (WINDOW * wnd);
  virtual void printOneLine (WINDOW * wnd);
public:
    Rts2NMFocus (Rts2CNMonConn *
		 in_connection):Rts2DevClientFocus (in_connection)
  {
    setStatusBegin (6);
  }
  virtual void postEvent (Rts2Event * event)
  {
    MonWindow::postEvent (event);
    Rts2DevClientFocus::postEvent (event);
  }
};

void
Rts2NMFocus::print (WINDOW * wnd)
{
  mvwprintw (wnd, 1, 1, "Typ: %-10s", getValueChar ("type"));
  mvwprintw (wnd, 2, 1, "Ser: %-10s", getValueChar ("serial"));
  mvwprintw (wnd, 3, 1, "Temp: %+05.1f oC", getValueDouble ("temp"));
  mvwprintw (wnd, 4, 1, "Pos: %+i", getValueInteger ("pos"));
  int switch_num = getValueInteger ("switch_num");
  if (switch_num > 0)
    {
      int switches = getValueInteger ("switches");
      mvwprintw (wnd, 5, 1, "Switch: ");
      for (int i = 0; i < switch_num; i++)
	{
	  mvwprintw (wnd, 5, 9 + i, (switches & (1 << i) ? "O" : "f"));
	}
    }

  printStatus (wnd, connection);
}

void
Rts2NMFocus::printOneLine (WINDOW * wnd)
{
  mvwprintw (wnd, 0, 0, "Pos %+i Temp %+05.1f oC", getValueInteger ("pos"),
	     getValueDouble ("temp"));
}

class Rts2NMPhot:public Rts2DevClientPhot, public MonWindow
{
protected:
  virtual void print (WINDOW * wnd);
  virtual void printOneLine (WINDOW * wnd);
public:
    Rts2NMPhot (Rts2CNMonConn *
		in_connection):Rts2DevClientPhot (in_connection)
  {
    setStatusBegin (5);
  }
  virtual void postEvent (Rts2Event * event)
  {
    MonWindow::postEvent (event);
    Rts2DevClientPhot::postEvent (event);
  }
};

void
Rts2NMPhot::print (WINDOW * wnd)
{
  mvwprintw (wnd, 1, 1, "Typ: %-10s", getValueChar ("type"));
  mvwprintw (wnd, 2, 1, "Ser: %-10s", getValueChar ("serial"));
  mvwprintw (wnd, 3, 1, "fil: %i", getValueInteger ("filter"));
  mvwprintw (wnd, 4, 1, "cnt: %i (%0.3f)", lastCount, lastExp);

  printStatus (wnd, connection);
}

void
Rts2NMPhot::printOneLine (WINDOW * wnd)
{
  mvwprintw (wnd, 0, 0, "Cnt %i (%0.3f) Fil %i Status %i",
	     lastCount, lastExp, getValueInteger ("filter"), getStatus (0));
}

class Rts2NMFilter:public Rts2DevClientFilter, public MonWindow
{
protected:
  virtual void print (WINDOW * wnd);
  virtual void printOneLine (WINDOW * wnd);
public:
    Rts2NMFilter (Rts2CNMonConn *
		  in_connection):Rts2DevClientFilter (in_connection)
  {
    setStatusBegin (4);
  }
  virtual void postEvent (Rts2Event * event)
  {
    MonWindow::postEvent (event);
    Rts2DevClientFilter::postEvent (event);
  }
};

void
Rts2NMFilter::print (WINDOW * wnd)
{
  mvwprintw (wnd, 1, 1, "Typ: %-10s", getValueChar ("type"));
  mvwprintw (wnd, 2, 1, "Ser: %-10s", getValueChar ("serial"));
  mvwprintw (wnd, 3, 1, "fil: %i", getValueInteger ("filter"));

  printStatus (wnd, connection);
}

void
Rts2NMFilter::printOneLine (WINDOW * wnd)
{
  mvwprintw (wnd, 0, 0, "Fil %i Status %i",
	     getValueInteger ("filter"), getStatus (0));
}

class Rts2NMMirror:public Rts2DevClientMirror, public MonWindow
{
protected:
  virtual void print (WINDOW * wnd);
  virtual void printOneLine (WINDOW * wnd);
public:
    Rts2NMMirror (Rts2CNMonConn *
		  in_connection):Rts2DevClientMirror (in_connection)
  {
    setStatusBegin (1);
  }
  virtual void postEvent (Rts2Event * event)
  {
    MonWindow::postEvent (event);
    Rts2DevClientMirror::postEvent (event);
  }
};

void
Rts2NMMirror::print (WINDOW * wnd)
{
  printStatus (wnd, connection);
}

void
Rts2NMMirror::printOneLine (WINDOW * wnd)
{
  mvwprintw (wnd, 0, 0, "Status %i", getStatus (0));
}


class Rts2NMAuger:public Rts2DevClientAugerShooter, public MonWindow
{
protected:
  virtual void print (WINDOW * wnd);
  virtual void printOneLine (WINDOW * wnd);
public:
    Rts2NMAuger (Rts2CNMonConn *
		 in_connection):Rts2DevClientAugerShooter (in_connection)
  {
    setStatusBegin (2);
  }
  virtual void postEvent (Rts2Event * event)
  {
    MonWindow::postEvent (event);
    Rts2DevClientAugerShooter::postEvent (event);
  }
};

void
Rts2NMAuger::print (WINDOW * wnd)
{
  mvwprintw (wnd, 1, 1, "Last packet: %i", getValueInteger ("last_packet"));

  printStatus (wnd, connection);
}

void
Rts2NMAuger::printOneLine (WINDOW * wnd)
{
  mvwprintw (wnd, 0, 0, "Last packet: %i", getValueInteger ("last_packet"));
}

class Rts2NMDome:public Rts2DevClientDome, public MonWindow
{
protected:
  virtual void print (WINDOW * wnd);
  virtual void printOneLine (WINDOW * wnd);
public:
    Rts2NMDome (Rts2CNMonConn *
		in_connection):Rts2DevClientDome (in_connection)
  {
    setStatusBegin (9);
  }
  virtual void postEvent (Rts2Event * event)
  {
    MonWindow::postEvent (event);
    Rts2DevClientDome::postEvent (event);
  }
};

void
Rts2NMDome::print (WINDOW * wnd)
{
  int dome = getValueInteger ("dome");
  time_t now;
  long time_to_open;
  double temp;
  double humi;
  double vapor;
  double dew;
  char *ignoreString;
  time (&now);
  time_to_open = getValueInteger ("next_open") - now;

  temp = getValueDouble ("temperature");
  humi = getValueDouble ("humidity");

  if (isnan (temp) || isnan (humi))
    {
      vapor = nan ("f");
      dew = nan ("f");
    }
  else
    {
      vapor = (humi / 100) * 0.611 * exp (17.27 * temp / (temp + 237.3));
      dew =
	ceil (10 * ((116.9 + 237.3 * log (vapor)) / (16.78 - log (vapor)))) /
	10;
    }

  mvwprintw (wnd, 1, 1, "Mod: %s", getValueChar ("type"));
  mvwprintw (wnd, 2, 1, "Tem: %+4.1f oC Vap: %4.1f", temp, vapor);
  mvwprintw (wnd, 3, 1, "Hum: %3.0f dew: %+4.1f", humi, dew);
  mvwprintw (wnd, 4, 1, "Wind: %4.1f rain:%i Cl: %2.1f",
	     getValueDouble ("windspeed"), getValueInteger ("rain"),
	     getValueDouble ("cloud"));
  printTimeDiff (wnd, 5, "NextO", time_to_open);
#define is_on(num)	((dome & (1 << num))? 'O' : 'f')
  mvwprintw (wnd, 6, 1, "Open sw: %c %c", is_on (0), is_on (1));
  mvwprintw (wnd, 7, 1, "Close s: %c %c", is_on (2), is_on (3));
  switch (getValueInteger ("ignoreMeteo"))
    {
    case 2:
      ignoreString = "METEO OVERRIDE";
      if (has_colors ())
	wcolor_set (wnd, CLR_WARNING, NULL);
      else
	wstandout (wnd);
      break;
    case 1:
      if (has_colors ())
	wcolor_set (wnd, CLR_OK, NULL);
      else
	wstandout (wnd);
      ignoreString = "meteo active";
      break;
    default:
      if (has_colors ())
	wcolor_set (wnd, CLR_WARNING, NULL);
      else
	wstandout (wnd);
      ignoreString = "ignore unknow";
      break;
    }
  mvwprintw (wnd, 8, 1, "%s", ignoreString);

  if (has_colors ())
    wcolor_set (wnd, CLR_TEXT, NULL);
  else
    wstandend (wnd);


  printStatus (wnd, connection);
}

void
Rts2NMDome::printOneLine (WINDOW * wnd)
{
  int dome = getValueInteger ("dome");
  time_t now;
  long time_to_open;
  double temp;
  double humi;
  double vapor;
  double dew;
  time (&now);
  time_to_open = getValueInteger ("next_open") - now;

  temp = getValueDouble ("temperature");
  humi = getValueDouble ("humidity");

  if (isnan (temp) || isnan (humi))
    {
      vapor = nan ("f");
      dew = nan ("f");
    }
  else
    {
      vapor = (humi / 100) * 0.611 * exp (17.27 * temp / (temp + 237.3));
      dew =
	ceil (10 * ((116.9 + 237.3 * log (vapor)) / (16.78 - log (vapor)))) /
	10;
    }

  mvwprintw (wnd, 0, 0,
	     "Temp %+2.2f oC Humi %2.2f Dew %+2.2f oC Wind %4.1f Rain %i Open sw %c %c Close sw %c %c Status %i",
	     temp, humi, dew, getValueDouble ("windspeed"),
	     getValueInteger ("rain"), is_on (0), is_on (1), is_on (2),
	     is_on (3), getStatus (0));
#undef is_on
}

class Rts2NMCopula:public Rts2DevClientCopula, public MonWindow
{
protected:
  virtual void print (WINDOW * wnd);
  virtual void printOneLine (WINDOW * wnd);
public:
    Rts2NMCopula (Rts2CNMonConn * in_connection):Rts2DevClientCopula
    (in_connection)
  {
    setStatusBegin (9);
  }
  virtual void postEvent (Rts2Event * event)
  {
    MonWindow::postEvent (event);
    Rts2DevClientCopula::postEvent (event);
  }
};

void
Rts2NMCopula::print (WINDOW * wnd)
{
  int dome = getValueInteger ("dome");
  time_t now;
  long time_to_open;
  time (&now);
  time_to_open = getValueInteger ("next_open") - now;
  mvwprintw (wnd, 1, 1, "Mod: %s", getValueChar ("type"));
  mvwprintw (wnd, 2, 1, "Tem: %+4.1f oC", getValueDouble ("temperature"));
  mvwprintw (wnd, 3, 1, "Hum: %3.0f %", getValueDouble ("humidity"));
  mvwprintw (wnd, 4, 1, "Wind: %4.1f rain:%i", getValueDouble ("windspeed"),
	     getValueInteger ("rain"));
  printTimeDiff (wnd, 5, "NextO", time_to_open);
#define is_on(num)	((dome & (1 << num))? 'O' : 'f')
  mvwprintw (wnd, 6, 1, "Open sw: %c %c", is_on (0), is_on (1));
  mvwprintw (wnd, 7, 1, "Close s: %c %c", is_on (2), is_on (3));
  mvwprintw (wnd, 8, 1, "Az: %06.2f", getValueDouble ("az"));

  printStatus (wnd, connection);
}

void
Rts2NMCopula::printOneLine (WINDOW * wnd)
{
  int dome = getValueInteger ("dome");
  time_t now;
  long time_to_open;
  double temp;
  double humi;
  double vapor;
  double dew;
  time (&now);
  time_to_open = getValueInteger ("next_open") - now;

  temp = getValueDouble ("temperature");
  humi = getValueDouble ("humidity");

  if (isnan (temp) || isnan (humi))
    {
      vapor = nan ("f");
      dew = nan ("f");
    }
  else
    {
      vapor = (humi / 100) * 0.611 * exp (17.27 * temp / (temp + 237.3));
      dew =
	ceil (10 * ((116.9 + 237.3 * log (vapor)) / (16.78 - log (vapor)))) /
	10;
    }

  mvwprintw (wnd, 0, 0,
	     "Az %06.2f Temp %+2.2f oC Humi %2.2f Dew %+2.2f oC Wind %4.1f Rain %i Open sw %c %c Close sw %c %c Status %i",
	     getValueDouble ("az"), temp, humi, dew,
	     getValueDouble ("windspeed"), getValueInteger ("rain"),
	     is_on (0), is_on (1), is_on (2), is_on (3), getStatus (0));
#undef is_on
}

class Rts2NMExecutor:public Rts2DevClientExecutor, public MonWindow
{
protected:
  virtual void print (WINDOW * wnd);
  virtual void printOneLine (WINDOW * wnd);
public:
    Rts2NMExecutor (Rts2CNMonConn * in_connection):Rts2DevClientExecutor
    (in_connection)
  {
    setStatusBegin (8);
  }
  virtual void postEvent (Rts2Event * event)
  {
    MonWindow::postEvent (event);
    Rts2DevClientExecutor::postEvent (event);
  }
};

void
Rts2NMExecutor::print (WINDOW * wnd)
{
  mvwprintw (wnd, 1, 1, "Curr: %i (%i)", getValueInteger ("current"),
	     getValueInteger ("current_sel"));
  mvwprintw (wnd, 2, 1, "Next: %i", getValueInteger ("next"));
  mvwprintw (wnd, 3, 1, "Prio: %i", getValueInteger ("priority_target"));
  mvwprintw (wnd, 4, 1, "ObsI: %i", getValueInteger ("obsid"));
  mvwprintw (wnd, 5, 1, "ScCo: %i", getValueInteger ("script_count"));
  mvwprintw (wnd, 6, 1, "Acq OK: %i", getValueInteger ("acqusition_ok"));
  mvwprintw (wnd, 7, 1, "Acq Fa: %i", getValueInteger ("acqusition_failed"));

  printStatus (wnd, connection);
}

void
Rts2NMExecutor::printOneLine (WINDOW * wnd)
{
  mvwprintw (wnd, 0, 0, "Cur %i (%i) Next %i Prio %i ObsI %i Status %i",
	     getValueInteger ("current"), getValueInteger ("current_sel"),
	     getValueInteger ("next"), getValueInteger ("priority_target"),
	     getValueInteger ("obsid"), getStatus (0));
}

class Rts2NMSelector:public Rts2DevClientSelector, public MonWindow
{
protected:
  virtual void print (WINDOW * wnd);
  virtual void printOneLine (WINDOW * wnd);
public:
    Rts2NMSelector (Rts2CNMonConn * in_connection):Rts2DevClientSelector
    (in_connection)
  {
    setStatusBegin (1);
  }
  virtual void postEvent (Rts2Event * event)
  {
    MonWindow::postEvent (event);
    Rts2DevClientSelector::postEvent (event);
  }
};

void
Rts2NMSelector::print (WINDOW * wnd)
{
  printStatus (wnd, connection);
}

void
Rts2NMSelector::printOneLine (WINDOW * wnd)
{
  mvwprintw (wnd, 0, 0, "Status %i", getStatus (0));
}

class Rts2NMImgproc:public Rts2DevClientImgproc, public MonWindow
{
protected:
  virtual void print (WINDOW * wnd);
  virtual void printOneLine (WINDOW * wnd);
public:
    Rts2NMImgproc (Rts2CNMonConn * in_connection):Rts2DevClientImgproc
    (in_connection)
  {
    setStatusBegin (5);
  }
  virtual void postEvent (Rts2Event * event)
  {
    MonWindow::postEvent (event);
    Rts2DevClientImgproc::postEvent (event);
  }
};

void
Rts2NMImgproc::print (WINDOW * wnd)
{
  mvwprintw (wnd, 1, 1, "Que : %-5i", getValueInteger ("que_size"));
  mvwprintw (wnd, 2, 1, "Good: %i", getValueInteger ("good_images"));
  mvwprintw (wnd, 3, 1, "Trash: %i", getValueInteger ("trash_images"));
  mvwprintw (wnd, 4, 1, "Morn: %i", getValueInteger ("morning_images"));

  printStatus (wnd, connection);
}

void
Rts2NMImgproc::printOneLine (WINDOW * wnd)
{
  mvwprintw (wnd, 0, 0, "Que %i Good %i Trash %i Morn %i",
	     getValueInteger ("que_size"), getValueInteger ("good_images"),
	     getValueInteger ("trash_images"),
	     getValueInteger ("morning_images"));
}

class Rts2NMGrb:public Rts2DevClientGrb, public MonWindow
{
protected:
  virtual void print (WINDOW * wnd);
  virtual void printOneLine (WINDOW * wnd);
public:
    Rts2NMGrb (Rts2CNMonConn * in_connection):Rts2DevClientGrb (in_connection)
  {
    setStatusBegin (6);
  }
  virtual void postEvent (Rts2Event * event)
  {
    MonWindow::postEvent (event);
    Rts2DevClientGrb::postEvent (event);
  }
};

void
Rts2NMGrb::print (WINDOW * wnd)
{
  time_t now;
  time (&now);
  long last_pack = (int) getValueDouble ("last_packet") - now;
  printTimeDiff (wnd, 1, "L Pac", last_pack);
  mvwprintw (wnd, 2, 1, "Delta: %-5i", getValueDouble ("delta"));
  mvwprintw (wnd, 3, 1, "L Tar: %5", getValueChar ("last_target"));
  mvwprintw (wnd, 4, 1, "LTime: %5f", getValueDouble ("last_target_time"));
  mvwprintw (wnd, 5, 1, "Exec_link: %s",
	     getValueInteger ("exec") ? "OK" : "failed");

  printStatus (wnd, connection);
}

void
Rts2NMGrb::printOneLine (WINDOW * wnd)
{
  time_t now;
  time (&now);
  long last_pack = (int) getValueDouble ("last_packet") - now;
  printTimeDiff (wnd, 0, "L Pac", last_pack);
}

// here begins nmonitor common part

void
Rts2CNMonConn::print ()
{
  if (has_colors ())
    wcolor_set (window, CLR_OK, NULL);
  else
    wstandout (window);

  werase (window);
  mvwprintw (window, 0, 0, "== %-10s ==", getName ());

  if (has_colors ())
    wcolor_set (window, CLR_TEXT, NULL);
  else
    wstandend (window);

  if (!otherDevice)
    {
      mvwprintw (window, 1, 0, "  UNKNOW DEV");
    }
  else
    {
      // print values
      otherDevice->postEvent (new Rts2Event (EVENT_PRINT, (void *) window));
    }
  wrefresh (window);
}

/*******************************************************
 *
 * This class represent window with messages.
 * 
 ******************************************************/

class MessageWindow:public Rts2Object
{
private:
  WINDOW * wnd;
  bool printWindow;
public:

    MessageWindow ()
  {
    wnd = newwin (LINES - 2, COLS - 2, 1, 1);
    scrollok (wnd, true);
    printWindow = false;
  }

  virtual ~ MessageWindow (void)
  {
    delwin (wnd);
  }

  virtual void postEvent (Rts2Event * event);
  virtual void message (Rts2Message & msg);
};

void
MessageWindow::postEvent (Rts2Event * event)
{
  switch (event->getType ())
    {
    case EVENT_PRINT:
      if (printWindow)
	wrefresh (wnd);
      break;
    case EVENT_PRINT_MESSAGES:
      printWindow = true;
      break;
    case EVENT_PRINT_SHORT:
    case EVENT_PRINT_FULL:
      printWindow = false;
      break;
    }
  Rts2Object::postEvent (event);
}

void
MessageWindow::message (Rts2Message & msg)
{
  waddstr (wnd, msg.toConn ().c_str ());
  waddstr (wnd, "\n");
  wrefresh (wnd);
}

/*******************************************************
 *
 * This class hold "root" window of display,
 * takes care about displaying it's connection etc..
 *
 ******************************************************/

class Rts2NMonitor:public Rts2Client
{
private:
  WINDOW * statusWindow;
  WINDOW *commandWindow;
    std::vector < Rts2CNMonConn * >clientConnections;
  int connCols;
  int connLines;
  int cmd_col;
  char cmd_buf[CMD_BUF_LEN];

  void executeCommand ();
  void relocatesWindows ();
  void processConnection (Rts2CNMonConn * conn);

  int paintWindows ();
  int repaint ();

  int messageBox (char *message);

  MessageWindow *messageWindow;

  void refreshCommandWindow ()
  {
    mvwprintw (commandWindow, 0, 0, "%-*s", COLS, cmd_buf);
    wrefresh (commandWindow);
    repaint ();
  }

protected:
    virtual Rts2ConnClient * createClientConnection (char *in_deviceName)
  {
    Rts2CNMonConn *conn;
    conn = new Rts2CNMonConn (this, in_deviceName);
    processConnection (conn);
    return conn;
  }

public:
  Rts2NMonitor (int argc, char **argv);
  virtual ~ Rts2NMonitor (void);

  virtual int init ();
  virtual int idle ();

  virtual void postEvent (Rts2Event * event);

  virtual int deleteConnection (Rts2Conn * conn);

  virtual Rts2DevClient *createOtherType (Rts2Conn * conn,
					  int other_device_type);

  virtual int willConnect (Rts2Address * in_addr);
  virtual int addAddress (Rts2Address * in_addr);

  virtual void message (Rts2Message & msg)
  {
    messageWindow->message (msg);
  }

  int resize ();

  void processKey (int key);
};

void
Rts2NMonitor::executeCommand ()
{
  char *cmd_sep;
  Rts2Conn *cmd_conn;
  // try to parse..
  cmd_sep = index (cmd_buf, '.');
  if (cmd_sep)
    {
      *cmd_sep = '\0';
      cmd_sep++;
      cmd_conn = getConnection (cmd_buf);
    }
  else
    {
      cmd_conn = getCentraldConn ();
      cmd_sep = cmd_buf;
    }
  cmd_conn->queCommand (new Rts2Command (this, cmd_sep));
  cmd_col = 0;
  cmd_buf[0] = '\0';
  refreshCommandWindow ();
}

void
Rts2NMonitor::relocatesWindows ()
{
  int win_num;
  WINDOW *connWin;
  std::vector < Rts2CNMonConn * >::iterator conn_iter;
  Rts2CNMonConn *conn;
  win_num = clientConnections.size ();

  // allocate window for our connection
  for (conn_iter = clientConnections.begin ();
       conn_iter != clientConnections.end (); conn_iter++)
    {
      conn = (*conn_iter);
      if (!conn->hasWindow ())
	{
	  connWin =
	    newwin (LINES / 4, COLS / 4, 1 + LINES / 4 * ((win_num - 1) / 4),
		    COLS / 4 * ((win_num - 1) % 4));
	  conn->setWindow (connWin);
	}
    }
}

void
Rts2NMonitor::processConnection (Rts2CNMonConn * conn)
{
  clientConnections.push_back (conn);
  relocatesWindows ();
}

Rts2NMonitor::Rts2NMonitor (int in_argc, char **in_argv):
Rts2Client (in_argc, in_argv)
{
  statusWindow = NULL;
  commandWindow = NULL;
  connCols = -1;
  connLines = 0;
  cmd_col = 0;
}

Rts2NMonitor::~Rts2NMonitor (void)
{
  if (connCols >= 0)
    {
      delwin (statusWindow);
      delwin (commandWindow);
    }
  delete messageWindow;
}

int
Rts2NMonitor::paintWindows ()
{
  curs_set (0);
  // prepare main window..
  if (statusWindow)
    delwin (statusWindow);
  if (commandWindow)
    delwin (commandWindow);
  statusWindow = newwin (1, COLS, LINES - 1, 0);
  wbkgdset (statusWindow, A_BOLD | COLOR_PAIR (CLR_STATUS));
  commandWindow = newwin (1, COLS, 0, 0);
  wbkgdset (commandWindow, A_BOLD | COLOR_PAIR (CLR_STATUS));
  waddch (commandWindow, 'C');
  curs_set (1);
  return 0;
}

int
Rts2NMonitor::repaint ()
{
  char dateBuf[40];
  time_t now;
  char stateBuf[20];
  time (&now);

  wcolor_set (statusWindow, CLR_STATUS, NULL);

  getMasterState (stateBuf);

  mvwprintw (statusWindow, 0, 0, "** Status: %s ** ", stateBuf);
  wcolor_set (statusWindow, CLR_TEXT, NULL);
  strftime (dateBuf, 40, "%Y-%m-%dT%H:%M:%S", gmtime (&now));
  mvwprintw (statusWindow, 0, COLS - 40, "%40s", dateBuf);
  refresh ();
  wrefresh (statusWindow);
  wrefresh (commandWindow);
  return 0;
}

int
Rts2NMonitor::init ()
{
  int ret;
  ret = Rts2Client::init ();
  if (ret)
    return ret;

  if (!initscr ())
    {
      std::cerr << "ncurses not supported (initscr call failed)!" << std::
	endl;
      return -1;
    }

  connCols = COLS / COL_SIZE;
  connLines = (LINES - 2) / LINE_SIZE;

  cbreak ();
  keypad (stdscr, TRUE);
  noecho ();

  start_color ();

  if (has_colors ())
    {
      init_pair (CLR_OK, COLOR_GREEN, 0);
      init_pair (CLR_TEXT, COLOR_WHITE, 0);
      init_pair (CLR_PRIORITY, COLOR_CYAN, 0);
      init_pair (CLR_WARNING, COLOR_RED, 0);
      init_pair (CLR_FAILURE, COLOR_YELLOW, 0);
      init_pair (CLR_STATUS, COLOR_RED, COLOR_CYAN);
    }
  getCentraldConn ()->queCommand (new Rts2Command (this, "info"));
  setMessageMask (MESSAGE_MASK_ALL);
  paintWindows ();

  messageWindow = new MessageWindow ();

  return repaint ();
}

int
Rts2NMonitor::idle ()
{
  repaint ();
  setTimeout (USEC_SEC);
  return Rts2Client::idle ();
}

void
Rts2NMonitor::postEvent (Rts2Event * event)
{
  messageWindow->postEvent (new Rts2Event (event));
  Rts2Client::postEvent (event);
}

int
Rts2NMonitor::deleteConnection (Rts2Conn * conn)
{
  // try to find us among clientConnections
  std::vector < Rts2CNMonConn * >::iterator conn_iter;
  Rts2CNMonConn *tmp_conn;

  // allocate window for our connection
  for (conn_iter = clientConnections.begin ();
       conn_iter != clientConnections.end (); conn_iter++)
    {
      tmp_conn = (*conn_iter);
      if (tmp_conn == conn)
	{
	  clientConnections.erase (conn_iter);
	  break;
	}
    }
  return Rts2Client::deleteConnection (conn);
}

Rts2DevClient *
Rts2NMonitor::createOtherType (Rts2Conn * conn, int other_device_type)
{
  switch (other_device_type)
    {
    case DEVICE_TYPE_MOUNT:
      return new Rts2NMTelescope ((Rts2CNMonConn *) conn);
    case DEVICE_TYPE_CCD:
      return new Rts2NMCamera ((Rts2CNMonConn *) conn);
    case DEVICE_TYPE_FOCUS:
      return new Rts2NMFocus ((Rts2CNMonConn *) conn);
    case DEVICE_TYPE_PHOT:
      return new Rts2NMPhot ((Rts2CNMonConn *) conn);
    case DEVICE_TYPE_FW:
      return new Rts2NMFilter ((Rts2CNMonConn *) conn);
    case DEVICE_TYPE_MIRROR:
      return new Rts2NMMirror ((Rts2CNMonConn *) conn);
    case DEVICE_TYPE_AUGERSH:
      return new Rts2NMAuger ((Rts2CNMonConn *) conn);
    case DEVICE_TYPE_DOME:
      return new Rts2NMDome ((Rts2CNMonConn *) conn);
    case DEVICE_TYPE_COPULA:
      return new Rts2NMCopula ((Rts2CNMonConn *) conn);
    case DEVICE_TYPE_EXECUTOR:
      return new Rts2NMExecutor ((Rts2CNMonConn *) conn);
    case DEVICE_TYPE_SELECTOR:
      return new Rts2NMSelector ((Rts2CNMonConn *) conn);
    case DEVICE_TYPE_IMGPROC:
      return new Rts2NMImgproc ((Rts2CNMonConn *) conn);
    case DEVICE_TYPE_GRB:
      return new Rts2NMGrb ((Rts2CNMonConn *) conn);
    default:
      return Rts2Client::createOtherType (conn, other_device_type);
    }
}

int
Rts2NMonitor::resize ()
{
  std::vector < Rts2CNMonConn * >::iterator conn_iter;
  Rts2CNMonConn *conn;
  for (conn_iter = clientConnections.begin ();
       conn_iter != clientConnections.end (); conn_iter++)
    {
      conn = (*conn_iter);
      conn->setWindow (NULL);
    }
  erase ();
  endwin ();
  initscr ();
  relocatesWindows ();
  paintWindows ();
  postEvent (new Rts2Event (EVENT_PRINT));
  return repaint ();
}

void
Rts2NMonitor::processKey (int key)
{
  switch (key)
    {
    case KEY_F (2):
      if (messageBox ("Are you sure to switch off (y/n)?") == 'y')
	getCentraldConn ()->queCommand (new Rts2Command (this, "off"));
      break;
    case KEY_F (3):
      if (messageBox ("Are you sure to switch to standby (y/n)?") == 'y')
	getCentraldConn ()->queCommand (new Rts2Command (this, "standby"));
      break;
    case KEY_F (4):
      if (messageBox ("Are you sure to switch to on (y/n)?") == 'y')
	getCentraldConn ()->queCommand (new Rts2Command (this, "on"));
      break;
    case KEY_F (5):
      queAll ("info");
      break;
    case KEY_F (8):
      postEvent (new Rts2Event (EVENT_PRINT_MESSAGES));
      break;
    case KEY_F (9):
      postEvent (new Rts2Event (EVENT_PRINT_SHORT));
      break;
    case KEY_F (10):
      endRunLoop ();
      break;
    case KEY_BACKSPACE:
      if (cmd_col > 0)
	{
	  cmd_col--;
	  cmd_buf[cmd_col] = '\0';
	  refreshCommandWindow ();
	  break;
	}
      beep ();
      break;
    case KEY_ENTER:
    case '\n':
      executeCommand ();
      break;
    default:
      if (cmd_col >= CMD_BUF_LEN)
	{
	  beep ();
	  break;
	}
      cmd_buf[cmd_col++] = key;
      cmd_buf[cmd_col] = '\0';
      refreshCommandWindow ();
      break;
    }
}

int
Rts2NMonitor::willConnect (Rts2Address * in_addr)
{
  return 1;
}

int
Rts2NMonitor::addAddress (Rts2Address * in_addr)
{
  int ret;
  ret = Rts2Client::addAddress (in_addr);
  repaint ();
  return ret;
}

int
Rts2NMonitor::messageBox (char *msg)
{
  int key = 0;
  WINDOW *wnd = newwin (5, COLS / 2, LINES / 2, COLS / 2 - COLS / 4);
  PANEL *msg_pan = new_panel (wnd);
  wcolor_set (wnd, CLR_WARNING, NULL);
  box (wnd, 0, 0);
  wcolor_set (wnd, CLR_TEXT, NULL);
  mvwprintw (wnd, 2, 2 + (COLS / 2 - strlen (msg)) / 2, msg);
  wrefresh (wnd);
  key = wgetch (wnd);
  del_panel (msg_pan);
  delwin (wnd);
  update_panels ();
  doupdate ();
  return key;
}

Rts2NMonitor *monitor = NULL;

// signal and keyboard handlers..

void
sigExit (int sig)
{
  delete monitor;
  exit (0);
}

sighandler_t old_Winch;

void
sigWinch (int sig)
{
  if (old_Winch)
    old_Winch (sig);
  monitor->resize ();
}

void *
inputThread (void *arg)
{
  int key;
  while (1)
    {
      key = getch ();
      monitor->processKey (key);
    }
}

int
main (int argc, char **argv)
{
  int ret;
  pthread_t keyThread;
  pthread_attr_t keyThreadAttr;

  monitor = new Rts2NMonitor (argc, argv);

  signal (SIGINT, sigExit);
  signal (SIGTERM, sigExit);
  old_Winch = signal (SIGWINCH, sigWinch);

  ret = monitor->init ();
  if (ret)
    {
      delete monitor;
      exit (0);
    }
  pthread_attr_init (&keyThreadAttr);
  pthread_attr_setstacksize (&keyThreadAttr, PTHREAD_STACK_MIN * 6);
  pthread_create (&keyThread, &keyThreadAttr, inputThread, NULL);
  monitor->run ();

  delete (monitor);

  erase ();
  refresh ();

  nocbreak ();
  echo ();
  endwin ();

  return 0;
}
