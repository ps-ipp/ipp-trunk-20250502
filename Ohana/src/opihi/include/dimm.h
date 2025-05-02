# include "external.h"
# include "shell.h"
# include "dvomath.h"
# include "convert.h"
# include "display.h"
# include "sbig.h"

void InitDIMM (void);

/* telescope.c */
double distSky (double r1, double r2, double d1, double d2);
int getRD (double *r, double *d) ;
int gotoRD (double r, double d);
int offset (char *direction, double distance);
int toffset (char *direction, char *rate, double duration);
int getXY (double *x, double *y);
int setRD (double r, double d);
int setSite (char *sitename, double lon, double lat);
int setTime (char *lst);
int getSite (double *lon, double *lat, double *lst);
int ParkScope(void);
int SleepScope(void);
int WakeScope(void);

/* Serial.c */
int SerialVerbose (int mode);
int SerialInit (char *port);
int SerialOpen (char *port);
int SerialBaudRate (int fdesc, int rate);
int SerialParity (int fdesc, int parity); 
int SerialDataBits(int fdesc, int bits); 
int SerialStopBit(int fdesc, int stpbit); 
void SerialStop (int fdesc);
int SerialCommand (char *in, char **out, int wait);

/* camera.c */
int InitCamera (int port);
void DumpCameraInfo (void);
void CameraFullSize (int *x, int *y);
int SetTemperature (double temp);
double GetTemperature (void);
int DumpCameraStatus (void);
int Exposure (double exptime);
static int readout_callback (float percent);
int ReadOut (int x, int y, int dx, int dy, int binning, unsigned short *buffer);
int OpenShutter (void);
int CloseShutter (void);

  /* this image structure stuff is considered for further development */ 
# if (0)
/** this Image is incompatible with the DVO Image struct */
typedef struct {

  /* image data area */
  int Nx, Ny;
  char *buffer;
  int Nbytes;

  /* image metadata */
  double ccdtemp;
  double airtemp;
  double ra, dec, airmass;
  double exptime;
  int binning;
} Image;

/* analysis.c */
int subtractImage (Image *a, Image *b);
void statsImage (Image *image, Stats *stats);
void findStars (Image *image, Stars **stars, int *Nstars, double threshold);
# endif
