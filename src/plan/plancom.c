/*! 
 * @file Commander interface
 *
 * Thats's unit responsibility is to handle planer commands. It takes command,
 * parse it for device/destinantion, send it to that device or execute it in
 * local content, and handle any responses.
 *
 * @author petr
 */

#define _GNU_SOURCE

#include <netinet/in.h>
#include <sys/socket.h>
#include <syslog.h>
#include <argz.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <malloc.h>
#include <stdlib.h>

#define NAMESPACE_LENGTH	8
#define MAX_NAMESPACE		20

#include "plancom.h"
#include "../status.h"
#include "../utils/devcli.h"

#include "../writers/imghdr.h"
#include "../writers/fits.h"
#include "fitsio.h"

struct net_namespace_t
{
  char name[NAMESPACE_LENGTH];
};

struct net_namespace_t namespaces[MAX_NAMESPACE];
size_t namespaces_size = 0;

int file = 0;

int
handle_return (char *argv, size_t argc)
{
  char *temp_argv = argv;
  printf ("Handling return:");
  do
    {
      printf ("%s ", temp_argv);
    }
  while ((temp_argv = argz_next (argv, argc, temp_argv)));

  printf ("\n");
  {
    printf ("unknow command\n");
  }
  return 0;
}

/*!
 * Internal, used to process single network command.
 * Gets parsed command and data handlers.
 *
 * @param comamnd	devcli command
 */

int
process_net_command (char *command)
{
  int ret;
  int i;
  if (devcli_execute (command, &ret) < 0)
    {
      return PLANCOMM_E_HOSTACCES;
    }
  return 0;
}

int
split_namespace (char *ns_command, char **ns, char **command)
{
  int i;
  *ns = *command = ns_command;
  //split command and namespace
  for (i = 0; i < NAMESPACE_LENGTH && **command; i++, (*command)++)
    {
      if (**command == '.')
	{
	  **command = 0;
	  break;
	}
    }
  if (i < NAMESPACE_LENGTH)
    (*command)++;
#ifdef DEBUG
  printf (" namespace: %s i: %i command: %s ", *ns, i, *command);
#endif
  return 0;
}

/*!
 * Process command. It must not contain any local variables references and so.
 * As it's passed to function, it's passed to device driver.
 * 
 * @param command command to execute, with namespace.
 * @return command return, <0 on errors
 */

int
plancom_process_command (char *ns_command)
{
  char *ns;
  char *command;

  split_namespace (ns_command, &ns, &command);

  // system default namespaces
  if (strncmp (ns, "RECEIVE", NAMESPACE_LENGTH) == 0)
    {
      // we got request to handle file receiving
      // syntax is:
      // RECEIVE.<file_type> <filename> <command>
      // Where file_type is system default, filename and commands are
      // defined in string.
      // So we only ought to find end of filename, and send the rest to
      // server as command with specified data recieving handler.
      char *args = command;
      while (*args && *args != ' ' && *args != '\t')
	args++;
      if (!(*args))
	return PLANCOMM_E_PARAMSNUM;
      *args = 0;
      args++;
      while (*args == ' ' || *args == '\t')
	args++;
      if (!(*args))
	return PLANCOMM_E_PARAMSNUM;
      if (strcmp (command, "fits") == 0)
	{
	  struct fits_receiver_data_t *rec_data;
	  char *filename;
	  int status = 0;

	  rec_data =
	    (struct fits_receiver_data_t *)
	    malloc (sizeof (struct fits_receiver_data_t));
#define fits_call(call) if (call) fits_report_error(stderr, status);

	  if (!rec_data)
	    {
	      perror ("malloc recdata");
	      exit (EXIT_FAILURE);
	    }

	  // init receiver structure
	  rec_data->offset = 0;
	  rec_data->header_processed = 0;

	  fits_clear_errmsg ();

	  filename = args;
	  args++;
	  while (*args && *args != ' ' && *args != '\t')
	    args++;
	  if (!(*args))
	    return PLANCOMM_E_PARAMSNUM;
	  *args = 0;

	  fits_call (fits_create_file (&rec_data->ffile, filename, &status));

	  args++;
	  while (*args == ' ' || *args == '\t')
	    args++;

	  split_namespace (args, &ns, &command);

	  return process_net_command (ns, command, fits_handler, rec_data);
	}
      return PLANCOMM_E_COMMAND;
    }

  // otherwise do normal processing
  return process_net_command (ns, command, NULL, NULL);
}
