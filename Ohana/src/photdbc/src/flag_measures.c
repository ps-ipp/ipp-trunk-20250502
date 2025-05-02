# include "photdbc.h"

void flag_measures (FITS_DB *db, Catalog *catalog) {

  int i, j, k, m, n, N, found;
  int *imagelink, Nimage, Nlist, PhotNsec;
  unsigned int Time;
  int Ncode, Photcode, Minst, Mag, BAD_MEASURE;
  double ra, dec, x, y, Nsigma;
  double *list, *dlist;

  Image *image;
  StatType stats;

  if (VERBOSE) fprintf (stderr, "flagging bad measurements\n");

  image = gfits_table_get_Image (&db[0].ftable, &Nimage, &db[0].scaledValue, &db[0].nativeOrder);
  if (!image) {
      fprintf (stderr, "ERROR: failed to read images\n");
      exit (2);
  }

  PhotNsec = GetPhotcodeNsecfilt ();
  /* need to replace dMcal with values from external table? */

  /* clear existing measure flags */
  for (i = 0; i < catalog[0].Nmeasure; i++) catalog[0].measure[i].flags = 0;

  /* generate image index */
  ALLOCATE (imagelink, int, catalog[0].Nmeasure);
  for (i = 0; i < catalog[0].Nmeasure; i++) {
    n = catalog[0].measure[i].averef;
    Time = catalog[0].measure[i].t;
    Photcode = catalog[0].measure[i].photcode;
    found = FALSE;
    for (k = 0; !found && (k < Nimage); k++) { 
      if ((Time == image[k].tzero) && (Photcode == image[k].photcode)) {
	imagelink[i] = k;
	found = TRUE;
      }
    }
    if (!found) {
      /* some error? */
    }
  }

  /* flag by area : find source image, get X,Y coords, set flag */
  for (i = 0; i < catalog[0].Nmeasure; i++) {
    n = catalog[0].measure[i].averef;
    ra  = catalog[0].average[n].R - catalog[0].measure[i].dR / 3600.0;
    dec = catalog[0].average[n].D - catalog[0].measure[i].dD / 3600.0;
    k = imagelink[i];
    RD_to_XY (&x, &y, ra, dec, &image[k].coords);
    if (x < XMIN) goto flagarea;
    if (x > XMAX) goto flagarea;
    if (y < YMIN) goto flagarea;
    if (y > YMAX) goto flagarea;
    continue;
  flagarea:
    catalog[0].measure[i].flags |= FLAG_AREA;
  }

  /* flag by Minst */
  for (i = 0; i < catalog[0].Nmeasure; i++) {
    Minst = iPhotInst (&catalog[0].measure[i]);
    if (Minst < MMIN) goto flagminst;
    if (Minst > MMAX) goto flagminst;
    continue;
  flagminst:
    catalog[0].measure[i].flags |= FLAG_MINST;
  }

  /* flag by image (dMcal? time from table?) */
  for (i = 0; i < catalog[0].Nmeasure; i++) {
    k = imagelink[i];
    if (image[k].dMcal > DMCAL_MIN) {
      catalog[0].measure[i].flags |= FLAG_DMCAL;
    }
  }

  /* allocate a list for temp storage of mag values */
  Nlist = 0;
  for (i = 0; i < catalog[0].Naverage; i++) {
    Nlist = MAX (Nlist, catalog[0].average[i].Nm);
  }
  ALLOCATE (list, double, Nlist);
  ALLOCATE (dlist, double, Nlist);
  initstats ("MEAN");

  /* the following sections apply only to PRI/SEC photcodes */
  for (n = 0; n < photcodes.Ncode; n++) {
    if (photcodes.code[n].type == PHOT_DEP) continue;
    if (photcodes.code[n].type == PHOT_REF) continue;
    Photcode = photcodes.code[n].code;
    
    BAD_MEASURE = FLAG_AREA | FLAG_MINST | FLAG_DMCAL;
    /* 2x 3sigma clipping */
    for (i = 0; i < catalog[0].Naverage; i++) {

      /* extract good measures, calculate stats */
      N = 0;
      m = catalog[0].average[i].offset;
      for (j = 0; j < catalog[0].average[i].Nm; j++, m++) {
	if ((Ncode = photcodes.hashcode[catalog[0].measure[m].photcode]) < 0) continue;
	if (photcodes.code[Ncode].equiv != Photcode) continue;
	if (photcodes.code[Ncode].type != PHOT_DEP) continue;
	if (catalog[0].measure[m].flags & BAD_MEASURE) continue;
	list[N] = iPhotRel (&catalog[0].measure[m], &catalog[0].average[i],  &catalog[0].secfilt[i*PhotNsec]);
	dlist[N] = sqrt (SQ(DMGAIN*catalog[0].measure[m].dM) + DMSYS);
	N++;
      }
      if (N < 3) continue;
      liststats (list, dlist, N, &stats);
      Nsigma = 3.0;
      if (N < 10) Nsigma = 0.9*N / sqrt (N - 1);

      /* re-extract good measures, calculate stats */
      N = 0;
      m = catalog[0].average[i].offset;
      for (j = 0; j < catalog[0].average[i].Nm; j++, m++) {
	if ((Ncode = photcodes.hashcode[catalog[0].measure[m].photcode]) < 0) continue;
	if (photcodes.code[Ncode].equiv != Photcode) continue;
	if (photcodes.code[Ncode].type != PHOT_DEP) continue;
	if (catalog[0].measure[m].flags & BAD_MEASURE) continue;
	Mag = iPhotRel (&catalog[0].measure[m], &catalog[0].average[i],  &catalog[0].secfilt[i*PhotNsec]);
	if (fabs(Mag - stats.mean) > Nsigma*stats.sigma) continue;
	list[N]  = Mag;
	dlist[N] = sqrt (SQ(DMGAIN*catalog[0].measure[m].dM) + DMSYS);
	N++;
      }
      if (N > 2) liststats (list, dlist, N, &stats);

      /* flag bad measures */
      m = catalog[0].average[i].offset;
      for (j = 0; j < catalog[0].average[i].Nm; j++, m++) {
	if ((Ncode = photcodes.hashcode[catalog[0].measure[m].photcode]) < 0) continue;
	if (photcodes.code[Ncode].equiv != Photcode) continue;
	if (photcodes.code[Ncode].type != PHOT_DEP) continue;
	Mag = iPhotRel (&catalog[0].measure[m], &catalog[0].average[i],  &catalog[0].secfilt[i*PhotNsec]);
	if (fabs(Mag - stats.mean) > Nsigma*stats.sigma) catalog[0].measure[m].flags |= FLAG_SIGCLIP;
      }
    }      
  
    BAD_MEASURE = FLAG_AREA | FLAG_MINST | FLAG_DMCAL | FLAG_SIGCLIP;
    /* flag variables */
    for (i = 0; i < catalog[0].Naverage; i++) {
      /* extract good measures, calculate chisq */
      N = 0;
      m = catalog[0].average[i].offset;
      for (j = 0; j < catalog[0].average[i].Nm; j++, m++) {
	if ((Ncode = photcodes.hashcode[catalog[0].measure[m].photcode]) < 0) continue;
	if (photcodes.code[Ncode].equiv != Photcode) continue;
	if (photcodes.code[Ncode].type != PHOT_DEP) continue;
	if (catalog[0].measure[m].flags & BAD_MEASURE) continue;
	list[N] = iPhotRel (&catalog[0].measure[m], &catalog[0].average[i],  &catalog[0].secfilt[i*PhotNsec]);
	dlist[N] = sqrt (SQ(DMGAIN*catalog[0].measure[m].dM) + DMSYS);
	N++;
      }
      liststats (list, dlist, N, &stats);
      if (stats.chisq < CHISQ_MAX) continue;

      /* flag bad measures */
      m = catalog[0].average[i].offset;
      for (j = 0; j < catalog[0].average[i].Nm; j++, m++) {
	if ((Ncode = photcodes.hashcode[catalog[0].measure[m].photcode]) < 0) continue;
	if (photcodes.code[Ncode].equiv != Photcode) continue;
	if (photcodes.code[Ncode].type != PHOT_DEP) continue;
	catalog[0].measure[m].flags |= FLAG_CHISQ;
      }
    }	

    /* flag too few valid measures */
    BAD_MEASURE = FLAG_AREA | FLAG_MINST | FLAG_DMCAL | FLAG_SIGCLIP | FLAG_CHISQ;
    for (i = 0; i < catalog[0].Naverage; i++) {
      N = 0;
      m = catalog[0].average[i].offset;
      for (j = 0; j < catalog[0].average[i].Nm; j++, m++) {
	if ((Ncode = photcodes.hashcode[catalog[0].measure[m].photcode]) < 0) continue;
	if (photcodes.code[Ncode].equiv != Photcode) continue;
	if (photcodes.code[Ncode].type != PHOT_DEP) continue;
	if (catalog[0].measure[m].flags & BAD_MEASURE) continue;
	N++;
      }
      if (N == 0) continue; // only flag objects with valid measurements, but too few
      if (N >= NMEAS_MIN) continue;
      m = catalog[0].average[i].offset;
      for (j = 0; j < catalog[0].average[i].Nm; j++, m++) {
	if ((Ncode = photcodes.hashcode[catalog[0].measure[m].photcode]) < 0) continue;
	if (photcodes.code[Ncode].equiv != Photcode) continue;
	if (photcodes.code[Ncode].type != PHOT_DEP) continue;
	catalog[0].measure[m].flags |= FLAG_TOOFEW;
      }
    }	
  }
}


/* notes
   
   flag by area: uses an inefficient search for the image.  
   images should be sorted by time and we should use bisection
   
   number of loops over Nmeasure: 9

*/

