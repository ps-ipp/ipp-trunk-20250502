# include "relphot.h"

/* 
   We define a 'grid correction', essentially the flat-field correction, as a correction per 
   photcode subdivided into an NxN array.  The dimensions of the chips corresponding to a
   photcode will need to be added to the photcode table.  This means a schema update,
   which I detest.  For now (2021.05.16), I will hard-wire the GPC1 / GPC2 chip size and
   worry about HSC & Megacam in the future.

   We have a collection of photcodes, with photcodeID limited by design to 64k (unsigned short).
   
   Thus we can generate an array of pointers to the grid correction structures and access them 
   by photcode.


 */

static GridCorrectionType **GridCorr = NULL;
static int                 NGridCorr = 0;

# if (0)
/* PS1 / PS2 values */
# define NX_CHIP 4900
# define NY_CHIP 4900
# define NX_BIN 16
# define NY_BIN 16
# endif

/* test values */
# define NX_CHIP_DEFAULT 1000
# define NY_CHIP_DEFAULT 1000
# define NX_BIN_DEFAULT 4
# define NY_BIN_DEFAULT 4

void initGridBins (void) {

  // allocate the full possible range of GridCorrectionType pointers, Nphotcode
  // loop over all images to find actual existing photcodes
  // generate initial grid values for each existing photcode

  if (!GRID_ZEROPT) return; // skip if we are ignoring the grid correction

  if (GridCorr) return;

  PhotCodeData *photcodes = GetPhotcodeTable();
  if (!photcodes) return;

  // we have photcodes->Ncodes actually loaded.  loop over them and allocate
  // array only as large as the max photcodes->code[i].code

  int maxCode = 0;
  for (int i = 0; i < photcodes->Ncode; i++) {
    maxCode = MAX(maxCode, photcodes->code[i].code);
    myAssert (maxCode < 0x10000, "oops");
  }

  NGridCorr = maxCode + 1;
  ALLOCATE (GridCorr, GridCorrectionType *, NGridCorr);
  for (int i = 0; i < NGridCorr; i++) {
    GridCorr[i] = NULL;
  }

  fprintf (stderr, "Generating grid corrections for %d photcodes\n", NGridCorr);
  return;
}

void freeGridBins() {

  if (!GridCorr) return;

  for (int code = 0; code < NGridCorr; code++) {
    if (!GridCorr[code]) continue;

    for (int ix = 0; ix < GridCorr[code]->Nx; ix++) {
      FREE (GridCorr[code]-> Mgrid[ix]);
      if (GridCorr[code]->dMgrid) FREE (GridCorr[code]->dMgrid[ix]);
      if (GridCorr[code]->nMgrid) FREE (GridCorr[code]->nMgrid[ix]);
      if (GridCorr[code]->nAlloc) FREE (GridCorr[code]->nAlloc[ix]);

      // dMval is a 2D array of vectors
      for (int iy = 0; iy < GridCorr[code]->Ny; iy++) {
	FREE (GridCorr[code]->dMval[ix][iy]);
      }
      FREE (GridCorr[code]->dMval[ix]);
    }      

    FREE (GridCorr[code]-> Mgrid);
    FREE (GridCorr[code]->dMgrid);
    FREE (GridCorr[code]->nMgrid);
    FREE (GridCorr[code]->nAlloc);
    FREE (GridCorr[code]-> dMval);

    FREE(GridCorr[code]);
  }

  FREE (GridCorr);
  GridCorr = NULL;

  return;
}

GridCorrectionType *getGridCorrNext (int *Nlast) {

  if (GridCorr == NULL) return NULL;

  if (*Nlast >= NGridCorr) return NULL;

  if (*Nlast < 0) *Nlast = -1;

  GridCorrectionType *result = NULL;
  for (int i = *Nlast + 1; i < NGridCorr; i++) {
    if (GridCorr[i] == NULL) continue;
    result = GridCorr[i];
    *Nlast = i;
    break;
  }
  return result;
}

// generate a new GridCorr structure in the array
GridCorrectionType *newGridCorrByCode (int code) {

  if (GridCorr == NULL) return NULL;

  if (code >= NGridCorr) return NULL;
  if (code <          0) return NULL;

  myAssert (GridCorr[code] == NULL, "oops, should not already be allocated!");
  ALLOCATE(GridCorr[code], GridCorrectionType, 1)
  GridCorrectionType *result = GridCorr[code];
  return result;
}

// return the GridCode structure corresponding to 'code'
GridCorrectionType *getGridCorrByCode (int code) {

  if (GridCorr == NULL) return NULL;

  if (code >= NGridCorr) return NULL;
  if (code <          0) return NULL;

  GridCorrectionType *result = GridCorr[code];
  return result;
}

/* for GPC1, we have 60 chips, 5 filters, 16x16 grid cells = 2MB of memory for this stuff
   if we go to 64x64 grid cells (~75 pixels), then it is still only 29MB */
void initGrid (void) {

  if (!GRID_ZEROPT) return;

  off_t Nimages = 0;
  Image *images = getimages (&Nimages, NULL);

  int NGridReal = 0;

  for (int i = 0; i < Nimages; i++) {
    int code = images[i].photcode;
    myAssert (code >= 0, "oops");
    myAssert (code < NGridCorr, "oops");
    
    // valid photcodes values (code) are in range 1 <= code < 0x10000
    // photcode == 0 are e.g., PHU (mosaic) images, and should be ignored here
    if (!code) continue;

    int NX_CHIP = NX_CHIP_DEFAULT;
    int NY_CHIP = NY_CHIP_DEFAULT;
    int NX_BIN  = NX_BIN_DEFAULT;
    int NY_BIN  = NY_BIN_DEFAULT;
    if (isGPC1chip(code)) { NX_CHIP = 4900; NY_CHIP = 4900; NX_BIN = NY_BIN = GRID_BIN_GPC1; }
    if (isGPC2chip(code)) { NX_CHIP = 4900; NY_CHIP = 4900; NX_BIN = NY_BIN = GRID_BIN_GPC2; }
    if (isHSCchip(code))  { NX_CHIP = 2100; NY_CHIP = 4200; NX_BIN = NY_BIN = GRID_BIN_HSC;  }
    if (isCFHchip(code))  { NX_CHIP = 2100; NY_CHIP = 4200; NX_BIN = NY_BIN = GRID_BIN_CFH;  }

    if (GridCorr[code]) continue; // already created this one

    NGridReal ++;

    ALLOCATE(GridCorr[code], GridCorrectionType, 1); 
    GridCorr[code]->photcode = code;
    GridCorr[code]->Nx = NX_BIN;
    GridCorr[code]->Ny = NY_BIN;
    GridCorr[code]->dX = NX_BIN / (float) NX_CHIP;
    GridCorr[code]->dY = NY_BIN / (float) NY_CHIP;

    // we are normally accessing this array randomly, so there is no advantage to
    // doing Mgrid[y][x] vs Mgrid[x][y]
    ALLOCATE (GridCorr[code]-> dMval, double **, NX_BIN);
    ALLOCATE (GridCorr[code]-> Mgrid, float *, NX_BIN);
    ALLOCATE (GridCorr[code]->dMgrid, float *, NX_BIN);
    ALLOCATE (GridCorr[code]->nMgrid,   int *, NX_BIN);
    ALLOCATE (GridCorr[code]->nAlloc,   int *, NX_BIN);
    for (int ix = 0; ix < NX_BIN; ix++) {
      ALLOCATE (GridCorr[code]-> Mgrid[ix], float, NY_BIN);
      ALLOCATE (GridCorr[code]->dMgrid[ix], float, NY_BIN);
      ALLOCATE (GridCorr[code]->nMgrid[ix],   int, NY_BIN);
      ALLOCATE (GridCorr[code]->nAlloc[ix],   int, NY_BIN);

      // when we first allocate this array, set the elements to 0 (NULL)
      // these arrays will be allocated or reallocated in resetMgrid
      ALLOCATE_ZERO (GridCorr[code]-> dMval[ix], double *, NX_BIN);
    }      
  }
  resetMgrid(); // start with values of 0
  fprintf (stderr, "Init grid corrections for %d photcodes\n", NGridReal);
}

// reset the values in the arrays to 0
void resetMgrid () {

  if (!GRID_ZEROPT) return;
  if (!GridCorr) return;

  for (int code = 0; code < NGridCorr; code++) {
    if (!GridCorr[code]) continue;

    for (int ix = 0; ix < GridCorr[code]->Nx; ix++) {
      for (int iy = 0; iy < GridCorr[code]->Ny; iy++) {
	GridCorr[code]-> Mgrid[ix][iy] = 0.0;
	GridCorr[code]->dMgrid[ix][iy] = 0.0;
	GridCorr[code]->nMgrid[ix][iy] =   0;
	GridCorr[code]->nAlloc[ix][iy] = 100;

	// allocate or reset vector length to default
	if (GridCorr[code]->dMval[ix][iy]) {
	  REALLOCATE (GridCorr[code]->dMval[ix][iy], double, GridCorr[code]->nAlloc[ix][iy]);
	} else {
	  ALLOCATE (GridCorr[code]->dMval[ix][iy], double, GridCorr[code]->nAlloc[ix][iy]);
	}
      }
    }      
  }
}

void setMgrid (Catalog *catalog, int Ncatalog) {

  // check if we are actually doing this step
  if (!GRID_ZEROPT) return;
  if (GRID_ZPT_MODE == GRID_ZPT_MODE_NONE) return;

  resetMgrid(); // start with values of 0

  // loop over all measurements, accumulate Sum (in Mgrid), Sum2 (in dMgrid), and Npts

  int Nsecfilt = GetPhotcodeNsecfilt ();

  for (int nc = 0; nc < Ncatalog; nc++) {
    for (int na = 0; na < catalog[nc].Naverage; na++) {

      int nm = catalog[nc].averageT[na].measureOffset;
      for (int k = 0; k < catalog[nc].averageT[na].Nmeasure; k++, nm++) {

	// skip measurements marked by AREA or TIME
	if (catalog[nc].measureT[nm].dbFlags & MEAS_BAD) continue;

	float Mcal = getMcal  (nm, nc, MAG_CLASS_PSF);
	if (isnan(Mcal)) continue;

	float Mgrp = getMgrp  (nm, nc, catalog[nc].measureT[nm].airmass, NULL);
	if (isnan(Mgrp)) continue;

	float Mmos  = getMmos  (nm, nc);
	if (isnan(Mmos)) continue;

	float Mflat = getMflat (nm, nc, catalog);
	if (isnan(Mflat)) continue;

	// Mrel* is the average magnitude for this star.  For PS1 stacks, we have too much
	// PSF variability.  We need to calibrate the PSF magnitudes separately from the
	// Aperture-like magnitues.  (We have an option to use the kron magnitudes or the
	// other apertures here).  I basically need to do this analysis separately for each
	// magnitude type
    
	float MrelPSF = getMrel  (catalog, nm, nc, MAG_CLASS_PSF, MAG_SRC_CHP);
	if (isnan(MrelPSF)) continue;
      
	float MsysPSF = PhotSysTiny (&catalog[nc].measureT[nm], &catalog[nc].averageT[na], &catalog[nc].secfilt[na*Nsecfilt], MAG_CLASS_PSF);
	if (isnan(MsysPSF)) continue;

	float Moff =  Mcal + Mgrp + Mmos + Mflat;

	// Msys = Mrel + Moff + Mgrid
	// thus Mgrid = Msys - Mrel - Moff

	int code = catalog[nc].measureT[nm].photcode;
	if (code <= 0) continue;
	if (code >= NGridCorr) continue; // does not match one of our image, skip
	
	GridCorrectionType *grid = GridCorr[code];
	if (!grid) continue; // does not match one of our images, skip

	// edge effects could cause some positions to be slightly out of range
	// probably should trap extreme outliers
	int ix = MIN(MAX(0, (int)(catalog[nc].measureT[nm].Xccd * grid->dX)), grid->Nx - 1);
	int iy = MIN(MAX(0, (int)(catalog[nc].measureT[nm].Yccd * grid->dY)), grid->Ny - 1);

	float dM = MsysPSF - MrelPSF - Moff;
	
	// XXX by the time we get here, we should have already mostly fixed up the zero
	// points.  reject measurements which are way off
	if (fabs(dM) > 0.5) continue;

	int ival = grid->nMgrid[ix][iy];
	grid-> dMval[ix][iy][ival] = dM;
	grid->nMgrid[ix][iy] ++;
	CHECK_REALLOCATE (grid-> dMval[ix][iy], double, grid->nAlloc[ix][iy], grid->nMgrid[ix][iy], 100);

	/* XXX old code to calculate simple means:
	   grid-> Mgrid[ix][iy] += dM;
	   grid->dMgrid[ix][iy] += dM*dM;
	   grid->nMgrid[ix][iy] ++;
	*/
      }
    }
  }

  StatType stats;
  liststats_setmode (&stats, "MEDIAN");

  // now calculate Mgrid, dMgrid, nMgrid from Sum, Sum, Npt
  for (int code = 0; code < NGridCorr; code++) {
    if (!GridCorr[code]) continue;

    for (int ix = 0; ix < GridCorr[code]->Nx; ix++) {
      for (int iy = 0; iy < GridCorr[code]->Ny; iy++) {

	// cells without sufficient coverage stay at 0.0
	if (GridCorr[code]->nMgrid[ix][iy] < 5) {
	  GridCorr[code]-> Mgrid[ix][iy] = 0.0;
	  GridCorr[code]->dMgrid[ix][iy] = 0.0;
	  continue;
	}

	liststats_init (&stats);

	liststats (GridCorr[code]->dMval[ix][iy], NULL, NULL, GridCorr[code]->nMgrid[ix][iy], &stats);
	double altSigma = (stats.Upper80 - stats.Lower20) / 1.6;  // 20% to 80% encompasses 60% of the values, corresponds to the range (-0.85 sigma : +0.85 sigma)

	GridCorr[code]-> Mgrid[ix][iy] = stats.median;
	GridCorr[code]->dMgrid[ix][iy] = altSigma; // sample stdev
      }
    }
  }
  return;
}

# if (0)

  // now calculate Mgrid, dMgrid, nMgrid from Sum, Sum, Npt
  for (int code = 0; code < NGridCorr; code++) {
    if (!GridCorr[code]) continue;

    // float GridSum = 0.0;
    // int   GridCnt =   0;
    for (int ix = 0; ix < GridCorr[code]->Nx; ix++) {
      for (int iy = 0; iy < GridCorr[code]->Ny; iy++) {

	// cells without sufficient coverage stay at 0.0
	if (GridCorr[code]->nMgrid[ix][iy] < 5) {
	  GridCorr[code]-> Mgrid[ix][iy] = 0.0;
	  GridCorr[code]->dMgrid[ix][iy] = 0.0;
	  continue;
	}

	float Mgrid  = GridCorr[code]-> Mgrid[ix][iy] / GridCorr[code]->nMgrid[ix][iy]; // average Mgrid
	float Mgrid2 = GridCorr[code]-> dMgrid[ix][iy] / GridCorr[code]->nMgrid[ix][iy]; // average Mgrid^2

	float r = GridCorr[code]->nMgrid[ix][iy] / (float) (GridCorr[code]->nMgrid[ix][iy] - 1.0); // pop -> sample stdev

	GridCorr[code]-> Mgrid[ix][iy] = Mgrid;
	GridCorr[code]->dMgrid[ix][iy] = sqrt(r*(Mgrid2 - Mgrid*Mgrid)); // sample stdev
	// fprintf (stderr, "grid code %d, %d x %d : %f +/- %f : %d\n", code, ix, iy, GridCorr[code]-> Mgrid[ix][iy], GridCorr[code]->dMgrid[ix][iy], GridCorr[code]->nMgrid[ix][iy]);
	// GridSum += Mgrid;
	// GridCnt ++;
      }
    }
    // float GridAve = GridSum / GridCnt;
    // fprintf (stderr, "grid average: %f\n", GridAve);
# endif

float getMgrid (Measure *measure) {

  if (!GRID_ZEROPT) return 0.0;
  if (GRID_ZPT_MODE == GRID_ZPT_MODE_NONE) return 0.0;

  int code = measure->photcode;
  if (code <= 0) return 0.0;
  if (code >= NGridCorr) return 0.0; // does not match one of our image, skip
	
  GridCorrectionType *grid = GridCorr[code];
  if (!grid) return 0.0; // does not match one of our images, skip
  
  // edge effects could cause some positions to be slightly out of range
  // probably should trap extreme outliers
  int ix = MIN(MAX(0, (int)(measure->Xccd * grid->dX)), grid->Nx - 1);
  int iy = MIN(MAX(0, (int)(measure->Yccd * grid->dY)), grid->Ny - 1);

  float Mgrid = grid-> Mgrid[ix][iy];
  return Mgrid;
}

float getMgridTiny (MeasureTiny *measure) {

  if (!GRID_ZEROPT) return 0.0;
  if (GRID_ZPT_MODE == GRID_ZPT_MODE_NONE) return 0.0;

  int code = measure->photcode;
  if (code <= 0) return 0.0;
  if (code >= NGridCorr) return 0.0; // does not match one of our image, skip
	
  GridCorrectionType *grid = GridCorr[code];
  if (!grid) return 0.0; // does not match one of our images, skip
  
  // edge effects could cause some positions to be slightly out of range
  // probably should trap extreme outliers
  int ix = MIN(MAX(0, (int)(measure->Xccd * grid->dX)), grid->Nx - 1);
  int iy = MIN(MAX(0, (int)(measure->Yccd * grid->dY)), grid->Ny - 1);

  float Mgrid = grid-> Mgrid[ix][iy];
  return Mgrid;
}

// for historical reasons, Mflat and Mgrid have opposite signs
void setMflatFromGrid (Catalog *catalog) {
  if (!GRID_ZEROPT) return;
  for (off_t j = 0; j < catalog->Nmeasure; j++) {
    float Mgrid = getMgrid (&catalog->measure[j]);
    catalog->measure[j].Mflat += Mgrid;
  }
}

