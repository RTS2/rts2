// $Id: OakFeatureReports.h 530 2009-02-11 01:30:20Z lukas $
//
/// \file  
/// \date 2008-10-17
/// \author Xavier Michelon, Toradex SA 
// Translation from c++ to plain c Copyright 2009, Lukas Zimmermann
/// \author Lukas Zimmermann, Basel
///  
/// \brief Implementation of wrapper functions for generic feature reports of
///        Oak devices


#ifndef OAK__FEATURE_REPORTS__H
#define OAK__FEATURE_REPORTS__H


#include "OakHidBase.h"


//*****************************************************************************
/// \brief Enumeration for the LED mode
//***************************************************************************** 
typedef enum EOakLedMode
{ 
   eLedModeOff =          0,  ///< LED is Off 
   eLedModeOn =           1,  ///< LED is On
   eLedModeBlinkSlowly =  2,  ///< LED is blinking slowly
   eLedModeBlinkFast =    3,  ///< LED is blinking fast
   eLedModeBlink4Pulses = 4   ///< LED is in 4 pulses blink mode
} EOakLedMode;


//*****************************************************************************
/// \brief Enumeration for the report mode
//***************************************************************************** 
typedef enum EOakReportMode
{
   eReportModeAfterSampling = 0,     ///< Interrupt reports are sent after sampling
   eReportModeAfterChange   = 1,     ///< Interrupt reports are sent after change
   eReportModeFixedRate     = 2,     ///< Interrupt reports are sent at a fixed rate
} EOakReportMode;


///< Set the device report mode
EOakStatus setReportMode(int deviceHandle,
                         EOakReportMode reportMode, bool persistent);

///< Retrieve the device report mode
EOakStatus getReportMode(int deviceHandle,
                         EOakReportMode* outReportMode, bool persistent);

//< Set the device LED blinking mode
EOakStatus setLedMode(int deviceHandle,
                      EOakLedMode ledMode, bool persistent);

///< Get the device LED blinking mode
EOakStatus getLedMode(int deviceHandle,
                      EOakLedMode* outLedMode, bool persistent);

///< Set the device report rate (period at which the device sends interrupt
///  reports to the host)
EOakStatus setReportRate(int deviceHandle,
                         unsigned int reportRate, bool persistent);

///< Get the device report rate (period at which the device send interrupt
///  reports to the host)
EOakStatus getReportRate(int deviceHandle,
                         unsigned int* outReportRate, bool persistent);

///< Set the device sample rate
EOakStatus setSampleRate(int deviceHandle,
                         unsigned int sampleRate, bool persistent);

///< Get the device sample rate
EOakStatus getSampleRate(int deviceHandle,
                         unsigned int* outSampleRate, bool persistent);

///< Set the user changeable device name
EOakStatus setUserDeviceName(int deviceHandle,
                             const char* deviceUserName, bool persistent);

///< Get the user changeable device name
EOakStatus getUserDeviceName(int deviceHandle,
                             char** outDeviceUserName, bool persistent);

///< Set the user changeable name for a channel of the device
EOakStatus setUserChannelName(int deviceHandle, int channel,
                              const char* userChannelName, bool persistent);

///< Get the user changeable name for a channel of the device
EOakStatus getUserChannelName(int deviceHandle, int channel,
                              char** outUserChannelName, bool persistent);

EOakStatus setInputMode(int deviceHandle,
                        unsigned int inputMode, bool persistent);

EOakStatus getInputMode(int deviceHandle,
                        unsigned int* inputMode, bool persistent);

EOakStatus setChannelOffset(int deviceHandle, unsigned int channel,
                            unsigned int offset_mV, bool persistent);

EOakStatus getChannelOffset(int deviceHandle, unsigned int channel,
                             unsigned int* offset_mV, bool persistent);

#endif // #ifndef OAK__FEATURE_REPORTS__H

