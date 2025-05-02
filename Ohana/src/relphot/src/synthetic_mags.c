# include "relphot.h"

static int NSr = -1;
static int NSi = -1;
static int W_SYNTH = -1;

int init_synthetic_mags () {
  int Pr = GetPhotcodeCodebyName ("r");
  int Pi = GetPhotcodeCodebyName ("i");

  NSr = GetPhotcodeNsec (Pr);
  NSi = GetPhotcodeNsec (Pi);

  W_SYNTH = GetPhotcodeCodebyName ("SYNTH.w");
  
  return TRUE;
}

int add_synthetic_mags (AverageTiny *average, SecFilt *secfilt, MeasureTiny *measure, off_t *Nmeasure, off_t *Nm) {
  OHANA_UNUSED_PARAM(average);

  if (!SyntheticPhotometry) return TRUE;

  myAssert (NSr >= 0, "failed to call init_synthetic_mags");
  myAssert (NSi >= 0, "failed to call init_synthetic_mags");
  myAssert (W_SYNTH >= 0, "failed to call init_synthetic_mags");

  // XXX for now, hard-wire the photometric relationship

  // we want to create a synthetic w-band measurement based on the observed mean r & i photometry

  // from Tonry 
  // (w-r)_obs = 0.042 + 0.166 (r-i)_obs - 0.398 (r-i)_obs^2,  (r-i)_obs < 0.5
  // (w-r)_obs = 0.268 - 0.435 (r-i)_obs - 0.078 (r-i)_obs^2,  (r-i)_obs > 0.5
  // thus, for (r-i < 0.5):
  // (r - i < 0.5) : w = r + 0.042 + 0.166*(r-i) - 0.398(r-i)^2
  // (r - i > 0.5) : w = r + 0.268 - 0.435 (r-i) - 0.078(r-i)^2

  float Mr = secfilt[NSr].MpsfChp;
  float Mi = secfilt[NSi].MpsfChp;

  if (!isfinite(Mr)) return FALSE;
  if (!isfinite(Mi)) return FALSE;

  // apply a color correction
  // XXX this is very GPC1 specific and hard-wired -- be very afraid!
  // select the filter; default to fixed photcode and mag limit otherwise

  float Mri = Mr - Mi;

  // saturate at some valid range limits
  Mri = MAX (MIN(Mri, 2.0), -0.2);

  float Mw = NAN;
  if (Mri < 0.5) {
    Mw = Mr + 0.042 + 0.166*(Mri) - 0.398*SQ(Mri);
  } else {
    Mw = Mr + 0.268 - 0.435*(Mri) - 0.078*SQ(Mri);
  }

  dvo_measureT_init (measure);

  measure->M = Mw;
  measure->dM = 0.001;
  measure->photcode = W_SYNTH;
  measure->dbFlags |= ID_MEAS_SYNTH_MAG;

  (*Nmeasure) ++;
  (*Nm) ++;

  return TRUE;
}
