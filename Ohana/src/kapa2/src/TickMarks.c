# include "Ximage.h"

TickMarkData *CreateAxisTicks (Axis *axis, int *nticks) {

  TickMarkData *ticks;
  double range, major, minor, first, value;
  int i, NTICKS, done, nsignif;

  *nticks = 0;

  if (isnan(axis->min) || isinf(axis->min)) return NULL;
  if (isnan(axis->max) || isinf(axis->max)) return NULL;
  
  // length of the axis in pixels
  // int nPixels = hypot(axis->dfx, axis->dfy);
  // double dPixels = nPixels / range; // axis pixel-scale

  AxisTickScale (axis, &range, &major, &minor, &nsignif);

  // be a little generous 
  NTICKS = MIN((int)(fabs(range / minor)) + 2, 1000);
  ALLOCATE (ticks, TickMarkData, NTICKS);

  // value of the first tick mark (near axis->min)
  double minValue = axis->min / minor;
  int ifirst = (int) minValue;

  if ((axis->min > 0.0) && (minValue - ifirst > +0.1)) {
    ifirst ++;
  } 
  if ((axis->min < 0.0) && (minValue - ifirst < -0.1)) {
    ifirst --;
  }
  first = minor*ifirst;
  
  // only write the labels for the inner fRange of the axies
  double minLabelValue = 0.5*(1.0 - 1.001*axis->fLabelRange)*(axis->max - axis->min) + axis->min;
  double maxLabelValue = 0.5*(1.0 + 1.001*axis->fLabelRange)*(axis->max - axis->min) + axis->min;

  // loop to find the ticks
  done = FALSE;
  value = first; 
  for (i = 0; !done && (i < NTICKS); i++) {
    ticks[i].IsMajor = FALSE;
    ticks[i].IsMajor |= (fabs((int)(value/major) - (value/major)) < 0.5*(minor/major));
    ticks[i].IsMajor |= (fabs((int)((value + 0.5*minor)/major) - (value/major)) < 0.5*(minor/major));
    ticks[i].IsMajor |= (fabs((int)((value - 0.5*minor)/major) - (value/major)) < 0.5*(minor/major));
    int inLabelRange = (range > 0) ? (value >= minLabelValue) && (value <= maxLabelValue) : (value <= minLabelValue) && (value >= maxLabelValue);
    ticks[i].IsLabel = inLabelRange && ticks[i].IsMajor && axis->islabel;
    ticks[i].value = value;
    ticks[i].nsignif = nsignif;
    if (range > 0) 
      value += minor;
    else 
      value -= minor;
    
    done |= (range > 0) && (value > axis->max + 0.1*minor);
    done |= (range < 0) && (value < axis->max - 0.1*minor);
    // fprintf (stderr, "%d : %f %f %f %f\n", done, range, value, axis->max, 0.1*minor);

    // set the format
    if (strcasecmp(axis->format, "auto")) {
      strcpy (ticks[i].format, axis->format);
    } else {
      int Nexp = abs(nsignif);
      if (Nexp > 3) {
	strcpy (ticks[i].format, "%.1e");
      } else {
	if (nsignif < 0) {
	  snprintf (ticks[i].format, 16, "%%.%df", -1 * nsignif);
	} else {
	  strcpy (ticks[i].format, "%.1f");
	}
      }
    }
  }

  *nticks = i;
  return (ticks);
}

int PrintTick (char *string, TickMarkData *tick, double min, double max) {

  double value = tick->value;

  // handle "0" a the axis
  if (fabs(value/(max - min)) < 0.001) { 
    value = 0.0; 
  }

  int Nchar = sprintf (string, tick->format, value);
  return Nchar;
}

// do I really want this?  allow an override?
# define MIN_RANGE 1e-30

// range : range o
void AxisTickScale (Axis *axis, double *range, double *major, double *minor, int *nsignif) {

  double lrange, factor, mantis, fmantis, power;

  // range is the full range of the plot axis (e.g., 10 - 1010 : range = 1000)
  *range = axis->fLabelRange*(axis[0].max - axis[0].min);
  if (fabs(*range) < MIN_RANGE) {
    *range = (*range < 0) ? -MIN_RANGE : +MIN_RANGE;
  }

  // lrange : log of the full range (e.g., 3000 -> ~3.5)
  lrange = log10(MAX(fabs(*range), MIN_RANGE));
  double truncRange = ((int) ((lrange + 0.005) * 1000)) / 1000.0;
  lrange = truncRange;

  // split the lrange into the integer and fractional portions
  // e.g., range = 3000, lrange ~ 3.5, factor = 3, mantis = 0.5
  // if lrange is negative (range < 1), and mantis is negative, shift so mantis positive
  // e.g., range = 0.2, lrange = -0.69, mantis = -0.69, factor = 0 -> factor = -1, mantis = 0.301 (2 x 0.1 instead of 0.2 x 1)
  mantis = modf (lrange, &factor);
  if (mantis < 0.0) {
    mantis += 1.0;
    factor -= 1.0;
  }

  // how many significant digits are needed?

  // power is 10^factor above, e.g., range = 3000, factor = 3, power = 1000 (integer power-of-ten)
  // fmantis is the 10^mantis above, e.g., range = 3000, mantis = 0.5, fmantis = 3
  power = MAX(pow(10.0, factor), MIN_RANGE);
  fmantis = pow(10.0, mantis);
  *major = MAX(0.5 * power, MIN_RANGE);
  
  if ((fmantis >= 1.0) && (fmantis <  1.999)) {
    *major = 0.5 * power;
    *nsignif = factor - 1;
    if (axis[0].areticks == 1) {
      *major = 0.25 * power;
      *nsignif = factor - 2;
    }	  
    if (axis[0].areticks == 3) {
      *major = 1.0 * power;
      *nsignif = factor - 1;
    }	  
  }
  if ((fmantis >= 1.999) && (fmantis <  3.999)) {
    *major = 1.0 * power;
    *nsignif = factor;
    if (axis[0].areticks == 1) {
      *major = 0.5 * power;
      *nsignif = factor - 1;
    }	  
    if (axis[0].areticks == 3) {
      *major = 2.0 * power;
      *nsignif = factor;
    }	  
  }
  if ((fmantis >= 3.999) && (fmantis <  5.999)) {
    *major = 1.0 * power;
    *nsignif = factor;
    if (axis[0].areticks == 1) {
      *major = 1.0 * power;
      *nsignif = factor;
    }	  
    if (axis[0].areticks == 3) {
      *major = 2.0 * power;
      *nsignif = factor;
    }	  
  }
  if ((fmantis >= 5.999) && (fmantis <   7.999)) {
    *major = 2.0 * power;
    *nsignif = factor;
    if (axis[0].areticks == 1) {
      *major = 1.0 * power;
      *nsignif = factor;
    }	  
    if (axis[0].areticks == 3) {
      *major = 4.0 * power;
      *nsignif = factor;
    }	  
  }
  if ((fmantis >= 7.999) && (fmantis <  10.000)) {
    *major = 2.5 * power;
    *nsignif = factor - 1;
    if (axis[0].areticks == 1) {
      *major = 2.0 * power;
      *nsignif = factor;
    }	  
    if (axis[0].areticks == 3) {
      *major = 4.0 * power;
      *nsignif = factor;
    }	  
  }
  if (isfinite(axis->dMajor)) *major = axis->dMajor;

  *minor = *major / axis->fMinor;
}

