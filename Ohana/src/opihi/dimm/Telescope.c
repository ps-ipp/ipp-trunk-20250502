# include "dimm.h"

# define SER_TIMEOUT 10
# define SLEW_TIMEOUT 30
# define dCOS(A)   ((double) cos ((double)RAD_DEG*A))
# define dSIN(A)   ((double) sin ((double)RAD_DEG*A))

double distSky (double r1, double r2, double d1, double d2) {

  double x1, y1, z1;
  double x2, y2, z2;
  double cosT, dist;

  x1 = dCOS (r1) * dCOS (d1);
  y1 = dSIN (r1) * dCOS (d1);
  z1 = dSIN (d1);

  x2 = dCOS (r2) * dCOS (d2);
  y2 = dSIN (r2) * dCOS (d2);
  z2 = dSIN (d2);

  cosT = x1*x2 + y1*y2 + z1*z2;
  dist = DEG_RAD * acos (cosT);

  return (dist);
}

int getRD (double *r, double *d) { 

  int status;
  char *rastr, *decstr;

  status = SerialCommand (":GR#", &rastr, SER_TIMEOUT);
  if (!status) return (FALSE);

  status = SerialCommand (":GD#", &decstr, SER_TIMEOUT); 
  if (!status) return (FALSE);

  status = ohana_str_to_radec (r, d, rastr, decstr);
  if (!status) return (FALSE);

  free (rastr);
  free (decstr);

  return (TRUE);
}

int gotoRD (double r, double d) {

  double R, D, dist;
  int Ntry, status;
  char *str, *answer, cmd[64];

  /* error on ra, dec means coords out of range */

  /* set telescope coords, send */
  str = meade_ra_to_str (r);
  sprintf (cmd, ":Sr%s#", str);   free (str);
  status = SerialCommand (cmd, &answer, SER_TIMEOUT);
  if (!status) return (FALSE); 
  if (answer == (char *) NULL) return (FALSE); 
  if (strcmp (answer, "1")) return (FALSE); 
  free (answer);

  str = meade_dec_to_str (d);
  sprintf (cmd, ":Sd%s#", str);  free (str);
  status = SerialCommand (cmd, &answer, SER_TIMEOUT);   
  if (!status) return (FALSE); 
  if (answer == (char *) NULL) return (FALSE); 
  if (strcmp (answer, "1")) return (FALSE); 
  free (answer);

  Ntry = 0;
  status = SerialCommand (":MS#", &answer, SER_TIMEOUT);   
  if (!status) return (FALSE); 
  if (answer == (char *) NULL) return (FALSE); 
  if (strcmp (answer, "0")) {
    gprint (GP_ERR, "error: %s\n", answer);
    return (FALSE); 
  }
  free (answer);

  /* watch for response? */
  status = FALSE;
  while (!status) {
    getRD (&R, &D);
    dist = distSky (R, r, D, d);
    if (dist < 0.1) return (TRUE);
    usleep (100000);
    Ntry ++;
    if (Ntry > SLEW_TIMEOUT) return (FALSE);
  }
  return (status);
}

/* actual offsets are x,y, convert to arcmin */
int offset (char *direction, double distance) {

  /* Four rate choices: 
     slew   (RS) -  8 degree / sec : rate 1
     find   (RM) - 30 arcmin / sec : rate 2
     center (RC) -  4 arcmin / sec : rate 3
     guide  (RG) - 15 arcsec / sec : rate 4

     communication requires ~1.0 sec:
     offset should use rate which gives shortest time > 2.0 sec 
  */

# define NRATE 4
  static double delay[NRATE]  = {0.1, 0.1, 0.1, 0.1};
  static double rate[NRATE]   = {480.0, 30.0, 4.0, 0.25};
  static char rcmd[NRATE][16] = {"RS", "RM", "RC", "RG"};

  int i, status, rsel;
  char dir, cmd[32];
  double tsel, dt;

  dir = 0;
  if (!strcasecmp (direction, "y")) dir = (distance > 0) ? 'n' : 's';
  if (!strcasecmp (direction, "x")) dir = (distance > 0) ? 'w' : 'e';
  if (!dir) return (FALSE);

  /* distance is in arcmin */
  distance = fabs (distance);

  /* logic is bad -- does not catch too small distances */  
  rsel = -1;
  tsel = SLEW_TIMEOUT;
  for (i = 0; i < NRATE; i++) {
    dt = distance / rate[i] - delay[i];
    if ((dt > 0) && (dt < tsel)) {
      rsel = i;
      tsel = dt;
    }
  }
  if (tsel < 0) {
    gprint (GP_ERR, "offset %f arcmin below minimum\n", distance);
    return (FALSE);
  }
  if (tsel > SLEW_TIMEOUT) {
    gprint (GP_ERR, "offset %f arcmin above maximum\n", distance);
    return (FALSE);
  }
  gprint (GP_ERR, "offsetting %c for %f seconds\n", dir, tsel);
  
  sprintf (cmd, ":%s#", rcmd[rsel]);
  status = SerialCommand (cmd, (char **) NULL, SER_TIMEOUT);
  if (!status) return (FALSE);

  sprintf (cmd, ":M%c#", dir);
  status = SerialCommand (cmd, (char **) NULL, SER_TIMEOUT);
  if (!status) return (FALSE);

  usleep ((int)(tsel*1000000));

  sprintf (cmd, ":Q%c#", dir);
  status = SerialCommand (cmd, (char **) NULL, SER_TIMEOUT);
  if (!status) return (FALSE);

  return (TRUE);
}  

/* actual offsets are x,y, convert to arcmin */
int toffset (char *direction, char *rate, double duration) {

# define NRATE 6
  /* static char rcmd[NRATE][16] = {"RS", "RM", "RC", "RG"};*/
  static char rcmd[NRATE][64] = {"RS", "RM", "RC", "RG", "RA0.0085", "RE0.0085"};

  int i, status, rsel;
  char dir, cmd[32];
  double tsel, dt;

  dir = 0;
  if (!strcasecmp (direction, "x")) dir = (duration > 0) ? 'w' : 'e';
  if (!strcasecmp (direction, "y")) dir = (duration > 0) ? 'n' : 's';
  if (!dir) return (FALSE);
  duration = fabs (duration);
  
  status = FALSE;
  for (i = 0; i < NRATE; i++) if (!strcmp (rcmd[i], rate)) status = TRUE;
  if (!status) {
    gprint (GP_ERR, "bad rate: %s\n", rate);
    return (FALSE);
  }

  sprintf (cmd, ":%s#", rate);
  status = SerialCommand (cmd, (char **) NULL, SER_TIMEOUT);
  if (!status) return (FALSE);

  sprintf (cmd, ":M%c#", dir);
  status = SerialCommand (cmd, (char **) NULL, SER_TIMEOUT);
  if (!status) return (FALSE);

  usleep ((int)(duration*1000000));

  sprintf (cmd, ":Q%c#", dir);
  status = SerialCommand (cmd, (char **) NULL, SER_TIMEOUT);
  if (!status) return (FALSE);

  return (TRUE);
}  

int getXY (double *x, double *y) {

  char *answer;

  SerialCommand (":GA#", &answer, SER_TIMEOUT);
  ohana_dms_to_ddd (x, answer);
  free (answer);

  SerialCommand (":GZ#", &answer, SER_TIMEOUT);
  ohana_dms_to_ddd (y, answer);
  free (answer);

  return (TRUE);
}

/* need error checking on these */
int setRD (double r, double d) {

  char *str, *answer, cmd[64];

  /* set telescope coords, send */
  str = meade_ra_to_str (r);
  sprintf (cmd, ":Sr%s#", str);
  SerialCommand (cmd, (char **) NULL, SER_TIMEOUT);
  free (str);

  str = meade_dec_to_str (d);
  sprintf (cmd, ":Sd%s#", str);
  SerialCommand (cmd, (char **) NULL, SER_TIMEOUT);
  free (str);

  SerialCommand (":CM#", &answer, SER_TIMEOUT);
  gprint (GP_ERR, "result: %s\n", answer);
  free (answer);
  return (TRUE);
}

int setSite (char *sitename, double lon, double lat) {

  struct tm *gmt;
  struct timeval now;
  char *str, line[32];

  gprint (GP_ERR, "careful, this causes problems\n");
  return (FALSE);

  SerialCommand (":W1#", (char **) NULL, SER_TIMEOUT);

  /* Set site name 1 */
  sprintf (line, ":SM %s#", sitename); 
  SerialCommand (line, (char **) NULL, SER_TIMEOUT);

  /* Set site long */
  str = meade_deg_to_str (lon);
  str[6] = 0;
  sprintf (line, ":Sg%s#", str); 
  SerialCommand (line, (char **) NULL, SER_TIMEOUT);
  free (str);

  /* Set site lat */
  str = meade_dec_to_str (lat);
  sprintf (line, ":St%s#", str); 
  SerialCommand (line, (char **) NULL, SER_TIMEOUT);
  free (str);

  /* set UTC offset to 0.0: offset + local = gmt */
  sprintf (line, ":SG+00#");
  SerialCommand (line, (char **) NULL, SER_TIMEOUT);

  /* Set local */
  gettimeofday (&now, (struct timezone *) NULL);
  gmt = gmtime (&now.tv_sec);
  sprintf (line, ":SL%02d:%02d:%02d#", gmt[0].tm_hour, gmt[0].tm_min, gmt[0].tm_sec); 
  SerialCommand (line, (char **) NULL, SER_TIMEOUT);

  return (TRUE);
}

int setTime (char *lst) {

  struct tm *gmt;
  struct timeval now;
  char line[32], *answer;

  /* set UTC offset to 0.0: offset + local = gmt */
  sprintf (line, ":SG+10#");
  SerialCommand (line, (char **) NULL, SER_TIMEOUT);

  /* Set local */
  gettimeofday (&now, (struct timezone *) NULL);
  gmt = localtime (&now.tv_sec);
  sprintf (line, ":SL%02d:%02d:%02d#", gmt[0].tm_hour, gmt[0].tm_min, gmt[0].tm_sec); 
  SerialCommand (line, (char **) NULL, SER_TIMEOUT);

  /*
  sprintf (line, ":SS%s#", lst);
  SerialCommand (line, (char **) NULL, SER_TIMEOUT);
  */

  return (TRUE);
}

int getSite (double *lon, double *lat, double *lst) {

  struct tm *gmt;
  struct timeval now;
  char *str, *answer, line[32];

  /* : get latitude */
  SerialCommand (":Gt#", &answer, SER_TIMEOUT);
  ohana_dms_to_ddd (lat, answer);
  free (answer);

  /* : get longitude */
  SerialCommand (":Gg#", &answer, SER_TIMEOUT);
  ohana_dms_to_ddd (lon, answer);
  free (answer);

  /* : get LST */
  SerialCommand (":GS#", &answer, SER_TIMEOUT);
  ohana_dms_to_ddd (lst, answer);
  free (answer);

  return (TRUE);
}

int ParkScope() {

  char *str, *answer, line[32];

  SerialCommand (":hP#", &answer, SER_TIMEOUT);
  free (answer);

  return (TRUE);

}

int SleepScope() {

  char *str, *answer, line[32];
  
  SerialCommand (":hN#", &answer, SER_TIMEOUT);
  free (answer);
  
  return (TRUE);

}

int WakeScope() {

  char *str, *answer, line[32];

  SerialCommand (":hW#", &answer, SER_TIMEOUT);
  free (answer);

  return (TRUE);

}
