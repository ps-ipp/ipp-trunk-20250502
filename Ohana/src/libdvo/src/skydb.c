
typedef struct {
  float r, d;
} SkyCoord;

/* each region is bounded on the sky by lines of constant RA & DEC.
   the region represents a specified depth, and it may or may not have
   children.  The start and end of the children in the array are childS and childE 
   If the region at the given depth is populated with an object table, then 'object' is TRUE.
   If the region at the given depth is populated with an image table, then 'image' is TRUE.
*/   

typedef struct {
  char  name[16];
  float Rmin, Rmax;
  float Dmin, Dmax;
  int   childS, childE;
  char  depth, child;
  char  object, image;
} SkyRegion; /* 48 bytes */ 

/* I have defined no accelerators other than the table hierarchy */

/* find region which overlaps c at given depth (-1 : max depth) */
SkyRegion *SkyFindPoint (SkyRegion *db, SkyCoord c, int depth) {
  
  Ns = 0;
  Ne = 1;

  while (1) {
    No = -1;
    for (i = Ns; (No == -1) && (i < Ne); i++) {
      if (c.r < db[i].Rmin) continue;
      if (c.r > db[i].Rmax) continue;
      if (c.d < db[i].Dmin) continue;
      if (c.d > db[i].Dmax) continue;
      // got the needed region at this depth
      No = i;
    }
    if (No == -1) return ((SkyRegion *) NULL);
    if (depth == db[No].depth) return (&db[No]);
    if ((depth > db[No].depth) && !db[No].child) return ((SkyRegion *) NULL);

    /* need to check Ns, Ne, or guarantee valid range */
    Ns = db[No].childS;
    Ne = db[No].childE;

    if ((depth > db[No].depth) && db[No].child) continue;
    return (&db[No]);
  }
}

/* find regions at all levels which overlap c */
SkyRegion **SkyFindLevels (SkyRegion *db, SkyCoord c, int *Nregion) {

  SkyRegion **region;
  
  Ns = 0;
  Ne = 1;

  N = 0;
  NREGION = 10;
  ALLOCATE (region, SkyRegion *, NREGION);

  while (1) {
    No = -1;
    for (i = Ns; (No == -1) && (i < Ne); i++) {
      if (c.r < db[i].Rmin) continue;
      if (c.r > db[i].Rmax) continue;
      if (c.d < db[i].Dmin) continue;
      if (c.d > db[i].Dmax) continue;
      // got the needed region at this depth
      No = i;
    }
    if (No == -1) return ((SkyRegion *) NULL);

    region[N] = &db[No];
    N++;
    if (N == NREGION) {
      NREGION += 10;
      REALLOCATE (region, SkyRegion *, NREGION);
    }      

    /* need to check Ns, Ne, or guarantee valid range */
    if (db[No].child) {
      Ns = db[No].childS;
      Ne = db[No].childE;
    } else {
      *Nregion = N; 
      return (region);
    }
  }
}

/* find regions contained within rectangular region  c1 - c2 */
SkyRegion **SkyFindArea (SkyRegion *db, SkyCoord c1, SkyCoord c2, *nlist) {

  int Nlist;
  SkyRegion *list, *new, *sub;

  c1.r = ohana_normalize_angle (c1.r);
  c2.r = ohana_normalize_angle (c2.r);

  /* check on c1.r > c2.r : cross boundary */
  if (c1.r > c2.r) {
    c0 = c2; c0.r = 360.0;
    list1 = SkyFindArea (db, c1, c0, &Nlist1);
    c0 = c1; c0.r = 0.0;
    list2 = SkyFindArea (db, c0, c2, &Nlist2);
    Nlist = Nlist1 + Nlist2;
    ALLOCATE (list, SkyRegion *, Nlist);
    memcpy (&list[0], list1, Nlist1*sizeof());
    memcpy (&list[Nlist1], list2, Nlist2*sizeof());
    free (list1);
    free (list2);
    *nlist = Nlist;
    return (list);
  }    

  Nlist = 1;
  ALLOCATE (list, SkyRegion *, 1);
  list[0] = &db[0];
  getchild = db[0].child;

  while (getchild) {

    getchild = FALSE;
    Nnew = 0;
    NNEW = Nlist + 100;
    ALLOCATE (new, SkyRegion *, NNEW);

    for (i = 0; i < Nlist; i++) {
      if (list[i][0].child) {
	sub = SkyFindAreaDB(db, list[i], c1, c2, &Nsub);
	if (Nnew + Nsub == NNEW) {
	  NNEW += 100;
	  REALLOCATE (new, SkyRegion *, NNEW);
	}
	for (i = 0; i < Nsub; i++) {
	  getchild |= sub[i][0].child;
	  new[Nnew] = sub[i];
	  Nnew++;
	}
	free (sub);
      } else {
	new[Nnew] = list[i];
	Nnew ++;
	if (Nnew == NNEW) {
	  NNEW += 100;
	  REALLOCATE (new, SkyRegion *, NNEW);
	}
      }
    }

    free (list);
    list = new;
    Nlist = Nnew;
  }

  return (list);
  *nlist = Nlist;

}

/* ?? */
SkyRegion *SkyFindAreaDB (SkyRegion *db, SkyRegion *ref, SkyCoord c1, SkyCoord c2, int *Nregion) {

  SkyRegion **region;
  
  Ns = ref[0].childS;
  Ne = ref[0].childE;

  N = 0;
  NREGION = 100;
  ALLOCATE (region, SkyRegion *, NREGION);

  /* c1 min, c2 max */

  for (i = Ns; (No == -1) && (i < Ne); i++) {
    if (c2.r < db[i].Rmin) continue;
    if (c1.r > db[i].Rmax) continue;
    if (c2.d < db[i].Dmin) continue;
    if (c1.d > db[i].Dmax) continue;

    region[N] = &db[i];
    N++;
    if (N == NREGION) {
      NREGION += 100;
      REALLOCATE (region, SkyRegion *, NREGION);
    }      
  }
  *Nregion = N; 
  return (region);
}

SkyRegion *SkyBuildTable (SkyRegion *seed, int Nseed, int level, int depth) {

  /* given a table of Rmin, Dmin, Rmax, Dmax, name, at level 3, generate
     all higher levels and 'depth' extra levels */

  /* levels:
     0 - fullsky.cpt
     1 - n????.cpt / s????.cpt
     2 - r????.cpt
     3 - ????.cpt
     4 - ????.??.cpt
  */
  
  SkyRegion *db;

  /* allocate at least 30 for levels 0 & 1 */
  NREGION = 100;
  ALLOCATE (db, SkyRegion, NREGION);

  /* full sky */
  strcpy (db[0].name, "fullsky.cpt");
  db[0].Rmin =   0; db[0].Rmax = 360;
  db[0].Dmin = -90; db[0].Dmax =  90;
  db[0].depth = 0;
  db[0].child = FALSE;
  N = 1;

  db[0].childS = N;
  /* north dec bands */
  for (dec = 0; dec < 90; dec += 7.5) {
    myAssert (snprintf (db[N].name, 18, "n%04d.cpt", (int) 100*dec) < 18, "overflow");
    db[N].Rmin =   0; db[N].Rmax = 360;
    db[N].Dmin = dec; db[N].Dmax = dec + 7.5;
    db[N].depth = 1;
    db[N].child = FALSE;
    N++;
  }
  /* south dec bands */
  for (dec = 0; dec > -90; dec -= 7.5) {
    myAssert (snprintf (db[N].name, 18, "s%04d.cpt", (int) 100*dec) < 18, "overflow");
    db[N].Rmin =   0;       db[N].Rmax = 360;
    db[N].Dmin = dec - 7.5; db[N].Dmax = dec;
    db[N].depth = 1;
    db[N].child = FALSE;
    N++;
  }
  db[0].childE = N;

  /* subdivide dec bands based on seed */
  Rnumber = 0;
  childS = db[0].childS;
  childE = db[0].childE;
  for (i = childS; i < childE; i++) {

    Nnew = 0;
    NNEW = 100;
    ALLOCATE (new, SkyRegion, NNEW);

    for (j = 0; j < Nseed; j++) {
      if (seed[j].Rmin > db[i].Rmax) continue;
      if (seed[j].Rmax < db[i].Rmin) continue;
      if (seed[j].Dmin > db[i].Dmax) continue;
      if (seed[j].Dmax < db[i].Dmin) continue;
      
      for (k = 0; k < Nnew; k++) {
	if ((seed[j].Rmin == new[k].Rmin) && 
	    (seed[j].Rmax == new[k].Rmax)) { 
	  goto next_seed;
	}
	if ((seed[j].Rmin == new[k].Rmin) ^^ 
	    (seed[j].Rmax != new[k].Rmax)) { 
	  fprintf (stderr, "inconsistent blocks in seed file\n");
	  return (NULL);
	}
      }	
      new[Nnew].Rmin = seed[j].Rmin;
      new[Nnew].Rmax = seed[j].Rmax;
      new[Nnew].Dmin = db[i].Dmin;
      new[Nnew].Dmax = db[i].Dmax;
      new[Nnew].depth = 2;
      new[Nnew].child = FALSE;
      strncpy_nowarn (root, db[i].name, 5); 
      myAssert (snprintf (new[Nnew].name, 18, "%s/r%04d.cpt", root, Rnumber) < 18, "overflow");
      Rnumber ++;      
      Nnew ++;
      if (Nnew == NNEW) {
	NNEW += 100;
	REALLOCATE (new, SkyRegion, NNEW);
      }
    next_seed:
    }
    /* update db list */
    db[i].childS = N;
    db[i].childE = N + Nnew;
    NREGION = N + Nnew + 100;
    REALLOCATE (db, SkyRegion, NREGION);
    memcpy (&db[N], new, Nnew*sizeof(SkyRegion));
    free (new);
    N += Nnew;
  }

  /* subdivide ra strips based on seed */
  childS = db[db[0].childS].childS;
  childE = db[db[0].childE - 1].childE;
  nextS = N;
  for (i = childS; i < childE; i++) {

    Nnew = 0;
    NNEW = 100;
    ALLOCATE (new, SkyRegion, NNEW);

    for (j = 0; j < Nseed; j++) {
      if (seed[j].Rmin > db[i].Rmax) continue;
      if (seed[j].Rmax < db[i].Rmin) continue;
      if (seed[j].Dmin > db[i].Dmax) continue;
      if (seed[j].Dmax < db[i].Dmin) continue;
      
      for (k = 0; k < Nnew; k++) {
	if ((seed[j].Dmin == new[k].Dmin) && 
	    (seed[j].Dmax == new[k].Dmax)) { 
	  goto next_seed;
	}
	if ((seed[j].Dmin == new[k].Dmin) ^^ 
	    (seed[j].Dmax != new[k].Dmax)) { 
	  fprintf (stderr, "inconsistent blocks in seed file\n");
	  return (NULL);
	}
      }	
      new[Nnew].Dmin = seed[j].Dmin;
      new[Nnew].Dmax = seed[j].Dmax;
      new[Nnew].Rmin = db[i].Rmin;
      new[Nnew].Rmax = db[i].Rmax;
      new[Nnew].depth = 3;
      new[Nnew].child = FALSE;
      strcpy (new[Nnew].name, seed[j].name);
      Nnew ++;
      if (Nnew == NNEW) {
	NNEW += 100;
	REALLOCATE (new, SkyRegion, NNEW);
      }
    next_seed:
    }
    
    /* update db list */
    db[i].childS = N;
    db[i].childE = N + Nnew;
    NREGION = N + Nnew + 100;
    REALLOCATE (db, SkyRegion, NREGION);
    memcpy (&db[N], new, Nnew*sizeof(SkyRegion));
    free (new);
    N += Nnew;
  }
  nextE = N;

  /* subdivide entries once more */
  childS = nextS;
  childE = nextE;
  for (i = childS; i < childE; i++) {
    Rnumber = 0;
    db[i].childS = N;
    dDec = (db[i].Dmax - db[i].Dmin) / 5.0;
    dRa  = (db[i].Rmax - db[i].Rmin) / 5.0;
    for (nx = 0; nx < 5; nx++) {
      for (ny = 0; ny < 5; ny++) {
	db[N].Dmin = db[i].Dmin + dDec * (nx + 0);
	db[N].Dmax = db[i].Dmax + dDec * (nx + 1);
	db[N].Rmin = db[i].Rmin + dDec * (nx + 0);
	db[N].Rmax = db[i].Rmax + dDec * (nx + 1);
	db[N].depth = 3;
	db[N].child = FALSE;
	strncpy_nowarn (root, db[i].name, 10); 
	myAssert (snprintf (db[N].name, 18, "%s.%02d.cpt", root, Rnumber) < 18, "overflow");
	Rnumber ++;
	N ++;
	if (N == NREGION) {
	  NREGION += 100;
	  REALLOCATE (new, SkyRegion, NREGION);
	}
      }
    }
  }

}

/* region is pointer to entry in db */
SkyRegion *SkyExtend (SkyRegion *db, SkyRegion *region) {

}

/* region is pointer to entry in db */
SkyRegion *SkyContract (SkyRegion *db, SkyRegion *region) {

}

SkyRegion *SkyFindGCircle (SkyRegion *db, SkyCoord c1, SkyCoord c2) {

}


/* is entire db in memory, or do we load from disk for each operation? 
   10,000 at level 3, 100,000 @ 4 : 0.5MB, 5MB */


