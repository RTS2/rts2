/* @file Header file for LX200 telescope driver
*
* @author skub
*/

#define SLEW 		1
#define FIND 		2
#define CENTER		3
#define GUIDE		4
#define NORTH		1
#define EAST		2
#define SOUTH		3
#define WEST		4
#define PORT_TIMEOUT	5

/* Park telescope
*
* @return 0 on succes, otherwise returns error code
*/

int park(void);
