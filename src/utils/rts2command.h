#ifndef __RTS2_COMMAND__
#define __RTS2_COMMAND__

#include "rts2block.h"

#define RTS2_COMMAND_REQUE	-5

typedef enum
{ EXP_LIGHT, EXP_DARK } exposureType;

typedef enum
{ NO_COND, NO_EXPOSURE, IN_WAIT_STATE } commandCondType;

class Rts2Command
{
protected:
  Rts2Block * owner;
  Rts2Conn *connection;
  char *text;
  commandCondType commandCond;
public:
    Rts2Command (Rts2Block * in_owner);
    Rts2Command (Rts2Block * in_owner, char *in_text);
    Rts2Command (Rts2Command * in_command)
  {
    owner = in_command->owner;
    connection = in_command->connection;
    setCommand (in_command->getText ());
    commandCond = in_command->getCommandCond ();
  }
  virtual ~ Rts2Command (void);
  int setCommand (char *in_text);
  void setConnection (Rts2Conn * conn)
  {
    connection = conn;
  }
  virtual int send ();
  int commandReturn (int status);
  char *getText ()
  {
    return text;
  }
  commandCondType getCommandCond ()
  {
    return commandCond;
  }
  virtual int commandReturnOK ();
  virtual int commandReturnFailed (int status);
};

class Rts2CentraldCommand:public Rts2Command
{

public:
  Rts2CentraldCommand (Rts2Block * in_owner,
		       char *in_text):Rts2Command (in_owner, in_text)
  {
  }
};

class Rts2CommandSendKey:public Rts2Command
{
private:
  int key;
public:
    Rts2CommandSendKey (Rts2Block * in_master, int in_key);
  virtual int send ();

  virtual int commandReturnOK ()
  {
    connection->setConnState (CONN_AUTH_OK);
    return -1;
  }
  virtual int commandReturnFailed ()
  {
    connection->setConnState (CONN_AUTH_FAILED);
    return -1;
  }
};

class Rts2CommandAuthorize:public Rts2Command
{
public:
  Rts2CommandAuthorize (Rts2Block * in_master, const char *device_name);
};

// devices commands

class Rts2CommandBinning:public Rts2Command
{
public:
  Rts2CommandBinning (Rts2Block * in_master, int binning_v, int binning_h);
};

class Rts2CommandExposure:public Rts2Command
{
private:
  Rts2DevClientCamera * camera;
public:
  Rts2CommandExposure (Rts2Block * in_master, Rts2DevClientCamera * in_camera,
		       exposureType exp_type, float exp_time);
  virtual int commandReturnFailed (int status);
};

class Rts2CommandFilter:public Rts2Command
{
public:
  Rts2CommandFilter (Rts2Block * in_master, int filter);
};

class Rts2CommandCenter:public Rts2Command
{
public:
  Rts2CommandCenter (Rts2Block * in_master, int chip, int width, int height);
};

class Rts2CommandMove:public Rts2Command
{
  Rts2DevClientTelescope *tel;
public:
    Rts2CommandMove (Rts2Block * in_master, Rts2DevClientTelescope * in_tel,
		     double ra, double dec);
  virtual int commandReturnFailed (int status);
};

class Rts2CommandChange:public Rts2Command
{
  Rts2DevClientTelescope *tel;
public:
    Rts2CommandChange (Rts2Block * in_master, double ra, double dec);
    Rts2CommandChange (Rts2CommandChange * in_command,
		       Rts2DevClientTelescope * in_tel);
  virtual int commandReturnFailed (int status);
};

class Rts2CommandCorrect:public Rts2Command
{
public:
  Rts2CommandCorrect (Rts2Block * in_master, int corr_mark, double ra,
		      double dec, double ra_err, double dec_err);
};

class Rts2CommandExecNext:public Rts2Command
{
public:
  Rts2CommandExecNext (Rts2Block * in_master, int next_id);
};

class Rts2CommandExecGrb:public Rts2Command
{
public:
  Rts2CommandExecGrb (Rts2Block * in_master, int grb_id);
};

class Rts2CommandKillAll:public Rts2Command
{
public:
  Rts2CommandKillAll (Rts2Block * in_master);
};

#endif /* !__RTS2_COMMAND__ */
