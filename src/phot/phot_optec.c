/*! 
 * @file Photometer deamon. 
 * $Id$
 * @author petr
 */

#define _GNU_SOURCE

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/io.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <syslog.h>
#include <mcheck.h>
#include <getopt.h>
#include <time.h>

#include "phot_info.h"
#include "../utils/hms.h"
#include "../utils/devdem.h"
#include "status.h"

#define SERVERD_PORT    	5557	// default serverd port
#define SERVERD_HOST		"localhost"	// default serverd hostname

#define DEVICE_NAME		"PHOT"
#define DEVICE_PORT		5559	// default camera TCP/IP port

int base_address = 0x300;

int initiate = 0;

void
phot_init ()
{
  if (initiate)
    return;
  initiate = 1;
  iopl (3);
  // timer 0 to generate a square wave
  outb (54, base_address + 7);
  sleep (1);
  // timer 1 to operate as a digital one shot
  outb (114, base_address + 7);
  sleep (1);
  // timer 2 to operate as an event counter
  outb (176, base_address + 7);
  sleep (1);
  // write divide by 2 to timer 0 to generate 1kHz square wave, LSB and then MSB
  outb (2, base_address + 4);
  sleep (1);
  outb (0, base_address + 4);
  sleep (1);
  // write 1000 to Timer 1 to generate a 1 second pulse, LSB and then MSB
  outb (1000 % 256, base_address + 5);
  sleep (1);
  outb (1000 / 256, base_address + 5);
  // set ouput port bit #0 to 0 bit #1 to 1 and bit #2 to 0
  sleep (1);
  outb (2, base_address + 1);
}

void
phot_home_filter ()
{
  int i;
  // reset stepper motor controller
  outb (0, base_address + 1);
  outb (2, base_address + 1);
  // move filter rack enough steps to put slider against cover wall and stall motor
  for (i = 0; i < 150; i++)
    {
      // each change takes 0.5 sec
      sleep (1);
      // move stepper motor one step in direction 1
      outb (3, base_address);
      outb (2, base_address);
    }
}

void
clean_integrate_cancel (void *agr)
{
  devdem_status_mask (0, PHOT_MASK_INTEGRATE, PHOT_NOINTEGRATE,
		      "Integration canceled");
}

void *
start_integrate (void *arg)
{
  int it_t;			// integration time in 0.0001 seconds
  int b, old_b = 0;
  int frequency = 0;
  iopl (3);
  // change period in seconds to and integer times 1000
  it_t = 1000 * *((float *) arg);
  // write integration time to timer 1, LSB and then MSB
  printf ("integration time:%i\n", it_t);
  outb (it_t % 256, base_address + 5);
  sleep (1);
  outb (it_t / 256, base_address + 5);

  // clear timer 2 counter
  outb (255, base_address + 6);
  sleep (1);
  outb (255, base_address + 6);

  // strobe bit 2 of 4-bit control port to initiate counting
  outb (4, base_address);
  sleep (1);
  outb (0, base_address);

  // wait for finish
  sleep (1);

  // value readout
  // poll timer 1 status until bit 7 is 1 indicating that the count is disabled
  do
    {
      outb (228, base_address + 7);
      sleep (1);
      b = inb (base_address + 5);
      if (b != old_b)
	{
	  printf ("b: %x\n", b);
	  old_b = b;
	}
    }
  while ((b & 128) != 128);
  // check if any pulses were received thus loading timer 2's counter with a valid count
  outb (232, base_address + 7);
  b = inb (base_address + 6);
  printf ("b: %x\n", b);
  if ((b & 64) == 0)
    {
      printf ("valid frequency\n");
      // if the counter's content is valid, read it in
      frequency = inb (base_address + 6) + inb (base_address + 6) * 256;
      frequency = 65536 - frequency;
    }
  devser_dprintf ("frequency %i\n", frequency);
  devdem_status_mask (0, PHOT_MASK_INTEGRATE, PHOT_NOINTEGRATE,
		      "Integration finished");
  return NULL;
}

/*! 
 * Handle photometer command.
 *
 * @param command	command to perform
 * 
 * @return -2 on exit, -1 and set errno on HW failure, 0 when command
 * was successfully performed
 */
int
phot_handle_command (char *command)
{
  int ret;

  if (strcmp (command, "ready") == 0)
    {
      phot_init ();
      ret = errno = 0;
    }

  else if (strcmp (command, "info") == 0)
    {
      phot_init ();
      ret = errno = 0;
    }
  else if (strcmp (command, "home") == 0)
    {
      phot_init ();
      phot_home_filter ();
      ret = errno = 0;
    }
  else if (strcmp (command, "integrate") == 0)
    {
      float it;			// integration time in seconds
      if (devser_param_test_length (1))
	return -1;
      if (devser_param_next_float (&it))
	return -1;
      if (devdem_priority_block_start ())
	return -1;
      devdem_status_mask (0, PHOT_MASK_INTEGRATE, PHOT_INTEGRATE,
			  "Integration started");
      if ((ret =
	   devser_thread_create (start_integrate, (void *) &it, sizeof it,
				 NULL, clean_integrate_cancel)) == -1)
	{
	  devdem_status_mask (0, PHOT_MASK_INTEGRATE, PHOT_NOINTEGRATE,
			      "thread create error");
	}
      devdem_priority_block_end ();
      return ret;
    }

  else if (strcmp (command, "help") == 0)
    {
      devser_dprintf ("ready - is dome ready");
      devser_dprintf ("info - dome informations");
      devser_dprintf ("exit - exit from main loop");
      devser_dprintf ("help - print, what you are reading just now");
      ret = errno = 0;
    }
  else
    {
      devser_write_command_end (DEVDEM_E_COMMAND, "Unknow command: '%s'",
				command);
      return -1;
    }
  return ret;
}

int
main (int argc, char **argv)
{
  char *stats[] = { "phot" };

  char *serverd_host = SERVERD_HOST;
  uint16_t serverd_port = SERVERD_PORT;

  char *device_name = DEVICE_NAME;
  uint16_t device_port = DEVICE_PORT;

  char *hostname = NULL;
  int c;

#ifdef DEBUG
  mtrace ();
#endif

  /* get attrs */
  while (1)
    {
      static struct option long_option[] = {
	{"phot_port", 1, 0, 'l'},
	{"port", 1, 0, 'p'},
	{"serverd_host", 1, 0, 's'},
	{"serverd_port", 1, 0, 'q'},
	{"device_name", 1, 0, 'd'},
	{"help", 0, 0, 0},
	{0, 0, 0, 0}
      };
      c = getopt_long (argc, argv, "l:p:s:q:d:h", long_option, NULL);

      if (c == -1)
	break;

      switch (c)
	{
	case 'l':
	  device_port = atoi (optarg);
	  break;
	case 'p':
	  base_address = atoi (optarg);
	  break;
	case 's':
	  serverd_host = optarg;
	  break;
	case 'q':
	  serverd_port = atoi (optarg);
	  break;
	case 'd':
	  device_name = optarg;
	  break;
	case 0:
	  printf ("Options:\n");
	  printf ("\tphot_port|l <port_num>\t\tdevice TCP/IP port\n");
	  printf ("\tport|p <port_num>\t\tPhotometer IO port base\n");
	  printf ("\tserverd_host|s <port_num>\t\thostname of the serverd\n");
	  printf ("\tserverd_port|q <port_num>\t\tport of the serverd\n");
	  printf ("\tdevice_name|d <device_name>\t\tdevice name\n");
	  printf ("\thelp\t\tprint this help and exits\n");
	  exit (EXIT_SUCCESS);
	case '?':
	  break;
	default:
	  printf ("?? getopt returned unknow character %o ??\n", c);
	}
    }

  if (optind != argc - 1)
    {
      printf ("hostname wasn't specified\n");
      exit (EXIT_FAILURE);
    }

  hostname = argv[argc - 1];

  // open syslog
  openlog (NULL, LOG_CONS | LOG_PID | LOG_NDELAY, LOG_LOCAL1);

  if (devdem_init (stats, 1, NULL))
    {
      syslog (LOG_ERR, "devdem_init: %m");
      return EXIT_FAILURE;
    }

  if (devdem_register
      (serverd_host, serverd_port, device_name, DEVICE_TYPE_UNKNOW, hostname,
       device_port) < 0)
    {
      perror ("devser_register");
      devdem_done ();
      return EXIT_FAILURE;
    }

  devdem_run (device_port, phot_handle_command);

  return EXIT_SUCCESS;
}
