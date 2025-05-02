/* dummy version of psfstats.c - disable use of the psf */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "burntool.h"
#include "math.h"

/****************************************************************/
/* psf_stats(): Tell us about this PSF star */
STATIC int psf_stats(int nx, int ny, IMTYPE *data, int bias,
                     double *fwhm, double *q)
{
   return(1);
}
