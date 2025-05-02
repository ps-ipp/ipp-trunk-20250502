#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "sbig.h"
#include "fh/fh.h" /* CFHT FITS Handling library */

// XXX RHL claims this is not needed
// #include <values.h>

#define N_FITS_ENTRIES 33

/*
 * The following hack is needed if you want to use sbig.a w/ libc5
 */
#ifndef __bzero
void __bzero(void* s, int n)
{
  memset(s, 0, n);
}
#endif

static struct sbig_init info;

static int write_fits(int width, int height, double etime, int binmode, unsigned short *data);

static struct sbig_readout readout;

/*
 * This is the 'progress' callback
 * passed to sbig_readout()
 */
static int readout_callback(float percent)
{
  gprint (GP_ERR, "progress: Reading %d x %d pixels (%d%%)\r",
	  readout.width, readout.height, (int)percent);
  /* return 1 to continue, 0 to abort */
  return 1;
}

/*
 *  Start an exposure, wait for it, then download from the camera
 *  in bands.  This is somewhat complex, since it handles any size
 *  subimage of the CCD, and bands too.
 */
static void take_picture(double etime, int binmode, int shuttermode, int x, int y, int w, int h)
{
  int  ret, i, needed;
  static unsigned short  *buffer;
  static int  buffer_size;
  struct sbig_expose  expose;
  struct sbig_status  status;
  double  f;
  
  expose.ccd = 0; /* Always in imaging mode, not tracking 
		    since CFHT PF does the tracking*/

  if(binmode<0 || binmode>3){
    gprint (GP_ERR, "Imaging binmode options: [0=1x1, *1=2x2, 2=3x3]\n");
    exit(EXIT_FAILURE);
  }
  readout.binning=binmode;
  
  readout.x=x;
  readout.y=y;
  readout.width=w;
  readout.height=h;
  if (readout.width == 0) {
    readout.width =
      info.camera_info[expose.ccd].readout_mode[readout.binning].width - x;
  } else
    if (readout.width + x >
	info.camera_info[expose.ccd].readout_mode[readout.binning].width) {
    gprint (GP_ERR, "Raster X or Width parameter out of range.\n");
    exit(EXIT_FAILURE);
  }

  if (readout.height == 0) {
    readout.height =
      info.camera_info[expose.ccd].readout_mode[readout.binning].height - y;
  } else
    if (readout.height + y >
	info.camera_info[expose.ccd].readout_mode[readout.binning].height) {
    gprint (GP_ERR, "Raster Y or Height parameter out of range.\n");
    exit(EXIT_FAILURE);
  }

  f = etime;
  expose.exposure_time = (int)(100.0*f);
  expose.abg_state = SBIG_ABG_OFF; /* SBIG_ABG_MEDIUM */

  expose.shutter = shuttermode;  /* No shutter present */

  sync();
  ret = sbig_expose(&expose);
  if (ret < 0)
    gprint (GP_ERR, "sbig error: %s\n", sbig_show_error(ret));
  
  gprint (GP_ERR, "progress: Sleeping for %g seconds\r", 0.01*expose.exposure_time);

  if (expose.exposure_time > 100) {
    for (i = 0; i < expose.exposure_time; i += 100) {
      sbig_get_status(&status);
      gprint (GP_ERR, "progress: Exposure in progress; ccd_status=%d, shutter=%d (%d%%)\r",
	      status.imaging_ccd_status, status.shutter_state,
	      i * 100 / expose.exposure_time);
      sleep(1);
    }
  } else
    usleep(expose.exposure_time*10000);
  
  /* wait for exposure to complete */
  gprint (GP_ERR, "progress: Waiting for controller                             (100%%)\r");

  for (i = 0; i < 1000; ++i) {
    ret = sbig_get_status(&status);
    if (ret < 0) {
      gprint (GP_ERR, "sbig error: %s\n", sbig_show_error(ret));
      break;
    }
    if (status.imaging_ccd_status != 3)
      gprint (GP_ERR, "logonly: ccd_status=%d\n", status.imaging_ccd_status);

    if (readout.ccd == 0) {
      if (status.imaging_ccd_status != 2)
	break;
    } else {
      if (status.tracking_ccd_status != 2)
	break;
    }
    usleep(50000);
  }
  if (i)
    gprint (GP_ERR, "warning: Exposure took an extra %g seconds\n", 0.050*i);

  if (i == 1000)
    gprint (GP_ERR, "error: EXPOSURE DIDN'T FINISH!\n");
  else
    gprint (GP_ERR, "progress: Exposure complete                                  (100%%)\n");

  readout.ccd = expose.ccd;
  readout.binning = readout.binning;
  needed = readout.height*readout.width*sizeof(short);
  if (buffer_size < needed) {
    if (buffer != NULL)
      free(buffer);
    buffer = malloc(buffer_size = needed);
  }
  readout.data = buffer;
  readout.data_size_in_bytes = buffer_size;
  readout.callback = readout_callback;
    
  sync();
  if ((ret = sbig_readout(&readout)) < 0) {
    gprint (GP_ERR, "sbig error: %s\n", sbig_show_error(ret));
    return;
  }

  gprint (GP_ERR, "progress: Writing FITS data to stdout\n");

  if(write_fits(readout.width, readout.height, etime, binmode, buffer)==-1)
    gprint (GP_ERR, "error: Could not write FITS file to stdout.\n");
  gprint (GP_ERR, "progress: sp_ccdcontrol done.\n");
}

static int write_fits(int w, int h, double etime, int binmode, unsigned short *data)
{
  HeaderUnit hu;
  register int i;
  double datamin = DBL_MAX, datamax = DBL_MIN;
  unsigned short u;
  char str[80]; /* For building string FITS headers */
  time_t date;

  int ret; /* Return value for sbig_status */
  struct sbig_status sbstat; /* Status structure */
  
  /* Get status to add to FITS header */
  sync();
  ret = sbig_get_status(&sbstat);

  if (ret < 0) {
    gprint (GP_ERR, "error: sbig_get_status failed: %s\n", sbig_show_error(ret));
    exit(EXIT_FAILURE);
  }

  /* shouldn't we scale this to fit, first? */
  for (i = w*h; --i >= 0; )
    {
      u = data[i];
      if (u > datamax) datamax = u;
      if (u < datamin) datamin = u;
      data[i] = ((u<<8)|(u>>8)) ^ 0x0080; /* -32768 */
    }

  time(&date);

  hu = fh_create();
  fh_reserve(hu, 50); /* Reserve space for 50 cards (TCS, Elixir?, etc.) */
  fh_set_bool(hu, 0., "SIMPLE", 1, "Standard FITS");
  fh_set_int(hu, 1.0, "BITPIX", 16,"16-bit data");
  fh_set_int(hu, 2.0, "NAXIS", 2,  "Number of axes");
  fh_set_int(hu, 2.1, "NAXIS1", w, "Number of pixel columns");
  fh_set_int(hu, 2.2, "NAXIS2", h, "Number of pixel rows");
  fh_set_int(hu, 5.0, "PCOUNT",	0, "No 'random' parameters");
  fh_set_int(hu, 6.0, "GCOUNT",	1, "Only one group");

  strftime(str, sizeof(str)-1, "%Y-%m-%dT%T", gmtime(&date));
  fh_set_str(hu, 104, "DATE", str, "UTC Date of file creation");
  strftime(str, sizeof(str)-1, "%a %b %d %H:%M:%S %Z %Y", localtime(&date));
  fh_set_str(hu, 104.1, "HSTTIME", str, "Local time in Hawaii");

  fh_set_str(hu, 105, "ORIGIN", "CFHT", "Canada-France-Hawaii Telescope");

  fh_set_flt(hu, 141.,"BZERO",	32768.0, 6,	"Zero factor");
  fh_set_flt(hu, 142.,"BSCALE",	1.0, 2, "Scale factor");
  fh_set_flt(hu, 150, "DATAMIN", datamin, 6, "Minimum value of the data");
  fh_set_flt(hu, 151, "DATAMAX", datamax, 6, "Maximum value of the data");
  fh_set_flt(hu, 160, "SATURATE", 4016.0, 6, "Saturation value");

  fh_set_flt(hu, 220, "EXPTIME", etime, 5, "Integration time (seconds)");
  sprintf(str, "%d %d", binmode + 1, binmode + 1);
  fh_set_str(hu, 230, "CCDSUM", str, "Binning factors");
  fh_set_int(hu, 231, "CCDBIN1", binmode + 1, "Binning factor along first axis");
  fh_set_int(hu, 232, "CCDBIN2", binmode + 1, "Binning factor along second axis");

  fh_set_com(hu, 1400.0, "COMMENT", "");
  fh_set_com(hu, 1400.1, "COMMENT", " SBIG status record:");
  fh_set_com(hu, 1400.2, "COMMENT", "");
  if (sbstat.imaging_ccd_status==0)
    fh_set_str(hu, 1500, "DETSTAT", "ok", "(SBIG imaging_ccd_status is 0)");
  else
    fh_set_int(hu, 1500, "DETSTAT", sbstat.imaging_ccd_status, "SBIG error!");
  fh_set_flt(hu, 1500.1, "DETTEM", sbstat.ccd_temperature/10., 3, "Detector temperature");
  fh_set_int(hu, 1500.2, "DETTEMRG", sbstat.temperature_regulation, "1=set temp 0=set power");
  fh_set_int(hu, 1500.3, "DETTEMSP", sbstat.temperature_setpoint, "Temperature setpoint");
  fh_set_int(hu, 1500.4, "DETTEMCP", sbstat.cooling_power, "Cooling power");
  fh_set_flt(hu, 1510,  "CAMTEM", sbstat.air_temperature/10., 3, "Outside air temperature");
  fh_set_com(hu, 1600.0, "COMMENT", "");
  fh_set_com(hu, 1600.1, "COMMENT", " SBIG info record:");
  fh_set_com(hu, 1600.2, "COMMENT", "");
  sprintf(str, "%u", info.firmware_version);
  fh_set_str(hu, 1600.9, "CONSWV", str, "SBIG firmware version");
  fh_set_str(hu, 1601,"DETECTOR", info.camera_name, info.serial_number);
  fh_set_bool(hu, 1700, "ABGON", info.imaging_abg_type, "Antiblooming gate present");
  fh_set_int(hu, 1701, "ABGMODE", info.imaging_abg_type, "Antiblooming gate control value");
  fh_set_int(hu, 1800, "BADCOL", info.nmbr_bad_columns, "Number of bad columns");
  for (i = 0; i < info.nmbr_bad_columns; ++i){
    sprintf(str, "BADCOL%d", i);
    fh_set_int(hu, 1800 + 0.1 + 0.1 * i, str, info.bad_columns[i], "Bad column");
  }
  fh_set_int(hu, 1901, "ST5ADSIZ", info.ST5_AD_size, "");
  fh_set_int(hu, 1902, "ST5FLTYP", info.ST5_filter_type, "");

  if (fh_write(hu, STDOUT_FILENO) != FH_SUCCESS)
    exit(EXIT_FAILURE);

  if (fh_write_padded_image(hu, STDOUT_FILENO, data, w*h*sizeof(unsigned short), FH_TYPESIZE_RAW) != FH_SUCCESS)
    exit(EXIT_FAILURE);

  gprint (GP_ERR, "logonly: Wrote %d pixels, min=%g, max=%g\n", w*h, datamin, datamax);
  sync();
  return 0;
}

static void show_format(void){
  gprint (GP_ERR, "Format for sp_ccdcontrol, SkyProbe CCD controller:\n"
	  "Writes FITS file to stdout, status messages to stderr.\n"
	  "NOTE: -port {hex} is an option for all forms of sp_ccdcontrol,\n"
	  "  to specify a hex adress for the parallel port.\n"
	  "sp_ccdcontrol expose -etime {Exp time}\n"
          "   [-binmode {Binning mode}]\n"
          "   [-shuttermode {Shutter mode}]\n"
          "   [-raster X Y W H]\n"
	  "Exp time: Exposure time given in seconds.\n"
	  "Binning mode: 0=1x1, 1=2x2, 2=3x3.  Note, 3x3 not allowed in -track mode.\n"
	  "   OR \n"
	  "sp_ccdcontrol cool {coolmode} [temp|power]\n"
	  "coolmode: 0=OFF, 1=ON, 2=Direct Drive\n"
	  "For coolmode=0: Do not specify temp or power\n"
	  "For coolmode=1: Specify temp in deg C\n"
	  "For coolmode=2: Specify power (0-255)\n");
  exit(EXIT_FAILURE);
}


int main(int argc, char *argv[])
{
  /*    char buffer[80], last_command[80]; */
  int  ret = -1, i, port;
  int  is_small_camera;
    
  /* Simon Kras--command line variables */
  char cmd[80];
  double etime;
  int binmode;
  int shuttermode;
  int coolmode;
  int raster_x, raster_y, raster_h, raster_w;
  double cooltemp, coolpwr;

  if(argc>1) strcpy(cmd, argv[1]);
  else show_format();

  port = 1; /* 0x378 */
  for (i = 1; i < argc; ++i) {
    if (!strcmp(argv[i], "-port")) {
      sscanf(argv[++i], "%x", &port);
      gprint (GP_ERR, "port override to 0x%x\n", port);
    }
  }

  for (i = 0; i < 4; ++i) {
    sync();
    /* set our verbosity to 0 (range is 0 to 5) */
    ret = sbig_init(port, 0, &info);
    if (ret == -6) ret = 0; /* %%% It always returns this error, but actually it worked.
			     * Since the library is binary only, we can't fix it.
			     */
    if (ret == 0) break;
    gprint (GP_ERR,"logonly: sbig_init() failed: %s\n", sbig_show_error(ret));
    usleep(100000);
  }
  if (ret < 0) {
    gprint (GP_ERR, "error: cannot open camera, giving up\n");
    exit(EXIT_FAILURE);
  }

  is_small_camera = (strstr(info.camera_name, "237") != NULL ||
		     index(info.camera_name, '5') != NULL);
  for (i = 0; i < info.camera_info[0].nmbr_readout_modes; ++i) {
    struct readout_mode  *rmp;
    rmp = &info.camera_info[0].readout_mode[i];
#ifdef DEBUG
    gprint (GP_ERR, "debug: ImagingCCD mode %d: w=%d h=%d "
	    "gain=%f "
	    "pw=%g ph=%g\n", rmp->mode, rmp->width,
	    rmp->height,  0.01*rmp->gain, 0.001*rmp->pixel_width, 
	    0.001*rmp->pixel_height);
#endif
  }

  for (i = 0; i < info.camera_info[1].nmbr_readout_modes; ++i) {
    struct readout_mode  *rmp;

    rmp = &info.camera_info[1].readout_mode[i];
#ifdef DEBUG
    gprint (GP_ERR, "debug: TrackingCCD mode %d: w=%d h=%d "
	    "gain=%f "
	    "pw=%g ph=%g\n"
	    , rmp->mode, rmp->width, rmp->height,
	    0.01*rmp->gain,
	    0.001*rmp->pixel_width, 0.001*rmp->pixel_height);
#endif
  }
  if(!strcmp(cmd, "expose")){
    etime=-999.;
    for(i=1;i<argc;++i){
      if(!strcmp(argv[i], "-etime")){
	if(argc>(i+1)) etime = atof(argv[++i]);
	else
	  gprint (GP_ERR, "No argument given to -etime\n");
      }
    }
    if(etime==-999.){
	gprint (GP_ERR, "Must specify exposure time.\n");
	exit(EXIT_FAILURE);
      }
      
      binmode=-999;
      for(i=1;i<argc;++i){
	if(!strcmp(argv[i], "-binmode")){
	  if(argc>(i+1)) binmode = atoi(argv[++i]);
	  else{
	    gprint (GP_ERR, "No argument given for -binmode\n");
	    exit(EXIT_FAILURE);
	  }
	}
      }
      if(binmode==-999) binmode=0;

      shuttermode=0;
      for (i=1; i<argc;++i){
	if (!strcmp(argv[i], "-shuttermode")){
	  if (argc>(i+1)) shuttermode = atoi(argv[++i]);
	  else{
	    gprint (GP_ERR, "No argument given for -shuttermode\n");
	    exit(EXIT_FAILURE);
	  }
	}
      }

      raster_x=0;
      raster_y=0;
      raster_w=0;
      raster_h=0;
      for (i=1; i<argc;++i){
	if (!strcmp(argv[i], "-raster")){
	  if (argc>(i+4)) {
	    raster_x = atoi(argv[++i]);
	    raster_y = atoi(argv[++i]);
	    raster_w = atoi(argv[++i]);
	    raster_h = atoi(argv[++i]);
	  }
	  else{
	    gprint (GP_ERR, "-raster requires 4 arguments: x y w h\n");
	    exit(EXIT_FAILURE);
	  }
	}
      }
      
      take_picture(etime, binmode, shuttermode, raster_x, raster_y, raster_w, raster_h);

  } else if(!strcmp(cmd, "cool")){
    double  f;
    struct sbig_cool  cool;
    
    if(argc<3){
      gprint (GP_ERR, "Too few arguments for cool mode\n");
      exit(EXIT_FAILURE);
    } 
    coolmode = atoi(argv[2]);
    
    if(coolmode<0 || coolmode>2){
      gprint (GP_ERR, "Cooling mode values allowed: [0 Off, 1 *On, 2 DirectDrive]\n");
      exit(EXIT_FAILURE);
    }
    cool.regulation = coolmode;
    cool.direct_drive = 0;
    if (cool.regulation == 2){
      if(argc<4){
	gprint (GP_ERR, "Must specify cooling power (0-255)\n");
	exit(EXIT_FAILURE);
      }
      coolpwr=atof(argv[3]);
      if(coolpwr<0 || coolpwr>255){
	gprint (GP_ERR, "Cooling power must be 0-255.\n");
	exit(EXIT_FAILURE);
      }
      cool.direct_drive = coolpwr;
    }
    else if (cool.regulation == 1) {
      if(argc<4){
	gprint (GP_ERR, "Must specify temperature in deg C\n");
	exit(EXIT_FAILURE);
      }
      cooltemp=atof(argv[3]);
      if(cooltemp<-50 || cooltemp>20){
	gprint (GP_ERR, "Cooling temperature must be -50 to 20 deg C.\n");
	exit(EXIT_FAILURE);
      }
      f = cooltemp;
      i = 10.0*f + 0.5;
      cool.temperature = i;
    }
    sync();
    ret = sbig_set_cooling(&cool);
    if (ret < 0){
      gprint (GP_ERR, "sbig error: %s\n", sbig_show_error(ret));
    }
  }else{
    gprint (GP_ERR, "Illegal command: '%s'\n", cmd);
    show_format();
  }
  exit(EXIT_SUCCESS);
}
 
