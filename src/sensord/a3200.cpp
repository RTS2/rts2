#include "sensord.h"

#include <Windows.h>
#include <AerSys.h>

// when we'll have more then 3 axes, please change this
#define MAX_REQUESTED 3

class Rts2DevSensorA3200:public Rts2DevSensor
{
private:
  HAERCTRL hAerCtrl;
  AXISINDEX mAxis;

  Rts2ValueDouble *ax1;
  Rts2ValueDouble *ax2;
  Rts2ValueDouble *ax3;

  LPCTSTR initFile;

  void logErr (char *proc, AERERR_CODE eRc);

  int home ();
  int moveAxis (AXISINDEX ax, LONG tar);

protected:
    virtual int setValue (Rts2Value * old_value, Rts2Value * new_value);
  virtual int processOption (int in_opt);

public:
    Rts2DevSensorA3200 (int in_argc, char **in_argv);
    virtual ~ Rts2DevSensorA3200 (void);

  virtual int init ();
  virtual int info ();
};

void
Rts2DevSensorA3200::logErr (char *proc, AERERR_CODE eRc)
{
  TCHAR szMsg[MAX_TEXT_LEN];

  AerErrGetMessage (eRc, szMsg, MAX_TEXT_LEN, false);
  logStream (MESSAGE_ERROR) << "Cannot initialize A3200 " << szMsg << sendLog;
}

int
Rts2DevSensorA3200::home ()
{
  AERERR_CODE eRc;
  eRc = AerMoveMEnable (hAerCtrl, mAxis);
  if (eRc != AERERR_NOERR)
    {
      logErr ("home MoveMEnable", eRc);
      return -1;
    }
  eRc = AerMoveMWaitDone (hAerCtrl, mAxis, 100, 0);
  if (eRc != AERERR_NOERR)
    {
      logErr ("home MoveMWait for Enable", eRc);
      return -1;
    }
  logStream (MESSAGE_DEBUG) << "All axis enabled, homing" << sendLog;
  eRc = AerMoveMHome (hAerCtrl, mAxis);
  if (eRc != AERERR_NOERR)
    {
      logErr ("home MHome for Enable", eRc);
      return -1;
    }
  eRc = AerMoveMWaitDone (hAerCtrl, mAxis, 100, 0);
  if (eRc != AERERR_NOERR)
    {
      logErr ("home MoveMWait for Home", eRc);
      return -1;
    }
  logStream (MESSAGE_DEBUG) << "All axis homed properly" << sendLog;
  return 0;
}

int
Rts2DevSensorA3200::moveAxis (AXISINDEX ax, LONG tar)
{
  AERERR_CODE eRc;
  eRc = AerMoveAbsolute (hAerCtrl, ax, tar, 100000);
  if (eRc != AERERR_NOERR)
    {
      logErr ("moveAxis MoveAbsolute", eRc);
      return -1;
    }
  logStream (MESSAGE_DEBUG) << "Moving " << ax << " to " << tar << sendLog;
  eRc = AerMoveWaitDone (hAerCtrl, ax, 10000, 0);
  if (eRc != AERERR_NOERR)
    {
      logErr ("home MoveMWait for Home", eRc);
      return -1;
    }
  return 0;
}

int
Rts2DevSensorA3200::setValue (Rts2Value * old_value, Rts2Value * new_value)
{
  if (old_value == ax1)
    {
      return moveAxis (AXISINDEX_1, new_value->getValueLong ());
    }
  if (old_value == ax2)
    {
      return moveAxis (AXISINDEX_2, new_value->getValueLong ());
    }
  if (old_value == ax3)
    {
      return moveAxis (AXISINDEX_3, new_value->getValueLong ());
    }
  return Rts2DevSensor::setValue (old_value, new_value);
}

Rts2DevSensorA3200::Rts2DevSensorA3200 (int in_argc, char **in_argv):Rts2DevSensor (in_argc,
	       in_argv)
{
  hAerCtrl = NULL;

  mAxis = AXISMASK_1 | AXISMASK_2 | AXISMASK_3;

  createValue (ax1, "AX1", "first axis", true);
  createValue (ax2, "AX2", "second axis", true);
  createValue (ax3, "AX3", "third axis", true);

  addOption ('f', NULL, 1, "Init file");
}

Rts2DevSensorA3200::~Rts2DevSensorA3200 (void)
{
  AerSysStop (hAerCtrl);
}

int
Rts2DevSensorA3200::processOption (int in_opt)
{
  switch (in_opt)
    {
    case 'f':
      initFile = optarg;
      return 0;
    }
  return Rts2DevSensor::processOption (in_opt);
}

int
Rts2DevSensorA3200::init ()
{
  AERERR_CODE eRc = AERERR_NOERR;
  int ret;
  ret = Rts2DevSensor::init ();
  if (ret)
    return ret;

  eRc =
    AerSysInitialize (0, initFile, 1, &hAerCtrl, NULL, NULL, NULL, NULL, NULL,
		      NULL, NULL, NULL);

  if (eRc != AERERR_NOERR)
    {
      logErr ("init AerSysInitialize", eRc);
      return -1;
    }
  ret = home ();
  return ret;
}

int
Rts2DevSensorA3200::info ()
{
  DWORD dwUnits;
  DWORD dwDriveStatus[MAX_REQUESTED];
  DWORD dwAxisStatus[MAX_REQUESTED];
  DWORD dwFault[MAX_REQUESTED];
  DOUBLE dPosition[MAX_REQUESTED];
  DOUBLE dPositionCmd[MAX_REQUESTED];
  DOUBLE dVelocityAvg[MAX_REQUESTED];

  AERERR_CODE eRc;

  dwUnits = 0;			// we want counts as units for the pos/vels

  eRc = AerStatusGetAxisInfoEx (hAerCtrl, mAxis, dwUnits,
				dwDriveStatus,
				dwAxisStatus,
				dwFault,
				dPosition, dPositionCmd, dVelocityAvg);
  if (eRc != AERERR_NOERR)
    {
      logErr ("info AerStatusGetAxisInfoEx", eRc);
      return -1;
    }
  ax1->setValueDouble (dPosition[0]);
  ax2->setValueDouble (dPosition[1]);
  ax3->setValueDouble (dPosition[2]);
  return Rts2DevSensor::info ();
}

int
main (int argc, char **argv)
{
  Rts2DevSensorA3200 device = Rts2DevSensorA3200 (argc, argv);
  return device.run ();
}
