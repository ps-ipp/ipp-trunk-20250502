# include "relphot.h"

# define UPPER90_VAL MpsfStk
# define UPPER80_VAL FpsfStk
# define LOWER20_VAL MkronStk
# define LOWER10_VAL FkronStk
# define UPPER90_SIG MpsfWrp 
# define UPPER80_SIG FpsfWrp 
# define LOWER20_SIG MkronWrp
# define LOWER10_SIG FkronWrp


# if (0)
# define TEST_OBJ_ID 0x000149b0 
# define TEST_CAT_ID 0x00000001
# else
# define TEST_OBJ_ID 0
# define TEST_CAT_ID 0
# endif

int magStatsByRanking (StatDataSet *dataset, StatType *stats);
int magStatsByRankingClipped (StatDataSet *dataset, StatType *stats);
int magStatsByRankingIRLS (StatDataSet *dataset, StatType *stats);
int markMeasureByRanking (StatDataSet *dataset, Measure *measure, int minrank, DVOMeasureFlags keepflag, DVOMeasureFlags maskflag);
void GetPhotFlagStats (uint32_t *photFlagUpper, uint32_t *photFlagLower, uint32_t *photflag_list, int Nphotflag);
void sort_entry_by_offset (double *offset, int *entry, int N);
void sort_StatDataSet (StatDataSet *dataset);
int liststats_irls (StatDataSet *dataset, int Npoints, StatType *stats);

// # define MAG_STATS_BY_RANKING magStatsByRankingIRLS
// # define MAG_STATS_BY_RANKING magStatsByRanking

# define UBERCAL_WEIGHT 100.0

# define SKIP_THIS_MEAS(REASON) {			\
    results->REASON ++;					\
    continue; }

# define SKIP_THIS_MEAS_STACK(REASON) {			\
    continue; }

# define CHECK_VALID_MAG(MAG,D_MAG) (isfinite(MAG) && isfinite(D_MAG) && (MAG > -5.0) && (MAG < 30.0))
# define CHECK_VALID_FLUX(FLUX,D_FLUX) (isfinite(FLUX) && isfinite(D_FLUX))

// set A to B if B > A or if A is NAN, as long as B is not NAN
# define MAX_NOT_NAN(A,B) { if (isfinite(B) && (!isfinite(A) || (B > A))) A = B; }

static float MagToFlux (float Mag) {
  float Flux = pow(10.0, -0.4*(Mag));
  return (Flux);
}

int print_measure_set_alt (Average *average, SecFilt *secfilt, Measure *measure) {

  int Ns;
  off_t k;

  int Nsecfilt = GetPhotcodeNsecfilt ();

  for (k = 0; k < average[0].Nmeasure; k++) {
    fprintf (stderr, "meas: %08x\n", measure[k].dbFlags);
  }

  for (Ns = 0; Ns < Nsecfilt; Ns++) {
    fprintf (stderr, "secf: %08x\n", secfilt[Ns].flags);
  }
  return (TRUE);
}

int useOLS = TRUE; // chosen based on iteration below

int setMrelCatalog (Catalog *catalog, int Nc, int isSetMrelFinal, SetMrelInfo *results, int Nsecfilt) {

  off_t j;

  // the same STATMODE is used for chip and warp averages
  liststats_setmode (&results->psfstats,  STATMODE);
  liststats_setmode (&results->apstats,   STATMODE);
  liststats_setmode (&results->kronstats, STATMODE);

  SetMrelInfoReset (results); // reset the counters

  // XXX if we are iterating on the image zero points, we should use OLS
  // until we have performed a few iterations.  UseStandardOLS() returns
  // true (use OLS) or false depending on the zero point mode (TGROUP, MOSAIC, IMAGE) and the
  // number of iterations.

  // On the final pass (isSetMrelFinal), we should use OLS or IRLS depending on
  // the user choice (USE_OLS_FOR_AVERAGES)

  useOLS = isSetMrelFinal ? USE_OLS_FOR_AVERAGES : UseStandardOLS (ZPT_STARS);
  for (j = 0; j < catalog[Nc].Naverage; j++) {

    if (STAGES & STAGE_CHIP) {
      setMrelAverageExposure (&catalog[Nc], Nc, j, Nsecfilt, isSetMrelFinal, results);
    }

    // only apply Stack operation on setMrelFinal in first pass 
    if (isSetMrelFinal && (STAGES & STAGE_STACK) && !IS_DIFF_DB) {
      setMrelAverageStack (&catalog[Nc], Nc, j, Nsecfilt);
    }

    // only measure force-warp mean values if issetMrelFinal (make it optional?)
    if (isSetMrelFinal && (STAGES & STAGE_WARP)) {
      setMrelAverageForcedWarp (&catalog[Nc], Nc, j, Nsecfilt, results);
    }
  }
  return (TRUE);
}

// NOTE: 
// Msys is measure[i].M + zp corrections
// Mcal is image[j].Mcal
// Mmos and Mgrid are offsets for mosaic and grid

// tie down reference photometry if the -refcode (code) option is selected
// eg, -refcode g_SDSS
// this probably makes no sense in the context of multifilter analysis
// XXX probably need to use the photcode table to assign reference mag weights.

// dlist gives the error per measurement, wlist gives the weight
// we can modify the error and weight in a few ways:
// 1) MIN_ERROR guarantees a floor
// 2) photomErrSys is added in quadrature as a sytematic error, set per photcode
// 3) UBERCAL measurements can have their weight increased by a big factor to help tie down the averages
// 4) some reference photcode of some kind can be specified as fixed and have a high weight

// Although I calculate McalAPER for exposures, I am only using McalPSF for chips.  Note
// in StarOps.c:setMcalOutput I am setting measure->McalAPER to image->McalPSF for chips
// and warps, but not stacks

// set mean of chip measurements (selected by photcode range for now):
int setMrelAverageExposure (Catalog *catalog, int cat, off_t ave, int Nsecfilt, int isSetMrelFinal, SetMrelInfo *results) {

  off_t k;

  // we are guaranteed to have averageT, but not average
  AverageTiny *averageT = &catalog[0].averageT[ave];

  off_t measureOffset = averageT->measureOffset;
  MeasureTiny *measureT = &catalog[0].measureT[measureOffset];

  // we are NOT guaranteed to have average, measure, secfilt
  Average *average  = catalog[0].average     ? &catalog[0].average[ave]               : NULL;
  Measure *measure  = catalog[0].measure     ? &catalog[0].measure[measureOffset]     : NULL;
  SecFilt *secfilt  = catalog[0].secfilt     ? &catalog[0].secfilt[ave*Nsecfilt]      : NULL;
  char *measureRank = catalog[0].measureRank ? &catalog[0].measureRank[measureOffset] : NULL;

  // we are measuring means for 3 types of mags: psf, ap, kron.

  // in the final assignment, set the mean mags even if only 1 measurements exists
  int Nminmeas = isSetMrelFinal ? 1 : STAR_TOOFEW + 1;

  // isSetMrelFinal : in the final pass, we set psf, kron, ap mag values and extra stats; other passes only do psf mags

  // option for a test print
  if ((averageT[0].objID == TEST_OBJ_ID) && (averageT[0].catID == TEST_CAT_ID)) {
    fprintf (stderr, "test obj\n");
    print_measure_set_alt (average, secfilt, measure);
  }

  SetMrelInfoResetObject (results); // reset the per-object arrays

  int NextPS1 = 0;
  int NpsfPS1 = 0;
  int GoodPS1 = FALSE;
  int Good2MASS = FALSE;
  int Galaxy2MASS = FALSE;
  int haveTYCHO = FALSE;
  int haveHSC = FALSE;
  int haveCFH = FALSE;

  float stargalmax = 0.0;

  // assign measurements to the photcode lists 
  for (k = 0; k < averageT[0].Nmeasure; k++) {

    // these bits should not be set unless we use them in this pass
    // (note that we can only un-set them in the final pass when we actually have measure, not just measureT
    // CCL updated 2021.12.09:  the following line will reset the dbFlags for measure so I comment it out.
    // if (measure) measure[k].dbFlags &= ~(ID_MEAS_PHOTOM_PSF | ID_MEAS_PHOTOM_APER | ID_MEAS_PHOTOM_KRON);

    // skip measurements that do not have a valid photcode (raise exception?)
    PhotCode *code = GetPhotcodebyCode (measureT[k].photcode);
    myAssert (code, "invalid photcode??");

    // SKIP gpc1 stack data (hard-wired photcodes)
    if (isGPC1stack(measureT[k].photcode)) continue;
    
    // SKIP gpc1 forced-warp data (hard-wired photcodes)
    if (isGPC1warp(measureT[k].photcode)) continue;
    
    if (isTYCHO(measureT[k].photcode)) { haveTYCHO = TRUE; }
    if (isHSCchip(measureT[k].photcode)) { haveHSC = TRUE; }
    if (isCFHchip(measureT[k].photcode)) { haveCFH = TRUE; }

    if (is2MASS(measureT[k].photcode)) {
      if (measureT[k].photFlags & 0x00c00000) {
	Galaxy2MASS = TRUE; // per object value
      }
      if (measureT[k].photFlags & 0x00000007) {
	Good2MASS = TRUE; // per object value 
      }
    }

    // assign the Nsec value so we can assign to the right lists
    int Nsec = GetPhotcodeNsec (code->equiv);
    if (Nsec < 0) continue; // skip measurements which do not have an equiv average photcode

    results->Nmeas[Nsec] ++;

    // various steps only relevant to the final pass 
    if (isSetMrelFinal) {
      MAX_NOT_NAN (results->psfQfMax[Nsec],     measure[k].psfQF);
      MAX_NOT_NAN (results->psfQfPerfMax[Nsec], measure[k].psfQFperf);

      // are we a PS1 exposure photcode? (hard-wired photcodes)
      if (isGPC1chip(measure[k].photcode)) { 
	results->NexpPS1[Nsec] ++; 
	results->havePS1[Nsec] = TRUE; 
	
	// only count psfQF > 0.85 here
	if (isfinite(measure[k].extNsigma) && isfinite(measure[k].psfQF) && (measure[k].psfQF > 0.85)) {
	  stargalmax = MAX (stargalmax, measure[k].extNsigma);
	  results->stargal_list[results->Nstargal] = measure[k].extNsigma;
	  results->Nstargal ++;
	}

	// for all GPC1 measurements, track the photflags
	results->photflag_list[results->Nphotflags] = measure[k].photFlags;
	results->Nphotflags ++;
      }

      // force the use of SYN even if we have PS1 mags?
      if (isGPC1synth(measure[k].photcode)) {
	results->haveSYN[Nsec] = TRUE;
	results->measSYN[Nsec] = k;
	measureT[k].dbFlags |= ID_MEAS_SYNTH_MAG; // redundant with photcode
	// note that synthetic mags have the real mags in measure.M (no zero point offset)
	if (measure[k].M < results->minSYN[Nsec]) results->needSYN[Nsec] = TRUE;
	continue;
      }
    }

    /* some things to note here:

       Mcal, Mmos, Mgrid are only relevant (non-zero) during the relphot_images and
       relphot_parallel_images analysis steps. For the final output steps (setMrelFinal),
       these are turned off

       During the image analysis, Mflat is the prior stored value while Mgrid is the new
       measurement.  In the final output step, Mgrid is transferred to Mflat and zero

       On the final calculation of Mrel, the value of Mgrid has been applied to Mflat, but
       it has not been removed from the GridOps.c structures.  In order to avoid
       double-counting, we need to skip Mgrid on the final calculation.
    */

    // ** Choose the calibration (depends on the mode : do I have an image reference or not?) 
    float Mcal = 0.0, Mmos = 0.0, Mgrid = 0.0, Mflat = 0.0, Mgrp = 0.0;
    float dMgrp = 0.0;
    off_t meas = measureOffset + k;
    if (getImageEntry (meas, cat) < 0) {
      // measurements without an image are either external reference photometry or
      // data for which the associated image has not been loaded (probably because of
      // overlaps).  Msys + measure.Mcal is our best guess of the true magnitude
      Mcal = measureT[k].McalPSF; // check that this is zero for loaded REF value
      // but external reference photometry might not have Mcal set, so set to 0.0 in that case.
      if (isnan(Mcal)) Mcal = 0.0;
    } else {
      // getMcal returns image[].Mcal
      Mcal  = getMcal (meas, cat, MAG_CLASS_PSF);
      if (isnan(Mcal))  SKIP_THIS_MEAS(Ncal);

      // the flat-field correction is stored in measure.Mflat
      // if measureT exists, we are in the relphot_image analysis loops
      // if measureT does not exist, we are updating the measurements and
      // need to use the 'measure' version
      if (measureT) {
	Mflat = isnan (measureT[k].Mflat) ? 0.0 : measureT[k].Mflat;
      } else if (measure) {
        Mflat = isnan (measure[k].Mflat) ? 0.0 : measure[k].Mflat;
      }

      // see note above re: final output vs image analysis
      if (!isSetMrelFinal) {
	Mmos  = getMmos  (meas, cat);
	if (isnan(Mmos))  SKIP_THIS_MEAS(Nmos);
	Mgrp  = getMgrp  (meas, cat, measureT[k].airmass, &dMgrp);
	if (isnan(Mgrp))  SKIP_THIS_MEAS(Ngrp);
	Mgrid = getMgridTiny (&measureT[k]); 
	if (isnan(Mgrid)) SKIP_THIS_MEAS(Ngrid);
      }
    }

    int myUbercalDist = getUbercalDist(meas, cat);
    results->minUbercalDist[Nsec] = MIN(results->minUbercalDist[Nsec], myUbercalDist);

    int isUbercal = (measureT[k].dbFlags & ID_MEAS_PHOTOM_UBERCAL);

    if (isUbercal) results->haveUbercal[Nsec] = TRUE; // haveUbercal is set per secfilt, isUbercal is per measure XXX define this array

    // logic for choosing a modified weight:
    float modifiedWeight = 1.0;
    if (isUbercal) modifiedWeight = UBERCAL_WEIGHT;
    if (refPhotcode && (code->code == refPhotcode->code)) modifiedWeight = UBERCAL_WEIGHT;
    if (USE_REFERENCE_WEIGHT && (code->type == PHOT_REF)) modifiedWeight = code->photomErrSys; // we are overloading this field for now

    // XXX I should probably do something more clever with the measured scatter in the nights, mosaics, images

    // combine the various errors in quadrature here:
    float dMsys = hypot (code->photomErrSys, dMgrp);

    float Map  = NAN;
    float dMap = NAN;

    // This definition is consistent with PhotRel: Mrel = Msys - Mcal - Mflat
    float Moff =  Mcal + Mmos + Mgrp + Mgrid + Mflat;

    if (isSetMrelFinal) {
      Map = PhotCat (&measure[k], MAG_CLASS_APER);

      // NOTE: for PV3, apFluxErr is broken (see pmSourcePhotometry.c:245).  we are going to
      // use PSF flux error instead here:
      // dMap = MAX (hypot(measure[k].dMap,   dMsys), MIN_ERROR);
      dMap = MAX (hypot(measure[k].dM,   dMsys), MIN_ERROR);

      if (CHECK_VALID_MAG(Map, dMap)) {
	int Nap = results->aperData[Nsec].Nlist;
	results->aperData[Nsec].flxlist[Nap] = Map - Moff; // this is consistent with PhotRel
	results->aperData[Nsec].errlist[Nap] = dMap;
	results->aperData[Nsec].wgtlist[Nap] = modifiedWeight;
	results->aperData[Nsec].ranking[Nap] = measureRank ? measureRank[k] : 0;
	results->aperData[Nsec].measSeq[Nap] = k;
	results->aperData[Nsec].msklist[Nap] = 0;
	results->aperData[Nsec].Nlist ++;
      }
    }

    // NOTE: we need to calculate the averge chip Kron on each pass to be able to calibrate the stacks
    float Mkron = PhotCatTiny (&measureT[k], MAG_CLASS_KRON);
    float dMkron;
    if (isSetMrelFinal) {
      dMkron = MAX (hypot(measure[k].dMkron, dMsys), MIN_ERROR);
    } else {
      dMkron = MAX (hypot(measureT[k].dM, dMsys), MIN_ERROR);
    }
    if (CHECK_VALID_MAG(Mkron, dMkron)) {
      int Nkron = results->kronData[Nsec].Nlist;
      results->kronData[Nsec].flxlist[Nkron] = Mkron - Moff; // this is consistent with PhotRel
      results->kronData[Nsec].errlist[Nkron] = dMkron;
      results->kronData[Nsec].wgtlist[Nkron] = modifiedWeight;
      results->kronData[Nsec].ranking[Nkron] = measureRank ? measureRank[k] : 0;
      results->kronData[Nsec].measSeq[Nkron] = k;
      results->kronData[Nsec].msklist[Nkron] = 0;
      results->kronData[Nsec].Nlist ++;
    }

    float Mpsf = PhotSysTiny (&measureT[k], &averageT[0], &secfilt[0], MAG_CLASS_PSF);
    float dMpsf  = MAX (hypot(measureT[k].dM, dMsys), MIN_ERROR);
    if (CHECK_VALID_MAG(Mpsf, dMpsf)) {
      int Npsf = results->psfData[Nsec].Nlist;
      results->psfData[Nsec].flxlist[Npsf] = Mpsf - Moff; // this is consistent with PhotRel
      results->psfData[Nsec].errlist[Npsf] = dMpsf;
      results->psfData[Nsec].wgtlist[Npsf] = modifiedWeight;
      results->psfData[Nsec].ranking[Npsf] = measureRank ? measureRank[k] : 0;
      results->psfData[Nsec].measSeq[Npsf] = k;
      results->psfData[Nsec].msklist[Npsf] = 0;
      results->psfData[Nsec].Nlist ++;
    }

    // count the extended detections (all PS1 bands)
    if (isSetMrelFinal && isGPC1chip(measureT[k].photcode) && !isnan(Map) && !isnan(Mpsf)) {
      float dMagAp = Mpsf - Map;
      float SigmaAp = hypot(0.1, 2.5*dMpsf);
      // XXX this is still quite ad hoc, but at least it:
      // (a) converges to 0.1 mag offset at the bright end
      // (b) converges to 0.5 mag offset at the faint end (dM = 0.2)
      if (dMagAp > SigmaAp) {
	results->Next[Nsec] ++;
	NextPS1 ++;
      } else {
	NpsfPS1 ++;
      }
    }
  }

  SynthZeroPoints *synthzpts = SynthZeroPointsGet();

  // find max values across filters
  float psfQfMax     = 0.0;
  float psfQfPerfMax = 0.0;

  // now calculate the mean stats for the selected Nsec bands.
  for (int Ns = 0; Ns < Nphotcodes; Ns++) {
    int thisCode = photcodes[Ns][0].code;
    int Nsec = GetPhotcodeNsec(thisCode);

    // -preserve-ps1 means keep an existing average PS1 value
    // if we did not detect this object in PS1, or do not request -preserve-ps1, keep the mean photometry values
    if (!PRESERVE_PS1 || !(secfilt[Nsec].flags & ID_SECF_HAS_PS1)) {
      dvo_secfilt_init (&secfilt[Nsec], SECFILT_RESET_CHIP); // this does not reset astrometry or STACK bits
    }

    if (haveTYCHO) {
      secfilt[Nsec].flags |= ID_SECF_HAS_TYCHO;
    }
    if (haveHSC) {
      secfilt[Nsec].flags |= ID_SECF_HAS_HSC;
    }
    if (haveCFH) {
      secfilt[Nsec].flags |= ID_SECF_HAS_CFH;
    }
//  secfilt[Nsec].flags |= ID_SECF_AST_GAIA ; // for building reference catalog only, STAR_FEW need to comment out
//  secfilt[Nsec].flags |= ID_SECF_PHO_SYNTH; // for building reference catalog only, STAR_FEW need to comment out
//  secfilt[Nsec].flags |= ID_SECF_PHO_ATLAS; // for building reference catalog only, STAR_FEW need to comment out
//  secfilt[Nsec].flags |= ID_SECF_PHO_PV3  ; // for building reference catalog only, STAR_FEW need to comment out
//  secfilt[Nsec].flags |= ID_SECF_PHO_SOUTH; // for building reference catalog only, STAR_FEW need to comment out

    // if -preserve-ps1 is set and this object has PS1 data, skip the rest of the steps:
    // NOTE: This test only applies if this bit was set by an earlier analysis
    if (PRESERVE_PS1 && (secfilt[Nsec].flags & ID_SECF_HAS_PS1)) continue;

    // XXX hardwired grizy = (01234) JHK = (567) w = (8)
    if (Nsec < 5) {
      secfilt[Nsec].Ncode = results->NexpPS1[Nsec]; 
    } else {
      secfilt[Nsec].Ncode = results->Nmeas[Nsec]; // 2MASS data if it exists
    }

    // use the synthetic magnitude for mean photometry (apply zpt calibration if no tycho
    // photometry exists)
    if (results->needSYN[Nsec] || (!results->havePS1[Nsec] && results->haveSYN[Nsec])) {
      if (!isSetMrelFinal) continue;

      // use the single SYNTH value instead of the other mags here
      float Mpsf = measure[results->measSYN[Nsec]].M;
      float ZP = 0.0;

      // if we have loaded the synth zpt correction, and do NOT have a tycho measurement, apply the correction
      if (!haveTYCHO && synthzpts) {
	// need to look up the (X,Y) coords from this (R,D) location
	double X, Y;
	double R = ohana_normalize_angle_to_midpoint (measure->R, 0.0); // XXX 0.0 or 180.0 for center?
	RD_to_XY (&X, &Y, R, measure->D, &synthzpts->coords);
	if (X < 0) return FALSE;
	if (Y < 0) return FALSE;
	if (X >= synthzpts->Nx) return FALSE;
	if (Y >= synthzpts->Ny) return FALSE;
	
	int Xpix = X;
	int Ypix = Y;
	int Npix = Xpix + Ypix*synthzpts->Nx;
	
	float *value = (float *) synthzpts->matrix[Nsec].buffer;
	ZP = !isnan(value[Npix]) ? value[Npix] : 0.0;
	if (isfinite(value[Npix])) {
	  secfilt[Nsec].flags |= ID_SECF_FIX_SYNTH;
	}
      }
      secfilt[Nsec].MpsfChp  = Mpsf + ZP;
      secfilt[Nsec].dMpsfChp = 0.6;
      secfilt[Nsec].Mchisq   = 0.0;
      secfilt[Nsec].flags   |= ID_SECF_USE_SYNTH;
      continue;
    } 

    // if too few valid measurements meet the minimum criteria, go to the next entry
    StatType *psfstats = &results->psfstats;

    // if isSetMrelFinal and varstats are requested, request percentile ranges
    if (isSetMrelFinal && VARIABILITY_STATS) psfstats->statmode |= STATS_VARSTATS;

    int NrankingPSF = useOLS ? magStatsByRanking (&results->psfData[Nsec], psfstats) : magStatsByRankingIRLS (&results->psfData[Nsec], psfstats);
    if (NrankingPSF < Nminmeas) { 
      secfilt[Nsec].flags |= ID_SECF_STAR_FEW;
    } else {
      secfilt[Nsec].MpsfChp  = psfstats->mean;
      secfilt[Nsec].dMpsfChp = psfstats->error;
      secfilt[Nsec].Mchisq   = (psfstats->Nmeas > 1) ? psfstats->chisq : NAN;
      secfilt[Nsec].flags &= ~ID_SECF_STAR_FEW;
    }

    // when running -averages, we have no information about the images, so we cannot set this
    if (results->minUbercalDist[Nsec] > -1) {
      secfilt[Nsec].ubercalDist = results->minUbercalDist[Nsec];
    }

    StatType *kronstats = &results->kronstats;
    int NrankingKRON = useOLS ? magStatsByRanking (&results->kronData[Nsec], kronstats) : magStatsByRankingIRLS (&results->kronData[Nsec], kronstats);
    if (NrankingKRON) {
      secfilt[Nsec].MkronChp  = kronstats->mean; 
      secfilt[Nsec].dMkronChp = kronstats->error; 
      secfilt[Nsec].sMkronChp = kronstats->sigma; 
      // actually used is Nmeas, NrankingKRON is including masked data
      secfilt[Nsec].NusedKron = kronstats->Nmeas;
    }

    if (isSetMrelFinal) {
      if ((average[0].objID == TEST_OBJ_ID) && (average[0].catID == TEST_CAT_ID)) {
	fprintf (stderr, "test obj\n");
      }

      if (NrankingPSF) {
	secfilt[Nsec].sMpsfChp = psfstats->sigma;
	secfilt[Nsec].Nused    = psfstats->Nmeas;
	secfilt[Nsec].Mmax     = psfstats->max;
	secfilt[Nsec].Mmin     = psfstats->min;
      }
      if (VARIABILITY_STATS) {
	// XXX NOTE: these are overloaded on warp and stack mags and fluxes
	secfilt[Nsec].UPPER90_VAL = psfstats->Upper90;
	secfilt[Nsec].UPPER80_VAL = psfstats->Upper80;
	secfilt[Nsec].LOWER20_VAL = psfstats->Lower20;
	secfilt[Nsec].LOWER10_VAL = psfstats->Lower10;
	secfilt[Nsec].UPPER90_SIG = psfstats->Upper90Nsig;
	secfilt[Nsec].UPPER80_SIG = psfstats->Upper80Nsig;
	secfilt[Nsec].LOWER20_SIG = psfstats->Lower20Nsig;
	secfilt[Nsec].LOWER10_SIG = psfstats->Lower10Nsig;
      }
      secfilt[Nsec].psfQfMax     = results->psfQfMax[Nsec];
      secfilt[Nsec].psfQfPerfMax = results->psfQfPerfMax[Nsec];

      MAX_NOT_NAN (psfQfMax,     secfilt[Nsec].psfQfMax);
      MAX_NOT_NAN (psfQfPerfMax, secfilt[Nsec].psfQfPerfMax);

      // mark the measurements matching this ranking 
      int minRankPSF = (NrankingPSF > 0) ? results->psfData[Nsec].ranking[0] : 10;
      markMeasureByRanking (&results->psfData[Nsec], measure, minRankPSF, ID_MEAS_PHOTOM_PSF, ID_MEAS_MASKED_PSF);

      int minRankKron = (NrankingKRON > 0) ? results->kronData[Nsec].ranking[0] : 10;
      markMeasureByRanking (&results->kronData[Nsec], measure, minRankKron, ID_MEAS_PHOTOM_KRON, ID_MEAS_MASKED_KRON);

      StatType *apstats = &results->apstats;
      int NrankingAPER = useOLS ? magStatsByRanking (&results->aperData[Nsec], apstats) : magStatsByRankingIRLS (&results->aperData[Nsec], apstats);
      if (NrankingAPER) {
	secfilt[Nsec].MapChp  = apstats->mean; 
	secfilt[Nsec].dMapChp = apstats->error; 
	secfilt[Nsec].sMapChp = apstats->sigma; 
        // actually used is Nmeas, NrankingAPER is including masked data
	secfilt[Nsec].NusedAp = apstats->Nmeas;
      }
      int minRankAper = (NrankingAPER > 0) ? results->aperData[Nsec].ranking[0] : 10;
      markMeasureByRanking (&results->aperData[Nsec], measure, minRankAper, ID_MEAS_PHOTOM_APER, ID_MEAS_MASKED_APER);

      // does this object appear extended in > 50% of measurements?
      if ((results->Next[Nsec] > 0) && (results->Next[Nsec] > 0.5*psfstats->Nmeas)) {
	secfilt[Nsec].flags |= ID_SECF_OBJ_EXT;
      }

      switch (minRankPSF) {
	case 0:
	  secfilt[Nsec].flags |= ID_SECF_RANK_0;
	  if (results->havePS1[Nsec]) GoodPS1 = TRUE;
	  break;
	case 1:
	  secfilt[Nsec].flags |= ID_SECF_RANK_1;
	  if (results->havePS1[Nsec]) GoodPS1 = TRUE;
	  break;
	case 2:
	  secfilt[Nsec].flags |= ID_SECF_RANK_2;
	  if (results->havePS1[Nsec]) GoodPS1 = TRUE;
	  break;
	case 3:
	  secfilt[Nsec].flags |= ID_SECF_RANK_3;
	  break;
	case 4:
	  secfilt[Nsec].flags |= ID_SECF_RANK_4;
	  break;
	default:
	  break;
      }
      if (results->havePS1[Nsec]) {
	secfilt[Nsec].flags |= ID_SECF_HAS_PS1;
      }	
      if (results->haveUbercal[Nsec]) {
	secfilt[Nsec].flags |= ID_SECF_USE_UBERCAL;
      }
    }
  }

  if (isSetMrelFinal) {
    if (NextPS1 && (NextPS1 > NpsfPS1)) {
      average[0].flags |= ID_OBJ_EXT;
    }
    if (GoodPS1) {
      average[0].flags |= ID_OBJ_GOOD;
    }
    if (Galaxy2MASS) {
      average[0].flags |= ID_OBJ_EXT_ALT;
    }
    if (Good2MASS) {
      average[0].flags |= ID_OBJ_GOOD_ALT;
    }

    GetPhotFlagStats (&average[0].photFlagsUpper, &average[0].photFlagsLower, results->photflag_list, results->Nphotflags);

    // calculate the median of the stargal (ExtNsigma) values
    dsort(results->stargal_list, results->Nstargal);
    double stargal_median;
    if (results->Nstargal % 2) {
      int Ncenter = results->Nstargal / 2;
      stargal_median = results->stargal_list[Ncenter];
    } else {
      int Ncenter = results->Nstargal / 2 - 1;
      stargal_median = 0.5*(results->stargal_list[Ncenter] + results->stargal_list[Ncenter + 1]);
    }
    average[0].stargal   = stargal_median;

    average[0].psfQF     = psfQfMax;
    average[0].psfQFperf = psfQfPerfMax;
  }
  return (TRUE);
}

// only apply Stack operation on setMrelFinal in first pass 
// this function has 3 goals, not to be confused:
// 1) find and mark the PRIMARY detections (regardless of the quality of the detection)
// 2) select the BEST detections per filter (regardless of PRIMARY)
// 3) apply the zero point and AB->Jy transformations

// I calculate McalAPER and McalPSF independently for stacks.  I use McalAPER for Mkron
// and Map, and McalPSF for Mpsf.  Note in StarOps.c:setMcalOutput I am setting
// measure->McalAPER to image->McalPSF for chips and warps, but not stacks

int setMrelAverageStack (Catalog *catalog, int cat, off_t ave, int Nsecfilt) {

  // we are guaranteed to have average, measure, secfilt
  Average *average = &catalog[0].average[ave];
  off_t measureOffset = average->measureOffset;
  int Nmeasure = average->Nmeasure;

  Measure *measure = &catalog[0].measure[measureOffset];
  SecFilt *secfilt = &catalog[0].secfilt[ave*Nsecfilt];

  float McalPSF = 0, McalAPER = 0;

  // set the primary projection cell and skycell for this coordinate
  int tessID    = -1;
  int projectID = -1;
  int skycellID = -1;
  get_tess_ids(&tessID, &projectID, &skycellID, average[0].R, average[0].D);

  average[0].tessID       = tessID;
  average[0].projectionID = projectID;
  average[0].skycellID    = skycellID;

  int NstackGood = 0;
  int NstackSuspect = 0;
  int haveStackObject = FALSE;

  for (int Ns = 0; Ns < Nphotcodes; Ns++) {

    int thisCode = photcodes[Ns][0].code;
    int Nsec = GetPhotcodeNsec(thisCode);

    /* calculate the average mag in this SEC photcode for a single star */

    int haveStack = FALSE;

    int isBad = FALSE;
    int isSuspect = FALSE;

    int NstackMeas = 0; // number of stack measurements for this photcode
    int NstackDet  = 0; // number of stack detections for this photcode (not forced)

    int   primaryLevelMax = 0;
    off_t primaryEntryMax = -1;
    float primaryValueMax = 0.0;

    int   bestLevelMax = 0;
    off_t bestEntryMax = -1;
    float bestValueMax = 0.0;

    int Nprimary = 0;

    // reset all stack-related values for this secfilt:
    dvo_secfilt_init (&secfilt[Nsec], SECFILT_RESET_STACK);

    for (off_t k = 0; k < Nmeasure; k++) {

      // only examine gpc1 stack data
      if (!isGPC1stack(measure[k].photcode)) continue;

      // skip measurements that do not match the current photcode
      PhotCode *code = GetPhotcodebyCode (measure[k].photcode);
      if (!code) continue;
      if (code->equiv != thisCode) { continue; }

      NstackMeas ++;
      if ((measure[k].photFlags2 & 0x00000004) == 0) NstackDet ++; // 0x04 = PM_SOURCE_MODE2_MATCHED

      // clear this bit for all measurements
      measure[k].dbFlags &= ~ID_MEAS_STACK_PRIMARY;
      measure[k].dbFlags &= ~ID_MEAS_STACK_PHOT_SRC;

      haveStack = TRUE;
      haveStackObject = TRUE;

      int isPrimary = FALSE;
      
      // ** is this a PRIMARY measurement? (there may be more than one)

      // if we request the primary (USE_TREE_FOR_PRIMARY), this is true if the measurement
      // is from the primary skycell for this position. (note that MatchImageSkycellID
      // requires the entry in the full Measure table, not just this object)
      if (MatchImageSkycellID (measureOffset + k, cat, tessID, projectID, skycellID)) {
	measure[k].dbFlags |= ID_MEAS_STACK_PRIMARY;
	isPrimary = TRUE;
	Nprimary ++;
      }

      // soften the error floor (can dM be 0.0?)
      // XXX EAM : this was int SNvalue -- isfinite(SNvalue) would always be finite
      float SNvalue = isfinite(measure[k].dM) ? 1.0 / hypot (measure[k].dM, MIN_ERROR) : NAN;
      int psfQFperfAboveLimit = isfinite(SNvalue) && isfinite(measure[k].psfQFperf) && (measure[k].psfQFperf > 0.95);

      // ** determine the BEST level
      int bestLevel = 0;
      if (!isPrimary && !psfQFperfAboveLimit) bestLevel = 1;
      if ( isPrimary && !psfQFperfAboveLimit) bestLevel = 2;
      if (!isPrimary &&  psfQFperfAboveLimit) bestLevel = 3;
      if ( isPrimary &&  psfQFperfAboveLimit) bestLevel = 4;

      // here is the rule for choosing the best value:
      float bestValue = (bestLevel > 2) ? SNvalue : measure[k].psfQFperf;

      // if we have not reached this level before, set the new level
      if (bestLevelMax < bestLevel) {
	bestValueMax = bestValue;
	bestLevelMax = bestLevel;
	bestEntryMax = k;
      }
      // if we have reached this level before, set the new value if we beat the old one
      if ((bestLevelMax == bestLevel) && (bestValueMax < bestValue)) {
	bestValueMax = bestValue;
	bestEntryMax = k;
      }
      myAssert (bestEntryMax > -1, "this should not happen");

      // ** determine the PRIMARY level
      int primaryLevel = 0;
      if ( isPrimary && !psfQFperfAboveLimit) primaryLevel = 1;
      if ( isPrimary &&  psfQFperfAboveLimit) primaryLevel = 2;

      // here is the rule for choosing the PRIMARY value:
      float primaryValue = 0.0;
      if (primaryLevel == 1) { primaryValue = measure[k].psfQFperf; }
      if (primaryLevel == 2) { primaryValue = SNvalue; }

      // if we have not reached this level before, set the new level
      if (isPrimary && (primaryLevelMax < primaryLevel)) {
	primaryValueMax = primaryValue;
	primaryLevelMax = primaryLevel;
	primaryEntryMax = k;
      }
      // if we have reached this level before, set the new value if we beat the old one
      if (isPrimary && (primaryLevelMax == primaryLevel) && (primaryValueMax < primaryValue)) {
	primaryValueMax = primaryValue;
	primaryEntryMax = k;
      }
    }

    // if we do not have a stack measurement for this photcode, skip everything below
    if (!haveStack) continue;
  
    // now we have a BEST and a PRIMARY entry (may be the same entry)

    off_t meas = bestEntryMax;
    off_t measSeq = meas + measureOffset;

    myAssert (meas >= 0, "this should not happen");

    // measurements without an image are either external reference photometry or
    // data for which the associated image has not been loaded (probably because of
    // overlaps).  we only want measurements associated with stack images in this loop
    
    // match measurement to its image (this is just a check, right?)
    if (getImageEntry (measSeq, cat) < 0) {
      // measurements without an image are either external reference photometry or
      // data for which the associated image has not been loaded (probably because of
      // overlaps).  Msys + measure.Mcal is our best guess of the true magnitude
      McalPSF  = measure[meas].McalPSF; // check that this is zero for loaded REF value
      McalAPER = McalPSF; // check that this is zero for loaded REF value
    } else {
      McalPSF   = getMcal  (measSeq, cat, MAG_CLASS_PSF);
      McalAPER  = USE_MCAL_PSF_FOR_STACK_APER ? getMcal  (measSeq, cat, MAG_CLASS_PSF) : getMcal  (measSeq, cat, MAG_CLASS_KRON);
    }
    
    // ** Here is the math to relate mag,zp to flux

    // flux_cgs : erg sec^1 cm^-2 Hz^-1
    // mag_inst : -2.5 log (cts/sec)
    // mag_inst : -2.5 log (flux_inst)
    // flux_inst = ten(-0.4*mag_inst)

    // mag_AB = -2.5 log (flux_cgs) - 48.6 (~by definition) [~Vega flux in V-band]
    // flux_cgs = ten(-0.4*(mag_AB + 48.6))

    // flux_AB : ten(-0.4*mag_AB)

    // flux_cgs = ten(-0.4*48.6) * flux_AB
    // flux_AB  = ten(+0.4*48.6) * flux_cgs
	    
    // flux_Jy : flux_cgs * 10^23

    // flux_AB = ten(+0.4*48.6) * ten(-23) * flux_Jy

    // mag_AB = mag_inst + ZP

    // flux_inst = ten(-0.4*(mag_AB - ZP)) = ten(0.4*ZP) * flux_AB

    // flux_AB = flux_inst * ten(-0.4*ZP)

    // flux_inst * ten(-0.4*ZP) = ten(+0.4*48.6 - 23) * flux_Jy

    // flux_inst = flux_Jy * ten(0.4*ZP + 0.4*48.6 - 23)
    // flux_inst = flux_Jy * ten(0.4*ZP - 3.56)
    // flux_Jy = flux_inst * ten(-0.4*ZP + 3.56)

    // get the zero point for the selected image
    // Use a different zero point for the PSF-like and APERTURE-like magnitudes
    float zpPSF  = PhotZeroPoint (&measure[meas], &average[0], &secfilt[0]) - McalPSF;
    float zpAPER = PhotZeroPoint (&measure[meas], &average[0], &secfilt[0]) - McalAPER;

    // zpFactor to go from instrumental flux to Janskies
    float zpFactorPSF  = pow(10.0, -0.4*zpPSF  + 3.56);
    float zpFactorAPER = pow(10.0, -0.4*zpAPER + 3.56);

    // need to put in AB mag factor to get to Janskies (or uJy?)
    secfilt[Nsec].FpsfStk   = zpFactorPSF  * measure[meas].FluxPSF;  
    secfilt[Nsec].dFpsfStk  = zpFactorPSF  * measure[meas].dFluxPSF; 
    secfilt[Nsec].FkronStk  = zpFactorAPER * measure[meas].FluxKron; 
    secfilt[Nsec].dFkronStk = zpFactorAPER * measure[meas].dFluxKron;
    secfilt[Nsec].FapStk    = zpFactorAPER * measure[meas].FluxAp;

    // NOTE: for PV3, apFluxErr is broken (see pmSourcePhotometry.c:245).  we are going to
    // use PSF flux error instead here:
    // secfilt[Nsec].dFapStk   = zpFactorAPER * measure[meas].dFluxAp;
    secfilt[Nsec].dFapStk   = zpFactorAPER * measure[meas].dFluxPSF;

    // Jy to AB mags
    secfilt[Nsec].MpsfStk   = (measure[meas].FluxPSF  > 0.0) ? 8.9 - 2.5*log10(secfilt[Nsec].FpsfStk)  : NAN;
    secfilt[Nsec].MkronStk  = (measure[meas].FluxKron > 0.0) ? 8.9 - 2.5*log10(secfilt[Nsec].FkronStk) : NAN;
    secfilt[Nsec].MapStk    = (measure[meas].FluxAp   > 0.0) ? 8.9 - 2.5*log10(secfilt[Nsec].FapStk)   : NAN;

    // record the measurement which gave the best value
    secfilt[Nsec].stackBestOff = (bestEntryMax == -1) ? -1 : bestEntryMax + measureOffset;
    myAssert (secfilt[Nsec].stackBestOff <= catalog[0].Nmeasure, "stackBestOff out of range");

    // record the selected primary measurement
    secfilt[Nsec].stackPrmryOff = (primaryEntryMax == -1) ? -1 : primaryEntryMax + measureOffset;
    myAssert (secfilt[Nsec].stackPrmryOff <= catalog[0].Nmeasure, "stackPrmryOff out of range");

    // this is the selected measurement used by secfilt[]
    measure[meas].dbFlags |= ID_MEAS_STACK_PHOT_SRC;
    if (Nprimary) {
      secfilt[Nsec].flags |= ID_SECF_STACK_PRIMARY;
      if (Nprimary > 1) {
	secfilt[Nsec].flags |= ID_SECF_STACK_PRIMARY_MULTIPLE;
      }
    }

    // stack measurement used for 'best' was a detection (not forced from the other bands)
    if ((measure[bestEntryMax].photFlags2 & 0x00000004) == 0) {
      secfilt[Nsec].flags |= ID_SECF_STACK_BESTDET;
    }

    // stack measurement used for 'primary' was a detection (not forced from the other bands)
    if ((primaryEntryMax >= 0) && ((measure[primaryEntryMax].photFlags2 & 0x00000004) == 0)) {
      secfilt[Nsec].flags |= ID_SECF_STACK_PRIMDET;
    }

    secfilt[Nsec].flags |= ID_SECF_HAS_PS1_STACK;

    // NOTE: negative and insignificant flux values are allowed, but not NAN flux values
    // float Finst = PhotFluxInst (&measure[meas], MAG_CLASS_PSF);
    
    // data quality assessment
    isBad |= (measure[meas].photFlags & photcodes[Ns][0].photomBadMask);
    isBad |= (measure[meas].psfQF < 0.85);
    isBad |= isnan(measure[meas].psfQF);
    isBad |= measure[meas].dM > 0.2; // S/N < 5.0

    isSuspect |= (measure[meas].photFlags & photcodes[Ns][0].photomPoorMask);
    isSuspect |= (measure[meas].psfQFperf < 0.85);

    if (!isSuspect && !isBad) {
      NstackGood ++;
    }
    if (isSuspect && !isBad) {
      NstackSuspect ++;
    }

    secfilt[Nsec].Nstack    = NstackMeas;
    secfilt[Nsec].NstackDet = NstackDet;

  } // Nsecfilt loop

  // in PSPS, there will be a stackObject row with the primary values.  if all filter primary values which
  // exist are also best values, then this row will be the best entry

  // the secfilt[Ns] entries are defined to be the Best values, so

  // conversely, if any secfilt[Ns] values are not PRIMARY, then the PRIMARY row is not the BEST row

  int PrimaryIsBest = TRUE;
  for (int Ns = 0; Ns < Nphotcodes; Ns++) {
    if (!(secfilt[Ns].flags & ID_SECF_HAS_PS1_STACK)) continue; // no stack detection in PS1, nothing is best
    if (secfilt[Ns].flags & ID_SECF_STACK_PRIMARY) continue;    // primary stack detection is best
    PrimaryIsBest = FALSE;
  }

  if (PrimaryIsBest) {
    average[0].flags |= ID_OBJ_BEST_STACK;
  }

  if (NstackGood >= 2) {
    average[0].flags |= ID_OBJ_GOOD_STACK;
  } else if (NstackGood + NstackSuspect >= 2) {
    average[0].flags |= ID_OBJ_SUSPECT_STACK;
  } else if (haveStackObject) {
    average[0].flags |= ID_OBJ_BAD_STACK;
  }

  return (TRUE);
}

// set mean of forced-warp measurements (selected by photcode range for now):
// somewhat simplified relative to chip-photometry:
// * no grid, no mosaic, no 2MASS, no SYNTH, no Ubercal, no flatcorr
// analysis is done on flux, not mags (as the faintest objects will be nearly insignificant)

// Although I calculate McalAPER for exposures, I am only using McalPSF for warps..
// Note in StarOps.c:setMcalOutput I am setting measure->McalAPER to image->McalPSF for
// chips and warps, but not stacks

int setMrelAverageForcedWarp (Catalog *catalog, int cat, off_t ave, int Nsecfilt, SetMrelInfo *results) {

  // we are guaranteed to have average, measure, secfilt
  Average *average    = &catalog[0].average[ave];
  off_t measureOffset = average->measureOffset;

  Measure *measure    = &catalog[0].measure[measureOffset];
  SecFilt *secfilt    = &catalog[0].secfilt[ave*Nsecfilt];
  char *measureRank   = &catalog[0].measureRank[measureOffset];

  off_t k, Nsec;
  float Mcal = 0;

  SetMrelInfoResetObject (results); // reset the per-object arrays

  // option for a test print
  if ((average[0].objID == TEST_OBJ_ID) && (average[0].catID == TEST_CAT_ID)) {
    fprintf (stderr, "test obj\n");
    print_measure_set_alt (average, secfilt, measure);
  }

  if (BOUNDARY_TREE) {
    int tessID, projID, skycellID;

    // for a diff db, we just want primary, if available or not.
    if (IS_DIFF_DB) {
      get_tess_ids(&tessID, &projID, &skycellID, average[0].R, average[0].D);
    }
    // save the tess ID for each secfilt
    for (Nsec = 0; Nsec < Nsecfilt; Nsec++) {
      // for warps, we want to use the skycell which provided the stack best entry
      if (!IS_DIFF_DB) {
	myAssert (secfilt[Nsec].stackBestOff <= catalog->Nmeasure, "stackBestOff out of range");
	if (!FindImageSkycellID (secfilt[Nsec].stackBestOff, cat, &tessID, &projID, &skycellID)) continue;
      }
      results->tessID[Nsec]    = tessID;
      results->projID[Nsec]    = projID;
      results->skycellID[Nsec] = skycellID;
    }
  }

  average->NwarpOK = 0; // count the number of warps with psfQF > 0.0

  // assign measurements to the photcode lists 
  for (k = 0; k < average->Nmeasure; k++) {
    off_t meas = measureOffset + k;

    measure[k].dbFlags &= ~ID_MEAS_WARP_USED; // this should not be set unless we use it in this pass

    // skip measurements that do not have a valid photcode (raise exception?)
    PhotCode *code = GetPhotcodebyCode (measure[k].photcode);
    myAssert (code, "invalid photcode??");

    // only examine gpc1 forced-warp data
    if (!isGPC1warp(measure[k].photcode)) continue;

    // assign the Nsec value so we can assign to the right lists
    int Nsec = GetPhotcodeNsec (code->equiv);
    if (Nsec < 0) continue; // skip measurements which do not have an equiv average photcode

    results->Nmeas[Nsec] ++;
    if (isfinite(measure[k].psfQFperf) && (measure[k].psfQFperf > 0.85)) results->NmeasGood[Nsec] ++;
    if (isfinite(measure[k].psfQF)     && (measure[k].psfQF     > 0.00)) average->NwarpOK ++;

    // restrict to measurements from the desired skycell
    if (BOUNDARY_TREE) {
      // use primary skycell for DIFF, stack skycell for WARP, otherwise skip this measurement
      if (!MatchImageSkycellID (meas, cat, results->tessID[Nsec], results->projID[Nsec], results->skycellID[Nsec])) {
	continue;
      }
    }

    // assign the max psfQF values for each Nsec value
    if (IS_DIFF_DB) {
      measure[k].dbFlags |= ID_MEAS_STACK_PRIMARY;
      MAX_NOT_NAN (results->psfQfMax[Nsec],     measure[k].psfQF);
      MAX_NOT_NAN (results->psfQfPerfMax[Nsec], measure[k].psfQFperf);
    }

    if (getImageEntry (meas, cat) < 0) {
      // measurements without an image are either external reference photometry or
      // data for which the associated image has not been loaded (probably because of
      // overlaps).  Msys + measure.Mcal is our best guess of the true magnitude
      Mcal = measure[k].McalPSF; // check that this is zero for loaded REF value
    } else {
      Mcal  = getMcal (meas, cat, MAG_CLASS_PSF);
      if (isnan(Mcal))  SKIP_THIS_MEAS(Ncal);
    }
    float Fcal = MagToFlux(-Mcal);

    // NOTE : I am using PhotCat not PhotSys for now since GPC1 chip-to-chip color terms
    // are small (and not measured)
    float Fpsf = PhotFluxCat (&measure[k], MAG_CLASS_PSF);
    float dFpsf = PhotFluxCatErr (&measure[k], MAG_CLASS_PSF);
    if (CHECK_VALID_FLUX(Fpsf, dFpsf)) {
      dFpsf = MAX (hypot(dFpsf, code->photomErrSys*Fpsf), MIN_ERROR*Fpsf); // bump up the error by a systematic floor
      int Npsf = results->psfData[Nsec].Nlist;
      results->psfData[Nsec].flxlist[Npsf] = Fpsf * Fcal;
      results->psfData[Nsec].errlist[Npsf] = dFpsf * Fcal;
      results->psfData[Nsec].wgtlist[Npsf] = 1.0;
      results->psfData[Nsec].ranking[Npsf] = measureRank ? measureRank[k] : 0;
      results->psfData[Nsec].measSeq[Npsf] = k;
      results->psfData[Nsec].msklist[Npsf] = 0;
      results->psfData[Nsec].Nlist ++;
    }     

    float Fap = PhotFluxCat (&measure[k], MAG_CLASS_APER);

    // NOTE: for PV3, apFluxErr is broken (see pmSourcePhotometry.c:245).  we are going to
    // use PSF flux error instead here:
    // float dFap = PhotFluxCatErr (&measure[k], MAG_CLASS_APER);
    float dFap = PhotFluxCatErr (&measure[k], MAG_CLASS_PSF);

    if (CHECK_VALID_FLUX(Fap, dFap)) {
      dFap = MAX (hypot(dFap, code->photomErrSys*Fap), MIN_ERROR*Fap); // bump up the error by a systematic floor
      int Naper = results->aperData[Nsec].Nlist;
      results->aperData[Nsec].flxlist[Naper] = Fap * Fcal;
      results->aperData[Nsec].errlist[Naper] = dFap * Fcal;
      results->aperData[Nsec].wgtlist[Naper] = 1.0;
      results->aperData[Nsec].ranking[Naper] = measureRank ? measureRank[k] : 0;
      results->aperData[Nsec].measSeq[Naper] = k;
      results->aperData[Nsec].msklist[Naper] = 0;
      results->aperData[Nsec].Nlist ++;
    }

    float Fkron = PhotFluxCat (&measure[k], MAG_CLASS_KRON);
    float dFkron = PhotFluxCatErr (&measure[k], MAG_CLASS_KRON);
    if (CHECK_VALID_FLUX(Fkron, dFkron)) {
      dFkron = MAX (hypot(dFkron, code->photomErrSys*Fkron), MIN_ERROR*Fkron); // bump up the error by a systematic floor
      int Nkron = results->kronData[Nsec].Nlist;
      results->kronData[Nsec].flxlist[Nkron] = Fkron * Fcal;
      results->kronData[Nsec].errlist[Nkron] = dFkron * Fcal;
      results->kronData[Nsec].wgtlist[Nkron] = 1.0;
      results->kronData[Nsec].ranking[Nkron] = measureRank ? measureRank[k] : 0;
      results->kronData[Nsec].measSeq[Nkron] = k;
      results->kronData[Nsec].msklist[Nkron] = 0;
      results->kronData[Nsec].Nlist ++;
    }
  }

  // find max values across filters
  float psfQfMax     = 0.0;
  float psfQfPerfMax = 0.0;

  // now calculate the mean stats for the Nsec bands.
  for (Nsec = 0; Nsec < Nsecfilt; Nsec++) {
    dvo_secfilt_init (&secfilt[Nsec], SECFILT_RESET_WARP);

    if (IS_DIFF_DB) {
      // for non DIFF_DB, these are set in setMrelAverageExposure
      secfilt[Nsec].psfQfMax     = results->psfQfMax[Nsec];
      secfilt[Nsec].psfQfPerfMax = results->psfQfPerfMax[Nsec];

      MAX_NOT_NAN (psfQfMax,     secfilt[Nsec].psfQfMax);
      MAX_NOT_NAN (psfQfPerfMax, secfilt[Nsec].psfQfPerfMax);
    }

    int Nranking, minRank;

    // if too few valid measurements meet the minimum criteria, go to the next entry
    StatType *psfstats = &results->psfstats;
    // Nranking = MAG_STATS_BY_RANKING (&results->psfData[Nsec], psfstats);
    Nranking = useOLS ? magStatsByRanking (&results->psfData[Nsec], psfstats) : magStatsByRankingIRLS (&results->psfData[Nsec], psfstats);
    if (Nranking) {
      secfilt[Nsec].FpsfWrp  = psfstats->mean;
      secfilt[Nsec].dFpsfWrp = psfstats->error;
      secfilt[Nsec].sFpsfWrp = (psfstats->Nmeas > 1) ? psfstats->chisq : NAN;
      secfilt[Nsec].NusedWrp = psfstats->Nmeas;
      secfilt[Nsec].MpsfWrp  = isnan(secfilt[Nsec].FpsfWrp) ? NAN : 8.9 - 2.5*log10(secfilt[Nsec].FpsfWrp); // 8.9 since flux is in Jy
    }
    minRank = (Nranking > 0) ? results->psfData[Nsec].ranking[0] : 10;
    markMeasureByRanking (&results->psfData[Nsec], measure, minRank, ID_MEAS_WARP_USED | ID_MEAS_PHOTOM_PSF, ID_MEAS_MASKED_PSF);

    // if too few valid measurements meet the minimum criteria, go to the next entry
    StatType *apstats = &results->apstats;
    // Nranking = MAG_STATS_BY_RANKING (&results->aperData[Nsec], apstats);
    Nranking = useOLS ? magStatsByRanking (&results->aperData[Nsec], apstats) : magStatsByRankingIRLS (&results->aperData[Nsec], apstats);
    if (Nranking) {
      secfilt[Nsec].FapWrp     = apstats->mean;
      secfilt[Nsec].dFapWrp    = apstats->error;
      secfilt[Nsec].sFapWrp    = (apstats->Nmeas > 1) ? apstats->chisq : NAN;
      secfilt[Nsec].NusedApWrp = apstats->Nmeas;
      secfilt[Nsec].MapWrp     = isnan(secfilt[Nsec].FapWrp) ? NAN : 8.9 - 2.5*log10(secfilt[Nsec].FapWrp); // 8.9 since flux is in Jy
    }
    minRank = (Nranking > 0) ? results->aperData[Nsec].ranking[0] : 10;
    markMeasureByRanking (&results->aperData[Nsec], measure, minRank, ID_MEAS_WARP_USED | ID_MEAS_PHOTOM_APER, ID_MEAS_MASKED_APER);

    // if too few valid measurements meet the minimum criteria, go to the next entry
    StatType *kronstats = &results->kronstats;
    // Nranking = MAG_STATS_BY_RANKING (&results->kronData[Nsec], kronstats);
    Nranking = useOLS ? magStatsByRanking (&results->kronData[Nsec], kronstats) : magStatsByRankingIRLS (&results->kronData[Nsec], kronstats);
    if (Nranking) {
      secfilt[Nsec].FkronWrp     = kronstats->mean;
      secfilt[Nsec].dFkronWrp    = kronstats->error;
      secfilt[Nsec].sFkronWrp    = (kronstats->Nmeas > 1) ? kronstats->chisq : NAN;
      secfilt[Nsec].NusedKronWrp = kronstats->Nmeas;
      secfilt[Nsec].MkronWrp     = isnan(secfilt[Nsec].FkronWrp) ? NAN : 8.9 - 2.5*log10(secfilt[Nsec].FkronWrp); // 8.9 since flux is in Jy
    }
    minRank = (Nranking > 0) ? results->kronData[Nsec].ranking[0] : 10;
    markMeasureByRanking (&results->kronData[Nsec], measure, minRank, ID_MEAS_WARP_USED | ID_MEAS_PHOTOM_KRON, ID_MEAS_MASKED_KRON);

    secfilt[Nsec].Nwarp     = results->Nmeas[Nsec];
    secfilt[Nsec].NwarpGood = results->NmeasGood[Nsec];
  }
  return (TRUE);
}

int magStatsByRanking (StatDataSet *dataset, StatType *stats) {

  liststats_init (stats);

  if (dataset->Nlist == 0) return 0;

  // we have a list of measurements for this Nsec value, along with errors, weights, and their rank
  // first, sort by the rank (increasing)
  sort_StatDataSet (dataset);

  // find the values with the same minimum rank:
  int Nranking = 0;
  int minRank = dataset->ranking[0]; // MIN (5, result->psfData[Nsec].ranking[0]);  -- only allow rank 5 or <

  int i;
  for (i = 0; (i < dataset->Nlist) && (dataset->ranking[i] == minRank); i++, Nranking++);

  liststats (dataset->flxlist, dataset->errlist, dataset->wgtlist, Nranking, stats);
  return (Nranking);
}

int magStatsByRankingIRLS (StatDataSet *dataset, StatType *stats) {

  liststats_init (stats);

  if (dataset->Nlist == 0) return 0;

  // we have a list of measurements for this Nsec value, along with errors, weights, and their rank
  // first, sort by the rank (increasing)
  sort_StatDataSet (dataset);

  // find the values with the same minimum rank:
  int Nranking = 0;
  int minRank = dataset->ranking[0]; // MIN (5, result->psfData[Nsec].ranking[0]);  -- only allow rank 5 or <

  int i;
  for (i = 0; (i < dataset->Nlist) && (dataset->ranking[i] == minRank); i++, Nranking++);

  liststats_irls (dataset, Nranking, stats);
  return (Nranking);
}

// outlier warp measurements are driving bad mean values.  
int magStatsByRankingClipped (StatDataSet *dataset, StatType *stats) {

  liststats_init (stats);

  if (dataset->Nlist == 0) return 0;

  // we have a list of measurements for this Nsec value, along with errors, weights, and their rank
  // first, sort by the rank (increasing)
  sort_StatDataSet (dataset);

  // find the values with the same minimum rank:
  int Nranking = 0;
  int minRank = dataset->ranking[0]; // MIN (5, result->psfData[Nsec].ranking[0]);  -- only allow rank 5 or <

  int i;
  for (i = 0; (i < dataset->Nlist) && (dataset->ranking[i] == minRank); i++, Nranking++);

  // we want to measure the weighted average, but first we want to clip points which are > 3 sigma from the median
  // since we are only allowed to clip at most 25% of the points, we need to have at least 4 points to do this clipping
  if (Nranking > 3) {

    double *fluxes = NULL;
    double *offset = NULL;
    int    *entry  = NULL;

    // now find the distances
    ALLOCATE (entry,  int,    Nranking);
    ALLOCATE (offset, double, Nranking);
    ALLOCATE (fluxes, double, Nranking);

    // first, measure the median of the flxlist:
    for (i = 0; i < Nranking; i++) {
      fluxes[i] = dataset->flxlist[i];
    }
    dsort (fluxes, Nranking);
    int Nmidpoint = (int)(0.5*Nranking); // if Nranking is odd (e.g., 5), Nmidpoint is central bin (e.g., 2); if Nranking is even (e.g., 6), Nmidpoint is central bin + 1 (e.g., 3);
    double medianFlux = (Nranking % 2) ? fluxes[Nmidpoint] : 0.5*(fluxes[Nmidpoint] + fluxes[Nmidpoint - 1]);

    for (i = 0; i < Nranking; i++) {
      offset[i] = fabs((dataset->flxlist[i] - medianFlux) / dataset->errlist[i]);
      entry[i] = i;
    }

    sort_entry_by_offset (offset, entry, Nranking);

    // we are only going to clip at most 25% of the points.  Start at the 75% point and check for outliers
    int Nreject = 0;
    for (i = 0.75*Nranking; i < Nranking; i++) {
      if (offset[i] > 4.0) {
	// this is an outlier -- add 10 to the ranking
	dataset->ranking[entry[i]] += 10;
	Nreject ++;
      }
    }
    Nranking -= Nreject; // the excluded points are now removed from the count

    // re-sort by the rank so liststats below only operates on the unrejected points
    sort_StatDataSet (dataset);

    free (fluxes);
    free (offset);
    free (entry);
  }

  // get stats on only the first Nranking entries
  liststats (dataset->flxlist, dataset->errlist, dataset->wgtlist, Nranking, stats);

  return (Nranking);
}

int markMeasureByRanking (StatDataSet *dataset, Measure *measure, int minrank, DVOMeasureFlags keepflag, DVOMeasureFlags maskflag) {
  int i;

  for (i = 0; i < dataset->Nlist; i++) {
    int k = dataset->measSeq[i];
    if (dataset->ranking[i] > minrank) {
      measure[k].dbFlags &= ~keepflag;
    } else {
      measure[k].dbFlags |= keepflag;
    }
    if (dataset->msklist[i]) measure[k].dbFlags |= maskflag;
  }
  return TRUE;
}

/* sort a coordinate pair (X,Y) and the associated index (S) */
void sort_StatDataSet (StatDataSet *dataset) {
  
# define SWAPFUNC(A,B){ double dtmp; int itmp; \
    dtmp = dataset->flxlist[A]; dataset->flxlist[A] = dataset->flxlist[B]; dataset->flxlist[B] = dtmp;     \
    dtmp = dataset->errlist[A]; dataset->errlist[A] = dataset->errlist[B]; dataset->errlist[B] = dtmp;     \
    dtmp = dataset->wgtlist[A]; dataset->wgtlist[A] = dataset->wgtlist[B]; dataset->wgtlist[B] = dtmp;     \
    itmp = dataset->ranking[A]; dataset->ranking[A] = dataset->ranking[B]; dataset->ranking[B] = itmp;     \
    itmp = dataset->measSeq[A]; dataset->measSeq[A] = dataset->measSeq[B]; dataset->measSeq[B] = itmp;     \
  }
# define COMPARE(A,B)(dataset->ranking[A] < dataset->ranking[B])
  
  OHANA_SORT (dataset->Nlist, COMPARE, SWAPFUNC);
  
# undef SWAPFUNC
# undef COMPARE

}

void sort_entry_by_offset (double *offset, int *entry, int N) {

# define SWAPFUNC(A,B){ double dtmp; int itmp; \
    dtmp = offset[A]; offset[A] = offset[B]; offset[B] = dtmp;     \
    itmp = entry[A];  entry[A]  = entry[B];  entry[B]  = itmp;     \
  }
# define COMPARE(A,B)(offset[A] < offset[B])
  
  OHANA_SORT (N, COMPARE, SWAPFUNC);
  
# undef SWAPFUNC
# undef COMPARE
}

void GetPhotFlagStats (uint32_t *photFlagUpper, uint32_t *photFlagLower, uint32_t *photflag_list, int Nphotflag) {

  int i, bit;

  uint32_t outputUpper = 0;
  uint32_t outputLower = 0;

  uint32_t flag = 0x1; // start with the flag on the 0 bit
  for (bit = 0; bit < 32; bit++) {

    int Nraised = 0;
    for (i = 0; i < Nphotflag; i++) {
      if (photflag_list[i] & flag) Nraised ++;
    }

    float fraction = Nraised / (float) Nphotflag;

    if (fraction < 0.25) {
      // do nothing (no bits raised)
    }
    if ((fraction >= 0.25) && (fraction < 0.50)) {
      outputLower |= flag;
    }
    if ((fraction >= 0.50) && (fraction < 0.75)) {
      outputUpper |= flag;
    }
    if (fraction >= 0.75) {
      outputLower |= flag;
      outputUpper |= flag;
    }
    flag = (flag << 1);
  }
  *photFlagUpper = outputUpper;
  *photFlagLower = outputLower;
}
