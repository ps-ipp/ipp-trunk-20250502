# include "relastro.h"
# define WEIGHTED_ERRORS 1

// XXX hard-wire the trends identified by CZW
static float BrightMo[] = {-15.6, -16.8, -17.0, -16.7, -16.0};
static float BrightMs[] = {1.3, 1.3, 1.3, 1.8, 2.0}; 

static int Nloop = -1;
static int isImage = FALSE;

// Nloop is used to modify the per detection errors
void AstromErrorSetLoop (int N, int isImageMode) {
  Nloop = N;
  isImage = isImageMode;
}

float GetAstromErrorTiny (MeasureTiny *measure, int mode) {

  PhotCode *code;
  float dPobs, dPsys, dPtotal, dM, AS, MS, dX, dY;

  if (!WEIGHTED_ERRORS) {
    // if we don't understand the errors at all, this at least lets us get things roughly
    // right:
    return 0.1;
  }

  switch (mode) {
    case ERROR_MODE_RA:
      dPobs = FromShortPixels(measure[0].dXccd);  // dXccd is a value in pixels
      break;
    case ERROR_MODE_DEC:
      dPobs = FromShortPixels(measure[0].dYccd);  // dYccd is a value in pixels
      break;
    case ERROR_MODE_POS:
      dX = FromShortPixels(measure[0].dXccd);  // dXccd is a value in pixels
      dY = FromShortPixels(measure[0].dYccd);  // dYccd is a value in pixels
      dPobs = hypot (dX, dY);
      break;
    default:
      abort();
  }

  code 	= GetPhotcodebyCode (measure[0].photcode);
  if (!code) return NAN;

  // do not raise an exception, just send back the result
  if (isnan(code[0].astromErrSys)) return NAN;

  if (measure[0].photcode == 1030) {
    if (mode == ERROR_MODE_RA) {
      dPobs = pow(10.0, (6.0 * measure[0].dXccd - 3.0));  // dXccd is a value in pixels
    }
    if (mode == ERROR_MODE_DEC) {
      dPobs = pow(10.0, (6.0 * measure[0].dYccd - 3.0));  // dXccd is a value in pixels
    }
  }

  AS   	= code[0].astromErrScale;
  MS   	= code[0].astromErrMagScale;
  dPsys = code[0].astromErrSys;
  dM    = measure[0].dM;
  dPtotal = sqrt(SQ(dPsys) + SQ(AS*dPobs) + SQ(MS*dM));

  // for GPC1 data, we have a bright end model:
  if ((measure[0].photcode > 10000) && (measure[0].photcode < 10480)) {
    int Np = ((int) (measure[0].photcode / 100)) % 100;
    myAssert (Np >= 0, "oops");
    myAssert (Np <= 4, "oops");

    float Minst = measure[0].M - measure[0].dt - 25.0;
    float dPbright = 0.335 / (1.0 + exp(BrightMs[Np]*(Minst - BrightMo[Np])));
    dPtotal = hypot(dPtotal, dPbright);
  }
  dPtotal = MAX (dPtotal, MIN_ERROR);

  // early on, we want 2MASS and Tycho to have a very high weight.  This will force images
  // to match the 2MASS / Tycho / ICRS reference frame.  As Nloop gets higher, the weight
  // needs to drop to allow the ps1 measurements to drive the solution
  int isGAIA   = USE_GALAXY_MODEL && !isImage && (measure[0].photcode == 1030);
  int is2MASS  = USE_GALAXY_MODEL && !isImage && (measure[0].photcode >= 2011) && (measure[0].photcode <= 2013);
  int isTycho  = USE_GALAXY_MODEL && !isImage && (measure[0].photcode >= 2020) && (measure[0].photcode <= 2021);

  int hasGAIA  = USE_GALAXY_MODEL &&  isImage && (measure[0].dbFlags & ID_MEAS_OBJECT_HAS_GAIA);
  int has2MASS = USE_GALAXY_MODEL &&  isImage && (measure[0].dbFlags & ID_MEAS_OBJECT_HAS_2MASS);
  int hasTycho = USE_GALAXY_MODEL &&  isImage && (measure[0].dbFlags & ID_MEAS_OBJECT_HAS_TYCHO);

  // modest hack: if the object has 2MASS or Tycho, we set this internal bit and adjust the
  // weight to ensure the image is tied down to the 2mass frame

  if (has2MASS && LoopWeight2MASS && (Nloop >= 0)) {
    dPtotal = dPtotal / LoopWeight2MASS[Nloop];
  }
  if (hasGAIA && LoopWeightGAIA && (Nloop >= 0)) {
    dPtotal = dPtotal / LoopWeightGAIA[Nloop];
  }
  if (hasTycho && LoopWeightTycho && (Nloop >= 0)) {
    dPtotal = dPtotal / LoopWeightTycho[Nloop];
  }

  if (is2MASS && LoopWeight2MASS && (Nloop >= 0)) {
    dPtotal = dPtotal / LoopWeight2MASS[Nloop];
  }
  if (isGAIA && LoopWeightGAIA && (Nloop >= 0)) {
    dPtotal = dPtotal / LoopWeightGAIA[Nloop];
  }
  if (isTycho && LoopWeightTycho && (Nloop >= 0)) {
    dPtotal = dPtotal / LoopWeightTycho[Nloop];
  }
  return (dPtotal);
}

float GetAstromError (Measure *measure, int mode) {

  PhotCode *code;
  float dPobs, dPsys, dPtotal, dM, AS, MS, dX, dY;

  if (!WEIGHTED_ERRORS) {
    // if we don't understand the errors at all, this at least lets us get things roughly
    // right:
    return 0.1;
  }

  switch (mode) {
    case ERROR_MODE_RA:
      dPobs = FromShortPixels(measure[0].dXccd);  // dXccd is a value in pixels
      break;
    case ERROR_MODE_DEC:
      dPobs = FromShortPixels(measure[0].dYccd);  // dYccd is a value in pixels
      break;
    case ERROR_MODE_POS:
      dX = FromShortPixels(measure[0].dXccd);  // dXccd is a value in pixels
      dY = FromShortPixels(measure[0].dYccd);  // dYccd is a value in pixels
      dPobs = hypot (dX, dY);
      break;
    default:
      abort();
  }

  code 	= GetPhotcodebyCode (measure[0].photcode);
  if (!code) return NAN;

  // do not raise an exception, just send back the result
  if (isnan(code[0].astromErrSys)) return NAN;

  if (measure[0].photcode == 1030) {
    if (mode == ERROR_MODE_RA) {
      dPobs = pow(10.0, (6.0 * measure[0].dXccd - 3.0));  // dXccd is a value in pixels
    }
    if (mode == ERROR_MODE_DEC) {
      dPobs = pow(10.0, (6.0 * measure[0].dYccd - 3.0));  // dXccd is a value in pixels
    }
  }

  AS   	= code[0].astromErrScale;
  MS   	= code[0].astromErrMagScale;
  dPsys = code[0].astromErrSys;
  dM    = measure[0].dM;
  dPtotal = sqrt(SQ(dPsys) + SQ(AS*dPobs) + SQ(MS*dM));

  // for GPC1 data, we have a bright end model:
  if ((measure[0].photcode > 10000) && (measure[0].photcode < 10480)) {
    int Np = ((int) (measure[0].photcode / 100)) % 100;
    myAssert (Np >= 0, "oops");
    myAssert (Np <= 4, "oops");

    float Minst = measure[0].M - measure[0].dt - 25.0;
    float dPbright = 0.335 / (1.0 + exp(BrightMs[Np]*(Minst - BrightMo[Np])));
    dPtotal = hypot(dPtotal, dPbright);
  }

  dPtotal = MAX (dPtotal, MIN_ERROR);

  // early on, we want 2MASS and Tycho to have a very high weight.  This will force images
  // to match the 2MASS / Tycho / ICRS reference frame.  As Nloop gets higher, the weight
  // needs to drop to allow the ps1 measurements to drive the solution
  int isGAIA   = USE_GALAXY_MODEL && !isImage && (measure[0].photcode == 1030);
  int is2MASS  = USE_GALAXY_MODEL && !isImage && (measure[0].photcode >= 2011) && (measure[0].photcode <= 2013);
  int isTycho  = USE_GALAXY_MODEL && !isImage && (measure[0].photcode >= 2020) && (measure[0].photcode <= 2021);

  int hasGAIA  = USE_GALAXY_MODEL &&  isImage && (measure[0].dbFlags & ID_MEAS_OBJECT_HAS_GAIA);
  int has2MASS = USE_GALAXY_MODEL &&  isImage && (measure[0].dbFlags & ID_MEAS_OBJECT_HAS_2MASS);
  int hasTycho = USE_GALAXY_MODEL &&  isImage && (measure[0].dbFlags & ID_MEAS_OBJECT_HAS_TYCHO);

  // modest hack: if the object has 2MASS or Tycho, we set this internal bit and adjust the
  // weight to ensure the image is tied down to the 2mass frame

  if (has2MASS && LoopWeight2MASS) {
    dPtotal = dPtotal / LoopWeight2MASS[Nloop];
  }
  if (hasGAIA && LoopWeightGAIA && (Nloop >= 0)) {
    dPtotal = dPtotal / LoopWeightGAIA[Nloop];
  }
  if (hasTycho && LoopWeightTycho && (Nloop >= 0)) {
    dPtotal = dPtotal / LoopWeightTycho[Nloop];
  }

  if (is2MASS && LoopWeight2MASS) {
    dPtotal = dPtotal / LoopWeight2MASS[Nloop];
  }
  if (isGAIA && LoopWeightGAIA && (Nloop >= 0)) {
    dPtotal = dPtotal / LoopWeightGAIA[Nloop];
  }
  if (isTycho && LoopWeightTycho) {
    dPtotal = dPtotal / LoopWeightTycho[Nloop];
  }

  return (dPtotal);
}

/* for a long time, psphot was either not reporting position errors, or was reporting
 * completely wrong astrometry errors.  This function lets us handle, in the
 * configuration, different strategies to generating a position error 
 *
 * astrometry systematic error : this is the minimum expected per-position error.  You
 * should probably measure this from you data.  watch out for the chicken and egg problem!
 *
 * astrometry error scale : this field lets you accept position errors in (say) pixels and
 * convert them with this term to arcsec.  AS : pixel scale in arcsec
 *
 * astrometry mag scale : this field lets you define position errors based on the
 * photometry error.  the scale factor should be something like a typical seeing number
 * (in arcsec) for the given instrument
 *
 */
