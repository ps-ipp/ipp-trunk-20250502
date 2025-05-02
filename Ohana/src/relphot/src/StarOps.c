# include "relphot.h"

# define STAR_BAD (ID_SECF_STAR_FEW | ID_SECF_STAR_POOR)

enum {THREAD_RUN, THREAD_DONE};

typedef struct {
  int entry;
  int state;
  Catalog *catalog;
  int Ncatalog;
  SetMrelInfo summary;
} ThreadInfo;

// ** internal functions:
void *setMrel_worker (void *data);
int setMrel_threaded (Catalog *catalog, int Ncatalog);
int print_measure_set (Average *average, SecFilt *secfilt, Measure *measure);

// we want to allocate the StatDataSet arrays only once (or once per thread).  this
// function finds the largest value of Nmeasure so we can allocate that max size when
// needed
static int Nmax; 
void initMrel (Catalog *catalog, int Ncatalog) {

  off_t i, j;
  
  Nmax = 0;
  for (i = 0; i < Ncatalog; i++) {
    for (j = 0; j < catalog[i].Naverage; j++) {
      Nmax = MAX (Nmax, catalog[i].averageT[j].Nmeasure);
    }
  }
}  

// 
float getMrel (Catalog *catalog, off_t meas, int cat, dvoMagClassType class, dvoMagSourceType source) {

  int ave = catalog[cat].measureT[meas].averef;
  int photcode = catalog[cat].measureT[meas].photcode;

  int ecode = GetPhotcodeEquivCodebyCode (photcode);
  int Nsec = GetPhotcodeNsec(ecode);
  int Nsecfilt = GetPhotcodeNsecfilt ();

  int entry = Nsecfilt*ave+Nsec;

  SecFilt *secfilt = &catalog[cat].secfilt[entry];

  // is this star OK?
  if (secfilt->flags & STAR_BAD) return (NAN);  

  switch (class) {
    case MAG_CLASS_PSF:
      switch (source) {
	case MAG_SRC_CHP:
	  return secfilt->MpsfChp;
	case MAG_SRC_WRP:
	  return secfilt->MpsfWrp;
	case MAG_SRC_STK:
	  return secfilt->MpsfStk;
	default:
	  return NAN;
      }
      break;
    case MAG_CLASS_KRON:
      switch (source) {
	case MAG_SRC_CHP:
	  return secfilt->MkronChp;
	case MAG_SRC_WRP:
	  return secfilt->MkronWrp;
	case MAG_SRC_STK:
	  return secfilt->MkronStk;
	default:
	  return NAN;
      }
      break;
    case MAG_CLASS_APER:
      switch (source) {
	case MAG_SRC_CHP:
	  return secfilt->MapChp;
	case MAG_SRC_WRP:
	  return secfilt->MapWrp;
	case MAG_SRC_STK:
	  return secfilt->MapStk;
	default:
	  return NAN;
      }
      break;
    case MAG_CLASS_NONE:
    case MAG_CLASS_DEV: // DeVaucouleur Model (only for galphot)
    case MAG_CLASS_EXP: // Exponential Model (only for galphot)
    case MAG_CLASS_SER: // Sersic Model (only for galphot)
      return NAN;
      break;
  }
  return NAN; // should not be able to reach here
}

void SetMrelInfoReset (SetMrelInfo *results) {
  results->Nfew  = 0;
  results->Ncode  = 0;
  results->Nsys  = 0;
  results->Nbad  = 0;
  results->Ncal  = 0;
  results->Nmos  = 0;
  results->Ngrp  = 0;
  results->Ngrid = 0;
}

static int NsynthNames = 6;
static float synthMaxMags[] = {13.5, 13.5, 13.5, 13.0, 12.0, 13.5};
static char *synthNames[]   = { "g",  "r",  "i",  "z",  "y", "w"};

void SetMrelInfoResetObject (SetMrelInfo *results) {

  int i;
  for (i = 0; i < results->Nsecfilt; i++) {
    results->havePS1[i] = FALSE;
    results->haveSYN[i] = FALSE;
    results->needSYN[i] = FALSE;
    results->measSYN[i] = -1;

    results->psfQfMax[i]  = NAN;
    results->psfQfPerfMax[i]  = NAN;

    results->Nmeas[i] = 0;
    results->NmeasGood[i] = 0;
    results->Next[i] = 0;
    results->NexpPS1[i] = 0;
    results->havePS1[i] = 0;
    results->haveUbercal[i] = 0;

    results->Nstargal   = 0;
    results->Nphotflags = 0;

    results->tessID[i]    = 0;
    results->projID[i]    = 0;
    results->skycellID[i] = 0;

    results->minUbercalDist[i] = 1000;

    results->psfData[i].Nlist = 0;
    results->aperData[i].Nlist = 0;
    results->kronData[i].Nlist = 0;
  }
}

void SetMrelInfoInit (SetMrelInfo *results, int allocLists) {

  int i;
  SetMrelInfoReset (results);
  if (allocLists) {
    results->Nsecfilt = GetPhotcodeNsecfilt ();

    results->psfData  = StatDataSetAlloc (results->Nsecfilt, Nmax);
    results->aperData = StatDataSetAlloc (results->Nsecfilt, Nmax);
    results->kronData = StatDataSetAlloc (results->Nsecfilt, Nmax);

    ALLOCATE (results->psfqf_list,     double, Nmax);
    ALLOCATE (results->psfqfperf_list, double, Nmax);
    ALLOCATE (results->stargal_list,   double, Nmax);
    ALLOCATE (results->photflag_list,  uint32_t, Nmax);

    ALLOCATE (results->havePS1,   int, results->Nsecfilt);
    ALLOCATE (results->haveSYN,   int, results->Nsecfilt);
    ALLOCATE (results->needSYN,   int, results->Nsecfilt);
    ALLOCATE (results->measSYN,   int, results->Nsecfilt);
    ALLOCATE (results->minSYN,  float, results->Nsecfilt);

    ALLOCATE (results->psfQfMax,     float, results->Nsecfilt);
    ALLOCATE (results->psfQfPerfMax, float, results->Nsecfilt);

    ALLOCATE (results->Nmeas,          int, results->Nsecfilt);
    ALLOCATE (results->NmeasGood,      int, results->Nsecfilt);
    ALLOCATE (results->Next,           int, results->Nsecfilt);
    ALLOCATE (results->NexpPS1,        int, results->Nsecfilt);
    ALLOCATE (results->haveUbercal,    int, results->Nsecfilt);

    ALLOCATE (results->tessID,         int, results->Nsecfilt);
    ALLOCATE (results->projID,         int, results->Nsecfilt);
    ALLOCATE (results->skycellID,      int, results->Nsecfilt);

    ALLOCATE (results->minUbercalDist, float, results->Nsecfilt);

    SetMrelInfoResetObject (results);

    for (i = 0; i < NsynthNames; i++) {
      int photcode = GetPhotcodeCodebyName (synthNames[i]);
      if (!photcode) continue;
      int Nsec = GetPhotcodeNsec (photcode);
      if (Nsec < 0) continue;
      results->minSYN[Nsec] = synthMaxMags[i];
    }
  } else {
    results->Nsecfilt = 0;
    results->psfData  = NULL;
    results->aperData = NULL;
    results->kronData = NULL;

    results->psfqf_list     = NULL;
    results->psfqfperf_list = NULL;
    results->stargal_list   = NULL;
    results->photflag_list  = NULL;

    results->havePS1 = NULL;
    results->haveSYN = NULL;
    results->needSYN = NULL;
    results->measSYN = NULL;
    results->minSYN  = NULL;

    results->psfQfMax = NULL;
    results->psfQfPerfMax = NULL;

    results->Nmeas = NULL;
    results->NmeasGood = NULL;
    results->Next = NULL;
    results->NexpPS1 = NULL;
    results->haveUbercal = NULL;

    results->tessID = NULL;
    results->projID = NULL;
    results->skycellID = NULL;

    results->minUbercalDist = NULL;
  }
}

void SetMrelInfoFree (SetMrelInfo *results) {

  StatDataSetFree (results->psfData,  results->Nsecfilt);
  StatDataSetFree (results->aperData, results->Nsecfilt);
  StatDataSetFree (results->kronData, results->Nsecfilt);

  FREE (results->psfqf_list);
  FREE (results->psfqfperf_list);
  FREE (results->stargal_list);
  FREE (results->photflag_list);

  FREE (results->havePS1);
  FREE (results->haveSYN);
  FREE (results->needSYN);
  FREE (results->measSYN);
  FREE (results->minSYN );

  FREE (results->psfQfMax);
  FREE (results->psfQfPerfMax);

  FREE (results->Nmeas);
  FREE (results->NmeasGood);
  FREE (results->Next);
  FREE (results->NexpPS1);
  FREE (results->haveUbercal);

  FREE (results->tessID);
  FREE (results->projID);
  FREE (results->skycellID);

  FREE (results->minUbercalDist);
}

void SetMrelInfoAccum (SetMrelInfo *summary, SetMrelInfo *results) {
  summary->Nfew  += results->Nfew ;
  summary->Ncode += results->Ncode ;
  summary->Nsys  += results->Nsys ;
  summary->Nbad  += results->Nbad ;
  summary->Ncal  += results->Ncal ;
  summary->Nmos  += results->Nmos ;
  summary->Ngrp  += results->Ngrp ;
  summary->Ngrid += results->Ngrid;
}

// mutex to lock setMrel_worker operations 
static pthread_mutex_t setMrel_mutex = PTHREAD_MUTEX_INITIALIZER;
static int nextCatalog = 0;

// we have an array of catalogs (catalog, Ncatalog).  we need to hand out catalogs one at a time to
// the worker threads as they need
off_t getNextCatalogForThread (int Ncatalog) {

  pthread_mutex_lock (&setMrel_mutex);
  if (nextCatalog >= Ncatalog) {
    pthread_mutex_unlock (&setMrel_mutex);
    return (-1);
  }
  int thisCatalog = nextCatalog;
  nextCatalog ++;

  pthread_mutex_unlock (&setMrel_mutex);
  return (thisCatalog);
}

// setMrel and setMrelOutput are extremely similar, but have slightly different implications:
// * both call setMrelCatalog, but setMrel set isSetMrelFinal to FALSE
// * setMrel only uses the internal Tiny structures
// * setMrel skips stars for which there are too few good measurements
// -> (setMrelOutput assigns an average mag for all objects, even those with only 1 measurement)
// * setMrelOutput sets psfQF, psfQFperf, extNsigma, rank info, etc
// * setMrelOutput sets average Map
// * setMrelOutput updates 2MASS average flags
// * setMrelOutput updates average EXT flags (PS1 and 2MASS)
// * setMrelOutput sets average stack & warp photometry (if requested), setMrel always skips those
// * setMrelOutput is NOT threaded it reads/writes catalogs one at a time

int setMrel (Catalog *catalog, int Ncatalog) {

  int i;

  if (NTHREADS) {
    int status = setMrel_threaded (catalog, Ncatalog);
    return status;
  }

  int Nsecfilt = GetPhotcodeNsecfilt ();

  SetMrelInfo summary, results;
  SetMrelInfoInit (&summary, FALSE);
  SetMrelInfoInit (&results, TRUE);

  for (i = 0; i < Ncatalog; i++) {
    setMrelCatalog (catalog, i, FALSE, &results, Nsecfilt);
    SetMrelInfoAccum (&summary, &results);
  }
  if (VERBOSE2) fprintf (stderr, "%d stars with no data in photcode, %d stars marked having too few measurements (Nbad: %d, Ncal: %d, Nmos: %d, Ngrp: %d, Ngrid: %d, Nsys: %d)\n", summary.Ncode, summary.Nfew, summary.Nbad, summary.Ncal, summary.Nmos, summary.Ngrp, summary.Ngrid, summary.Nsys);

  SetMrelInfoFree (&results);
  SetMrelInfoFree (&summary);

  return (TRUE);
}

int setMrelOutput (Catalog *catalog, int Ncatalog) {

  int i;

  int Nsecfilt = GetPhotcodeNsecfilt ();

  SetMrelInfo summary, results;
  SetMrelInfoInit (&summary, FALSE);
  SetMrelInfoInit (&results, TRUE); // allocates results->list,dlist,wlist

  for (i = 0; i < Ncatalog; i++) {
    setMrelCatalog (catalog, i, TRUE, &results, Nsecfilt);
    SetMrelInfoAccum (&summary, &results);
  }
  if (VERBOSE2) fprintf (stderr, "%d stars with no data in photcode, %d stars marked having too few measurements (Nbad: %d, Ncal: %d, Nmos: %d, Ngrp: %d, Ngrid: %d, Nsys: %d)\n", summary.Ncode, summary.Nfew, summary.Nbad, summary.Ncal, summary.Nmos, summary.Ngrp, summary.Ngrid, summary.Nsys);

  SetMrelInfoFree (&results);
  SetMrelInfoFree (&summary);
  return (TRUE);
}

int setMrel_threaded (Catalog *catalog, int Ncatalog) {

  int i;

  SetMrelInfo summary;
  SetMrelInfoInit (&summary, FALSE);

  pthread_attr_t attr;
  pthread_attr_init (&attr);
  pthread_attr_setdetachstate (&attr, PTHREAD_CREATE_DETACHED);
  
  pthread_t *threads;
  ALLOCATE (threads, pthread_t, NTHREADS);

  ThreadInfo *threadinfo;
  ALLOCATE (threadinfo, ThreadInfo, NTHREADS);

  // each time this function is called, we cycle through the available catalogs.
  // make sure we start at 0
  nextCatalog = 0;;

  // launch N worker threads
  for (i = 0; i < NTHREADS; i++) {
    threadinfo[i].entry = i;
    threadinfo[i].state = THREAD_RUN;
    threadinfo[i].catalog  =  catalog;
    threadinfo[i].Ncatalog = Ncatalog;
    SetMrelInfoInit (&threadinfo[i].summary, FALSE);
    pthread_create (&threads[i], NULL, setMrel_worker, &threadinfo[i]);
  }
  pthread_attr_destroy (&attr);

  // wait until all threads have finished
  while (1) {
    int allDone = TRUE;
    for (i = 0; i < NTHREADS; i++) {
      if (threadinfo[i].state == THREAD_RUN) allDone = FALSE;
    }
    if (allDone) {
      break;
    }
    usleep (500000);
  }

  // all threads are done, free the threads array and grab the info
  free (threads);
  
  // report stats & summary from the threads
  for (i = 0; i < NTHREADS; i++) {
    if (VERBOSE2) fprintf (stderr, "setMrel thread %d : %d stars with no data in photcode, %d stars marked having too few measurements (Nbad: %d, Ncal: %d, Nmos: %d, Ngrp: %d, Ngrid: %d, Nsys: %d)\n", 
	     i, 
	     threadinfo[i].summary.Ncode, 
	     threadinfo[i].summary.Nfew, 
	     threadinfo[i].summary.Nbad, 
	     threadinfo[i].summary.Ncal, 
	     threadinfo[i].summary.Nmos, 
	     threadinfo[i].summary.Ngrp, 
	     threadinfo[i].summary.Ngrid, 
	     threadinfo[i].summary.Nsys);
    SetMrelInfoAccum (&summary, &threadinfo[i].summary);
    SetMrelInfoFree (&threadinfo[i].summary);
  }
  if (VERBOSE2) fprintf (stderr, "total : %d stars with no data in photcode, %d stars marked having too few measurements (Nbad: %d, Ncal: %d, Nmos: %d, Ngrp: %d, Ngrid: %d, Nsys: %d)\n", summary.Ncode, summary.Nfew, summary.Nbad, summary.Ncal, summary.Nmos, summary.Ngrp, summary.Ngrid, summary.Nsys);
  free (threadinfo);

  SetMrelInfoFree (&summary);
  return TRUE;
}

void *setMrel_worker (void *data) {

  ThreadInfo *threadinfo = data;

  int Nsecfilt = GetPhotcodeNsecfilt ();

  SetMrelInfo results;
  SetMrelInfoInit (&results, TRUE); // allocate list, dlist arrays here

  while (1) {
    off_t i = getNextCatalogForThread(threadinfo->Ncatalog);
    if (i == -1) {
      threadinfo->state = THREAD_DONE;
      SetMrelInfoFree (&results);
      return NULL;
    }

    Catalog *catalog = threadinfo->catalog;

    setMrelCatalog (catalog, i, FALSE, &results, Nsecfilt);
    SetMrelInfoAccum (&threadinfo->summary, &results);
  }

  SetMrelInfoFree (&results);
  return NULL;
}

int print_measure_set (Average *average, SecFilt *secfilt, Measure *measure) {

  off_t k;

  int Nsecfilt = GetPhotcodeNsecfilt ();

  off_t m = average[0].measureOffset;

  for (k = 0; k < average[0].Nmeasure; k++, m++) {
    fprintf (stderr, "meas: %08x\n", measure[m].dbFlags);
  }

  int Ns;
  for (Ns = 0; Ns < Nsecfilt; Ns++) {
    fprintf (stderr, "secf: %08x\n", secfilt[Ns].flags);
  }
  return 1;
}

/* set measure.Mcal for all measures except ID_MEAS_NOCAL */
int setMcalOutput (Catalog *catalog, int Ncatalog) {

  int i;
  off_t j, k, m;

  for (i = 0; i < Ncatalog; i++) {
    for (j = 0; j < catalog[i].Naverage; j++) {

      m = catalog[i].averageT[j].measureOffset;
      for (k = 0; k < catalog[i].averageT[j].Nmeasure; k++, m++) {
	if (catalog[i].measureT[m].dbFlags & ID_MEAS_NOCAL) continue;
	float McalPSF  = getMcal  (m, i, MAG_CLASS_PSF);
	float McalAPER = getMcal  (m, i, MAG_CLASS_APER);
	if (isnan(McalPSF)) continue;

	// Mmos, Mgrp have already been transfered to Mcal and should be zero here
	// float Mmos  = getMmos  (m, i);
	// if (isnan(Mmos)) continue;
	// float Mgrp  = getMgrp  (m, i, catalog[i].measureT[m].airmass, NULL);
	// if (isnan(Mgrp)) continue;

	// Mgrid is applied to Mflat and should be ignored here
	// float Mgrid = getMgridTiny (&catalog[i].measureT[m]);
	// if (isnan(Mgrid)) continue;

	// Note that this operation is setting measure->McalAPER to image->McalAPER only
	// for the stacks.  Elsewhere (setMrelCatalog) we are using image->McalPSF for all
	// types of measurements EXCEPT stacks.

	// we need to use McalAPER for stacks (and only stacks)
	int useStackAper = !USE_MCAL_PSF_FOR_STACK_APER && isGPC1stack(catalog[i].measureT[m].photcode);

	// note that measurements for which the image is not selected will not be modified
	// (that is a good thing! -- we keep a prior calibration)

	// set the output calibration
	catalog[i].measure[m].McalPSF  = McalPSF;
	catalog[i].measure[m].McalAPER = useStackAper ? McalAPER : McalPSF;

	if (catalog[i].measureT[m].dbFlags & ID_MEAS_PHOTOM_UBERCAL) {
	  myAssert (isfinite(catalog[i].measure[m].McalPSF), "oops, broke an ubercal mag");
	}
      }
    }
  }
  return (TRUE);
}

int markObjects (Catalog *catalog, int Ncatalog) {

  int i, n;
  off_t j;

  // How strongly do I own this object?
  for (i = 0; i < Ncatalog; i++) {
    for (j = 0; j < catalog[i].Naverage; j++) {
      int nOwn = 0;
      int m = catalog[i].averageT[j].measureOffset;
      for (n = 0; n < catalog[i].averageT[j].Nmeasure; n++) {
	if (!catalog[i].measureT[m+n].myDet) continue;
	nOwn ++;
      }
      catalog[i].averageT[j].nOwn = nOwn;
    }
  }
  return TRUE;
}

int dumpObjects (char *filename, Catalog *catalog, int Ncatalog) {

  int i, n;
  off_t j;

  FILE *ftest = fopen (filename, "w");

  for (i = 0; i < Ncatalog; i++) {
    for (j = 0; j < catalog[i].Naverage; j++) {
      int m = catalog[i].averageT[j].measureOffset;
      for (n = 0; n < catalog[i].averageT[j].Nmeasure; n++) {
	fprintf (ftest, "%08x %08x %10.6f %10.6f  %3d %1d\n", catalog[i].averageT[j].catID, catalog[i].averageT[j].objID, catalog[i].averageT[j].R, catalog[i].averageT[j].D, catalog[i].averageT[j].Nmeasure, catalog[i].measureT[m+n].myDet); 
      }
    }
  }
  fclose (ftest);
  return TRUE;
}

int dumpMags (FILE *fout, Catalog *catalog, int Ncatalog) {

  int i, n;
  off_t j;

  int Nsecfilt = GetPhotcodeNsecfilt ();

  for (i = 0; i < Ncatalog; i++) {
    for (j = 0; (j < catalog[i].Naverage) && (j < 2); j++) {
      fprintf (fout, "%08x %08x %10.6f %10.6f : \n", catalog[i].averageT[j].catID, catalog[i].averageT[j].objID, catalog[i].averageT[j].R, catalog[i].averageT[j].D); 
      for (n = 0; n < 5; n++) {
	fprintf (fout, "secf  %5d | %6.3f %6.3f | %6.3f %6.3f | %6.3f %6.3f\n", n, catalog[i].secfilt[j*Nsecfilt + n].MpsfChp, catalog[i].secfilt[j*Nsecfilt + n].MkronChp, catalog[i].secfilt[j*Nsecfilt + n].MpsfStk, catalog[i].secfilt[j*Nsecfilt + n].MkronStk, catalog[i].secfilt[j*Nsecfilt + n].MpsfWrp, catalog[i].secfilt[j*Nsecfilt + n].MkronWrp);
      }
      int m = catalog[i].averageT[j].measureOffset;
      for (n = 0; n < catalog[i].averageT[j].Nmeasure; n++) {
	fprintf (fout, "meas %5d %5d | %6.3f %6.3f | %6.3f %6.3f\n", m+n, catalog[i].measureT[m+n].photcode, catalog[i].measureT[m+n].M, catalog[i].measureT[m+n].Mkron, catalog[i].measureT[m+n].McalPSF, catalog[i].measureT[m+n].McalAPER); 
      }
    }
  }
  return TRUE;
}

static float MinChiSqLim = NAN;
static float MaxChiSqLim = NAN;
static float MinScatterLim = NAN;
static float MaxScatterLim = NAN;

void clean_stars (Catalog *catalog, int Ncatalog) {

  int Ndel, Nave, Ntot, Nscat, Nchi, Nnan;
  double *xlist, *slist;

  if (isnan (MaxChiSqLim))   MaxChiSqLim   = STAR_CHISQ;
  if (isnan (MaxScatterLim)) MaxScatterLim = STAR_SCATTER;

  StatType stats;
  liststats_setmode (&stats, "MEAN");

  if (VERBOSE) fprintf (stderr, "marking poor stars\n");

  /* find Mchisq median -> ChiSq lim must be > median */
  for (int i = Ntot = 0; i < Ncatalog; i++) {
    Ntot += catalog[i].Naverage; 
  }
  ALLOCATE (xlist, double, Ntot);
  ALLOCATE (slist, double, Ntot);

  int Nsecfilt = GetPhotcodeNsecfilt ();

  // eliminate bad stars using the stats for a single secfilt at a time
  for (int Ns = 0; Ns < Nphotcodes; Ns ++) {
    
    int thisCode = photcodes[Ns][0].code;
    int Nsec = GetPhotcodeNsec(thisCode);

    for (int i = Ntot = 0; i < Ncatalog; i++) {
      for (int j = 0; j < catalog[i].Naverage; j++) {
	if (catalog[i].secfilt[Nsecfilt*j+Nsec].flags & STAR_BAD) continue;
	float Mchisq = catalog[i].secfilt[Nsecfilt*j+Nsec].Mchisq;
	if (isnan(Mchisq)) continue;
	xlist[Ntot] = Mchisq;
	slist[Ntot] = catalog[i].secfilt[Nsecfilt*j+Nsec].dMpsfChp;
	Ntot ++;
      }
    }

    liststats (xlist, NULL, NULL, Ntot, &stats);
    float ChiSqUpper90 = stats.Upper90; 
    if (isnan (MinChiSqLim)) MinChiSqLim = 2.0*stats.median;             // chi-square cut cannot fall below this value (even if this is > MaxChiSqLim)
    float ChiSqLimit = MAX(MinChiSqLim, MIN(MaxChiSqLim, ChiSqUpper90)); // chi-square cut should be between MinChiSqLim and MaxChiSqLim

    liststats (slist, NULL, NULL, Ntot, &stats);
    float ScatterUpper90 = stats.Upper90;
    if (isnan (MinScatterLim)) MinScatterLim = 2.0*stats.median;         // scatter cut cannto fall below this value (even if this is > MaxScatterLim)
    float ScatterLimit = MAX(MinScatterLim, MIN(MaxScatterLim, ScatterUpper90));

    fprintf (stderr, "STARS: ChiSqLimit: %f, ScatterLimit: %f | ChiSquare Upper 90: %f, Scatter Upper 90: %f\n", ChiSqLimit, ScatterLimit, ChiSqUpper90, ScatterUpper90);

    Ndel = Nave = Nscat = Nnan = Nchi = 0;
    for (int i = 0; i < Ncatalog; i++) {
      for (int j = 0; j < catalog[i].Naverage; j++) {
	float dM = catalog[i].secfilt[Nsecfilt*j+Nsec].dMpsfChp;
	float Mchisq = catalog[i].secfilt[Nsecfilt*j+Nsec].Mchisq;
	int mark = (dM > ScatterLimit) || (isnan(Mchisq)) || (Mchisq > ChiSqLimit);
	if (mark) {
	  catalog[i].secfilt[Nsecfilt*j+Nsec].flags |= ID_SECF_STAR_POOR;
	  Ndel ++;
	  if (dM > ScatterLimit)   { Nscat ++; }
	  if (isnan(Mchisq))       { Nnan ++; }
	  if (Mchisq > ChiSqLimit) { Nchi ++; }
	} else {
	  catalog[i].secfilt[Nsecfilt*j+Nsec].flags &= ~ID_SECF_STAR_POOR;
	}
	Nave ++;
      }
    }
    fprintf (stderr, "%d of %d stars marked variable (%d scat, %d nan, %d chi)\n", Ndel, Nave, Nscat, Nnan, Nchi);
  }
  free (xlist);
  free (slist);
}

StatType statsStarN (Catalog *catalog, int Ncatalog, int Nsec, int seccode) {

  off_t j, k, m, Ntot;
  int i, n, N;
  double *list, *dlist;
  float Mcal, Mmos, Mgrid;
  StatType stats;
  // int N1, N2, N3, N4, N0;
  // N1 = N2 = N3 = N4 = N0 = 0;

  int Nsecfilt = GetPhotcodeNsecfilt ();

  Ntot = 0;
  for (i = 0; i < Ncatalog; i++) {
    Ntot += catalog[i].Naverage; 
  }

  ALLOCATE (list, double, Ntot);
  ALLOCATE (dlist, double, Ntot);

  n = 0;
  for (i = 0; i < Ncatalog; i++) {
    for (j = 0; j < catalog[i].Naverage; j++) {

      /* calculate the average value for a single star */
      if (catalog[i].secfilt[Nsecfilt*j+Nsec].flags & STAR_BAD) { continue;  }
      m = catalog[i].averageT[j].measureOffset;

      N = 0;
      for (k = 0; k < catalog[i].averageT[j].Nmeasure; k++, m++) {
	int ecode = GetPhotcodeEquivCodebyCode (catalog[i].measureT[m].photcode);
	if (ecode != seccode) { continue;}
	Mcal = getMcal  (m, i, MAG_CLASS_PSF);
	if (isnan(Mcal)) { continue;}
	Mmos = getMmos  (m, i);
	if (isnan(Mmos)) { continue; }
	Mgrid = getMgridTiny (&catalog[i].measureT[m]);
	if (isnan(Mgrid)) { continue;}
	N++;
      }
      
      list[n] = N;
      dlist[n] = 1;
      n++;
    }
  }

  liststats_setmode (&stats, "MEAN");

  liststats (list, dlist, NULL, n, &stats);
  free (list);
  free (dlist);
  return (stats);
}

// stats for a single secfilt at a time
StatType statsStarX (Catalog *catalog, int Ncatalog, int Nsec) {

  off_t j, Ntot;
  int i, n;
  double *list, *dlist;
  StatType stats;

  int Nsecfilt = GetPhotcodeNsecfilt ();

  Ntot = 0;
  for (i = 0; i < Ncatalog; i++) {
    Ntot += catalog[i].Naverage; 
  }

  ALLOCATE (list, double, Ntot);
  ALLOCATE (dlist, double, Ntot);

  n = 0;
  for (i = 0; i < Ncatalog; i++) {
    for (j = 0; j < catalog[i].Naverage; j++) {

      /* calculate the average value for a single star */
      if (catalog[i].secfilt[Nsecfilt*j+Nsec].flags & STAR_BAD) continue;  
	
      float Mchisq = catalog[i].secfilt[Nsecfilt*j+Nsec].Mchisq;
      if (isnan(Mchisq)) continue;
      list[n] = Mchisq;
      dlist[n] = 1;
      n++;
    }
  }

  liststats_setmode (&stats, "MEAN");

  liststats (list, dlist, NULL, n, &stats);
  free (list);
  free (dlist);
  return (stats);
}

StatType statsStarS (Catalog *catalog, int Ncatalog, int Nsec) {

  int i, n;
  off_t j, Ntot;
  double *list, *dlist;
  float dM;
  StatType stats;

  int Nsecfilt = GetPhotcodeNsecfilt ();

  Ntot = 0;
  for (i = 0; i < Ncatalog; i++) {
    Ntot += catalog[i].Naverage; 
  }

  ALLOCATE (list, double, Ntot);
  ALLOCATE (dlist, double, Ntot);

  n = 0;
  for (i = 0; i < Ncatalog; i++) {
    for (j = 0; j < catalog[i].Naverage; j++) {

      /* calculate the average value for a single star */
      if (catalog[i].secfilt[Nsecfilt*j+Nsec].flags & STAR_BAD) continue;  

      dM = catalog[i].secfilt[Nsecfilt*j+Nsec].dMpsfChp;
      if (isnan(dM)) continue;
      list[n] = dM;
      dlist[n] = 1;
      n++;
    }
  }

  liststats_setmode (&stats, "MEAN");

  liststats (list, dlist, NULL, n, &stats);
  free (list);
  free (dlist);
  return (stats);
}

void plot_stars (Catalog *catalog, int Ncatalog) {

  int i, bin;
  off_t j;
  float dMrel;
  double *xlist, *Mlist;
  Graphdata graphdata;

# define NBIN 200
  ALLOCATE (xlist, double, NBIN);
  ALLOCATE (Mlist, double, NBIN);

  int Nsecfilt = GetPhotcodeNsecfilt ();

  int Ns;
  for (Ns = 0; Ns < Nphotcodes; Ns++) {

    int thisCode = photcodes[Ns][0].code;
    int Nsec = GetPhotcodeNsec(thisCode);

    for (i = 0; i < NBIN; i++) xlist[i] = 0.00025*i;
    bzero (Mlist, NBIN*sizeof(double));
    for (i = 0; i < Ncatalog; i++) {
      for (j = 0; j < catalog[i].Naverage; j++) {
	if (catalog[i].secfilt[Nsecfilt*j+Nsec].flags & STAR_BAD) continue;  
	dMrel = catalog[i].secfilt[Nsecfilt*j+Nsec].dMpsfChp;
	bin = dMrel / 0.00025;
	bin = MAX (0, MIN (NBIN-1, bin));
	Mlist[bin] += 1.0;
      }
    }

    plot_defaults (&graphdata);
    graphdata.style = 1;
    plot_list (&graphdata, xlist, Mlist, NBIN, "dMrel hist", "%s.dMhist.png", OUTROOT);
  }
  free (xlist);
  free (Mlist);
}

void plot_chisq (Catalog *catalog, int Ncatalog) {

  off_t j, Ntotal;
  int i, N, value;
  double *xlist, *ylist;
  Graphdata graphdata;

  int Nsecfilt = GetPhotcodeNsecfilt ();

  Ntotal = 0;
  for (i = 0; i < Ncatalog; i++) Ntotal += catalog[i].Naverage;

  ALLOCATE (xlist, double, Ntotal);
  ALLOCATE (ylist, double, Ntotal);

  int Ns;
  for (Ns = 0; Ns < Nphotcodes; Ns++) {

    int thisCode = photcodes[Ns][0].code;
    int Nsec = GetPhotcodeNsec(thisCode);

    N = 0;
    for (i = 0; i < Ncatalog; i++) {
      for (j = 0; j < catalog[i].Naverage; j++) {
	if (catalog[i].secfilt[Nsecfilt*j+Nsec].flags & STAR_BAD) continue;
	xlist[N] = catalog[i].secfilt[Nsecfilt*j+Nsec].MpsfChp;
	value    = catalog[i].secfilt[Nsecfilt*j+Nsec].Mchisq;
	if (isnan((double)(value))) continue;
	ylist[N] = value;
	N++;
      }
    }

    plot_defaults (&graphdata);
    graphdata.ymin = -3.0;
    plot_list (&graphdata, xlist, ylist, N, "chisq", "%s.chisq.png", OUTROOT);
  }

  free (xlist);
  free (ylist);
}

void plot_star_coords (Catalog *catalog, int Ncatalog) {

  int i;
  off_t j, N;
  double *xlist, *ylist;
  // double Xmin, Ymin, Xmax, Ymax;
  Graphdata graphdata;

  N = 0; 
  for (i = 0; i < Ncatalog; i++) {
    N += catalog[i].Naverage;
  }
  ALLOCATE (xlist, double, N);
  ALLOCATE (ylist, double, N);

  N = 0;
  // Xmin = Ymin = +360.0;
  // Xmax = Ymax = -360.0;
  for (i = 0; i < Ncatalog; i++) {
    for (j = 0; j < catalog[i].Naverage; j++) {
      xlist[N] = catalog[i].averageT[j].R;
      ylist[N] = catalog[i].averageT[j].D;
      N++;
    }
  }
  plot_defaults (&graphdata);
  plot_list (&graphdata, xlist, ylist, N, "coords", "%s.coords.png", OUTROOT);

  free (xlist);
  free (ylist);
}

void dump_catalog (Catalog *catalog, off_t c, int Npass) {

  return;

  int Nsecfilt = GetPhotcodeNsecfilt ();

  FILE *fout = NULL;
  char filename[64];
  snprintf (filename, 64, "tcat.%02d.dat", Npass);
  fout = fopen (filename, "w");

  for (off_t n = 0; n < catalog[c].Naverage; n++) {

    off_t m = catalog[c].averageT[n].measureOffset;
    for (off_t k = 0; k < catalog[c].averageT[n].Nmeasure; k++, m++) {

      float Mcal     = getMcal  (m, c, MAG_CLASS_PSF);     // image zero point
      float Mmos     = getMmos  (m, c);                    // mosaic zero point
      float Mgrp     = getMgrp  (m, c, catalog[c].measureT[m].airmass, NULL);

      float MrelPSF  = getMrel  (catalog, m, c, MAG_CLASS_PSF, MAG_SRC_CHP); // average magnitude
      // float Mgrid    = getMgridTiny (&catalog[c].measureT[m]);                    // camera offset (deprecated?)
      // float Mflat    = getMflat (m, c, catalog); // flat-field correction

      off_t n = catalog[c].measureT[m].averef;

      // nominal magnitude for this measurement (instrumental + Ko(airmass - 1) + Co
      float MsysPSF = PhotSysTiny (&catalog[c].measureT[m], &catalog[c].averageT[n], &catalog[c].secfilt[n*Nsecfilt], MAG_CLASS_PSF);

      // for historical reasons, Mflat is defined with the wrong sign
      float delta = MsysPSF - MrelPSF - Mcal - Mmos - Mgrp;

      fprintf (fout, "%f %f : %f %f : %f %f %f  : %f %f\n", catalog[c].averageT[n].R, catalog[c].averageT[n].D, MsysPSF, MrelPSF, Mcal, Mmos, Mgrp, catalog[c].measureT[m].airmass, delta);
    }
  }
  fclose (fout);
}

// maybe make this a macro
float getMflat (off_t meas, int cat, Catalog *catalog) {
  float offset = catalog[cat].measureT[meas].Mflat;
  if (!isfinite(offset)) {
    offset = 0.0;
  }
  return (offset);
}
