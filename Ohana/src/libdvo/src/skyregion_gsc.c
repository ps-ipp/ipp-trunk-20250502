# include "dvo.h"
# define NDECBANDS 24
# define NDIV 4
# define DEBUG 0

static int DecLines[] = {593, 584, 551, 530, 522, 465, 406, 362, 280, 198, 123, 25, 597, 578, 574, 577, 534, 499, 442, 376, 294, 212, 144, 48};
static double DecMin[] = {0.0, +7.5, +15.0, +22.5, +30.0, +37.5, +45.0, +52.5, +60.0, +67.5, +75.0, +82.5, -7.5, -15.0, -22.5, -30.0, -37.5, -45.0, -52.5, -60.0, -67.5, -75.0, -82.5, -90.0};
static double DecMax[] = {+7.5, +15.0, +22.5, +30.0, +37.5, +45.0, +52.5, +60.0, +67.5, +75.0, +82.5, +90.0, 0.0, -7.5, -15.0, -22.5, -30.0, -37.5, -45.0, -52.5, -60.0, -67.5, -75.0, -82.5};
static char *DecNames[] = {"n0000", "n0730", "n1500", "n2230", "n3000", "n3730", "n4500", "n5230", "n6000", "n6730", "n7500", "n8230", "s0000", "s0730", "s1500", "s2230", "s3000", "s3730", "s4500", "s5230", "s6000", "s6730", "s7500", "s8230"};

// L0 : full sky
// L1 : Dec bands
// L2 : RA segments
// L3 : GSC regions
// L4 : GSC subdivisions

// a zone contains a set of L3 regions of the same size
typedef struct {
  int L1band;   // parent band
  int L3start;  // start of zone in L1 band
  int L3end;    // end of zone in L1 band (last + 1)
  float dDec;   // height of L3 region in zone
  float Rmin;
  float Rmax;
  float Dmin;
  float Dmax;
  int Nset;
} SkyRegionZone;

SkyTable *SkyRegionForDecBand (char *buffer, int Nregions, char *DecName, float Dmin, float Dmax);
SkyRegionZone *SkyRegionFindZones (SkyTable *band, int *Nzones, int parent);
void SkyTableL2fromZone (SkyTable *L2, SkyTable *L3, SkyTable *L4, SkyTable *band, SkyRegionZone *zone, int parent);
void SkyTableL3fromL2 (SkyRegion *L2, SkyTable *L3, SkyTable *L4, SkyTable *band, int Ns, int Ne);
void SkyTableL4fromL3 (SkyRegion *L3, SkyTable *L4);
void SkyTableL5fromL4 (SkyRegion *L4, SkyTable *L5);

void SkyTableSort (SkyTable *table);
void SkyTableAppend (SkyTable *old, SkyTable *new, int Nprev);
int NsetForDecRange (float dDec);
void SkyRegionPrint (SkyRegion *region);

void SkyTableExtend (SkyTable *old, SkyTable *new, int depth);
void SkyTableL5fromL4_List (SkyRegion *L4, SkyTable *L5, int Nfirst);

void SkyTableValidate (SkyTable *table);

SkyTable *SkyTableFromGSC (char *filename, int depth, int VERBOSE) {

  int i, j, skipLines, Nzones;

  Header theader;
  FTable ftable;
  SkyTable *skytable;
  SkyTable L0, L1, L2, L3, L4;
  SkyRegionZone *zones;

  FILE *f;
  
  // ohana_memdump (0);

  if (filename == NULL) {
    if (VERBOSE) fprintf (stderr, "template GSC Region file not defined\n");
    return (NULL);
  }

  f = fopen (filename, "r");
  if (f == NULL) {
    if (VERBOSE) fprintf (stderr, "can't find GSC Region file %s\n", filename);
    return (NULL);
  }

/* load in table data */
  ftable.header = &theader;
  if (!gfits_fread_ftable (f, &ftable, "REGIONS")) {
    if (VERBOSE) fprintf (stderr, "can't read GSC Region table\n");
    fclose (f);
    return (NULL);
  }

/* generate the regions for each level independently. indices (parent,childE,childS) 
   refer to the value within the independent group.  at the end, we combine and adjust
   the indices */

/* L0 : full sky */
  L0.Nregions = 1;
  L0.Nalloc = 1;
  ALLOCATE (L0.regions, SkyRegion, L0.Nalloc);
  L0.regions[0].Rmin 	=   0;
  L0.regions[0].Rmax 	= 360;
  L0.regions[0].Dmin 	= -90;
  L0.regions[0].Dmax 	= +90;
  L0.regions[0].index  	=  0;
  L0.regions[0].depth  	=  0;
  L0.regions[0].parent 	= -1;
  L0.regions[0].child  	=  TRUE;
  L0.regions[0].table  	= -1;
  L0.regions[0].childS  =  0;
  L0.regions[0].childE  =  NDECBANDS;
  L0.regions[0].hostFlags = 0;
  L0.regions[0].hostID    = 0;
  L0.regions[0].backupID  = 0;
  strcpy (L0.regions[0].name, "fullsky");
  if (DEBUG) SkyRegionPrint (&L0.regions[0]);

  /* allocate space for all levels */
  L1.Nregions = L2.Nregions = L3.Nregions = L4.Nregions = 0;
  L1.Nalloc = NDECBANDS;         ALLOCATE (L1.regions, SkyRegion, L1.Nalloc);
  L2.Nalloc = NDECBANDS*0x10;    ALLOCATE (L2.regions, SkyRegion, L2.Nalloc);
  L3.Nalloc = NDECBANDS*0x100;   ALLOCATE (L3.regions, SkyRegion, L3.Nalloc);
  L4.Nalloc = NDECBANDS*0x1000;  ALLOCATE (L4.regions, SkyRegion, L4.Nalloc);

  // skipLines = 0;
  // for (i = 0; i < 16; i++) skipLines += DecLines[i];
  // for (i = 16; i < 17; i++) {

  /* L1 : dec bands */
  skipLines = 0;
  for (i = 0; i < NDECBANDS; i++) {
    L1.regions[i].Rmin     = 0;
    L1.regions[i].Rmax     = 360;
    L1.regions[i].Dmin     = DecMin[i];
    L1.regions[i].Dmax     = DecMax[i];
    L1.regions[i].index    = i;
    L1.regions[i].depth    = 1;
    L1.regions[i].parent   = 0;
    L1.regions[i].child    = TRUE;
    L1.regions[i].table    = -1;
    L1.regions[i].hostFlags = 0;
    L1.regions[i].hostID    = 0;
    L1.regions[i].backupID  = 0;
    strcpy (L1.regions[i].name, DecNames[i]);
    if (DEBUG) SkyRegionPrint (&L1.regions[i]);

    /* build the L2 regions for this L1 region (based on zones) */
    L1.regions[i].childS   = L2.Nregions;

    /* load all GSC Regions in this band */
    SkyTable *band = SkyRegionForDecBand (&ftable.buffer[skipLines*48], DecLines[i], DecNames[i], L1.regions[i].Dmin, L1.regions[i].Dmax);
    skipLines += DecLines[i];

    /* sort the band regions by Rmin */
    SkyTableSort (band);
    
    // free zones below
    zones = SkyRegionFindZones (band, &Nzones, i);

    /* subdivide each zone */
    for (j = 0; j < Nzones; j++) {
      if (DEBUG >= 1) fprintf (stderr, "zone: %d : %f - %f : %f - %f\n", j, zones[j].Rmin, zones[j].Rmax, zones[j].Dmin, zones[j].Dmax);
      SkyTableL2fromZone (&L2, &L3, &L4, band, &zones[j], i);
    }
    free (zones);
    SkyTableFree (band);

    /* at end of loop, L2.Nregions has been updated to end of L2.regions for L1.region */
    L1.regions[i].childE   = L2.Nregions;

    L1.Nregions ++;
    myAssert (L1.Nregions <= NDECBANDS, "too many L1 regions");
  }

  /* merge L1 into L0 */
  ALLOCATE (skytable, SkyTable, 1);
  ALLOCATE (skytable[0].regions, SkyRegion, 1);
  skytable[0].Nregions = 0;

  skytable[0].filename = NULL;

  // define the host table filename
  strcpy (skytable->hosts, "HostTable.dat");

  // L0, L1, L2, L3, L4 all have index ranges of 0 -> Nregions and parent values pointing
  // at the index of the parent.  SkyTableAppend updates these values based on the
  // cumulative count

  SkyTableAppend (skytable, &L0, 0);
  SkyTableAppend (skytable, &L1, skytable[0].Nregions - L0.Nregions);
  SkyTableAppend (skytable, &L2, skytable[0].Nregions - L1.Nregions);
  SkyTableAppend (skytable, &L3, skytable[0].Nregions - L2.Nregions);
  SkyTableAppend (skytable, &L4, skytable[0].Nregions - L3.Nregions);

  SkyTableValidate (skytable);

  free (L0.regions);
  free (L1.regions);
  free (L2.regions);
  free (L3.regions);
  free (L4.regions);
  gfits_free_header (&theader);
  gfits_free_table (&ftable);

  // XXX TEST : for L4 entries in a certain ra,dec range, generate L5 entries:
  // NOTE : this code was just for testing.  Use dvosplitsky on an L4 catdir to
  // update SkyTable.fits then run dvosplit to move from L4 to L5.
# if (0)
  SkyTable L5; 
  L5.Nregions = 0;
  L5.Nalloc   = 1000;
  ALLOCATE (L5.regions, SkyRegion, L5.Nalloc);

  for (i = 0; i < L4.Nregions; i++) {
    // only work on a specific square region:
    if (L4.regions[i].Rmin <  9.5) continue;
    if (L4.regions[i].Rmin > 10.5) continue;
    if (L4.regions[i].Dmin < 40.0) continue;
    if (L4.regions[i].Dmin > 42.0) continue;
    
    SkyTableL5fromL4 (&L4.regions[i], &L5);
  }

  SkyTableAppend (skytable, &L5, skytable[0].Nregions - L4.Nregions);
  free (L5.regions);
# endif

  /* fix the depth elements and create the filename array */
  ALLOCATE (skytable[0].filename, char *, skytable[0].Nregions);
  for (i = 0; i < skytable[0].Nregions; i++) {
    skytable[0].filename[i] = NULL;
    skytable[0].regions[i].table = (skytable[0].regions[i].depth == depth);
  }

  // XXX TEST : for L4 entries in a certain ra,dec range, generate L5 entries:
# if (0)
  SkyTable L5; 
  L5.Nregions = 0;
  L5.Nalloc   = 1000;
  ALLOCATE (L5.regions, SkyRegion, L5.Nalloc);

  // get a list of the SkyRegions which overlap the point at the depth (only 1)
  SkyList *skylist = SkyRegionByPoint (skytable, 4, 10.0, 40.0);

  for (i = 0; i < skylist->Nregions; i++) {
    SkyTableL5fromL4_List (skylist->regions[i], &L5, skytable->Nregions);
  }

  SkyTableExtend (skytable, &L5, depth);
  free (L5.regions);
# endif

  // ohana_memdump (0);
  return (skytable);
}

// allocates band, band->regions
SkyTable *SkyRegionForDecBand (char *buffer, int Nregions, char *DecName, float DminBand, float DmaxBand) {

  int i;
  double Rmin, Rmax, Dmin, Dmax;
  char temp[50], name[80];
  SkyTable *band;
  SkyRegion *regions;

  ALLOCATE (band, SkyTable, 1);
  ALLOCATE (regions, SkyRegion, Nregions);
  for (i = 0; i < Nregions; i++) {
    strncpy_nowarn (temp, &buffer[i*48], 48);
    hstgsc_hms_to_deg (&Rmin, &Rmax, &Dmin, &Dmax, &temp[7]);
    if (Dmax < Dmin) SWAP (Dmin, Dmax);

    /* check that we are in correct Dec band */
    if ((Dmax < DminBand) || (Dmin > DmaxBand)) {
      fprintf (stderr, "error with raw table: table mis-match\n");
      fprintf (stderr, "line: %s\n", temp);
      fprintf (stderr, "region %d: %f - %f vs %f - %f\n", i, Dmin, Dmax, DminBand, DmaxBand);
      exit (2);
    }

    /* regions near 0,360 boundary have Rmax == 0.0 */
    if (Rmax < Rmin) Rmax += 360.0;

    regions[i].Dmin = Dmin;
    regions[i].Dmax = Dmax;
    regions[i].Rmin = Rmin;
    regions[i].Rmax = Rmax;

    temp[5] = 0;
    snprintf (name, 80, "%s/%s", DecName, &temp[1]);
    strcpy (regions[i].name, name);
  }
  band[0].filename = NULL;
  band[0].regions = regions;
  band[0].Nregions = Nregions;
  band[0].Nalloc   = Nregions;
  return (band);
}

/* sort skytable by Rmin */
void SkyTableSort (SkyTable *table) {

  SkyRegion *regions;
  char **filename = NULL;

  regions = table[0].regions;
  filename = table[0].filename;

# define SWAPFUNC(A,B){ \
  if (table[0].filename) { \
    char     *tempfile = filename[A]; filename[A] = filename[B]; filename[B] = tempfile; \
  } \
  SkyRegion tempregion = regions[A]; regions[A] = regions[B]; regions[B] = tempregion; \
}
# define COMPARE(A,B)(regions[A].Rmin < regions[B].Rmin)

  OHANA_SORT (table[0].Nregions, COMPARE, SWAPFUNC);

# undef SWAPFUNC
# undef COMPARE
}

/* given a complete set of regions in a Dec band, subdivide into zones of equal-sized regions.
 * this function assumes the DEC band is sliced first by lines of constant RA, then subdivided
 * with lines of constant DEC.  Groups ('zones') of these smaller boxes have the same size in
 * delta-Dec.  We are trying to fine the boundaries of these zones.  The initial assumption
 * breaks down for the pole region (86.25 < DEC < 90), where the entire range is divided into a
 * single circular patch.  we need to treat it differently.
 */

// allocates zones
SkyRegionZone *SkyRegionFindZones (SkyTable *band, int *Nzones, int parent) {
  
  float Dmin, Dmax, dDec, dDecZone;
  int i, j, Nz, NZ, Nregions, poleRegion;
  SkyRegion *regions;
  SkyRegionZone *zones;
  char basename[64];
  
  regions = band[0].regions;
  Nregions = band[0].Nregions;

  NZ = 100;
  ALLOCATE (zones, SkyRegionZone, NZ);
 
  /* the pole regions need to be separately handled.  skip in analysis below */
  poleRegion = 0;

  /* search for transitions in the region dDec, find the max Dec range */
  Nz = -1;
  dDecZone = -1;
  Dmin = +90;
  Dmax = -90;
  for (i = 0; i < Nregions; i++) {
    if ((regions[i].Dmin >= +86.00) || (regions[i].Dmax <= -86.00)) {
      /* drop the pole region (re-generated below) */
      poleRegion = SIGN(regions[i].Dmin);
      Nregions --;
      for (j = i; j < Nregions; j++) {
	regions[j] = regions[j+1];
      }
      i--;
      continue;
    }
    /* is this region in the current zone? */
    Dmin = MIN (regions[i].Dmin, Dmin);
    Dmax = MAX (regions[i].Dmax, Dmax);
    dDec = regions[i].Dmax - regions[i].Dmin;
    if (fabs(dDec - zones[Nz].dDec) > 0.001) {
      /* we've found the end of the current zone; set the ending info */
      if (Nz >= 0) {
	zones[Nz].L3end = i;
	zones[Nz].Rmax = regions[i-1].Rmax;
      }

      /* go to the next zone */
      Nz++;
      CHECK_REALLOCATE (zones, SkyRegionZone, NZ, Nz, 100);

      /* start info for the new zone */
      dDecZone = dDec;
      zones[Nz].Rmin = regions[i].Rmin;
      zones[Nz].dDec = dDecZone;
      zones[Nz].L3start = i;
      zones[Nz].L1band = parent;
      zones[Nz].Nset = NsetForDecRange (zones[Nz].dDec);
    }
  }
  zones[Nz].L3end = i;
  zones[Nz].Rmax = regions[i-1].Rmax;
  Nz ++;

  for (i = 0; i < Nz; i++) {
    zones[i].Dmin = Dmin;
    zones[i].Dmax = Dmax;
  }
  
  /* the pole region is mis-allocated in the gsc table to L3.  elevate it 
     to L2 (zone) and subdivide */
  if (poleRegion) {
    CHECK_REALLOCATE (zones, SkyRegionZone, NZ, Nz, 10);
    /* pole region @ L2 */
    zones[Nz].Rmin = 0.0;
    zones[Nz].Rmax = 360.0;
    if (poleRegion > 0) {
      zones[Nz].Dmin = +86.25;
      zones[Nz].Dmax = +90.00;
      strcpy (basename, "n8230/pole");
    } else {
      zones[Nz].Dmin = -90.00;
      zones[Nz].Dmax = -86.25;
      strcpy (basename, "s8230/pole");
    }
    zones[Nz].dDec = 90.00 - 86.25;
    zones[Nz].L3start = Nregions;
    zones[Nz].L3end = Nregions + 5;
    zones[Nz].L1band = parent;

    band[0].Nregions += 4;
    REALLOCATE (band[0].regions, SkyRegion, band[0].Nregions);
    for (i = 0; i < 4; i++) {
      band[0].regions[Nregions + i].Rmin = i * 90.0;
      band[0].regions[Nregions + i].Rmax = i * 90.0 + 90.0;
      if (poleRegion > 0) {
	band[0].regions[Nregions + i].Dmin = +86.250;
	band[0].regions[Nregions + i].Dmax = +88.125;
      } else {
	band[0].regions[Nregions + i].Dmin = -88.125;
	band[0].regions[Nregions + i].Dmax = -86.250;
      }
      myAssert (snprintf (band[0].regions[Nregions + i].name, 18, "%s.%d", basename, i) < 18, "overflow");
    }
    band[0].regions[Nregions + i].Rmin = 0.0;
    band[0].regions[Nregions + i].Rmax = 360.0;
    if (poleRegion > 0) {
      band[0].regions[Nregions + i].Dmin = +88.125;
      band[0].regions[Nregions + i].Dmax = +90.000;
    } else {
      band[0].regions[Nregions + i].Dmin = -90.000;
      band[0].regions[Nregions + i].Dmax = -88.125;
    }
    myAssert (snprintf (band[0].regions[Nregions + i].name, 18, "%s.%d", basename, i) < 18, "overflow");

    zones[0].Nset = 6;
    zones[1].Nset = 5;

    Nz ++;
  }

  *Nzones = Nz;
  return (zones);
}

int NsetForDecRange (float dDec) {

  int Ndiv, Nset;
  
  /* max segment size is ~7.5 x 15 degrees */
  Ndiv = (int) (0.5 + 7.5 / dDec);
  Nset = 2*Ndiv*Ndiv;
  return (Nset);
}
  
// memory neutral
void SkyTableL2fromZone (SkyTable *L2, SkyTable *L3, SkyTable *L4, SkyTable *band, SkyRegionZone *zone, int parent) {

  int i, Ns, Ne;
  char *p, name[80];

  int Nr = L2[0].Nregions;
  CHECK_REALLOCATE (L2[0].regions, SkyRegion, L2[0].Nalloc, L2[0].Nregions, 100);
  
  /* divide this zone into L2 regions with Nset L3 regions each (fewer on ends) */
  for (i = zone[0].L3start; i < zone[0].L3end; i += zone[0].Nset) {
    Ns = i; 
    Ne = MIN (i + zone[0].Nset, zone[0].L3end);

    L2[0].regions[Nr].Rmin = band[0].regions[Ns].Rmin;
    L2[0].regions[Nr].Rmax = band[0].regions[Ne - 1].Rmax;
    L2[0].regions[Nr].Dmin = zone[0].Dmin;
    L2[0].regions[Nr].Dmax = zone[0].Dmax;
    
    L2[0].regions[Nr].index    =  Nr;
    L2[0].regions[Nr].depth    =  2;
    L2[0].regions[Nr].table    =  -1;
    L2[0].regions[Nr].parent   =  parent;
    L2[0].regions[Nr].child    =  FALSE;
    L2[0].regions[Nr].childS   =  0;
    L2[0].regions[Nr].childE   =  0;
    L2[0].regions[Nr].hostFlags = 0;
    L2[0].regions[Nr].hostID    = 0;
    L2[0].regions[Nr].backupID  = 0;

    /* define names for L2 regions */
    p = strchr (band[0].regions[Ns].name, '/');
    if (p == NULL) {
      fprintf (stderr, "programming error in SkyTableL2fromZone\n");
      exit (2);
    }
    *p = 0;
    myAssert (snprintf (name, 80, "%s/z%03d", band[0].regions[Ns].name, Nr) < 80, "overflow");
    *p = '/';
    strcpy (L2[0].regions[Nr].name, name);
    if (DEBUG >= 2) SkyRegionPrint (&L2[0].regions[Nr]);

    /* childS and childE are set in SkyTableL3fromL2 */
    SkyTableL3fromL2 (&L2[0].regions[Nr], L3, L4, band, Ns, Ne);

    Nr++;
    CHECK_REALLOCATE (L2[0].regions, SkyRegion, L2[0].Nalloc, Nr, 100);
  }
  L2[0].Nregions = Nr;
}

// memory neutral
void SkyTableL3fromL2 (SkyRegion *L2, SkyTable *L3, SkyTable *L4, SkyTable *band, int Ns, int Ne) {

  int i, Nr;

  Nr = L3[0].Nregions;
  L3[0].Nregions += Ne - Ns;
  CHECK_REALLOCATE (L3[0].regions, SkyRegion, L3[0].Nalloc, L3[0].Nregions, 0.5*L3[0].Nalloc);

  L2[0].child  = TRUE;
  L2[0].childS = Nr;
  L2[0].childE = L3[0].Nregions;

  /* copy the band entries corresponding to this region in the L3 table */
  for (i = Ns; i < Ne; i++) {

    L3[0].regions[Nr] = band[0].regions[i];
    
    L3[0].regions[Nr].index    =  Nr;
    L3[0].regions[Nr].depth    =  3;
    L3[0].regions[Nr].table    =  -1;
    L3[0].regions[Nr].parent   =  L2[0].index;
    L3[0].regions[Nr].child    =  FALSE;
    L3[0].regions[Nr].childS   =  0;
    L3[0].regions[Nr].childE   =  0;
    L3[0].regions[Nr].hostFlags = 0;
    L3[0].regions[Nr].hostID    = 0;
    L3[0].regions[Nr].backupID  = 0;

    if (DEBUG >= 3) SkyRegionPrint (&L3[0].regions[Nr]);
    /* name is set for the band in SkyRegionForDecBand */

    /* childS and childE are set in SkyTableL3fromL3 */
    SkyTableL4fromL3 (&L3[0].regions[Nr], L4);
    Nr++;
  }

  return;
}

// memory neutral
void SkyTableL4fromL3 (SkyRegion *L3, SkyTable *L4) {

  int nx, ny, Nr, Nbox;
  double Rmin, Dmin, dR, dD;
  char name[80];

  Nr = L4[0].Nregions;
  L4[0].Nregions += NDIV*NDIV;
  CHECK_REALLOCATE (L4[0].regions, SkyRegion, L4[0].Nalloc, L4[0].Nregions, 0.5*L4[0].Nalloc);

  L3[0].child  = TRUE;
  L3[0].childS = Nr;
  L3[0].childE = L4[0].Nregions;

  // XXX handle the pole regions just a bit differently...

  /* subdivide L3 into NDIV boxes */
  Rmin = L3[0].Rmin;
  Dmin = L3[0].Dmin;
  dR = (L3[0].Rmax - L3[0].Rmin) / NDIV;
  dD = (L3[0].Dmax - L3[0].Dmin) / NDIV;

  Nbox = 0;
  for (ny = 0; ny < NDIV; ny ++) {
    for (nx = 0; nx < NDIV; nx ++) {
      L4[0].regions[Nr].Rmin     = Rmin  + (nx + 0)*dR;
      L4[0].regions[Nr].Rmax     = Rmin  + (nx + 1)*dR;
      L4[0].regions[Nr].Dmin     = Dmin + (ny + 0)*dD;
      L4[0].regions[Nr].Dmax     = Dmin + (ny + 1)*dD;

      L4[0].regions[Nr].index    =  Nr;
      L4[0].regions[Nr].depth    =  4;
      L4[0].regions[Nr].table    =  -1;
      L4[0].regions[Nr].parent   =  L3[0].index;
      L4[0].regions[Nr].child    =  FALSE;
      L4[0].regions[Nr].childS   =  0;
      L4[0].regions[Nr].childE   =  0;
      L4[0].regions[Nr].hostFlags = 0;
      L4[0].regions[Nr].hostID    = 0;
      L4[0].regions[Nr].backupID  = 0;

      myAssert (snprintf (name, 80, "%s.%02d", L3[0].name, Nbox) < 80, "overflow");
      strcpy (L4[0].regions[Nr].name, name);
      if (DEBUG >= 4) SkyRegionPrint (&L4[0].regions[Nr]);

      Nr ++;
      Nbox ++;
    }
  }
  return;
}

int NDIV_L5 = 5;

// append new regions on to the supplied L5 list (may be empty)
void SkyTableL5fromL4 (SkyRegion *L4, SkyTable *L5) {

  int nx, ny, Nr, Nbox;
  double Rmin, Dmin, dR, dD;
  char name[80];

  Nr = L5[0].Nregions;
  L5[0].Nregions += NDIV_L5*NDIV_L5;
  CHECK_REALLOCATE (L5[0].regions, SkyRegion, L5[0].Nalloc, L5[0].Nregions, 0.5*L5[0].Nalloc);

  L4[0].child  = TRUE;
  L4[0].childS = Nr;
  L4[0].childE = L5[0].Nregions;

  // XXX handle the pole regions just a bit differently...

  /* subdivide L4 into NDIV_L5 boxes */
  Rmin = L4[0].Rmin;
  Dmin = L4[0].Dmin;
  dR = (L4[0].Rmax - L4[0].Rmin) / NDIV_L5;
  dD = (L4[0].Dmax - L4[0].Dmin) / NDIV_L5;

  Nbox = 0;
  for (ny = 0; ny < NDIV_L5; ny ++) {
    for (nx = 0; nx < NDIV_L5; nx ++) {
      L5[0].regions[Nr].Rmin     = Rmin  + (nx + 0)*dR;
      L5[0].regions[Nr].Rmax     = Rmin  + (nx + 1)*dR;
      L5[0].regions[Nr].Dmin     = Dmin + (ny + 0)*dD;
      L5[0].regions[Nr].Dmax     = Dmin + (ny + 1)*dD;

      L5[0].regions[Nr].index    =  Nr;
      L5[0].regions[Nr].depth    =  5;
      L5[0].regions[Nr].table    =  -1;
      L5[0].regions[Nr].parent   =  L4[0].index;
      L5[0].regions[Nr].child    =  FALSE;
      L5[0].regions[Nr].childS   =  0;
      L5[0].regions[Nr].childE   =  0;
      L5[0].regions[Nr].hostFlags = 0;
      L5[0].regions[Nr].hostID    = 0;
      L5[0].regions[Nr].backupID  = 0;

      myAssert (snprintf (name, 80, "%s.%02d", L4[0].name, Nbox) < 80, "overflow");
      strcpy (L5[0].regions[Nr].name, name);
      if (DEBUG >= 4) SkyRegionPrint (&L5[0].regions[Nr]);

      Nr ++;
      Nbox ++;
    }
  }
  return;
}

// append new regions on to the supplied L5 list (may be empty)
void SkyTableL5fromL4_List (SkyRegion *L4, SkyTable *L5, int Nfirst) {

  int nx, ny, Nr, Nbox;
  double Rmin, Dmin, dR, dD;
  char name[80];

  Nr = L5[0].Nregions;
  L5[0].Nregions += NDIV_L5*NDIV_L5;
  CHECK_REALLOCATE (L5[0].regions, SkyRegion, L5[0].Nalloc, L5[0].Nregions, 0.5*L5[0].Nalloc);

  // Nfirst is the index value which the first entry in the new table of regions would get.

  // These child ranges are relative to the index of the first entry in this L5 region.
  // This is the value of Nregions of the full table.

  L4[0].child  = TRUE;
  L4[0].childS = Nfirst + Nr;
  L4[0].childE = Nfirst + L5[0].Nregions;

  // XXX handle the pole regions just a bit differently...

  /* subdivide L4 into NDIV_L5 boxes */
  Rmin = L4[0].Rmin;
  Dmin = L4[0].Dmin;
  dR = (L4[0].Rmax - L4[0].Rmin) / NDIV_L5;
  dD = (L4[0].Dmax - L4[0].Dmin) / NDIV_L5;

  Nbox = 0;
  for (ny = 0; ny < NDIV_L5; ny ++) {
    for (nx = 0; nx < NDIV_L5; nx ++) {
      L5[0].regions[Nr].Rmin     = Rmin  + (nx + 0)*dR;
      L5[0].regions[Nr].Rmax     = Rmin  + (nx + 1)*dR;
      L5[0].regions[Nr].Dmin     = Dmin + (ny + 0)*dD;
      L5[0].regions[Nr].Dmax     = Dmin + (ny + 1)*dD;

      L5[0].regions[Nr].index    =  Nfirst + Nr;
      L5[0].regions[Nr].depth    =  5;
      L5[0].regions[Nr].table    =  -1;
      L5[0].regions[Nr].parent   =  L4[0].index;
      L5[0].regions[Nr].child    =  FALSE;
      L5[0].regions[Nr].childS   =  0;
      L5[0].regions[Nr].childE   =  0;
      L5[0].regions[Nr].hostFlags = 0;
      L5[0].regions[Nr].hostID    = 0;
      L5[0].regions[Nr].backupID  = 0;

      myAssert (snprintf (name, 80, "%s.%02d", L4[0].name, Nbox) < 80, "overflow");
      strcpy (L5[0].regions[Nr].name, name);
      if (DEBUG >= 4) SkyRegionPrint (&L5[0].regions[Nr]);

      Nr ++;
      Nbox ++;
    }
  }
  return;
}

// add the supplied skytable regions to the end.
// the parent, child, and index values are already correct
void SkyTableExtend (SkyTable *old, SkyTable *new, int depth) {

  int i, Nold;

  Nold = old[0].Nregions;

  old[0].Nregions += new[0].Nregions;
  REALLOCATE (old[0].regions,  SkyRegion, old[0].Nregions);
  REALLOCATE (old[0].filename, char *,    old[0].Nregions);

  for (i = 0; i < new[0].Nregions; i++) {
    old[0].regions[i + Nold] = new[0].regions[i];
    old[0].regions[i + Nold].table = (old[0].regions[i + Nold].depth == depth);
    old[0].filename[i + Nold] = NULL;
  }
  return;
}

// memory neutral

/* We have a skytable with a certain number of entries.
   Add the new entries on to the end of this table
 */

void SkyTableAppend (SkyTable *old, SkyTable *new, int Nprev) {

  int i, Nold;

  Nold = old[0].Nregions;

  old[0].Nregions += new[0].Nregions;
  REALLOCATE (old[0].regions, SkyRegion, old[0].Nregions);

  // this code assumes that the parents of all supplied
  // new regions are the last block of regions
  for (i = Nprev; i < Nold; i++) {
    old[0].regions[i].childS += Nold;
    old[0].regions[i].childE += Nold;
  }

  for (i = 0; i < new[0].Nregions; i++) {
    old[0].regions[i + Nold] = new[0].regions[i];
    old[0].regions[i + Nold].parent += Nprev;
    old[0].regions[i + Nold].index += Nold;
  }
  return;
}

void SkyTableValidate (SkyTable *table) {

  for (int i = 0; i < table[0].Nregions; i++) {
    int idx = table[0].regions[i].index; // my index

    // index should match sequence
    myAssert (i == idx, "bad index");

    int parent = table[0].regions[i].parent;
    if (parent != -1) {
      // my index should be in the range of my parent's children
      myAssert (idx >= table[0].regions[parent].childS, "bad childS");
      myAssert (idx  < table[0].regions[parent].childE, "bad childE");
    }
    int Ns = table[0].regions[i].childS;
    int Ne = table[0].regions[i].childE;
    if (Ns && Ne) {
      for (int j = Ns; j < Ne; j++) {
	// my children should all have my index as the parent
	myAssert (table[0].regions[j].parent == idx, "bad parent");
      }
    }
  }

  fprintf (stderr, "validated skytable\n");
  return;
}

// memory neutral
void SkyRegionPrint (SkyRegion *region) {

  int i;

  fprintf (stderr, "L%d:", (int) region[0].depth);
  for (i = 0; i < region[0].depth; i++) {
    fprintf (stderr, " ");
  } 
  fprintf (stderr, "%s : %f - %f : %f - %f\n", region[0].name, region[0].Rmin, region[0].Rmax, region[0].Dmin, region[0].Dmax);
  return;
}

/***
    notes and questions:
    1) is the regions.index value used?
***/
