# include "dvo.h"

/* each region is bounded on the sky by lines of constant RA & DEC.
   the region represents a specified depth, and it may or may not have
   children.  The start and end of the children in the array are childS and childE 
   If the region at the given depth is populated with an object table, then 'object' is TRUE.
   If the region at the given depth is populated with an image table, then 'image' is TRUE.
   I have defined no accelerators other than the table hierarchy */

/* given /path/file.ext return pointer to file.ext */
char *filebasename_ptr (char *name) {
 
  char *c;

  c = strrchr (name, '/');
  if (c) return (c+1);
  return name;
}

/* find region which corresponds to the given index */
SkyList *SkyRegionByIndex (SkyTable *table, int index) {
  
  SkyList *list;

  ALLOCATE (list, SkyList, 1);
  ALLOCATE (list[0].regions,  SkyRegion *, 1);
  ALLOCATE (list[0].filename,  char *, 1);
  list[0].Nregions = 0;
  list[0].ownElements = FALSE; // this list is only holding a view to the elements
  strcpy (list[0].hosts, table[0].hosts);

  list[0].regions[0] = &table[0].regions[index];
  list[0].filename[0] = table[0].filename[index];
  list[0].Nregions = 1;
  return (list);
}

/* find region which matches the named file */
SkyList *SkyRegionByCPT (SkyTable *table, char *filename) {
  
  int i;
  SkyList *list;

  ALLOCATE (list, SkyList, 1);
  ALLOCATE (list[0].regions,  SkyRegion *, 1);
  ALLOCATE (list[0].filename,  char *, 1);
  list[0].Nregions = 0;
  list[0].ownElements = FALSE; // this list is only holding a view to the elements
  strcpy (list[0].hosts, table[0].hosts);

  // i'd like to select the region from the given cpt filename, but
  // in a parallel environment, the filename does not match the canonical name
  // i select the basename to match 

  // we might have been given foo.cpt or just foo:
  char *tgtbase = filebasename_ptr (filename);
  int Ntgtbase = strlen(tgtbase);
  if ((Ntgtbase > 4) && (!strcmp (&tgtbase[Ntgtbase - 4], ".cpt"))) {
    Ntgtbase -= 4;
  }

  for (i = 0; i < table[0].Nregions; i++) {
    char *regbase = filebasename_ptr (table[0].regions[i].name);
    if (strncmp (regbase, tgtbase, Ntgtbase)) continue;

    list[0].regions[0] = &table[0].regions[i];
    list[0].filename[0] = table[0].filename[i];
    list[0].Nregions = 1;
    return (list);
  }
  
  free (list[0].regions);
  free (list[0].filename);
  free (list);
  return NULL;
}

/* find region which overlaps c at given depth (-1 : populated ) */
SkyList *SkyRegionByPoint (SkyTable *table, int depth, double ra, double dec) {
  
  int i, Ns, Ne, No;
  SkyRegion *region;
  SkyList *list;

  ALLOCATE (list, SkyList, 1);
  ALLOCATE (list[0].regions,  SkyRegion *, 1);
  ALLOCATE (list[0].filename,  char *, 1);
  list[0].Nregions = 0;
  list[0].ownElements = FALSE; // this list is only holding a view to the elements
  strcpy (list[0].hosts, table[0].hosts);

  region = table[0].regions;

  Ns = 0;
  Ne = 1;

  while (1) {
    No = -1;
    for (i = Ns; (i < Ne) && (i < table[0].Nregions); i++) {
      if (ra  < region[i].Rmin) continue;
      if (ra  > region[i].Rmax) continue;
      if (dec < region[i].Dmin) continue;
      if (dec > region[i].Dmax) continue;
      No = i;
      break;
    }
    if (No == -1) return (list);
    if ((depth == -1) && (region[No].table)) break;
    if (depth == region[No].depth) break;
    if ((depth > region[No].depth) && !region[No].child) return (list);

    /* need to check Ns, Ne, or guarantee valid range */
    Ns = region[No].childS;
    Ne = region[No].childE;
  }

  list[0].regions[0] = &region[No];
  if (table[0].filename) {
    list[0].filename[0] = table[0].filename[No];
  }
  list[0].Nregions = 1;
  return (list);
}

/* find regions at all levels which match name */
/* XXX : need to add support for selected level / populated level */
SkyList *SkyListByName (SkyTable *table, char *name) {

  int i, Nchar, N, NREGIONS;
  SkyList *list;
  SkyRegion *region;
  
  N = 0;
  NREGIONS = 10;
  ALLOCATE (list, SkyList, 1);
  ALLOCATE (list[0].regions, SkyRegion *, NREGIONS);
  ALLOCATE (list[0].filename, char *, NREGIONS);
  list[0].Nregions = N;
  list[0].ownElements = FALSE; // this list is only holding a view to the elements
  strcpy (list[0].hosts, table[0].hosts);

  region = table[0].regions;

  Nchar = strlen (name);

  for (i = 0; i < table[0].Nregions; i++) {
    if (strncasecmp (region[i].name, name, Nchar)) continue;

    list[0].regions[N] = &region[i];
    list[0].filename[N] = table[0].filename[i];
    N++;
    if (N >= NREGIONS) {
	NREGIONS += 10;
	REALLOCATE (list[0].regions, SkyRegion *, NREGIONS);
	REALLOCATE (list[0].filename, char *, NREGIONS);
    }
  }
  list[0].Nregions = N;
  return (list);
}

/* find regions at all levels which match list of names */
SkyList *SkyListMatchList (SkyList *inlist, char **cptlist, int Ncptlist) {

  int i, j;
  SkyList *list;
  
  int N = 0;
  int NREGIONS = 10;
  ALLOCATE (list, SkyList, 1);
  ALLOCATE (list[0].regions, SkyRegion *, NREGIONS);
  ALLOCATE (list[0].filename, char *, NREGIONS);
  list[0].Nregions = N;
  list[0].ownElements = FALSE; // this list is only holding a view to the elements
  strcpy (list[0].hosts, inlist[0].hosts);

  SkyRegion **regions = inlist[0].regions;

  for (i = 0; i < inlist[0].Nregions; i++) {
    int found = FALSE;
    for (j = 0; !found && (j < Ncptlist); j++) {
      int Nchar = strlen (cptlist[j]);
      if (strncasecmp (regions[i][0].name, cptlist[j], Nchar)) continue;

      list[0].regions[N] = regions[i];
      list[0].filename[N] = inlist[0].filename[i];
      N++;
      if (N >= NREGIONS) {
	NREGIONS += 10;
	REALLOCATE (list[0].regions, SkyRegion *, NREGIONS);
	REALLOCATE (list[0].filename, char *, NREGIONS);
      }
      found = TRUE;
    }
    // we do not care if one of the regions does not match.  do we care if any of the
    // cptfile do not match?
  }
  SkyListFree (inlist);

  list[0].Nregions = N;
  return (list);
}

/* find regions at all levels which overlap c */
SkyList *SkyListByPoint (SkyTable *table, double ra, double dec) {

  int i, Ns, Ne, No, N, NREGIONS;
  SkyList *list;
  SkyRegion *region;
  
  N = 0;
  NREGIONS = 10;
  ALLOCATE (list, SkyList, 1);
  ALLOCATE (list[0].regions,  SkyRegion *, NREGIONS);
  ALLOCATE (list[0].filename,  char *, NREGIONS);
  list[0].Nregions = N;
  list[0].ownElements = FALSE; // this list is only holding a view to the elements
  strcpy (list[0].hosts, table[0].hosts);

  region = table[0].regions;

  Ns = 0;
  Ne = 1;

  while (1) {
    No = -1;
    for (i = Ns; (i < Ne) && (i < table[0].Nregions); i++) {
      if (ra  < region[i].Rmin) continue;
      if (ra  > region[i].Rmax) continue;
      if (dec < region[i].Dmin) continue;
      if (dec > region[i].Dmax) continue;
      No = i;
      break;
    }
    if (No == -1) return (list);

    list[0].regions[N] = &region[No];
    list[0].filename[N] = table[0].filename[No];
    N++;
    if (N >= NREGIONS) {
	NREGIONS += 10;
	REALLOCATE (list[0].regions, SkyRegion *, NREGIONS);
	REALLOCATE (list[0].filename, char *, NREGIONS);
    }
    list[0].Nregions = N;

    /* need to check Ns, Ne, or guarantee valid range */
    if (region[No].child) {
      Ns = region[No].childS;
      Ne = region[No].childE;
    } else {
      return (list);
    }
  }
}

SkyList *SkyListByRadius (SkyTable *table, int depth, double RA, double DEC, double radius) {

  double rad;
  double Rmin, Rmax, Dmin, Dmax;
  SkyList *list;

  Dmin = DEC - radius;
  Dmax = DEC + radius;

  if ((Dmin <= -89) || (Dmax >= 89)) {
    Rmin = 0;
    Rmax = 360;
  } else {
    rad = MAX (radius / (cos(Dmin*RAD_DEG)), radius / (cos(Dmax*RAD_DEG)));
    Rmin = RA - rad;
    Rmax = RA + rad;
  }

  list = SkyListByBounds (table, depth, Rmin, Rmax, Dmin, Dmax);
  return (list);
}

SkyList *SkyListByPatch (SkyTable *table, int depth, SkyRegion *patch) {

  SkyList *list;

  list = SkyListByBounds (table, depth, patch[0].Rmin, patch[0].Rmax, patch[0].Dmin, patch[0].Dmax);
  return (list);
}

/* user must be careful about mosaic registration */
SkyList *SkyListByImage (SkyTable *table, int depth, Image *image) {

  int j;
  SkyList *list;
  double r, d, X[4], Y[4];
  double Rmin, Rmax, Dmin, Dmax;
  
  // XXX EAM : image/mosaic MUST be registered (if WRP) 
  SetImageCorners (X, Y, image);

  Rmin = 360.0;
  Rmax =   0.0;
  Dmin = +90.0;
  Dmax = -90.0;

  /* does this work at 0,360 boundary? XY_to_RD must return 
     coord offsets relative to CRVAL1,2 (ie, NOT renormalize) */
  for (j = 0; j < 4; j++) {
    /* XY_to_RD is two-level if ctype == WRP */
    XY_to_RD (&r, &d, X[j], Y[j], &image[0].coords);
    Rmin = MIN (Rmin, r);
    Rmax = MAX (Rmax, r);
    Dmin = MIN (Dmin, d);
    Dmax = MAX (Dmax, d);
  }

  list = SkyListByBounds (table, depth, Rmin, Rmax, Dmin, Dmax);

  // fprintf (stderr, "%s : %f - %f, %f - %f : %d\n", 
  // image->name, Rmin, Rmax, Dmin, Dmax, (int) list->Nregions);

  return (list);
}

// a region like -10 10 0 20 will become 350 10 0 20 and return all regions on both sides of 0,360
// a region like -10 370 should NOT become 350 10, but should instead become 0 360
SkyList *SkyListByBounds (SkyTable *table, int depth, double Rmin, double Rmax, double Dmin, double Dmax) {

  int i, j, Ns;
  SkyList *list, *extra;

  if (Rmax - Rmin > 360.0) {
    Rmin =   0.0;
    Rmax = 360.0;
  }

  Rmin = ohana_normalize_angle (Rmin);
  Rmax = ohana_normalize_angle (Rmax);

  /* handle 0,360 boundary requests */
  /* this is probably wrong: I will get duplicates for all Dec bands... */
  if (Rmin > Rmax) {
    list = SkyListChildrenByBounds (table, -1, depth, 0.0, Rmax, Dmin, Dmax);

    extra = SkyListChildrenByBounds (table, -1, depth, Rmin, 360.0, Dmin, Dmax);
    REALLOCATE (list[0].regions, SkyRegion *, list[0].Nregions + extra[0].Nregions);
    REALLOCATE (list[0].filename, char *, list[0].Nregions + extra[0].Nregions);
    Ns = list[0].Nregions;
    for (i = 0; i < extra[0].Nregions; i++) {
      // search for pre-existing match
      for (j = 0; j < list[0].Nregions; j++) {
	if (list[0].regions[j] == extra[0].regions[i]) {
	  goto skip;
	}
      }
      list[0].regions[Ns] = extra[0].regions[i];
      list[0].filename[Ns] = extra[0].filename[i];
      Ns ++;
    skip:
      continue;
    }
    list[0].Nregions = Ns;
    SkyListFree (extra);
  } else {
    list = SkyListChildrenByBounds (table, -1, depth, Rmin, Rmax, Dmin, Dmax);
  }

  return (list);
}

SkyList *SkyListChildrenByBounds (SkyTable *table, int No, int depth, double Rmin, double Rmax, double Dmin, double Dmax) {

  int i, j, Ns, Ne, Nnew, NNEW;
  int append;
  SkyList *children;
  SkyList *list;
  SkyRegion *region;

  Nnew = 0;
  NNEW = 50;
  ALLOCATE (list, SkyList, 1);
  ALLOCATE (list[0].regions, SkyRegion *, NNEW);
  ALLOCATE (list[0].filename, char *, NNEW);
  list[0].ownElements = FALSE; // this list is only holding a view to the elements
  strcpy (list[0].hosts, table[0].hosts);

  region = table[0].regions;

  if (No == -1) {
    Ns = 0;
    Ne = 1;
  } else {
    Ns = region[No].childS;
    Ne = region[No].childE;
  }

  for (i = Ns; (i < Ne) && (i < table[0].Nregions); i++) {
    if (Rmax <= region[i].Rmin) continue;
    if (Rmin >= region[i].Rmax) continue;
    if (Dmax <= region[i].Dmin) continue;
    if (Dmin >= region[i].Dmax) continue;

    if ((depth > region[i].depth) && (region[i].child == FALSE)) continue;

    append = FALSE;
    append |= (depth > region[i].depth) && (region[i].child == TRUE);
    append |= (depth ==             -1) && (region[i].table == FALSE);

    if (append) {
      /* append children to new */
      children = SkyListChildrenByBounds (table, i, depth, Rmin, Rmax, Dmin, Dmax);
      if (Nnew + children[0].Nregions >= NNEW) {
	NNEW = Nnew + children[0].Nregions + 50;
	REALLOCATE (list[0].regions, SkyRegion *, NNEW);
	REALLOCATE (list[0].filename, char *, NNEW);
      }
      for (j = 0; j < children[0].Nregions; j++) {
	list[0].regions[Nnew + j] = children[0].regions[j];
	list[0].filename[Nnew + j] = children[0].filename[j];
      }
      Nnew += children[0].Nregions;
      SkyListFree (children);
    } else {
      list[0].regions[Nnew] = &region[i];
      list[0].filename[Nnew] = table[0].filename[i];
      Nnew ++;
      if (Nnew >= NNEW) {
	  NNEW += 50;
	  REALLOCATE (list[0].regions, SkyRegion *, NNEW);
	  REALLOCATE (list[0].filename, char *, NNEW);
      }
    }
  }

  list[0].Nregions = Nnew;
  return (list);
}

/* find region which overlaps c at given depth (-1 : populated ) */
SkyList *SkyRegionByPoint_List (SkyList *inList, int depth, double ra, double dec) {
  
  int i;
  SkyRegion **region;
  SkyList *list;

  ALLOCATE (list, SkyList, 1);
  ALLOCATE (list[0].regions,  SkyRegion *, 1);
  ALLOCATE (list[0].filename,  char *, 1);
  list[0].Nregions = 0;
  list[0].ownElements = FALSE; // this list is only holding a view to the elements
  strcpy (list[0].hosts, inList[0].hosts);

  region = inList[0].regions;

  for (i = 0; i < inList[0].Nregions; i++) {

    // once we pass our desired depth, quit
    if ((depth > -1) && (depth < region[i][0].depth)) break;

    // skip tables that do not overlap 
    if (ra  < region[i][0].Rmin) continue;
    if (ra  > region[i][0].Rmax) continue;
    if (dec < region[i][0].Dmin) continue;
    if (dec > region[i][0].Dmax) continue;

    // skip tables that are not at our depth
    if ((depth >  -1) && (depth > region[i][0].depth)) continue;
    if ((depth == -1) && (region[i][0].table == FALSE)) continue;
    
    list[0].regions[0] = region[i];
    list[0].filename[0] = inList[0].filename[i];
    list[0].Nregions = 1;
    break;
  }
  return (list);
}

SkyList *SkyListByBounds_List (SkyList *table, int depth, double Rmin, double Rmax, double Dmin, double Dmax) {

  int i, j, Ns;
  SkyList *list, *extra;

  Rmin = ohana_normalize_angle (Rmin);
  Rmax = ohana_normalize_angle (Rmax);

  /* handle 0,360 boundary requests */
  /* this is probably wrong: I will get duplicates for all Dec bands... */
  if (Rmin > Rmax) {
    list = SkyListChildrenByBounds_List (table, depth, 0.0, Rmax, Dmin, Dmax);

    extra = SkyListChildrenByBounds_List (table, depth, Rmin, 360.0, Dmin, Dmax);
    REALLOCATE (list[0].regions, SkyRegion *, list[0].Nregions + extra[0].Nregions);
    REALLOCATE (list[0].filename, char *, list[0].Nregions + extra[0].Nregions);
    Ns = list[0].Nregions;
    for (i = 0; i < extra[0].Nregions; i++) {
      // search for pre-existing match
      for (j = 0; j < list[0].Nregions; j++) {
	if (list[0].regions[j] == extra[0].regions[i]) {
	  goto skip;
	}
      }
      list[0].regions[Ns] = extra[0].regions[i];
      list[0].filename[Ns] = extra[0].filename[i];
      Ns ++;
    skip:
      continue;
    }
    list[0].Nregions = Ns;
    SkyListFree (extra);
  } else {
    list = SkyListChildrenByBounds_List (table, depth, Rmin, Rmax, Dmin, Dmax);
  }

  return (list);
}

SkyList *SkyListChildrenByBounds_List (SkyList *table, int depth, double Rmin, double Rmax, double Dmin, double Dmax) {

  int i, Nnew, NNEW;
  SkyList *list;
  SkyRegion **region;

  Nnew = 0;
  NNEW = 50;
  ALLOCATE (list, SkyList, 1);
  ALLOCATE (list[0].regions, SkyRegion *, NNEW);
  ALLOCATE (list[0].filename, char *, NNEW);
  list[0].ownElements = FALSE; // this list is only holding a view to the elements
  strcpy (list[0].hosts, table[0].hosts);

  region = table[0].regions;

  // can we assume the skylist is globally sorted by depth?  i think so...
  for (i = 0; i < table[0].Nregions; i++) {
    
    // once we pass our desired depth, quit
    if ((depth > -1) && (depth < region[i][0].depth)) break;

    // skip tables that do not overlap 
    if (Rmax <= region[i][0].Rmin) continue;
    if (Rmin >= region[i][0].Rmax) continue;
    if (Dmax <= region[i][0].Dmin) continue;
    if (Dmin >= region[i][0].Dmax) continue;

    // skip tables that are not at our depth
    if ((depth >  -1) && (depth > region[i][0].depth)) continue;
    if ((depth == -1) && (region[i][0].table == FALSE)) continue;

    list[0].regions[Nnew] = region[i];
    list[0].filename[Nnew] = table[0].filename[i];
    Nnew ++;
    if (Nnew >= NNEW) {
      NNEW += 50;
      REALLOCATE (list[0].regions, SkyRegion *, NNEW);
      REALLOCATE (list[0].filename, char *, NNEW);
    }
  }

  list[0].Nregions = Nnew;
  return (list);
}

/* XXX EAM : this should go elsewhere (libdvo?) */
void SetImageCorners (double *X, double *Y, Image *image) {

  if (!strcmp(&image[0].coords.ctype[4], "-DIS")) {
    X[0] = -0.5*image[0].NX; Y[0] = -0.5*image[0].NY;
    X[1] = +0.5*image[0].NX; Y[1] = -0.5*image[0].NY;
    X[2] = +0.5*image[0].NX; Y[2] = +0.5*image[0].NY;
    X[3] = -0.5*image[0].NX; Y[3] = +0.5*image[0].NY;
  } else {
    X[0] = 0;           Y[0] = 0;
    X[1] = image[0].NX; Y[1] = 0;
    X[2] = image[0].NX; Y[2] = image[0].NY;
    X[3] = 0;           Y[3] = image[0].NY;
  }
}

int SkyTableSetDepth (SkyTable *sky, int depth) {

  int i;

  for (i = 0; i < sky[0].Nregions; i++) {
    sky[0].regions[i].table = (sky[0].regions[i].depth == depth);
  }
  return (TRUE);
}

int SkyListFree (SkyList *list) {

  int i;

  /* XXX do we need to free the filename array as well? */
  if (list == NULL) return (TRUE);
  if (list[0].regions != NULL) {
    if (list[0].ownElements) {
      for (i = 0; i < list[0].Nregions; i++) {
	if (list[0].filename[i] != NULL) {
	  free (list[0].filename[i]);
	}
	free (list[0].regions[i]);
      }
    }
    free (list[0].regions);
    free (list[0].filename);
  }
  free (list);
  list = NULL;
  return (TRUE);
}

int SkyTableFree (SkyTable *table) {

  int i;

  if (table == NULL) return (TRUE);
  if (table[0].filename != NULL) {
    for (i = 0; i < table[0].Nregions; i++) {
      if (table[0].filename[i] != NULL) {
	free (table[0].filename[i]);
      }
    }
    free (table[0].filename);
  }
  if (table[0].regions != NULL) {
    free (table[0].regions);
  }
  free (table);
  table = NULL;
  return (TRUE);
}

int SkyListMerge (SkyList **outlist, SkyList *newlist) {

    int i, j, skip, Nlist, Ntotal;
    SkyList *list;
    
    if (*outlist == NULL) {
	ALLOCATE (list, SkyList, 1);
	ALLOCATE (list[0].regions, SkyRegion *, 1);
	ALLOCATE (list[0].filename, char *, 1);
	list[0].ownElements = FALSE; // this list is only holding a view to the elements
	list[0].Nregions = 0;
	*outlist = list;
    } else {
	list = *outlist;
    }

    Ntotal = list[0].Nregions + newlist[0].Nregions;
    REALLOCATE (list[0].regions, SkyRegion *, Ntotal);
    REALLOCATE (list[0].filename, char *, Ntotal);

    Nlist = list[0].Nregions;
    for (i = 0; i < newlist[0].Nregions; i++) {
	/* check existing list to see if we already have this region */
	skip = FALSE;
	for (j = 0; j < list[0].Nregions; j++) {
	    if (list[0].regions[j] == newlist[0].regions[i]) skip = TRUE;
	}
	if (skip) continue;
	list[0].regions[Nlist] = newlist[0].regions[i];
	list[0].filename[Nlist] = newlist[0].filename[i];
	Nlist++;
    }
    list[0].Nregions = Nlist;
    REALLOCATE (list[0].regions, SkyRegion *, Nlist);
    REALLOCATE (list[0].filename, char *, Nlist);

    return (TRUE);
}
