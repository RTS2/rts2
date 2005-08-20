#ifndef __RTS2_DOME__
#define __RTS2_DOME__

#include "../utils/rts2block.h"
#include "../utils/rts2device.h"

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

  int observingPossible;
  virtual void cancelPriorityOperations ()
  {
    // we don't want to get back to not-moving state if we were moving..so we don't request to reset our state
  }
public:
    Rts2DevDome (int argc, char **argv);
  virtual int openDome ()
  {
    maskState (0, DOME_DOME_MASK, DOME_OPENING, "opening dome");
    return 0;
  }
  virtual int endOpen ()
  {
    maskState (0, DOME_DOME_MASK, DOME_OPENED, "dome opened");
    return 0;
  };
  virtual long isOpened ()
  {
    return -2;
  };
  virtual int closeDome ()
  {
    maskState (0, DOME_DOME_MASK, DOME_CLOSING, "closing dome");
    return 0;
  };
  virtual int endClose ()
  {
    maskState (0, DOME_DOME_MASK, DOME_CLOSED, "dome closed");
    return 0;
  };
  virtual long isClosed ()
  {
    return -2;
  };
  int checkOpening ();
  virtual int init ();
  virtual Rts2DevConn *createConnection (int in_sock, int conn_num);
  virtual int idle ();

  virtual int ready ()
  {
    return -1;
  };
  virtual int baseInfo ()
  {
    return -1;
  };
  virtual int info ()
  {
    return -1;
  };

  // callback function from dome connection
  virtual int ready (Rts2Conn * conn);
  virtual int sendBaseInfo (Rts2Conn * conn);
  virtual int sendInfo (Rts2Conn * conn);

  virtual int observing ();
  virtual int standby ();
  virtual int off ();

  int setMasterStandby ();
  int setMasterOn ();
  virtual int changeMasterState (int new_state);

  void setTemperatur (float in_temp)
  {
    temperature = in_temp;
  }
  void setHumidity (float in_humidity)
  {
    humidity = in_humidity;
  }
  void setRain (int in_rain)
  {
    rain = in_rain;
  }
  void setSwState (int in_sw_state)
  {
    sw_state = in_sw_state;
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
