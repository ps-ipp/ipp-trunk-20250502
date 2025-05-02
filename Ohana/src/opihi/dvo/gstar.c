# include "dvoshell.h"

typedef enum {
  GSTAR_BY_SEQ,
  GSTAR_BY_TIME,
  GSTAR_BY_PHOTCODE,
  GSTAR_BY_IMAGE_ID,
  GSTAR_BY_DET_ID,
} GSTAR_SORT_CODES;

typedef enum {
  GSTAR_FILTER_NAMES,
  GSTAR_SECF_FLAGS,
  GSTAR_UCDIST,
  GSTAR_AVE_MAG,
  GSTAR_AVE_MAG_ERR,
  GSTAR_AVE_MAG_CHISQ,
  GSTAR_AVE_MAG_STDEV,
  GSTAR_AVE_AP_MAG,
  GSTAR_AVE_AP_MAG_ERR,
  GSTAR_AVE_AP_MAG_CHISQ,
  GSTAR_AVE_AP_MAG_STDEV,
  GSTAR_AVE_MAG_MIN,
  GSTAR_AVE_MAG_MAX,
  GSTAR_AVE_KRON_MAG,
  GSTAR_AVE_KRON_MAG_ERR,
  GSTAR_AVE_KRON_MAG_CHISQ,
  GSTAR_AVE_KRON_MAG_STDEV,
  GSTAR_AVE_NCODE,
  GSTAR_AVE_NUSED,
  GSTAR_AVE_AP_NUSED,
  GSTAR_AVE_KRON_NUSED,
  GSTAR_STACK_FLUX_PSF,
  GSTAR_STACK_FLUX_PSF_ERR,
  GSTAR_STACK_FLUX_KRON,
  GSTAR_STACK_FLUX_KRON_ERR,
  GSTAR_STACK_FLUX_APER,
  GSTAR_STACK_FLUX_APER_ERR,
  GSTAR_WARP_FLUX_PSF,
  GSTAR_WARP_FLUX_PSF_ERR,
  GSTAR_WARP_FLUX_PSF_CHISQ,
  GSTAR_WARP_FLUX_KRON,
  GSTAR_WARP_FLUX_KRON_ERR,
  GSTAR_WARP_FLUX_KRON_CHISQ,
  GSTAR_WARP_FLUX_APER,
  GSTAR_WARP_FLUX_APER_ERR,
  GSTAR_WARP_FLUX_APER_CHISQ,
  GSTAR_STACK_MAG_PSF,
  GSTAR_STACK_MAG_PSF_ERR,
  GSTAR_STACK_MAG_KRON,
  GSTAR_STACK_MAG_KRON_ERR,
  GSTAR_STACK_MAG_APER,
  GSTAR_STACK_MAG_APER_ERR,
  GSTAR_STACK_NUSED,
  GSTAR_WARP_MAG_PSF,
  GSTAR_WARP_MAG_PSF_ERR,
  GSTAR_WARP_MAG_PSF_CHISQ,
  GSTAR_WARP_MAG_KRON,
  GSTAR_WARP_MAG_KRON_ERR,
  GSTAR_WARP_MAG_KRON_CHISQ,
  GSTAR_WARP_MAG_APER,
  GSTAR_WARP_MAG_APER_ERR,
  GSTAR_WARP_MAG_APER_CHISQ,
  GSTAR_WARP_NWARP,
  GSTAR_WARP_NWARP_GOOD,
  GSTAR_WARP_NWARP_USED_PSF,
  GSTAR_WARP_NWARP_USED_KRON,
  GSTAR_WARP_NWARP_USED_AP,
} GSTAR_SECF_CODES;

void initPhotcodeSequence (int Nsecfilt);
void freePhotcodeSequence ();
void printPhotcodeSequence (Average *average, SecFilt *secfilt, int entry, int type);

int gstar (int argc, char **argv) {
  
  char *date;
  double Ra, Dec, Radius, Radius2, r, dec0, dec1;
  double Mcat, Mrel;
  double *RA, *DEC;
  off_t i, Nstars, *N1;
  off_t j, k, m, N, Nlo, Nhi;
  int Nsecfilt, NPTS, QUIET, INST, SHOW_MASKS;
  int found, GetMeasure, GetLensing;
  int SaveVectors;
  Vector *vec1, *vec2, *vec3, *vec4, *vec5, *vec6;
  SkyTable *sky;
  SkyList *skylist;
  Catalog catalog;
  int TimeFormat;
  time_t TimeReference;

  if (!InitPhotcodes ()) return (FALSE);
  Nsecfilt = GetPhotcodeNsecfilt ();

  GSTAR_SORT_CODES sortCode = GSTAR_BY_SEQ;
  if ((N = get_argument (argc, argv, "-sort"))) {
    remove_argument (N, &argc, argv);
    if (!strcasecmp (argv[N], "time")) {
      sortCode = GSTAR_BY_TIME;
    }
    if (!strcasecmp (argv[N], "photcode")) {
      sortCode = GSTAR_BY_PHOTCODE;
    }
    if (!strcasecmp (argv[N], "image")) {
      sortCode = GSTAR_BY_IMAGE_ID;
    }
    if (!strcasecmp (argv[N], "det")) {
      sortCode = GSTAR_BY_DET_ID;
    }
    if (!strcasecmp (argv[N], "imageID")) {
      sortCode = GSTAR_BY_IMAGE_ID;
    }
    if (!strcasecmp (argv[N], "detID")) {
      sortCode = GSTAR_BY_DET_ID;
    }
    remove_argument (N, &argc, argv);
  }

  QUIET = FALSE;
  if ((N = get_argument (argc, argv, "-q"))) {
    QUIET = TRUE;
    remove_argument (N, &argc, argv);
  }

  INST = FALSE;
  if ((N = get_argument (argc, argv, "-inst"))) {
    INST = TRUE;
    remove_argument (N, &argc, argv);
  }

  SHOW_MASKS = FALSE;
  if ((N = get_argument (argc, argv, "-masks"))) {
    SHOW_MASKS = TRUE;
    remove_argument (N, &argc, argv);
  }

  NPTS = 0;
  vec1 = vec2 = vec3 = vec4 = vec5 = vec6 = NULL;
  SaveVectors = FALSE;
  if ((N = get_argument (argc, argv, "-save"))) {
    remove_argument (N, &argc, argv);
    SaveVectors = TRUE;
    if ((vec1 = SelectVector ("gs:m", ANYVECTOR, TRUE)) == NULL) return (FALSE);
    if ((vec2 = SelectVector ("gs:t", ANYVECTOR, TRUE)) == NULL) return (FALSE);
    if ((vec3 = SelectVector ("gs:z", ANYVECTOR, TRUE)) == NULL) return (FALSE);
    if ((vec4 = SelectVector ("gs:f", ANYVECTOR, TRUE)) == NULL) return (FALSE);
    if ((vec5 = SelectVector ("gs:dr", ANYVECTOR, TRUE)) == NULL) return (FALSE);
    if ((vec6 = SelectVector ("gs:dd", ANYVECTOR, TRUE)) == NULL) return (FALSE);
  }

  int FULL_OUTPUT = FALSE;
  if ((N = get_argument (argc, argv, "-full"))) {
    FULL_OUTPUT = TRUE;
    remove_argument (N, &argc, argv);
  }

  int STACK_OUTPUT = FALSE;
  if ((N = get_argument (argc, argv, "+stack"))) {
    STACK_OUTPUT = TRUE;
    remove_argument (N, &argc, argv);
  }

  int EXTRA_OUTPUT = FALSE;
  if ((N = get_argument (argc, argv, "+extras"))) {
    EXTRA_OUTPUT = TRUE;
    remove_argument (N, &argc, argv);
  }

  int WARP_OUTPUT = FALSE;
  if ((N = get_argument (argc, argv, "+warp"))) {
    WARP_OUTPUT = TRUE;
    remove_argument (N, &argc, argv);
  }

  int FLUX_OUTPUT = FALSE;
  if ((N = get_argument (argc, argv, "+flux"))) {
    FLUX_OUTPUT = TRUE;
    remove_argument (N, &argc, argv);
  }

  int MAG_OUTPUT = TRUE;
  if ((N = get_argument (argc, argv, "-mag"))) {
    MAG_OUTPUT = FALSE;
    remove_argument (N, &argc, argv);
  }

  int KRON_OUTPUT = FALSE;
  if ((N = get_argument (argc, argv, "+kron"))) {
    KRON_OUTPUT = TRUE;
    remove_argument (N, &argc, argv);
  }

  int APER_OUTPUT = FALSE;
  if ((N = get_argument (argc, argv, "+aper"))) {
    APER_OUTPUT = TRUE;
    remove_argument (N, &argc, argv);
  }

  GetMeasure = FALSE;
  if ((N = get_argument (argc, argv, "-m"))) {
    GetMeasure = TRUE;
    remove_argument (N, &argc, argv);
  }
  if ((N = get_argument (argc, argv, "-meas"))) {
    GetMeasure = TRUE;
    remove_argument (N, &argc, argv);
  }
  if (SaveVectors) GetMeasure = TRUE;

  GetLensing = FALSE;
  if ((N = get_argument (argc, argv, "-lens"))) {
    GetLensing = TRUE;
    remove_argument (N, &argc, argv);
  }

  PhotCode *photcode = NULL;
  int photcodeEquiv = FALSE;
  if ((N = get_argument (argc, argv, "-p"))) {
    remove_argument (N, &argc, argv);
    photcode = GetPhotcodebyName (argv[N]);
    if (photcode == NULL) {
      gprint (GP_ERR, "photcode %s not found\n", argv[N]);
      return FALSE;
    }
    remove_argument (N, &argc, argv);
    int Nsec = GetPhotcodeNsec (photcode->code);
    if (Nsec != -1) photcodeEquiv = TRUE;
  }
  if ((N = get_argument (argc, argv, "-photcode"))) {
    remove_argument (N, &argc, argv);
    photcode = GetPhotcodebyName (argv[N]);
    if (photcode == NULL) {
      gprint (GP_ERR, "photcode %s not found\n", argv[N]);
      return FALSE;
    }
    remove_argument (N, &argc, argv);
    int Nsec = GetPhotcodeNsec (photcode->code);
    if (Nsec != -1) photcodeEquiv = TRUE;
  }

  if (argc != 4) {
    gprint (GP_ERR, "USAGE: gstar RA DEC Radius [options]\n");
    gprint (GP_ERR, "OPTIONS:\n");    
    gprint (GP_ERR, "-sort [photcode,time,image,det,imageID,detID]\n");    
    gprint (GP_ERR, "-q : quiet\n");    
    gprint (GP_ERR, "-inst : report instrumental mags\n");    
    gprint (GP_ERR, "-masks : show masks\n");    
    gprint (GP_ERR, "-save : save m, t, z, f, dr, dd in vectors\n");    
    gprint (GP_ERR, "-full : show more information\n");    
    gprint (GP_ERR, "+stack: report stack averages\n");    
    gprint (GP_ERR, "+warp : report warp averages\n");    
    gprint (GP_ERR, "+extras : extra output fields\n");    
    gprint (GP_ERR, "+flux : report average fluxes\n");    
    gprint (GP_ERR, "-mag : suppress magnitude fielss\n");    
    gprint (GP_ERR, "+kron : report kron mags\n");    
    gprint (GP_ERR, "+aper : report aper mags\n");    
    gprint (GP_ERR, "-m : report individual measurements\n");    
    gprint (GP_ERR, "-meas : report individual measurements\n");    
    gprint (GP_ERR, "-lens : report lensing data\n");    
    gprint (GP_ERR, "-p [photcode] : restrict to photcode\n");    
    gprint (GP_ERR, "-photcode [photcode] : restrict to photcode\n");    
    gprint (GP_ERR, "\n");    
    return (FALSE);
  }
  
  GetTimeFormat (&TimeReference, &TimeFormat);

  Ra = atof (argv[1]);
  Dec = atof (argv[2]);
  Radius = atof (argv[3]);

  Ra = ohana_normalize_angle (Ra);
  
  /* load sky from correct table */
  sky = GetSkyTable ();
  skylist = SkyListByRadius (sky, -1, Ra, Dec, Radius);

  if (skylist[0].Nregions > 1) {
    gprint (GP_ERR, "warning, radius overlaps region boundary, not yet implemented\n");
  }

  dvo_catalog_init (&catalog, TRUE);

  HostTable *table = NULL;  
  char *CATDIR = GetCATDIR();
  if (HostTableExists (CATDIR, sky->hosts)) {
    table = HostTableLoad (CATDIR, sky->hosts);
    if (!table) {
      gprint (GP_ERR, "ERROR: failure reading Host Table %s for database %s\n", sky->hosts, CATDIR);
      return FALSE;
    }    

    SkyRegion *region = skylist[0].regions[0];
    int hostID = (region->hostFlags & DATA_USE_BCK) ? region->backupID : region->hostID;
    int index = table->index[hostID];
    
    char hostfile[1024];
    snprintf (hostfile, 1024, "%s/%s.cpt", table->hosts[index].pathname, region->name);
    catalog.filename = hostfile;
  } else {
    catalog.filename = skylist[0].filename[0];
  }

  /* lock, load, unlock catalog */
  catalog.catflags = DVO_LOAD_AVERAGE | DVO_LOAD_SECFILT;
  catalog.catflags |= GetMeasure ? DVO_LOAD_MEASURE : DVO_SKIP_MEASURE;
  catalog.catflags |= GetLensing ? DVO_LOAD_LENSING : DVO_SKIP_LENSING;
  catalog.Nsecfilt = 0;

  // an error exit status here is a significant error
  if (!dvo_catalog_open (&catalog, NULL, FALSE, "r")) {
    fprintf (stderr, "ERROR: failure to open catalog file %s\n", catalog.filename);
    exit (2);
  }
  dvo_catalog_unlock (&catalog);

  Nstars = catalog.Naverage;
  ALLOCATE (RA, double, Nstars);
  ALLOCATE (DEC, double, Nstars);
  ALLOCATE (N1, off_t, Nstars);

  /* find star(s) in RA, DEC list -- use a dumb algorithm for now, improve later */
  /* stars are not guaranteed to be sorted in RA or in DEC, so first sort the list */
  for (i = 0; i < Nstars; i++) {
    RA[i] = catalog.average[i].R;
    DEC[i] = catalog.average[i].D;
    N1[i] = i;
  }
  /* sort list by DEC */
  if (Nstars > 1) sort_coords_index (DEC, RA, N1, Nstars);
  /* at this point, RA, DEC, and N1 are sorted by DEC.  
     catalog.average[N1[i]].R = RA[i] */

  /* bracket the RA range of interest */
  dec0 = Dec - Radius;
  dec1 = Dec + Radius;

  Nlo = 0; Nhi = catalog.Naverage;
  while (Nhi - Nlo > 10) {
    N = 0.5*(Nlo + Nhi);
    if (DEC[N] < dec0) {
      Nlo = N;
    } else {
      Nhi = N + 1;
    }
  }
  /* DEC[Nlo] is guaranteed to be just lower than dec0 */

  Radius2 = Radius*Radius;
  found = FALSE;

  /* data has been loaded, get ready to plot it */
  if (SaveVectors) {
    N = 0;
    NPTS = 1000;
    ResetVector (vec1, OPIHI_FLT, NPTS);
    ResetVector (vec2, OPIHI_FLT, NPTS);
    ResetVector (vec3, OPIHI_FLT, NPTS);
    ResetVector (vec4, OPIHI_FLT, NPTS);
    ResetVector (vec5, OPIHI_FLT, NPTS);
    ResetVector (vec6, OPIHI_FLT, NPTS);
  }

  initPhotcodeSequence (Nsecfilt);

  for (i = Nlo; (i < catalog.Naverage) && !found; i++) {

    if (dec0 > DEC[i]) continue;
    if (dec1 < DEC[i]) found = TRUE;
    
    r = SQ(Dec - DEC[i]) + SQ((Ra - RA[i])*cos(Dec*RAD_DEG));
    if (r < Radius2) {
      k = N1[i];
      if (!QUIET) {
	gprint (GP_LOG, "star: "OFF_T_FMT"\n",  k);
	gprint (GP_LOG, "%11.7f ", catalog.average[k].R);
	gprint (GP_LOG, "%11.7f ", catalog.average[k].D);
	gprint (GP_LOG, "%5.2f ",   3600.0*sqrt(r));
	gprint (GP_LOG, "%3d   ",  catalog.average[k].Nmeasure);
	gprint (GP_LOG, "%4.1f ",  catalog.average[k].ChiSqAve);
	gprint (GP_LOG, "0x%08x ", catalog.average[k].flags);
	gprint (GP_LOG, "0x%08x ", catalog.average[k].objID);
	gprint (GP_LOG, "0x%08x ", catalog.average[k].catID);
	
	if (FULL_OUTPUT) {
	  gprint (GP_LOG, "%f ",     catalog.average[k].dR);
	  gprint (GP_LOG, "%f ",     catalog.average[k].dD);
	  gprint (GP_LOG, "%f ",     catalog.average[k].uR);
	  gprint (GP_LOG, "%f ",     catalog.average[k].uD);
	  gprint (GP_LOG, "%f ",     catalog.average[k].duR);
	  gprint (GP_LOG, "%f ",     catalog.average[k].duD);
	  gprint (GP_LOG, "%f ",     catalog.average[k].P);
	  gprint (GP_LOG, "%f   ",     catalog.average[k].dP);

	  gprint (GP_LOG, "%f   ",     catalog.average[k].ChiSqPM);
	  gprint (GP_LOG, "%f   ",     catalog.average[k].ChiSqPar);
	  gprint (GP_LOG, "%d   ",     catalog.average[k].Npos);

	  date = ohana_sec_to_date (catalog.average[k].Tmean);
	  gprint (GP_LOG, "%20s ",     date);
	  gprint (GP_LOG, "%f   ",     catalog.average[k].Trange / 86400.0);
	}

	gprint (GP_LOG, "\n\n");
      
	/* filter names */
	gprint (GP_LOG, "filter     : ");
	for (j = 0; j < Nsecfilt; j++) printPhotcodeSequence (&catalog.average[k], &catalog.secfilt[Nsecfilt*k], j, GSTAR_FILTER_NAMES);
	gprint (GP_LOG, "\n");

	/* average mags */
	gprint (GP_LOG, "chp_psf_ave: ");
	for (j = 0; j < Nsecfilt; j++) printPhotcodeSequence (&catalog.average[k], &catalog.secfilt[Nsecfilt*k], j, GSTAR_AVE_MAG);
	gprint (GP_LOG, "\n");

	if (EXTRA_OUTPUT) {
	  /* Mmin */
	  gprint (GP_LOG, "chp_psf_min: ");
	  for (j = 0; j < Nsecfilt; j++) printPhotcodeSequence (&catalog.average[k], &catalog.secfilt[Nsecfilt*k], j, GSTAR_AVE_MAG_MIN);
	  gprint (GP_LOG, "\n");

	  /* Mmax */
	  gprint (GP_LOG, "chp_psf_max: ");
	  for (j = 0; j < Nsecfilt; j++) printPhotcodeSequence (&catalog.average[k], &catalog.secfilt[Nsecfilt*k], j, GSTAR_AVE_MAG_MAX);;
	  gprint (GP_LOG, "\n");
	}

	/* average mag errors */
	gprint (GP_LOG, "chp_psf_err: ");
	for (j = 0; j < Nsecfilt; j++) printPhotcodeSequence (&catalog.average[k], &catalog.secfilt[Nsecfilt*k], j, GSTAR_AVE_MAG_ERR);
	gprint (GP_LOG, "\n");

	/* average mag chisq */
	gprint (GP_LOG, "chp_psf_chi: ");
	for (j = 0; j < Nsecfilt; j++) printPhotcodeSequence (&catalog.average[k], &catalog.secfilt[Nsecfilt*k], j, GSTAR_AVE_MAG_CHISQ);
	gprint (GP_LOG, "\n");

	/* average mag Ncode */
	gprint (GP_LOG, "chp_Ncode:   ");
	for (j = 0; j < Nsecfilt; j++) printPhotcodeSequence (&catalog.average[k], &catalog.secfilt[Nsecfilt*k], j, GSTAR_AVE_NCODE);
	gprint (GP_LOG, "\n");
	
	/* average mag Nused */
	gprint (GP_LOG, "chp_Nused:   ");
	for (j = 0; j < Nsecfilt; j++) printPhotcodeSequence (&catalog.average[k], &catalog.secfilt[Nsecfilt*k], j, GSTAR_AVE_NUSED);
	gprint (GP_LOG, "\n");
	
	if (APER_OUTPUT) {
	  /* Map */
	  gprint (GP_LOG, "chp_ap__ave: ");
	  for (j = 0; j < Nsecfilt; j++) printPhotcodeSequence (&catalog.average[k], &catalog.secfilt[Nsecfilt*k], j, GSTAR_AVE_AP_MAG);
	  gprint (GP_LOG, "\n");

	  gprint (GP_LOG, "chp_ap__err: ");
	  for (j = 0; j < Nsecfilt; j++) printPhotcodeSequence (&catalog.average[k], &catalog.secfilt[Nsecfilt*k], j, GSTAR_AVE_AP_MAG_ERR);
	  gprint (GP_LOG, "\n");

	  gprint (GP_LOG, "chp_ap__chi: ");
	  for (j = 0; j < Nsecfilt; j++) printPhotcodeSequence (&catalog.average[k], &catalog.secfilt[Nsecfilt*k], j, GSTAR_AVE_AP_MAG_CHISQ);
	  gprint (GP_LOG, "\n");

	  /* average mag Nused */
	  gprint (GP_LOG, "chp_ap__N:   ");
	  for (j = 0; j < Nsecfilt; j++) printPhotcodeSequence (&catalog.average[k], &catalog.secfilt[Nsecfilt*k], j, GSTAR_AVE_AP_NUSED);
	  gprint (GP_LOG, "\n");
	}

	if (KRON_OUTPUT) {
	  /* Mkron */
	  gprint (GP_LOG, "chp_krn_ave: ");
	  for (j = 0; j < Nsecfilt; j++) printPhotcodeSequence (&catalog.average[k], &catalog.secfilt[Nsecfilt*k], j, GSTAR_AVE_KRON_MAG);
	  gprint (GP_LOG, "\n");

	  /* dMkron */
	  gprint (GP_LOG, "chp_krn_err: ");
	  for (j = 0; j < Nsecfilt; j++) printPhotcodeSequence (&catalog.average[k], &catalog.secfilt[Nsecfilt*k], j, GSTAR_AVE_KRON_MAG_ERR);
	  gprint (GP_LOG, "\n");

	  gprint (GP_LOG, "chp_krn_chi: ");
	  for (j = 0; j < Nsecfilt; j++) printPhotcodeSequence (&catalog.average[k], &catalog.secfilt[Nsecfilt*k], j, GSTAR_AVE_KRON_MAG_CHISQ);
	  gprint (GP_LOG, "\n");

	  /* average mag Nused */
	  gprint (GP_LOG, "chp_krn_N:   ");
	  for (j = 0; j < Nsecfilt; j++) printPhotcodeSequence (&catalog.average[k], &catalog.secfilt[Nsecfilt*k], j, GSTAR_AVE_KRON_NUSED);
	  gprint (GP_LOG, "\n");
	} 

	if (STACK_OUTPUT && MAG_OUTPUT) {
	  /* FluxPSF */
	  gprint (GP_LOG, "stk_psf_ave: ");
	  for (j = 0; j < Nsecfilt; j++) printPhotcodeSequence (&catalog.average[k], &catalog.secfilt[Nsecfilt*k], j, GSTAR_STACK_MAG_PSF);
	  gprint (GP_LOG, "\n");
  
	  /* dFluxPSF */
	  gprint (GP_LOG, "stk_psf_err: ");
	  for (j = 0; j < Nsecfilt; j++) printPhotcodeSequence (&catalog.average[k], &catalog.secfilt[Nsecfilt*k], j, GSTAR_STACK_MAG_PSF_ERR);
	  gprint (GP_LOG, "\n");
  
	  if (!FLUX_OUTPUT) {
	    gprint (GP_LOG, "stk_Nstack: ");
	    for (j = 0; j < Nsecfilt; j++) printPhotcodeSequence (&catalog.average[k], &catalog.secfilt[Nsecfilt*k], j, GSTAR_STACK_NUSED);
	    gprint (GP_LOG, "\n");
	  }

	  if (KRON_OUTPUT) {
	    /* MagKron */
	    gprint (GP_LOG, "stk_krn_ave: ");
	    for (j = 0; j < Nsecfilt; j++) printPhotcodeSequence (&catalog.average[k], &catalog.secfilt[Nsecfilt*k], j, GSTAR_STACK_MAG_KRON);
	    gprint (GP_LOG, "\n");
  
	    /* dMagKron */
	    gprint (GP_LOG, "stk_krn_err: ");
	    for (j = 0; j < Nsecfilt; j++) printPhotcodeSequence (&catalog.average[k], &catalog.secfilt[Nsecfilt*k], j, GSTAR_STACK_MAG_KRON_ERR);
	    gprint (GP_LOG, "\n");
	  }

	  if (APER_OUTPUT) {
	    /* MagAper */
	    gprint (GP_LOG, "stk_ap__ave: ");
	    for (j = 0; j < Nsecfilt; j++) printPhotcodeSequence (&catalog.average[k], &catalog.secfilt[Nsecfilt*k], j, GSTAR_STACK_MAG_APER);
	    gprint (GP_LOG, "\n");
  
	    /* dMagAper */
	    gprint (GP_LOG, "stk_ap__err: ");
	    for (j = 0; j < Nsecfilt; j++) printPhotcodeSequence (&catalog.average[k], &catalog.secfilt[Nsecfilt*k], j, GSTAR_STACK_MAG_APER_ERR);
	    gprint (GP_LOG, "\n");
	  }
	}
	if (WARP_OUTPUT && MAG_OUTPUT) {
	  /* FluxPSF */
	  gprint (GP_LOG, "wrp_psf_ave: ");
	  for (j = 0; j < Nsecfilt; j++) printPhotcodeSequence (&catalog.average[k], &catalog.secfilt[Nsecfilt*k], j, GSTAR_WARP_MAG_PSF);
	  gprint (GP_LOG, "\n");
  
	  /* dFluxPSF */
	  gprint (GP_LOG, "wrp_psf_err: ");
	  for (j = 0; j < Nsecfilt; j++) printPhotcodeSequence (&catalog.average[k], &catalog.secfilt[Nsecfilt*k], j, GSTAR_WARP_MAG_PSF_ERR);
	  gprint (GP_LOG, "\n");
  
	  gprint (GP_LOG, "wrp_psf_chi: ");
	  for (j = 0; j < Nsecfilt; j++) printPhotcodeSequence (&catalog.average[k], &catalog.secfilt[Nsecfilt*k], j, GSTAR_WARP_MAG_PSF_CHISQ);
	  gprint (GP_LOG, "\n");
  
	  if (!FLUX_OUTPUT) {
	    gprint (GP_LOG, "wrp_Nwarp:   ");
	    for (j = 0; j < Nsecfilt; j++) printPhotcodeSequence (&catalog.average[k], &catalog.secfilt[Nsecfilt*k], j, GSTAR_WARP_NWARP);
	    gprint (GP_LOG, "\n");
	    
	    gprint (GP_LOG, "wrp_NGood:   ");
	    for (j = 0; j < Nsecfilt; j++) printPhotcodeSequence (&catalog.average[k], &catalog.secfilt[Nsecfilt*k], j, GSTAR_WARP_NWARP_GOOD);
	    gprint (GP_LOG, "\n");

	    gprint (GP_LOG, "wrp_NUsed:   ");
	    for (j = 0; j < Nsecfilt; j++) printPhotcodeSequence (&catalog.average[k], &catalog.secfilt[Nsecfilt*k], j, GSTAR_WARP_NWARP_USED_PSF);
	    gprint (GP_LOG, "\n");
	  }
	  
	  if (KRON_OUTPUT) {
	    /* MagKron */
	    gprint (GP_LOG, "wrp_krn_ave: ");
	    for (j = 0; j < Nsecfilt; j++) printPhotcodeSequence (&catalog.average[k], &catalog.secfilt[Nsecfilt*k], j, GSTAR_WARP_MAG_KRON);
	    gprint (GP_LOG, "\n");
  
	    /* dMagKron */
	    gprint (GP_LOG, "wrp_krn_err: ");
	    for (j = 0; j < Nsecfilt; j++) printPhotcodeSequence (&catalog.average[k], &catalog.secfilt[Nsecfilt*k], j, GSTAR_WARP_MAG_KRON_ERR);
	    gprint (GP_LOG, "\n");

	    /* dMagKron */
	    gprint (GP_LOG, "wrp_krn_chp: ");
	    for (j = 0; j < Nsecfilt; j++) printPhotcodeSequence (&catalog.average[k], &catalog.secfilt[Nsecfilt*k], j, GSTAR_WARP_MAG_KRON_CHISQ);
	    gprint (GP_LOG, "\n");

	    if (!FLUX_OUTPUT) {
	      gprint (GP_LOG, "wrp_krn_Nuse:");
	      for (j = 0; j < Nsecfilt; j++) printPhotcodeSequence (&catalog.average[k], &catalog.secfilt[Nsecfilt*k], j, GSTAR_WARP_NWARP_USED_KRON);
	      gprint (GP_LOG, "\n");
	    }
	  }
	  
	  if (APER_OUTPUT) {
	    /* MagAper */
	    gprint (GP_LOG, "wrp_ap__ave: ");
	    for (j = 0; j < Nsecfilt; j++) printPhotcodeSequence (&catalog.average[k], &catalog.secfilt[Nsecfilt*k], j, GSTAR_WARP_MAG_APER);
	    gprint (GP_LOG, "\n");
  
	    /* dMagAper */
	    gprint (GP_LOG, "wrp_ap__err: ");
	    for (j = 0; j < Nsecfilt; j++) printPhotcodeSequence (&catalog.average[k], &catalog.secfilt[Nsecfilt*k], j, GSTAR_WARP_MAG_APER_ERR);
	    gprint (GP_LOG, "\n");

	    /* dMagAper */
	    gprint (GP_LOG, "wrp_ap__chi: ");
	    for (j = 0; j < Nsecfilt; j++) printPhotcodeSequence (&catalog.average[k], &catalog.secfilt[Nsecfilt*k], j, GSTAR_WARP_MAG_APER_CHISQ);
	    gprint (GP_LOG, "\n");

	    if (!FLUX_OUTPUT) {
	      gprint (GP_LOG, "wrp_ap_Nuse: ");
	      for (j = 0; j < Nsecfilt; j++) printPhotcodeSequence (&catalog.average[k], &catalog.secfilt[Nsecfilt*k], j, GSTAR_WARP_NWARP_USED_AP);
	      gprint (GP_LOG, "\n");
	    }
	  }
	}
	if (STACK_OUTPUT && FLUX_OUTPUT) {
	  /* FluxPSF */
	  gprint (GP_LOG, "stk_psf_ave: ");
	  for (j = 0; j < Nsecfilt; j++) printPhotcodeSequence (&catalog.average[k], &catalog.secfilt[Nsecfilt*k], j, GSTAR_STACK_FLUX_PSF);
	  gprint (GP_LOG, "\n");
  
	  /* dFluxPSF */
	  gprint (GP_LOG, "stk_psf_err: ");
	  for (j = 0; j < Nsecfilt; j++) printPhotcodeSequence (&catalog.average[k], &catalog.secfilt[Nsecfilt*k], j, GSTAR_STACK_FLUX_PSF_ERR);
	  gprint (GP_LOG, "\n");
  
	  if (KRON_OUTPUT) {
	    /* FluxKron */
	    gprint (GP_LOG, "stk_krn_ave: ");
	    for (j = 0; j < Nsecfilt; j++) printPhotcodeSequence (&catalog.average[k], &catalog.secfilt[Nsecfilt*k], j, GSTAR_STACK_FLUX_KRON);
	    gprint (GP_LOG, "\n");
  
	    /* dFluxKron */
	    gprint (GP_LOG, "stk_krn_err: ");
	    for (j = 0; j < Nsecfilt; j++) printPhotcodeSequence (&catalog.average[k], &catalog.secfilt[Nsecfilt*k], j, GSTAR_STACK_FLUX_KRON_ERR);
	    gprint (GP_LOG, "\n");
	  }

	  if (APER_OUTPUT) {
	    /* FluxAper */
	    gprint (GP_LOG, "stk_ap_ave:  ");
	    for (j = 0; j < Nsecfilt; j++) printPhotcodeSequence (&catalog.average[k], &catalog.secfilt[Nsecfilt*k], j, GSTAR_STACK_FLUX_APER);
	    gprint (GP_LOG, "\n");
  
	    /* dFluxAper */
	    gprint (GP_LOG, "stk_ap_err:  ");
	    for (j = 0; j < Nsecfilt; j++) printPhotcodeSequence (&catalog.average[k], &catalog.secfilt[Nsecfilt*k], j, GSTAR_STACK_FLUX_APER_ERR);
	    gprint (GP_LOG, "\n");
	  }

	  gprint (GP_LOG, "stk_Nstack:  ");
	  for (j = 0; j < Nsecfilt; j++) printPhotcodeSequence (&catalog.average[k], &catalog.secfilt[Nsecfilt*k], j, GSTAR_STACK_NUSED);
	  gprint (GP_LOG, "\n");
	}
	if (WARP_OUTPUT && FLUX_OUTPUT) {
	  /* FluxPSF */
	  gprint (GP_LOG, "wrp_psf_ave: ");
	  for (j = 0; j < Nsecfilt; j++) printPhotcodeSequence (&catalog.average[k], &catalog.secfilt[Nsecfilt*k], j, GSTAR_WARP_FLUX_PSF);
	  gprint (GP_LOG, "\n");
  
	  /* dFluxPSF */
	  gprint (GP_LOG, "wrp_psf_err: ");
	  for (j = 0; j < Nsecfilt; j++) printPhotcodeSequence (&catalog.average[k], &catalog.secfilt[Nsecfilt*k], j, GSTAR_WARP_FLUX_PSF_ERR);
	  gprint (GP_LOG, "\n");
  
	  gprint (GP_LOG, "wrp_psf_chi: ");
	  for (j = 0; j < Nsecfilt; j++) printPhotcodeSequence (&catalog.average[k], &catalog.secfilt[Nsecfilt*k], j, GSTAR_WARP_FLUX_PSF_CHISQ);
	  gprint (GP_LOG, "\n");
  
	  gprint (GP_LOG, "wrp_Nwarp:   ");
	  for (j = 0; j < Nsecfilt; j++) printPhotcodeSequence (&catalog.average[k], &catalog.secfilt[Nsecfilt*k], j, GSTAR_WARP_NWARP);
	  gprint (GP_LOG, "\n");
	  
	  gprint (GP_LOG, "wrp_NGood:   ");
	  for (j = 0; j < Nsecfilt; j++) printPhotcodeSequence (&catalog.average[k], &catalog.secfilt[Nsecfilt*k], j, GSTAR_WARP_NWARP_GOOD);
	  gprint (GP_LOG, "\n");

	  gprint (GP_LOG, "wrp_NUsed:   ");
	  for (j = 0; j < Nsecfilt; j++) printPhotcodeSequence (&catalog.average[k], &catalog.secfilt[Nsecfilt*k], j, GSTAR_WARP_NWARP_USED_PSF);
	  gprint (GP_LOG, "\n");

	  if (KRON_OUTPUT) {
	    /* FluxKron */
	    gprint (GP_LOG, "wrp_krn_ave: ");
	    for (j = 0; j < Nsecfilt; j++) printPhotcodeSequence (&catalog.average[k], &catalog.secfilt[Nsecfilt*k], j, GSTAR_WARP_FLUX_KRON);
	    gprint (GP_LOG, "\n");
  
	    /* dFluxKron */
	    gprint (GP_LOG, "wrp_krn_err: ");
	    for (j = 0; j < Nsecfilt; j++) printPhotcodeSequence (&catalog.average[k], &catalog.secfilt[Nsecfilt*k], j, GSTAR_WARP_FLUX_KRON_ERR);
	    gprint (GP_LOG, "\n");

	    gprint (GP_LOG, "wrp_krn_chi: ");
	    for (j = 0; j < Nsecfilt; j++) printPhotcodeSequence (&catalog.average[k], &catalog.secfilt[Nsecfilt*k], j, GSTAR_WARP_FLUX_KRON_CHISQ);
	    gprint (GP_LOG, "\n");

	    gprint (GP_LOG, "wrp_krn_Nuse:");
	    for (j = 0; j < Nsecfilt; j++) printPhotcodeSequence (&catalog.average[k], &catalog.secfilt[Nsecfilt*k], j, GSTAR_WARP_NWARP_USED_KRON);
	    gprint (GP_LOG, "\n");
	  }

	  if (APER_OUTPUT) {
	    /* FluxAper */
	    gprint (GP_LOG, "wrp_ap__ave: ");
	    for (j = 0; j < Nsecfilt; j++) printPhotcodeSequence (&catalog.average[k], &catalog.secfilt[Nsecfilt*k], j, GSTAR_WARP_FLUX_APER);
	    gprint (GP_LOG, "\n");
  
	    /* dFluxAper */
	    gprint (GP_LOG, "wrp_ap__err: ");
	    for (j = 0; j < Nsecfilt; j++) printPhotcodeSequence (&catalog.average[k], &catalog.secfilt[Nsecfilt*k], j, GSTAR_WARP_FLUX_APER_ERR);
	    gprint (GP_LOG, "\n");

	    gprint (GP_LOG, "wrp_ap__chi: ");
	    for (j = 0; j < Nsecfilt; j++) printPhotcodeSequence (&catalog.average[k], &catalog.secfilt[Nsecfilt*k], j, GSTAR_WARP_FLUX_APER_CHISQ);
	    gprint (GP_LOG, "\n");

	    gprint (GP_LOG, "wrp_ap_Nuse: ");
	    for (j = 0; j < Nsecfilt; j++) printPhotcodeSequence (&catalog.average[k], &catalog.secfilt[Nsecfilt*k], j, GSTAR_WARP_NWARP_USED_AP);
	    gprint (GP_LOG, "\n");
	  }
	}
	if (EXTRA_OUTPUT) {
	  /* secfilt flags */
	  gprint (GP_LOG, "filtflags:   ");
	  for (j = 0; j < Nsecfilt; j++) printPhotcodeSequence (&catalog.average[k], &catalog.secfilt[Nsecfilt*k], j, GSTAR_SECF_FLAGS);
	  gprint (GP_LOG, "\n");

	  /* UCDIST */
	  gprint (GP_LOG, "ubercal_dst: ");
	  for (j = 0; j < Nsecfilt; j++) printPhotcodeSequence (&catalog.average[k], &catalog.secfilt[Nsecfilt*k], j, GSTAR_UCDIST);;
	  gprint (GP_LOG, "\n");
	}
      }

      gprint (GP_LOG, "\n");
      
      if (GetMeasure) {

	if (catalog.average[k].Nmeasure == 0) continue;

	m = catalog.average[k].measureOffset;

	int *index = NULL;
	float *value = NULL;
	ALLOCATE (index, int, catalog.average[k].Nmeasure);
	ALLOCATE (value, float, catalog.average[k].Nmeasure);
	for (j = 0; j < catalog.average[k].Nmeasure; j++) {
	  index[j] = j + m;
	  switch (sortCode) {
	    case GSTAR_BY_SEQ:
	      value[j] = j + m;
	      break;
	    case GSTAR_BY_PHOTCODE:
	      value[j] = catalog.measure[j + m].photcode;
	      break;
	    case GSTAR_BY_TIME:
	      value[j] = catalog.measure[j + m].t;
	      break;
	    case GSTAR_BY_IMAGE_ID:
	      value[j] = catalog.measure[j + m].imageID;
	      break;
	    case GSTAR_BY_DET_ID:
	      value[j] = catalog.measure[j + m].detID;
	      break;
	  }
	}

	sort_float_index (value, index, catalog.average[k].Nmeasure);

	for (j = 0; j < catalog.average[k].Nmeasure; j++, m++) {

	  int Nv = index[j];

	  if (photcode) {
	    if ( photcodeEquiv && (photcode[0].code != GetPhotcodeEquivCodebyCode(catalog.measure[Nv].photcode))) continue;
	    if (!photcodeEquiv && (photcode[0].code != catalog.measure[Nv].photcode)) continue;
	  }

	  Mcat = PhotCat (&catalog.measure[Nv], MAG_CLASS_PSF);
	  if (INST) {
	    Mrel = PhotInst (&catalog.measure[Nv], MAG_CLASS_PSF);
	  } else {
	    Mrel = PhotRel (&catalog.measure[Nv], &catalog.average[k], &catalog.secfilt[k*Nsecfilt], MAG_CLASS_PSF);
	  }

	  float dRoff = dvoOffsetR(&catalog.measure[Nv], &catalog.average[k]);
	  float dDoff = dvoOffsetD(&catalog.measure[Nv], &catalog.average[k]);

	  if (GetMeasure && !QUIET) {
	    date = ohana_sec_to_date (catalog.measure[Nv].t);
	    gprint (GP_LOG, "%6.3f ",  Mcat);
	    gprint (GP_LOG, "%6.3f ",  Mrel);
	    gprint (GP_LOG, "%6.3f  ", catalog.measure[Nv].dM);
	    gprint (GP_LOG, "%20s  ",  date);
	    
	    gprint (GP_LOG, "%7.4f ",  dRoff);
	    gprint (GP_LOG, "%7.4f ",  dDoff);
	    gprint (GP_LOG, "0x%08x ", catalog.measure[Nv].photFlags);
	    gprint (GP_LOG, "0x%08x ", catalog.measure[Nv].dbFlags);
	    if (SHOW_MASKS) {
	      gprint (GP_LOG, "%5d ",    catalog.measure[Nv].photcode);
	      PhotCode *code = GetPhotcodebyCode (catalog.measure[Nv].photcode);
	      int badMask = 0;
	      int poorMask = 0;
	      if (code) {
		badMask = catalog.measure[Nv].photFlags & code->photomBadMask;
		poorMask = catalog.measure[Nv].photFlags & code->photomPoorMask;
	      }
	      gprint (GP_LOG, "0x%08x ", poorMask);
	      gprint (GP_LOG, "0x%08x ", badMask);
	    }
	    gprint (GP_LOG, "%5d ",    catalog.measure[Nv].photcode);
	    gprint (GP_LOG, "%-20s ",  GetPhotcodeNamebyCode (catalog.measure[Nv].photcode));
 	    gprint (GP_LOG, "%5.2f ",  FromShortPixels(catalog.measure[Nv].FWx));
	    gprint (GP_LOG, "%5.2f ",  FromShortPixels(catalog.measure[Nv].FWy));
 	    gprint (GP_LOG, "%5.2f ",  FromShortPixels(catalog.measure[Nv].Mxx));
	    gprint (GP_LOG, "%5.2f ",  FromShortPixels(catalog.measure[Nv].Mxy));
	    gprint (GP_LOG, "%5.2f ",  FromShortPixels(catalog.measure[Nv].Myy));

	    if (FULL_OUTPUT) {
	      gprint (GP_LOG, "%6.3f ", catalog.measure[Nv].McalPSF);
	      gprint (GP_LOG, "%6.3f ", catalog.measure[Nv].McalAPER);
	      gprint (GP_LOG, "%6.3f ", catalog.measure[Nv].Mflat);
	      Mrel = PhotRel (&catalog.measure[Nv], &catalog.average[k], &catalog.secfilt[k*Nsecfilt], MAG_CLASS_APER);
	      gprint (GP_LOG, "%6.3f ", Mrel);
	      Mrel = PhotRel (&catalog.measure[Nv], &catalog.average[k], &catalog.secfilt[k*Nsecfilt], MAG_CLASS_KRON);
	      gprint (GP_LOG, "%6.3f ", Mrel);
	      gprint (GP_LOG, "%6.3f ", catalog.measure[Nv].dMkron);
	      gprint (GP_LOG, "%5.1f ", pow(10.0, 0.4*catalog.measure[Nv].dt));
	      gprint (GP_LOG, "%5.3f ", catalog.measure[Nv].airmass);
	      gprint (GP_LOG, "%6.1f ", catalog.measure[Nv].az);
	      gprint (GP_LOG, "%6.1f ", catalog.measure[Nv].Xccd);
	      gprint (GP_LOG, "%6.1f ", catalog.measure[Nv].Yccd);
	      gprint (GP_LOG, "%3.1f ", FromShortPixels(catalog.measure[Nv].dXccd));
	      gprint (GP_LOG, "%3.1f ", FromShortPixels(catalog.measure[Nv].dYccd));
	      gprint (GP_LOG, "%6.1f ", catalog.measure[Nv].Sky);
	      gprint (GP_LOG, "%5.1f ", catalog.measure[Nv].dSky);
	      gprint (GP_LOG, "%8d ", catalog.measure[Nv].averef);
	      gprint (GP_LOG, "0x%08x ", catalog.measure[Nv].detID);
	      gprint (GP_LOG, "0x%08x ", catalog.measure[Nv].imageID);
	      gprint (GP_LOG, "%5.3f ", catalog.measure[Nv].psfQF);
	      gprint (GP_LOG, "%5.3f ", catalog.measure[Nv].psfQFperf);
	      gprint (GP_LOG, "%7.1f ", catalog.measure[Nv].psfChisq);
	      // gprint (GP_LOG, "%3.1f ", catalog.measure[Nv].crNsigma);
	      gprint (GP_LOG, "%4.1f ", catalog.measure[Nv].extNsigma);
	      gprint (GP_LOG, "%5.1f ", FromShortDegrees(catalog.measure[Nv].theta));
	    }
	    if (FLUX_OUTPUT) {
	      gprint (GP_LOG, "%10.3e ", catalog.measure[Nv].FluxPSF);
	      gprint (GP_LOG, "%10.3e ", catalog.measure[Nv].dFluxPSF);
	      gprint (GP_LOG, "%10.3e ", catalog.measure[Nv].FluxKron);
	      gprint (GP_LOG, "%10.3e ", catalog.measure[Nv].dFluxKron);
	      gprint (GP_LOG, "%10.3e ", catalog.measure[Nv].FluxAp);
	      gprint (GP_LOG, "%10.3e ", catalog.measure[Nv].dFluxAp);
	    }

	    if (GetLensing && catalog.average[k].Nlensing) {
	      int mLens = catalog.average[k].lensingOffset;
	      int foundLens = FALSE;
	      for (int Nlens = 0; !foundLens && (Nlens < catalog.average[k].Nlensing); Nlens ++) {
		Lensing *lensing = &catalog.lensing[mLens + Nlens];
		if (lensing->imageID != catalog.measure[Nv].imageID) continue;

		gprint (GP_LOG, "%10.3e ",  lensing->X11_sm_obj);
		gprint (GP_LOG, "%10.3e ",  lensing->X12_sm_obj);
		gprint (GP_LOG, "%10.3e ",  lensing->X22_sm_obj);
		gprint (GP_LOG, "%10.3e ",  lensing-> E1_sm_obj);
		gprint (GP_LOG, "%10.3e ",  lensing-> E2_sm_obj);
		gprint (GP_LOG, "%10.3e ",  lensing->X11_sh_obj);
		gprint (GP_LOG, "%10.3e ",  lensing->X12_sh_obj);
		gprint (GP_LOG, "%10.3e ",  lensing->X22_sh_obj);
		gprint (GP_LOG, "%10.3e ",  lensing-> E1_sh_obj);
		gprint (GP_LOG, "%10.3e ",  lensing-> E2_sh_obj);
		gprint (GP_LOG, "%10.3e ",  lensing->X11_sm_psf);
		gprint (GP_LOG, "%10.3e ",  lensing->X12_sm_psf);
		gprint (GP_LOG, "%10.3e ",  lensing->X22_sm_psf);
		gprint (GP_LOG, "%10.3e ",  lensing-> E1_sm_psf);
		gprint (GP_LOG, "%10.3e ",  lensing-> E2_sm_psf);
		gprint (GP_LOG, "%10.3e ",  lensing->X11_sh_psf);
		gprint (GP_LOG, "%10.3e ",  lensing->X12_sh_psf);
		gprint (GP_LOG, "%10.3e ",  lensing->X22_sh_psf);
		gprint (GP_LOG, "%10.3e ",  lensing-> E1_sh_psf);
		gprint (GP_LOG, "%10.3e ",  lensing-> E2_sh_psf);
		gprint (GP_LOG, " %10.3e ", lensing-> E1_psf);
		gprint (GP_LOG,  "%10.3e ", lensing-> E2_psf);
		foundLens = TRUE;
	      }	      
	    }

	    gprint (GP_LOG, "\n");
	    free (date);
	  }
	  
	  if (SaveVectors) {
	    vec1[0].elements.Flt[N] = Mcat;
	    vec2[0].elements.Flt[N] = TimeValue (catalog.measure[Nv].t, TimeReference, TimeFormat);
	    vec3[0].elements.Flt[N] = catalog.measure[Nv].airmass;
	    vec4[0].elements.Flt[N] = catalog.measure[Nv].photcode;
	    vec5[0].elements.Flt[N] = dRoff;
	    vec6[0].elements.Flt[N] = dDoff;
	    N ++;
	    if (N == NPTS - 1) {
	      NPTS += 2000;
	      REALLOCATE (vec1[0].elements.Flt, opihi_flt, NPTS);
	      REALLOCATE (vec2[0].elements.Flt, opihi_flt, NPTS);
	      REALLOCATE (vec3[0].elements.Flt, opihi_flt, NPTS);
	      REALLOCATE (vec4[0].elements.Flt, opihi_flt, NPTS);
	      REALLOCATE (vec5[0].elements.Flt, opihi_flt, NPTS);
	      REALLOCATE (vec6[0].elements.Flt, opihi_flt, NPTS);
	    }
	  }
	}
	free (value);
	free (index);
      }
    }
  }

  if (SaveVectors) {
    vec1[0].Nelements = N;
    vec2[0].Nelements = N;
    vec3[0].Nelements = N;
    vec4[0].Nelements = N;
    vec5[0].Nelements = N;
    vec6[0].Nelements = N;
  }

  free (RA);
  free (DEC);
  free (N1);
  dvo_catalog_free (&catalog);

  freePhotcodeSequence ();
  return (TRUE);

}

void print_double (double value) {
  if (isnan(value)) 
    gprint (GP_LOG, "       NaN ");
  else 
    gprint (GP_LOG, "   %7.4f ", value);
}

void print_double_exp (double value) {
  if (isnan(value)) 
    gprint (GP_LOG, "       NaN ");
  else 
    gprint (GP_LOG, "%10.3e ", value);
}

void print_short (double value, short int ival) {
  if (ival == NAN_S_SHORT) 
    gprint (GP_LOG, "       NaN ");
  else 
    gprint (GP_LOG, "   %7.4f ", value);
}

// XXX fix printing to be in photcode numerical order for PRI/SEC data
static int *sequence = NULL;

void initPhotcodeSequence (int Nsecfilt) {

  int j;
  int *codeNumber;
  PhotCode *code;

  // sequence contains, in desired order, secfilt number (0 == pri)
  ALLOCATE (sequence, int, Nsecfilt);
  ALLOCATE (codeNumber, int, Nsecfilt);
  
  /* filter names -- primary code is 0 in this function */
  for (j = 0; j < Nsecfilt; j++) {
    code = GetPhotcodebyNsec (j);
    codeNumber[j] = code[0].code;
    sequence[j] = j;
  }

  isortpair (codeNumber, sequence, Nsecfilt);
  free (codeNumber);
}

void freePhotcodeSequence () {
  free (sequence);
}

void printPhotcodeSequence (Average *average, SecFilt *secfilt, int entry, int type) {
  OHANA_UNUSED_PARAM(average);

  int seq;
  PhotCode *code;

  seq = sequence[entry];

  switch (type) {
    case GSTAR_AVE_MAG_MIN: /* average ap mags */
      if (seq == -1) {
	print_double (NAN);
      } else {
	print_double (secfilt[seq].Mmin);
      }
      break;

    case GSTAR_AVE_MAG_MAX: /* average ap mags */
      if (seq == -1) {
	print_double (NAN);
      } else {
	print_double (secfilt[seq].Mmax);
      }
      break;

    case GSTAR_AVE_MAG_CHISQ: /* average mag chisq */
      if (seq == -1) {
	print_double (NAN);
      } else {
	print_double_exp (secfilt[seq].Mchisq);
      }
      break;

    case GSTAR_AVE_MAG_STDEV: /* average mag chisq */
      if (seq == -1) {
	print_double (NAN);
      } else {
	print_double_exp (secfilt[seq].sMpsfChp);
      }
      break;

    case GSTAR_FILTER_NAMES: /* filter names */
      code = GetPhotcodebyNsec (seq);
      gprint (GP_LOG, "%10s ", code[0].name);
      break;

    case GSTAR_SECF_FLAGS: /* secfilt flags */
      if (seq == -1) {
	gprint (GP_LOG, "0x%08x ", 0);
      } else {
	gprint (GP_LOG, "0x%08x ", secfilt[seq].flags);
      }
      break;

    case GSTAR_UCDIST: /* ubercal distance */
      if (seq == -1) {
	gprint (GP_LOG, "%10d ", 0);
      } else {
	gprint (GP_LOG, "%10d ", secfilt[seq].ubercalDist);
      }
      break;

    case GSTAR_AVE_NCODE: /* Ncode */
      if (seq == -1) {
	gprint (GP_LOG, "%10d ", 0);
      } else {
	gprint (GP_LOG, "%10d ", secfilt[seq].Ncode);
      }
      break;

    case GSTAR_AVE_NUSED: /* Nused */
      if (seq == -1) {
	gprint (GP_LOG, "%10d ", 0);
      } else {
	gprint (GP_LOG, "%10d ", secfilt[seq].Nused);
      }
      break;

    case GSTAR_AVE_KRON_NUSED: /* Nused */
      if (seq == -1) {
	gprint (GP_LOG, "%10d ", 0);
      } else {
	gprint (GP_LOG, "%10d ", secfilt[seq].NusedKron);
      }
      break;

    case GSTAR_AVE_AP_NUSED: /* Nused */
      if (seq == -1) {
	gprint (GP_LOG, "%10d ", 0);
      } else {
	gprint (GP_LOG, "%10d ", secfilt[seq].NusedAp);
      }
      break;

      /*** CHIP : MAG ***/

    case GSTAR_AVE_MAG: /* average mags */
      if (seq == -1) {
	print_double (NAN);
      } else {
	print_double (secfilt[seq].MpsfChp);
      }
      break;

    case GSTAR_AVE_MAG_ERR: /* average mags errors */
      if (seq == -1) {
	print_double (NAN);
      } else {
	print_double (secfilt[seq].dMpsfChp);
      }
      break;

    case GSTAR_AVE_AP_MAG: /* average ap mags */
      if (seq == -1) {
	print_double (NAN);
      } else {
	print_double (secfilt[seq].MapChp);
      }
      break;

    case GSTAR_AVE_AP_MAG_ERR: /* average ap mags */
      if (seq == -1) {
	print_double (NAN);
      } else {
	print_double (secfilt[seq].dMapChp);
      }
      break;

    case GSTAR_AVE_AP_MAG_CHISQ: /* average ap mags */
      if (seq == -1) {
	print_double (NAN);
      } else {
	print_double_exp (secfilt[seq].sMapChp);
      }
      break;

    case GSTAR_AVE_AP_MAG_STDEV: /* average ap mags */
      if (seq == -1) {
	print_double (NAN);
      } else {
	print_double_exp (secfilt[seq].dMapChp);
      }
      break;

    case GSTAR_AVE_KRON_MAG: /* average ap mags */
      if (seq == -1) {
	print_double (NAN);
      } else {
	print_double (secfilt[seq].MkronChp);
      }
      break;

    case GSTAR_AVE_KRON_MAG_ERR: /* average ap mags */
      if (seq == -1) {
	print_double (NAN);
      } else {
	print_double (secfilt[seq].dMkronChp);
      }
      break;

    case GSTAR_AVE_KRON_MAG_CHISQ: /* average ap mags */
      if (seq == -1) {
	print_double (NAN);
      } else {
	print_double_exp (secfilt[seq].sMkronChp);
      }
      break;

    case GSTAR_AVE_KRON_MAG_STDEV: /* average ap mags */
      if (seq == -1) {
	print_double (NAN);
      } else {
	print_double_exp (secfilt[seq].dMkronChp);
      }
      break;

      /******************* STACK : FLUX **********************/

    case GSTAR_STACK_FLUX_PSF: /* average ap mags */
      if (seq == -1) {
	print_double (NAN);
      } else {
	print_double_exp (secfilt[seq].FpsfStk);
      }
      break;

    case GSTAR_STACK_FLUX_PSF_ERR: /* average ap mags */
      if (seq == -1) {
	print_double (NAN);
      } else {
	print_double_exp (secfilt[seq].dFpsfStk);
      }
      break;

    case GSTAR_STACK_FLUX_KRON: /* average ap mags */
      if (seq == -1) {
	print_double (NAN);
      } else {
	print_double_exp (secfilt[seq].FkronStk);
      }
      break;

    case GSTAR_STACK_FLUX_KRON_ERR: /* average ap mags */
      if (seq == -1) {
	print_double (NAN);
      } else {
	print_double_exp (secfilt[seq].dFkronStk);
      }
      break;

    case GSTAR_STACK_FLUX_APER: /* average ap mags */
      if (seq == -1) {
	print_double (NAN);
      } else {
	print_double_exp (secfilt[seq].FapStk);
      }
      break;

    case GSTAR_STACK_FLUX_APER_ERR: /* average ap mags */
      if (seq == -1) {
	print_double (NAN);
      } else {
	print_double_exp (secfilt[seq].dFapStk);
      }
      break;

    case GSTAR_STACK_NUSED: /* Nused */
      if (seq == -1) {
	gprint (GP_LOG, "%10d ", 0);
      } else {
	gprint (GP_LOG, "%10d ", secfilt[seq].Nstack);
      }
      break;

      /******************* STACK : MAG **********************/

    case GSTAR_STACK_MAG_PSF: /* average ap mags */
      if (seq == -1) {
	print_double (NAN);
      } else {
	print_double (secfilt[seq].MpsfStk);
      }
      break;

    case GSTAR_STACK_MAG_PSF_ERR: /* average ap mags */
      if (seq == -1) {
	print_double (NAN);
      } else {
	print_double (secfilt[seq].dFpsfStk / secfilt[seq].FpsfStk);
      }
      break;

    case GSTAR_STACK_MAG_KRON: /* average ap mags */
      if (seq == -1) {
	print_double (NAN);
      } else {
	print_double (secfilt[seq].MkronStk);
      }
      break;

    case GSTAR_STACK_MAG_KRON_ERR: /* average ap mags */
      if (seq == -1) {
	print_double (NAN);
      } else {
	print_double (secfilt[seq].dFkronStk / secfilt[seq].FkronStk);
      }
      break;

    case GSTAR_STACK_MAG_APER: /* average ap mags */
      if (seq == -1) {
	print_double (NAN);
      } else {
	print_double (secfilt[seq].MapStk);
      }
      break;

    case GSTAR_STACK_MAG_APER_ERR: /* average ap mags */
      if (seq == -1) {
	print_double (NAN);
      } else {
	print_double (secfilt[seq].dFapStk / secfilt[seq].FapStk);
      }
      break;

      /******************* WARP : FLUX **********************/

    case GSTAR_WARP_FLUX_PSF: /* average ap mags */
      if (seq == -1) {
	print_double (NAN);
      } else {
	print_double_exp (secfilt[seq].FpsfWrp);
      }
      break;

    case GSTAR_WARP_FLUX_PSF_ERR: /* average ap mags */
      if (seq == -1) {
	print_double (NAN);
      } else {
	print_double_exp (secfilt[seq].dFpsfWrp);
      }
      break;

    case GSTAR_WARP_FLUX_PSF_CHISQ: /* average ap mags */
      if (seq == -1) {
	print_double (NAN);
      } else {
	print_double_exp (secfilt[seq].sFpsfWrp);
      }
      break;

    case GSTAR_WARP_FLUX_KRON: /* average ap mags */
      if (seq == -1) {
	print_double (NAN);
      } else {
	print_double_exp (secfilt[seq].FkronWrp);
      }
      break;

    case GSTAR_WARP_FLUX_KRON_ERR: /* average ap mags */
      if (seq == -1) {
	print_double (NAN);
      } else {
	print_double_exp (secfilt[seq].dFkronWrp);
      }
      break;

    case GSTAR_WARP_FLUX_KRON_CHISQ: /* average ap mags */
      if (seq == -1) {
	print_double (NAN);
      } else {
	print_double_exp (secfilt[seq].sFkronWrp);
      }
      break;

    case GSTAR_WARP_FLUX_APER: /* average ap mags */
      if (seq == -1) {
	print_double (NAN);
      } else {
	print_double_exp (secfilt[seq].FapWrp);
      }
      break;

    case GSTAR_WARP_FLUX_APER_ERR: /* average ap mags */
      if (seq == -1) {
	print_double (NAN);
      } else {
	print_double_exp (secfilt[seq].dFapWrp);
      }
      break;

    case GSTAR_WARP_FLUX_APER_CHISQ: /* average ap mags */
      if (seq == -1) {
	print_double_exp (NAN);
      } else {
	print_double_exp (secfilt[seq].sFapWrp);
      }
      break;

      /******************* WARP : MAG **********************/

    case GSTAR_WARP_MAG_PSF: /* average ap mags */
      if (seq == -1) {
	print_double (NAN);
      } else {
	print_double (secfilt[seq].MpsfWrp);
      }
      break;

    case GSTAR_WARP_MAG_PSF_ERR: /* average ap mags */
      if (seq == -1) {
	print_double (NAN);
      } else {
	print_double (secfilt[seq].dFpsfWrp / secfilt[seq].FpsfWrp);
      }
      break;

    case GSTAR_WARP_MAG_PSF_CHISQ: /* average ap mags */
      if (seq == -1) {
	print_double_exp (NAN);
      } else {
	print_double_exp (secfilt[seq].sFpsfWrp);
      }
      break;

    case GSTAR_WARP_MAG_KRON: /* average ap mags */
      if (seq == -1) {
	print_double (NAN);
      } else {
	print_double (secfilt[seq].MkronWrp);
      }
      break;

    case GSTAR_WARP_MAG_KRON_ERR: /* average ap mags */
      if (seq == -1) {
	print_double (NAN);
      } else {
	print_double (secfilt[seq].dFkronWrp / secfilt[seq].FkronWrp);
      }
      break;

    case GSTAR_WARP_MAG_KRON_CHISQ: /* average ap mags */
      if (seq == -1) {
	print_double_exp (NAN);
      } else {
	print_double_exp (secfilt[seq].sFkronWrp);
      }
      break;

    case GSTAR_WARP_MAG_APER: /* average ap mags */
      if (seq == -1) {
	print_double (NAN);
      } else {
	print_double (secfilt[seq].MapWrp);
      }
      break;

    case GSTAR_WARP_MAG_APER_ERR: /* average ap mags */
      if (seq == -1) {
	print_double (NAN);
      } else {
	print_double (secfilt[seq].dFapWrp / secfilt[seq].FapWrp);
      }
      break;

    case GSTAR_WARP_MAG_APER_CHISQ: /* average ap mags */
      if (seq == -1) {
	print_double_exp (NAN);
      } else {
	print_double_exp (secfilt[seq].sFapWrp);
      }
      break;

    case GSTAR_WARP_NWARP: /* Nused */
      if (seq == -1) {
	gprint (GP_LOG, "%10d ", 0);
      } else {
	gprint (GP_LOG, "%10d ", secfilt[seq].Nwarp);
      }
      break;

    case GSTAR_WARP_NWARP_GOOD: /* Nused */
      if (seq == -1) {
	gprint (GP_LOG, "%10d ", 0);
      } else {
	gprint (GP_LOG, "%10d ", secfilt[seq].NwarpGood);
      }
      break;

    case GSTAR_WARP_NWARP_USED_PSF: /* Nused */
      if (seq == -1) {
	gprint (GP_LOG, "%10d ", 0);
      } else {
	gprint (GP_LOG, "%10d ", secfilt[seq].NusedWrp);
      }
      break;

    case GSTAR_WARP_NWARP_USED_KRON: /* Nused */
      if (seq == -1) {
	gprint (GP_LOG, "%10d ", 0);
      } else {
	gprint (GP_LOG, "%10d ", secfilt[seq].NusedKronWrp);
      }
      break;

    case GSTAR_WARP_NWARP_USED_AP: /* Nused */
      if (seq == -1) {
	gprint (GP_LOG, "%10d ", 0);
      } else {
	gprint (GP_LOG, "%10d ", secfilt[seq].NusedApWrp);
      }
      break;
  }
}
