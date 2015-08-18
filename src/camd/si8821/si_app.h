/* 

app header  for
Spectral Instruments Camera Interface

Copyright (C) 2006  Jeffrey R Hagen
Copyright (C) 2015  Petr Kubánek

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

*/

/*
  application specific support for the si8821
  used by test_app,  and gui
*/

#define SI_STATUS_MAX                 16
#define SI_CONFIG_MAX                 32
#define SI_READOUT_MAX                32
#define SI_READSPEED_MAX               8

#define CFG_TYPE_NOTUSED               0 
#define CFG_TYPE_INPUTD                1 
#define CFG_TYPE_DROPDI                8 
#define CFG_TYPE_DROPDP                9 
#define CFG_TYPE_BITF                  3 

#define STATUS_CCD_TEMP_IX             0
#define STATUS_BACKPLATE_TEMP_IX       1
#define STATUS_PRESSURE_IX             2
#define STATUS_SHUTTER_IX              8
#define STATUS_COOLER_IX              10

#define READOUT_SERIAL_ORIGIN_IX       0
#define READOUT_SERIAL_LENGTH_IX       1
#define READOUT_SERIAL_BINNING_IX      2
#define READOUT_SERIAL_POSTSCAN_IX     3
#define READOUT_PARALLEL_ORIGIN_IX     4
#define READOUT_PARALLEL_LENGTH_IX     5
#define READOUT_PARALLEL_BINNING_IX    6
#define READOUT_PARALLEL_POSTSCAN_IX   7
#define READOUT_EXPOSURE_IX            8
#define READOUT_CCLEAR_IX              9
#define READOUT_DSI_SAMPLE_IX         10
#define READOUT_AATTENUATION_IX       11
#define READOUT_PORT1_OFFSET_IX       12
#define READOUT_PORT2_OFFSET_IX       13
#define READOUT_TDIDELAY_IX           12
#define READOUT_SAMPLEPIX_IX          20


#define CONFIG_SERIAL_PHASING_IX       3
#define CONFIG_SERIAL_SPLIT_IX         4
#define CONFIG_SERIAL_SIZE_IX          5
#define CONFIG_PARALLEL_PHASING_IX     6
#define CONFIG_PARALLEL_SPLIT_IX       7
#define CONFIG_PARALLEL_SIZE_IX        8
#define CONFIG_PARALLEL_SHIFTD_IX      9
#define CONFIG_PORTS_IX               10
#define CONFIG_SHUTTER_CLOSE_D_IX     11
#define CONFIG_SET_TEMP_IX            14

struct CFG_ENTRY {
  char *name;
  int index;
  char *cfg_string;
  int type;
  int security;

  union { 

    struct IOBOX {
      int min;
      int max;
      char *units;
      double mult;
      double offset;
      int status;
    }iobox;

    struct DROPDOWN {
      int min;
      int max;
      char **list;
    }dropi;

    struct DROPDOWNP {
      int val;
      char **list;
    }dropp;

    struct BITFIELD {
      unsigned int mask;
      char **list;
    }bitf;
  }u;
};


struct SI_DINTERLACE {
  int interlace_type;  /* kind of image */
  int n_cols;          /* input cols */
  int n_rows;          /* input rows */
  int n_ptr_pos;       /* output words transferred */
};


struct SI_CAMERA {
  int fd;               /* open device */
  int dma_active;
  unsigned short *ptr;  /* mmaped data */
  struct SI_DMA_STATUS dma_status;
  struct SI_DMA_CONFIG dma_config;

  int status[SI_STATUS_MAX]; /* values returned from camera via uart */
  int config[SI_CONFIG_MAX];
  int readout[SI_READOUT_MAX];
  int read_speed[SI_READSPEED_MAX];

  struct CFG_ENTRY *e_status[SI_STATUS_MAX]; /* parsed out cfg file */
  struct CFG_ENTRY *e_config[SI_CONFIG_MAX];
  struct CFG_ENTRY *e_readout[SI_READOUT_MAX];
  struct CFG_ENTRY *e_readspeed[SI_READSPEED_MAX];

  int fill_done; /* initial fill complete */
  double fraction;

  struct SI_DINTERLACE dinter;

  int dma_done;
  int dma_done_handle;
  int dma_configed;
  int dma_aborted;

  int command;
  int contin;
  char *fname;
  unsigned short *flip_data;
  pthread_t fill;
  int side;
};
