# include "data.h"
    
double get_epoch (char *in_epoch, char mode) {

  int done;
  double epoch;

  epoch = 2000.0;
  done = FALSE;
  if (in_epoch[0] == 'B') {
    epoch = BtoJ(atof(&in_epoch[1]));
    done = TRUE;
  }

  if (in_epoch[0] == 'J') {
    epoch = atof(&in_epoch[1]);
    done = TRUE;
  }

  if (!done && (mode == 'B')) {
    epoch = BtoJ(atof(in_epoch));
    done = TRUE;
  }
    
  if (!done && (mode == 'J')) {
    epoch = atof(in_epoch);
    done = TRUE;
  }

  if (!done) {
    gprint (GP_ERR, "error finding epoch %s\n", in_epoch);
    return FALSE;
  }
  
  return (epoch);

}

  
double BtoJ (double in_epoch) {

  double JD, out_epoch;

  JD = (in_epoch - 1900.0)*365.242198781 + 2415020.31352;
  out_epoch = 2000.0 + (JD - 2451545.0)/365.25;

  return (out_epoch);
}

