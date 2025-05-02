# include "relastro.h"

// we are sharing mean and specific positions for all known ICRF objects which I own
// this function occurs on the remote hosts in parallel-region processing
int share_icrf_obj (Catalog *catalog, int Ncatalog, RegionHostTable *regionHosts, int nloop) {

  if (!USE_ICRF_CORRECT) return TRUE;

  ICRFobj *icrfobj = get_ICRF_data (catalog, Ncatalog);

  // write out the meanmag fits table AND write state in some file
  int myHost = regionHosts->index[REGION_HOST_ID];
  char *hostname = regionHosts->hosts[myHost].hostname;

  char *filename = make_filename (CATDIR, hostname, REGION_HOST_ID, "icrfobj.fits");
  ICRFobjSave (filename, icrfobj);
  free (filename);

  free (icrfobj);

  char *syncfile = make_filename (CATDIR, hostname, REGION_HOST_ID, "icrfobj.sync");
  update_sync_file (syncfile, nloop);
  free (syncfile);

  return TRUE;
}

// load mean and specific positions for all known ICRF objects from remote hosts.
// this function occurs on the master host in parallel-region processing
ICRFobj *slurp_icrf_obj (RegionHostTable *regionHosts, int nloop) {

  off_t i;

  ICRFobj *icrfobj = NULL;
  ALLOCATE (icrfobj, ICRFobj, 1);
  ALLOCATE (icrfobj->Rave,  double, 1);
  ALLOCATE (icrfobj->Dave,  double, 1);
  ALLOCATE (icrfobj->dRoff, double, 1);
  ALLOCATE (icrfobj->dDoff, double, 1);
  icrfobj->Nicrfobj = 0;

  fprintf (stderr, "grabbing icrf objects from remote hosts...\n");

  for (i = 0; i < regionHosts->Nhosts; i++) {
    char *syncfile = make_filename (CATDIR, regionHosts->hosts[i].hostname, regionHosts->hosts[i].hostID, "icrfobj.sync");
    check_sync_file (syncfile, nloop);
    free (syncfile);
    
    char *filename = make_filename (CATDIR, regionHosts->hosts[i].hostname, regionHosts->hosts[i].hostID, "icrfobj.fits");
    ICRFobj *icrfobjSubset = ICRFobjLoad (filename);
    free (filename);

    // merge_mean_pos reallocs icrfobj and frees the input icrfobjSubset
    icrfobj = merge_icrf_obj (icrfobj, icrfobjSubset);
  }
  return icrfobj;
}

ICRFobj *merge_icrf_obj (ICRFobj *target, ICRFobj *source) {

  off_t i;

  REALLOCATE (target->Rave,  double, target->Nicrfobj + source->Nicrfobj);
  REALLOCATE (target->Dave,  double, target->Nicrfobj + source->Nicrfobj);
  REALLOCATE (target->dRoff, double, target->Nicrfobj + source->Nicrfobj);
  REALLOCATE (target->dDoff, double, target->Nicrfobj + source->Nicrfobj);

  for (i = 0; i < source->Nicrfobj; i++) {
    off_t n = i + target->Nicrfobj;
    target->Rave[n]  = source->Rave[i];
    target->Dave[n]  = source->Dave[i];
    target->dRoff[n] = source->dRoff[i];
    target->dDoff[n] = source->dDoff[i];
  }
  target->Nicrfobj += source->Nicrfobj;

  ICRFobjFree (source);
  return (target);
}

ICRFobj *get_ICRF_data (Catalog *catalog, int Ncatalog) {

  int i;
  int Nicrf = ICRFmax();

  // we need to write an empty table none are available
  ICRFobj *icrfobj = NULL;
  ALLOCATE (icrfobj, ICRFobj, 1);

  ALLOCATE (icrfobj->Rave,  double, Nicrf);
  ALLOCATE (icrfobj->Dave,  double, Nicrf);
  ALLOCATE (icrfobj->dRoff, double, Nicrf);
  ALLOCATE (icrfobj->dDoff, double, Nicrf);

  // select the ICRF QSOS and save the necessary data
  int Npts = 0;
  for (i = 0; i < Nicrf; i++) {

    int cat, ave, meas;
    ICRFdata (i, &cat, &ave, &meas);
    myAssert (cat < Ncatalog, "oops");

    Average *average = &catalog[cat].average[ave];
    MeasureTiny *measure = &catalog[cat].measureT[meas]; // MeasureTiny?

    // record these in arcsec or degree?
    // correct for cos(D) or not?
    double dR = 3600.0*(average->R - measure->R)*cos(average->R*RAD_DEG);
    double dD = 3600.0*(average->D - measure->D);

    // dR = 3600*(Rave - Rmeas) -> Rmeas = Rave - dR / 36000

    if (isnan(dR)) continue;
    if (isnan(dD)) continue;

    icrfobj->Rave[Npts]  = average->R;
    icrfobj->Dave[Npts]  = average->D;
    icrfobj->dRoff[Npts] = dR;
    icrfobj->dDoff[Npts] = dD;
    Npts ++;
  }
  icrfobj->Nicrfobj = Npts;

  return icrfobj;
}

void ICRFobjFree (ICRFobj *icrfobj) {

  if (!icrfobj) return;

  FREE (icrfobj->Rave);
  FREE (icrfobj->Dave);
  FREE (icrfobj->dRoff);
  FREE (icrfobj->dDoff);
  return;
}

