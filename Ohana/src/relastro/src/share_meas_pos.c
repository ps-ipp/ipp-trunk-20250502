# include "relastro.h"
// we are sharing meas positions for all objects which (a) I own and (b) which have unowned detections

# define D_NMEASPOS 10000
int share_meas_pos (Catalog *catalog, int Ncatalog, RegionHostTable *regionHosts, int nloop) {

  int i;
  off_t j, k;

  off_t Nmeaspos = 0;
  off_t NMEASPOS = D_NMEASPOS;

  MeasPos *measpos = NULL;
  ALLOCATE (measpos, MeasPos, NMEASPOS);

  INITTIME;
  int myHost = regionHosts->index[REGION_HOST_ID];
  char *myHostName = regionHosts->hosts[myHost].hostname;

  // XXX skip some catalogs based on UserPatch?
  for (i = 0; i < Ncatalog; i++) {
    for (j = 0; j < catalog[i].Naverage; j++) {

      // does this object have missing detections (does someone else need it?)
      if (catalog[i].nOwn_t[j] == catalog[i].average[j].Nmeasure) continue;

      off_t m = catalog[i].average[j].measureOffset;
      for (k = 0; k < catalog[i].average[j].Nmeasure; k++, m++) {
	if (!catalog[i].measureT[m].myDet) continue;

	set_meas_pos (&measpos[Nmeaspos], &catalog[i].average[j], &catalog[i].measureT[m]);
	Nmeaspos ++;
	CHECK_REALLOCATE (measpos, MeasPos, NMEASPOS, Nmeaspos, D_NMEASPOS);
      }
    }
  }
  LOGRTIME("set_meas_pos loop %d on %s, host %d: %f sec\n", nloop, myHostName, REGION_HOST_ID, dtime);

  // write out the measmag fits table AND write state in some file
  char *hostname = regionHosts->hosts[myHost].hostname;

  char *posfile = make_filename (CATDIR, hostname, REGION_HOST_ID, "measpos.fits");
  MeasPosSave (posfile, measpos, Nmeaspos);
  free (measpos);
  free (posfile);
  LOGRTIME("MeasPosSave loop %d on %s, host %d: %f sec\n", nloop, myHostName, REGION_HOST_ID, dtime);

  char *syncfile = make_filename (CATDIR, hostname, REGION_HOST_ID, "measpos.sync");
  update_sync_file (syncfile, nloop);
  free (syncfile);
  LOGRTIME("update_sync_file loop %d on %s, host %d: %f sec\n", nloop, myHostName, REGION_HOST_ID, dtime);

  return TRUE;
}

int slurp_meas_pos (Catalog *catalog, int Ncatalog, RegionHostTable *regionHosts, int nloop) {

  off_t i, k;

  fprintf (stderr, "grabbing meas object pos from other hosts...\n");

  INITTIME;
  int myHost = regionHosts->index[REGION_HOST_ID];
  char *myHostName = regionHosts->hosts[myHost].hostname;

  LOGRTIME("meas_load_start loop %d on %s, host %d: %f sec\n", nloop, myHostName, REGION_HOST_ID, dtime);
  for (i = 0; i < regionHosts->Nhosts; i++) {
    if (regionHosts->hosts[i].hostID == REGION_HOST_ID) continue;
    if (REGION_HOST_ID && !regionHosts->hosts[i].isNeighbor) continue;

    char *syncfile = make_filename (CATDIR, regionHosts->hosts[i].hostname, regionHosts->hosts[i].hostID, "measpos.sync");
    check_sync_file (syncfile, nloop);
    free (syncfile);
    LOGRTIME("meas_load_sync host %d loop %d on %s, host %d: %f sec\n", i, nloop, myHostName, REGION_HOST_ID, dtime);
    
    off_t Nmeaspos = 0;
    char *posfile = make_filename (CATDIR, regionHosts->hosts[i].hostname, regionHosts->hosts[i].hostID, "measpos.fits");
    MeasPos *measpos = MeasPosLoad (posfile, &Nmeaspos);
    free (posfile);
    LOGRTIME("MeasPosLoad host %d loop %d on %s, host %d: %f sec\n", i, nloop, myHostName, REGION_HOST_ID, dtime);

    // apply the loaded measurement positions:
    for (off_t j = 0; j < Nmeaspos; j++) {
      int objID = measpos[j].objID;
      int catID = measpos[j].catID;

      // set the meas mag
      int catSeq;
      off_t objSeq;
      if (!catID_and_objID_to_seq (catID, objID, &catSeq, &objSeq)) {
	// XXX what should I do if this does not match?
	continue;
      }
      myAssert (catSeq < Ncatalog, "oops");

      off_t m = catalog[catSeq].average[objSeq].measureOffset;
      for (k = 0; k < catalog[catSeq].average[objSeq].Nmeasure; k++, m++) {
	if (catalog[catSeq].measureT[m].imageID != measpos[j].imageID) continue;
	catalog[catSeq].measureT[m].R = measpos[j].R;
	catalog[catSeq].measureT[m].D = measpos[j].D;
	if (catalog[catSeq].measure) {
	  catalog[catSeq].measure[m].R = measpos[j].R;
	  catalog[catSeq].measure[m].D = measpos[j].D;
	}
      }
    }
    free (measpos);
    LOGRTIME("match_catID_and_objID/meas loop %d on %s, host %d: %f sec\n", nloop, myHostName, REGION_HOST_ID, dtime);
  }
  LOGRTIME("meas_load_done loop %d on %s, host %d: %f sec\n", nloop, myHostName, REGION_HOST_ID, dtime);

  fprintf (stderr, "DONE grabbing meas object pos from other hosts...\n");

  return TRUE;
}

int set_meas_pos (MeasPos *measpos, Average *average, MeasureTiny *measure) {

  measpos->R       = measure->R;
  measpos->D       = measure->D;
  measpos->objID   = average->objID;
  measpos->catID   = average->catID;
  measpos->imageID = measure->imageID;

  return TRUE;
}

MeasPos *merge_meas_pos (MeasPos *target, int *ntarget, MeasPos *source, int Nsource) {

  off_t i;

  REALLOCATE (target, MeasPos, *ntarget + Nsource);
  for (i = 0; i < Nsource; i++) {
    off_t n = i + *ntarget;
    target[n] = source[i];
  }
  
  free (source);

  *ntarget += Nsource;
  return (target);
}

