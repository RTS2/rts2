/*
 * SSD650v inverter communication
 * Copyright (C) 2010 Markus Wildi, version for RTS2
 * Copyright (C) 2009 Lukas Zimmermann, Basel, Switzerland
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

#ifndef _GNU_SOURCE
#define _GNU_SOURCE

#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
//#include <malloc.h>
#include <sys/time.h>
#include <math.h>
#include <time.h>
#include <errno.h>

#endif

#include <door_vermes.h>
#include <vermes/ssd650v_comm.h>
#include <vermes/bisync.h>
#include <vermes/serial.h>
#include <vermes/util.h>
#include <vermes/move_door.h>
extern int debug;

// default serial port device and it's default settings
#define DEFAULT_SSD650V_SERPORT_DEV      "/dev/ssd650v"
#define DEFAULT_SSD650V_BITRATE          19200
#define DEFAULT_SSD650V_DATABITS         7
#define DEFAULT_SSD650V_PARITY           PARITY_EVEN
#define DEFAULT_SSD650V_STOPBITS         1

static int ser_dev;
static int ser_open = 0;
static int inverter_seq_status = -1;
static int mot_cmd_status = -1;
static struct timeval time_last_stat;   /* time of last status enquiry sent to
                                         * SSD650V */

pthread_t  move_door_id;
// 
int motorState= SSD650V_MS_UNDEFINED ;

// SSD650v frequency inverter states (ex INDI states)
// wildi ToDo, will goaway
//
//MotorFastStopSP.sp[0].s "MOTOR_FASTSTOP_SET" "Motor Fast Stop"
//MotorFastStopSP.sp[1].s "MOTOR_FASTSTOP_RESET" "Reset Motor Fast Stop"
struct ssd650v_fast_stop_state {
int set ;
int reset ;
} ;
static struct ssd650v_fast_stop_state fast_stop_state ;

struct ssd650v_motor_run_state {
int start ;
int stop ;
} ;
static struct ssd650v_motor_run_state motor_run_state ;

struct ssd650v_coast_state {
int set ;
int reset ;
} ;
static struct ssd650v_coast_state coast_state ;

struct ssd650v_seq_state {
int ssd_motor_seq ; // SSD650V_IS_IDLE, SSD650V_IS_OK
int ready;
int on;
int running;
int tripped;
int coast;
int fast_stop;
int on_disable;
int remote;
int sp_reached;
int intern_limit;
} ;
static struct ssd650v_seq_state seq_state ;


int ssd_start_comm( char *device_ssd650v);
int ssd_stop_comm() ;
char * SSD_qry_identity(int sd);
char * SSD_qry_major_state(int sd);
int SSD_qry_comms_status(int sd);
int SSD_qry_last_error(int sd);
float SSD_qry_setpoint(int sd);
int SSD_qry_hexword(int sd, int tag);
float SSD_qry_real(int sd, int tag);
int SSD_set_tag(int sd, int tag, char * data);
int motor_run_switch_state( int on_off) ;
typedef enum TAG_TYPES {
  TAGT_BOOL,
  TAGT_REAL,
  TAGT_INT,
  TAGT_WORD,
  TAGT_ENUM,
  TAGT_NBR
} tag_types_t;

typedef enum CMD_FORMATS {
  CMDF_MNEMO,
  CMDF_TAGNBR
} cmd_formats_t;

typedef enum PARAM_ACCESS {
  PARAM_RO,
  PARAM_WO,
  PARAM_RW
} param_access_t;

#define TRUE 1
#define FALSE 0

typedef unsigned char BOOLEAN;

/* structure holding all relevant information on a certain SSD650V command */
typedef struct tagdata {
  char * cmd_description;
  param_access_t param_perm;
  cmd_formats_t cmd_format;
  union {
    char mnemo[3];
    int tag;
  } command;
  tag_types_t data_type;
  union {
    BOOLEAN b; 
    float f;
    unsigned int i;
    unsigned char s;
    unsigned int tag_nbr;
  } data;
  unsigned int special_delay; /* a value != 0 indicates that this command needs
                               * more time than usual to complete,
                               * i.e. `special_delay' in milliseconds */
} tagdata_t;

tagdata_t load_settings_tseq[] = {
  {
    "Reset SSD650V",
    PARAM_WO,
    CMDF_MNEMO,
    {"!1"},
    TAGT_WORD,
    {(BOOLEAN)0x7777},
    1000
  },
  {
    "Restore configuration from non-volatile memory",
    PARAM_WO,
    CMDF_MNEMO,
    {"!1"},
    TAGT_WORD,
    {(BOOLEAN)0x0101},
    3000
  },
  {
    "Motor speed setpoint",
    PARAM_RW,
    CMDF_TAGNBR,
//    {(char)247},
    {""},
    TAGT_REAL,
    {100.0},
    0
  },
  {
    "Ramp accel time",
    PARAM_RW,
    CMDF_TAGNBR,
//    {(char)258},
    {""},
    TAGT_REAL,
    {5.0},
    0
  },
  {
    "Ramp decel time",
    PARAM_RW,
    CMDF_TAGNBR,
//    {(char)259},
    {""},
    TAGT_REAL,
    {2.0},
    0
  },
};
/******************************************************************************
 * motor_on(...)
 * Starts motor at setpoint.
 * 
 * Args:
 * Return:
 * state
 *****************************************************************************/
int motor_on()
{
  int ret ;
  if(( ret= motor_run_switch_state( SSD650V_CMD_ON)) != SSD650V_MS_RUNNING) {

    motorState=SSD650V_MS_UNDEFINED ;
    return SSD650V_MS_UNDEFINED ;
  }
  motorState= SSD650V_MS_RUNNING ;
  return SSD650V_MS_RUNNING;
}
/******************************************************************************
 * motor_off(...)
 * Stops motor.
 * 
 * Args:
 * Return:
 * state
 *****************************************************************************/
int motor_off()
{
  int ret ;
  if(( ret= motor_run_switch_state(SSD650V_CMD_OFF)) != SSD650V_MS_STOPPED) {

    motorState=SSD650V_MS_UNDEFINED ;
    return SSD650V_MS_UNDEFINED ;
  }
  motorState= SSD650V_MS_STOPPED ;
  return SSD650V_MS_STOPPED ;
}
/******************************************************************************
 * get_current()
 * get current [A]
 * 
 * Args:
 * Return:
 * state
 *****************************************************************************/
double
get_current()
{
  double current = (double) SSD_qry_real(ser_dev, 67);
  if (isnan(current)) {
    fprintf( stderr, "Failure querying current-----------------------------\n");
  }
  return current ;
}
/******************************************************************************
 * get_current_percentage()
 * get current as percentage of maximum
 * 
 * Args:
 * Return:
 * state
 *****************************************************************************/
double
get_current_percentage()
{
  double percent = (double) SSD_qry_real(ser_dev, 66);
  if (isnan(percent)) {
    fprintf( stderr, "Failure querying current percentage-----------------------------\n");
  }
  return percent ;
}
/******************************************************************************
 * get_setpoint(...)
 * Retrieves the motor speed setpoint from the SSD650V.
 * 
 * Args:
 * Return:
 *   a float representing the absolute value of the current setpoint
 *   or NaN if an error occured.
 *****************************************************************************/
float
get_setpoint()
{
  float tmp_set= -199 ; 
  if(isnan(tmp_set= SSD_qry_setpoint(ser_dev))) {
    fprintf( stderr, "ssd650v_comm_vermes: get_setpoint: failed %f \n", tmp_set) ;
  }
// wildi ToDo: make a real message
//  if(isnan( tmp_set)) {
//      return (float) SSD650V_MS_GETTING_SET_POINT_FAILED ;
//  } else {
    return tmp_set ;
//  }
}

/******************************************************************************
 * set_setpoint(...)
 * Transmits the motor speed setpoint to the SSD650V.
 * 
 * Args:
 *   setpoint:
 *   direction: 0: don't change, 1: clockwise, -1: counter clockwise
 * Return:
 *   BISYNC_ERROR status, 0 when all ok.
 *****************************************************************************/
int
set_setpoint(float setpoint)
{
  char data[8];
  
  if (isnan(setpoint)) {
    sys_log(ILOG_WARNING, "Don't set setpoint to NaN!");
    fprintf( stderr, "Don't set setpoint to NaN!\n");
    return BISYNC_ERR;
  }
  
  if ( fabs( setpoint) > 100.) {
    sys_log(ILOG_WARNING, "Don't set setpoint to larger than 100.%");
    fprintf( stderr, "Don't set setpoint to larger than 100.\n");
    return BISYNC_ERR;
  }
  // wildi ToDo: fails at the beginning
  /*   float ssd_setp = SSD_qry_real(ser_dev, 247);; */
  /*   if (isnan(ssd_setp)) { */
  /*     sys_log(ILOG_WARNING, "Failure querying setpoint"); */
  /*     fprintf( stderr, "Failure querying setpoint\n"); */
  /*     return BISYNC_ERR; */
  /*   } */
  snprintf(data, 8, "%3.1f", setpoint);

  //wildi  int ret= SSD_set_tag(ser_dev, 247, data) ;
  // Version 5
  int ret= SSD_set_tag(ser_dev, 269, data) ;
  if( ret == BISYNC_OK) {
    //fprintf( stderr, "set_setpoint: value %s\n", data) ; 
    return SSD650V_MS_OK ;
  } else {
    return SSD650V_MS_SETTING_SET_POINT_FAILED ;
  }
}

/******************************************************************************
 * qry_mot_cmd()
 * Queries the motor command parameter (tag 273) and signals the state.
 *****************************************************************************/

// wildi ToDo: integrate me
int
qry_mot_cmd()
{
  char * id_str;
  int cmd_stat = SSD_qry_hexword(ser_dev, 273);
  if (cmd_stat >= 0) {
    asprintf(&id_str, "0x%04X", cmd_stat);
    /*     IUSaveText(&SSD_device_infoTP.tp[3], id_str); */
    free(id_str);
    if (cmd_stat != mot_cmd_status) {
      /*       SSD_MotorCmdLP.s = IPS_OK; */
      mot_cmd_status = cmd_stat;
    }
  } else {
    mot_cmd_status = -1;
    /*     SSD_device_infoTP.s = IPS_ALERT; */
  }
  return cmd_stat;
}

/******************************************************************************
 * qry_mot_status()
 * Queries the motor sequencing status (tag 272) and signals the state.
 *****************************************************************************/
int
qry_mot_status()
{
  char * id_str;
  int cmd_stat = SSD_qry_hexword(ser_dev, 272);
  if (cmd_stat >= 0) {
    asprintf(&id_str, "0x%04X", cmd_stat);
    /*     IUSaveText(&SSD_device_infoTP.tp[2], id_str); */
    free(id_str);
    if (cmd_stat != inverter_seq_status) {
      seq_state.ssd_motor_seq= SSD650V_IS_OK ;
      seq_state.ready       = cmd_stat & 0x0001 ? SSD650V_IS_OK : SSD650V_IS_IDLE;
      seq_state.on          = cmd_stat & 0x0002 ? SSD650V_IS_OK : SSD650V_IS_IDLE;
      seq_state.running     = cmd_stat & 0x0004 ? SSD650V_IS_OK : SSD650V_IS_IDLE;
      seq_state.tripped     = cmd_stat & 0x0008 ? SSD650V_IS_OK : SSD650V_IS_IDLE;
      seq_state.coast       = cmd_stat & 0x0010 ? SSD650V_IS_IDLE : SSD650V_IS_OK;
      seq_state.fast_stop   = cmd_stat & 0x0020 ? SSD650V_IS_IDLE : SSD650V_IS_OK;
      seq_state.on_disable  = cmd_stat & 0x0040 ? SSD650V_IS_OK : SSD650V_IS_IDLE;
      seq_state.remote      = cmd_stat & 0x0200 ? SSD650V_IS_OK : SSD650V_IS_IDLE;
      seq_state.sp_reached  = cmd_stat & 0x0400 ? SSD650V_IS_OK : SSD650V_IS_IDLE;
      seq_state.intern_limit= cmd_stat & 0x0800 ? SSD650V_IS_OK : SSD650V_IS_IDLE;

      inverter_seq_status = cmd_stat;
    }
  } else {
    inverter_seq_status = -1;
    /*     SSD_device_infoTP.s = IPS_ALERT; */
    /*     SSD_MotorSeqLP.s = IPS_ALERT; */
    /*     for (i = 0; i < 10; i++) { */
    /*       SSD_MotorSeqLP.lp[i].s = IPS_BUSY; */
    /*     } */
    seq_state.ssd_motor_seq= SSD650V_IS_ALERT ;
    seq_state.ready       = SSD650V_IS_BUSY ;
    seq_state.on          = SSD650V_IS_BUSY ;
    seq_state.running     = SSD650V_IS_BUSY ;
    seq_state.tripped     = SSD650V_IS_BUSY ;
    seq_state.coast       = SSD650V_IS_BUSY ;
    seq_state.fast_stop   = SSD650V_IS_BUSY ;
    seq_state.on_disable  = SSD650V_IS_BUSY ;
    seq_state.remote      = SSD650V_IS_BUSY ;
    seq_state.sp_reached  = SSD650V_IS_BUSY ;
    seq_state.intern_limit= SSD650V_IS_BUSY ;
  }
  return cmd_stat;
}

/******************************************************************************
 * motor_faststop_switch_state()
 * Switches the motor from running (or ready to run) to fast stopping and back
 * to stopped.
 *****************************************************************************/
void
motor_faststop_switch_state()
{
  char* msg;
  char cmd_word[6];
  int mot_stat;
  __attribute__ ((unused)) int res;
  
  mot_stat = qry_mot_cmd();
  if (mot_stat < 0) {
    msg = "failure getting motor commanding status.";
    sys_log(ILOG_ERR, msg); 
    
  } else {
    // wildi ToDo:   if( MotorFastStopSP.sp[0].s == ISS_ON) {
    if (fast_stop_state.set == SSD650V_IS_ON) {
      mot_stat &= 0xfffa;
      sprintf(cmd_word, ">%.4X", mot_stat);
      res = SSD_set_tag(ser_dev, 271, cmd_word);
      msg = "Motor is fast stoping";

      //    } wildi ToDo: else if (MotorFastStopSP.sp[1].s == ISS_ON) {
    } else if (fast_stop_state.reset == SSD650V_IS_ON) {
      mot_stat |= 0x0004;
      sprintf(cmd_word, ">%.4X", mot_stat);
      res = SSD_set_tag(ser_dev, 271, cmd_word);
      msg = "Motor is not fast stoping";

    } else {
      msg = "??? none of the 1-of-many switches is on!";
      sys_log(ILOG_WARNING, msg);
    }
  }
  qry_mot_cmd();
  qry_mot_status();
}

/******************************************************************************
 * motor_coast_switch_state()
 * Switches the motor from running (or ready to run) to coasting and back to
 * stopped.
 *****************************************************************************/
// wildi ToDo: not used here and not planned within RTS2
void
motor_coast_switch_state()
{
  char* msg;
  char cmd_word[6];
  int mot_stat;
  __attribute__ ((unused)) int res;

  mot_stat = qry_mot_cmd();
  if (mot_stat < 0) {
    /*     MotorCoastSP.s = IPS_ALERT; */
    msg = "failure getting motor commanding status.";
    sys_log(ILOG_ERR, msg);

  } else {
    // wildi ToDo if (MotorCoastSP.sp[0].s == ISS_ON) {
    if (coast_state.set == SSD650V_IS_ON) {
      mot_stat &= 0xfffc;
      sprintf(cmd_word, ">%.4X", mot_stat);
      res = SSD_set_tag(ser_dev, 271, cmd_word);
      msg = "Motor is coasting";
      /*       MotorCoastSP.sp[0].s = ISS_ON; */
      /*       MotorCoastSP.sp[1].s = ISS_OFF; */
      coast_state.set  = SSD650V_IS_ON ;
      coast_state.reset= SSD650V_IS_OFF ;

      /*       MotorStartSP.sp[0].s = ISS_OFF; */
      /*       MotorStartSP.sp[1].s = ISS_ON; */
      //motor_run_state.start= SSD650V_IS_OFF ;
      //motor_run_state.stop = SSD650V_IS_ON ;
  
      // wildi ToDo } else if (MotorCoastSP.sp[1].s == ISS_ON) {
    } else if (coast_state.reset == SSD650V_IS_ON) {
      mot_stat |= 0x0002;
      sprintf(cmd_word, ">%.4X", mot_stat);
      res = SSD_set_tag(ser_dev, 271, cmd_word);
      msg = "Motor is not coasting";
      /*       MotorCoastSP.sp[0].s = ISS_OFF; */
      /*       MotorCoastSP.sp[1].s = ISS_ON; */
      coast_state.set  = SSD650V_IS_OFF ;
      coast_state.reset= SSD650V_IS_ON ;

    } else {
      /*       MotorCoastSP.s = IPS_ALERT; */
      msg = "??? none of the 1-of-many switches is on!";
      sys_log(ILOG_WARNING, msg);
    }
  }
  
  /*   IDSetSwitch(&MotorCoastSP, msg); */
  qry_mot_cmd();
  qry_mot_status();
}

/******************************************************************************
 * motor_run_switch_state()
 * Switches the motor from stopped to running or  to stopped.
 *****************************************************************************/
int motor_run_switch_state( int cmd)
{
  int ret ;
  int res;
  char * msg;

/*   float setpoint = SSD_qry_setpoint(ser_dev); */
/*   if (!isnan(setpoint)) { */
/*     // OK */
/*   } else { */
/*     sys_log(ILOG_WARNING, "Failure querying setpoint - aborting motor command"); */
/*     fprintf( stderr, "motor_run_switch_state: SSD650V_MS_GETTING_SET_POINT_FAILED\n") ; */
/*     return SSD650V_MS_GETTING_SET_POINT_FAILED; */
/*   } */

  if ( cmd== SSD650V_CMD_ON) {
    /* msg = "Motor is started"; */
    /* fprintf( stderr, "%s\n", msg) ; */
// wildi ToDo no set point to set here    res = set_setpoint(setpoint);
/*     if (res != BISYNC_OK) { */
/*       sys_log(ILOG_WARNING, "Failure setting setpoint (%d) - motor not started", res); */
/*       fprintf( stderr, "motor_run_switch_state: SSD650V_MS_SETTING_SET_POINT_FAILED %3.1f, error %d\n", setpoint, res) ; */
      
/*       return SSD650V_MS_SETTING_SET_POINT_FAILED; */
/*     } */
    /* start the motor */
    res = SSD_set_tag(ser_dev, 271, ">047F");
    
    coast_state.set  = SSD650V_IS_OFF ;
    coast_state.reset= SSD650V_IS_ON ;

    //qry_mot_cmd();
    //qry_mot_status();

    if( res != BISYNC_OK) {
      return SSD650V_MS_GENERAL_FAILURE ;
    } else {
      return SSD650V_MS_RUNNING ;
    }

  } else if ( cmd  == SSD650V_CMD_OFF) {
    /* msg = "Motor is stopped"; */
    /* fprintf( stderr, "%s\n", msg) ; */
   
    struct timespec sl ;
    struct timespec rsl ;

    sl.tv_sec= 0 ;
    sl.tv_nsec= REPEAT_RATE_NANO_SEC ;
    ret= nanosleep( &sl, &rsl) ;
    if((ret== EFAULT) || ( ret== EINTR)||( ret== EINVAL ))  {
      fprintf( stderr, "Error in nanosleep\n") ;
    }

    res = SSD_set_tag(ser_dev, 271, ">047E");

    coast_state.set  = SSD650V_IS_OFF ;
    coast_state.reset= SSD650V_IS_ON ;
    
    //qry_mot_cmd();
    //qry_mot_status();

    if( res != BISYNC_OK) {
      return SSD650V_MS_GENERAL_FAILURE ;
    } else {
      return SSD650V_MS_STOPPED ;
    }
  } else {
    msg = "------------------ No good value";
    fprintf( stderr, "%s\n", msg) ;
  }
  return SSD650V_MS_GENERAL_FAILURE ;
}

/******************************************************************************
 * difftimeval()
 * Gets difference of two timeval in milli seconds.
 *****************************************************************************/
time_t
difftimeval(struct timeval *t1, struct timeval *t0)
{
  time_t msec;
  msec = (t1->tv_sec - t0->tv_sec) * 1000;
  msec += (t1->tv_usec - t0->tv_usec) / 1000;
  return msec;
}

/******************************************************************************
 * query_all_ssd650_status()
 *****************************************************************************/
void
query_all_ssd650_status(__attribute__ ((unused)) void * user_p)
{
  int old_debug = debug;
  struct timeval time_enq;

  debug -= 1;
  qry_mot_cmd();
  qry_mot_status();
  debug = old_debug;

  /* send a heart beat */
  gettimeofday(&time_enq, NULL);
  if (difftimeval(&time_enq, &time_last_stat) > 500) {
    time_last_stat = time_enq;
    // wildi ToDo if (SSD_MotorSeqLP.s == IPS_IDLE) {
    if (seq_state.ssd_motor_seq == SSD650V_IS_IDLE) {
      seq_state.ssd_motor_seq= SSD650V_IS_OK; 
    } else {
      seq_state.ssd_motor_seq= SSD650V_IS_IDLE; 
    }
  }
}
/******************************************************************************
 * connectSSD650vDevice()
 *****************************************************************************/
int connectSSD650vDevice( int power_state)
{
  int ret ;
  int res = 0;
  int mot_stat;
  char device_ssd650v[]="/dev/ssd650v" ; // azimuth and door devices are the same
  struct timespec sl ;
  struct timespec rsl ;
  int thread_stat_ssd;
  
  switch (power_state) {
  case SSD650V_CMD_CONNECT:
    ser_dev = ssd_start_comm( &device_ssd650v[0]);
    if (ser_dev >= 0) {
      if(( res= motor_off()) != SSD650V_MS_STOPPED) {
	fprintf( stderr, "connectSSD650vDevice: initial  motor_off failed\n") ;
      }
      if(( res= set_setpoint(SETPOINT_ZERO)) != SSD650V_MS_OK) {
	  fprintf( stderr, "connectSSD650vDevice:  initial set_setpoint() failed, setpoint is %4.1f should be %4.1f\n", get_setpoint(), SETPOINT_ZERO) ;
      }
      /*connected: query identity from SSD650V */
      sl.tv_sec= 0 ;
      sl.tv_nsec= REPEAT_RATE_NANO_SEC ;
      ret= nanosleep( &sl, &rsl) ;
      if((ret== EFAULT) || ( ret== EINTR)||( ret== EINVAL ))  {
	fprintf( stderr, "Error in nanosleep\n") ;
      }

      char * id_str = SSD_qry_identity(ser_dev);
      if (id_str) {
	/*           IUSaveText(&SSD_device_infoTP.tp[0], id_str); */
	free(id_str);
      } else {
	fprintf( stderr, "connectSSD650vDevice: SSD651V_IDENTITY_FAILED\n") ;
	fprintf( stderr, "connectSSD650vDevice: -------------------------IGNORING return SSD650V_MS_IDENTITY_FAILED\n") ;
	// return SSD650V_MS_IDENTITY_FAILED ;
      }
      /* query major state from SSD650V indicating powering up and
       * initialisation */
      ret= nanosleep( &sl, &rsl) ;
      if((ret== EFAULT) || ( ret== EINTR)||( ret== EINVAL ))  {
	fprintf( stderr, "Error in nanosleep\n") ;
      }
      id_str = SSD_qry_major_state(ser_dev);
      if (id_str) {
	/*           IUSaveText(&SSD_device_infoTP.tp[1], id_str); */
      } else {
	fprintf( stderr, "connectSSD650vDevice: SSD650V_MS_MAJOR_STATE_FAILED\n") ;
	return SSD650V_MS_MAJOR_STATE_FAILED ;
      }
      /* query the motor sequencing status from SSD650V */
      qry_mot_status();
      /* query last communication error condition from SSD650V
       * includes invalid Mnemonic, BCC error, read from write-only, write to
       * read-only, invalid data, out of range data */
      int last_error = SSD_qry_last_error(ser_dev);
      if (last_error >= 0) {
	asprintf(&id_str, "0x%04X", last_error);
	/*           IUSaveText(&SSD_device_infoTP.tp[4], id_str); */
	free(id_str);
      } else {
	fprintf( stderr, "connectSSD650vDevice: SSD650V_MS_LAST_ERROR_FAILED\n") ;
	return SSD650V_MS_LAST_ERROR_FAILED ;
      }

      /* get current setpoint, accel and decel times */
      
      //float setpoint = SSD_qry_real(ser_dev, 247);
      //Version 5
      float setpoint = SSD_qry_real(ser_dev, 269);

      if (!isnan(setpoint)) {
	/*           IDSetNumber(&motorOperationNP, "setpoint currently is %3.1f", abs_setpoint); */
      } else {
	sys_log(ILOG_WARNING, "Failure querying setpoint");
	fprintf( stderr, "connectSSD650vDevice: SSD650V_MS_GETTING_SET_POINT_FAILED\n") ;
	return SSD650V_MS_GETTING_SET_POINT_FAILED ;
      }

      float accel_tm = SSD_qry_real(ser_dev, 258);
      if (!isnan(accel_tm)) {
	//fprintf( stderr, "connectSSD650vDevice: accel time currently is %3.1f-----------------------------\n", accel_tm);
      } else {
	sys_log(ILOG_WARNING, "Failure querying accel_tm");
	fprintf( stderr, "Failure querying accel_tm-----------------------------\n");
	return SSD650V_MS_GETTING_ACCELERATION_TIME_FAILED ;
      }
      char data[8];
      float decel_tm= 0.5 ; 
      snprintf(data, 8, "%3.1f", decel_tm);
      res = SSD_set_tag(ser_dev, 259, data);

      decel_tm = SSD_qry_real(ser_dev, 259);
      if (!isnan(decel_tm)) {
	  //fprintf( stderr, "connectSSD650vDevice: decel time currently is %3.1f-----------------------------\n", decel_tm);
      } else {
	sys_log(ILOG_WARNING, "Failure querying decel_tm");
	fprintf( stderr, "Failure querying decel_tm-----------------------------\n");
	return SSD650V_MS_GETTING_DECELERATION_TIME_FAILED ;
      }
      /* query the motor control command status from SSD650V */
      mot_stat = qry_mot_cmd();
      if (mot_stat >= 0) {
	if ((mot_stat & 0x0001) != 0) {
	  /*             MotorStartSP.sp[1].s = ISS_OFF; */
	  motor_run_state.stop= SSD650V_IS_OFF ;
	  if (setpoint >= 0) {
	    /*               MotorStartSP.sp[0].s = ISS_ON; */
	    /*               MotorStartSP.sp[2].s = ISS_OFF; */
	    motor_run_state.start= SSD650V_IS_ON ;
	  } else {
	    /*               MotorStartSP.sp[0].s = ISS_OFF; */
	    /*               MotorStartSP.sp[2].s = ISS_ON; */
	    motor_run_state.start= SSD650V_IS_OFF ;
	  }
	} else {
	  /*             MotorStartSP.sp[0].s = ISS_OFF; */
	  /*             MotorStartSP.sp[1].s = ISS_ON; */
	  motor_run_state.start= SSD650V_IS_OFF ;
	  motor_run_state.stop = SSD650V_IS_ON ;
	}
	if ((mot_stat & 0x0002) != 0) {
	  /*             MotorCoastSP.sp[0].s = ISS_OFF; */
	  /*             MotorCoastSP.sp[1].s = ISS_ON; */
	  coast_state.set  = SSD650V_IS_OFF ;
	  coast_state.reset= SSD650V_IS_ON ;
	} else {
	  /*             MotorCoastSP.sp[0].s = ISS_ON; */
	  /*             MotorCoastSP.sp[1].s = ISS_OFF; */
	  coast_state.set  = SSD650V_IS_ON ;
	  coast_state.reset= SSD650V_IS_OFF ;
	}

	if ((mot_stat & 0x0004) != 0) {
	  /*             MotorFastStopSP.sp[0].s = ISS_OFF; */
	  /*             MotorFastStopSP.sp[1].s = ISS_ON; */
	  fast_stop_state.set  = SSD650V_IS_OFF ;
	  fast_stop_state.reset= SSD650V_IS_ON ;
	} else {
	  /*             MotorFastStopSP.sp[0].s = ISS_ON; */
	  /*             MotorFastStopSP.sp[1].s = ISS_OFF; */
	  fast_stop_state.set  = SSD650V_IS_ON ;
	  fast_stop_state.reset= SSD650V_IS_OFF ;
	}
      } else {
	fprintf( stderr, "connectSSD650vDevice: SSD650V_MS_GETTING_MOTOR_COMMAND_FAILED\n") ;
	return SSD650V_MS_GETTING_MOTOR_COMMAND_FAILED ;
      }
    } else {
      fprintf( stderr, "connectSSD650vDevice: SSD650V_MS_CONNECTION_FAILED\n") ;  
      return SSD650V_MS_CONNECTION_FAILED ;
    }
    // set the initial values
    fast_stop_state.set = SSD650V_IS_OFF ;
    fast_stop_state.reset = SSD650V_IS_ON ;
    motor_faststop_switch_state() ;

    coast_state.set  = SSD650V_IS_OFF ;
    coast_state.reset= SSD650V_IS_ON ;
    motor_coast_switch_state() ;

    thread_stat_ssd = pthread_create( &move_door_id, NULL, move_door, NULL) ;
    if (thread_stat_ssd != 0) {
      fprintf( stderr,  "connectSSD650vDevice: failure starting thread: error %d:\n", thread_stat_ssd);
    } 

    fprintf( stderr, "connectSSD650vDevice: connection OK, thread started\n") ;
    return SSD650V_MS_CONNECTION_OK ;
    break;

  case SSD650V_CMD_DISCONNECT:

    thread_stat_ssd = pthread_cancel(move_door_id); // 0 = success
    if( thread_stat_ssd != 0 ) {
      return SSD650V_MS_CONNECTION_FAILED ;
    }
    fprintf( stderr, "connectSSD650vDevice: SSD650v thread stopped\n") ;
    res = ssd_stop_comm();
    if (!res) {
      fprintf( stderr, "connectSSD650vDevice: closed connection\n") ;
      return SSD650V_MS_CONNECTION_OK ;
    } else {
      fprintf( stderr, "connectSSD650vDevice: closing connection failed\n") ;
      return SSD650V_MS_CONNECTION_FAILED ;
    }

    break ;
  default: ;
  }
  return SSD650V_MS_GENERAL_FAILURE;
}
/******************************************************************************
 * ssd_start_comm()
 *****************************************************************************/
int
ssd_start_comm(char * ser_dev_name)
{
  int sd;  /* serial device file descriptor */
  int bitrate  = DEFAULT_SSD650V_BITRATE;
  int databits = DEFAULT_SSD650V_DATABITS;
  int parity   = DEFAULT_SSD650V_PARITY;
  int stopbits = DEFAULT_SSD650V_STOPBITS;

  // open serial ports
  if ((sd = serial_init(ser_dev_name, bitrate, databits,
                        parity, stopbits)) < 0)
  {
    sys_log(ILOG_ERR, "open of %s failed.", ser_dev_name);
    return -1;
  }
  ser_open = 1;
  bisync_init();

  return sd;
}

/******************************************************************************
 * ssd_stop_comm()
 *****************************************************************************/
int
ssd_stop_comm()
{
  int res = 0;
  if (ser_open) {
    res = serial_shutdown(ser_dev);
    sys_debug_log(1, "restored terminal settings on serial port.");
  }
  return res;
}

/******************************************************************************
 * SSD_tag2memonic(...)
 * Translates an SSD650V parameter tag number to the mnemonic id which
 * can be used to specifiy a parameter to set or enquire via BISYNC protocol.
 * mnemonic must point to a char array of at least 3 chars size.
 *****************************************************************************/
void
SSD_tag2mnemonic(int tag, char *mnemonic)
{
  unsigned char m;
  unsigned char n;

  if (tag < 1296) {
    m = tag / 36;
    n = tag % 36;

    if (m > 9) {
      mnemonic[0] = 'a' + m - 10;
    } else {
      mnemonic[0] = '0' + m;
    }
    if (n > 9) {
      mnemonic[1] = 'a' + n - 10;
    } else {
      mnemonic[1] = '0' + n;
    }

  } else {
    m = (tag - 1296) / 126;
    n = (tag - 1296) % 26;
    mnemonic[0] = 'a' + n;
    mnemonic[1] = 'A' + m;
  }
  mnemonic[2] = '\0';
}

/******************************************************************************
 * SSD_chkresp_bool(...)
 * Checks the given response frame for a valid boolean value.
 * The response frame buffer is freed before return.
 * Return:
 *   the data from the reply as a BOOLEAN (0=false, 1=true) or -1 on error.
 *****************************************************************************/
int
SSD_chkresp_bool(__attribute__ ((unused)) int sd, char *response_frame)
{
  BOOLEAN b = FALSE;

  free(response_frame);
  return b;
}

/******************************************************************************
 * SSD_chkresp_tagnbr(...)
 * Checks the given response frame for a valid boolean value.
 * The response frame buffer is freed before return.
 * Return:
 *   the data from the reply as a BOOLEAN (0=false, 1=true) or -1 on error.
 *****************************************************************************/
int
SSD_chkresp_tagnbr(__attribute__ ((unused)) int sd, char *response_frame)
{
  int i = 0;

  free(response_frame);
  return i;
}

/******************************************************************************
 * SSD_chkresp_real(...)
 * Checks the given response frame for a valid real value.
 * The response frame buffer is freed before return.
 * Return:
 *   the data from the reply as a float or NaN on error.
 *****************************************************************************/
float
SSD_chkresp_real(__attribute__ ((unused)) int sd, char *response_frame)
{
  float st_real;

  if (((response_frame[3] < '0') || (response_frame[3] > '9'))
      && (response_frame[3] != '.') && (response_frame[3] != '-'))
  {
    sys_log(ILOG_ERR, "SSD_qry_real(): reply has wrong data type: %s.",
                       &response_frame[3]);
    free(response_frame);
    return sqrt(-1);  /* return NaN */
  }

  int cnt = sscanf(&response_frame[3], "%f", &st_real);
  if (cnt != 1) {
    sys_log(ILOG_ERR, "Expected a real type reply but was: %s.",
                       &response_frame[3]);
    free(response_frame);
    return sqrt(-1);  /* return NaN */
  }

  free(response_frame);
  return st_real;
}

/******************************************************************************
 * SSD_qry_real(...)
 * Queries the SSD650V for the value of the given parameter tag. The queried
 * tag must be of real data type.
 * Return:
 *   the data from the reply as a float or NaN on error.
 *****************************************************************************/
float
SSD_qry_real(int sd, int tag)
{
  int err;
  char *response_frame;
  char mn[3];

  SSD_tag2mnemonic(tag, mn);
  err = bisync_enquiry(sd, 0, 0, mn, &response_frame, ENQUIRY_SINGLE);
  if (err) {
    sys_log(ILOG_ERR, "SSD_qry_real() for mnemo \"%s\" failed (%d: %s).",
                       mn, err, bisync_errstr(err));
    return sqrt(-1);  /* return NaN */
  }

  return SSD_chkresp_real(sd, response_frame);
}

/******************************************************************************
 * SSD_chkresp_integer(...)
 * Checks the given response frame for a valid integer value.
 * The response frame buffer is freed before return.
 * Return:
 *   the data from the reply as an positive integer or -1 on error.
 *****************************************************************************/
int
SSD_chkresp_integer(__attribute__ ((unused)) int sd, char *response_frame)
{
  unsigned int st_int;

  if ((response_frame[3] < '0') || (response_frame[3] > '9')){
    sys_log(ILOG_ERR, "SSD_qry_integer(): reply has wrong data type: %s.",
                       &response_frame[3]);
    free(response_frame);
    return -1;
  }

  int cnt = sscanf(&response_frame[3], "%d.", &st_int);
  if (cnt != 1) {
    sys_log(ILOG_ERR, "Expected an integer reply but was: %s.",
                       &response_frame[3]);
    free(response_frame);
    return -1;
  }

  free(response_frame);
  return st_int;
}

/******************************************************************************
 * SSD_qry_integer(...)
 * Queries the SSD650V for the value of the given parameter tag. The queried
 * tag must be of unsigned integer data type.
 * Return:
 *   the data from the reply as an positive integer or -1 on error.
 *****************************************************************************/
int
SSD_qry_integer(int sd, int tag)
{
  int err;
  char *response_frame;
  char mn[3];

  SSD_tag2mnemonic(tag, mn);
  err = bisync_enquiry(sd, 0, 0, mn, &response_frame, ENQUIRY_SINGLE);
  if (err) {
    fprintf(stderr, "SSD_qry_integer() for tag %d failed (%d: %s)\n",
                    tag, err, bisync_errstr(err));
    return -1;
  }

  return SSD_chkresp_integer(sd, response_frame);
}

/******************************************************************************
 * SSD_chkresp_hexword(...)
 * Checks the given response frame for a valid hex word value.
 * The response frame buffer is freed before return.
 * Return:
 *   the data from the reply as a positive integer or -1 on error.
 *****************************************************************************/
int
SSD_chkresp_hexword(__attribute__ ((unused)) int sd, char *response_frame)
{
  unsigned int st_word;

  if (response_frame[3] != '>') {
    sys_log(ILOG_ERR, "SSD__hexword(): reply has wrong data type: %s.",
                       &response_frame[3]);
    free(response_frame);
    return -1;
  }

  int cnt = strlen(&response_frame[4]);
  if (cnt != 4) {
    sys_log(ILOG_ERR, "SSD__hexword(): hex word reply has wrong "
                       "data length: %s.", &response_frame[4]);
    free(response_frame);
    return -1;
  }

  cnt = sscanf(&response_frame[4], "%X", &st_word);
  if (cnt != 1) {
    sys_log(ILOG_ERR, "Expected a hex word but reply was: %s.",
                       &response_frame[3]);
    free(response_frame);
    return -1;
  }

  free(response_frame);
  return st_word;
}

/******************************************************************************
 * SSD_qry_hexword(...)
 * Queries the SSD650V for the value of the given parameter tag. The queried
 * tag must be of hex word data type.
 * Return:
 *   the data from the reply as a positive integer or -1 on error.
 *****************************************************************************/
int
SSD_qry_hexword(int sd, int tag)
{
  int err;
  char *response_frame;
  char mn[3];
  
  SSD_tag2mnemonic(tag, mn);
  err = bisync_enquiry(sd, 0, 0, mn, &response_frame, ENQUIRY_SINGLE);
  if (err) {
    sys_log(ILOG_ERR, "SSD_qry_hexword() for tag %d failed (%d: %s)",
                    tag, err, bisync_errstr(err));
    return -1;
  }

  return SSD_chkresp_hexword(sd, response_frame);
}

/******************************************************************************
 * SSD_qry_last_error(..)
 * Query the SSD650V for it's last error status.
 * Return:
 *****************************************************************************/
int 
SSD_qry_last_error(int sd)
{
  char *mn;
  int err;
  char *response_frame;
  unsigned int err_word;

  /* query instrument identity */
  mn = "EE";
  err = bisync_enquiry(sd, 0, 0, mn, &response_frame, ENQUIRY_SINGLE);
  if (err) {
    fprintf(stderr, "Query of last error state failed (%d: %s)\n",
                    err, bisync_errstr(err));
    return -1;
  }

  if (response_frame[3] != '>') {
    fprintf(stderr, "SSD_qry_last_error(): reply has wrong data type: %s\n",
                    &response_frame[3]);
    free(response_frame);
    return -1;
  }

  int cnt = sscanf(&response_frame[4], "%X", &err_word);
  if (cnt != 1) {
    fprintf(stderr, "Expected a hex word reply but was: %s\n",
                    &response_frame[3]);
    free(response_frame);
    return -1;
  }
  free(response_frame);

  return err_word;
}

/******************************************************************************
 * SSD_qry_identity(...)
 * Query the SSD650V for it's instrument identity and it's software version.
 * Return:
 *   A newly allocated string describing the connected instrument
 *   or NULL if something is wrong.
 *   (don't forget to free() the returned string buffer, when not needed
 *    anymore.)
 *****************************************************************************/
char *
SSD_qry_identity(int sd)
{
  char *mn;
  int err;
  char *response_frame;
  char *ii_str;
  
  /* query instrument identity */
  mn = "II";
  err = bisync_enquiry(sd, 0, 0, mn, &response_frame, ENQUIRY_SINGLE);
  if (err) {
    fprintf(stderr, "Query of instrument identity failed (%d: %s)\n",
                    err, bisync_errstr(err));
    return NULL;
  }

  if (response_frame[3] != '>') {
    fprintf(stderr, "SSD_qry_identity(): reply has wrong data type: %s\n",
                    &response_frame[3]);
    free(response_frame);
    return NULL;
  }

  if (strcmp("1650", &response_frame[4])) {
    fprintf(stderr, "Not the expected instrument model, reply was: \"%s\"\n",
                    &response_frame[1]);
    free(response_frame);
    return NULL;
  }
  if (debug > 1) {
    fprintf(stderr, "Got instrument identity: \"%s\"\n", &response_frame[4]);
  }
  free(response_frame);
 
  /* query instrument main software version */
  mn = "V0";
  err = bisync_enquiry(sd, 0, 0, mn, &response_frame, ENQUIRY_SINGLE);
  if (err) {
    fprintf(stderr, "Query of instrument main software version failed (%d: %s)\n",
                    err, bisync_errstr(err));
    return NULL;
  }

  if (response_frame[3] != '>') {
    fprintf(stderr, "SSD_qry_identity(): reply has wrong data type: %s\n",
                    &response_frame[3]);
    free(response_frame);
    return NULL;
  }
  if (debug > 1) {
    fprintf(stderr, "Got instrument software version: \"%s\"\n", &response_frame[4]);
  }

  ii_str = concat("SSD650V, ver. ", &response_frame[4], NULL);
  free(response_frame);
  return ii_str;
}

/******************************************************************************
 * SSD_qry_major_state(...)
 * Query SSD650V's major state.
 * Return:
 *   A string describing SSD650V's major state
 *   or NULL if something is wrong.
 *****************************************************************************/
char *
SSD_qry_major_state(int sd)
{
  char *mn;
  int err;
  char *response_frame;
  unsigned int st_word;
  char *ii_str;

  mn = "!2";
  err = bisync_enquiry(sd, 0, 0, mn, &response_frame, ENQUIRY_SINGLE);
  if (err) {
    fprintf(stderr, "Query of SSD650V's major state failed (%d: %s)\n",
                    err, bisync_errstr(err));
    return NULL;
  }

  if (response_frame[3] != '>') {
    fprintf(stderr, "SSD_qry_major_state(): reply has wrong data type: %s\n",
                    &response_frame[3]);
    free(response_frame);
    return NULL;
  }

  int cnt = sscanf(&response_frame[4], "%X", &st_word);
  if (cnt != 1) {
    fprintf(stderr, "SSD_qry_major_state(): Expected a hex word reply "
                    "but was: %s\n", &response_frame[3]);
    free(response_frame);
    return NULL;
  }
  free(response_frame);

  switch (st_word) {
    case 0:
      ii_str = "Initialising";
      break;
    case 1:
      ii_str = "Corrupted product code and configuration";
      break;
    case 2:
      ii_str = "Corrupted configuration";
      break;
    case 3:
      ii_str = "Restoring configuration";
      break;
    case 4:
      ii_str = "Re-configuring mode";
      break;
    case 5:
      ii_str = "Normal operation mode";
      break;
    default:
      ii_str = "unknown (illegal) state";
  }
  return ii_str;
}

/******************************************************************************
 * SSD_qry_comms_status(...)
 * Queries the comm status prameter from the SSD650V via the open serial port
 * given in the serial port device descriptor `ser_dev' using bisync protocol.
 * Return:
 *   positive integer value of the replied 16-bit hex value or negative value
 *   if an error occured.
 *****************************************************************************/
int
SSD_qry_comms_status(int sd)
{
  return SSD_qry_hexword(sd, 272);
}

/******************************************************************************
 * SSD_qry_setpoint(...)
 * Queries the motor speed setpoint from the SSD650V via the open serial port.
 * Return:
 *   value of the current setpoint
 *   or NaN if an error occured.
 *****************************************************************************/
float
SSD_qry_setpoint(int sd)
{
  //return SSD_qry_real(sd, 247);
  // Version 5
  return SSD_qry_real(sd, 269);
}

/******************************************************************************
 * SSD_set_tag(...)
 *****************************************************************************/
int
SSD_set_tag(int sd, int tag, char * data)
{
  char mn[3];
  int err;

  if (strlen(data) > 7) {
    fprintf(stderr, "SSD_set_tag(): data string too long.\n");
    return BISYNC_ERR;
  }

  SSD_tag2mnemonic(tag, mn);
  //sys_debug_log(1, "going to set parameter tag %s to %s.", mn, data);
  err = bisync_setparam(sd, 0, 0, mn, data, SETPARAM_SINGLE);
  if (err) {
    fprintf(stderr, "set param tag %d to \"%s\" failed (%d: %s)\n",
                    tag, data, err, bisync_errstr(err));
    return err;
  }

  return BISYNC_OK;
}

/******************************************************************************
 * SSD_set_mnemo(...)
 *****************************************************************************/
int
SSD_set_mnemo(int sd, char * mn, char * data)
{
  int err;

  if (strlen(data) > 7) {
    fprintf(stderr, "SSD_set_mnemo(): data string too long.\n");
    return BISYNC_ERR;
  }

  //sys_debug_log(1, "going to set parameter tag %s to %s.", mn, data);
  err = bisync_setparam(sd, 0, 0, mn, data, SETPARAM_SINGLE);
  if (err) {
    fprintf(stderr, "set param mnemo \"%s\" to \"%s\" failed (%d: %s)\n",
                    mn, data, err, bisync_errstr(err));
    return err;
  }

  return BISYNC_OK;
}

/******************************************************************************
 * SSD_query(...)
 *****************************************************************************/
int
SSD_query(int sd, tagdata_t *param)
{
  int err = BISYNC_OK;
  char *response_frame;
  char mn[3];

  if (param->param_perm == PARAM_RO) {
    err = BISYNC_ERR;

  } else {
    if (param->cmd_format == CMDF_TAGNBR) {
      SSD_tag2mnemonic(param->command.tag, mn);

    } else if (param->cmd_format == CMDF_MNEMO) {
      memcpy(mn, param->command.mnemo, 3);
    }

    err = bisync_enquiry(sd, 0, 0, mn, &response_frame, ENQUIRY_SINGLE);
    if (err) {
      sys_log(ILOG_ERR, "SSD_query() for mnemo \"%s\" failed (%d: %s)\n",
                          mn, err, bisync_errstr(err));
    }
  }

  switch (param->data_type) {
    case TAGT_REAL:
      param->data.f = SSD_chkresp_real(sd, response_frame);
      break;
    case TAGT_WORD:
      param->data.i = SSD_chkresp_hexword(sd, response_frame);
      break;
    case TAGT_INT:
      break;
    case TAGT_BOOL:
      param->data.b = SSD_chkresp_bool(sd, response_frame);
      break;
    case TAGT_NBR:
      param->data.tag_nbr = SSD_chkresp_tagnbr(sd, response_frame);
      break;
    case TAGT_ENUM:
      break;
  } // switch param->data_type)

  return err;
}

/******************************************************************************
 * SSD_run_cmd_sequence(...)
 *****************************************************************************/
int
SSD_run_cmd_sequence(__attribute__ ((unused)) int sd, __attribute__ ((unused)) tagdata_t * cmd_seq, int cmd_list_sz)
{
  int res = 0;
  int i;
  for (i = 0; i < cmd_list_sz; i++) {
  }
  return res;
}

/******************************************************************************
 * SSD_comm_queue(...)
 * Runs as a 
 *****************************************************************************/
void
SSD_comm_queue()
{
}

/******************************************************************************
 * SSD_setup(...)
 *****************************************************************************/
void
SSD_setup()
{
  /*
  The important commands for using an SSD650V frequency inverter
  (see svn-vermes/experiment/perl/650V_init.sh):
  
  type value   data     group comment

  mnem    !1   >7777    0     # Reset everything
  delay    1   delay    0     #
  mnem    !1   >0101    0     # Restore configuration from non-volatile memory
  delay    3   delay    0     #

  mnem    !1   >5555    0     # Enter configuration mode
  tag    300   >01      1     # Remote comms select
  mnem    !1   >4444    0     # Exit configuration mode

  tag    265   1.       1     # Ref Modes: local only
  tag    247   90.      1     # Local Setpoint
  tag    251   20.      1     # Local Min Speed
  tag    271   >047E    1     # Communications Command: enable drive, reset fault
  tag    299   1.       1     # Power Up Mode: remote

  tag   1159   50.0     2     # Motor Data, Base Frequency
  tag   1160   230.0    2     # Motor Data, Motor Voltage
  tag     64   0.75     2     # Motor Data, Motor Current
  tag     83   1350.0   2     # Motor Data, Nameplate rpm
  tag     84   4.       2     # Motor Data, Motor Poles
  tag   1158   .07      2     # Motor Data, Motor Power
  tag    124   0.       2     # Motor Data, Motor Connection: Delta
  tag    258   5.       3     # Reference Ramp Accel Time
  tag    259   2.       3     # Reference Ramp Decel Time

  mnem    !3   >0001    6     # Save configuration to non-volatile memory

  tag    272   query    4     # Query comms sequencing state

  tag    271   >047E    4     # Communications Sequencing Command: initial comms sequencing setup
  tag    271   >047F    4     # Communications Sequencing Command: run forward
  #
  tag    271   >047E    5     # Communications Sequencing Command: stop
  #
  mnem    EE   >0000   -1     # Reset last error code (when enquiry: get last error code)
  mnem    II   -       -1     # (read only) query instrument identity
  mnem    V0   -       -1     # (read only) query instrument main software version
  mnem    V1   -       -1     # (read only) query instrument keypad software version

  mnem    !4   -       -1     # (read only) query state of non-volatile saving operation:
                              #   >0000: idle; >0001: saving; >0002: failed

  */
}

