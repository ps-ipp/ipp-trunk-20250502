# include "delstar.h"

// this function identifies detections to be deleted as being duplicates based on imageID
// + detID *** at the edges of catalogs ***

// unlike many other versions of dvo operations, this function needs to gather detections
// back to the master program and then do the analysis (more like relphot).  the first
// block of the code slurps the data from the catalogs, the second block looks for duplicates

MeasureEdge *delete_fix_LAP_edges (off_t *nmeasure_edge) {

  int i;
  Catalog catalog;

  // load the current sky table (layout of all SkyRegions) 
  SkyTable *sky = SkyTableLoadOptimal (CATDIR, NULL, NULL, TRUE, -1, VERBOSE);
  if (!sky) {
    fprintf (stderr, "ERROR running loading sky table from %s\n", CATDIR);
    exit (2);
  }
  SkyTableSetFilenames (sky, CATDIR, "cpt");

  // determine the populated SkyRegions overlapping the requested area (default depth)
  SkyList *skylist = SINGLE_CPT ? SkyRegionByCPT (sky, SINGLE_CPT) : SkyListByPatch (sky, -1, &UserPatch);
  if (!skylist) {
    fprintf (stderr, "ERROR setting up skylist for %s\n", CATDIR);
    exit (2);
  }

  // launch the remote jobs
  if (PARALLEL && !HOST_ID) {
    MeasureEdge *measure_edge = delete_fix_LAP_edges_parallel (skylist, nmeasure_edge);
    return measure_edge;
  }

  // XXX accumulate detections in a local structure:
  off_t Nmeasure_edge = 0;
  off_t NMEASURE_EDGE = 1000;
  MeasureEdge *measure_edge = NULL;
  ALLOCATE (measure_edge, MeasureEdge, NMEASURE_EDGE);

  // delete detections from region
  for (i = 0; i < skylist[0].Nregions; i++) {

    // does this host ID match the desired location for the table?
    if (!HostTableTestHost(skylist[0].regions[i], HOST_ID)) continue;

    // set up the basic catalog info
    char hostfile[1024];
    snprintf_nowarn (hostfile, 1024, "%s/%s.cpt", HOSTDIR, skylist[0].regions[i]->name);

    dvo_catalog_init (&catalog, TRUE);
    catalog.filename  = HOST_ID ? hostfile : skylist[0].filename[i];
    catalog.Nsecfilt  = GetPhotcodeNsecfilt ();
    catalog.catflags = DVO_LOAD_AVERAGE | DVO_LOAD_MEASURE | DVO_LOAD_MISSING | DVO_LOAD_SECFILT;

    if (VERBOSE) fprintf (stderr, "deleting from %s\n", catalog.filename);

    // an error exit status here is a significant error
    if (!dvo_catalog_open (&catalog, skylist[0].regions[i], VERBOSE2, "a")) {
      fprintf (stderr, "ERROR: failure to open catalog file %s\n", catalog.filename);
      exit (2);
    }
    if (!catalog.Naverage_disk) {
      if (VERBOSE2) fprintf (stderr, "no data in %s, skipping\n", catalog.filename);
      dvo_catalog_unlock (&catalog);
      dvo_catalog_free (&catalog);
      continue;
    }

    // off_t NaverageStart = catalog.Naverage;
    // off_t NmeasureStart = catalog.Nmeasure;
    measure_edge = delete_fix_LAP_edges_get_measures (&catalog, measure_edge, &Nmeasure_edge, &NMEASURE_EDGE);

    dvo_catalog_unlock (&catalog);
    dvo_catalog_free (&catalog);
  }

  *nmeasure_edge = Nmeasure_edge;
  return measure_edge;
}

// CATDIR is supplied globally
MeasureEdge *delete_fix_LAP_edges_parallel (SkyList *sky, off_t *nmeasure_edge) {

  char uniquer[12];
  int TIME = time(NULL);
  int PID = getpid();
  snprintf_nowarn (uniquer, 12, "%05d.%05d", PID, TIME % 100000);

  // launch the delstar_client jobs to the parallel hosts

  // load the list of hosts
  HostTable *table = HostTableLoad (CATDIR, sky->hosts);
  if (!table) {
    fprintf (stderr, "ERROR: failure reading Host Table %s for database %s\n", sky->hosts, CATDIR);
    exit (1);
  }    

  int i, j;
  for (i = 0; i < table->Nhosts; i++) {

    if (sky->Nregions < table->Nhosts) {
      // do any of the regions want this host?
      int wantThisHost = FALSE;
      for (j = 0; j < sky->Nregions; j++) {
	if (HostTableTestHost (sky->regions[j], table->hosts[i].hostID)) {
	  wantThisHost = TRUE;
	  break;
	}
      }
      if (!wantThisHost) {
	// fprintf (stderr, "skip host %s\n", table->hosts[i].hostname);
	continue;
      }
    }

    // ensure that the paths are absolute path names
    char *tmppath = abspath (table->hosts[i].pathname, DVO_MAX_PATH);
    free (table->hosts[i].pathname);
    table->hosts[i].pathname = tmppath;

    ALLOCATE (table->hosts[i].results, char, 1024);
    snprintf_nowarn (table->hosts[i].results, 1024, "%s/delstar.fixLAPedges.measures.%s.dat", table->hosts[i].pathname, uniquer);

    char command[1024];
    snprintf_nowarn (command, 1024, "delstar_client -hostID %d -D CATDIR %s -hostdir %s -measures %s -region %f %f %f %f -fix-LAP-edges", 
	      table->hosts[i].hostID, CATDIR, table->hosts[i].pathname, table->hosts[i].results,
	      UserPatch.Rmin, UserPatch.Rmax, UserPatch.Dmin, UserPatch.Dmax);

    char tmpline[1024];
    if (VERBOSE)    { snprintf_nowarn (tmpline, 1024, "%s -v",      command);             strcpy (command, tmpline); }
    if (VERBOSE2)   { snprintf_nowarn (tmpline, 1024, "%s -vv",     command);             strcpy (command, tmpline); }
    if (SINGLE_CPT) { snprintf_nowarn (tmpline, 1024, "%s -cpt %s", command, SINGLE_CPT); strcpy (command, tmpline); }
    // if (UPDATE)     { snprintf_nowarn (tmpline, 1024, "%s -update", command);             strcpy (command, tmpline); }

    fprintf (stderr, "command: %s\n", command);

    if (PARALLEL_MANUAL) continue;

    if (PARALLEL_SERIAL) {
      int status = system (command);
      if (status) {
	fprintf (stderr, "ERROR running delstar_client\n");
	exit (2);
      }
    } else {
      // launch the job on the remote machine (no handshake)
      int errorInfo = 0;
      int pid = rconnect ("ssh", table->hosts[i].hostname, command, table->hosts[i].stdio, &errorInfo, FALSE);
      if (!pid) {
	fprintf (stderr, "failure to start %s (error %d)\n", table->hosts[i].hostname, errorInfo);
	exit (1);
      }
      table->hosts[i].pid = pid; // save for future reference
    }
  }

  if (PARALLEL_MANUAL) {
    fprintf (stderr, "run the client commands above.  when these are done, hit return\n");
    getchar();
  }
  if (!PARALLEL_MANUAL && !PARALLEL_SERIAL) {
    HostTableWaitJobsGetIO (table, __FILE__, __LINE__, VERBOSE);
  }

  off_t Nmeasure_edge_all = 0;
  off_t NMEASURE_EDGE_ALL = 1000;
  MeasureEdge *measure_edge_all = NULL;
  ALLOCATE (measure_edge_all, MeasureEdge, NMEASURE_EDGE_ALL);
  
  for (i = 0; i < table->Nhosts; i++) {
    if (!table->hosts[i].results) continue;
    off_t Nmeasure_edge = 0;
    MeasureEdge *measure_edge = MeasureEdgeLoad (table->hosts[i].results, &Nmeasure_edge);
    if (!measure_edge) {
      fprintf (stderr, "failed to read data from %s\n", table->hosts[i].hostname);
      continue;
    }
    // merge new measures into a single array
    measure_edge_all = MeasureEdgeMerge (measure_edge_all, &Nmeasure_edge_all, &NMEASURE_EDGE_ALL, measure_edge, Nmeasure_edge);
  }

  *nmeasure_edge = Nmeasure_edge_all;
  return measure_edge_all;
}

MeasureEdge *delete_fix_LAP_edges_get_measures (Catalog *catalog, MeasureEdge *measure_edge, off_t *Nmeasure_edge_in, off_t *NMEASURE_EDGE_IN) {

  off_t i, j, m;

  off_t Nmeasure_edge = *Nmeasure_edge_in;
  off_t NMEASURE_EDGE = *NMEASURE_EDGE_IN;

  // boundaries of the catalog file
  double Rmin, Rmax, Dmin, Dmax;
  gfits_scan (&catalog[0].header, "RA0",  "%lf", 1, &Rmin);
  gfits_scan (&catalog[0].header, "DEC0", "%lf", 1, &Dmin);
  gfits_scan (&catalog[0].header, "RA1",  "%lf", 1, &Rmax);
  gfits_scan (&catalog[0].header, "DEC1", "%lf", 1, &Dmax);

# define GAP 2.0
  double dD = GAP / 3600.0;
  double dR = dD / cos(RAD_DEG * 0.5 * (Dmin + Dmax));
  
  double Rgapmin = Rmin + dR;
  double Rgapmax = Rmax - dR;
  double Dgapmin = Dmin + dD;
  double Dgapmax = Dmax - dD;
  
  float cosDec = cos(RAD_DEG * 0.5 * (Dmin + Dmax));

  // XXX should deal with pole, but not yet...
  for (i = 0; i < catalog[0].Naverage; i++) {
    // keep all objects which are near the boundary
    if (catalog[0].average[i].R < Rgapmin) goto save_measures;
    if (catalog[0].average[i].R > Rgapmax) goto save_measures;
    if (catalog[0].average[i].D < Dgapmin) goto save_measures;
    if (catalog[0].average[i].D > Dgapmax) goto save_measures;

    // keep all objects for which the ra or dec range is too large (> 2 arcsec)
    float maxOff = 0.0;
    m = catalog[0].average[i].measureOffset;

    for (j = 0; j < catalog[0].average[i].Nmeasure; j++) {

      float dRoff = dvoOffsetR(&catalog[0].measure[m+j], &catalog[0].average[i])*cosDec;
      float dDoff = dvoOffsetD(&catalog[0].measure[m+j], &catalog[0].average[i]);
      float dOff = hypot (dRoff, dDoff);
      maxOff = MAX (maxOff, dOff);
    }
    if (maxOff > 3.0) goto save_measures;

    continue;
    
  save_measures:
    m = catalog[0].average[i].measureOffset;
    for (j = 0; j < catalog[0].average[i].Nmeasure; j++) {
      measure_edge[Nmeasure_edge].R       = catalog[0].average[i].R;
      measure_edge[Nmeasure_edge].D       = catalog[0].average[i].D;
      measure_edge[Nmeasure_edge].objID   = catalog[0].measure[m+j].objID;
      measure_edge[Nmeasure_edge].catID   = catalog[0].measure[m+j].catID;
      measure_edge[Nmeasure_edge].detID   = catalog[0].measure[m+j].detID;
      measure_edge[Nmeasure_edge].imageID = catalog[0].measure[m+j].imageID;
      Nmeasure_edge ++;
      CHECK_REALLOCATE (measure_edge, MeasureEdge, NMEASURE_EDGE, Nmeasure_edge, 1000);
    }
  }

  *Nmeasure_edge_in = Nmeasure_edge;
  *NMEASURE_EDGE_IN = NMEASURE_EDGE;
  
  return measure_edge;
}

// this function makes the needed modifications to the measure table (to imageIDs) and updates
// the array measureDrop to mark the entries we want to remove

// needed modifications:
// * delete all photcodes corresponding to 2mass, synth, or wise (not gpc1 or stack)
// * delete all duplicates
// * check the imageID 
// * re-assign the imageID if needed

int delete_fix_LAP_edges_find_dups (MeasureEdge *measure_edge, off_t Nmeasure_edge) {

  off_t i;

  // XXX TEST
# if (0)
  FILE *f = fopen ("delstar.fixLAPedge.dat", "w");
  for (i = 0; i < Nmeasure_edge; i++) {
    fprintf (f, "0x%08x 0x%08x | 0x%08x 0x%08x | %8.4f %8.4f\n", measure_edge[i].imageID, measure_edge[i].detID, measure_edge[i].objID, measure_edge[i].catID, measure_edge[i].R, measure_edge[i].D);
  }
  fclose (f);
# endif

  uint64_t *fullID;
  int *seq, *duplicates;

  ALLOCATE (fullID,     uint64_t, Nmeasure_edge);
  ALLOCATE (seq,             int, Nmeasure_edge);
  ALLOCATE (duplicates,      int, Nmeasure_edge);

  // memset (measureDrop, 0, sizeof(off_t) * Nmeasure_edge);
  memset (duplicates,  0, sizeof(int) * Nmeasure_edge);

  // make an array of the full IDs
  for (i = 0; i < Nmeasure_edge; i++) {
    fullID[i] = ((uint64_t) (measure_edge[i].imageID) << 32) | (measure_edge[i].detID);
    seq[i] = i;
  }

  // sort the array and sequence number
  sort_fullIDs (fullID, seq, Nmeasure_edge);

  uint64_t current = fullID[0];	      // current unique value
  int count = 0;		      // number of duplicates for the current unique value
  duplicates[0] = count;	      // duplicate sequence

  for (i = 1; i < Nmeasure_edge; i++) {
    off_t j = seq[i];
    int newID = (measure_edge[j].imageID == 0) || (fullID[i] != current);
    if (newID) {
      count = 0;
      current = fullID[i];
    } else {
      count ++;
    }
    duplicates[i] = count;
  }
    
  FILE *f = fopen ("delstar.fixLAPedge.dat", "w");

  // mark the measures to be dropped
  for (i = 0; i < Nmeasure_edge; i++) {
    off_t j = seq[i];

    int keep = TRUE;
    if (duplicates[i]) keep = FALSE;

    if (!keep) {
      // measureDrop[j] = TRUE;
      fprintf (f, "0x%08x 0x%08x | 0x%08x 0x%08x | %8.4f %8.4f\n", measure_edge[j].imageID, measure_edge[j].detID, measure_edge[j].objID, measure_edge[j].catID, measure_edge[j].R, measure_edge[j].D);
    }
  }
  fclose (f);

  FREE (fullID);
  FREE (seq);
  FREE (duplicates);

  return TRUE;
}
