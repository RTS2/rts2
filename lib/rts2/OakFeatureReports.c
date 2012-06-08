// $Id: OakFeatureReports.c 531 2009-02-11 01:31:21Z lukas $
//
/// \file  /
/// \date 2008-10-17
/// \author Xavier Michelon, Toradex SA 
// Translation from c++ to plain c Copyright 2009, Lukas Zimmermann
/// \author Lukas Zimmermann, Basel
///  
/// \brief Implementation of wrapper functions for generic feature reports
///        of Oak devices

#include "OakFeatureReports.h"


//******************************************************************************* 
/// \param[in] deviceHandle The device handle
/// \param[in] reportMode The report mode to set
/// \param[in] persistent Should the setting be persistent (i.e. saved in Flash)
///                       or not (saved in ROM)
/// \return A status code for the operation
//******************************************************************************* 
EOakStatus setReportMode(int deviceHandle,
                         EOakReportMode reportMode, bool persistent)
{
   OakFeatureReport report = { 0,
                               persistent ? 1: 0,
                               1, 0, 0, (unsigned char) (reportMode), 0 };
   return sendReportAndWaitForReply(deviceHandle, report);
}


//******************************************************************************* 
/// \param[in] deviceHandle The device handle
/// \param[out] outReportMode The report mode
/// \param[in] persistent Should the function retriveve the persistent setting
///                       (i.e. saved in Flash) or not (saved in ROM)
/// \return A status code for the operation
//******************************************************************************* 
EOakStatus getReportMode(int deviceHandle,
                         EOakReportMode* outReportMode, bool persistent)
{
   OakFeatureReport report = { 1, persistent ? 1 : 0, 1, 0 , 0, 0};
   EOakStatus status = sendReportAndWaitForReply(deviceHandle, report);
   if (eOakStatusOK != status) return status;
   *outReportMode = (EOakReportMode) report[1];
   return status;
}


//******************************************************************************* 
/// \param[in] deviceHandle The device handle
/// \param[in] ledMode The LED blinking mode to set
/// \param[in] persistent Should the setting be persistent (i.e. saved in Flash)
///                       or not (saved in ROM)
/// \return A status code for the operation
//******************************************************************************* 
EOakStatus setLedMode(int deviceHandle, EOakLedMode ledMode, bool persistent)
{
   OakFeatureReport report = { 0 ,
                               persistent ? 1 : 0,
                               1, 1, 0,
                               (unsigned char)(ledMode),
                               0
                             }; 
   return sendReportAndWaitForReply(deviceHandle, report);
}


//******************************************************************************* 
/// \param[in] deviceHandle The device handle
/// \param[out] outLedMode The LED blinking mode
/// \param[in] persistent Should the function retrieve the setting be persistent
///                       (i.e. saved in  Flash) or not (saved in ROM)
/// \return A status code for the operation
//*******************************************************************************
EOakStatus getLedMode(int deviceHandle, EOakLedMode* outLedMode, bool persistent)
{
   OakFeatureReport report = { 1, persistent ? 1 : 0, 0x1, 0x1, 0 };
   EOakStatus status = sendReportAndWaitForReply(deviceHandle, report);
   if (eOakStatusOK != status) return status;
   *outLedMode = (EOakLedMode) report[1];
   return status;
}


//******************************************************************************* 
/// \note This value is only relevant if the report mode is set to 
///
/// \param[in] deviceHandle The device handle
/// \param[in] reportRate The new value for the report rate, in ms
/// \param[in] persistent Should the function set the persistent value of the
///             setting (i.e. saved in Flash) or not (saved in ROM)
/// \return A status code for the operation
//******************************************************************************* 
EOakStatus setReportRate(int deviceHandle,
                         unsigned int reportRate, bool persistent)
{
   OakFeatureReport report = { 0,
                               persistent ? 1 : 0,
                               2,
                               0,
                               0,
                               (unsigned char)(reportRate & 0xff),
                               (unsigned char)(reportRate >> 8),
                               0
                             };
   return sendReportAndWaitForReply(deviceHandle, report);
}


//******************************************************************************* 
/// \note Depending on the current report mode, this setting may have not effect
///       on the actual report rate
///
/// \param[in] deviceHandle The device handle
/// \param[out] outReportRate The retrieved value for the report rate, in ms
/// \param[in] persistent Should the function retrieve persistent value of the
///            setting (i.e. save in Flash) or not (saved in ROM)
/// \return A status code for the operation
//******************************************************************************* 
EOakStatus getReportRate(int deviceHandle,
                         unsigned int* outReportRate, bool persistent)
{
   OakFeatureReport report = { 1, persistent ? 1 : 0, 2, 0, 0, 0 };
   EOakStatus status = sendReportAndWaitForReply(deviceHandle, report);
   if (eOakStatusOK != status)
      return status;
   *outReportRate = (unsigned int)(report[1]) + (unsigned int)(report[2] << 8);
   return status;
}


//******************************************************************************* 
/// \param[in] deviceHandle The device handle
/// \param[in] sampleRate The new value for the sampling rate, in ms
/// \param[in] persistent Should the function set the persistent value of the
///                       setting (i.e. saved in Flash) or not (saved in ROM)
/// \return A status code for the operation
//******************************************************************************* 
EOakStatus setSampleRate(int deviceHandle,
                         unsigned int sampleRate, bool persistent)
{
   OakFeatureReport report = { 0, persistent ? 1 : 0,
                               2,
                               1,
                               0, 
                               (unsigned char)(sampleRate& 0xff),
                               (unsigned char)(sampleRate >> 8),
                               0
                             };
   return sendReportAndWaitForReply(deviceHandle, report);
}



//******************************************************************************* 
/// \param[in] deviceHandle The device handle
/// \param[out] outSampleRate The retrieved sampling rate, in ms
/// \param[in] persistent Should the function retrieve the persistent value of
///                       the setting (i.e. saved in Flash) or not (saved in ROM)
/// \return A status code for the operation
//******************************************************************************* 
EOakStatus getSampleRate(int deviceHandle,
                         unsigned int* outSampleRate, bool persistent)
{
   OakFeatureReport report = { 1, persistent ? 1 : 0, 2, 1, 0, 0, 0, 0 };
   EOakStatus status = sendReportAndWaitForReply(deviceHandle, report);
   if (eOakStatusOK != status)
     return status;
   *outSampleRate = (unsigned int)(report[1]) + (unsigned int)(report[2] << 8);     
   return status;
}


//*******************************************************************************
/// \param[in] deviceHandle The device handle
/// \param[out] inputMode The input mode to be set (0=single ended, 1=differential)
/// \param[in] persistent Should the function retrieve the persistent value of
///                       the setting (i.e. saved in Flash) or not (saved in ROM)
/// \return A status code for the operation
//*******************************************************************************
EOakStatus setInputMode(int deviceHandle,
                        unsigned int inputMode, bool persistent)
{
   OakFeatureReport report = { 0,
                               persistent ? 1 : 0,
                               1, 2, 0,
                               (unsigned char)inputMode,
                               0
                             };
   return sendReportAndWaitForReply(deviceHandle, report);
}


//*******************************************************************************
/// \param[in] deviceHandle The device handle
/// \param[out] inputMode The retrieved input mode (0=single ended 
/// \param[in] persistent Should the function retrieve the persistent value of
///                       the setting (i.e. saved in Flash) or not (saved in ROM)
/// \return A status code for the operation
//*******************************************************************************
EOakStatus getInputMode(int deviceHandle,
                        unsigned int* inputMode, bool persistent)
{
   OakFeatureReport report = { 1, persistent ? 1 : 0, 1, 2, 0 };
   EOakStatus status = sendReportAndWaitForReply(deviceHandle, report);
   if (eOakStatusOK != status)
     return status;
   *inputMode = (unsigned int)report[1];
   return status;
}


//*******************************************************************************
/// \param[in] deviceHandle The device handle
/// \param[in] channel number of the channel to set an offset for.
/// \param[in] offset_mV the value of the offset to set for the given channel.
/// \param[in] persistent Should the function set the persistent value of the
///                       setting (i.e. saved in Flash) or not (saved in ROM)
/// \return A status code for the operation
//*******************************************************************************
EOakStatus setChannelOffset(int deviceHandle, unsigned int channel,
                            unsigned int offset_mV, bool persistent)
{
   OakFeatureReport report = { 0,
                               persistent ? 1 : 0,
                               2,
                               channel + 3,
                               0,
                               (unsigned char)(offset_mV & 0xff),
                               (unsigned char)(offset_mV >> 8),
                               0
                             };
   return sendReportAndWaitForReply(deviceHandle, report);
}


//*******************************************************************************
/// \param[in] deviceHandle The device handle
/// \param[in] channel number of the channel to set an offset for.
/// \param[out] offset_mV The retrieved value of the offset for the given channel.
/// \param[in] persistent Should the function retrieve the persistent value of
///                       the setting (i.e. saved in Flash) or not (saved in ROM)
/// \return A status code for the operation
//*******************************************************************************
EOakStatus getChannelOffset(int deviceHandle, unsigned int channel,
                             unsigned int* offset_mV, bool persistent)
{
   OakFeatureReport report = { 1,
                               persistent ? 1 : 0,
                               2,
                               channel + 3,
                               0
                             };
   EOakStatus status = sendReportAndWaitForReply(deviceHandle, report);
   if (eOakStatusOK != status)
     return status;
   *offset_mV = (unsigned int)(report[1]) + (unsigned int)(report[2] << 8);
   return status;
}


//******************************************************************************* 
/// \param[in] deviceHandle the device handle
/// \param[in] deviceUserName the new device name
/// \param[in] persistent Should the function set the persistent value of the
///                       setting (i.e. saved in Flash) or not (saved in ROM)
/// \return A status code for the operation
//******************************************************************************* 
EOakStatus setUserDeviceName(int deviceHandle,
                             const char* deviceUserName, bool persistent)
{
   OakFeatureReport report = { 0, persistent ? 1 : 0, 0x15, 0, 0 };
   putStringInReport(report, deviceUserName);
   return sendReportAndWaitForReply(deviceHandle, report);
}


// Note: the function getUserDeviceName is implemented in file OakHIDBase.h as it
// is required by the getDeviceInfo() low-level function


//******************************************************************************* 
/// \param[in] deviceHandle the device handle
/// \param[in] channel The target channel
/// \param[in] userChannelName the zero based index of the channel
/// \param[in] persistent Should the function set the persistent value of the
///                       setting (i.e. saved in Flash) or not (saved in ROM)
/// \return A status code for the operation
//******************************************************************************* 
EOakStatus setUserChannelName(int deviceHandle, int channel,
                              const char* userChannelName, bool persistent)
{
   OakFeatureReport report = { 0,
                               persistent ? 1 : 0,
                               0x15,
                               (unsigned char)(channel),
                               0
                             };
   putStringInReport(report, userChannelName);
   return sendReportAndWaitForReply(deviceHandle, report);
}


// Note: the function setUserChannelName is implemented in file OakHIDBase.h
// as it is required by the getChannelInfo() low-level function


