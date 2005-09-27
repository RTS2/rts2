#ifndef __RTS_DEVHND__
#define __RTS_DEVHND__

#include "image_info.h"
#include "param.h"
#include "status.h"
#include "serverd_info.h"

struct supp_info
{
  devcli_handle_response_t command_handler;	//! handler to handle responses
  devcli_handle_response_t message_handler;	//! handler to asynchoronous messages
};

extern struct supp_info devhnd_devices[];

#endif /* __RTS_DEVHND__ */
