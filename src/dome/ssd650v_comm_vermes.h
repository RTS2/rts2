/*
 *
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

 *
 */

#ifndef __ssd650v_comm_h__
#define __ssd650v_comm_h__


// default serial port device and it's default settings
#define DEFAULT_SERPORT_DEV      "/dev/ssd650v"
#define DEFAULT_BITRATE          19200
#define DEFAULT_DATABITS         7
#define DEFAULT_PARITY           PARITY_EVEN
#define DEFAULT_STOPBITS         1


static int ser_dev;
static int ser_open = 0;
static int inverter_seq_status = -1;
static int mot_cmd_status = -1;
static struct timeval time_last_stat;   /* time of last status enquiry sent to
                                         * SSD650V */
// wildi ToDo INDI replacements
#define ISS_ON 0
#define ISS_OFF 1 
#define IPS_IDLE 1 

// SSD650V states
#define SSD650V_ON    0 // active
#define SSD650V_OFF   1 // inactive
#define SSD650V_IDLE  2 
#define SSD650V_OK    3 
#define SSD650V_BUSY  4 
#define SSD650V_ALERT 5 

// SSD650v frequency inverter states
//
//MotorFastStopSP.sp[0].s "MOTOR_FASTSTOP_SET" "Motor Fast Stop"
//MotorFastStopSP.sp[1].s "MOTOR_FASTSTOP_RESET" "Reset Motor Fast Stop"
struct ssd650v_fast_stop_state {
int set ;
int reset ;
} ;
static struct ssd650v_fast_stop_state fast_stop_state ;

// MotorStartSP.sp[0].s "START_CW" "Start CW" (clockwise)
// MotorStartSP.sp[1].s "STOP" "Stop"
// MotorStartSP.sp[2].s "START_CCW" "Start CCW" (counter clockwise)
struct ssd650v_motor_state {
int start_cw ;
int stop ;
int start_ccw ;
} ;
static struct ssd650v_motor_state motor_state ;

// MotorCoastSP.sp[0].s "MOTOR_COAST_SET" "Motor Coast"
// MotorCoastSP.sp[1].s "MOTOR_COAST_RESET" "Reset Motor Coast"
struct ssd650v_coast_state {
int set ;
int reset ;
} ;
static struct ssd650v_coast_state coast_state ;

//SSD_MotorSeqLP.lp[0].s "SEQ_READY" "Inverter ready"
//SSD_MotorSeqLP.lp[1].s "SEQ_ON" "Inverter on
//SSD_MotorSeqLP.lp[2].s "SEQ_RUNNING" "Motor running"
//SSD_MotorSeqLP.lp[3].s "SEQ_TRIPPED" "Motor tripped"
//SSD_MotorSeqLP.lp[4].s "SEQ_COAST" "Motor coasting"
//SSD_MotorSeqLP.lp[5].s "SEQ_FAST_STOP" "Motor fast stoping"
//SSD_MotorSeqLP.lp[6].s "SEQ_ON_DISABLE" "Inverter on inhibited"
//SSD_MotorSeqLP.lp[7].s "SEQ_REMOTE" "Inverter remotely controlled"
//SSD_MotorSeqLP.lp[8].s "SEQ_SP_REACHED" "Motor setpoint reached" 
//SSD_MotorSeqLP.lp[9].s "SEQ_INTERN_LIMIT" "Motor current limit"
struct ssd650v_seq_state {
int ssd_motor_seq ; // SSD650V_IDLE, SSD650V_OK
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

#endif   // #ifndef __ssd650v_comm_h__

