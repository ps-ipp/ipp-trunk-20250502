# include "dimm.h"

static struct sbig_init info;

int InitCamera (int port) {
  
  int i, state;

  for (i = 0; i < 10; i++) {
    state = sbig_init (port, SBIG_IMAGING_CCD, &info);
    if (state == -6) state = 0;
    if (state ==  0) {
      DumpCameraInfo ();
      return (TRUE);
    }
    gprint (GP_ERR, "retry...\n");
  }
  
  gprint (GP_ERR, "failed to init sbig camera on %d\n", port);
  gprint (GP_ERR, "%s\n", sbig_show_error (state));
  return (FALSE);
}

void DumpCameraInfo () {

      gprint (GP_ERR, "opened sbig camera:\n");
      gprint (GP_ERR, "linux_version: %f\n",      info.linux_version);
      gprint (GP_ERR, "nmbr_bad_columns: %d\n",   info.nmbr_bad_columns);
      gprint (GP_ERR, "imaging_abg_type: %d\n",   info.imaging_abg_type);
      gprint (GP_ERR, "serial_number: %s\n",      info.serial_number);
      gprint (GP_ERR, "firmware_version: %d\n",   info.firmware_version);
      gprint (GP_ERR, "camera_name: %s\n",        info.camera_name);
      gprint (GP_ERR, "nmbr_readout_modes: %d\n", info.camera_info[0].nmbr_readout_modes);
      gprint (GP_ERR, "mode: %d\n",               info.camera_info[0].readout_mode[0].mode);
      gprint (GP_ERR, "width: %d\n",              info.camera_info[0].readout_mode[0].width);
      gprint (GP_ERR, "height: %d\n",             info.camera_info[0].readout_mode[0].height);
      gprint (GP_ERR, "gain: %d\n",               info.camera_info[0].readout_mode[0].gain);
      gprint (GP_ERR, "pixel_width: %d\n",        info.camera_info[0].readout_mode[0].pixel_width);
      gprint (GP_ERR, "pixel_height: %d\n",       info.camera_info[0].readout_mode[0].pixel_height);

      gprint (GP_ERR, "ST5_AD_size: %d\n", info.ST5_AD_size);
      gprint (GP_ERR, "ST5_filter_type: %d\n", info.ST5_filter_type);
}      

void CameraFullSize (int *x, int *y) {
  *x = info.camera_info[0].readout_mode[0].width;
  *y = info.camera_info[0].readout_mode[0].height;
}

int SetTemperature (double temp) {

  int state;
  struct sbig_cool cool;

  if (temp < -50) return (FALSE);
  if (temp > +20) return (FALSE);

  cool.regulation = SBIG_TEMP_REGULATION_ON;
  cool.temperature = (int) (10.0*temp + 0.5);
  cool.direct_drive = 0;

  state = sbig_set_cooling (&cool);
  if (state < 0) {
    gprint (GP_ERR, "sbig error: %s\n", sbig_show_error (state));
    return (FALSE);
  }
  return (TRUE);
}

double GetTemperature () {

  int state;
  double temp;
  struct sbig_status status;

  state = sbig_get_status (&status);
  if (state < 0) {
    gprint (GP_ERR, "sbig error: %s\n", sbig_show_error (state));
    return (-200.0);
  }
  temp = (status.ccd_temperature - 0.5) / 10.0;
  return (temp);
}

int DumpCameraStatus () {

  int state;
  double temp;
  struct sbig_status status;

  state = sbig_get_status (&status);
  if (state < 0) {
    gprint (GP_ERR, "sbig error: %s\n", sbig_show_error (state));
    return (FALSE);
  }

  gprint (GP_ERR, "imaging_ccd_status: %d\n", status.imaging_ccd_status);
  gprint (GP_ERR, "tracking_ccd_status: %d\n", status.tracking_ccd_status);
  gprint (GP_ERR, "fan_on: %d\n", status.fan_on);
  gprint (GP_ERR, "shutter_state: %d\n", status.shutter_state);
  gprint (GP_ERR, "led_state: %d\n", status.led_state);
  gprint (GP_ERR, "shutter_edge: %d\n", status.shutter_edge);
  gprint (GP_ERR, "plus_x_relay: %d\n", status.plus_x_relay);
  gprint (GP_ERR, "minus_x_relay: %d\n", status.minus_x_relay);
  gprint (GP_ERR, "plus_y_relay: %d\n", status.plus_y_relay);
  gprint (GP_ERR, "minus_y_relay: %d\n", status.minus_y_relay);
  gprint (GP_ERR, "pulse_active: %d\n", status.pulse_active);
  gprint (GP_ERR, "temperature_regulation: %d\n", status.temperature_regulation);
  gprint (GP_ERR, "temperature_setpoint: %d\n", status.temperature_setpoint);
  gprint (GP_ERR, "cooling_power: %d\n", status.cooling_power);
  gprint (GP_ERR, "air_temperature: %d\n", status.air_temperature);
  gprint (GP_ERR, "ccd_temperature: %d\n", status.ccd_temperature);

  return (TRUE);
}

/* block until exposure is complete */
int Exposure (double exptime) {

  int i, state;
  struct sbig_expose expose;
  struct sbig_status status;

  expose.ccd = SBIG_IMAGING_CCD;
  expose.exposure_time = (int)(100.0*exptime);
  expose.abg_state = SBIG_ABG_OFF;
  expose.shutter = SBIG_EXPOSE_SHUTTER_NORMAL;  /* shuttermode = ? */

  /* drop this ? */
  /* usleep ((int)(exptime*1000000)); */
  state = sbig_expose (&expose);
  if (state < 0) {
    gprint (GP_ERR, "exposure error\n");
    gprint (GP_ERR, "%s\n", sbig_show_error (state));
    return (FALSE);
  }
  
  for (i = 0; i < expose.exposure_time + 10; ++i) {
    state = sbig_get_status (&status);
    /* gprint (GP_ERR, "%d\n", state); */
    /* gprint (GP_ERR, "%d  %d\n", status.imaging_ccd_status, status.shutter_state); */
    /*    if (state == 0) return (TRUE); */
    if (status.imaging_ccd_status == -SBIG_NO_EXPOSURE_IN_PROGRESS) return (TRUE);
    if (status.imaging_ccd_status == -SBIG_EXPOSURE_IN_PROGRESS) {
      usleep (10000);
      continue;
    }
    gprint (GP_ERR, "exposure error\n");
    gprint (GP_ERR, "%s\n", sbig_show_error (state));
    return (FALSE);
  }
  gprint (GP_ERR, "exposure timeout\n");
  return (FALSE);
}

int   readout_abort;
float readout_percent;
static int readout_callback (float percent) {
  /* return 1 to continue, 0 to abort */
  if (((int)(percent) % 10) == 0) { gprint (GP_ERR, "."); }
  readout_percent = percent;
  if (readout_abort) return 0;
  return 1;
}

int ReadOut (int x, int y, int dx, int dy, int binning, unsigned short *buffer) {

  int state, Nbytes;
  static struct sbig_readout readout;

  readout.x = x;
  readout.y = y;
  readout.width  = dx;
  readout.height = dy;

  Nbytes = readout.width*readout.height*sizeof(short);

  /* for bin 2x2 or 3x3, need to adjust dx, dy above */
  readout.ccd = SBIG_IMAGING_CCD;
  readout.binning = SBIG_BIN_1X1;
  readout.data = buffer;
  readout.data_size_in_bytes = Nbytes;
  readout.callback = readout_callback;
    
  gprint (GP_ERR, "%d, %d : %d x %d\n", readout.x, readout.y, readout.width, readout.height);
  sync (); 
  readout_abort = FALSE;
  state = sbig_readout (&readout);
  gprint (GP_ERR, "\n");
  if (state < 0) {
    gprint (GP_ERR, "sbig error: %s\n", sbig_show_error (state));
    return (FALSE);
  }
  return (TRUE);
}

int OpenShutter () {

  int state;
  struct sbig_control control;

  control.shutter = SBIG_OPEN_SHUTTER;
  state = sbig_control (&control);
  if (state < 0) {
    gprint (GP_ERR, "sbig error: %s\n", sbig_show_error (state));
    return (FALSE);
  }
  return (TRUE);
}

int CloseShutter () {

  int state;
  struct sbig_control control;

  control.shutter = SBIG_CLOSE_SHUTTER;
  state = sbig_control (&control);
  if (state < 0) {
    gprint (GP_ERR, "sbig error: %s\n", sbig_show_error (state));
    return (FALSE);
  }
  return (TRUE);
}
