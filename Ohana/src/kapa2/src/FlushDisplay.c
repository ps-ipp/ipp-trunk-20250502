# include "Ximage.h"

static struct timeval reftime; 
static char reftimeset = FALSE;
# define DTIME(A,B) ((A.tv_sec - B.tv_sec) + 1e-6*(A.tv_usec - B.tv_usec))

void FlushDisplay (void) {

  struct timeval now;
  int flush;
  double dtime;
  Graphic *graphic;

  graphic = GetGraphic();

  if (!USE_XWINDOW) return;

  flush = FALSE;
  if (!reftimeset) {
    flush = TRUE;
    gettimeofday (&reftime, NULL);
  } 

  gettimeofday (&now, NULL);
  dtime = DTIME (now, reftime);

  if (dtime > 0.1) {
    flush = TRUE;
  }

  if (flush) {
    // I changed XFlush to XSync to avoid lag
    // of the display (build up of config events)
    XSync (graphic->display, FALSE);
    reftime = now;
  }

}

