# include "Ximage.h"

void hh_hms (char *line, double ra, double dec, char sep, int Nchar) {

  int h, m, flag;
  double s;
  
  ra /= 15.0;  /* convert from degrees to hours */
  flag = SIGN(ra);
  ra *= flag;
  h = ra;
  m = 60.000001*(ra - h);
  s = 3600*(ra - h - m / 60.0);
  if (flag > 0)
    snprintf (line, Nchar, " %02d%c%02d%c%04.1f  ", h, sep, m, sep, s);
  else
    snprintf (line, Nchar, "-%02d%c%02d%c%04.1f  ", h, sep, m, sep, s);
  
  flag = SIGN(dec);
  dec *= flag;
  h = dec;
  m = 60.000001*(dec - h);
  s = 3600*(dec - h - m / 60.0);
  if (flag > 0)
    snprintf (&line[13], Nchar, " %02d%c%02d%c%04.1f", h, sep, m, sep, s);
  else
    snprintf (&line[13], Nchar, "-%02d%c%02d%c%04.1f", h, sep, m, sep, s);
}
