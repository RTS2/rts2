#ifndef __RTS2_CENTERING__
#define __RTS2_CENTERING__

#include "../utils/devcli.h"
#include "../utils/devconn.h"

int rts2_centering (struct device *camera, struct device *telescope,
		    char obs_type);

#endif /* __RTS2_CENTERING__ */
