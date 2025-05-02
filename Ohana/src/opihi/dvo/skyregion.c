# include "dvoshell.h"

// define the sky region for which extractions are limited
int skyregion (int argc, char **argv) {
  
  int N;

  // alternate call form: skyregion -box Rcenter Dcenter radius
  if ((N = get_argument (argc, argv, "-box"))) {
    if ((N != 1) && (argc != 5)) {
      gprint (GP_ERR, "USAGE: skyregion -box (Rcenter) (Dcenter) (radius)\n");
      gprint (GP_ERR, "  Rcenter, Dcenter, radius in decimal degrees\n");
      return (FALSE);
    }

    // argv[1] = -box
    double Rcenter = atof(argv[2]);
    double Dcenter = atof(argv[3]);
    double Radius  = atof(argv[4]);

    double Rmin = Rcenter - Radius/cos(DEG_RAD*Dcenter);
    double Rmax = Rcenter + Radius/cos(DEG_RAD*Dcenter);
    double Dmin = Dcenter - Radius;
    double Dmax = Dcenter + Radius;

    set_skyregion (Rmin, Rmax, Dmin, Dmax);
    return TRUE;
  }
  

  // dvo_client should have 2 standard arguments: -hostID and -hostdir
  int SaveRegion = FALSE;
  if ((N = get_argument (argc, argv, "-save"))) {
    remove_argument (N, &argc, argv);
    SaveRegion = TRUE;
  }

  if (argc == 1) {
    double Rmin, Rmax, Dmin, Dmax;
    get_skyregion(&Rmin, &Rmax, &Dmin, &Dmax);

    if (SaveRegion) {
      set_variable ("Rmin", Rmin);
      set_variable ("Rmax", Rmax);
      set_variable ("Dmin", Dmin);
      set_variable ("Dmax", Dmax);
      return TRUE;
    } else {
      gprint (GP_ERR, "current skyregion: %f - %f : %f - %f\n", Rmin, Rmax, Dmin, Dmax);
      gprint (GP_ERR, "USAGE:  skyregion (min RA) (max RA) (min DEC) (max DEC)\n");
      gprint (GP_ERR, "        skyregion -box (RA) (DEC) (radius) [sets Rmin,Rmax & Dmin,Dmax]\n");
      return FALSE;
    }
  }

  if (argc != 5) {
    gprint (GP_ERR, "USAGE: skyregion (min RA) (max RA) (min DEC) (max DEC)\n");
    return (FALSE);
  }

  set_skyregion (atof(argv[1]), atof(argv[2]), atof(argv[3]), atof(argv[4]));

  return (TRUE);
}

/* find region which overlaps c at given depth (-1 : populated ) */
int SkyRegionByPoint_r (SkyTable *table, SkyList *list, int depth, double ra, double dec) {
  
  int i, Ns, Ne, No;
  SkyRegion *skyregion;

  list[0].Nregions = 0;
  list[0].ownElements = FALSE; // this list is only holding a view to the elements

  skyregion = table[0].regions;

  Ns = 0;
  Ne = 1;

  while (1) {
    No = -1;
    for (i = Ns; (i < Ne) && (i < table[0].Nregions); i++) {
      if (ra  < skyregion[i].Rmin) continue;
      if (ra  > skyregion[i].Rmax) continue;
      if (dec < skyregion[i].Dmin) continue;
      if (dec > skyregion[i].Dmax) continue;
      No = i;
      break;
    }
    if (No == -1) return (FALSE);
    if ((depth == -1) && (skyregion[No].table)) break;
    if (depth == skyregion[No].depth) break;
    if ((depth > skyregion[No].depth) && !skyregion[No].child) return (FALSE);

    /* need to check Ns, Ne, or guarantee valid range */
    Ns = skyregion[No].childS;
    Ne = skyregion[No].childE;
  }

  list[0].regions[0] = &skyregion[No];
  list[0].filename[0] = table[0].filename[No];
  list[0].Nregions = 1;
  return (TRUE);
}

SkyList *SelectRegionsByCoordVectors (Vector *RA, Vector *DEC) {
  
  int i, j, Npts, Nout, NOUT, *found;
  double ra, dec;
  SkyList *new, *list;
  SkyTable *sky;

  sky = GetSkyTable ();

  Npts = RA->Nelements;
  ALLOCATE (found, int, Npts);
  memset (found, 0, Npts*sizeof(int));

  ALLOCATE (new, SkyList, 1);
  ALLOCATE (new[0].regions,  SkyRegion *, 1);
  ALLOCATE (new[0].filename,  char *, 1);
  new[0].Nregions = 0;
  new[0].ownElements = FALSE; // this list is only holding a view to the elements
  strcpy (new[0].hosts, sky[0].hosts);

  // output list
  Nout = 0;
  NOUT = 100;
  ALLOCATE (list, SkyList, 1);
  ALLOCATE (list[0].regions,  SkyRegion *, NOUT);
  ALLOCATE (list[0].filename,  char *, NOUT);
  list[0].Nregions = 0;
  list[0].ownElements = FALSE; // this list is only holding a view to the elements
  strcpy (list[0].hosts, sky[0].hosts);

  for (i = 0; i < Npts; i++) {
    if (found[i]) continue;

    SkyRegionByPoint_r (sky, new, -1, RA->elements.Flt[i], DEC->elements.Flt[i]);
    if (new->Nregions == 0) continue;
    assert (new->Nregions == 1);
    // append new region to output list
    list[0].regions[Nout] = new[0].regions[0];
    list[0].filename[Nout] = new[0].filename[0];
    Nout ++;
    if (Nout >= NOUT) {
	NOUT += 100;
	REALLOCATE (list[0].regions, SkyRegion *, NOUT);
	REALLOCATE (list[0].filename, char *, NOUT);
    }
    found[i] = TRUE;

    // scan over the remaining points to find any that lie within this region
    // if we sorted the ra,dec inputs we could break out of this more quickly
    for (j = i + 1; j < Npts; j++) {
      ra = RA->elements.Flt[j];
      dec = DEC->elements.Flt[j];
      if (ra  < new[0].regions[0][0].Rmin) continue;
      if (ra  > new[0].regions[0][0].Rmax) continue;
      if (dec < new[0].regions[0][0].Dmin) continue;
      if (dec > new[0].regions[0][0].Dmax) continue;
      found[j] = TRUE;
    }
  }
  list[0].Nregions = Nout;

  free (found);
  SkyListFree (new);

  return (list);
}

void sort_skylist_by_index (SkyList *list) {

# define SWAPFUNC(A,B){ \
    SkyRegion *tmp; tmp = list[0].regions[A]; list[0].regions[A] = list[0].regions[B]; list[0].regions[B] = tmp; \
    char *tmpname; tmpname = list[0].filename[A]; list[0].filename[A] = list[0].filename[B]; list[0].filename[B] = tmpname; \
}
# define COMPARE(A,B)(list[0].regions[A][0].index < list[0].regions[B][0].index)

  OHANA_SORT (list[0].Nregions, COMPARE, SWAPFUNC);

# undef SWAPFUNC
# undef COMPARE

}

SkyList *SkyListUniqueSubset (SkyList *input) {

  sort_skylist_by_index (input);
  // for (int i = 0; i < input->Nregions; i++) {
  //   fprintf (stderr, "idx: %s : %s : %d\n", input->regions[i][0].name, input->filename[i], input->regions[i][0].index);
  // }

  SkyList *list;
  
  int Nout = 0;
  int NOUT = input->Nregions;

  ALLOCATE (list, SkyList, 1);
  ALLOCATE (list[0].regions, SkyRegion *, NOUT);
  ALLOCATE (list[0].filename, char *, NOUT);
  list[0].ownElements = FALSE; // this list is only holding a view to the elements
  
  if (input->Nregions == 0) {
    list->Nregions = 0;
    return list;
  }

  list->regions[Nout] = input->regions[0];
  list->filename[Nout] = input->filename[0];
  Nout ++;

  for (int i = 0; i < input->Nregions; i++) {
    if (list->regions[Nout-1][0].index == input->regions[i][0].index) continue;
    list->regions[Nout] = input->regions[i];
    list->filename[Nout] = input->filename[i];
    Nout ++;
  }
  list->Nregions = Nout;
  REALLOCATE (list[0].regions, SkyRegion *, Nout);
  REALLOCATE (list[0].filename, char *, Nout);

  strcpy (list[0].hosts, input[0].hosts);

  // free the input list?
  return list;
}

// return a list of all regions covered by this list of coordinates and the given radius
SkyList *SelectRegionsByCoordVectorsAndRadius (Vector *RA, Vector *DEC, float Radius) {
  
  int Npts, Nout, NOUT, *found;
  double ra, dec;
  SkyList *list;
  SkyTable *sky;

  sky = GetSkyTable ();

  Npts = RA->Nelements;
  ALLOCATE (found, int, Npts);
  memset (found, 0, Npts*sizeof(int));

  // output list
  Nout = 0;
  NOUT = 100;
  ALLOCATE (list, SkyList, 1);
  ALLOCATE (list[0].regions,  SkyRegion *, NOUT);
  ALLOCATE (list[0].filename,  char *, NOUT);
  list[0].Nregions = 0;
  list[0].ownElements = FALSE; // this list is only holding a view to the elements
  strcpy (list[0].hosts, sky[0].hosts);

  for (int i = 0; i < Npts; i++) {
    if (found[i]) continue;

    SkyList *new = SkyListByRadius (sky, -1, RA->elements.Flt[i], DEC->elements.Flt[i], Radius);
    if (new->Nregions == 0) continue;

    // append new regions to output list
    if (Nout + new->Nregions >= NOUT) {
	NOUT += new->Nregions + 100;
	REALLOCATE (list[0].regions, SkyRegion *, NOUT);
	REALLOCATE (list[0].filename, char *, NOUT);
    }
    
    for (int n = 0; n < new->Nregions; n++) {
      list[0].regions[Nout] = new[0].regions[n];
      list[0].filename[Nout] = new[0].filename[n];
      Nout ++;
    }
    found[i] = TRUE;

    // scan over the remaining points to find any that lie within these regions
    // if we sorted the ra,dec inputs we could break out of this more quickly
    for (int j = i + 1; FALSE && (j < Npts); j++) {
      ra = RA->elements.Flt[j];
      dec = DEC->elements.Flt[j];
      for (int n = 0; n < new->Nregions; n++) {
	if (ra  < new[0].regions[n][0].Rmin) continue;
	if (ra  > new[0].regions[n][0].Rmax) continue;
	if (dec < new[0].regions[n][0].Dmin) continue;
	if (dec > new[0].regions[n][0].Dmax) continue;
	found[j] = TRUE;
      }
    }
    SkyListFree (new);
  }
  list[0].Nregions = Nout;

  // before I go any further, I need to strip down to a unique subset
  SkyList *uniq = SkyListUniqueSubset(list);

  free (found);

  return (uniq);
}

