#include <stdio.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>

#define MSG_SIZE		50

//! devser_2dever message typ
struct devser_msg
{
  long mtype;
  char mtext[MSG_SIZE];
}
msg;

int
main (char argc, char **argv)
{
  int que;

  while (*argv)
    {
      que = atoi (*argv);
      printf ("reading que %i\n", que);
      while (msgrcv (que, &msg, MSG_SIZE, 0, IPC_NOWAIT) != -1)
	{
	  printf ("id: %i text: '%s'\n", msg.mtype, msg.mtext);
	}
      argv++;
    }
  return 0;
}
