#ifndef __RTS_DEVHND__
#define __RTS_DEVHND__

#include "camera_info.h"
#include "telescope_info.h"
#include "param.h"
#include "../status.h"

union devhnd_info
{
  struct telescope_info telescope;
  struct camera_info camera;
};

typedef int (*devcli_handle_response_t) (struct param_status * params,
					 union devhnd_info * devhnd_info);
typedef int (*devcli_handle_data_t) (int sock, size_t size, void *data);

struct devcli_channel_handlers
{
  devcli_handle_response_t command_handler;	//! handler to handle responses
  devcli_handle_response_t message_handler;	//! handler to asynchoronous messages
  devcli_handle_data_t data_handler;		//! handler to ANY received data
  void *data;					//! data about data
};

struct supp_info
{
  devcli_handle_response_t command_handler;	//! handler to handle responses
  devcli_handle_response_t message_handler;	//! handler to asynchoronous messages
};

int telescope_command_handler (struct param_status *params,
			       struct telescope_info *info);
int telescope_message_handler (struct param_status *params,
			       struct telescope_info *info);

int camera_command_handler (struct param_status *params,
			    struct camera_info *info);
int camera_message_handler (struct param_status *params,
			    struct camera_info *info);
int camera_data_handler (int sock, size_t size);

extern struct supp_info devhnd_devices[];

#endif /* __RTS_DEVHND__ */
