#ifndef __RTS2_CENTRALD__
#define __RTS2_CENTRALD__

#include "riseset.h"

#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <malloc.h>
#include <libnova/libnova.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <arpa/inet.h>

#include <config.h>
#include "../utils/rts2daemon.h"
#include "../utils/rts2config.h"
#include "status.h"

#ifndef HOST_NAME_MAX
#define HOST_NAME_MAX		255
#endif


class Rts2ConnCentrald;

// defined in rts2commandstatusinfo.h
class Rts2CommandStatusInfo;

class Rts2Centrald:public Rts2Daemon
{
private:
  int priority_client;

  int next_event_type;
  time_t next_event_time;
  struct ln_lnlat_posn *observer;

  int morning_off;
  int morning_standby;

  char *configFile;
    std::string logFile;
  // which sets logfile
  enum
  { LOGFILE_ARG, LOGFILE_DEF, LOGFILE_CNF } logFileSource;

    std::ofstream * fileLog;

  void openLog ();
  int reloadConfig ();

  int connNum;
protected:
  int changeState (int new_state, const char *user);

  virtual int processOption (int in_opt);

  // those callbacks are now empty; they can be used in future
  // to link two centrald to enable cooperative observation
  virtual Rts2Conn *createClientConnection (char *in_deviceName)
  {
    return NULL;
  }
  virtual Rts2Conn *createClientConnection (Rts2Address * in_addr)
  {
    return NULL;
  }

  virtual bool queValueChange (Rts2CondValue * old_value)
  {
    return false;
  }

  virtual int init ();

  virtual void connectionRemoved (Rts2Conn * conn);

public:
  Rts2Centrald (int in_argc, char **in_argv);
  virtual ~ Rts2Centrald (void);

  virtual int idle ();

  int changePriority (time_t timeout);

  int changeStateOn (const char *user)
  {
    return changeState ((next_event_type + 5) % 6, user);
  }
  int changeStateStandby (const char *user)
  {
    return changeState (SERVERD_STANDBY | ((next_event_type + 5) % 6), user);
  }
  int changeStateOff (const char *user)
  {
    return changeState (SERVERD_OFF, user);
  }
  inline int getPriorityClient ()
  {
    return priority_client;
  }

  virtual Rts2Conn *createConnection (int in_sock);
  void connAdded (Rts2ConnCentrald * added);
  Rts2Conn *getConnection (int conn_num);

  void sendMessage (messageType_t in_messageType,
		    const char *in_messageString);

  virtual void message (Rts2Message & msg);
  void processMessage (Rts2Message & msg)
  {
    sendMessageAll (msg);
  }

  virtual void signaledHUP ();

  void bopMaskChanged ();

  virtual int statusInfo (Rts2Conn * conn);
};

class Rts2ConnCentrald:public Rts2Conn
{
private:
  int authorized;
  char login[CLIENT_LOGIN_SIZE];
  Rts2Centrald *master;
  char hostname[HOST_NAME_MAX];
  int port;
  int device_type;

  int command ();
  int commandDevice ();
  int commandClient ();
  // command handling functions
  int priorityCommand ();
  int sendDeviceKey ();
  int sendInfo ();
  int sendStatusInfo ();
  int sendAValue (char *name, int value);
  int messageMask;
  Rts2CommandStatusInfo *statusCommand;
protected:
    virtual void setState (int in_value);
public:
    Rts2ConnCentrald (int in_sock, Rts2Centrald * in_master,
		      int in_centrald_id);
    virtual ~ Rts2ConnCentrald (void);
  virtual int sendMessage (Rts2Message & msg);
  virtual int sendInfo (Rts2Conn * conn);
  void deleteStatusCommand ();
  void setStatusCommand (Rts2CommandStatusInfo * cmd)
  {
    statusCommand = cmd;
  }
  void sendStatusCommand ();
  void updateStatusWait ();
};

#endif /*! __RTS2_CENTRALD__ */
