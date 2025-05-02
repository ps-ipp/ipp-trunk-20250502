# include "pantasks.h"

# define MAX_N_JOBS 0x80000000

static IDtype JobIDMax = 0;

void InitJobIDs () {
  JobIDMax = 0;
}  

void FreeJobIDs () { }

/* return next unique ID, recycle every MAX_N_JOBS */
IDtype NextJobID () {

  JobIDMax ++;
  if (JobIDMax >= MAX_N_JOBS) {
    gprint (GP_ERR, "ERROR: too many jobs spawned: %d, aborting\n", JobIDMax);
    abort();
  }

  return (JobIDMax);
}

/* I am going to rework the jobID so that it just grows.  no need to ever free them
   as an int, that allows 2^31 jobs (2e9 jobs) */
