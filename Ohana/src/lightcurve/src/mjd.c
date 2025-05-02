# include "lightcurve.h"
# define GREG (15+31*(10+12*1582))

double mjd (year, hour, second)
double year, hour, second;
{

  int iyear;
  int Months[] = {0,31,59,90,120,151,181,212,243,273,304,334};

  /* convert year, which contains year + day, into year, month, day */
  iyear = year;
  if ((!(iyear % 4) && (iyear % 100)) || !(iyear % 400)) { /* leap year */
    day = 366*(year - iyear) + 1;
    for (i = 2; i < 12; i++) 
      Months[i]++;
  }
  else {
    day = 365*(year - iyear) + 1;
  }
  for (i = 0; month[i] < day; i++);
  month = Months[i-1];
  
  if (day + 31*(month + 12*iyear) >= GREG) 
    extra = 2 - (int)(0.01*jy) + (int)(0.25*((int)(0.01*jy)));
  else 
    extra = 0;

  if (year == 0) {
    fprintf (stderr, "error: there is no year zero\n");
    exit (0);
  }
  if (year < 0) 
    year += 1.0;
  if (month > 2) 
    jmonth = month + 1;
  else {
    year -= 1;
    month += 13;
  }

  julday = (int)(365.25*year)+int(30.6001*jm)+id+1720995
