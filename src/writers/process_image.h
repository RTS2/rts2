#ifndef __RTS2_PROCESS_IMAGE__
#define __RTS2_PROCESS_IMAGE__

#include "image_info.h"


void *process_images (void *);
int data_handler (int sock, size_t size, struct image_info *image);

#endif /* __RTS2_PROCESS_IMAGE__ */
