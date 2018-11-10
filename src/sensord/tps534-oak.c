/*
 * $Id: oak_cloudsens.c 961 2010-07-19 11:59:58Z lukas $
 *
 * Copyright (C) 2010 Lukas Zimmermann, Basel, Switzerland
 *
 *
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software Foundation,
   Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

   Or visit http://www.gnu.org/licenses/gpl.html.

 */

/*****************************************************************************
 * This driver implements the interface to the home built Perkin Elmer 
 * TPS534 based cloud sensor using an Oak USB A/D converter for digitizing.
 *****************************************************************************/

/* _GNU_SOURCE is needed for mempcpy which is a GNU libc extension */
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <math.h>
#include <asm/types.h>     //needed for <linux/hiddev.h>
#include <linux/hiddev.h>
#include <pthread.h>
#include <errno.h>
#include <syslog.h>

#include "OakHidBase.h"
#include "OakFeatureReports.h"
#include "tps534-oak.h"


pthread_t tps534_oak_digin_th_id;
void *tps534_oak_digin_thread(void * args);

extern tps534_state tps534State ;

/// \brief wrapper function for error report that add file,
///        line and function in which an error occured
#define CHECK_OAK_CALL(status) \
        checkOakCall(status, __FILE__, __LINE__, __FUNCTION__)

#define TORADEX_DEVICE_NAME "Toradex +/-10V ADC"
#define K -6.0e-10
#define EXPONENT 4
#define T0_ABS -273.16
#define REF_VOLTAGE 4.97
#define BRIDGE_R_ONE 390000.0
#define BRIDGE_R_PARALL 68000.0
#define THERMIST_R_THETA25 30000.0
#define THERMIST_BETA 3964
#define THERMIST_AMPLIFICATION 7.12

// table of resistances of the thermistor of the TPS534 as a function of
// temperature. Taken from Perkin Elmer Product Specification sheet.
typedef struct {
  float res;
  float temp;
} res_table;

#define NR_THERMISTOR 29 
res_table r_thermistor[] = {
  { 2073.0, 100.0 },
  { 2405.0, 95.0 },
  { 2799.0, 90.0 },
  { 3273.0, 85.0 },
  { 3840.0, 80.0 },
  { 4524.0, 75.0 },
  { 5349.0, 70.0 },
  { 6354.0, 65.0 },
  { 7581.0, 60.0 },
  { 9090.0, 55.0 },
  { 10950.0, 50.0 },
  { 13254.0, 45.0 },
  { 16119.0, 40.0 },
  { 19716.0, 35.0 },
  { 24249.0, 30.0 },
  { 30000.0, 25.0 },
  { 37320.0, 20.0 },
  { 46710.0, 15.0 },
  { 58890.0, 10.0 },
  { 74730.0, 5.0 },
  { 95490.0, 0.0 },
  { 122910.0, -5.0 },
  { 159390.0, -10.0 },
  { 208350.0, -15.0 },
  { 274590.0, -20.0 },
  { 365100.0, -25.0 },
  { 489600.0, -30.0 },
  { 663000.0, -35.0 },
  { 907200.0, -40.0 },
};

int debug = 0;           /* a debug value > 0 will print debugging information
                            to syslog. */

int oakAdHandle = -1;
DeviceInfo devInfo;      // structure holding oak digital input device informations
ChannelInfo* channelInfos = NULL;
int* outReadValues = NULL;

int oak_cb_id = -1;

int read_oak_cnt = 0;

float sky_temp_K = K;
float sky_temp_exponent = EXPONENT;
float thermist_Rt25 = THERMIST_R_THETA25;
float thermist_beta = THERMIST_BETA;
float thermist_amplification = THERMIST_AMPLIFICATION;
float ref_voltage = REF_VOLTAGE;
float bridge_R1 = BRIDGE_R_ONE;
float bridge_Rp = BRIDGE_R_PARALL;
int sample_rate = 1000;   /* sampling/report rate in milli seconds of the oak
                            a/d converter */

//*****************************************************************************
// Calculates the resistance of the TPS534 thermistor from the measured voltage
// in a half bridge configuration with a resistor in parallel to the thermistor.
//*****************************************************************************
double
r_theta(double u)
{
  return 1.0 / (((ref_voltage - u / thermist_amplification)
               / (u / thermist_amplification * bridge_R1) - 1.0 / bridge_Rp));
}

//*****************************************************************************
// Calculates the temperature of the TPS534 thermistor from the resistance 
// based on the beta value and the resistance at 25°C.
//*****************************************************************************
double
theta(double r)
{
  return 1.0 /
         ((1.0 / (-T0_ABS + 25.0))
         - (log(thermist_Rt25 / r) / thermist_beta)) + T0_ABS;
}

//*****************************************************************************
// Calculates the sky temperature
//*****************************************************************************
double
sky(double u_tpile, double t_amb)
{
  double res = u_tpile / sky_temp_K + pow(t_amb - T0_ABS, sky_temp_exponent);
  return res >= 0.0 ?
                pow(res, 1.0/sky_temp_exponent) + T0_ABS :
                -pow(-res, 1.0/sky_temp_exponent) + T0_ABS;
}

//*****************************************************************************
// Calculates the temperature for the TPS534 thermistor from the measured
// resistance based on a lookup table with linearly interpolated value.
//*****************************************************************************
float
resistance2temp(float r)
{
  int i = 0;
  int j = NR_THERMISTOR;
  int p = j / 2;

  // binary search
  do {
    if (r <= r_thermistor[p].res) {
      //c = 1;
      j = p;
      p = ((j - i) / 2) + i;
    } else {
      //c = 0;
      i = p;
      p = ((j - i) / 2) + i;
    }
  } while (j - 1 > i);

  return r_thermistor[j].temp
         + (r_thermistor[i].temp - r_thermistor[j].temp)
           / (r_thermistor[j].res - r_thermistor[i].res)
           * (r - r_thermistor[i].res);

}

//*****************************************************************************
// Calculates the temperature difference of the ambient temperature and the
// thermopile temperature of the TPS534 sensor from the thermo voltage and the
// ambient temperature.
//*****************************************************************************
double
thermopile2tempdiff(double u, double t_amb)
{
  double d = u/sky_temp_K + pow(T0_ABS + t_amb, sky_temp_exponent);
  return (d >= 0 ?
     pow(d, 1.0/sky_temp_exponent) : -pow(-d, 1.0/sky_temp_exponent)) - T0_ABS;
}

//****************************************************************************
/// \brief Check the status code of a Oak library call.
///
/// If the status is not OK, this function displays an error message to the
/// standard error output, then quits the application.
///
/// Note: This function is not to be called directly, but via the
///       CHECK_OAK_CALL macro that inserts the line, file and function where
///       the error occured.
///
/// \param[in] status the status code to check
/// \param[in] file The file where the error occured
/// \param[in] line The line number where the error occured
/// \param[in] function The name of the function where the error occured
//****************************************************************************
int
checkOakCall(EOakStatus status,
                  const char *file, int line, const char* function)
{
  if (status != eOakStatusOK) {
    if (debug >= 1) {
      syslog (LOG_ERR, "Error: %s in function %s (file %s, line %d)\n",
                      getStatusString(status), function, file, line);
    }
    return status;

  } else {
    return 0;
  }
}

//****************************************************************************
/// \brief Calculates physical value from a read channel.
///
/// \param[in] index Channel number.
/// \param[in] value 4-byte value from a channel of the connected hid device.
/// \param[in] channelInfos Vector of channel infos of the connected hid device.
//****************************************************************************
double channelValue(int local_index, int value, ChannelInfo* local_channelInfos)
{
  ChannelInfo* chanInfoFrame = &local_channelInfos[local_index];
  double dresult;
  //chanInfoFrame->channelName;
  if (chanInfoFrame->isSigned) {
    dresult = value;
  } else {
    dresult = (unsigned int) value;
  }
  if (0 != chanInfoFrame->unitExponent) {
    dresult *= pow(10.0, chanInfoFrame->unitExponent);
  }
  return dresult;
}

/*****************************************************************************
 * oak_ad_setup(..)
 *****************************************************************************/
int
oak_ad_setup(char *device, int* deviceHandle, DeviceInfo* local_devInfo, 
             ChannelInfo* local_channelInfos[], char* oakDevice)
{
  int oak_stat = eOakStatusOK;
  int report_mode = eReportModeFixedRate;
  int report_rate = sample_rate;

  oak_stat = CHECK_OAK_CALL(openDevice(oakDevice, deviceHandle));
  if (oak_stat != eOakStatusOK) return oak_stat;

  // get the device informations
  oak_stat = CHECK_OAK_CALL(getDeviceInfo(*deviceHandle, local_devInfo));
  if (oak_stat != eOakStatusOK) return oak_stat;

  if (debug >= 1) {
    syslog (LOG_DEBUG, "Device Name: %s\n", local_devInfo->deviceName);
    syslog (LOG_DEBUG, "Volatile User Device Name: %s\n",
                    local_devInfo->volatileUserDeviceName);
    syslog (LOG_DEBUG, "Persistent User Device Name: %s\n",
                    local_devInfo->persistentUserDeviceName);
    syslog (LOG_DEBUG, "Serial Number: %s\n", local_devInfo->serialNumber);
    syslog (LOG_DEBUG, "VendorID: 0x%x :: ProductID: 0x%x :: Version 0x%x\n",
                    local_devInfo->vendorID, local_devInfo->productID, local_devInfo->version);
    syslog (LOG_DEBUG, "Number of channels: %d\n", local_devInfo->numberOfChannels);
  }

  if (strcmp(local_devInfo->deviceName, TORADEX_DEVICE_NAME) != 0) {
    syslog (LOG_ERR, "%s is not the correct type: %s (should be %s)",
                     device,
                     local_devInfo->deviceName, TORADEX_DEVICE_NAME);
    return -20;
  }

  // retrieve Channel infos
  *local_channelInfos = (ChannelInfo*) realloc( *local_channelInfos, 
                              local_devInfo->numberOfChannels * sizeof(ChannelInfo));
  int i;
  ChannelInfo* chInf = *local_channelInfos;
  for (i = 0; i < local_devInfo->numberOfChannels; i++) {
  // make shure that all pointers int the struct ChannelInfo that get
  // malloced have NULL value.
     memset(chInf, 0, sizeof(ChannelInfo));
    CHECK_OAK_CALL(getChannelInfo(*deviceHandle, i, chInf));
    if (debug >= 1) {
      syslog (LOG_DEBUG, "Ch %d: %s, sign: %d, exp: %d, unit: %s\n",
                      i,
                      chInf->channelName,
                      chInf->isSigned,
                      chInf->unitExponent,
                      chInf->unit);
    }
    chInf++;
  }

/*
  // Set the LED Mode to indicate initialize state
  oak_stat = CHECK_OAK_CALL(setLedMode(*deviceHandle, eLedModeBlinkFast, false));
  EOakLedMode ledMode;
  oak_stat = CHECK_OAK_CALL(getLedMode(*deviceHandle, &ledMode, false));
*/
  oak_stat = CHECK_OAK_CALL(setInputMode(*deviceHandle, 0, false));
  unsigned int inpMode;
  oak_stat = CHECK_OAK_CALL(getInputMode(*deviceHandle, &inpMode, false));
  if (inpMode != 0) {
    syslog (LOG_ERR, "failed setting input mode to 0.\n");
  }
 
  // Set the report Mode
  oak_stat = CHECK_OAK_CALL(setReportMode(*deviceHandle, report_mode, false));
  if (oak_stat != eOakStatusOK) return oak_stat;
  EOakReportMode reportMode;
  oak_stat = CHECK_OAK_CALL(getReportMode(*deviceHandle, &reportMode, false));
  if (oak_stat != eOakStatusOK) return oak_stat;
  if (debug >= 1) {
    syslog (LOG_DEBUG, "Report mode: %u\n", reportMode);
  }

  // Set the report rate
  oak_stat = CHECK_OAK_CALL(setReportRate(*deviceHandle, report_rate, false));
  if (oak_stat != eOakStatusOK) return oak_stat;
  unsigned int reportRate;
  oak_stat = CHECK_OAK_CALL(getReportRate(*deviceHandle, &reportRate, false));
  if (oak_stat != eOakStatusOK) return oak_stat;
  if (debug >= 1) {
    syslog (LOG_DEBUG, "Report rate: %u\n", reportRate);
  }

  // Set the sample rate
  oak_stat = CHECK_OAK_CALL(setSampleRate(*deviceHandle, sample_rate, false));
  if (oak_stat != eOakStatusOK) return oak_stat;
  unsigned int sampleRate;
  oak_stat = CHECK_OAK_CALL(getSampleRate(*deviceHandle, &sampleRate, false));
  if (oak_stat != eOakStatusOK) return oak_stat;
  if (debug >= 1) {
    syslog (LOG_DEBUG, "Sample rate: %u\n", sampleRate);
  }

  return oak_stat;
}
/******************************************************************************
 * restart thread
 *****************************************************************************/
int restartThread() {

  int thread_stat= -1; 

  thread_stat = pthread_cancel(tps534_oak_digin_th_id); // 0 = success
  if( thread_stat != 0 ) {
    syslog (LOG_ERR, "restartThread: Oak thread NOT stopped\n");
    return thread_stat ;
  }
 
  thread_stat = pthread_create(&tps534_oak_digin_th_id, NULL, &tps534_oak_digin_thread, NULL); // 0 success
  if( thread_stat != 0 ) {
    syslog (LOG_ERR, "restartThread: Oak thread NOT started\n");
    return thread_stat ;
  }

  return 0 ;

}

/******************************************************************************
 * connectDevice(int connecting)
 *****************************************************************************/
int
connectDevice(char *oakAdDevice, int connecting)
{
  int thread_stat= -1; 
  int oak_stat;


  if (connecting) {
    oak_stat = oak_ad_setup(oakAdDevice, &oakAdHandle, &devInfo, &channelInfos, oakAdDevice);
    if (oak_stat != eOakStatusOK) {
      syslog (LOG_ERR, "setting up Oak AD \"%s\" failed.", oakAdDevice);
      return OAK_CONNECTION_FAILED;
    }
    syslog (LOG_DEBUG, "connected to Oak AD %s\n", oakAdDevice);

    
    openlog ("tps534", LOG_CONS | LOG_PID | LOG_NDELAY, LOG_LOCAL0);
    // create receive communication thread
    thread_stat = pthread_create(&tps534_oak_digin_th_id, NULL, &tps534_oak_digin_thread, NULL); // 0 success

  } else { // if (connecting)
    if (oakAdHandle > 0) {
      closeDevice(oakAdHandle);
      oakAdHandle = -1;
      thread_stat = pthread_cancel(tps534_oak_digin_th_id); // 0 = success
      free(channelInfos) ; 
      closelog ();
      if( thread_stat != 0 ) {
        syslog (LOG_ERR, "connectDevice: Oak thread NOT stopped\n");
        return thread_stat ;
      }
    }
    /* Disconnect: prepare for reconnect. */
    syslog (LOG_DEBUG, "disconnected from Oak AD %s\n", oakAdDevice);
  }
  return 0 ;
}

/*****************************************************************************
 * tps534_oak_digin_thread()
 * Reads interrupt reports arriving from Oak digital input module in an
 * infinite loop, checks whether any of the inputs changed state and signals
 * the state transition
 *****************************************************************************/
#include <stdlib.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
void *
tps534_oak_digin_thread(__attribute__ ((unused)) void * args)
{
  static struct hiddev_event ev[64];
  int rd ;

  while (1) {


  NEXT:   rd = read(oakAdHandle, ev, sizeof(ev));
    //original libary call NEXT:   rd= readInterruptReport(oakAdHandle, ev);

    if (rd == -1) {
      syslog (LOG_ERR, "read error in tps534_oak_digin_thread(): %s", strerror(errno));
      goto NEXT;
    }
    if (debug >= 3) {
      syslog (LOG_DEBUG, "tps534_oak_digin_thread(): %d bytes read\n", rd);
    }

    if (rd < (int)sizeof(ev[0])) {
      syslog (LOG_ERR, "incomplete read from Oak device (%d bytes)\nsleeping\n", rd);
      goto NEXT;
    }

    size_t kCount = rd / sizeof(ev[0]);
    if (debug >= 3) {
      syslog (LOG_DEBUG, "tps534_oak_digin_thread(): %zd hiddev_event structs\n", kCount);
    }

    int i;
    double f;
    for (i = 0;(i < devInfo.numberOfChannels) && (i < 6) && (i < (int)kCount);i++) {
      if (debug >= 3) {
	syslog (LOG_DEBUG, "tps534_oak_digin_thread(): ch %d, exp: %d, sign: %d\n", i, channelInfos[i].unitExponent, channelInfos[i].isSigned);
      }
      
      f = pow(10.0, channelInfos[i].unitExponent);
      if (channelInfos[i].isSigned) {
	f *= ev[i].value;
      } else {
	f *= (unsigned) ev[i].value;
      }
      if (i > 0) {
	tps534State.analogIn[i - 1] = f;
      }
    }
    if (tps534State.analogIn[0] < 0.0) {
      tps534State.analogIn[0]= 0.0;
    }
    f = r_theta(tps534State.analogIn[0]);
    tps534State.calcIn[0]= f;
    tps534State.calcIn[1] = theta(f);
    tps534State.calcIn[2] = resistance2temp(f);
    tps534State.calcIn[3]= sky(tps534State.analogIn[1], tps534State.calcIn[1]);

    if (debug >= 1) {
      syslog (LOG_DEBUG, "Analog: %f %f %f %f %f\n", tps534State.analogIn[0], tps534State.analogIn[1], tps534State.analogIn[2], tps534State.analogIn[3], tps534State.analogIn[4]) ;
      syslog (LOG_DEBUG, "Calc  : %f %f %f %f\n", tps534State.calcIn[0], tps534State.calcIn[1], tps534State.calcIn[2], tps534State.calcIn[3]) ;
    }
    read_oak_cnt++;

  } /* while (1) */
  
  return NULL;
}
