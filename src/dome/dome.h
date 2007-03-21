#ifndef __RTS2_DOME__
#define __RTS2_DOME__

#include "../utils/rts2block.h"
#include "../utils/rts2device.h"

#define DEF_WEATHER_TIMEOUT	600

class Rts2DevDome:public Rts2Device
{
private:
  Rts2ValueInteger * ignoreMeteo;

protected:
  char *domeModel;
  Rts2ValueInteger *sw_state;

  Rts2ValueFloat *temperature;
  Rts2ValueFloat *humidity;
  Rts2ValueTime *nextOpen;
  Rts2ValueInteger *rain;
  Rts2ValueFloat *windspeed;
  Rts2ValueDouble *cloud;

  Rts2ValueInteger *observingPossible;
  int maxWindSpeed;
  int maxPeekWindspeed;
  bool weatherCanOpenDome;

  time_t nextGoodWeather;

  virtual int processOption (int in_opt);

  virtual void cancelPriorityOperations ()
  {
    // we don't want to get back to not-moving state if we were moving..so we don't request to reset our state
  }

  void domeWeatherGood ();
  virtual int isGoodWeather ();
public:
  Rts2DevDome (int argc, char **argv, int in_device_type = DEVICE_TYPE_DOME);
  virtual int openDome ()
  {
    maskState (DOME_DOME_MASK, DOME_OPENING, "opening dome");
    return 0;
  }
  virtual int endOpen ()
  {
    maskState (DOME_DOME_MASK, DOME_OPENED, "dome opened");
    return 0;
  };
  virtual long isOpened ()
  {
    return -2;
  };
  int closeDomeWeather ();
  virtual int closeDome ()
  {
    maskState (DOME_DOME_MASK, DOME_CLOSING, "closing dome");
    return 0;
  };
  virtual int endClose ()
  {
    maskState (DOME_DOME_MASK, DOME_CLOSED, "dome closed");
    return 0;
  };
  virtual long isClosed ()
  {
    return -2;
  };
  int checkOpening ();
  virtual int init ();
  virtual int initValues ();
  virtual Rts2DevConn *createConnection (int in_sock);
  virtual int idle ();

  virtual int info ();

  // callback function from dome connection

  virtual int observing ();
  virtual int standby ();
  virtual int off ();

  int setMasterStandby ();
  int setMasterOn ();
  virtual int changeMasterState (int new_state);

  int setIgnoreMeteo (bool newIgnore)
  {
    ignoreMeteo->setValueInteger (newIgnore ? 2 : 1);
    infoAll ();
    return 0;
  }

  bool getIgnoreMeteo ()
  {
    if (ignoreMeteo->getValueInteger () == 2)
      return true;
    return false;
  }

  void setTemperature (float in_temp)
  {
    temperature->setValueFloat (in_temp);
  }
  float getTemperature ()
  {
    return temperature->getValueFloat ();
  }
  void setHumidity (float in_humidity)
  {
    humidity->setValueFloat (in_humidity);
  }
  float getHumidity ()
  {
    return humidity->getValueFloat ();
  }
  void setRain (int in_rain)
  {
    rain->setValueInteger (in_rain);
  }
  virtual void setRainWeather (int in_rain)
  {
    setRain (in_rain);
  }
  int getRain ()
  {
    return rain->getValueInteger ();
  }
  void setWindSpeed (float in_windpseed)
  {
    windspeed->setValueFloat (in_windpseed);
  }
  float getWindSpeed ()
  {
    return windspeed->getValueFloat ();
  }
  void setCloud (double in_cloud)
  {
    cloud->setValueDouble (in_cloud);
  }
  double getCloud ()
  {
    return cloud->getValueDouble ();
  }
  void setWeatherTimeout (time_t wait_time);
  void setSwState (int in_sw_state)
  {
    sw_state->setValueInteger (in_sw_state);
  }

  int getMaxPeekWindspeed ()
  {
    return maxPeekWindspeed;
  }

  int getMaxWindSpeed ()
  {
    return maxWindSpeed;
  }
  time_t getNextOpen ()
  {
    return nextGoodWeather;
  }
};

class Rts2DevConnDome:public Rts2DevConn
{
private:
  Rts2DevDome * master;
protected:
  virtual int commandAuthorized ();
public:
    Rts2DevConnDome (int in_sock,
		     Rts2DevDome * in_master_device):Rts2DevConn (in_sock,
								  in_master_device)
  {
    master = in_master_device;
  };
};

#endif /* ! __RTS2_DOME__ */
