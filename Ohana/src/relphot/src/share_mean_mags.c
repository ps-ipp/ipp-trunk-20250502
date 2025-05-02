# include "relphot.h"
// we are sharing mean mags for all objects which (a) I own and (b) which have unowned detections

# define D_NMEANMAGS 10000
int share_mean_mags (Catalog *catalog, int Ncatalog, RegionHostTable *regionHosts, int nloop) {

  int i;
  off_t j;

  off_t Nmeanmags = 0;
  off_t NMEANMAGS = D_NMEANMAGS;

  MeanMag *meanmags = NULL;
  ALLOCATE (meanmags, MeanMag, NMEANMAGS);

  int Ns;
  int Nsecfilt = GetPhotcodeNsecfilt();

  int myHost = regionHosts->index[REGION_HOST_ID];
  double Rmin = regionHosts->hosts[myHost].Rmin;
  double Rmax = regionHosts->hosts[myHost].Rmax;
  double Dmin = regionHosts->hosts[myHost].Dmin;
  double Dmax = regionHosts->hosts[myHost].Dmax;

  // XXX skip some catalogs based on UserPatch?
  for (i = 0; i < Ncatalog; i++) {
    for (j = 0; j < catalog[i].Naverage; j++) {

      // do I own this object? (in region range?)
      if (catalog[i].averageT[j].R <  Rmin) continue;
      if (catalog[i].averageT[j].R >= Rmax) continue;
      if (catalog[i].averageT[j].D <  Dmin) continue;
      if (catalog[i].averageT[j].D >= Dmax) continue;

      // does this object have missing detections (does someone else need it?)
      // XXX : sky objects without missing detections
      // XXX watch out for detections which are not associated with an image (REF)
      if (catalog[i].averageT[j].nOwn == catalog[i].averageT[j].Nmeasure) continue;

      for (Ns = 0; Ns < Nphotcodes; Ns++) {
	int thisCode = photcodes[Ns][0].code;
	int Nsec = GetPhotcodeNsec(thisCode);
	set_mean_mags (&meanmags[Nmeanmags], &catalog[i].averageT[j], &catalog[i].secfilt[Nsecfilt*j + Nsec], Nsec);
	Nmeanmags ++;
	CHECK_REALLOCATE (meanmags, MeanMag, NMEANMAGS, Nmeanmags, D_NMEANMAGS);
      }
    }
  }

  // write out the meanmag fits table AND write state in some file
  char *hostname = regionHosts->hosts[myHost].hostname;

  char *magsfile = make_filename (CATDIR, hostname, REGION_HOST_ID, "meanmags.fits");
  MeanMagSave (magsfile, meanmags, Nmeanmags);
  free (meanmags);
  free (magsfile);

  char *syncfile = make_filename (CATDIR, hostname, REGION_HOST_ID, "meanmags.sync");
  update_sync_file (syncfile, nloop);
  free (syncfile);

  return TRUE;
}

int slurp_mean_mags (Catalog *catalog, int Ncatalog, RegionHostTable *regionHosts, int nloop) {
  OHANA_UNUSED_PARAM(Ncatalog);

  off_t i;

  int Nmeanmags = 0;
  MeanMag *meanmags = NULL;
  ALLOCATE (meanmags, MeanMag, 1);

  fprintf (stderr, "grabbing mean object mags from other hosts...\n");

  int Nsecfilt = GetPhotcodeNsecfilt();

  for (i = 0; i < regionHosts->Nhosts; i++) {
    // if (not_neighbor(host[i])) continue;
    if (regionHosts->hosts[i].hostID == REGION_HOST_ID) continue;
    if (REGION_HOST_ID && !regionHosts->hosts[i].isNeighbor) continue;

    char *syncfile = make_filename (CATDIR, regionHosts->hosts[i].hostname, regionHosts->hosts[i].hostID, "meanmags.sync");
    check_sync_file (syncfile, nloop);
    free (syncfile);
    
    off_t Nsubset = 0;
    char *magsfile = make_filename (CATDIR, regionHosts->hosts[i].hostname, regionHosts->hosts[i].hostID, "meanmags.fits");
    MeanMag *meanmagsSubset = MeanMagLoad (magsfile, &Nsubset);
    free (magsfile);

    // merge_mean_mags reallocs meanmags and frees the input meanmagsSubset
    meanmags = merge_mean_mags (meanmags, &Nmeanmags, meanmagsSubset, Nsubset);
  }

  for (i = 0; i < Nmeanmags; i++) {
    int objID = meanmags[i].objID;
    int catID = meanmags[i].catID;

    // set the mean mag
    int catSeq;
    off_t objSeq;
    if (!catID_and_objID_to_seq (catID, objID, &catSeq, &objSeq)) {
	// XXX what should I do if this does not match?
	continue;
    }

    int ecode = GetPhotcodeEquivCodebyCode (meanmags[i].photcode);
    if (ecode < 0) continue;
    int Nsec = GetPhotcodeNsec (ecode);
    if (Nsec < 0) continue;

    catalog[catSeq].secfilt[objSeq*Nsecfilt + Nsec].MpsfChp = meanmags[i].M;
  }
  free (meanmags);

  fprintf (stderr, "DONE grabbing mean object mags from other hosts...\n");

  return TRUE;
}

int set_mean_mags (MeanMag *meanmags, AverageTiny *average, SecFilt *secfilt, int Nsec) {

  meanmags->M  = secfilt->MpsfChp;
  meanmags->dM = secfilt->dMpsfChp;
  meanmags->Mchisq = secfilt->Mchisq;
  meanmags->Nsec = Nsec; // key to secfilt entry

  meanmags->objID = average->objID;
  meanmags->catID = average->catID;

  return TRUE;
}

MeanMag *merge_mean_mags (MeanMag *target, int *ntarget, MeanMag *source, int Nsource) {

  off_t i;

  REALLOCATE (target, MeanMag, *ntarget + Nsource);
  for (i = 0; i < Nsource; i++) {
    off_t n = i + *ntarget;
    target[n] = source[i];
  }
  
  free (source);

  *ntarget += Nsource;
  return (target);
}

