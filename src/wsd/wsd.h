#ifndef __RTS2_WSD__
#define __RTS2_WSD__

#include <libwebsockets.h>

#define resource_path "/usr/share/rts2"

#ifdef __cplusplus
extern "C" {
#endif

extern int max_poll_elements;

extern struct lws_pollfd *pollfds;
extern int *fd_lookup;
extern int count_pollfds;

struct per_session_data__http {
	int fd;
	unsigned int client_finished:1;
};

struct per_session_data__dumb_increment
{
	int number;
};

int callback_http (struct lws *wsi, enum lws_callback_reasons reason, void *user, void *in, size_t len);

#ifdef __cplusplus
};
#endif

#endif //! __RTS2_WSD__
