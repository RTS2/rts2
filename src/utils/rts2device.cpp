#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <getopt.h>

#include "status.h"
#include "rts2device.h"

#define MINDATAPORT		5556
#define MAXDATAPORT		5656

int
Rts2DevConn::connectionError ()
{
  if (conn_state == RTS2_CONN_AUTH_PENDING)
    master->authorize (NULL);	// cancel pendig authorization
  return -1;
}

int
Rts2DevConn::command ()
{
  int ret;
  if (getCentraldId () != -1)	// authorized and running
    {
      ret = commandAuthorized ();
      if (ret == DEVDEM_E_HW)
	{
	  sendCommandEnd (DEVDEM_E_HW, "device error");
	  return -1;
	}
      return 0;
    }
  if (isCommand ("auth"))
    {
      int auth_id;
      int auth_key;
      if (paramNextInteger (&auth_id)
	  || paramNextInteger (&auth_key) || !paramEnd ())
	return -2;
      setCentraldId (auth_id);
      setKey (auth_key);
      ret = master->authorize (this);
      if (ret)
	{
	  sendCommandEnd (DEVDEM_E_SYSTEM,
			  "cannot authorize; try again later");
	  return -1;
	}
      conn_state = RTS2_CONN_AUTH_PENDING;
      return -1;
    }
  sendCommandEnd (DEVDEM_E_COMMAND, "buf");
  return -1;
}

int
Rts2DevConn::add (fd_set * set)
{
  if (conn_state == RTS2_CONN_AUTH_PENDING)
    return 0;
  return Rts2Conn::add (set);
}

int
Rts2DevConn::authorizationOK ()
{
  conn_state = RTS2_CONN_AUTH_OK;
  master->sendStatusInfo (this);
  sendCommandEnd (0, "OK authorized");
  return 0;
}

int
Rts2DevConn::authorizationFailed ()
{
  conn_state = RTS2_CONN_AUTH_FAILED;
  sendCommandEnd (DEVDEM_E_SYSTEM, "authorization failed");
  return 0;
}

void
Rts2DevConn::setHavePriority (int in_have_priority)
{
  if (havePriority () != in_have_priority)
    {
      Rts2Conn::setHavePriority (in_have_priority);
      if (havePriority ())
	{
	  send ("S priority 1 priority received");
	}
      else
	{
	  send ("S priority 0 priority lost");
	}
    }
}

Rts2DevConnMaster::Rts2DevConnMaster (Rts2Block * in_master,
				      int in_device_port,
				      char *in_device_name,
				      int in_device_type,
				      char *in_master_host,
				      int in_master_port):
Rts2Conn (-1, in_master)
{
  device_port = in_device_port;
  strncpy (device_name, in_device_name, DEVICE_NAME_SIZE);
  device_type = in_device_type;
  strncpy (master_host, in_master_host, HOST_NAME_MAX);
  master_port = in_master_port;
  auth_conn = NULL;
}

int
Rts2DevConnMaster::registerDevice ()
{
  char *msg;
  char device_host[HOST_NAME_MAX];
  int ret;
  ret = gethostname (device_host, HOST_NAME_MAX);
  if (ret < 0)
    return -1;
  asprintf (&msg, "register %s %i %s %i", device_name, device_type,
	    device_host, device_port);
  ret = send (msg);
  free (msg);
  return ret;
}

int
Rts2DevConnMaster::init ()
{
  struct hostent *master_info;
  struct sockaddr_in address;
  int ret;
  // init sock address
  address.sin_family = AF_INET;
  address.sin_port = htons (master_port);
  // get remote host
  if ((master_info = gethostbyname (master_host)))
    {
      address.sin_addr = *(struct in_addr *) master_info->h_addr;
    }
  else
    {
      return -1;
    }
  // get hostname
  if (ret < 0)
    return -1;
  sock = socket (PF_INET, SOCK_STREAM, 0);
  if (sock < 0)
    return -1;
  ret = connect (sock, (struct sockaddr *) &address, sizeof (address));
  if (ret < 0)
    {
      close (sock);
      sock = -1;
      return -1;
    }
  // have to wait for reply
  return registerDevice ();
}

int
Rts2DevConnMaster::command ()
{
  if (isCommand ("authorization_ok"))
    {
      if (auth_conn)
	{
	  int auth_id;
	  if (paramNextInteger (&auth_id))
	    return -1;
	  if (auth_conn->getCentraldId () != auth_id)
	    return -1;
	  auth_conn->authorizationOK ();
	  auth_conn = NULL;
	  return -1;
	}
      return 0;
    }
  else if (isCommand ("authorization_failed"))
    {
      if (auth_conn)
	{
	  int auth_id;
	  if (paramNextInteger (&auth_id))
	    return -1;
	  if (auth_conn->getCentraldId () != auth_id)
	    return -1;
	  auth_conn->authorizationFailed ();
	  auth_conn = NULL;
	  return -1;
	}
      return 0;
    }
  return -2;
}

int
Rts2DevConnMaster::message ()
{
  char *msg;
  if (paramNextString (&msg))
	  return -1;
  if (!strcmp (msg, "priority_change"))
    {
      // change priority
      int priority_client;
      int timeout;
      if (paramNextInteger (&priority_client))
	return -1;
      if (paramNextInteger (&timeout))
	return -1;
      master->setPriorityClient (priority_client, timeout);
      return -1;
    }
  return -2;
}

int
Rts2DevConnMaster::authorize (Rts2DevConn * conn)
{
  char *msg;
  int ret;
  if (auth_conn)
    {
      if (!conn)
	auth_conn = NULL;	// request for canceling authorization
      return -1;		// authorization already pending, cannot authorize second device at same time
    }
  auth_conn = conn;
  asprintf (&msg, "authorize %i %i", auth_conn->getCentraldId (),
	    auth_conn->getKey ());
  ret = send (msg);
  free (msg);
  return ret;
}

int
Rts2DevConnData::init ()
{
  // find empty port
  sock = socket (PF_INET, SOCK_STREAM, 0);
  if (sock < 0)
    return -1;

  struct sockaddr_in server;
  int test_port;

  server.sin_family = AF_INET;
  server.sin_addr.s_addr = htonl (INADDR_ANY);

  // find empty port
  for (test_port = MINDATAPORT; test_port < MAXDATAPORT; test_port++)
    {
      server.sin_port = htons (test_port);
      if (bind (sock, (struct sockaddr *) &server, sizeof (server)) == 0)
	break;
    }
  if (test_port == MAXDATAPORT)
    {
      close (sock);
      sock = -1;
      return -1;
    }

  if (listen (sock, 1))
    {
      close (sock);
      sock = -1;
      return -1;
    }

  setPort (test_port);

  conn_state = 1;
  return 0;
}

int
Rts2DevConnData::send (char *message)
{
  return 0;
}

int
Rts2DevConnData::send (char *data, size_t data_size)
{
  if (conn_state != 0)
    return -1;
  return write (sock, data, data_size);
}

int
Rts2DevConnData::sendHeader ()
{

}

int
Rts2DevConnData::acceptConn ()
{
  int new_sock;
  struct sockaddr_in other_side;
  socklen_t addr_size = sizeof (struct sockaddr_in);
  new_sock = accept (sock, (struct sockaddr *) &other_side, &addr_size);
  if (new_sock == -1)
    {
      syslog (LOG_ERR, "Rts2DevConnData::acceptConn error %m");
      return -1;
    }
  else
    {
      close (sock);
      sock = new_sock;
      syslog (LOG_DEBUG, "Rts2DevConnData::acceptConn connection accepted");
      conn_state = 2;
      return 0;
    }
}

void
Rts2State::setState (int new_state, char *description)
{
  state = new_state;
  syslog (LOG_DEBUG, "Rts2State::setState new_state: %i desc: %s this: %p", new_state, description, this);
  master->sendStatusMessage (state_name, state);
};

void
Rts2State::maskState (int state_mask, int new_state, char *description)
{
  state &= !state_mask;
  state |= new_state;
  setState (state, description);
}

int
Rts2State::sendInfo (Rts2Conn * conn, int state_num)
{
  int ret;
  char *msg;
  asprintf (&msg, "I status %i %s", state_num, state_name);
  ret = conn->sendValue (msg, state);
  free (msg);
  return ret;
}

Rts2Device::Rts2Device (int argc, char **argv, int device_type, int default_port, char *default_name):Rts2Block
  ()
{
  int
    c;
  int
    device_port = default_port;
  char *
    device_name = default_name;
  char *
    device_file = NULL;

  char *
    centrald_host = "localhost";
  int
    centrald_port = 5557;

  int
    deamonize = 1;

  int
    log_option = 0;

  /* get attrs */
  while (1)
    {
      static struct option
	long_option[] = {
	{"port", 1, 0, 'p'},
	{"interactive", 0, 0, 'i'},
	{"centrald_host", 1, 0, 's'},
	{"centrald_port", 1, 0, 'q'},
	{"device_name", 1, 0, 'd'},
	{"device_file", 1, 0, 'f'},
	{"log_stderr", 1, 0, 'e'},
	{"help", 0, 0, 'h'},
	{0, 0, 0, 0}
      };

      c = getopt_long (argc, argv, "p:is:q:d:f:eh", long_option, NULL);

      if (c == -1)
	break;

      switch (c)
	{
	case 'p':
	  device_port = atoi (optarg);
	  break;
	case 'i':
	  deamonize = 0;
	  break;
	case 's':
	  centrald_host = optarg;
	  break;
	case 'q':
	  centrald_port = atoi (optarg);
	  break;
	case 'd':
	  device_name = optarg;
	  break;
	case 'f':
	  device_file = optarg;
	  break;
	case 'e':
	  log_option |= LOG_PERROR;
	  break;
	case 0:
	  help ();
	  exit (EXIT_SUCCESS);
	case '?':
	  break;
	default:
	  printf ("?? getopt returned unknow character %o ??\n", c);
	}
    }

  openlog (NULL, log_option, LOG_LOCAL0);
      
  setPort (device_port);

  conn_master =
    new Rts2DevConnMaster (this, device_port, device_name, device_type,
			   centrald_host, centrald_port);
  connections[0] = conn_master;
  statesSize = 0;
  states = NULL;
}

Rts2Device::~Rts2Device (void)
{
  int i;
  for (i = 0; i < statesSize; i++)
    delete states[i];
  free (states);
}

void
Rts2Device::help ()
{
	  printf
	    ("Options:\n"
	     "\tport|p <port_num>                port to listen for request\n"
	     "\tinteractive|i                    run in interactive mode, don't loose console\n"
	     "\tcentrald_host|s <hostname>       name of computer, on which central server runs\n"
	     "\tcentrald_port|q <port_num>       port number of central host\n"
	     "\tdevice_name|d <device_name>      name of device\n"
	     "\tdevice_file|f <file>             file to read device\n"
	     "\tlog_stderr|e			 logs also to stderr (not only to syslogd)\n"
	     "\thelp|h                           write this help\n");
}

void
Rts2Device::setStateNames (int in_states_size, char **states_names)
{
  if (in_states_size == 0)
    return;

  int i;
  char *state_name = *states_names;
  syslog (LOG_DEBUG, "Rts2Device::setStateNames states: %i\n", in_states_size);
  states = (Rts2State **) malloc (sizeof (Rts2State *) * in_states_size);
  for (i = 0; i < in_states_size; i++)
    {
      states[i] = new Rts2State (this, state_name);
      state_name++;
    }
  statesSize = in_states_size;
}

int
Rts2Device::changeState (int state_num, int new_state, char *description)
{
  states[state_num]->setState (new_state, description);
  return 0;
}

int
Rts2Device::maskState (int state_num, int state_mask, int new_state,
		       char *description)
{
  syslog (LOG_DEBUG, "Rts2Device::maskState state: %i state_mask: %i new_state: %i desc: %s", state_num,
	  state_mask, new_state, description);
  states[state_num]->maskState (state_mask, new_state, description);
}

int
Rts2Device::init ()
{
  while (connections[0]->init () < 0)
    {
      syslog (LOG_DEBUG, "Rts2Device::init waiting for master");
      sleep (60);
    }
  return Rts2Block::init ();
}

int
Rts2Device::idle ()
{
  Rts2Block::idle ();
}

int
Rts2Device::authorize (Rts2DevConn * conn)
{
  return conn_master->authorize (conn);
}

int
Rts2Device::sendStatusInfo (Rts2DevConn * conn)
{
  int i;
  int ret;

  // last state holds priority info
  ret = conn->sendValue ("I status_num", statesSize + 1);

  if (ret)
    return ret;
  for (i = 0; i < statesSize; i++)
    {
      ret = states[i]->sendInfo (conn, i);
      if (ret)
	return ret;
    }
  return conn->sendPriorityInfo (i);
}
