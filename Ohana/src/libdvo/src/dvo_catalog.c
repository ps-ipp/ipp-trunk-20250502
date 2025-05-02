# include <dvo.h>
# define DEBUG 1

void dvo_catalog_test (Catalog *catalog, int halt) {

  Catalog *subcat;

  // fprintf (stderr, "catalog: Naverage = %d, average = %zx\n", catalog[0].Naverage, (size_t) catalog[0].average);
  // fprintf (stderr, "catalog: Nmeasure = %d, measure = %zx\n", catalog[0].Nmeasure, (size_t) catalog[0].measure);
  // fprintf (stderr, "catalog: Nmissing = %d, missing = %zx\n", catalog[0].Nmissing, (size_t) catalog[0].missing);
  // fprintf (stderr, "catalog: Nsecfilt = %d, secfilt = %zx\n", catalog[0].Nsecfilt, (size_t) catalog[0].secfilt);

  if (!catalog[0].measure || !catalog[0].secfilt) {
    fprintf (stderr, "error: %s\n", catalog[0].filename);
    if (halt) abort ();
  }

  // XXX test that things are correctly initialized
  if (catalog[0].catmode != DVO_MODE_SPLIT) return;

  subcat = catalog[0].measure_catalog;
  if (subcat) {
    if (subcat[0].measure_catalog || subcat[0].secfilt_catalog || subcat[0].missing_catalog) {
      fprintf (stderr, "error in init\n");
      abort ();
    }
  }
  subcat = catalog[0].missing_catalog;
  if (subcat) {
    if (subcat[0].measure_catalog || subcat[0].secfilt_catalog || subcat[0].missing_catalog) {
      fprintf (stderr, "error in init\n");
      abort ();
    }
  }
  subcat = catalog[0].secfilt_catalog;
  if (subcat) {
    if (subcat[0].measure_catalog || subcat[0].secfilt_catalog || subcat[0].missing_catalog) {
      fprintf (stderr, "error in init\n");
      abort ();
    }
  }
}

DVOCatFormat dvo_catalog_catformat (char *catformat) {
  
  /* set the specified CATFORMAT */
  if (!strcasecmp (catformat, "INTERNAL"))        return (DVO_FORMAT_INTERNAL);
  if (!strcasecmp (catformat, "LONEOS"))    	  return (DVO_FORMAT_LONEOS);
  if (!strcasecmp (catformat, "ELIXIR"))    	  return (DVO_FORMAT_ELIXIR);
  if (!strcasecmp (catformat, "PANSTARRS_DEV_0")) return (DVO_FORMAT_PANSTARRS_DEV_0);
  if (!strcasecmp (catformat, "PANSTARRS_DEV_1")) return (DVO_FORMAT_PANSTARRS_DEV_1);
  if (!strcasecmp (catformat, "PS1_DEV_1"))       return (DVO_FORMAT_PS1_DEV_1);
  if (!strcasecmp (catformat, "PS1_DEV_2"))       return (DVO_FORMAT_PS1_DEV_2);
  if (!strcasecmp (catformat, "PS1_V1"))          return (DVO_FORMAT_PS1_V1);
  if (!strcasecmp (catformat, "PS1_V2"))          return (DVO_FORMAT_PS1_V2);
  if (!strcasecmp (catformat, "PS1_V3"))          return (DVO_FORMAT_PS1_V3);
  if (!strcasecmp (catformat, "PS1_V4"))          return (DVO_FORMAT_PS1_V4);
  if (!strcasecmp (catformat, "PS1_V5"))          return (DVO_FORMAT_PS1_V5);
  if (!strcasecmp (catformat, "PS1_V6"))          return (DVO_FORMAT_PS1_V6);
  if (!strcasecmp (catformat, "PS1_V5_LOAD"))     return (DVO_FORMAT_PS1_V5_LOAD);
  if (!strcasecmp (catformat, "PS1_REF"))         return (DVO_FORMAT_PS1_REF);
  if (!strcasecmp (catformat, "PS1_REF_V2"))      return (DVO_FORMAT_PS1_REF_V2);
  if (!strcasecmp (catformat, "PS1_REF_V3"))      return (DVO_FORMAT_PS1_REF_V3);
  if (!strcasecmp (catformat, "PS1_SIM"))         return (DVO_FORMAT_PS1_SIM);
  return (DVO_FORMAT_UNDEF);
}

DVOCatMode dvo_catalog_catmode (char *catmode) {

  /* set the specified CATMODE */
  if (!strcasecmp (catmode, "RAW"))   return (DVO_MODE_RAW);
  if (!strcasecmp (catmode, "MEF"))   return (DVO_MODE_MEF);
  if (!strcasecmp (catmode, "SPLIT")) return (DVO_MODE_SPLIT);
  return (DVO_MODE_UNDEF);
}

DVOCatCompress dvo_catalog_catcompress (char *catcompress) {

  /* set the specified CATMODE */
  if (!strcasecmp (catcompress, "NONE"))   return (DVO_COMPRESS_NONE);
  if (!strcasecmp (catcompress, "AUTO"))   return (DVO_COMPRESS_AUTO);
  if (!strcasecmp (catcompress, "AUTO_1")) return (DVO_COMPRESS_AUTO_1);
  if (!strcasecmp (catcompress, "AUTO_2")) return (DVO_COMPRESS_AUTO_2);
  if (!strcasecmp (catcompress, "NONE_1")) return (DVO_COMPRESS_NONE_1);
  if (!strcasecmp (catcompress, "NONE_2")) return (DVO_COMPRESS_NONE_2);
  if (!strcasecmp (catcompress, "GZIP_1")) return (DVO_COMPRESS_GZIP_1);
  if (!strcasecmp (catcompress, "GZIP_2")) return (DVO_COMPRESS_GZIP_2);
  if (!strcasecmp (catcompress, "RICE_1")) return (DVO_COMPRESS_RICE_1);
  return (DVO_COMPRESS_NONE);
}

// we can return a static string here unless we run multiple outputs threads at once
char *dvo_catalog_compress_string (DVOCatCompress catcompress) {
  char *compress_string = NULL;
  switch (catcompress) {
    case DVO_COMPRESS_NONE:
      compress_string = strcreate("NONE");
      break;
    case DVO_COMPRESS_AUTO:
      compress_string = strcreate("AUTO");
      break;
    case DVO_COMPRESS_AUTO_1:
      compress_string = strcreate("AUTO_1");
      break;
    case DVO_COMPRESS_AUTO_2:
      compress_string = strcreate("AUTO_2");
      break;
    case DVO_COMPRESS_GZIP_1:
      compress_string = strcreate("GZIP_1");
      break;
    case DVO_COMPRESS_GZIP_2:
      compress_string = strcreate("GZIP_2");
      break;
    case DVO_COMPRESS_NONE_1:
      compress_string = strcreate("NONE_1");
      break;
    case DVO_COMPRESS_NONE_2:
      compress_string = strcreate("NONE_2");
      break;
    case DVO_COMPRESS_RICE_1:
      compress_string = strcreate("RICE_1");
      break;
    default:
      myAbort ("error in compress option");
  }
  return compress_string;
}

float dvoOffsetR (Measure *measure, Average *average) {

  float dR = (measure[0].R - average[0].R) * 3600.0;
  return dR;
}

float dvoOffsetD (Measure *measure, Average *average) {

  float dD = (measure[0].D - average[0].D) * 3600.0;
  return dD;
}

double dvoMeanR (float dR, Average *average) {

  double ra = average[0].R - dR / 3600.0;
  return ra;
}

double dvoMeanD (float dD, Average *average) {

  double dec = average[0].D - dD / 3600.0;
  return dec;
}

// init all data, or just catalog data
void dvo_average_init (Average *average) {
  average->R         	   = 0;
  average->D         	   = 0;
  average->dR        	   = NAN;
  average->dD        	   = NAN;

  average->uR        	   = 0;
  average->uD        	   = 0;
  average->duR       	   = NAN;
  average->duD       	   = NAN;
  average->P         	   = 0;
  average->dP        	   = NAN;

  average->Rstk        	   = NAN;
  average->Dstk        	   = NAN;
  average->dRstk       	   = NAN;
  average->dDstk       	   = NAN;

  average->ChiSqAve  	   = NAN;
  average->ChiSqPM   	   = NAN;
  average->ChiSqPar  	   = NAN;
  average->Tmean   	   = 0;
  average->Trange   	   = 0;

  average->psfQF   	   = NAN;
  average->psfQFperf   	   = NAN;

  average->stargal    	   = 0.0;
  average->Npos    	   = 0;

  average->Nmeasure        = 0;
  average->Nmissing        = 0;
  average->Nlensing        = 0;
  average->Nlensobj        = 0;
  average->Nstarpar        = 0;
  average->Ngalphot        = 0;

  average->measureOffset   = -1;
  average->missingOffset   = -1;
  average->lensingOffset   = -1;
  average->lensobjOffset   = -1;
  average->starparOffset   = -1;
  average->galphotOffset   = -1;

  average->refColorBlue    = NAN;
  average->refColorRed     = NAN;

  average->tessID          = 0;
  average->skycellID       = 0;
  average->projectionID    = 0;

  average->NwarpOK         = 0;

  average->flags           = 0;
  average->photFlagsUpper  = 0;
  average->photFlagsLower  = 0;

  average->objID     	   = 0;
  average->catID     	   = 0;
  average->extID     	   = 0;

  average->uRgal           = NAN;
  average->uDgal           = NAN;
}

// init all data, or just catalog data
void dvo_averageT_init (AverageTiny *average) {
  average->R         	   = 0;
  average->D         	   = 0;
  average->flags           = 0;
  average->Nmeasure        = 0;
  average->measureOffset   = -1;
  average->catID     	   = 0;
  average->nOwn     	   = 0;
}

void dvo_secfilt_init (SecFilt *secfilt, SecFiltInitMode mode) {

  myAssert (mode & SECFILT_RESET_ALL, "at least one of the CHIP,WARP,STACK bits must be set");

  if ((mode & SECFILT_RESET_ALL) == SECFILT_RESET_ALL) {
    secfilt->flags       = 0;
  }

  if (mode & SECFILT_RESET_CHIP) {
    secfilt->MpsfChp     = NAN;
    secfilt->dMpsfChp    = NAN;
    secfilt->sMpsfChp    = NAN;
    secfilt->MapChp      = NAN;
    secfilt->dMapChp     = NAN;
    secfilt->sMapChp     = NAN;
    secfilt->MkronChp    = NAN;
    secfilt->dMkronChp   = NAN;
    secfilt->sMkronChp   = NAN;

    secfilt->psfQfMax     = NAN;
    secfilt->psfQfPerfMax = NAN;

    secfilt->Mmin        = NAN;
    secfilt->Mmax        = NAN;
    secfilt->Mchisq      = NAN;

    secfilt->Ncode       = 0;
    secfilt->Nused       = 0;
    secfilt->NusedKron   = 0;
    secfilt->NusedAp     = 0;
    secfilt->ubercalDist = 1000;
    secfilt->flags      &= ~ID_SECF_CHIP_FLAGS;
  }

  if (mode & SECFILT_RESET_STACK) {
    secfilt->MpsfStk     = NAN;
    secfilt->FpsfStk     = NAN;
    secfilt->dFpsfStk    = NAN;

    secfilt->MkronStk    = NAN;
    secfilt->FkronStk    = NAN;
    secfilt->dFkronStk   = NAN;

    secfilt->MapStk      = NAN;
    secfilt->FapStk      = NAN;
    secfilt->dFapStk     = NAN;

    secfilt->Nstack      = 0;
    secfilt->NstackDet   = 0;

    secfilt->stackPrmryOff = -1;
    secfilt->stackBestOff  = -1;
    secfilt->flags      &= ~ID_SECF_STACK_FLAGS;
  }

  if (mode & SECFILT_RESET_WARP) {
    secfilt->MpsfWrp     = NAN;
    secfilt->FpsfWrp     = NAN;
    secfilt->dFpsfWrp    = NAN;
    secfilt->sFpsfWrp    = NAN;

    secfilt->MkronWrp    = NAN;
    secfilt->FkronWrp    = NAN;
    secfilt->dFkronWrp   = NAN;
    secfilt->sFkronWrp   = NAN;

    secfilt->MapWrp      = NAN;
    secfilt->FapWrp      = NAN;
    secfilt->dFapWrp     = NAN;
    secfilt->sFapWrp     = NAN;

    secfilt->NusedWrp     = 0;
    secfilt->NusedKronWrp = 0;
    secfilt->NusedApWrp   = 0;

    secfilt->Nwarp        = 0;
    secfilt->NwarpGood    = 0;
  }
}

// init all data, or just catalog data
void dvo_measure_init (Measure *measure) {
 measure->R         = NAN;
 measure->D         = NAN;

 measure->M         = NAN;
 measure->dM        = NAN;
 measure->Map       = NAN;
 measure->dMap      = NAN;
 measure->Mkron     = NAN;
 measure->dMkron    = NAN;
 measure->McalPSF   = NAN;
 measure->McalAPER  = NAN;
 measure->dMcal     = NAN;
 measure->dt        = NAN;

 measure->FluxPSF   = NAN;
 measure->dFluxPSF  = NAN;
 measure->FluxKron  = NAN;
 measure->dFluxKron = NAN;
 measure->FluxAp    = NAN;
 measure->dFluxAp   = NAN;

 measure->airmass   = NAN;
 measure->az        = NAN;

 measure->Xccd      = NAN;
 measure->Yccd      = NAN;
 measure->Xfix      = NAN;
 measure->Yfix      = NAN;

 measure->XoffKH    = NAN;
 measure->YoffKH    = NAN;
 measure->XoffDCR   = NAN;
 measure->YoffDCR   = NAN;
 measure->XoffCAM   = NAN;
 measure->YoffCAM   = NAN;

 measure->Mflat     = 0.0;

 measure->Sky       = NAN;
 measure->dSky      = NAN;

 measure->t         = 0;
 measure->averef    = 0;

 measure->detID     = 0;
 measure->objID     = 0;
 measure->catID     = 0;

 measure->extID     = 0;

 measure->imageID   = 0;

 measure->psfQF     = NAN;
 measure->psfQFperf = NAN;
 measure->psfChisq  = NAN;

 measure->psfNdof   = 0;
 measure->psfNpix   = 0;
 measure->extNsigma = NAN;

 measure->FWx       = 0;
 measure->FWy       = 0;
 measure->theta     = 0;

 measure->Mxx       = 0;
 measure->Mxy       = 0;
 measure->Myy       = 0;

 measure->t_msec    = 0;
 measure->photcode  = 0;

 measure->dXccd     = 0;
 measure->dYccd     = 0;
 measure->dRsys     = 0;

 measure->posangle  = 0;
 measure->pltscale  = NAN;

 measure->dbFlags   = 0;
 measure->photFlags = 0;
 measure->photFlags2= 0;
}

void dvo_measureT_init (MeasureTiny *measure) {
 measure->R         = NAN;
 measure->D         = NAN;
 measure->M         = NAN;
 measure->McalPSF   = NAN;
 measure->McalAPER  = NAN;
 measure->dM        = NAN;

 measure->airmass   = NAN;
 measure->Xccd      = NAN;
 measure->Yccd      = NAN;
 measure->Xfix      = NAN;
 measure->Yfix      = NAN;

 measure->t         = 0;
 measure->dt        = NAN;
 measure->psfQF     = NAN;
 measure->averef    = 0;

 measure->imageID   = 0;

 measure->dbFlags   = 0;
 measure->photFlags = 0;
 measure->photcode  = 0;

 measure->catID     = 0;

 measure->dXccd     = 0;
 measure->dYccd     = 0;
 measure->dRsys     = 0;
 measure->myDet     = FALSE;
}

// init all data, or just catalog data
void dvo_lensing_init (Lensing *lensing) {
  lensing->X11_sm_obj = NAN;
  lensing->X12_sm_obj = NAN;
  lensing->X22_sm_obj = NAN;
  lensing->E1_sm_obj  = NAN;
  lensing->E2_sm_obj  = NAN;

  lensing->X11_sh_obj = NAN;
  lensing->X12_sh_obj = NAN;
  lensing->X22_sh_obj = NAN;
  lensing->E1_sh_obj  = NAN;
  lensing->E2_sh_obj  = NAN;

  lensing->X11_sm_psf = NAN;
  lensing->X12_sm_psf = NAN;
  lensing->X22_sm_psf = NAN;
  lensing->E1_sm_psf  = NAN;
  lensing->E2_sm_psf  = NAN;

  lensing->X11_sh_psf = NAN;
  lensing->X12_sh_psf = NAN;
  lensing->X22_sh_psf = NAN;
  lensing->E1_sh_psf  = NAN;
  lensing->E2_sh_psf  = NAN;

  lensing->E1_psf     = NAN;
  lensing->E2_psf     = NAN;

  lensing->F_ApR5     = NAN;
  lensing->dF_ApR5    = NAN;
  lensing->sF_ApR5    = NAN;
  lensing->fF_ApR5    = NAN;

  lensing->F_ApR6     = NAN;
  lensing->dF_ApR6    = NAN;
  lensing->sF_ApR6    = NAN;
  lensing->fF_ApR6    = NAN;

  lensing->F_ApR7     = NAN;
  lensing->dF_ApR7    = NAN;
  lensing->sF_ApR7    = NAN;
  lensing->fF_ApR7    = NAN;

  lensing->detID = -1;
  lensing->objID = -1;
  lensing->catID = -1;
  lensing->averef = 0;

  lensing->imageID = -1;
  lensing->oldImID = -1;
}

// init all data, or just catalog data
void dvo_lensobj_init (Lensobj *lensobj, int toZero) {
  lensobj->X11_sm_obj = toZero ? 0 : NAN;
  lensobj->X12_sm_obj = toZero ? 0 : NAN;
  lensobj->X22_sm_obj = toZero ? 0 : NAN;
  lensobj->E1_sm_obj  = toZero ? 0 : NAN;
  lensobj->E2_sm_obj  = toZero ? 0 : NAN;
  lensobj->X11_sh_obj = toZero ? 0 : NAN;
  lensobj->X12_sh_obj = toZero ? 0 : NAN;
  lensobj->X22_sh_obj = toZero ? 0 : NAN;
  lensobj->E1_sh_obj  = toZero ? 0 : NAN;
  lensobj->E2_sh_obj  = toZero ? 0 : NAN;
  lensobj->X11_sm_psf = toZero ? 0 : NAN;
  lensobj->X12_sm_psf = toZero ? 0 : NAN;
  lensobj->X22_sm_psf = toZero ? 0 : NAN;
  lensobj->E1_sm_psf  = toZero ? 0 : NAN;
  lensobj->E2_sm_psf  = toZero ? 0 : NAN;
  lensobj->X11_sh_psf = toZero ? 0 : NAN;
  lensobj->X12_sh_psf = toZero ? 0 : NAN;
  lensobj->X22_sh_psf = toZero ? 0 : NAN;
  lensobj->E1_sh_psf  = toZero ? 0 : NAN;
  lensobj->E2_sh_psf  = toZero ? 0 : NAN;
  lensobj->F_ApR5     = toZero ? 0 : NAN;
  lensobj->dF_ApR5    = toZero ? 0 : NAN;
  lensobj->sF_ApR5    = toZero ? 0 : NAN;
  lensobj->fF_ApR5    = toZero ? 0 : NAN;
  lensobj->F_ApR6     = toZero ? 0 : NAN;
  lensobj->dF_ApR6    = toZero ? 0 : NAN;
  lensobj->sF_ApR6    = toZero ? 0 : NAN;
  lensobj->fF_ApR6    = toZero ? 0 : NAN;
  lensobj->F_ApR7     = toZero ? 0 : NAN;
  lensobj->dF_ApR7    = toZero ? 0 : NAN;
  lensobj->sF_ApR7    = toZero ? 0 : NAN;
  lensobj->fF_ApR7    = toZero ? 0 : NAN;

  lensobj->gamma = NAN;
  lensobj->E1 = NAN;
  lensobj->E2 = NAN;

  lensobj->objID = -1;
  lensobj->catID = -1;

  lensobj->photcode = 0;
  lensobj->Nmeas = 0;
}

// init all data, or just catalog data
void dvo_starpar_init (StarPar *starpar) {
  starpar->R        = NAN;
  starpar->D        = NAN;
  starpar->galLat   = NAN;
  starpar->galLon   = NAN;
  starpar->Ebv      = NAN;
  starpar->dEbv     = NAN;
  starpar->DistMag  = NAN;
  starpar->dDistMag = NAN;
  starpar->M_r      = NAN;
  starpar->dM_r     = NAN;
  starpar->FeH      = NAN;
  starpar->dFeH     = NAN;
  starpar->uRA      = NAN;
  starpar->uDEC     = NAN;
  starpar->averef   = -1;
  starpar->objID    = -1;
  starpar->catID    = -1;
}

// init all data, or just catalog data
void dvo_galphot_init (GalPhot *galphot) {
  galphot->Xfit         = NAN;
  galphot->Yfit         = NAN;
  galphot->mag          = NAN;
  galphot->magErr       = NAN;
  galphot->majorAxis    = NAN;
  galphot->minorAxis    = NAN;
  galphot->majorAxisErr = NAN;
  galphot->minorAxisErr = NAN;
  galphot->theta        = NAN;
  galphot->thetaErr     = NAN;
  galphot->index        = NAN;
  galphot->chisq        = NAN;
  galphot->Npix         = 0;

  galphot->detID   = -1;
  galphot->objID   = -1;
  galphot->catID   = -1;
  galphot->imageID = -1;
  galphot->averef  = 0;
  galphot->flags   = 0;

  galphot->photcode = 0;
  galphot->modelType = 0;
}

# define INIT_TABLE(TABLE)			\
  catalog[0].TABLE = NULL;			\
  catalog[0].N##TABLE = 0;			\
  catalog[0].N##TABLE##_disk = 0;		\
  catalog[0].N##TABLE##_off  = 0;		\
  catalog[0].TABLE##_catalog  = NULL;

// init all data, or just catalog data
void dvo_catalog_init (Catalog *catalog, int complete) {

  // the following are used to guide open/create/load
  if (complete) {
    catalog[0].f = NULL;
    catalog[0].filename = NULL;

    catalog[0].lockmode    = 0;
    catalog[0].catmode     = DVO_MODE_UNDEF;
    catalog[0].catformat   = DVO_FORMAT_UNDEF;
    catalog[0].catcompress = DVO_COMPRESS_NONE;
    catalog[0].catflags    = DVO_LOAD_NONE;
    catalog[0].Nsecfilt    = 0;
  }

  gfits_init_header (&catalog[0].header);
  
  // the following describe the catalog files on disk
  catalog[0].average = NULL;
  catalog[0].Naverage = 0;
  catalog[0].Naverage_disk = 0;
  catalog[0].Naverage_off  = 0;
  // average lacks average_catalog

  catalog[0].secfilt = NULL;
  catalog[0].Nsecfilt_mem = 0;
  catalog[0].Nsecfilt_disk = 0;
  catalog[0].Nsecfilt_off  = 0;
  catalog[0].secfilt_catalog = NULL;
  // secfilt uses Nsecfilt_mem not Nsecfilt

  catalog[0].averageT = NULL;
  catalog[0].measureT = NULL; 

  INIT_TABLE(measure);
  INIT_TABLE(missing);
  INIT_TABLE(lensing);
  INIT_TABLE(lensobj);
  INIT_TABLE(starpar);
  INIT_TABLE(galphot);

  catalog[0].objID = 0;
  catalog[0].catID = 0;
  catalog[0].sorted = 0;

  /* pointers for data manipulation */
  catalog[0].nOwn_t = NULL;
  catalog[0].found_t = NULL;
  catalog[0].foundWarp_t = NULL;
  catalog[0].measureRank = NULL;
}

/* possible exit status for lock_catalog: 
   DVO_CAT_OPEN_FAIL - failure (including lock failure)
   DVO_CAT_OPEN_OK - success
   DVO_CAT_OPEN_EMPTY - empty file (file may be open or closed!) 
*/
int dvo_catalog_lock (Catalog *catalog, int lockmode) {

  int dbstate;

  /* record lockmode choice */
  catalog[0].lockmode = lockmode;

  /* set lock on database, create stream f */
  // fprintf (stderr, "locking: %s\n", catalog[0].filename);
  catalog[0].f = fsetlockfile (catalog[0].filename, 3600.0, catalog[0].lockmode, &dbstate);

  if (dbstate == LCK_MISSING) return (DVO_CAT_OPEN_EMPTY);
  if (dbstate == LCK_EMPTY)   return (DVO_CAT_OPEN_EMPTY);
  if (catalog[0].f == NULL)   return (DVO_CAT_OPEN_FAIL);

  if (fseeko (catalog[0].f, 0, SEEK_SET)) {
    perror ("fseeko: ");
    exit (1);
  }
  return (DVO_CAT_OPEN_OK);
}

int dvo_catalog_unlock (Catalog *catalog) {

  int fd, dbstate;
  struct stat filestat;

  if (catalog[0].f == (FILE *) NULL) return (2);

  if (fflush (catalog[0].f)) {
    perror ("fflush: ");
    fprintf (stderr, "failed to flush file %s\n", catalog[0].filename);
    return FALSE;
  }

  // fprintf (stderr, "unlocking: %s\n", catalog[0].filename);

  // attempt to unlink an empty file
  fd = fileno (catalog[0].f);
  if (!fstat (fd, &filestat)) {
    if (filestat.st_size == 0) {
      unlink (catalog[0].filename);
    }
  }

  // closes f but does not set back to NULL
  if (!fclearlockfile (catalog[0].filename, catalog[0].f, catalog[0].lockmode, &dbstate)) {
    fprintf (stderr, "failed to unlock or close file\n");
    return (0);
  }

  if (catalog[0].catmode == DVO_MODE_SPLIT) {
    if (catalog[0].measure_catalog) { if (!dvo_catalog_unlock (catalog[0].measure_catalog)) { fprintf (stderr, "failed to unlock measures\n"); return (0); }}
    if (catalog[0].missing_catalog) { if (!dvo_catalog_unlock (catalog[0].missing_catalog)) { fprintf (stderr, "failed to unlock missing\n"); return (0); }}
    if (catalog[0].secfilt_catalog) { if (!dvo_catalog_unlock (catalog[0].secfilt_catalog)) { fprintf (stderr, "failed to unlock secfilt\n"); return (0); }}
    if (catalog[0].lensing_catalog) { if (!dvo_catalog_unlock (catalog[0].lensing_catalog)) { fprintf (stderr, "failed to unlock lensing\n"); return (0); }}
    if (catalog[0].lensobj_catalog) { if (!dvo_catalog_unlock (catalog[0].lensobj_catalog)) { fprintf (stderr, "failed to unlock lensobj\n"); return (0); }}
    if (catalog[0].starpar_catalog) { if (!dvo_catalog_unlock (catalog[0].starpar_catalog)) { fprintf (stderr, "failed to unlock starpar\n"); return (0); }}
    if (catalog[0].galphot_catalog) { if (!dvo_catalog_unlock (catalog[0].galphot_catalog)) { fprintf (stderr, "failed to unlock galphot\n"); return (0); }}
  }
  return (1);
}

// open an existing catalog file or or create a new one as needed (iomode == "w")
// returns FALSE if a catalog could not be opened
// returns TRUE if the catalog was empty
// XXX allow stdout as destination (don't lock)
enum {DVO_OPEN_NONE, DVO_OPEN_READ, DVO_OPEN_WRITE, DVO_OPEN_UPDATE};
int dvo_catalog_open (Catalog *catalog, SkyRegion *region, int VERBOSE, char *iomode) {

  int Nsecfilt, mode;
  int BACKUP, READWRITE;

  mode = DVO_OPEN_NONE;
  if (!strcasecmp (iomode, "r")) mode = DVO_OPEN_READ;
  if (!strcasecmp (iomode, "w")) mode = DVO_OPEN_WRITE;
  if (!strcasecmp (iomode, "a")) mode = DVO_OPEN_UPDATE;
  if (mode == DVO_OPEN_NONE) return (FALSE);

  // save the expected Nsecfilt value for testing
  Nsecfilt = catalog[0].Nsecfilt;

  dvo_catalog_init (catalog, FALSE);

  // default access control options:
  catalog[0].lockmode  = LCK_XCLD;
  BACKUP = TRUE;
  READWRITE = TRUE;

  // in read-only mode, do not backup or require write access
  if (mode == DVO_OPEN_READ) {
    catalog[0].lockmode  = LCK_SOFT;
    BACKUP = FALSE;
    READWRITE = FALSE;
  }
  
  // NOTE: this only check if we can write a backup; it does not actually write the backup
  if (!check_file_access (catalog[0].filename, BACKUP, READWRITE, VERBOSE)) {
    fprintf (stderr, "no permission to access %s\n", catalog[0].filename);
    return (FALSE);
  }

  if (VERBOSE) fprintf (stderr, "opening %s\n", catalog[0].filename);
  
  switch (dvo_catalog_lock (catalog, catalog[0].lockmode)) {
  case DVO_CAT_OPEN_FAIL:
    fprintf (stderr, "can't lock file %s\n", catalog[0].filename);
    return (FALSE);
  case DVO_CAT_OPEN_OK:
    if (!dvo_catalog_load (catalog, VERBOSE)) {
      fprintf (stderr, "failure loading catalog\n");
      return (FALSE);
    }
    if (!dvo_catalog_check (catalog, Nsecfilt, TRUE)) {
      fprintf (stderr, "can't reduce number of secondary filters\n");
      return (FALSE);
    }
    if (VERBOSE) fprintf (stderr, "loaded existing file %s\n", catalog[0].filename);
    break;
  case DVO_CAT_OPEN_EMPTY:
    if ((mode == DVO_OPEN_READ) || (mode == DVO_OPEN_UPDATE)) return (TRUE);
    catalog[0].Nsecfilt = Nsecfilt;
    dvo_catalog_create (region, catalog); /* fills in new header info */
    if (VERBOSE) fprintf (stderr, "creating new file %s\n", catalog[0].filename);
    break;
  }
  return (TRUE);
}

// load an existing dvo_catalog currenly, the catmode information is carried per
// catalog.  the user-set value of catmode will set the output mode for new
// tables an old table will keep its mode.
int dvo_catalog_load (Catalog *catalog, int VERBOSE) {
  
  int Naxis, split, status;
  char measure[80];

  dvo_catalog_init (catalog, FALSE);

  // reset the file pointer in case we've already read some data
  fseeko (catalog[0].f, 0, SEEK_SET);

  // load the main catalog header, determine characteristics
  if (!gfits_fread_header (catalog[0].f, &catalog[0].header)) {
    if (VERBOSE) fprintf (stderr, "failure to read main catalog header\n");
    return (FALSE);
  }
  // check if the table has been sorted or not 
  status = gfits_scan_alt (&catalog[0].header, "SORTED", "%t", 1, &catalog[0].sorted);
  if (!status) catalog[0].sorted = TRUE;

  // determine catmode
  if (!gfits_scan (&catalog[0].header, "NAXIS", "%d", 1, &Naxis)) {
    if (VERBOSE) fprintf (stderr, "can't determine catalog db mode\n");
    return (FALSE);
  }
  catalog[0].catmode = DVO_MODE_MEF;
  split = gfits_scan (&catalog[0].header, "MEASURE", "%s", 1, measure);
  if (split) catalog[0].catmode = DVO_MODE_SPLIT;
  if (Naxis == 2) catalog[0].catmode = DVO_MODE_RAW;
  /* don't use Naxis == 2 and split mode! */

  switch (catalog[0].catmode) {
    case DVO_MODE_RAW:
      if (VERBOSE) fprintf (stderr, "reading catalog (mode DVO_MODE_RAW)\n");
      if (!dvo_catalog_load_raw (catalog, VERBOSE)) {
	if (VERBOSE) fprintf (stderr, "failed to load RAW catalog file %s\n", catalog[0].filename);
	return (FALSE);
      }
      break;
    case DVO_MODE_MEF:
      if (VERBOSE) fprintf (stderr, "reading catalog (mode DVO_MODE_MEF)\n");
      if (!dvo_catalog_load_mef (catalog, VERBOSE)) {
	if (VERBOSE) fprintf (stderr, "failed to load MEF catalog file %s\n", catalog[0].filename);
	return (FALSE);
      }
      break;
    case DVO_MODE_SPLIT:
      if (VERBOSE) fprintf (stderr, "reading catalog (mode DVO_MODE_SPLIT)\n");
      if (!dvo_catalog_load_split (catalog, VERBOSE)) {
	if (VERBOSE) fprintf (stderr, "failed to load SPLIT catalog file %s\n", catalog[0].filename);
	return (FALSE);
      }
      break;
    default:
      fprintf (stderr, "error getting catalog mode\n");
      exit (2);
  }
  return (TRUE);
}

// write out the data, unlink if empty?  'save' means: write out all data currently in
// memory.  NOTE: this is currently not always possible: for non-SPLIT mode files, this
// operation may require expanding the file size, which does not automatically happen
int dvo_catalog_save (Catalog *catalog, char VERBOSE) {

  int status = FALSE;

  // set the 'sorted' header keyword
  gfits_modify_alt (&catalog[0].header, "SORTED",  "%t", 1, catalog[0].sorted);

  // XXX handle return status
  switch (catalog[0].catmode) {
    case DVO_MODE_RAW:
      status = dvo_catalog_save_raw (catalog, VERBOSE);
      break;
    case DVO_MODE_MEF:
      status = dvo_catalog_save_mef (catalog, VERBOSE);
      break;
    case DVO_MODE_SPLIT:
      status = dvo_catalog_save_split (catalog, VERBOSE);
      break;
    default:
      fprintf (stderr, "invalid catalog mode\n");
      exit (2);
  }
  return (status);
}

// write out the data, assuming the in-memory data represents all data for the table.
// NOTE: This operation may truncate the file and delete entries.
int dvo_catalog_save_complete (Catalog *catalog, char VERBOSE) {

  // set the 'sorted' header keyword
  gfits_modify_alt (&catalog[0].header, "SORTED",  "%t", 1, catalog[0].sorted);

  // XXX handle return status
  switch (catalog[0].catmode) {
    case DVO_MODE_RAW:
      dvo_catalog_save_raw (catalog, VERBOSE);
      break;
    case DVO_MODE_MEF:
      dvo_catalog_save_mef (catalog, VERBOSE);
      break;
    case DVO_MODE_SPLIT:
      dvo_catalog_save_split_complete (catalog, VERBOSE);
      break;
    default:
      fprintf (stderr, "invalid catalog mode\n");
      exit (2);
  }
  return (TRUE);
}

// write out the in-memory data which extends beyond the data on disk.  NOTE: this is
// currently only possible for the SPLIT mode.
int dvo_catalog_update (Catalog *catalog, char VERBOSE) {

  // set the 'sorted' header keyword
  catalog[0].sorted = FALSE;
  gfits_modify_alt (&catalog[0].header, "SORTED",  "%t", 1, catalog[0].sorted);

  /* update is only valid for catmode SPLIT */
  switch (catalog[0].catmode) {
    case DVO_MODE_RAW:
      fprintf (stderr, "not allowed for RAW mode\n");
      break;
    case DVO_MODE_MEF:
      fprintf (stderr, "not allowed for MEF mode\n");
      break;
    case DVO_MODE_SPLIT:
      dvo_catalog_update_split (catalog, VERBOSE);
      break;
    default:
      fprintf (stderr, "failure updating catalog\n");
      fprintf (stderr, "invalid catalog mode\n");
      exit (2);
  }
  return (TRUE);
}

int dvo_catalog_check (Catalog *catalog, int Nsecfilt, int extend) {

  off_t in, out, i;
  int j, Nextra;
  SecFilt *insec, *outsec;

  if (Nsecfilt == 0) return (TRUE); // if Nsecfilt is not set, don't do this check
  if (catalog[0].Nsecfilt == Nsecfilt) return (TRUE);
  if (catalog[0].Nsecfilt > Nsecfilt) return (FALSE);
  if ((catalog[0].Nsecfilt < Nsecfilt) && !extend) return (FALSE);

  if ((catalog[0].Nsecfilt < Nsecfilt) && extend) {
    Nextra = Nsecfilt - catalog[0].Nsecfilt;
    insec = catalog[0].secfilt;
    ALLOCATE (outsec, SecFilt, catalog[0].Naverage * Nsecfilt);
    for (in = out = i = 0; i < catalog[0].Naverage; i++) {
      for (j = 0; j < catalog[0].Nsecfilt; j++, in++, out++) {
	outsec[out].MpsfChp  = insec[in].MpsfChp;
	outsec[out].dMpsfChp = insec[in].dMpsfChp;
	outsec[out].Mchisq   = insec[in].Mchisq;
      }
      for (j = 0; j < Nextra; j++, out++) {
	outsec[out].MpsfChp  = NAN;
	outsec[out].dMpsfChp = NAN;
	outsec[out].Mchisq   = NAN;
      }
    }
    free (catalog[0].secfilt);
    catalog[0].secfilt = outsec;
    catalog[0].Nsecfilt = Nsecfilt;
    catalog[0].Nsecfilt_mem = Nsecfilt * catalog[0].Naverage;
  }
  return (TRUE);
}

void dvo_catalog_free (Catalog *catalog) {

  if (!catalog) return;

  if (catalog[0].catmode == DVO_MODE_SPLIT) {

    if (catalog[0].measure_catalog) {
      free (catalog[0].measure_catalog[0].filename);
      dvo_catalog_free (catalog[0].measure_catalog);
      free (catalog[0].measure_catalog);
    }
    if (catalog[0].missing_catalog) {
      free (catalog[0].missing_catalog[0].filename);
      dvo_catalog_free (catalog[0].missing_catalog);
      free (catalog[0].missing_catalog);
    }
    if (catalog[0].secfilt_catalog) {
      free (catalog[0].secfilt_catalog[0].filename);
      dvo_catalog_free (catalog[0].secfilt_catalog);
      free (catalog[0].secfilt_catalog);
    }
    if (catalog[0].lensing_catalog) {
      free (catalog[0].lensing_catalog[0].filename);
      dvo_catalog_free (catalog[0].lensing_catalog);
      free (catalog[0].lensing_catalog);
    }
    if (catalog[0].lensobj_catalog) {
      free (catalog[0].lensobj_catalog[0].filename);
      dvo_catalog_free (catalog[0].lensobj_catalog);
      free (catalog[0].lensobj_catalog);
    }
    if (catalog[0].starpar_catalog) {
      free (catalog[0].starpar_catalog[0].filename);
      dvo_catalog_free (catalog[0].starpar_catalog);
      free (catalog[0].starpar_catalog);
    }
    if (catalog[0].galphot_catalog) {
      free (catalog[0].galphot_catalog[0].filename);
      dvo_catalog_free (catalog[0].galphot_catalog);
      free (catalog[0].galphot_catalog);
    }
  }
  dvo_catalog_free_data (catalog);

  gfits_free_header (&catalog[0].header);
}

void dvo_catalog_free_data (Catalog *catalog) {

  /* free, initialize data structures */
  if (catalog[0].average) {
    free (catalog[0].average); 
    catalog[0].Naverage = 0;
    catalog[0].average = NULL;
  }
  if (catalog[0].measure) {
    free (catalog[0].measure); 
    catalog[0].Nmeasure = 0;
    catalog[0].measure = NULL;
  }
  if (catalog[0].measureT) {
    free (catalog[0].measureT); 
  }
  if (catalog[0].missing) {
    free (catalog[0].missing); 
    catalog[0].Nmissing = 0;
    catalog[0].missing = NULL;
  }
  if (catalog[0].secfilt) {
    free (catalog[0].secfilt); 
    catalog[0].Nsecfilt_mem = 0;
    catalog[0].secfilt = NULL;
  }
  if (catalog[0].lensing) {
    free (catalog[0].lensing); 
    catalog[0].Nlensing = 0;
    catalog[0].lensing = NULL;
  }
  if (catalog[0].lensobj) {
    free (catalog[0].lensobj); 
    catalog[0].Nlensobj = 0;
    catalog[0].lensobj = NULL;
  }
  if (catalog[0].starpar) {
    free (catalog[0].starpar); 
    catalog[0].Nstarpar = 0;
    catalog[0].starpar = NULL;
  }
  if (catalog[0].galphot) {
    free (catalog[0].galphot); 
    catalog[0].Ngalphot = 0;
    catalog[0].galphot = NULL;
  }
  if (catalog[0].nOwn_t)      { free (catalog[0].nOwn_t); catalog[0].nOwn_t = NULL; }
  if (catalog[0].found_t)     { free (catalog[0].found_t); catalog[0].found_t = NULL; }
  if (catalog[0].foundWarp_t) { free (catalog[0].foundWarp_t); catalog[0].foundWarp_t = NULL; }
  if (catalog[0].measureRank) { free (catalog[0].measureRank); catalog[0].measureRank = NULL; }
}

/*
  mode   : items to read (DVO_LOAD_AVERAGE | DVO_LOAD_MEASURE | DVO_LOAD_MISSING | DVO_LOAD_SECFILT)
  format : what table structure on disk (INTERNAL, LONEOS, etc, )
  style  : raw, mef, split, mysql
*/

int dvo_catalog_load_segment (Catalog *catalog, int VERBOSE, off_t start, off_t Nrows) {

  switch (catalog[0].catmode) {
    case DVO_MODE_RAW:
      if (VERBOSE) fprintf (stderr, "reading catalog (mode DVO_MODE_RAW)\n");
      fprintf (stderr, "cannot do this in raw mode\n");
      // dvo_catalog_load_segment_raw (catalog, VERBOSE, start, Nrows);
      break;
    case DVO_MODE_MEF:
      if (VERBOSE) fprintf (stderr, "reading catalog (mode DVO_MODE_MEF)\n");
      fprintf (stderr, "cannot do this in mef mode\n");
      // dvo_catalog_load_segment_mef (catalog, VERBOSE, start, Nrows);
      break;
    case DVO_MODE_SPLIT:
      if (VERBOSE) fprintf (stderr, "reading catalog (mode DVO_MODE_SPLIT)\n");
      dvo_catalog_load_segment_split (catalog, VERBOSE, start, Nrows);
      break;
    default:
      fprintf (stderr, "error getting catalog mode\n");
      exit (2);
  }
  return (TRUE);
}

// make a backup of this catalog (including the measure, secfilt, and missing tables as needed)
int dvo_catalog_backup (Catalog *catalog, char *suffix, int primary) {

  // skip empty cpt files
  if (primary && !catalog->Naverage_disk) {
    return TRUE;
  }

  // if we do not have a filename, we did not open the file
  if (!catalog->filename && !catalog->f) {
    return TRUE;
  }

  char tmpfilename[DVO_MAX_PATH];
  int status = snprintf (tmpfilename, DVO_MAX_PATH, "%s%s", catalog->filename, suffix);
  if (status >= DVO_MAX_PATH) {
    fprintf (stderr, "path name too long: %s\n", catalog->filename);
    return FALSE;
  }
      
  // unlock the catalog here (closes file as well) (also closes subcat files Measure, Secfilt, Missing)
  if (primary) {
    status = dvo_catalog_unlock (catalog);
    if (!status) {
      fprintf (stderr, "failed to unlock catalog %s\n", catalog->filename);
      return FALSE;
    }
  }

  // play it safe: do not overwrite an existing backup file
  struct stat fileStats;
  status = stat (tmpfilename, &fileStats);
  if (!status) {
    fprintf (stderr, "ERROR: backup file %s already exists, exiting\n", tmpfilename);
    return FALSE;
  }
  
  // some error accessing the file.  there is only one acceptable error: file not found
  if (status && (errno != ENOENT)) {
    perror ("problem with output target");
    return FALSE;
  }

  status = rename (catalog->filename, tmpfilename);
  if (status) {
    fprintf (stderr, "failed to rename catalog %s\n", catalog->filename);
    return FALSE;
  }
      
  // keep the same lockmode
  int lockmode = catalog->lockmode;

  // lock the new catalog file here (re-opens file as well) (does NOT open/lock the subcat files)
  status = dvo_catalog_lock (catalog, lockmode);
  if (!status) {
    fprintf (stderr, "failed to lock new catalog file %s\n", catalog->filename);
    return FALSE;
  }

  if (primary && (catalog[0].catmode == DVO_MODE_SPLIT)) {
    if (catalog[0].measure_catalog != NULL) {
      if (!dvo_catalog_backup (catalog[0].measure_catalog, suffix, FALSE)) {
	return FALSE;
      }
    }
    if (catalog[0].missing_catalog != NULL) {
      if (!dvo_catalog_backup (catalog[0].missing_catalog, suffix, FALSE)) {
	return FALSE;
      }
    }
    if (catalog[0].secfilt_catalog != NULL) {
      if (!dvo_catalog_backup (catalog[0].secfilt_catalog, suffix, FALSE)) {
	return FALSE;
      }
    }
    if (catalog[0].lensing_catalog != NULL) {
      if (catalog[0].Nlensing_disk == 0) {
	// need to relock (and re-open) file for close elsewhere
	status = dvo_catalog_lock (catalog[0].lensing_catalog, lockmode);
      } else {
	if (!dvo_catalog_backup (catalog[0].lensing_catalog, suffix, FALSE)) {
	  return FALSE;
	}
      }
    }
    if (catalog[0].lensobj_catalog != NULL) {
      if (catalog[0].Nlensobj_disk == 0) {
	// need to relock (and re-open) file for close elsewhere
	status = dvo_catalog_lock (catalog[0].lensobj_catalog, lockmode);
      } else {
	if (!dvo_catalog_backup (catalog[0].lensobj_catalog, suffix, FALSE)) {
	  return FALSE;
	}
      }
    }
    if (catalog[0].starpar_catalog != NULL) {
      if (catalog[0].Nstarpar_disk == 0) {
	// need to relock (and re-open) file for close elsewhere
	status = dvo_catalog_lock (catalog[0].starpar_catalog, lockmode);
      } else {
	if (!dvo_catalog_backup (catalog[0].starpar_catalog, suffix, FALSE)) {
	  return FALSE;
	}
      }
    }
    if (catalog[0].galphot_catalog != NULL) {
      if (catalog[0].Ngalphot_disk == 0) {
	// need to relock (and re-open) file for close elsewhere
	status = dvo_catalog_lock (catalog[0].galphot_catalog, lockmode);
      } else {
	if (!dvo_catalog_backup (catalog[0].galphot_catalog, suffix, FALSE)) {
	  return FALSE;
	}
      }
    }
  }
  return TRUE;
}

// make a backup of this catalog (including the measure, secfilt, and missing tables as needed)
int dvo_catalog_unlink_backup (Catalog *catalog, char *suffix, int primary) {

  if (primary && !catalog->Naverage_disk) {
    // skip empty files (empty when read, but output may not be empty)
    return TRUE;
  }

  // if we do not have a filename, we did not open the file
  if (!catalog->filename && !catalog->f) {
    return TRUE;
  }

  char tmpfilename[DVO_MAX_PATH];
  int status = snprintf (tmpfilename, DVO_MAX_PATH, "%s%s", catalog->filename, suffix);
  if (status >= DVO_MAX_PATH) {
    fprintf (stderr, "path name too long: %s\n", catalog->filename);
    return FALSE;
  }
  
  int outStatus = TRUE;
  status = unlink (tmpfilename);
  if (status) {
    perror ("unlink: ");
    fprintf (stderr, "failed to unlink catalog %s~\n", catalog->filename);
    outStatus = FALSE;
  }
      
  if (catalog[0].catmode == DVO_MODE_SPLIT) {
    if (catalog[0].measure_catalog != NULL) {
      if (!dvo_catalog_unlink_backup (catalog[0].measure_catalog, suffix, FALSE)) {
	outStatus = FALSE;
      }
    }
    if (catalog[0].missing_catalog != NULL) {
      if (!dvo_catalog_unlink_backup (catalog[0].missing_catalog, suffix, FALSE)) {
	outStatus = FALSE;
      }
    }
    if (catalog[0].secfilt_catalog != NULL) {
      if (!dvo_catalog_unlink_backup (catalog[0].secfilt_catalog, suffix, FALSE)) {
	outStatus = FALSE;
      }
    }
    if ((catalog[0].lensing_catalog != NULL) && (catalog[0].Nlensing_disk > 0)) {
      if (!dvo_catalog_unlink_backup (catalog[0].lensing_catalog, suffix, FALSE)) {
	outStatus = FALSE;
      }
    }
    if ((catalog[0].lensobj_catalog != NULL) && (catalog[0].Nlensobj_disk > 0)) {
      if (!dvo_catalog_unlink_backup (catalog[0].lensobj_catalog, suffix, FALSE)) {
	outStatus = FALSE;
      }
    }
    if ((catalog[0].starpar_catalog != NULL) && (catalog[0].Nstarpar_disk > 0)) {
      if (!dvo_catalog_unlink_backup (catalog[0].starpar_catalog, suffix, FALSE)) {
	outStatus = FALSE;
      }
    }
    if ((catalog[0].galphot_catalog != NULL) && (catalog[0].Ngalphot_disk > 0)) {
      if (!dvo_catalog_unlink_backup (catalog[0].galphot_catalog, suffix, FALSE)) {
	outStatus = FALSE;
      }
    }
  }
  return outStatus;
}


