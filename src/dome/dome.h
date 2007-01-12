#ifndef __RTS2_DOME__
#define __RTS2_DOME__

#include "../utils/rts2block.h"
#include "../utils/rts2device.h"

#define DEF_WEATHER_TIMEOUT	600

class Rts2DevDome:public Rts2Device
{
protected:
  char *domeModel;
  int sw_state;

  float temperature;
  float humidity;
  int power_telescope;
  int power_cameras;
  time_t nextOpen;
  int rain;
  float windspeed;
  double cloud;

  int observingPossible;
  int maxWindSpeed;
  int maxPeekWindspeed;
  bool weatherCanOpenDome;

  time_t nextGoodWeather;

  virtual int processOption (int in_opt);

  virtual void cancelPriorityOperations ()
  {
    // we don't want to get back to not-moving state if we were moving..so we don't request to reset our state
  }

  bool ignoreMeteo;

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
  virtual Rts2DevConn *createConnection (int in_sock);
  virtual int idle ();

  // callback function from dome connection
  virtual int sendBaseInfo (Rts2Conn * conn);
  virtual int sendInfo (Rts2Conn * conn);

  virtual int observing ();
  virtual int standby ();
  virtual int off ();

  int setMasterStandby ();
  int setMasterOn ();
  virtual int changeMasterState (int new_state);

  int setIgnoreMeteo (bool newIgnore)
  {
    ignoreMeteo = newIgnore;
    infoAll ();
    return 0;
  }

  void setTemperature (float in_temp)
  {
    temperature = in_temp;
  }
  float getTemperature ()
  {
    return temperature;
  }
  void setHumidity (float in_humidity)
  {
    humidity = in_humidity;
  }
  void setRain (int in_rain)
  {
    rain = in_rain;
  }
  void setWindSpeed (float in_windpseed)
  {
    windspeed = in_windpseed;
  }
  void setCloud (double in_cloud)
  {
    cloud = in_cloud;
  }
  void setWeatherTimeout (time_t wait_time);
  void setSwState (int in_sw_state)
  {
    sw_state = in_sw_state;
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
