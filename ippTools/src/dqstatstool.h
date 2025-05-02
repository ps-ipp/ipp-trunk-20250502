#ifndef DQSTATS_H
#define DQSTATS_H 1

#include "pxtools.h"

typedef enum {
  DQSTATS_MODE_NONE            = 0x0,
  DQSTATS_MODE_DEFINEBYQUERY,
  DQSTATS_MODE_PENDINGBUNDLE,  
  DQSTATS_MODE_CREATEBUNDLE,
  DQSTATS_MODE_CLEANBUNDLE,
  DQSTATS_MODE_UPDATERUN,
  DQSTATS_MODE_REVERTRUN,
} dqstatstoolMode;

pxConfig *dqstatstoolConfig(pxConfig *config, int argc, char **argv);

#endif
