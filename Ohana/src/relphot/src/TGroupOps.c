# include "relphot.h"

void assignMosaicsToTGroups (void);
int save_test_night_measures (FILE *fout, TGroup *myTGroup, Catalog *catalog);

// tgroupTimes carries the times of the tgroups (initially, just the photometric nights)
static     int  NtgroupTimes = 0;
static     int  NTGROUPTIMES = 0;
static TGTimes **tgroupTimes = NULL;

// TGroup pointers for each image
static TGroup **ImageToTGroup = NULL;

// elsewhere, we have loaded a set of catalogs with measures (catalog[cat].measure[meas])

// TGroup pointers for each measure,catalog set
static TGroup  ***MeasureToTGroup = NULL;

void sort_times (unsigned int *T, int N);

void sort_tgtimes (TGTimes **T, int N) {

# define SWAPFUNC(A,B){ TGTimes *tmp;		\
    tmp = T[A]; T[A] = T[B]; T[B] = tmp;	\
  }
# define COMPARE(A,B)(T[A][0].start < T[B][0].start)

  OHANA_SORT (N, COMPARE, SWAPFUNC);

# undef SWAPFUNC
# undef COMPARE

}

// load the photometric nights from a file : these will be used to define the tgroups 
// this is called in args.c after TGROUP_ZEROPT is set
void loadTGroups (char *filename) {

  if (!TGROUP_ZEROPT) return;

  // read start times from a file (currently a list of MJD values)
  FILE *f = fopen (filename, "r");
  if (!f) {
    fprintf (stderr, "tgroup file %s not found\n", filename);
    exit (4);
  }
  // ReadTGroupFile returns a list of the time values (int)
  unsigned int *tgroupTimesRaw = ReadTGroupFile (f, &NtgroupTimes);
  if (!tgroupTimesRaw) {
    fprintf (stderr, "error reading tgroup file %s\n", filename);
    exit (4);
  }
  fprintf (stderr, "loaded %d tgroups from file %s\n", NtgroupTimes, filename);

  // sort the times for quick bisection
  sort_times (tgroupTimesRaw, NtgroupTimes);

  // we first define the collection of times (TGTimes) then for each time, we define a tgroup for
  // each photcode being considered below, if there are no images in this tgroup with a
  // given photcode, it is left empty (no images)

  NTGROUPTIMES = NtgroupTimes; // if we have loaded a list of times, use that as the starting alloc number
  ALLOCATE (tgroupTimes, TGTimes *, NTGROUPTIMES);

  // the start of an MJD day is 14:00 HST, so the end time can be the start time + 23h 59m 59s (86399s)
  // XXX NOTE that this is HST-centric

  for (int i = 0; i < NtgroupTimes; i++) {
    ALLOCATE (tgroupTimes[i], TGTimes, 1);
    tgroupTimes[i][0].start = tgroupTimesRaw[i];
    tgroupTimes[i][0].stop  = tgroupTimesRaw[i] + 86399;

    // One TGroup per *active* photcode.  Check if photcode is active with:
    // GetActivePhotcodeIndex (photcode) (returns -1 if not active)

    ALLOCATE (tgroupTimes[i][0].byCode, TGroup, Nphotcodes);
    tgroupTimes[i][0].nCode = Nphotcodes;
    TGroup *tgroup = tgroupTimes[i][0].byCode;

    for (int j = 0; j < Nphotcodes; j++) {
      tgroup[j].parent    = (void *) tgroupTimes[i];
      tgroup[j].McalPSF   = 0.0; // note : at the end, tgroup.Mcal is added back to the input images
      tgroup[j].McalAPER  = 0.0; // note : tgroup stores only offsets relative to the original image values
      tgroup[j].dKlam     = 0.0; 
      tgroup[j].dMcal     = 0.0; // note : at the end, tgroup.Mcal is added back to the input images
      tgroup[j].dMsys     = 0.0;
      tgroup[j].dMmin     = NAN;
      tgroup[j].dMmax     = NAN;
      tgroup[j].McalChiSq = 0.0; // NAN or 0.0?

      tgroup[j].stdev      = NAN;
      tgroup[j].nFitPhotom = 0;
      tgroup[j].nValPhotom = 0;

      tgroup[j].flags     = ID_IMAGE_PHOTOM_UBERCAL; // we start by treating the suspected photometric nights as photometric
      tgroup[j].photcode  = photcodes[j][0].code;
    
      tgroup[j].NIMAGE    = 1000;
      tgroup[j].Nimage    = 0;
      ALLOCATE (tgroup[j].image, off_t, tgroup[j].NIMAGE);
      tgroup[j].image[0]  = -1;

      tgroup[j].NMOSAIC   = 100;
      tgroup[j].Nmosaic   = 0;
      ALLOCATE (tgroup[j].mosaic, Mosaic *, tgroup[j].NMOSAIC);

      tgroup[j].Nmeasure  = 0;
      tgroup[j].NMEASURE  = 1000;
      ALLOCATE (tgroup[j].measure, off_t, tgroup[j].NMEASURE);
      ALLOCATE (tgroup[j].catalog, off_t, tgroup[j].NMEASURE);
    }
  }

  free (tgroupTimesRaw);
  return;
}

// create a new TGroup based on the supplied time and photcode
void extendTGroups (unsigned int tzero) {

  if (!TGROUP_ZEROPT) return;

  // index of the new entry
  int Ntgt = NtgroupTimes;
  NtgroupTimes ++;

  if (NtgroupTimes >= NTGROUPTIMES) {
    NTGROUPTIMES += 100;
    REALLOCATE (tgroupTimes, TGTimes *, NTGROUPTIMES);
  }
  
  double mjd = ohana_sec_to_mjd (tzero);
  double mjdInt = (int) mjd;
  time_t start = ohana_mjd_to_sec (mjdInt);

  ALLOCATE (tgroupTimes[Ntgt], TGTimes, 1);
  tgroupTimes[Ntgt][0].start = start;
  tgroupTimes[Ntgt][0].stop  = start + 86399;

  ALLOCATE (tgroupTimes[Ntgt][0].byCode, TGroup, Nphotcodes);
  tgroupTimes[Ntgt][0].nCode = Nphotcodes;
  TGroup *tgroup = tgroupTimes[Ntgt][0].byCode;

  for (int j = 0; j < Nphotcodes; j++) {
    tgroup[j].parent    = (void *) tgroupTimes[Ntgt];
    tgroup[j].McalPSF   = 0.0; // note : at the end, tgroup.Mcal is added back to the input images
    tgroup[j].McalAPER  = 0.0; // note : tgroup stores only offsets relative to the original image values
    tgroup[j].dKlam     = 0.0; 
    tgroup[j].dMcal     = 0.0; // note : at the end, tgroup.Mcal is added back to the input images
    tgroup[j].dMsys     = 0.0;
    tgroup[j].dMmin     = NAN;
    tgroup[j].dMmax     = NAN;
    tgroup[j].McalChiSq = 0.0; // NAN or 0.0?

    tgroup[j].stdev      = NAN;
    tgroup[j].nFitPhotom = 0;
    tgroup[j].nValPhotom = 0;

    tgroup[j].flags     = 0;   // tgroups NOT in the original file are not suspected as photometric
    tgroup[j].photcode  = photcodes[j][0].code;

    tgroup[j].NIMAGE    = 1000;
    tgroup[j].Nimage    = 0;   // the new TGroup is not suspected to be photometric
    ALLOCATE (tgroup[j].image, off_t, tgroup[j].NIMAGE);
    tgroup[j].image[0]  = -1;

    tgroup[j].NMOSAIC   = 100;
    tgroup[j].Nmosaic   = 0;
    ALLOCATE (tgroup[j].mosaic, Mosaic *, tgroup[j].NMOSAIC);

    tgroup[j].Nmeasure  = 0;
    tgroup[j].NMEASURE  = 1000;
    ALLOCATE (tgroup[j].measure, off_t, tgroup[j].NMEASURE);
    ALLOCATE (tgroup[j].catalog, off_t, tgroup[j].NMEASURE);
  }

  // sort the times for quick bisection
  sort_tgtimes (tgroupTimes, NtgroupTimes);
  return;
}

// Assign the images to the tgroups.  The initial tgroups table contains only suspected
// photometric nights.  As we find images which are not in the current tgroups list, add
// new non-photometric entries.
void initTGroups (Image *subset, off_t Nsubset) {

  if (!TGROUP_ZEROPT) return;

  /* a 'tgroup' in relphot is a virtual concept: there is no 
   * entry in the image table that represents this tgroup.  Instead, it is an
   * internal construct that defines a group of related images 
   */

  if (!tgroupTimes) {
    NTGROUPTIMES = 100; 
    ALLOCATE (tgroupTimes, TGTimes *, NTGROUPTIMES);
  } 

  ALLOCATE (ImageToTGroup, TGroup *, Nsubset); // tgroup to which image belongs

  // assign each image to a tgroup
  int Nsimple = 0;
  for (off_t i = 0; i < Nsubset; i++) {
    ImageToTGroup[i] = NULL;

    // ignore mosaic images (photcode == 0)
    if (!subset[i].photcode) {
      continue;
    }

    // skip inactive photcodes (do not assign to a TGroup)
    if (GetActivePhotcodeIndex (subset[i].photcode) < 0) continue;
    
    TGroup *myGroup = findTGroup(subset[i].tzero, subset[i].photcode);
    if (!myGroup) {
      // generate a new TGroup which is not photometric
      extendTGroups (subset[i].tzero);
      myGroup = findTGroup(subset[i].tzero, subset[i].photcode);
      myAssert (myGroup, "oops, we just extended to include this");
    }

    // add pointer to tgroup for this image
    ImageToTGroup[i] = myGroup;

    // note the array 'subset' is registered in ImageOps.c:images by initImages
    // and can be retrieve with getimages

    // add image to tgroup image list
    myGroup[0].image[myGroup[0].Nimage] = i; // reference to entry in 'subset'
    myGroup[0].Nimage ++;
    if (myGroup[0].Nimage == myGroup[0].NIMAGE) {
      myGroup[0].NIMAGE += 1000;
      REALLOCATE (myGroup[0].image, off_t, myGroup[0].NIMAGE);
    }
  }

  // assign the mosaics to each tgroup -- we do this by image
  assignMosaicsToTGroups ();

  initTGroupsMcal ();

  fprintf (stderr, "matched %d images to %d tgroups, %d simple chips not matched to tgroups\n", (int) (Nsubset - Nsimple), (int) NtgroupTimes, (int) Nsimple);
  return;
}

void freeTGroups (void) {

  if (!TGROUP_ZEROPT) return;

  free (ImageToTGroup);

  for (int i = 0; i < NtgroupTimes; i++) {
    TGroup *tgroup = tgroupTimes[i][0].byCode;
    for (int j = 0; j < tgroupTimes[i][0].nCode; j++) {
      free (tgroup[j].image);
      free (tgroup[j].mosaic);
      free (tgroup[j].measure);
      free (tgroup[j].catalog);
    }
    free (tgroup);
    free (tgroupTimes[i]);
  }
  free (tgroupTimes);

  // free (ImageToGroup); why not?
  return;
}

void initTGroupsMcal (void) {

  off_t Nimages;
  off_t *LineNumber;
  Image *images = getimages (&Nimages, &LineNumber);

  if (!TGROUP_ZEROPT) return;

  // we use liststats to calculate the median Mcal for the input images
  // in each tgroup as a starting point.
  StatType stats;
  liststats_setmode (&stats, "MEDIAN");

  // init the tgroup array values
  for (int i = 0; i < NtgroupTimes; i++) {
    TGroup *tgroup = tgroupTimes[i][0].byCode;
    for (int j = 0; j < tgroupTimes[i][0].nCode; j++) {
      tgroup[j].McalPSF   = 0.0; // note : at the end, tgroup.Mcal is added back to the input images
      tgroup[j].McalAPER  = 0.0; // note : tgroup stores only offsets relative to the original image values
      tgroup[j].dMcal     = 0.0; // note : at the end, tgroup.Mcal is added back to the input images
      tgroup[j].dMsys     = 0.0;
      tgroup[j].McalChiSq = 0.0;// NAN or 0.0?

      tgroup[j].stdev      = NAN;
      tgroup[j].nFitPhotom = 0;
      tgroup[j].nValPhotom = 0;

      if (tgroup[j].Nimage == 0) continue; // no images, ignore the tgroup

      // calculate the median of the Mcal values for the images in this tgroup:

      ALLOCATE_PTR (McalPSF,  double, tgroup[j].Nimage);
      ALLOCATE_PTR (McalAPER, double, tgroup[j].Nimage);
      for (int im = 0; im < tgroup[j].Nimage; im++) {
	int Nsub = tgroup[j].image[im];
	McalPSF[im] = images[Nsub].McalPSF;
	McalAPER[im] = images[Nsub].McalAPER;

	// set these back to 0.0 to have no future effect
	images[Nsub].McalPSF = 0.0;
	images[Nsub].McalAPER = 0.0;
      }
    
      liststats (McalPSF, NULL, NULL, tgroup[j].Nimage, &stats);
      tgroup[j].McalPSF = stats.median;
      FREE (McalPSF);
  
      liststats (McalAPER, NULL, NULL, tgroup[j].Nimage, &stats);
      tgroup[j].McalAPER = stats.median;
      FREE (McalAPER);
    }
  }
  return;
}

void assignMosaicsToTGroups (void) {

  if (!TGROUP_ZEROPT) return;

  // init the tgroup array values
  for (int i = 0; i < NtgroupTimes; i++) {
    TGroup *tgroup = tgroupTimes[i][0].byCode;
    for (int j = 0; j < tgroupTimes[i][0].nCode; j++) {

      if (tgroup[j].Nimage == 0) continue; // no images, ignore the tgroup

      // find the mosaics by image (if MOSAIC_ZEROPT is active)
      for (int im = 0; MOSAIC_ZEROPT && (im < tgroup[j].Nimage); im++) {
	int imageIdx = tgroup[j].image[im];
	Mosaic *mosaic = getMosaicForImage (imageIdx);

	if (!mosaic) continue; // this is a simple chip, not part of a mosaic
	if (mosaic->inTGroup) continue;

	// mosaic to tgroup and extend as needed
	tgroup[j].mosaic[tgroup[j].Nmosaic] = mosaic;
	mosaic->inTGroup = TRUE;

	// advance array and extend if needed
	tgroup[j].Nmosaic ++;
	if (tgroup[j].Nmosaic >= tgroup[j].NMOSAIC) {
	  tgroup[j].NMOSAIC += 100;
	  REALLOCATE (tgroup[j].mosaic, Mosaic *, tgroup[j].NMOSAIC);
	}
      }
    }
  }
  return;
}

TGroup *getTGroupForImage (off_t im) {

  if (im < 0) return NULL;
  if (!ImageToTGroup) return NULL;

  // test if im > Nimages / Nsubset?
  TGroup *myGroup = ImageToTGroup[im];
  return (myGroup);
}

// use bisection to find the overlapping tgroup (returns exact match)
// tgroupTimes is a sorted, unique list of times
// assume a 1day range for now
TGroup *findTGroup (unsigned int start, int photcode) {

  if (!NtgroupTimes) return NULL; // if none are yet defined, do not even try

  // find index of matching photcode from our list of active secondary photcodes
  int Ns = GetActivePhotcodeIndex (photcode);
  if (Ns < 0) return NULL;

  off_t Nlo, Nhi, N;

  // find the last tgroup before start
  Nlo = 0; // first valid tgroupTimes value
  Nhi = NtgroupTimes - 1; // last valid tgroupTimes value

  // if start is not in this range, return NULL
  if (start < tgroupTimes[Nlo][0].start) return NULL;
  if (start > tgroupTimes[Nhi][0].start + 86399) return NULL; // 1-day range for tgroup for now

  while (Nhi - Nlo > 4) {
    N = 0.5*(Nlo + Nhi);
    if (tgroupTimes[N][0].start < start) {
      Nlo = MAX(N, 0);
    } else {
      Nhi = MIN(N, NtgroupTimes - 1);
    }
  }
  // we now have : tgroupTimes[Nlo] < start <= tgroupTimes[Nhi]

  // find a matched tgroup starting from Nlo, or return NULL
  // tgroupTimes is a lower bound, the upper bound is tgroupTimes + 86399 (1d)
  for (N = Nlo; N <= Nhi; N++) { 
    if (start < tgroupTimes[N][0].start) continue;
    if (start > tgroupTimes[N][0].start + 86399) continue; // XXX since they are sorted, probably do not need this check
    // N is the tgroupTimes entry we want.
    // double-check that tgroupTimes[N][0].byCode[Ns].photcode = ecode?
    return &tgroupTimes[N][0].byCode[Ns];
  }
  return (NULL);
}

void setMcalFromTGroups () {

  off_t Nimage;
  Image *image;

  if (!TGROUP_ZEROPT) return;

  image = getimages (&Nimage, NULL);

  fprintf (stderr, "*** return Mcal from tgroup.Mcal to image.Mcal ***\n");

  // NOTE: nights which are poor should have had Mcal set to 0, making this a NOP for those nights

  // copy the tgroup results to the images.  set the tgroup Mcal to 0.0 since we have moved its
  // impact to the images
  for (off_t k = 0; k < NtgroupTimes; k++) {
    TGroup *tgroup = tgroupTimes[k][0].byCode;
    for (off_t i = 0; i < tgroupTimes[k][0].nCode; i++) {
      for (off_t j = 0; j < tgroup[i].Nimage; j++) {
	off_t im = tgroup[i].image[j];
	double Mgrp = (TGROUP_FIT_AIRMASS) ? tgroup[i].McalPSF + tgroup[i].dKlam*(image[im].secz - 1.0) : tgroup[i].McalPSF;
	if (tgroup[i].flags & ID_IMAGE_TGROUP_PHOTCAL) {
	  image[im].McalPSF     = Mgrp;
	  image[im].McalAPER    = Mgrp;
	  image[im].dMcal       = tgroup[i].dMcal;
	  image[im].McalChiSq   = tgroup[i].McalChiSq;
	  image[im].dMagSys     = tgroup[i].dMsys;
	  image[im].nFitPhotom  = tgroup[i].nFitPhotom;
	}
	image[im].flags 	   |= (tgroup[i].flags & ID_IMAGE_NIGHT_POOR); // probably not necessary : it is set in markBadTGroup
	image[im].flags 	   |= (tgroup[i].flags & ID_IMAGE_MOSAIC_POOR); // probably not necessary : it is set in markBadTGroup
	image[im].flags 	   |= (tgroup[i].flags & ID_IMAGE_PHOTOM_FEW);
	image[im].flags 	   |= (tgroup[i].flags & ID_IMAGE_PHOTOM_POOR);
	image[im].flags 	   |= (tgroup[i].flags & ID_IMAGE_PHOTOM_UBERCAL);
	image[im].flags 	   |= (tgroup[i].flags & ID_IMAGE_TGROUP_PHOTCAL);
	image[im].flags 	   |= (tgroup[i].flags & ID_IMAGE_MOSAIC_PHOTCAL);
	image[im].flags 	   |= (tgroup[i].flags & ID_IMAGE_IMAGE_PHOTCAL);
	
	// fprintf (stderr, "TG to IMAGE: %f +/- %f -> %f (%d)\n", tgroup[i].McalPSF, tgroup[i].dMcal, image[im].McalPSF, tgroup[i].nFitPhotom);
      }
      tgroup[i].McalPSF  = 0.0;
      tgroup[i].McalAPER = 0.0;
      tgroup[i].dKlam    = 0.0;
    }
  }      
}

void markBadTGroup (TGroup *tgroup) {

  off_t Nimage;
  Image *image;

  // this tgroup has been identified as poor.
  tgroup->flags |= ID_IMAGE_NIGHT_POOR;

  // all associated mosaics should be marked as coming from a bad night
  // these will be fitted independently.  Make the starting solution consistent by
  // setting the mosaic McalPSF for the current best fit from the tgroup

  for (off_t j = 0; j < tgroup->Nmosaic; j++) {
    Mosaic *mos = tgroup->mosaic[j];
    mos->flags |= ID_IMAGE_NIGHT_POOR;
    mos->McalPSF = (TGROUP_FIT_AIRMASS) ? tgroup->McalPSF + tgroup->dKlam*(mos->secz - 1.0) : tgroup->McalPSF;
    mos->McalAPER = (TGROUP_FIT_AIRMASS) ? tgroup->McalAPER + tgroup->dKlam*(mos->secz - 1.0) : tgroup->McalAPER;
  }

  // all associated images should be marked as coming from a bad night
  // these will be fitted independently
  image = getimages (&Nimage, NULL);
  for (off_t j = 0; j < tgroup->Nimage; j++) {
    off_t im = tgroup->image[j];
    image[im].flags |= ID_IMAGE_NIGHT_POOR;
  }

  // reset the tgroup to have no impact on the solution 
  tgroup->McalPSF  = 0.0;
  tgroup->McalAPER = 0.0;
  tgroup->dKlam    = 0.0;
}

// for now, we do not allow a night to be redeemed, so this
// operation should have no impact.
void markGoodTGroup (TGroup *tgroup) {

  off_t Nimage;
  Image *image;

  // this tgroup has been identified as good.
  tgroup->flags &= ~ID_IMAGE_NIGHT_POOR;

  // all associated mosaics should be marked as NOT coming from a bad night
  for (off_t j = 0; j < tgroup->Nmosaic; j++) {
    tgroup->mosaic[j]->flags &= ~ID_IMAGE_NIGHT_POOR;
  }

  // all associated images should be marked as NOT coming from a bad night
  image = getimages (&Nimage, NULL);
  for (off_t j = 0; j < tgroup->Nimage; j++) {
    off_t im = tgroup->image[j];
    image[im].flags &= ~ID_IMAGE_NIGHT_POOR;
  }
}

void initTGroupBins (Catalog *catalog, int Ncatalog) {

  off_t i, j;

  /* measure -> tgroup */
  if (!TGROUP_ZEROPT) return;

  ALLOCATE (MeasureToTGroup, TGroup **, Ncatalog);
  for (i = 0; i < Ncatalog; i++) {
    ALLOCATE (MeasureToTGroup[i], TGroup *, MAX (catalog[i].Nmeasure, 1));
    for (j = 0; j < catalog[i].Nmeasure; j++) MeasureToTGroup[i][j] = NULL;
  }
}

void freeTGroupBins (int Ncatalog) {

  off_t i;

  /* measure -> tgroup */
  if (!TGROUP_ZEROPT) return;

  for (i = 0; i < Ncatalog; i++) {
    free (MeasureToTGroup[i]);
  }
  free (MeasureToTGroup);
}

int findTGroups (Catalog *catalog, int Ncatalog) {
  
  if (!TGROUP_ZEROPT) return (FALSE);
  // if we are calibrating by tgroup, redefine myDet == on one of my tgroups

  int Nmatch = 0;
  for (int i = 0; i < Ncatalog; i++) {
    for (off_t j = 0; j < catalog[i].Nmeasure; j++) {
      catalog[i].measureT[j].myDet = FALSE; // a detetion is not mine until proven otherwise

      if (TimeSelect) {
	if (catalog[i].measureT[j].t < TSTART) continue;
	if (catalog[i].measureT[j].t > TSTOP) continue;
      }

      // check if this measure has a relevant photcode 
      int Ns = GetActivePhotcodeIndex (catalog[i].measureT[j].photcode);
      if (Ns < 0) continue;

      matchTGroups (catalog, j, i);
      Nmatch ++;
    }
  }
  fprintf (stderr, "Matched %d detections to tgroups\n", Nmatch);

  return (TRUE);
}

void matchTGroups (Catalog *catalog, off_t meas, int cat) {

  MeasureTiny *measure = &catalog[cat].measureT[meas];

  off_t ID = measure[0].imageID; // ID is a unique ID in DVO
  off_t idx = getImageByID (ID); // idx is the sequence number in the subset list
  if (idx == -1) {
    if (VERBOSE2) fprintf (stderr, "missed measurement "OFF_T_FMT", %d\n", meas, cat);
    return;
  }

  TGroup *myGroup = ImageToTGroup[idx];
  if (!myGroup) return;

  // this measurement is on one of my tgroups, mark it as mine.
  catalog[cat].measureT[meas].myDet = TRUE;
  MeasureToTGroup[cat][meas] = myGroup;

  myGroup->catalog[myGroup->Nmeasure] = cat;
  myGroup->measure[myGroup->Nmeasure] = meas;
  myGroup->Nmeasure ++;

  if (myGroup->Nmeasure == myGroup->NMEASURE) {
    myGroup->NMEASURE += 1000;
    REALLOCATE (myGroup->catalog, off_t, myGroup->NMEASURE);
    REALLOCATE (myGroup->measure, off_t, myGroup->NMEASURE);
  }
  return;
}

// dZpt (if supplied) returns the error on Mgrp
float getMgrp (off_t meas, int cat, float airmass, float *dZpt) {

  if (dZpt) { *dZpt = 0; }

  if (!TGROUP_ZEROPT) return 0.0;

  // unassigned measurements belong to simple chips
  TGroup *myGroup = MeasureToTGroup[cat][meas];
  if (!myGroup) return 0.0;

  float value = myGroup->McalPSF + myGroup->dKlam*(airmass - 1.0);
  if (dZpt) { *dZpt = myGroup->dMcal; }
  return value;
}

int getTGroupFlags (off_t meas, int cat) {

  if (!TGROUP_ZEROPT) return (0);

  // unassigned measurements belong to simple chips
  TGroup *myGroup = MeasureToTGroup[cat][meas];
  if (!myGroup) return (0);

  return (myGroup->flags);
}

typedef struct {
  int Nfew;
  int Nbad;
  int Ncal;
  int Nmos;
  int Ngrid;
  int Nrel;
  int Nsys;
  int Nskip;
  off_t Nmax;
  FitDataSet psfStars;
  FitDataSet kronStars;
  FitDataSet brightStars;
} SetMgrpInfo;

enum {THREAD_RUN, THREAD_DONE};

typedef struct {
  int entry;
  int state;
  Catalog *catalog;
  Image *image;
  SetMgrpInfo info;
} ThreadInfo;

int   setMgrp_tgroup (TGroup *tgroup, off_t Nmos, Image *image, Catalog *catalog, SetMgrpInfo *info);
void *setMgrp_worker (void *data);
int   setMgrp_threaded (Catalog *catalog);

void SetMgrpInfoInit (SetMgrpInfo *info, off_t Nmax, int allocLists) {
  info->Nfew = 0;
  info->Nbad = 0;
  info->Ncal = 0;
  info->Nmos = 0;
  info->Nrel = 0;
  info->Ngrid = 0;
  info->Nskip = 0;
  info->Nsys = 0;

  info->Nmax = Nmax;

  if (allocLists) {
    int fitOrder = TGROUP_FIT_AIRMASS ? 1 : 0;
    
    FitDataSetAlloc (&info->psfStars,    Nmax, fitOrder, 0); 
    FitDataSetAlloc (&info->kronStars,   Nmax, fitOrder, 0); 
    FitDataSetAlloc (&info->brightStars, Nmax, fitOrder, 0); 

    // until the analysis has converged a bit, do not use the IRLS analysis
    if (UseStandardOLS(ZPT_TGROUP)) {
      info->brightStars.MaxIterations = 0;
      info->kronStars.MaxIterations = 0;
      info->psfStars.MaxIterations = 0;
    }
    FitDataSetAddPriors (&info->psfStars); // why only psfStars?
    if (TGROUP_FIT_AIRMASS) {
      info->psfStars.bPriorValue[1] = 0.0;  // note that we are fitting relative to the nominal slope
      info->psfStars.bPriorSigma[1] = 0.04; // XXX this prior sigma needs to be user-configured
    }
  }
}

void SetMgrpInfoFree (SetMgrpInfo *info) {
  FitDataSetFree (&info->brightStars);
  FitDataSetFree (&info->kronStars);
  FitDataSetFree (&info->psfStars);
}

void SetMgrpInfoAccum (SetMgrpInfo *summary, SetMgrpInfo *results) {
  summary->Nfew  += results->Nfew ;
  summary->Nsys  += results->Nsys ;
  summary->Nbad  += results->Nbad ;
  summary->Ncal  += results->Ncal ;
  summary->Nmos  += results->Nmos ;
  summary->Nrel  += results->Nrel ;
  summary->Ngrid += results->Ngrid;
  summary->Nskip += results->Nskip;
}

static int npass_output = 0;

// mutex to lock setMgrp_worker operations 
static pthread_mutex_t setMgrp_mutex = PTHREAD_MUTEX_INITIALIZER;
static int nextTGroup = 0;

// we have an array of (tgroupTimes, NtgroupTimes).  we need to hand out tgroupTimes one at a time to
// the worker threads as they need
off_t getNextTGroupForThread () {

  pthread_mutex_lock (&setMgrp_mutex);
  if (nextTGroup >= NtgroupTimes) {
    pthread_mutex_unlock (&setMgrp_mutex);
    return (-1);
  }
  int thisTGroup = nextTGroup;
  nextTGroup ++;

  pthread_mutex_unlock (&setMgrp_mutex);
  return (thisTGroup);
}

int setMgrp (Catalog *catalog) {

  off_t N;
  Image *image;

  if (!TGROUP_ZEROPT) return (FALSE);
  if (TGROUP_ZPT_MODE == TGROUP_ZPT_MODE_NONE) return (FALSE);

  // plots cannot be done in a threaded context (the plot commands collide)
  // so do not run setMgrp in threaded mode if PLOTSTUFF is set
  if (NTHREADS && !PLOTSTUFF) {
    int status = setMgrp_threaded (catalog);
    return status;
  }

  image = getimages (&N, NULL);

  off_t Nmax = 0;
  for (off_t i = 0; i < NtgroupTimes; i++) {
    TGroup *tgroup = tgroupTimes[i][0].byCode;
    for (off_t j = 0; j < tgroupTimes[i][0].nCode; j++) {
      Nmax = MAX (Nmax, tgroup[j].Nmeasure);
    }
  }

  SetMgrpInfo info;
  SetMgrpInfoInit (&info, Nmax, TRUE);

  for (off_t i = 0; i < NtgroupTimes; i++) {
    TGroup *tgroup = tgroupTimes[i][0].byCode;
    for (off_t j = 0; j < tgroupTimes[i][0].nCode; j++) {
      setMgrp_tgroup (&tgroup[j], i, image, catalog, &info);
      // fprintf (stderr, "TGROUP time %.0f photcode %d Mcal: %f, dK: %f, dMcal: %f, sigma: %f, chisq: %f, %d of %d, limiting negative clouds to %f\n",
      // ohana_sec_to_mjd(tgroupTimes[i][0].start), tgroup[j].photcode, tgroup[j].McalPSF, tgroup[j].dKlam, tgroup[j].dMcal, tgroup[j].stdev, tgroup[j].McalChiSq, tgroup[j].nFitPhotom, tgroup[j].nValPhotom, CLOUD_TOLERANCE);
    }
  }
  SetMgrpInfoFree (&info);

  if (PLOTSTUFF) {
    fprintf (stdout, "press return\n"); 
    if (fscanf (stdin, "%*c") != 1) fprintf (stderr, "\n");
  }

  npass_output ++;

  fprintf (stderr, "%d tgroups marked having too few measurements (Nbad: %d, Ncal: %d, Ngrid: %d, Nrel: %d, Nsys: %d)\n", info.Nfew, info.Nbad, info.Ncal, info.Ngrid, info.Nrel, info.Nsys);

  return (TRUE);
}
  
# define NIGHT_TOOFEW 100
# define NIGHT_GOOD_FRACTION 0.1

// 'tgroup' is a pointer to the current tgroup of interest (entry Ngrp)
int setMgrp_tgroup (TGroup *myTGroup, off_t Ngrp, Image *image, Catalog *catalog, SetMgrpInfo *info) {

  off_t j;

  // calculate the statistics for both good and bad nights, but only set Mcal for the good nights
  // we only skip the statistics for nights with too few measurements or exposures

  FitDataSet *psfStars = &info->psfStars;
  FitDataSet *kronStars = &info->kronStars;
  FitDataSet *brightStars = &info->brightStars;

  assert (Ngrp >= 0);
  assert (Ngrp < NtgroupTimes);

  int Nsecfilt = GetPhotcodeNsecfilt ();

  TGTimes *tgroup = (TGTimes *) myTGroup->parent;
  double mjdStart = ohana_sec_to_mjd (tgroup->start);

  // unset this flag at start (set below if zeropoint is calculated)
  myTGroup->flags &= ~ID_IMAGE_TGROUP_PHOTCAL; 
  
  // in clean_tgroups (run on each iteration), we identify good and bad tgroups.  We mark
  // bad tgroups and do not save a zero point for them. Instead we will fit those
  // exposures or images separately
  int useMgrp = TRUE;  
  if (TGROUP_ZPT_MODE == TGROUP_ZPT_MODE_GOOD_NIGHT) {
    if (myTGroup->flags & ID_IMAGE_NIGHT_POOR) useMgrp = FALSE;
  }

  // for testing, supply the MJD of a night or nights here to dump the full list of measurements
  int testImage = FALSE;
  // testImage |= (abs(mjdStart - 56586) < 0.1);
  // testImage |= (abs(mjdStart - 57974) < 0.1);
  // testImage |= (abs(mjdStart - 57975) < 0.1);
  // testImage |= (abs(mjdStart - 57977) < 0.1);
  // testImage |= (abs(mjdStart - 57981) < 0.1);
  // testImage |= (abs(mjdStart - 58710) < 0.1);

  FILE *fout = NULL;
  if (testImage) {
    char filename[64];
    snprintf (filename, 64, "test.%05d.%02d.dat", (int) Ngrp, npass_output);
    fout = fopen (filename, "w");
    fprintf (fout, "# tgroup %s (%.0f)\n", ohana_sec_to_date(tgroup->start), mjdStart);
    save_test_night_measures (fout, myTGroup, catalog);
    fclose (fout);
  }

  // number of stars to measure the bright-end scatter
  int Nbright = 0;

  int N = 0;
  for (j = 0; j < myTGroup->Nmeasure; j++) {
      
    off_t m = myTGroup->measure[j];
    off_t c = myTGroup->catalog[j];
      
    if (catalog[c].measureT[m].dbFlags & MEAS_BAD) {
      info->Nbad ++;
      continue;
    }
    float Mcal = getMcal  (m, c, MAG_CLASS_PSF);
    if (isnan(Mcal)) {
      info->Ncal++;
      continue;
    }
    float Mmos = getMmos  (m, c);
    if (isnan(Mmos)) {
      info->Nmos++;
      continue;
    }
    float Mgrid = getMgridTiny (&catalog[c].measureT[m]);
    if (isnan(Mgrid)) {
      info->Ngrid ++;
      continue;
    }

    // Mrel* is the average magnitude for this star.  For PS1 stacks, we have too much
    // PSF variability.  We need to calibrate the PSF magnitudes separately from the
    // Aperture-like magnitues.  (We have an option to use the kron magnitudes or the
    // other apertures here).  I basically need to do this analysis separately for each
    // magnitude type
    
    float MrelPSF = getMrel  (catalog, m, c, MAG_CLASS_PSF, MAG_SRC_CHP);
    if (isnan(MrelPSF)) {
      info->Nrel ++;
      continue;
    }
    float MrelKron = getMrel  (catalog, m, c, MAG_CLASS_KRON, MAG_SRC_CHP);
      
    // image.Mcal is not supposed to include the flat-field correction, so we need to
    // apply that offset as well here for this image (in other words, each detection is
    // being compared to the model, excluding the zero point, Mcal.  The model includes
    // the flat-correction.  NOTE the sign of Mflat (Image.Mcal = Measure.Mcal + Mflat)
    // this was inconsistent with PhotRel pre-r41606

    float Mflat = getMflat (m, c, catalog);

    off_t n = catalog[c].measureT[m].averef;
    float MsysPSF = PhotSysTiny (&catalog[c].measureT[m], &catalog[c].averageT[n], &catalog[c].secfilt[n*Nsecfilt], MAG_CLASS_PSF);
    if (isnan(MsysPSF)) {
      info->Nsys++;
      continue;
    }
    float MsysKron = PhotSysTiny (&catalog[c].measureT[m], &catalog[c].averageT[n], &catalog[c].secfilt[n*Nsecfilt], MAG_CLASS_KRON);

    // here we are only checking for equiv photcodes (not active photcodes)
    PhotCode *code = GetPhotcodebyCode (catalog[c].measureT[m].photcode);
    if (!code) goto skip;
    if (code->equiv < 1) goto skip;
    int Nsec = GetPhotcodeNsec (code->equiv);
    if (Nsec == -1) goto skip;

  skip:
    assert (N < info->Nmax);
    assert (N >= 0);
    assert (Nbright < info->Nmax);
    assert (Nbright >= 0);

    float Moff =  Mcal + Mmos + Mgrid + Mflat;

    psfStars->alldata-> yVector[N] = MsysPSF - MrelPSF - Moff;
    psfStars->alldata->dyVector[N] = MAX (catalog[c].measureT[m].dM, MIN_ERROR);

    kronStars->alldata-> yVector[N] = MsysKron - MrelKron - Moff;
    kronStars->alldata->dyVector[N] = psfStars->alldata->dyVector[N];

    if (TGROUP_FIT_AIRMASS) {
      psfStars->alldata-> xVector[N] = (catalog[c].measureT[m].airmass - 1.0);
      kronStars->alldata-> xVector[N] = psfStars->alldata-> xVector[N];
    }

    if (catalog[c].measureT[m].dM < IMFIT_SYS_SIGMA_LIM) {
      brightStars->alldata-> yVector[Nbright] = psfStars->alldata-> yVector[N];
      brightStars->alldata->dyVector[Nbright] = psfStars->alldata->dyVector[N];
      if (TGROUP_FIT_AIRMASS) {
	brightStars->alldata-> xVector[Nbright] = psfStars->alldata-> xVector[N];
      }
      Nbright ++;
    }
    N++;
  }

  /* skip nights with too few good measurements or too many bad measurements */
  int mark = (N < NIGHT_TOOFEW) || (N < NIGHT_GOOD_FRACTION*myTGroup->Nmeasure);
  if (mark) {
    myTGroup->flags |= ID_IMAGE_PHOTOM_FEW;
    myTGroup->McalPSF    = 0.0;
    myTGroup->McalAPER   = 0.0;
    myTGroup->dMcal      = NAN;
    myTGroup->stdev      = NAN;
    myTGroup->dMmin      = NAN;
    myTGroup->dMmax      = NAN;
    myTGroup->McalChiSq  = NAN;
    myTGroup->nFitPhotom = 0;
    myTGroup->nValPhotom = N;
    myTGroup->dKlam      = (TGROUP_FIT_AIRMASS) ? psfStars->bSaveArray[1][0] : 0;
    info->Nfew ++;
    if (testImage) {
      fprintf (stderr, "NOTE: *** marked test image poor : %d %d %d***\n", (int) N, (int) NIGHT_TOOFEW, (int) (NIGHT_GOOD_FRACTION*myTGroup->Nmeasure));
    }
    return TRUE; // skip nights with too few good measurements
  } else {
    myTGroup->flags &= ~ID_IMAGE_PHOTOM_FEW;
  }

  /* we have a non-trivial fraction of measurements which are extreme outliers, and
     measurements with very small errors.  To avoid over-weighting measurements with very
     small errors, let's add something in quadarature.  
  */

  StatType stats = FitDataSetSoften (psfStars, N);
  double altSigma = (stats.Upper80 - stats.Lower20) / 1.6;

  // do anything special with identified good nights?
  // (myTGroup->flags & ID_IMAGE_PHOTOM_UBERCAL)

  fit1d_irls (psfStars, N);
  fit1d_irls (kronStars, N);
  fit1d_irls (brightStars, Nbright);

  if (useMgrp) {
    myTGroup->McalPSF    = psfStars->bSaveArray[0][0];
    myTGroup->McalAPER   = kronStars->bSaveArray[0][0];   // XXX this does not make sense: I need to apply the airmass slope calculated above first
    myTGroup->flags |= ID_IMAGE_TGROUP_PHOTCAL;  // set this flag (unset by default)
  } else {
    myTGroup->McalPSF    = 0.0;
    myTGroup->McalAPER   = 0.0;
  }

  // record statistics on the fit regardless if it is used or not
  myTGroup->dMcal      = psfStars->bSigma[0];
  myTGroup->stdev      = psfStars->sigma;
  myTGroup->dMmin      = psfStars->min;
  myTGroup->dMmax      = psfStars->max;
  myTGroup->McalChiSq  = psfStars->chisq;
  myTGroup->nFitPhotom = psfStars->Nmeas;
  myTGroup->nValPhotom = N;
  myTGroup->dKlam      = (TGROUP_FIT_AIRMASS) ? psfStars->bSaveArray[1][0] : 0;

  // bright end scatter (systematic error)
  myTGroup->dMsys = brightStars->sigma;

  TGTimes *mygroup = (TGTimes *) myTGroup->parent;
  fprintf (stderr, "TGroup Stats %f,%d : %d %d %6.3f %6.3f %6.3f %f %d\n", ohana_sec_to_mjd(mygroup->start), myTGroup->photcode, (int) N, (int) myTGroup->Nmeasure, psfStars->bSaveArray[0][0], stats.median, altSigma, myTGroup->McalChiSq, useMgrp);

  if (testImage) {
    TGTimes *parent = (TGTimes *) myTGroup->parent;
    fprintf (stderr, "test night %.0f aper: %f %f %d ... ", ohana_sec_to_mjd(parent->start), myTGroup->McalPSF, myTGroup->dMcal, myTGroup->nFitPhotom);
    fprintf (stderr, "%f %f  :  %f\n", myTGroup->McalPSF, myTGroup->dMsys, myTGroup->McalChiSq);
  }
  if (PLOTSTUFF) {
    fprintf (stderr, "Mgrp: %6.3f +/- %6.3f %5d of %5d | %.0f\n", myTGroup->McalPSF, myTGroup->dMcal, myTGroup->nFitPhotom, N, ohana_sec_to_mjd(tgroup->start));
    plot_setMcal (psfStars->alldata->yVector, N);
  }

  return TRUE;
}
  
int save_test_night_measures (FILE *fout, TGroup *myTGroup, Catalog *catalog) {

  int Nsecfilt = GetPhotcodeNsecfilt ();

  for (int j = 0; j < myTGroup->Nmeasure; j++) {
      
    off_t m = myTGroup->measure[j];
    off_t c = myTGroup->catalog[j];
      
    // XXX these two should both be 0.0 if we are fitting tgroups
    float Mcal     = getMcal  (m, c, MAG_CLASS_PSF);     // image zero point
    float Mmos     = getMmos  (m, c);                    // mosaic zero point

    float Mgrid    = getMgridTiny (&catalog[c].measureT[m]);                    // camera offset (deprecated?)
    float MrelPSF  = getMrel  (catalog, m, c, MAG_CLASS_PSF, MAG_SRC_CHP); // average magnitude
    float Mflat    = getMflat (m, c, catalog); // flat-field correction

    off_t n = catalog[c].measureT[m].averef;

    // magnitude for this measurement
    float MsysPSF = PhotSysTiny (&catalog[c].measureT[m], &catalog[c].averageT[n], &catalog[c].secfilt[n*Nsecfilt], MAG_CLASS_PSF);

    // for historical reasons, Mflat is defined with the wrong sign
    float delta = MsysPSF - MrelPSF - Mcal - Mmos - Mgrid - Mflat - myTGroup->McalPSF;

    int isBad = (catalog[c].measureT[m].dbFlags & MEAS_BAD);

    double mjdMeas = ohana_sec_to_mjd (catalog[c].measureT[m].t);

    fprintf (fout, "%f %f : %f %f %f %f %f  : %f %f %d : %f\n",
	     catalog[c].averageT[n].R, catalog[c].averageT[n].D,
	     MsysPSF, MrelPSF, Mcal, Mgrid, Mflat, catalog[c].measureT[m].airmass, delta, isBad, mjdMeas);

  }
  return TRUE;
}

int setMgrp_threaded (Catalog *catalog) {

  int i;
  off_t N;

  Image *image = getimages (&N, NULL);

  off_t Nmax = 0;
  for (off_t i = 0; i < NtgroupTimes; i++) {
    TGroup *tgroup = tgroupTimes[i][0].byCode;
    for (off_t j = 0; j < tgroupTimes[i][0].nCode; j++) {
      Nmax = MAX (Nmax, tgroup[j].Nmeasure);
    }
  }

  SetMgrpInfo summary;
  SetMgrpInfoInit (&summary, Nmax, FALSE);

  pthread_attr_t attr;
  pthread_attr_init (&attr);
  pthread_attr_setdetachstate (&attr, PTHREAD_CREATE_DETACHED);
  
  pthread_t *threads;
  ALLOCATE (threads, pthread_t, NTHREADS);

  ThreadInfo *threadinfo;
  ALLOCATE (threadinfo, ThreadInfo, NTHREADS);

  // each time this function is called, we cycle through the available tgroups
  // make sure we start at 0
  nextTGroup = 0;;

  // launch N worker threads
  for (i = 0; i < NTHREADS; i++) {
    threadinfo[i].entry = i;
    threadinfo[i].state = THREAD_RUN;
    threadinfo[i].catalog  =  catalog;
    threadinfo[i].image    =    image;

    // we do NOT allocate the arrays here, we only supply basic info used in the threads
    // to allocate the arrays and set the MaxIterations
    SetMgrpInfoInit (&threadinfo[i].info, Nmax, FALSE);
    pthread_create (&threads[i], NULL, setMgrp_worker, &threadinfo[i]);
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
    fprintf (stderr, "setMgrp thread %d : %d tgroups marked having too few measurements (Nbad: %d, Ncal: %d, Nmos: %d, Ngrid: %d, Nrel: %d, Nsys: %d), %d partials skipped\n", 
	     i, 
	     threadinfo[i].info.Nfew, 
	     threadinfo[i].info.Nbad, 
	     threadinfo[i].info.Ncal, 
	     threadinfo[i].info.Nmos, 
	     threadinfo[i].info.Ngrid, 
	     threadinfo[i].info.Nrel, 
	     threadinfo[i].info.Nsys,
	     threadinfo[i].info.Nskip);
    SetMgrpInfoAccum (&summary, &threadinfo[i].info);
  }
  fprintf (stderr, "total : %d tgroups marked having too few measurements (Nbad: %d, Ncal: %d, Nmos: %d, Ngrid: %d, Nrel: %d, Nsys: %d), %d partials skipped\n", 
	   summary.Nfew, 
	   summary.Nbad, 
	   summary.Ncal, 
	   summary.Nmos, 
	   summary.Ngrid, 
	   summary.Nrel, 
	   summary.Nsys, 
	   summary.Nskip);
  free (threadinfo);

  // XXX not allocated SetMgrpInfoFree (&summary);

  npass_output ++;

  return TRUE;
}

void *setMgrp_worker (void *data) {

  ThreadInfo *threadinfo = data;

  SetMgrpInfo results;
  SetMgrpInfoInit (&results, threadinfo->info.Nmax, TRUE); // allocate list, dlist arrays here

  while (1) {

    off_t i = getNextTGroupForThread();
    if (i == -1) {
      threadinfo->state = THREAD_DONE;
      return NULL;
    }

    Catalog *catalog = threadinfo->catalog;
    Image *image = threadinfo->image;

    TGroup *tgroup = tgroupTimes[i][0].byCode;
    for (int j = 0; j < tgroupTimes[i][0].nCode; j++) {
      setMgrp_tgroup (&tgroup[j], i, image, catalog, &results);
      SetMgrpInfoAccum (&threadinfo->info, &results);
    }
  }

  SetMgrpInfoFree (&results);
  return NULL;
}

StatType statsTGroupM (Catalog *catalog) {
  OHANA_UNUSED_PARAM(catalog);

  double *list, *dlist;
  StatType stats;

  bzero (&stats, sizeof (StatType));
  if (!TGROUP_ZEROPT) return (stats);

  // XXX do this by time & photcode?
  ALLOCATE (list, double, NtgroupTimes*Nphotcodes);
  ALLOCATE (dlist, double, NtgroupTimes*Nphotcodes);

  int n = 0;
  for (int i = 0; i < NtgroupTimes; i++) {
    TGroup *tgroup = tgroupTimes[i][0].byCode;
    for (int j = 0; j < tgroupTimes[i][0].nCode; j++) {
      if (tgroup[j].flags & (ID_IMAGE_NIGHT_POOR | ID_IMAGE_PHOTOM_FEW)) continue;
      list[n] = tgroup[j].McalPSF;
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

StatType statsTGroupdM (Catalog *catalog) {
  OHANA_UNUSED_PARAM(catalog);

  double *list, *dlist;
  StatType stats;

  bzero (&stats, sizeof (StatType));
  if (!TGROUP_ZEROPT) return (stats);

  ALLOCATE (list, double, NtgroupTimes*Nphotcodes);
  ALLOCATE (dlist, double, NtgroupTimes*Nphotcodes);

  int n = 0;
  for (int i = 0; i < NtgroupTimes; i++) {
    TGroup *tgroup = tgroupTimes[i][0].byCode;
    for (int j = 0; j < tgroupTimes[i][0].nCode; j++) {
      if (tgroup[j].flags & (ID_IMAGE_NIGHT_POOR | ID_IMAGE_PHOTOM_FEW)) continue;
      list[n] = tgroup[j].dMcal;
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

StatType statsTGroupX (Catalog *catalog) {
  OHANA_UNUSED_PARAM(catalog);

  double *list, *dlist;
  StatType stats;

  bzero (&stats, sizeof (StatType));
  if (!TGROUP_ZEROPT) return (stats);

  ALLOCATE (list, double, NtgroupTimes*Nphotcodes);
  ALLOCATE (dlist, double, NtgroupTimes*Nphotcodes);

  int n = 0;
  for (int i = 0; i < NtgroupTimes; i++) {
    TGroup *tgroup = tgroupTimes[i][0].byCode;
    for (int j = 0; j < tgroupTimes[i][0].nCode; j++) {
      if (tgroup[j].flags & (ID_IMAGE_NIGHT_POOR | ID_IMAGE_PHOTOM_FEW)) continue;
      list[n] = tgroup[j].McalChiSq;
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

/* new rules for bad tgroups (bad nights) 
 * too few exposures (Nmosaic < X)
 * dMsys or stdev > X
 * X%-ile of dMsys or stdev
 * McalChiSq > X
 * X%-ile of McalChiSq

 * I can define rules for these but they would be arbitrary
 * I think I should do an analysis of the observed distribution

 */

static float MinChiSqLim = NAN;
static float MaxChiSqLim = NAN;
static float MinScatterLim = NAN;
static float MaxScatterLim = NAN;

void clean_tgroups () {

  double *mlist, *slist;

  if (!TGROUP_ZEROPT) return;

  if (VERBOSE) fprintf (stderr, "marking poor tgroups\n");

  if (isnan (MaxChiSqLim))   MaxChiSqLim   = NIGHT_CHISQ;
  if (isnan (MaxScatterLim)) MaxScatterLim = NIGHT_SCATTER;

  ALLOCATE (mlist, double, NtgroupTimes*Nphotcodes);
  ALLOCATE (slist, double, NtgroupTimes*Nphotcodes);

  // measure the good/bad statistics using nights which are actually calibrated
  // But they could be applied to all nights below (allowing bad nights to become good)
  // However, for now, we will NOT allow nights to be redeemed (bad -> good)
  int N = 0;
  for (int i = 0; i < NtgroupTimes; i++) {
    TGroup *tgroup = tgroupTimes[i][0].byCode;
    for (int j = 0; j < tgroupTimes[i][0].nCode; j++) {
      if (!(tgroup[j].flags & ID_IMAGE_TGROUP_PHOTCAL)) continue;
      mlist[N] = tgroup[j].McalChiSq;
      slist[N] = tgroup[j].stdev; // stdev of all measurements
      N++;
    }
  }

  StatType stats;
  liststats_setmode (&stats, "MEAN");

  liststats (mlist, NULL, NULL, N, &stats);
  float ChiSqUpper90 = stats.Upper90; 
  if (isnan (MinChiSqLim)) MinChiSqLim = 2.0*stats.median;             // chi-square cut cannot fall below this value (even if this is > MaxChiSqLim)
  float ChiSqLimit = MAX(MinChiSqLim, MIN(MaxChiSqLim, ChiSqUpper90)); // chi-square cut should be between MinChiSqLim and MaxChiSqLim

  liststats (slist, NULL, NULL, N, &stats);
  float ScatterUpper90 = stats.Upper90; 
  if (isnan (MinScatterLim)) MinScatterLim = 2.0*stats.median;         // scatter cut cannto fall below this value (even if this is > MaxScatterLim)
  float ScatterLimit = MAX(MinScatterLim, MIN(MaxScatterLim, ScatterUpper90));

  fprintf (stderr, "TGROUPS: ChiSqLimit: %f, ScatterLimit: %f | ChiSquare Upper 90: %f, Scatter Upper 90: %f\n", ChiSqLimit, ScatterLimit, ChiSqUpper90, ScatterUpper90);
  
  int Ntotal = 0, Npoor = 0, Nmark = 0, Nscatter = 0, Nchisq = 0, NfewNights = 0, NfewExp = 0;
  for (int i = 0; i < NtgroupTimes; i++) {
    TGroup *tgroup = tgroupTimes[i][0].byCode;
    for (int j = 0; j < tgroupTimes[i][0].nCode; j++) {
      
      Ntotal ++;

      int mark = FALSE;

      // we do not allow bad nights to be redeemed
      if (tgroup[j].flags & ID_IMAGE_NIGHT_POOR) {
	fprintf (stderr, "TGroup Already Poor, not redeemed %f,%d : %6.3f %6.3f %6.3f\n", ohana_sec_to_mjd(tgroupTimes[i][0].start), tgroup[j].photcode, tgroup[j].McalPSF, tgroup[j].stdev, tgroup[j].McalChiSq);
	Npoor ++;
	continue;
      }

      // too few measurements (set in setMgrp_tgroup)
      if (tgroup[j].flags & ID_IMAGE_PHOTOM_FEW) {
	mark = TRUE;
	NfewNights ++;
      }
      // too few exposures (configure)
      if ((tgroup[j].Nmosaic < 4) && (tgroup[j].Nimage < 4)) {
	mark = TRUE;
	NfewExp ++;
      }
      // scatter too large
      if (tgroup[j].stdev > ScatterLimit) {
	mark = TRUE;
	Nscatter ++;
      }
      // chisq too large
      if (tgroup[j].McalChiSq > ChiSqLimit) {
	mark = TRUE;
	Nchisq ++;
      }
      if (mark) { 
	Nmark ++;
	markBadTGroup(&tgroup[j]); // mark the tgroup & associated images & mosaics with a bad night
	fprintf (stderr, "TGroup Poor %f,%d : %6.3f %6.3f %6.3f\n", ohana_sec_to_mjd(tgroupTimes[i][0].start), tgroup[j].photcode, tgroup[j].McalPSF, tgroup[j].stdev, tgroup[j].McalChiSq);
      } else {
	markGoodTGroup(&tgroup[j]); // mark the tgroup & associated images & mosaics with a good night
      }
    }
  }

  fprintf (stderr, "%d + %d of %d tgroups marked poor (%d scatter, %d few nights, %d few exposures, %d chisq)\n",  Nmark, Npoor, Ntotal, Nscatter, NfewNights, NfewExp, Nchisq);

  free (mlist);
  free (slist);
}

double get_median_zpt_tgroups (short photcode) {

  double *mlist;

  if (!TGROUP_ZEROPT) return NAN;

  ALLOCATE (mlist, double, NtgroupTimes);

  int N = 0;
  for (int i = 0; i < NtgroupTimes; i++) {
    TGroup *tgroup = tgroupTimes[i][0].byCode;
    for (int j = 0; j < tgroupTimes[i][0].nCode; j++) {
      if (tgroup[j].photcode != photcode) continue;
      if (!(tgroup[j].flags & ID_IMAGE_TGROUP_PHOTCAL)) continue;
      mlist[N] = tgroup[j].McalPSF;
      N++;
    }
  }

  if (N == 0) {
    free (mlist);
    return NAN;
  }

  StatType stats;
  liststats_setmode (&stats, "MEAN");

  liststats (mlist, NULL, NULL, N, &stats);
  free (mlist);

  fprintf (stderr, "rationalize by tgroup using %d pts\n", N);
  return stats.median; 
}

void set_median_zpt_tgroups (short photcode, double zpt) {

  if (!TGROUP_ZEROPT) return;
  if (!isfinite(zpt)) return; // do not break the zero points

  for (int i = 0; i < NtgroupTimes; i++) {
    TGroup *tgroup = tgroupTimes[i][0].byCode;
    for (int j = 0; j < tgroupTimes[i][0].nCode; j++) {
      if (tgroup[j].photcode != photcode) continue;
      if (!(tgroup[j].flags & ID_IMAGE_TGROUP_PHOTCAL)) continue;
      tgroup[j].McalPSF -= zpt;
      tgroup[j].McalAPER -= zpt;
    }
  }
  return; 
}

void plot_tgroup_fields (Catalog *catalog) {

  off_t m, c, Nimage;
  double *xlist, *ylist;
  char string[64];
  Graphdata graphdata;

  if (!TGROUP_ZEROPT) return;

  // Image *image = getimages (&Nimage, NULL); returned value ignored
  getimages (&Nimage, NULL);

  off_t N = 0;
  for (off_t i = 0; i < NtgroupTimes; i++) {
    TGroup *tgroup = tgroupTimes[i][0].byCode;
    for (off_t j = 0; j < tgroupTimes[i][0].nCode; j++) {
      N = MAX (N, tgroup[j].Nmeasure);
    }
  }

  ALLOCATE (xlist, double, N);
  ALLOCATE (ylist, double, N);

  for (off_t k = 0; k < NtgroupTimes; k++) {
    N = 0;
    TGroup *tgroup = tgroupTimes[k][0].byCode;
    for (off_t i = 0; i < tgroupTimes[i][0].nCode; i++) {
      for (off_t j = 0; j < tgroup[i].Nmeasure; j++) {
	
	m = tgroup[i].measure[j];
	c = tgroup[i].catalog[j];
      
	if (catalog[c].measureT[m].dbFlags & MEAS_BAD) continue;

	// ave = catalog[c].measureT[m].averef;
	xlist[N] = catalog[c].measureT[m].R;
	ylist[N] = catalog[c].measureT[m].D;
	N++;
      }
    }
    sprintf (string, "TGroup "OFF_T_FMT,  k);
    plot_defaults (&graphdata);
    plot_list (&graphdata, xlist, ylist, N, string, NULL);
  }

  free (ylist);
  free (xlist);
}

void plot_tgroups () {

  off_t i, bin;
  double *xlist, *Mlist, *dlist;
  Graphdata graphdata;

  if (!TGROUP_ZEROPT) return;

  ALLOCATE (xlist, double, NtgroupTimes*Nphotcodes);
  ALLOCATE (dlist, double, NtgroupTimes*Nphotcodes);
  ALLOCATE (Mlist, double, NtgroupTimes*Nphotcodes);

  int Npts = 0;
  for (i = 0; i < NtgroupTimes; i++) {
    TGroup *tgroup = tgroupTimes[i][0].byCode;
    for (int j = 0; j < tgroupTimes[i][0].nCode; j++) {
      Mlist[Npts] = tgroup[j].McalPSF;
      dlist[Npts] = tgroup[j].dMcal;
      xlist[Npts] = tgroupTimes[i][0].start;
      Npts ++;
    }
  }

  plot_defaults (&graphdata);
  graphdata.xmin = 0.95;
  graphdata.xmax = 2.50;
  graphdata.ymin = PlotdMmin;
  graphdata.ymax = PlotdMmax;
  plot_list (&graphdata, xlist, Mlist, Npts, "airmass vs Mcal", "%s.airmass.png", OUTROOT);
  plot_defaults (&graphdata);
  graphdata.size = 1.5;
  graphdata.ptype = 7;
  plot_list (&graphdata, Mlist, dlist, Npts, "Mcal vs dMcal", "%s.MdM.png", OUTROOT);

# define NBIN 200
  REALLOCATE (xlist, double, NBIN);
  REALLOCATE (Mlist, double, NBIN);

  /**** dMcal histgram ****/
  for (i = 0; i < NBIN; i++) xlist[i] = 0.00005*i;
  bzero (Mlist, NBIN*sizeof(double));

  for (i = 0; i < NtgroupTimes; i++) {
    TGroup *tgroup = tgroupTimes[i][0].byCode;
    for (int j = 0; j < tgroupTimes[i][0].nCode; j++) {
      bin = tgroup[j].dMcal / 0.00005;
      bin = MAX (0, MIN (NBIN - 1, bin));
      Mlist[bin] += 1.0;
    }
  }
  plot_defaults (&graphdata);
  graphdata.style = 1;
  plot_list (&graphdata, xlist, Mlist, NBIN, "dMcal hist", "%s.dMcalhist.png", OUTROOT);

  free (dlist);
  free (xlist);
  free (Mlist);
}

// temporary test output 
void dump_tgroups (Catalog *catalog, int Npass) {

  return;

  int Nsecfilt = GetPhotcodeNsecfilt ();

  int Ngrp = 0;
  for (off_t i = 0; i < NtgroupTimes; i++) {

    TGroup *tgroup = tgroupTimes[i][0].byCode;
    for (off_t j = 0; j < tgroupTimes[i][0].nCode; j++) {

      FILE *fout = NULL;
      char filename[64];
      snprintf (filename, 64, "tgrp.%05d.%02d.dat", Ngrp, Npass);
      fout = fopen (filename, "w");
      Ngrp ++;

      TGroup *myTGroup = &tgroup[j];

      for (int k = 0; k < myTGroup->Nmeasure; k++) {
      
	off_t m = myTGroup->measure[k];
	off_t c = myTGroup->catalog[k];
      
	float Mcal     = getMcal  (m, c, MAG_CLASS_PSF);     // image zero point
	float Mmos     = getMmos  (m, c);                    // mosaic zero point
	float Mgrp     = getMgrp  (m, c, catalog[c].measureT[m].airmass, NULL);

	float MrelPSF  = getMrel  (catalog, m, c, MAG_CLASS_PSF, MAG_SRC_CHP); // average magnitude
	// float Mgrid    = getMgridTiny (&catalog[c].measureT[m]); // camera offset (deprecated?)
	float Mflat    = getMflat (m, c, catalog); // flat-field correction

	off_t n = catalog[c].measureT[m].averef;

	// nominal magnitude for this measurement (instrumental + Ko(airmass - 1) + Co
	float MsysPSF = PhotSysTiny (&catalog[c].measureT[m], &catalog[c].averageT[n], &catalog[c].secfilt[n*Nsecfilt], MAG_CLASS_PSF);

	// for historical reasons, Mflat is defined with the wrong sign
	float delta = MsysPSF - MrelPSF - Mcal - Mmos - Mgrp - Mflat;

	fprintf (fout, "%f %f : %f %f : %f %f %f  : %f %f\n", catalog[c].averageT[n].R, catalog[c].averageT[n].D, MsysPSF, MrelPSF, Mcal, Mmos, Mgrp, catalog[c].measureT[m].airmass, delta);
      }
      fclose (fout);
    }
  }
}

void dump_tgroup_imstats (int Npass) {

  FILE *fout = NULL;
  char filename[64];

  snprintf (filename, 64, "imstats.grp.%02d.dat", Npass);
  fout = fopen (filename, "w");

  for (off_t i = 0; i < NtgroupTimes; i++) {

    TGroup *tgroup = tgroupTimes[i][0].byCode;
    for (off_t j = 0; j < tgroupTimes[i][0].nCode; j++) {
      fprintf (fout, "%.0f %5d | %6ld %6d %6d %4ld %4ld | 0x%08x %6.3f %6.3f %6.3f %6.3f %6.3f %6.3f %6.3f %6.3f\n",
	       ohana_sec_to_mjd(tgroupTimes[i][0].start), tgroup[j].photcode,
	       tgroup[j].Nmeasure, tgroup[j].nFitPhotom, tgroup[j].nValPhotom, tgroup[j].Nimage, tgroup[j].Nmosaic,
	       tgroup[j].flags, tgroup[j].McalPSF, tgroup[j].dKlam, tgroup[j].dMcal, tgroup[j].dMsys, tgroup[j].stdev, tgroup[j].dMmin, tgroup[j].dMmax, tgroup[j].McalChiSq);
    }
  }
  fclose (fout);

  // ************ mosaic stats

  snprintf (filename, 64, "imstats.mos.%02d.dat", Npass);
  fout = fopen (filename, "w");

  for (off_t i = 0; i < NtgroupTimes; i++) {

    TGroup *tgroup = tgroupTimes[i][0].byCode;
    for (off_t j = 0; j < tgroupTimes[i][0].nCode; j++) {

      for (off_t k = 0; k < tgroup->Nmosaic; k++) {
	Mosaic *mosaic = tgroup[j].mosaic[k];

	fprintf (fout, "%4ld %3ld %3ld %.0f %5d %5d %5d | %12.5f %5.3f 0x%08x | %6.3f %6.3f %6.3f %6.3f\n",
		 i, j, k, ohana_sec_to_mjd(tgroupTimes[i][0].start), mosaic->photcode, mosaic->nFitPhotom, mosaic->nValPhotom,
		 ohana_sec_to_mjd(mosaic->start), mosaic->secz, mosaic->flags,
		 mosaic->McalPSF, mosaic->dMcal, mosaic->dMsys, mosaic->McalChiSq);
      }
    }
  }
  fclose (fout);

  // ************ image stats

  snprintf (filename, 64, "imstats.chp.%02d.dat", Npass);
  fout = fopen (filename, "w");

  off_t Nimage = 0;
  Image *image = getimages (&Nimage, NULL);
  for (off_t i = 0; i < NtgroupTimes; i++) {

    TGroup *tgroup = tgroupTimes[i][0].byCode;
    for (off_t j = 0; j < tgroupTimes[i][0].nCode; j++) {

      for (off_t k = 0; k < tgroup->Nimage; k++) {
	off_t im = tgroup[j].image[k];

	fprintf (fout, "%4ld %3ld %3ld %.0f %5d | %12.5f %5.3f 0x%08x | %6.3f %6.3f %6.3f %6.3f %6.3f\n",
		 i, j, k, ohana_sec_to_mjd(tgroupTimes[i][0].start), image[im].photcode,
		 ohana_sec_to_mjd(image[im].tzero), image[im].secz, image[im].flags,
		 image[im].McalPSF, image[im].McalAPER, image[im].dMcal, image[im].dMagSys, image[im].McalChiSq);
      }
    }
  }

  fclose (fout);
}

/*
  for testing, put lines like these in relphot_images.c within the iteration loop
      dump_tgroups (catalog, i + 40);
      dump_catalog (catalog, 0, i + 40); // for a test, just dump a specific catalog
      dump_tgroup_imstats (i + 40);
*/
