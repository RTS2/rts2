// $Id: OakHidBase.h 529 2009-02-11 01:26:23Z lukas $
//
/// \file  
/// \date 2008-10-14
/// \author Xavier Michelon, Toradex SA 
// Translation from c++ to plain c Copyright 2009, Lukas Zimmermann
/// \author Lukas Zimmermann, Basel
///  
/// \brief Declaration of Oak HID base functions
///
/// These functions encapsulate the Linux hiddev calls


#ifndef OAK__HID__BASE__H
#define OAK__HID__BASE__H

#include <stddef.h>


//**************************************************************************** 
/// \brief A few integer constants
//**************************************************************************** 
enum 
{ 
   kFeatureReportSize   = 32,        ///< Size of the feature report in bytes
   kToradexVendorID     = 0x1b67,    ///< USB Vendor ID for Toradex
   kOakMoveProductID    = 0x6        ///< Oak Move sensor product ID
};


//**************************************************************************** 
/// \brief Enumeration for status code return by oak functions
//**************************************************************************** 
typedef enum EOakStatus {
   eOakStatusReadError = -1,                 ///< A read error occured
   eOakStatusOK  = 0,                        ///< Success
   eOakStatusErrorOpeningFile,               ///< The device file could not be
                                             //   opened
   eOakStatusInvalidDeviceType,              ///< The device is not a Oak sensor
   eOakStatusInternalError,                  ///< Other errors,
                                             //   unlikely to happen
   eOakStatusInvalidStringDescriptorIndex,   ///< The string descriptor index
                                             //   is invalid
   eOakStatusWriteError,                     ///< A write error occured
} EOakStatus;

 ///< Type definition for the Oak sensor feature report
typedef unsigned char OakFeatureReport[kFeatureReportSize];


//**************************************************************************** 
/// \brief Structure used for storing device information
//**************************************************************************** 
typedef struct DeviceInfo {
   char* deviceName;                ///< The device hard-coded name
   char* volatileUserDeviceName;    ///< The volatile (stored in RAM) user
                                    //   settable device name
   char* persistentUserDeviceName;  ///< The persistent (stored in FLASH) user
                                    //   settable device name
   char* serialNumber;              ///< The serial number of the device
   short vendorID;                  ///< The vendor ID of the device
   short productID;                 ///< The product ID of the device
   short version;                   ///< The version of the device
   int numberOfChannels;            ///< The number of channels of the device
} DeviceInfo;


//**************************************************************************** 
/// \brief Structure for holding channel information
//****************************************************************************
typedef struct ChannelInfo {
   char* channelName;               ///< The hard-coded name of the channel
   char* volatileUserChannelName;   ///< The user settable name of the device
                                    //    stored in RAM
   char* persistentUserChannelName; ///< The user settable name of the device
                                    //   stored in FLASH
   int isSigned;                    ///< Is the read value signed ?
   int bitSize;                     ///< The size in bits of the channel
   int unitExponent;                ///< The unit exponent (received value must
                                    //   be multiplied by 10e[unitExponent]
   unsigned int unitCode;           ///< The USB standardized unit code
   char* unit;                      ///< The 'fancy' unit name
} ChannelInfo;


//*****************************************************************************
/// \brief boolean type
//*****************************************************************************
typedef enum bool
{
  false = 0,
  true = 1,
} bool;


void * xmalloc(size_t size);

void * xrealloc(void *ptr, size_t size);


///< Open a Oak sensor device
EOakStatus openDevice(const char* devicePath, int* outDeviceHandle);

///< Close a Oak sensor device
EOakStatus closeDevice(int deviceHandle);

///< Retrieve global information about the device
EOakStatus getDeviceInfo(int deviceHandle, DeviceInfo* outDeviceInfo);

///< Retrieve information about a channel
EOakStatus getChannelInfo(int deviceHandle,
                          int index, ChannelInfo* outChannelInfo);

///< Read an interrupt report and put the read values inside the
//   outReadValues vector
EOakStatus readInterruptReport(int deviceHandle, int* outReadValues);

///< Send a feature report to the specified device
EOakStatus sendFeatureReport(int deviceHandle, OakFeatureReport report);

///< Read a feature report from the device
EOakStatus readFeatureReport(int deviceHandle, OakFeatureReport report);

///< Display a feature report to the standard output
EOakStatus printFeatureReport(OakFeatureReport* report);

///< Send a report and wait for the reply
EOakStatus sendReportAndWaitForReply(int deviceHandle,
                                     OakFeatureReport report);

///< Utility function that insert a string into a report
EOakStatus putStringInReport(OakFeatureReport report, const char* theString);

///< Utility function that extract a string from a feature report sent by an
//   Oak device
EOakStatus getStringFromReport(OakFeatureReport report, char** outString);

///< Return a string containing a description of the given status code
char* getStatusString(EOakStatus oakStatus);


#endif // #ifndef OAK__HID__BASE__H

