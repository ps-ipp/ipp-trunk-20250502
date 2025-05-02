# include "relastro.h"

int plotChipFits (double *Ro, double *Do, char *mode, int Nimage);
int saveCenter (Image *image, double *Ro, double *Do, int im);
off_t getNextImageForThread ();
int UpdateChips_threaded (Catalog *catalog, int Ncatalog);
void *UpdateChips_worker (void *data);

enum {THREAD_RUN, THREAD_DONE};

typedef struct {
  int entry;
  int state;
  double *Ro;
  double *Do;
  char *mode;
  off_t Nskip;
  off_t Nmosaic;
  off_t NnewFit;
  off_t NoldFit;
  Catalog *catalog;
  int Ncatalog;
} ThreadInfo;

static Image *image = NULL;
static off_t Nimage = 0;
static off_t nextImage = 0;

// update astrometry of all chips relative to the average positions
// if NTHREADS is non-zero, call the threaded version of this function
int UpdateChips (Catalog *catalog, int Ncatalog, int Nloop) {

  off_t Nskip, Nmosaic, NnewFit, NoldFit;

  /* we can measure new image parameters for each non-mosaic chip independently */
  off_t i, Nraw, Nref, nFitAstr;
  StarData *raw, *ref;
  float dXpixSys, dYpixSys;
  double *Ro, *Do;
  char *mode;

  AstromErrorSetLoop (Nloop, TRUE);

  // if ChipMapLoop or ChipOrderLoop is set use that to define the value of CHIPMAP and/or CHIPORDER this loop
  if (ChipMapLoop) { CHIPMAP = ChipMapLoop[Nloop]; }
  if (ChipOrderLoop) { CHIPORDER = ChipOrderLoop[Nloop]; }
  
  if (NTHREADS) {
    UpdateChips_threaded (catalog, Ncatalog);
    return TRUE;
  }

  Nskip = Nmosaic = NnewFit = NoldFit = 0;

  image = getimages (&Nimage, NULL);

  // save fit results for summary plot
  ALLOCATE (Ro, double, Nimage);
  ALLOCATE (Do, double, Nimage);
  ALLOCATE (mode, char, Nimage);

  // XXX for faster processing in the future, this can be easily run in parallel
  // each chip is fitted independently, so we could do N at once in parallel
  for (i = 0; i < Nimage; i++) {

    VERBOSE = FALSE;
    VERBOSE_IMAGE |= !strcmp(image[i].name, "o5745g0516o.356887.cm.982631.smf[XY45]");
    VERBOSE_IMAGE |= !strcmp(image[i].name, "o5745g0526o.356899.cm.982643.smf[XY45]");
    VERBOSE_IMAGE |= !strcmp(image[i].name, "o5748g0436o.358811.cm.982690.smf[XY34]");

    // XXX looks like everything below is thread safe : we can unroll this into a set of
    // helper functions that grab the next available chip....

    // allow certain cameras to stay static
    if (SKIP_PS1_CHIP  && isGPC1chip (image[i].photcode)) { Nskip ++;  mode[i] = 0; continue; }
    if (SKIP_PS1_STACK && isGPC1stack(image[i].photcode)) { Nskip ++;  mode[i] = 0; continue; }
    if (SKIP_HSC       && isHSCchip  (image[i].photcode)) { Nskip ++;  mode[i] = 0; continue; }
    if (SKIP_CFH       && isCFHchip  (image[i].photcode)) { Nskip ++;  mode[i] = 0; continue; }
    
    /* skip all except WRP images */
    if (strcmp(&image[i].coords.ctype[4], "-WRP")) {
      Nmosaic ++;
      mode[i] = 0;
      continue;
    }

    /* convert measure coordinates to raw entries */
    raw = getImageRaw (catalog, Ncatalog, i, &Nraw, MODE_MOSAIC);
    if (!raw) {
      // fprintf (stderr, "skip 1 %s\n", image[i].name);
      Nskip ++;
      mode[i] = 0;
      continue;
    }
    if (Nraw <= IMFIT_TOO_FEW) {
      // fprintf (stderr, "skip 2 %s\n", image[i].name);
      Nskip ++;
      mode[i] = 0;
      free (raw);
      continue;
    }

    /* convert average coordinates to ref entries */
    ref = getImageRef (catalog, Ncatalog, i, &Nref, MODE_MOSAIC);
    if (!ref) {
      // fprintf (stderr, "skip 3 %s\n", image[i].name);
      Nskip ++;
      mode[i] = 0;
      free (raw);
      continue;
    }

    if (VERBOSE_IMAGE) { 
      dump_stardata_pts (raw, Nraw, "testimage.raw.dat");
      dump_stardata_pts (ref, Nref, "testimage.ref.dat");
      fprintf (stderr, "dumped test image\n");
    }

    // note that Nraw & Nref must be equal: if not, we made a programming error in one of these two functions.
    assert (Nraw == Nref);

    if (IMSTATS_ONLY) {
      int Nstat;
      float dLsig, dMsig, dRsig;
      GetScatterRawRef(&dLsig, &dMsig, &dRsig, &Nstat, raw, ref, Nraw, IMFIT_SYS_SIGMA_LIM);

      // XXX: I need to convert dLsig, dMsig from degrees to pixels
      dLsig *= 3600.0;
      dMsig *= 3600.0;

      image[i].dXpixSys = dLsig;
      image[i].dYpixSys = dMsig;
      image[i].nFitAstrom = Nstat;
      continue;
    }

    // save these in case of failure
    Coords oldCoords;
    SaveCoords (&oldCoords, &image[i].coords);

    dXpixSys = image[i].dXpixSys;
    dYpixSys = image[i].dYpixSys;
    nFitAstr = image[i].nFitAstrom;

    // FitChip does iterative, clipped fitting
    // fprintf (stderr, "image "OFF_T_FMT" : Nstars: "OFF_T_FMT"\n",  i,  Nraw);
    if (!FitChip (raw, ref, Nraw, &image[i])) {
      if (VERBOSE) fprintf (stderr, "reject fit for image %s ("OFF_T_FMT") : Nstars: "OFF_T_FMT", Nused %d of %d\n", image[i].name,  i,  Nraw, image[i].nFitAstrom, image[i].nstar);

      if (1) {
	// restore status quo ante (replace truMap with tmpMap)
	RestoreCoords (&image[i].coords, &oldCoords, &image[i]);
	image[i].dXpixSys = dXpixSys;
	image[i].dYpixSys = dYpixSys; 
	image[i].nFitAstrom = nFitAstr;

	saveCenter (image, &Ro[i], &Do[i], i);
	mode[i] = 1;
	NoldFit ++;
	free (raw);
	free (ref);
	continue;
      }
    }

    if (!checkStarMap (i)) {
      if (VERBOSE) fprintf (stderr, "fit diverges too much for image %s ("OFF_T_FMT") : Nstars: "OFF_T_FMT", Nused: %d\n", image[i].name,  i,  Nraw, image[i].nFitAstrom);

      if (1) {
	// restore status quo ante (replace truMap with tmpMap)
	RestoreCoords (&image[i].coords, &oldCoords, &image[i]);
	image[i].dXpixSys = dXpixSys;
	image[i].dYpixSys = dYpixSys; 
	image[i].nFitAstrom = nFitAstr;

	saveCenter (image, &Ro[i], &Do[i], i);
	mode[i] = 2;
	image[i].flags |= ID_IMAGE_ASTROM_POOR;
	NoldFit ++;
	free (raw);
	free (ref);
	continue;
      }
    } 

    AstromOffsetMapFree (oldCoords.offsetMap);

    // Apply the modified coords back to the measure.R,D.  Note that raw.R,D, ref.L,M, etc
    // are all automatically updated in this block because they are re-generated from
    // image.coords on each pass.
    setImageRaw (catalog, Ncatalog, i, raw, Nraw, MODE_MOSAIC);
    if (USE_GALAXY_MODEL) {
      // the image calibration was calculated using a galaxy motion model
      image[i].flags |= ID_IMAGE_ASTROM_GMM;
    }

    saveCenter (image, &Ro[i], &Do[i], i);
    mode[i] = 3;
    NnewFit ++;
    free (raw);
    free (ref);
  }

  plotChipFits (Ro, Do, mode, Nimage);
  free (Ro);
  free (Do);
  free (mode);

  fprintf (stderr, "UpdateChips: %d fitted, %d keep old, %d skipped, %d mosaic (skipped)\n", 
	   (int) NnewFit, (int) NoldFit, (int) Nskip, (int) Nmosaic);
  return (TRUE);
}

int UpdateChips_threaded (Catalog *catalog, int Ncatalog) {

  int i;
  off_t Nskip, Nmosaic, NnewFit, NoldFit;
  double *Ro, *Do;
  char *mode;

  image = getimages (&Nimage, NULL);
  nextImage = 0;

  // save fit results for summary plot
  ALLOCATE (Ro, double, Nimage);
  ALLOCATE (Do, double, Nimage);
  ALLOCATE (mode, char, Nimage);

  pthread_attr_t attr;
  pthread_attr_init (&attr);
  pthread_attr_setdetachstate (&attr, PTHREAD_CREATE_DETACHED);
  
  pthread_t *threads;
  ALLOCATE (threads, pthread_t, NTHREADS);

  ThreadInfo *threadinfo;
  ALLOCATE (threadinfo, ThreadInfo, NTHREADS);

  // launch N worker threads
  for (i = 0; i < NTHREADS; i++) {
    threadinfo[i].entry = i;
    threadinfo[i].state = THREAD_RUN;
    threadinfo[i].Ro = Ro;
    threadinfo[i].Do = Do;
    threadinfo[i].mode = mode;
    threadinfo[i].Nskip = 0;
    threadinfo[i].Nmosaic = 0;
    threadinfo[i].NnewFit = 0;
    threadinfo[i].NoldFit = 0;
    threadinfo[i].catalog  =  catalog;
    threadinfo[i].Ncatalog = Ncatalog;
    pthread_create (&threads[i], NULL, UpdateChips_worker, &threadinfo[i]);
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
  for (i = 0; i < NTHREADS; i++) {
    pthread_detach (threads[i]);
  }
  free (threads);
  
  plotChipFits (Ro, Do, mode, Nimage);
  free (Ro);
  free (Do);
  free (mode);

  // report stats & summary from the threads
  Nskip = Nmosaic = NnewFit = NoldFit = 0;
  for (i = 0; i < NTHREADS; i++) {
    fprintf (stderr, "UpdateChips thread %d : %d fitted, %d keep old, %d skipped, %d mosaic (skipped)\n", 
	     i, (int) threadinfo[i].NnewFit, (int) threadinfo[i].NoldFit, (int) threadinfo[i].Nskip, (int) threadinfo[i].Nmosaic);

    NnewFit += threadinfo[i].NnewFit;
    NoldFit += threadinfo[i].NoldFit;
    Nskip   += threadinfo[i].Nskip;
    Nmosaic += threadinfo[i].Nmosaic;
  }
  free (threadinfo);

  fprintf (stderr, "UpdateChips: %d fitted, %d keep old, %d skipped, %d mosaic (skipped)\n", 
	   (int) NnewFit, (int) NoldFit, (int) Nskip, (int) Nmosaic);
  return (TRUE);
}

void *UpdateChips_worker (void *data) {

  /* we can measure new image parameters for each non-mosaic chip independently */
  off_t Nraw, Nref, nFitAstr;
  StarData *raw, *ref;
  float dXpixSys, dYpixSys;

  ThreadInfo *threadinfo = data;

  while (1) {

    off_t i = getNextImageForThread();
    if (i == -1) {
      threadinfo->state = THREAD_DONE;
      return NULL;
    }

    // allow certain cameras to stay static
    if (SKIP_PS1_CHIP  && isGPC1chip (image[i].photcode)) { threadinfo->Nskip ++;  threadinfo->mode[i] = 0; continue; }
    if (SKIP_PS1_STACK && isGPC1stack(image[i].photcode)) { threadinfo->Nskip ++;  threadinfo->mode[i] = 0; continue; }
    if (SKIP_HSC       && isHSCchip  (image[i].photcode)) { threadinfo->Nskip ++;  threadinfo->mode[i] = 0; continue; }
    if (SKIP_CFH       && isCFHchip  (image[i].photcode)) { threadinfo->Nskip ++;  threadinfo->mode[i] = 0; continue; }
    
    /* skip all except WRP images */
    if (strcmp(&image[i].coords.ctype[4], "-WRP")) {
      threadinfo->Nmosaic ++;
      threadinfo->mode[i] = 0;
      continue;
    }

    /* convert measure coordinates to raw entries */
    raw = getImageRaw (threadinfo->catalog, threadinfo->Ncatalog, i, &Nraw, MODE_MOSAIC);
    if (!raw) {
      threadinfo->Nskip ++;
      threadinfo->mode[i] = 0;
      continue;
    }
    if (Nraw <= IMFIT_TOO_FEW) {
      threadinfo->Nskip ++;
      threadinfo->mode[i] = 0;
      free (raw);
      continue;
    }

    /* convert average coordinates to ref entries */
    ref = getImageRef (threadinfo->catalog, threadinfo->Ncatalog, i, &Nref, MODE_MOSAIC);
    if (!ref) {
      threadinfo->Nskip ++;
      threadinfo->mode[i] = 0;
      free (raw);
      continue;
    }

    // note that Nraw & Nref must be equal: if not, we made a programming error in one of these two functions.
    assert (Nraw == Nref);

    // save these in case of failure
    Coords oldCoords;
    SaveCoords (&oldCoords, &image[i].coords);
    dXpixSys = image[i].dXpixSys;
    dYpixSys = image[i].dYpixSys;
    nFitAstr = image[i].nFitAstrom;

    // FitChip does iterative, clipped fitting
    // fprintf (stderr, "image "OFF_T_FMT" : Nstars: "OFF_T_FMT"\n",  i,  Nraw);
    if (!FitChip (raw, ref, Nraw, &image[i])) {
      if (VERBOSE) fprintf (stderr, "reject fit for image %s ("OFF_T_FMT") : Nstars: "OFF_T_FMT", Nused %d of %d\n", image[i].name,  i,  Nraw, image[i].nFitAstrom, image[i].nstar);

      // restore status quo ante (replace truMap with tmpMap)
      RestoreCoords (&image[i].coords, &oldCoords, &image[i]);
      image[i].dXpixSys = dXpixSys;
      image[i].dYpixSys = dYpixSys; 
      image[i].nFitAstrom = nFitAstr;

      saveCenter (image, &threadinfo->Ro[i], &threadinfo->Do[i], i);
      threadinfo->mode[i] = 1;
      threadinfo->NoldFit ++;
      free (raw);
      free (ref);
      continue;
    }

    if (!checkStarMap (i)) {
      if (VERBOSE) fprintf (stderr, "fit diverges too much for image %s ("OFF_T_FMT") : Nstars: "OFF_T_FMT", Nused: %d\n", image[i].name,  i,  Nraw, image[i].nFitAstrom);

      // restore status quo ante (replace truMap with tmpMap)
      RestoreCoords (&image[i].coords, &oldCoords, &image[i]);
      image[i].dXpixSys = dXpixSys;
      image[i].dYpixSys = dYpixSys; 
      image[i].nFitAstrom = nFitAstr;

      saveCenter (image, &threadinfo->Ro[i], &threadinfo->Do[i], i);
      threadinfo->mode[i] = 2;
      image[i].flags |= ID_IMAGE_ASTROM_POOR;
      threadinfo->NoldFit ++;
      free (raw);
      free (ref);
      continue;
    } 

    AstromOffsetMapFree (oldCoords.offsetMap);

    // apply the modified R,D back to the measures
    setImageRaw (threadinfo->catalog, threadinfo->Ncatalog, i, raw, Nraw, MODE_MOSAIC);

    saveCenter (image, &threadinfo->Ro[i], &threadinfo->Do[i], i);
    threadinfo->mode[i] = 3;
    threadinfo->NnewFit ++;
    free (raw);
    free (ref);
  }
  
  // we should never reach here...
  return NULL;
}

// mutex to lock UpdateChips_worker operations 
static pthread_mutex_t UpdateChips_mutex = PTHREAD_MUTEX_INITIALIZER;

void lockUpdateChips () {
  pthread_mutex_lock (&UpdateChips_mutex);
}
void unlockUpdateChips () {
  pthread_mutex_unlock (&UpdateChips_mutex);
}

// we have an array of chips (image, Nimage).  we need to hand out images one at a time to
// the worker threads as they need
off_t getNextImageForThread () {

  lockUpdateChips ();
  if (nextImage >= Nimage) {
    unlockUpdateChips ();
    return (-1);
  }
  off_t thisImage = nextImage;
  nextImage ++;

  unlockUpdateChips ();
  return (thisImage);
}

// This function uses the mosaic array in MosaicOps.c to associate PHU + CHIP this is
// thread safe : the lookup (getMosaicForImage) is selecting an element of a previously
// allocated and assigned static array.  This function does NOT use the thread-unsafe (and
// somewhat slower) functions in libdvo/src/mosaic_astrom.c to associate CHIP and PHU
int saveCenter (Image *image, double *Ro, double *Do, int im) {

  Mosaic *mosaic;
  Coords *moscoords, *imcoords;
  double X, Y, L, M, P, Q, R, D;

  moscoords = NULL;
  if (!strcmp(&image[im].coords.ctype[4], "-WRP")) {
    mosaic = getMosaicForImage (im);
    if (mosaic == NULL) return FALSE;  // if we cannot find the associated image, skip it
    moscoords = &mosaic[0].coords;
  }
  imcoords = &image[im].coords;
  
  if (!strcmp(&image[im].coords.ctype[4], "-WRP")) {
    X = 0.5*image[im].NX; 
    Y = 0.5*image[im].NY;
  } else {
    X = 0.0; 
    Y = 0.0;
  }

  if (moscoords == NULL) {
    // this is a Simple image (not a mosaic)
    // note that for a Simple image, L,M = P,Q
    XY_to_LM (&L, &M, X, Y, imcoords);
    LM_to_RD (&R, &D, L, M, imcoords); // because of the block above, ctype is not -WRP 
  } else {
    XY_to_LM (&L, &M, X, Y, imcoords);
    XY_to_LM (&P, &Q, L, M, moscoords);
    LM_to_RD (&R, &D, P, Q, moscoords); // moscoords.ctype is -DIS
  }

  double Rmid = 0.5*(UserPatch.Rmin + UserPatch.Rmax);

  R = ohana_normalize_angle_to_midpoint (R, Rmid);

  *Ro = R;
  *Do = D;

  return (TRUE);
}
 
int plotChipFits (double *Ro, double *Do, char *mode, int Nimage) {

  static int kapa = -1; 

  int i, N;
  double Rmin, Rmax, Dmin, Dmax;
  float *xvec, *yvec;
  Graphdata graphdata;

  if (!relastroGetVisual()) return (TRUE);

  if (kapa == -1) {
    kapa = KapaOpenNamedSocket("kapa", "relastro");
    if (kapa == -1) {
      fprintf (stderr, "can't open kapa window\n");
      return FALSE;
    }
  }

  Rmin = +720;
  Rmax = -720;
  Dmin = +90;
  Dmax = -90;

  // find the R, D range
  for (i = 0; i < Nimage; i++) {
    if (!mode[i]) continue;

    Rmin = MIN(Rmin, Ro[i]);
    Rmax = MAX(Rmax, Ro[i]);
    Dmin = MIN(Dmin, Do[i]);
    Dmax = MAX(Dmax, Do[i]);
  }

  ALLOCATE (xvec, float, Nimage);
  ALLOCATE (yvec, float, Nimage);

  bzero (&graphdata, sizeof(Graphdata));
  plot_defaults (&graphdata);
  graphdata.xmin = Rmin;
  graphdata.xmax = Rmax;
  graphdata.ymin = Dmin;
  graphdata.ymax = Dmax;
  graphdata.style = 2;
  graphdata.size = 1;

  KapaSetFont (kapa, "helvetica", 14);
  KapaSetLimits (kapa, &graphdata);
  KapaBox (kapa, &graphdata);

  // *** good images ***
  N = 0;
  for (i = 0; i < Nimage; i++) {
    if (mode[i] != 3) continue;
    xvec[N] = Ro[i];
    yvec[N] = Do[i];
    N++;
  }
  graphdata.ptype = 7;
  graphdata.color = KapaColorByName("black");
  KapaPrepPlot (kapa, N, &graphdata);
  KapaPlotVector (kapa, N, xvec, "x");
  KapaPlotVector (kapa, N, yvec, "y");
  
  // *** reject fit ***
  N = 0;
  for (i = 0; i < Nimage; i++) {
    if (mode[i] != 1) continue;
    xvec[N] = Ro[i];
    yvec[N] = Do[i];
    N++;
  }
  graphdata.ptype = 3;
  graphdata.color = KapaColorByName("red");
  KapaPrepPlot (kapa, N, &graphdata);
  KapaPlotVector (kapa, N, xvec, "x");
  KapaPlotVector (kapa, N, yvec, "y");
  
  // *** divergent fit ***
  N = 0;
  for (i = 0; i < Nimage; i++) {
    if (mode[i] != 2) continue;
    xvec[N] = Ro[i];
    yvec[N] = Do[i];
    N++;
  }
  graphdata.ptype = 2;
  graphdata.color = KapaColorByName("blue");
  KapaPrepPlot (kapa, N, &graphdata);
  KapaPlotVector (kapa, N, xvec, "x");
  KapaPlotVector (kapa, N, yvec, "y");
  
  free (xvec);
  free (yvec);

  return (TRUE);
}

// save the coords and offset map in src at tgt
void SaveCoords (Coords *tgt, Coords *src) {
  myAssert (tgt, "oops");
  myAssert (src, "oops");
  CopyCoords (tgt, src); // src retains a pointer to the AstromOffsetMap table, to be saved
  tgt->offsetMap = AstromOffsetMapCopy(src->offsetMap); // oldCoords now saves the old values in a new structure
}

// restore the coords and offset map at tgt from src
void RestoreCoords (Coords *tgt, Coords *src, Image *image) {

  AstromOffsetMap *truMap = tgt->offsetMap;
  AstromOffsetMap *tmpMap = src->offsetMap;

  // copy the coords structure data first, then fix the offsetMap pointers
  // this modifies tgt.coords, but we need to keep the original pointer
  CopyCoords (tgt, src);
  tgt->offsetMap = truMap;

  // we want to keep the old solution, which is (maybe) a pointer to a stand-alone
  // map.  the image has a bad solution, but it is a pointer to the map in the I/O
  // table.  those values will be saved on exit.

  if (!tmpMap && !truMap) return;

  if (tmpMap && truMap) {
    AstromOffsetMapSetOrder (truMap, tmpMap->Nx, tmpMap->Ny, image);
    AstromOffsetMapCopyData (truMap, tmpMap);
    AstromOffsetMapFree (tmpMap);
    return;
  }

  if (tmpMap && !truMap) {
    // is this even possible?
    lockUpdateChips ();
    AstromOffsetTable *table = get_astrom_table ();
    AstromOffsetTableNewMap(table, tmpMap->Nx, tmpMap->Ny, image);  // registers the map with the image
    unlockUpdateChips ();
    AstromOffsetMapCopyData (image->coords.offsetMap, tmpMap);
    AstromOffsetMapFree (tmpMap);
    return;
  }
  if (!tmpMap && truMap) {
    truMap->keep = FALSE;
    return;
  }
  myAbort ("oops");
}
