# include "addusno.h"

find_proper (catstats, catalog, usnostats, usno, Nusno)
CatStats catstats[];
Catalog catalog[];
USNOstats usnostats[];
USNOdata usno[];
int Nusno;
{

  int i, j, k, n, m, N, first_j;
  double RADIUS2, PROPER2;
  float *X1, *Y1, *X2, *Y2, *usnodist;
  float dX, dY, dR, dR2;
  int *N1, *N2,  *next, last;
  int Nave, Nmeas, NMEAS, Nmatch;
  unsigned int flags;
  Measure *tmpmeasure;
  Coords *tcoords;
  int already_matched, far_enough;

  X1 = catstats[0].X;
  Y1 = catstats[0].Y;
  N1 = catstats[0].N;
  Nave = catalog[0].Naverage;

  /* no need to do this twice!! */
  ALLOCATE (usnodist, float, Nave);
  memset (usnodist, 0, Nave*sizeof(float));

  X2 = usnostats[0].X;
  Y2 = usnostats[0].Y;
  N2 = usnostats[0].N;

 /* set up link listed pointers for new measurements */
  Nmatch = 0;
  Nmeas = catalog[0].Nmeasure;
  NMEAS = Nmeas + 1000;
  ALLOCATE (next, int, NMEAS);
  REALLOCATE (catalog[0].measure, Measure, NMEAS);
  /* set up pointers for linked list of measurements */
  for (i = 0; i < Nmeas - 1; i++) {
    next[i] = i+1;
  }
  next[i] = -1;
  last = i;
  
  /* choose a radius for matches */
  PROPER2 = PROPER*PROPER;
  RADIUS2 = RADIUS*RADIUS;

  /** find matched stars **/
  for (i = j = 0; (i < Nave) && (j < Nusno); ) {
    if (catalog[0].average[N1[i]].code & ID_MOVING) { 
      /* this is not a star, skip */
      i++;
      continue;
    }
    if (catalog[0].average[N1[i]].Nm < 3) { 
      /* may just be a noise spike, skip */
      i++;
      continue;
    }
    if (catalog[0].average[N1[i]].code & ID_USNO) {
      /* already matched with USNO, skip this one */
      i++;
      continue;
    }
    dX = X1[i] - X2[j];
    if (dX <= -2*PROPER) {
      i++;
      continue;
    }
    if (dX >= 2*PROPER) {
      j++;
      continue;
    }
    /* negative dX: j is too large, positive dX, i is too large */
    first_j = j;
    for (; (dX > -2*PROPER) && (j < Nusno); j++) {
      dX = X1[i] - X2[j];
      dY = Y1[i] - Y2[j];
      dR = dX*dX + dY*dY;
      if (dR < PROPER2) {  
	n = N1[i];  /* N1 refers to the average[] list */
	N = N2[j];  /* N2 refers to the usno[] list */
	if (usnostats[0].match[N] > -1) 
	  continue;
	if ((catalog[0].average[n].code & ID_USNO) && (usnodist[i] < dR)) {
	  /* existing USNO match is closer than this new one, skip this one */
	  continue;
	}
	usnodist[i] = dR;
	m = catalog[0].average[n].offset;  /* first measurement of this star */
	for (k = 0; k < catalog[0].average[n].Nm - 1; k++)
	  m = next[m];
	next[Nmeas+1] = next[m]; /* insert 2 measurements in linked list */
	next[Nmeas] = Nmeas + 1;
	next[m] = Nmeas;
	if (next[Nmeas+1] == -1) { /* last just was moved */
	  last = Nmeas+1;
	}
	Nmatch ++;
	
	/** add measurements for this star **/
	catalog[0].measure[Nmeas].dR  	     = 360000.0*(catalog[0].average[n].R - usno[N].R);
	catalog[0].measure[Nmeas].dD  	     = 360000.0*(catalog[0].average[n].D - usno[N].D);
	catalog[0].measure[Nmeas].M   	     = 1000.0*fabs(usno[N].r);
	catalog[0].measure[Nmeas].McalPSF    = 0;    /* above measurement is exact */
	catalog[0].measure[Nmeas].McalAPER   = 0;    /* above measurement is exact */
	catalog[0].measure[Nmeas].dM         = 100;  /* error in input files stored in thousandths of mag */
	catalog[0].measure[Nmeas].t          = 0;    /* a flag: if 0, image is not in database */
	catalog[0].measure[Nmeas].averef     = n;
	catalog[0].measure[Nmeas].photcode   = USNO_RED; 
	catalog[0].measure[Nmeas+1].dR       = catalog[0].measure[Nmeas].dR;
	catalog[0].measure[Nmeas+1].dD       = catalog[0].measure[Nmeas].dD;
	catalog[0].measure[Nmeas+1].M        = 1000.0*fabs(usno[N].b);
	catalog[0].measure[Nmeas+1].McalPSF  = 0;    /* above measurement is exact */
	catalog[0].measure[Nmeas+1].McalAPER = 0;    /* above measurement is exact */
	catalog[0].measure[Nmeas+1].dM       = 100;  /* error in input files stored in thousandths of mag */
	catalog[0].measure[Nmeas+1].t        = 0;    /* a flag: if 0, image is not in database */
	catalog[0].measure[Nmeas+1].averef   = n;
	catalog[0].measure[Nmeas+1].photcode = USNO_BLUE; 
	/* add flag in average to mark as matched with the USNO catalog */
	catalog[0].average[n].code |= (ID_PROPER | ID_USNO);
	
	/* we add two measurement for each star: 1 in r, 1 in b */
	catalog[0].average[n].Nm +=2;
	Nmeas +=2;  
	if (Nmeas == NMEAS - 2) {  /* just to be safe... */
	  NMEAS = Nmeas + 1000;
	  REALLOCATE (next, int, NMEAS);
	  REALLOCATE (catalog[0].measure, Measure, NMEAS);
	}
      }
    }
    j = first_j;
    i++;
  }
  
  REALLOCATE (catalog[0].measure, Measure, Nmeas);

  /* fix order of Measure (memory intensive, but fast) */
  N = 0; 
  ALLOCATE (tmpmeasure, Measure, Nmeas);
  for (i = 0; i < Nave; i++) {
    n = catalog[0].average[i].offset;
    catalog[0].average[i].offset = N;
    for (k = 0; k < catalog[0].average[i].Nm; k++, N++) {
      tmpmeasure[N] = catalog[0].measure[n]; 
      n = next[n];
    }
  }
  free (catalog[0].measure);
  catalog[0].measure = tmpmeasure;
    
  catalog[0].Nmeasure = Nmeas;
  if (VERBOSE) fprintf (stderr, "Nusno, Nave, Nmeas: %d %d %d, (%d matches)\n", Nusno, Nave, Nmeas, Nmatch);
}

