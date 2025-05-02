# include "relastro.h"
// we are sharing mean positions for all objects which (a) I own and (b) which have unowned detections

# define D_NMEANPOS 10000
int share_mean_pos (Catalog *catalog, int Ncatalog, RegionHostTable *regionHosts, int nloop) {

  int i;
  off_t j;

  off_t Nmeanpos = 0;
  off_t NMEANPOS = D_NMEANPOS;

  MeanPos *meanpos = NULL;
  ALLOCATE (meanpos, MeanPos, NMEANPOS);

  int myHost = regionHosts->index[REGION_HOST_ID];
  char *myHostName = regionHosts->hosts[myHost].hostname;

  double Rmin = regionHosts->hosts[myHost].Rmin;
  double Rmax = regionHosts->hosts[myHost].Rmax;
  double Dmin = regionHosts->hosts[myHost].Dmin;
  double Dmax = regionHosts->hosts[myHost].Dmax;

  INITTIME;

  // XXX skip some catalogs based on UserPatch?
  for (i = 0; i < Ncatalog; i++) {
    for (j = 0; j < catalog[i].Naverage; j++) {

      // do I own this object? (in region range?) --- CAREFUL HERE!!
      if (catalog[i].average[j].R <  Rmin) continue;
      if (catalog[i].average[j].R >= Rmax) continue;
      if (catalog[i].average[j].D <  Dmin) continue;
      if (catalog[i].average[j].D >= Dmax) continue;

      // does this object have missing detections (does someone else need it?)
      // XXX : sky objects without missing detections
      // XXX watch out for detections which are not associated with an image (REF)
      if (catalog[i].nOwn_t[j] == catalog[i].average[j].Nmeasure) continue;

      set_mean_pos (&meanpos[Nmeanpos], &catalog[i].average[j]);
      Nmeanpos ++;
      CHECK_REALLOCATE (meanpos, MeanPos, NMEANPOS, Nmeanpos, D_NMEANPOS);
    }
  }
  LOGRTIME("set_mean_pos loop %d on %s, host %d: %f sec\n", nloop, myHostName, REGION_HOST_ID, dtime);

  // write out the meanmag fits table AND write state in some file
  char *hostname = regionHosts->hosts[myHost].hostname;

  char *posfile = make_filename (CATDIR, hostname, REGION_HOST_ID, "meanpos.fits");
  MeanPosSave (posfile, meanpos, Nmeanpos);
  free (meanpos);
  free (posfile);
  LOGRTIME("MeanPosSave loop %d on %s, host %d: %f sec\n", nloop, myHostName, REGION_HOST_ID, dtime);

  char *syncfile = make_filename (CATDIR, hostname, REGION_HOST_ID, "meanpos.sync");
  update_sync_file (syncfile, nloop);
  free (syncfile);
  LOGRTIME("update_sync_file loop %d on %s, host %d: %f sec\n", nloop, myHostName, REGION_HOST_ID, dtime);

  return TRUE;
}

int slurp_mean_pos (Catalog *catalog, int Ncatalog, RegionHostTable *regionHosts, int nloop) {

  off_t i;

  int Nmeanpos = 0;
  MeanPos *meanpos = NULL;
  ALLOCATE (meanpos, MeanPos, 1);

  int myHost = regionHosts->index[REGION_HOST_ID];
  char *myHostName = regionHosts->hosts[myHost].hostname;

  fprintf (stderr, "grabbing mean object pos from other hosts...\n");

  INITTIME;

  for (i = 0; i < regionHosts->Nhosts; i++) {
    if (regionHosts->hosts[i].hostID == REGION_HOST_ID) continue;
    if (REGION_HOST_ID && !regionHosts->hosts[i].isNeighbor) continue;

    char *syncfile = make_filename (CATDIR, regionHosts->hosts[i].hostname, regionHosts->hosts[i].hostID, "meanpos.sync");
    check_sync_file (syncfile, nloop);
    free (syncfile);
    
    off_t Nsubset = 0;
    char *posfile = make_filename (CATDIR, regionHosts->hosts[i].hostname, regionHosts->hosts[i].hostID, "meanpos.fits");
    MeanPos *meanposSubset = MeanPosLoad (posfile, &Nsubset);
    free (posfile);

    // merge_mean_pos reallocs meanpos and frees the input meanposSubset
    meanpos = merge_mean_pos (meanpos, &Nmeanpos, meanposSubset, Nsubset);
  }
  LOGRTIME("MeanPosLoad/merge_mean_pos loop %d on %s, host %d: %f sec\n", nloop, myHostName, REGION_HOST_ID, dtime);

  for (i = 0; i < Nmeanpos; i++) {
    int objID = meanpos[i].objID;
    int catID = meanpos[i].catID;

    // set the mean mag
    int catSeq;
    off_t objSeq;
    if (!catID_and_objID_to_seq (catID, objID, &catSeq, &objSeq)) {
	// XXX what should I do if this does not match?
	continue;
    }
    myAssert (catSeq < Ncatalog, "oops");
    
    catalog[catSeq].average[objSeq].R = meanpos[i].R;
    catalog[catSeq].average[objSeq].D = meanpos[i].D;
  }
  free (meanpos);
  LOGRTIME("match_catID_and_objID loop %d on %s, host %d: %f sec\n", nloop, myHostName, REGION_HOST_ID, dtime);

  fprintf (stderr, "DONE grabbing mean object pos from other hosts...\n");

  return TRUE;
}

int set_mean_pos (MeanPos *meanpos, Average *average) {

  meanpos->R     = average->R;
  meanpos->D     = average->D;
  meanpos->objID = average->objID;
  meanpos->catID = average->catID;

  return TRUE;
}

MeanPos *merge_mean_pos (MeanPos *target, int *ntarget, MeanPos *source, int Nsource) {

  off_t i;

  REALLOCATE (target, MeanPos, *ntarget + Nsource);
  for (i = 0; i < Nsource; i++) {
    off_t n = i + *ntarget;
    target[n] = source[i];
  }
  
  free (source);

  *ntarget += Nsource;
  return (target);
}

