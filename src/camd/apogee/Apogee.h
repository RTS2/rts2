enum Camera_Status
{
  Camera_Status_Idle = 0,
  Camera_Status_Waiting,
  Camera_Status_Exposing,
  Camera_Status_Downloading,
  Camera_Status_LineReady,
  Camera_Status_ImageReady,
  Camera_Status_Flushing
};

enum Camera_CoolerStatus
{
  Camera_CoolerStatus_Off = 0,
  Camera_CoolerStatus_RampingToSetPoint,
  Camera_CoolerStatus_Correcting,
  Camera_CoolerStatus_RampingToAmbient,
  Camera_CoolerStatus_AtAmbient,
  Camera_CoolerStatus_AtMax,
  Camera_CoolerStatus_AtMin,
  Camera_CoolerStatus_AtSetPoint
};

enum Camera_CoolerMode
{
  Camera_CoolerMode_Off = 0,	// shutdown immediately
  Camera_CoolerMode_On,		// enable cooler, starts searching for set point
  Camera_CoolerMode_Shutdown	// ramp to ambient, then shutdown
};
