
# include "psastroInternal.h"

# define ESCAPE { \
    psError(PS_ERR_UNKNOWN, false, "I/O failure in pmMaskStats");	\
    psFree (view);							\
    return false;							\
  }



