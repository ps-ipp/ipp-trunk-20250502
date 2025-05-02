# include "addstar.h"

// this is the complete list of FITS format input files the addstar knows 
// (excluding SDSS data and reference database info, such as 2MASS)
// NOTE: these must also be listed in MatchHeaders.c (line ~ 62)
Catalog *Convert_SMPDATA           PROTO((FTable *table));
Catalog *Convert_PS1_DEV_0         PROTO((FTable *table));
Catalog *Convert_PS1_DEV_1         PROTO((FTable *table));
Catalog *Convert_PS1_V1            PROTO((FTable *table));
Catalog *Convert_PS1_V1_Alt        PROTO((FTable *table));
Catalog *Convert_PS1_V2            PROTO((FTable *table));
Catalog *Convert_PS1_V3            PROTO((FTable *table));
Catalog *Convert_PS1_V4            PROTO((FTable *table));
Catalog *Convert_PS1_V5            PROTO((FTable *table));
Catalog *Convert_PS1_V5_R0         PROTO((FTable *table));
Catalog *Convert_PS1_V5_R0_Lensing PROTO((FTable *table));
Catalog *Convert_PS1_V5_R1_Lensing PROTO((FTable *table));
Catalog *Convert_PS1_V5_R2_Lensing PROTO((FTable *table));
Catalog *Convert_PS1_SV1           PROTO((FTable *table));
Catalog *Convert_PS1_SV1_Alt       PROTO((FTable *table));
Catalog *Convert_PS1_SV2           PROTO((FTable *table));
Catalog *Convert_PS1_SV3           PROTO((FTable *table));
Catalog *Convert_PS1_SV4           PROTO((FTable *table));
Catalog *Convert_PS1_DV3           PROTO((FTable *table));
Catalog *Convert_PS1_DV4           PROTO((FTable *table));
Catalog *Convert_PS1_DV5           PROTO((FTable *table));

Catalog *addstar_catalog_init (int Nstars) {
  Catalog *catalog = NULL;
  ALLOCATE (catalog, Catalog, 1);
  dvo_catalog_init (catalog, TRUE);

  ALLOCATE (catalog->measure, Measure, Nstars);

  int i;
  for (i = 0; i < Nstars; i++) {
    dvo_measure_init (&catalog->measure[i]);
  }
  catalog->Nmeasure = Nstars;
  return catalog;
}

// given a file with the pointer at the start of the table block and the 
// corresponding image header, load the stars from the table
Catalog *ReadStarsFITS (FILE *f, Header *header, Header *in_theader) {
  OHANA_UNUSED_PARAM(header);

  off_t Nskip;
  char type[80];
  Header theader;
  FTable table;
  
  if (in_theader == NULL) {
    table.header = &theader;
    if (!gfits_fread_header (f, table.header)) Shutdown ("ERROR: can't read table header");
  } else {
    table.header = in_theader;
    Nskip = in_theader[0].datasize;
    fseeko (f, Nskip, SEEK_CUR); 
  }

  /* load the table data */
  if (!gfits_fread_ftable_data (f, &table, FALSE)) {
    fprintf (stderr, "ERROR: can't read table header\n");
    exit (1);
  }

  if (!gfits_scan (table.header, "EXTTYPE", "%s", 1, type)) {
    strcpy (type, "SMPDATA");
  }

  Catalog *catalog = NULL;
  if (!strcmp (type, "SMPDATA")) {
    catalog = Convert_SMPDATA (&table);
  }
  if (!strcmp (type, "PS1_DEV_0")) {
    catalog = Convert_PS1_DEV_0 (&table);
  }
  if (!strcmp (type, "PS1_DEV_1")) {
    catalog = Convert_PS1_DEV_1 (&table);
  }
  if (!strcmp (type, "PS1_V1")) {
    catalog = Convert_PS1_V1 (&table);
  }
  if (!strcmp (type, "PS1_V2")) {
    if (table.header[0].Naxis[0] == 20) {
      // skip the invalid DETEFF tables which were mistakenly labeled as PS1_V2
      return (NULL);
    }
    catalog = Convert_PS1_V2 (&table);
  }
  if (!strcmp (type, "PS1_V3")) {
    catalog = Convert_PS1_V3 (&table);
  }
  if (!strcmp (type, "PS1_V4")) {
    catalog = Convert_PS1_V4 (&table);
  }
  if (!strcmp (type, "PS1_V5")) {
    switch (table.header[0].Naxis[0]) {
      case 232:
        catalog = Convert_PS1_V5 (&table);
        break;
      case 288:
        catalog = Convert_PS1_V5_R0 (&table);
        break;
      case 312:
        catalog = Convert_PS1_V5_R0_Lensing (&table);
        break;
      case 320:
        catalog = Convert_PS1_V5_R1_Lensing (&table);
        break;
      case 328:
        catalog = Convert_PS1_V5_R2_Lensing (&table);
        break;
      default:
        fprintf (stderr, "invalid PS1_V5 table size %d\n", (int) table.header[0].Naxis[0]);
        return NULL;
    }
  }
  if (!strcmp (type, "PS1_SV1")) {
    catalog = Convert_PS1_SV1 (&table);
  }
  if (!strcmp (type, "PS1_SV2")) {
    catalog = Convert_PS1_SV2 (&table);
  }
  if (!strcmp (type, "PS1_SV3")) {
    catalog = Convert_PS1_SV3 (&table);
  }
  if (!strcmp (type, "PS1_SV4")) {
    catalog = Convert_PS1_SV4 (&table);
  }
  if (!strcmp (type, "PS1_DV3")) {
    catalog = Convert_PS1_DV3 (&table);
  }
  if (!strcmp (type, "PS1_DV4")) {
    catalog = Convert_PS1_DV4 (&table);
  }
  if (!strcmp (type, "PS1_DV5")) {
    catalog = Convert_PS1_DV5 (&table);
  }
  if (catalog == NULL) {
    fprintf (stderr, "invalid table type %s\n", type);
    return (NULL);
  }

  gfits_free_table (&table);
  return catalog;
}

float GetFluxFromFluxOrMag (float flux, float mag) {

  if (isnan(mag) && isnan(flux)) return NAN;

  if (isnan(flux)) return pow(10.0,-0.4*mag);
  return flux;
}

float GetFluxErrFromFluxOrMag (float dFlux, float flux, float dMag) {

  if (isnan(dMag) && isnan(dFlux)) return NAN;
  if (isnan(dFlux) && isnan(flux)) return NAN;

  if (isnan(dFlux)) return (fabs(dMag * flux));
  return dFlux;
}

Catalog *Convert_SMPDATA (FTable *table) {

  off_t Nstars;
  unsigned int i;
  double ZeroPt;
  SMPData *smpdata = NULL;

  smpdata = gfits_table_get_SMPData (table, &Nstars, NULL, NULL);
  if (!smpdata) {
    fprintf (stderr, "skipping inconsistent entry\n");
    return (NULL);
  }
  ZeroPt = GetZeroPoint();

  Catalog *catalog = addstar_catalog_init (Nstars);

  for (i = 0; i < Nstars; i++) {
    catalog->measure[i].Xccd       = smpdata[i].X;
    catalog->measure[i].Yccd       = smpdata[i].Y;
    catalog->measure[i].dXccd      = NAN_S_SHORT; // not provided by SMPDATA:
    catalog->measure[i].dYccd      = NAN_S_SHORT; // not provided by SMPDATA:
   
    catalog->measure[i].posangle   = NAN_S_SHORT; // not provided by SMPDATA:
    catalog->measure[i].pltscale   = NAN;         // not provided by SMPDATA:

    if ((smpdata[i].M >= ZeroPt) || isnan(smpdata[i].M)) {
      catalog->measure[i].M        = NAN;
      catalog->measure[i].Map      = NAN;
      catalog->measure[i].FluxPSF  = NAN;
      catalog->measure[i].dFluxPSF = NAN;
    } else {
      catalog->measure[i].M        = smpdata[i].M;
      catalog->measure[i].Map      = smpdata[i].M;
      catalog->measure[i].FluxPSF  = pow(10.0, -0.4*smpdata[i].M);
      catalog->measure[i].dFluxPSF = catalog->measure[i].FluxPSF * smpdata[i].dM;
    }
    catalog->measure[i].dM         = smpdata[i].dM*0.001;
    catalog->measure[i].dMcal      = NAN; // not provided by SMPDATA:

    catalog->measure[i].Mkron      = NAN; // not provided by SMPDATA:
    catalog->measure[i].dMkron     = NAN; // not provided by SMPDATA:
    catalog->measure[i].FluxKron   = NAN; // not provided by SMPDATA:
    catalog->measure[i].dFluxKron  = NAN; // not provided by SMPDATA:

    catalog->measure[i].Sky        = NAN; // not provided by SMPDATA:
    catalog->measure[i].dSky       = NAN; // not provided by SMPDATA:

    catalog->measure[i].psfChisq   = NAN;       // not provided by SMPDATA:
    catalog->measure[i].psfQF      = NAN;       // not provided by SMPDATA:
    catalog->measure[i].psfNdof    = NAN_S_INT; // not provided by SMPDATA:
    catalog->measure[i].psfNpix    = NAN_S_INT; // not provided by SMPDATA:
    catalog->measure[i].extNsigma  = NAN;       // not provided by SMPDATA:

    catalog->measure[i].FWx        = ToShortPixels (smpdata[i].fx);
    catalog->measure[i].FWy        = ToShortPixels (smpdata[i].fy);
    catalog->measure[i].theta      = ToShortDegrees (smpdata[i].df);

    catalog->measure[i].Mxx        = NAN_S_SHORT; // not provided by SMPDATA:
    catalog->measure[i].Mxy        = NAN_S_SHORT; // not provided by SMPDATA:
    catalog->measure[i].Myy        = NAN_S_SHORT; // not provided by SMPDATA:
                        
    // the dophot type information gets pushed into the upper 2 bytes of photFlags
    catalog->measure[i].photFlags  = (smpdata[i].dophot << 16);
  }    
  return catalog;
}

Catalog *Convert_PS1_DEV_0 (FTable *table) {

  off_t Nstars;
  unsigned int i;
  double ZeroPt;
  PS1_DEV_0 *ps1data;

  ps1data = gfits_table_get_PS1_DEV_0 (table, &Nstars, NULL, NULL);
  if (!ps1data) {
    fprintf (stderr, "skipping inconsistent entry\n");
    return (NULL);
  }
  ZeroPt = GetZeroPoint();

  Catalog *catalog = addstar_catalog_init (Nstars);

  for (i = 0; i < Nstars; i++) {
    catalog->measure[i].Xccd       = ps1data[i].X;
    catalog->measure[i].Yccd       = ps1data[i].Y;
    catalog->measure[i].dXccd      = ToShortPixels(ps1data[i].dX);
    catalog->measure[i].dYccd      = ToShortPixels(ps1data[i].dY);
   
    catalog->measure[i].posangle   = NAN_S_SHORT; // not provided by PS1_DEV_0:
    catalog->measure[i].pltscale   = NAN;         // not provided by PS1_DEV_0:

    if ((ps1data[i].M >= 0.0) || isnan(ps1data[i].M)) {
      catalog->measure[i].M        = NAN;
      catalog->measure[i].FluxPSF  = NAN;
      catalog->measure[i].dFluxPSF = NAN;
    } else {
      catalog->measure[i].M        = ps1data[i].M + ZeroPt;
      catalog->measure[i].FluxPSF  = pow(10.0, -0.4*ps1data[i].M);
      catalog->measure[i].dFluxPSF = catalog->measure[i].FluxPSF * ps1data[i].dM;
    }
    catalog->measure[i].dM         = ps1data[i].dM;
    catalog->measure[i].dMcal      = NAN; // not provided by PS1_DEV_0:
    catalog->measure[i].Map        = NAN; // not provided by PS1_DEV_0:

    catalog->measure[i].Mkron      = NAN; // not provided by PS1_DEV_0:
    catalog->measure[i].dMkron     = NAN; // not provided by PS1_DEV_0:
    catalog->measure[i].FluxKron   = NAN; // not provided by PS1_DEV_0:
    catalog->measure[i].dFluxKron  = NAN; // not provided by PS1_DEV_0:

    catalog->measure[i].Sky        = ps1data[i].sky;
    catalog->measure[i].dSky       = ps1data[i].dSky;

    catalog->measure[i].psfChisq   = ps1data[i].psfChisq;
    catalog->measure[i].psfQF      = ps1data[i].psfQF;
    catalog->measure[i].psfNdof    = NAN_S_INT; // not provided by PS1_DEV_0:
    catalog->measure[i].psfNpix    = NAN_S_INT; // not provided by PS1_DEV_0:
    catalog->measure[i].extNsigma  = NAN;        // not provided by PS1_DEV_0:

    catalog->measure[i].FWx        = ToShortPixels(ps1data[i].fx);
    catalog->measure[i].FWy        = ToShortPixels(ps1data[i].fy);
    catalog->measure[i].theta      = ToShortDegrees(ps1data[i].df);

    catalog->measure[i].Mxx        = NAN_S_SHORT; // not provided by PS1_DEV_0:
    catalog->measure[i].Mxy        = NAN_S_SHORT; // not provided by PS1_DEV_0:
    catalog->measure[i].Myy        = NAN_S_SHORT; // not provided by PS1_DEV_0:
                        
    catalog->measure[i].photFlags  = 0; // not provided by PS1_DEV_0:

    catalog->measure[i].detID      = ps1data[i].detID;
  }    
  return catalog;
}

// XXX I need to make the IPP I/O functions and these functions
// consistent wrt ZERO_POINT....
Catalog *Convert_PS1_DEV_1 (FTable *table) {

  off_t Nstars; 
  unsigned int i;
  double ZeroPt;
  PS1_DEV_1 *ps1data;

  ps1data = gfits_table_get_PS1_DEV_1 (table, &Nstars, NULL, NULL);
  if (!ps1data) {
    fprintf (stderr, "skipping inconsistent entry\n");
    return (NULL);
  }
  ZeroPt = GetZeroPoint();

  Catalog *catalog = addstar_catalog_init (Nstars);

  for (i = 0; i < Nstars; i++) {
    catalog->measure[i].Xccd       = ps1data[i].X;
    catalog->measure[i].Yccd       = ps1data[i].Y;
    catalog->measure[i].dXccd      = ToShortPixels(ps1data[i].dX);
    catalog->measure[i].dYccd      = ToShortPixels(ps1data[i].dY);

    catalog->measure[i].posangle   = NAN_S_SHORT; // not provided by PS1_DEV_1:
    catalog->measure[i].pltscale   = NAN;         // not provided by PS1_DEV_1:

    if ((ps1data[i].M >= 0.0) || isnan(ps1data[i].M)) {
      catalog->measure[i].M      = NAN;
      catalog->measure[i].FluxPSF  = NAN;
      catalog->measure[i].dFluxPSF = NAN;
    } else {
      catalog->measure[i].M      = ps1data[i].M + ZeroPt;
      catalog->measure[i].FluxPSF  = pow(10.0, -0.4*ps1data[i].M);
      catalog->measure[i].dFluxPSF = catalog->measure[i].FluxPSF * ps1data[i].dM;
    }
    catalog->measure[i].dM         = ps1data[i].dM;
    catalog->measure[i].dMcal      = NAN; // not provided by PS1_DEV_1:
    catalog->measure[i].Map        = NAN; // not provided by PS1_DEV_1:

    catalog->measure[i].Mkron      = NAN; // not provided by PS1_DEV_1:
    catalog->measure[i].dMkron     = NAN; // not provided by PS1_DEV_1:
    catalog->measure[i].FluxKron   = NAN; // not provided by PS1_DEV_1:
    catalog->measure[i].dFluxKron  = NAN; // not provided by PS1_DEV_1:

    catalog->measure[i].Sky        = ps1data[i].sky;
    catalog->measure[i].dSky       = ps1data[i].dSky;

    catalog->measure[i].psfChisq   = ps1data[i].psfChisq;
    catalog->measure[i].psfQF      = ps1data[i].psfQF;
    catalog->measure[i].psfNdof    = NAN_S_INT; // not provided by PS1_DEV_1:
    catalog->measure[i].psfNpix    = NAN_S_INT; // not provided by PS1_DEV_1:
    catalog->measure[i].extNsigma  = ps1data[i].extNsigma;

    catalog->measure[i].FWx        = ToShortPixels(ps1data[i].fx);
    catalog->measure[i].FWy        = ToShortPixels(ps1data[i].fy);
    catalog->measure[i].theta      = ToShortDegrees(ps1data[i].df);

    catalog->measure[i].Mxx        = NAN_S_SHORT; // not provided by PS1_DEV_1:
    catalog->measure[i].Mxy        = NAN_S_SHORT; // not provided by PS1_DEV_1:
    catalog->measure[i].Myy        = NAN_S_SHORT; // not provided by PS1_DEV_1:
                        
    catalog->measure[i].photFlags  = ps1data[i].flags;

    // this is may optionally be replaced by the internal sequence (see FilterStars.c)
    catalog->measure[i].detID      = ps1data[i].detID;
  }    
  return catalog;
}

Catalog *Convert_PS1_V1 (FTable *table) {

  off_t Nstars; 
  unsigned int i;
  double ZeroPt;
  CMF_PS1_V1 *ps1data;

  // CMF_PS1_V1 was modified 2009.05.26 (r24251) to use doubles for ra & dec.  this was a
  // mistake in two ways: a new format should have been defined (eg, CMF_PS1_V2), and the
  // layout used did not have clean byte-boundaries for the corresponding structure.  The
  // former means we have two varieties of CMF_PS1_V1 out there; the latter means that the
  // autocode tools do not work to read in the new version, even if we recognize it.  Here we
  // test for the existence of the broken version (table[0].headers[0].Naxis[0] == 136), and
  // call a special conversion function if it is found.

  if (table[0].header[0].Naxis[0] == 136) {
    Catalog *catalog = Convert_PS1_V1_Alt (table);
    return catalog;
  }

  ps1data = gfits_table_get_CMF_PS1_V1 (table, &Nstars, NULL, NULL);
  if (!ps1data) {
    fprintf (stderr, "skipping inconsistent entry\n");
    return (NULL);
  }
  ZeroPt = GetZeroPoint();

  Catalog *catalog = addstar_catalog_init (Nstars);

  for (i = 0; i < Nstars; i++) {
    catalog->measure[i].Xccd       = ps1data[i].X;
    catalog->measure[i].Yccd       = ps1data[i].Y;
    catalog->measure[i].dXccd      = ToShortPixels(ps1data[i].dX);
    catalog->measure[i].dYccd      = ToShortPixels(ps1data[i].dY);

    catalog->measure[i].posangle   = ToShortDegrees(ps1data[i].posangle);
    catalog->measure[i].pltscale   = ps1data[i].pltscale;

    if ((ps1data[i].M >= 0.0) || isnan(ps1data[i].M)) {
      catalog->measure[i].M      = NAN;
      catalog->measure[i].FluxPSF  = NAN;
      catalog->measure[i].dFluxPSF = NAN;
    } else {
      catalog->measure[i].M      = ps1data[i].M + ZeroPt;
      catalog->measure[i].FluxPSF  = pow(10.0, -0.4*ps1data[i].M);
      catalog->measure[i].dFluxPSF = catalog->measure[i].FluxPSF * ps1data[i].dM;
    }
    catalog->measure[i].dM         = ps1data[i].dM;
    catalog->measure[i].dMcal      = ps1data[i].dMcal;
    catalog->measure[i].Map        = ps1data[i].Map + ZeroPt;
    catalog->measure[i].dMap       = ps1data[i].dM; // a proxy measure
                        
    catalog->measure[i].Mkron      = NAN; // not provided by PS1_V1:
    catalog->measure[i].dMkron     = NAN; // not provided by PS1_V1:
    catalog->measure[i].FluxKron   = NAN; // not provided by PS1_V1:
    catalog->measure[i].dFluxKron  = NAN; // not provided by PS1_V1:

    catalog->measure[i].Sky        = ps1data[i].sky;
    catalog->measure[i].dSky       = ps1data[i].dSky;
                        
    catalog->measure[i].psfChisq   = ps1data[i].psfChisq;
    catalog->measure[i].psfQF      = ps1data[i].psfQF;
    catalog->measure[i].psfNdof    = ps1data[i].psfNdof;
    catalog->measure[i].psfNpix    = ps1data[i].psfNpix;
    catalog->measure[i].extNsigma  = ps1data[i].extNsigma;

    catalog->measure[i].FWx        = ToShortPixels(ps1data[i].fx);
    catalog->measure[i].FWy        = ToShortPixels(ps1data[i].fy);
    catalog->measure[i].theta      = ToShortDegrees(ps1data[i].df);

    catalog->measure[i].Mxx        = ToShortPixels(ps1data[i].Mxx);
    catalog->measure[i].Mxy        = ToShortPixels(ps1data[i].Mxy);
    catalog->measure[i].Myy        = ToShortPixels(ps1data[i].Myy);
                        
    catalog->measure[i].photFlags  = ps1data[i].flags;

    // this is may optionally be replaced by the internal sequence (see FilterStars.c)
    catalog->measure[i].detID      = ps1data[i].detID; 

    // the Average fields and the following Measure fields are set in FilterStars after
    // the image metadata is in hand:  dR, dD, Mcal, dt, airmass, az, t, imageID, extID, 
    // averef is set in find_matches, dbFlags is zero on ingest.

    // the following fields are currently not being set anywhere: t_msec
  }    
  return catalog;
}

Catalog *Convert_PS1_V1_Alt (FTable *table) {

  off_t Nstars; 
  unsigned int i;
  double ZeroPt;
  CMF_PS1_V2 *ps1data;

  // CMF_PS1_V1 was modified 2009.05.26 (r24251) to use doubles for ra & dec.  this was a
  // mistake in two ways: a few format should have been defined (eg, CMF_PS1_V2), and the
  // layout used did not have clean byte-boundaries for the corresponding structure.  The
  // former means we have two varieties of CMF_PS1_V1 out there; the latter means that the
  // autocode tools do not work to read in the new version, even if we recognize it.  Here we
  // test for the existence of the broken version (table[0].headers[0].Naxis[0] == 136), and
  // call a special conversion function if it is found.

  ps1data = gfits_table_get_CMF_PS1_V1_Alt (table, &Nstars, NULL); 
  if (!ps1data) {
    fprintf (stderr, "skipping inconsistent entry\n");
    return (NULL);
  }
  ZeroPt = GetZeroPoint();

  Catalog *catalog = addstar_catalog_init (Nstars);

  for (i = 0; i < Nstars; i++) {
    catalog->measure[i].Xccd       = ps1data[i].X;
    catalog->measure[i].Yccd       = ps1data[i].Y;
    catalog->measure[i].dXccd      = ToShortPixels(ps1data[i].dX);
    catalog->measure[i].dYccd      = ToShortPixels(ps1data[i].dY);

    catalog->measure[i].posangle   = ToShortDegrees(ps1data[i].posangle);
    catalog->measure[i].pltscale   = ps1data[i].pltscale;

    if ((ps1data[i].M >= 0.0) || isnan(ps1data[i].M)) {
      catalog->measure[i].M      = NAN;
      catalog->measure[i].FluxPSF  = NAN;
      catalog->measure[i].dFluxPSF = NAN;
    } else {
      catalog->measure[i].M        = ps1data[i].M + ZeroPt;
      catalog->measure[i].FluxPSF  = pow(10.0, -0.4*ps1data[i].M);
      catalog->measure[i].dFluxPSF = catalog->measure[i].FluxPSF * ps1data[i].dM;
    }
    catalog->measure[i].dM         = ps1data[i].dM;
    catalog->measure[i].dMcal      = ps1data[i].dMcal;
    catalog->measure[i].Map        = ps1data[i].Map + ZeroPt;
    catalog->measure[i].dMap       = ps1data[i].dM; // a proxy measure
                        
    catalog->measure[i].Mkron      = NAN; // not provided by PS1_V1_Alt:
    catalog->measure[i].dMkron     = NAN; // not provided by PS1_V1_Alt:
    catalog->measure[i].FluxKron   = NAN; // not provided by PS1_V1_Alt:
    catalog->measure[i].dFluxKron  = NAN; // not provided by PS1_V1_Alt:

    catalog->measure[i].Sky        = ps1data[i].sky;
    catalog->measure[i].dSky       = ps1data[i].dSky;
                        
    catalog->measure[i].psfChisq   = ps1data[i].psfChisq;
    catalog->measure[i].psfQF      = ps1data[i].psfQF;
    catalog->measure[i].psfNdof    = ps1data[i].psfNdof;
    catalog->measure[i].psfNpix    = ps1data[i].psfNpix;
    catalog->measure[i].extNsigma  = ps1data[i].extNsigma;

    catalog->measure[i].FWx        = ToShortPixels(ps1data[i].fx);
    catalog->measure[i].FWy        = ToShortPixels(ps1data[i].fy);
    catalog->measure[i].theta      = ToShortDegrees(ps1data[i].df);

    catalog->measure[i].Mxx        = ToShortPixels(ps1data[i].Mxx);
    catalog->measure[i].Mxy        = ToShortPixels(ps1data[i].Mxy);
    catalog->measure[i].Myy        = ToShortPixels(ps1data[i].Myy);
                        
    catalog->measure[i].photFlags  = ps1data[i].flags;

    // this is may optionally be replaced by the internal sequence (see FilterStars.c)
    catalog->measure[i].detID      = ps1data[i].detID; 

    // the Average fields and the following Measure fields are set in FilterStars after
    // the image metadata is in hand:  dR, dD, Mcal, dt, airmass, az, t, imageID, extID, 
    // averef is set in find_matches, dbFlags is zero on ingest.

    // the following fields are currently not being set anywhere: t_msec
  }    
  return catalog;
}

Catalog *Convert_PS1_V2 (FTable *table) {

  off_t Nstars; 
  unsigned int i;
  double ZeroPt;
  CMF_PS1_V2 *ps1data;

  ps1data = gfits_table_get_CMF_PS1_V2 (table, &Nstars, NULL, NULL);
  if (!ps1data) {
    fprintf (stderr, "skipping inconsistent entry\n");
    return (NULL);
  }
  ZeroPt = GetZeroPoint();

  Catalog *catalog = addstar_catalog_init (Nstars);

  for (i = 0; i < Nstars; i++) {
    catalog->measure[i].Xccd       = ps1data[i].X;
    catalog->measure[i].Yccd       = ps1data[i].Y;
    catalog->measure[i].dXccd      = ToShortPixels(ps1data[i].dX);
    catalog->measure[i].dYccd      = ToShortPixels(ps1data[i].dY);

    catalog->measure[i].posangle   = ToShortDegrees(ps1data[i].posangle);
    catalog->measure[i].pltscale   = ps1data[i].pltscale;

    if ((ps1data[i].M >= 0.0) || isnan(ps1data[i].M)) {
      catalog->measure[i].M        = NAN;
      catalog->measure[i].FluxPSF  = NAN;
      catalog->measure[i].dFluxPSF = NAN;
    } else {
      catalog->measure[i].M      = ps1data[i].M + ZeroPt;
      catalog->measure[i].FluxPSF  = pow(10.0, -0.4*ps1data[i].M);
      catalog->measure[i].dFluxPSF = catalog->measure[i].FluxPSF * ps1data[i].dM;
    }
    catalog->measure[i].dM         = ps1data[i].dM;
    catalog->measure[i].dMcal      = ps1data[i].dMcal;
    catalog->measure[i].Map        = ps1data[i].Map + ZeroPt;
    catalog->measure[i].dMap       = ps1data[i].dM; // a proxy measure
                        
    catalog->measure[i].Mkron      = NAN; // not provided by PS1_V2:
    catalog->measure[i].dMkron     = NAN; // not provided by PS1_V2:
    catalog->measure[i].FluxKron   = NAN; // not provided by PS1_V2:
    catalog->measure[i].dFluxKron  = NAN; // not provided by PS1_V2:

    catalog->measure[i].Sky        = ps1data[i].sky;
    catalog->measure[i].dSky       = ps1data[i].dSky;
                        
    catalog->measure[i].psfChisq   = ps1data[i].psfChisq;
    catalog->measure[i].psfQF      = ps1data[i].psfQF;
    catalog->measure[i].psfNdof    = ps1data[i].psfNdof;
    catalog->measure[i].psfNpix    = ps1data[i].psfNpix;
    catalog->measure[i].extNsigma  = ps1data[i].extNsigma;

    catalog->measure[i].FWx        = ToShortPixels(ps1data[i].fx);
    catalog->measure[i].FWy        = ToShortPixels(ps1data[i].fy);
    catalog->measure[i].theta      = ToShortDegrees(ps1data[i].df);

    catalog->measure[i].Mxx        = ToShortPixels(ps1data[i].Mxx);
    catalog->measure[i].Mxy        = ToShortPixels(ps1data[i].Mxy);
    catalog->measure[i].Myy        = ToShortPixels(ps1data[i].Myy);
                        
    catalog->measure[i].photFlags  = ps1data[i].flags;

    // this is may optionally be replaced by the internal sequence (see FilterStars.c)
    catalog->measure[i].detID      = ps1data[i].detID; 

    // the Average fields and the following Measure fields are set in FilterStars after
    // the image metadata is in hand:  dR, dD, Mcal, dt, airmass, az, t, imageID, extID, 
    // averef is set in find_matches, dbFlags is zero on ingest.

    // the following fields are currently not being set anywhere: t_msec
  }    
  return catalog;
}

Catalog *Convert_PS1_V3 (FTable *table) {

  off_t Nstars; 
  unsigned int i;
  double ZeroPt;
  CMF_PS1_V3 *ps1data;

  ps1data = gfits_table_get_CMF_PS1_V3 (table, &Nstars, NULL, NULL);
  if (!ps1data) {
    fprintf (stderr, "skipping inconsistent entry\n");
    return (NULL);
  }
  ZeroPt = GetZeroPoint();

  Catalog *catalog = addstar_catalog_init (Nstars);

  for (i = 0; i < Nstars; i++) {
    catalog->measure[i].Xccd       = ps1data[i].X;
    catalog->measure[i].Yccd       = ps1data[i].Y;
    catalog->measure[i].dXccd      = ToShortPixels(ps1data[i].dX);
    catalog->measure[i].dYccd      = ToShortPixels(ps1data[i].dY);

    catalog->measure[i].posangle   = ToShortDegrees(ps1data[i].posangle);
    catalog->measure[i].pltscale   = ps1data[i].pltscale;

    if ((ps1data[i].M >= 0.0) || isnan(ps1data[i].M)) {
      catalog->measure[i].M      = NAN;
    } else {
      catalog->measure[i].M      = ps1data[i].M + ZeroPt;
    }
    catalog->measure[i].dM         = ps1data[i].dM;
    catalog->measure[i].dMcal      = ps1data[i].dMcal;
    catalog->measure[i].Map        = ps1data[i].Map + ZeroPt;
    catalog->measure[i].dMap       = ps1data[i].dM; // a proxy measure
                        
    catalog->measure[i].Mkron      = (ps1data[i].kronFlux > 0.0) ? -2.5*log10(ps1data[i].kronFlux) + ZeroPt : NAN;
    catalog->measure[i].dMkron     = (ps1data[i].kronFlux > 0.0) ? ps1data[i].kronFluxErr / ps1data[i].kronFlux : NAN;
                        
    // these fluxes are converted from counts to counts/sec in FilterStars.c
    catalog->measure[i].FluxPSF    = GetFluxFromFluxOrMag (ps1data[i].Flux, ps1data[i].M); 
    catalog->measure[i].dFluxPSF   = GetFluxErrFromFluxOrMag (ps1data[i].dFlux, catalog->measure[i].FluxPSF, ps1data[i].dM);
    catalog->measure[i].FluxKron   = ps1data[i].kronFlux;
    catalog->measure[i].dFluxKron  = ps1data[i].kronFluxErr;
    catalog->measure[i].FluxAp     = GetFluxFromFluxOrMag (NAN, ps1data[i].Map); 
    catalog->measure[i].dFluxAp    = GetFluxErrFromFluxOrMag (NAN, catalog->measure[i].FluxAp, catalog->measure[i].dMap);

    catalog->measure[i].Sky        = ps1data[i].sky;
    catalog->measure[i].dSky       = ps1data[i].dSky;
                        
    catalog->measure[i].psfChisq   = ps1data[i].psfChisq;
    catalog->measure[i].psfQF      = ps1data[i].psfQF;
    catalog->measure[i].psfQFperf  = ps1data[i].psfQFperf;
    catalog->measure[i].psfNdof    = ps1data[i].psfNdof;
    catalog->measure[i].psfNpix    = ps1data[i].psfNpix;
    catalog->measure[i].extNsigma  = ps1data[i].extNsigma;

    catalog->measure[i].FWx        = ToShortPixels(ps1data[i].fx);
    catalog->measure[i].FWy        = ToShortPixels(ps1data[i].fy);
    catalog->measure[i].theta      = ToShortDegrees(ps1data[i].df);

    catalog->measure[i].Mxx        = ToShortPixels(ps1data[i].Mxx);
    catalog->measure[i].Mxy        = ToShortPixels(ps1data[i].Mxy);
    catalog->measure[i].Myy        = ToShortPixels(ps1data[i].Myy);
                        
    catalog->measure[i].photFlags  = ps1data[i].flags;
    catalog->measure[i].photFlags2 = ps1data[i].flags2;

    // this is may optionally be replaced by the internal sequence (see FilterStars.c)
    catalog->measure[i].detID      = ps1data[i].detID; 

    // the Average fields and the following Measure fields are set in FilterStars after
    // the image metadata is in hand:  dR, dD, Mcal, dt, airmass, az, t, imageID, extID.

    // averef is set in find_matches

    // dbFlags is zero on ingest.

    // the following fields are currently not being set anywhere: t_msec
  }    
  return catalog;
}

Catalog *Convert_PS1_V4 (FTable *table) {

  off_t Nstars; 
  unsigned int i;
  double ZeroPt;
  CMF_PS1_V4 *ps1data;

  ps1data = gfits_table_get_CMF_PS1_V4 (table, &Nstars, NULL, NULL);
  if (!ps1data) {
    fprintf (stderr, "skipping inconsistent entry\n");
    return (NULL);
  }
  ZeroPt = GetZeroPoint();

  Catalog *catalog = addstar_catalog_init (Nstars);

  for (i = 0; i < Nstars; i++) {
    catalog->measure[i].Xccd       = ps1data[i].X;
    catalog->measure[i].Yccd       = ps1data[i].Y;
    catalog->measure[i].dXccd      = ToShortPixels(ps1data[i].dX);
    catalog->measure[i].dYccd      = ToShortPixels(ps1data[i].dY);

    catalog->measure[i].posangle   = ToShortDegrees(ps1data[i].posangle);
    catalog->measure[i].pltscale   = ps1data[i].pltscale;

    if ((ps1data[i].M >= 0.0) || isnan(ps1data[i].M)) {
      catalog->measure[i].M      = NAN;
    } else {
      catalog->measure[i].M      = ps1data[i].M + ZeroPt;
    }
    catalog->measure[i].dM         = ps1data[i].dM;
    catalog->measure[i].dMcal      = ps1data[i].dMcal;
    catalog->measure[i].Map        = ps1data[i].Map + ZeroPt;
    catalog->measure[i].dMap       = (ps1data[i].apFlux > 0.0) ? fabs(ps1data[i].apFluxErr / ps1data[i].apFlux) : NAN;
                        
    catalog->measure[i].Mkron      = (ps1data[i].kronFlux > 0.0) ? -2.5*log10(ps1data[i].kronFlux) + ZeroPt : NAN;
    catalog->measure[i].dMkron     = (ps1data[i].kronFlux > 0.0) ? ps1data[i].kronFluxErr / ps1data[i].kronFlux : NAN;
                        
    // these fluxes are converted from counts to counts/sec in FilterStars.c
    catalog->measure[i].FluxPSF    = GetFluxFromFluxOrMag (ps1data[i].Flux, ps1data[i].M); 
    catalog->measure[i].dFluxPSF   = GetFluxErrFromFluxOrMag (ps1data[i].dFlux, catalog->measure[i].FluxPSF, ps1data[i].dM);
    catalog->measure[i].FluxKron   = ps1data[i].kronFlux;
    catalog->measure[i].dFluxKron  = ps1data[i].kronFluxErr;
    catalog->measure[i].FluxAp     = GetFluxFromFluxOrMag (ps1data[i].apFlux, ps1data[i].Map); 
    catalog->measure[i].dFluxAp    = GetFluxErrFromFluxOrMag (ps1data[i].apFluxErr, catalog->measure[i].FluxAp, catalog->measure[i].dMap);

    catalog->measure[i].Sky        = ps1data[i].sky;
    catalog->measure[i].dSky       = ps1data[i].dSky;
                        
    catalog->measure[i].psfChisq   = ps1data[i].psfChisq;
    catalog->measure[i].psfQF      = ps1data[i].psfQF;
    catalog->measure[i].psfQFperf  = ps1data[i].psfQFperf;

    catalog->measure[i].psfNdof    = ps1data[i].psfNdof;
    catalog->measure[i].psfNpix    = ps1data[i].psfNpix;
    catalog->measure[i].extNsigma  = ps1data[i].extNsigma;

    catalog->measure[i].FWx        = ToShortPixels(ps1data[i].fx);
    catalog->measure[i].FWy        = ToShortPixels(ps1data[i].fy);
    catalog->measure[i].theta      = ToShortDegrees(ps1data[i].df);

    catalog->measure[i].Mxx        = ToShortPixels(ps1data[i].Mxx);
    catalog->measure[i].Mxy        = ToShortPixels(ps1data[i].Mxy);
    catalog->measure[i].Myy        = ToShortPixels(ps1data[i].Myy);
                        
    catalog->measure[i].photFlags  = ps1data[i].flags;
    catalog->measure[i].photFlags2 = ps1data[i].flags2;

    // this is may optionally be replaced by the internal sequence (see FilterStars.c)
    catalog->measure[i].detID      = ps1data[i].detID; 

    // the Average fields and the following Measure fields are set in FilterStars after
    // the image metadata is in hand:  dR, dD, Mcal, dt, airmass, az, t, imageID, extID.

    // averef is set in find_matches

    // dbFlags is zero on ingest.

    // the following fields are currently not being set anywhere: t_msec
  }    
  return catalog;
}

Catalog *Convert_PS1_V5 (FTable *table) {

  off_t Nstars; 
  unsigned int i;
  double ZeroPt;
  CMF_PS1_V5 *ps1data;

  ps1data = gfits_table_get_CMF_PS1_V5 (table, &Nstars, NULL);
  if (!ps1data) {
    fprintf (stderr, "skipping inconsistent entry\n");
    return (NULL);
  }
  ZeroPt = GetZeroPoint();

  Catalog *catalog = addstar_catalog_init (Nstars);

  for (i = 0; i < Nstars; i++) {
    catalog->measure[i].Xccd       = ps1data[i].X;
    catalog->measure[i].Yccd       = ps1data[i].Y;
    catalog->measure[i].dXccd      = ToShortPixels(ps1data[i].dX);
    catalog->measure[i].dYccd      = ToShortPixels(ps1data[i].dY);

    catalog->measure[i].posangle   = ToShortDegrees(ps1data[i].posangle);
    catalog->measure[i].pltscale   = ps1data[i].pltscale;

    if ((ps1data[i].M >= 0.0) || isnan(ps1data[i].M)) {
      catalog->measure[i].M      = NAN;
    } else {
      catalog->measure[i].M      = ps1data[i].M + ZeroPt;
    }
    catalog->measure[i].dM         = ps1data[i].dM;
    catalog->measure[i].dMcal      = ps1data[i].dMcal;
    catalog->measure[i].Map        = ps1data[i].Map + ZeroPt;
    catalog->measure[i].dMap       = (ps1data[i].apFlux > 0.0) ? fabs(ps1data[i].apFluxErr / ps1data[i].apFlux) : NAN;
                        
    catalog->measure[i].Mkron      = (ps1data[i].kronFlux > 0.0) ? -2.5*log10(ps1data[i].kronFlux) + ZeroPt : NAN;
    catalog->measure[i].dMkron     = (ps1data[i].kronFlux > 0.0) ? ps1data[i].kronFluxErr / ps1data[i].kronFlux : NAN;
                        
    // these fluxes are converted from counts to counts/sec in FilterStars.c
    catalog->measure[i].FluxPSF    = GetFluxFromFluxOrMag (ps1data[i].Flux, ps1data[i].M); 
    catalog->measure[i].dFluxPSF   = GetFluxErrFromFluxOrMag (ps1data[i].dFlux, catalog->measure[i].FluxPSF, ps1data[i].dM);
    catalog->measure[i].FluxKron   = ps1data[i].kronFlux;
    catalog->measure[i].dFluxKron  = ps1data[i].kronFluxErr;
    catalog->measure[i].FluxAp     = GetFluxFromFluxOrMag (ps1data[i].apFlux, ps1data[i].Map); 
    catalog->measure[i].dFluxAp    = GetFluxErrFromFluxOrMag (ps1data[i].apFluxErr, catalog->measure[i].FluxAp, catalog->measure[i].dMap);

    catalog->measure[i].Sky        = ps1data[i].sky;
    catalog->measure[i].dSky       = ps1data[i].dSky;
                        
    catalog->measure[i].psfChisq   = ps1data[i].psfChisq;
    catalog->measure[i].psfQF      = ps1data[i].psfQF;
    catalog->measure[i].psfQFperf  = ps1data[i].psfQFperf;

    catalog->measure[i].psfNdof    = ps1data[i].psfNdof;
    catalog->measure[i].psfNpix    = ps1data[i].psfNpix;
    catalog->measure[i].extNsigma  = ps1data[i].extNsigma;

    catalog->measure[i].FWx        = ToShortPixels(ps1data[i].fx);
    catalog->measure[i].FWy        = ToShortPixels(ps1data[i].fy);
    catalog->measure[i].theta      = ToShortDegrees(ps1data[i].df);

    catalog->measure[i].Mxx        = ToShortPixels(ps1data[i].Mxx);
    catalog->measure[i].Mxy        = ToShortPixels(ps1data[i].Mxy);
    catalog->measure[i].Myy        = ToShortPixels(ps1data[i].Myy);
                        
    catalog->measure[i].photFlags  = ps1data[i].flags;
    catalog->measure[i].photFlags2 = ps1data[i].flags2;

    // this is may optionally be replaced by the internal sequence (see FilterStars.c)
    catalog->measure[i].detID      = ps1data[i].detID; 

    // the Average fields and the following Measure fields are set in FilterStars after
    // the image metadata is in hand:  dR, dD, Mcal, dt, airmass, az, t, imageID, extID.

    // averef is set in find_matches

    // dbFlags is zero on ingest.

    // the following fields are currently not being set anywhere: t_msec
  }    
  return catalog;
}

// alternate version of PS1_V5 (UNIONS TEST?)
Catalog *Convert_PS1_V5_R0 (FTable *table) {

  off_t Nstars; 
  unsigned int i;
  double ZeroPt;
  CMF_PS1_V5_R0 *ps1data;

  ps1data = gfits_table_get_CMF_PS1_V5_R0 (table, &Nstars, NULL);
  if (!ps1data) {
    fprintf (stderr, "skipping inconsistent entry\n");
    return (NULL);
  }
  ZeroPt = GetZeroPoint();

  Catalog *catalog = addstar_catalog_init (Nstars);

  for (i = 0; i < Nstars; i++) {
    catalog->measure[i].Xccd       = ps1data[i].X;
    catalog->measure[i].Yccd       = ps1data[i].Y;
    catalog->measure[i].dXccd      = ToShortPixels(ps1data[i].dX);
    catalog->measure[i].dYccd      = ToShortPixels(ps1data[i].dY);

    catalog->measure[i].posangle   = ToShortDegrees(ps1data[i].posangle);
    catalog->measure[i].pltscale   = ps1data[i].pltscale;

    if ((ps1data[i].M >= 0.0) || isnan(ps1data[i].M)) {
      catalog->measure[i].M      = NAN;
    } else {
      catalog->measure[i].M      = ps1data[i].M + ZeroPt;
    }
    catalog->measure[i].dM         = ps1data[i].dM;
    catalog->measure[i].dMcal      = ps1data[i].dMcal;
    catalog->measure[i].Map        = ps1data[i].Map + ZeroPt;
    catalog->measure[i].dMap       = (ps1data[i].apFlux > 0.0) ? fabs(ps1data[i].apFluxErr / ps1data[i].apFlux) : NAN;
                        
    catalog->measure[i].Mkron      = (ps1data[i].kronFlux > 0.0) ? -2.5*log10(ps1data[i].kronFlux) + ZeroPt : NAN;
    catalog->measure[i].dMkron     = (ps1data[i].kronFlux > 0.0) ? ps1data[i].kronFluxErr / ps1data[i].kronFlux : NAN;
                        
    // these fluxes are converted from counts to counts/sec in FilterStars.c
    catalog->measure[i].FluxPSF    = GetFluxFromFluxOrMag (ps1data[i].Flux, ps1data[i].M); 
    catalog->measure[i].dFluxPSF   = GetFluxErrFromFluxOrMag (ps1data[i].dFlux, catalog->measure[i].FluxPSF, ps1data[i].dM);
    catalog->measure[i].FluxKron   = ps1data[i].kronFlux;
    catalog->measure[i].dFluxKron  = ps1data[i].kronFluxErr;
    catalog->measure[i].FluxAp     = GetFluxFromFluxOrMag (ps1data[i].apFlux, ps1data[i].Map); 
    catalog->measure[i].dFluxAp    = GetFluxErrFromFluxOrMag (ps1data[i].apFluxErr, catalog->measure[i].FluxAp, catalog->measure[i].dMap);

    catalog->measure[i].Sky        = ps1data[i].sky;
    catalog->measure[i].dSky       = ps1data[i].dSky;
                        
    catalog->measure[i].psfChisq   = ps1data[i].psfChisq;
    catalog->measure[i].psfQF      = ps1data[i].psfQF;
    catalog->measure[i].psfQFperf  = ps1data[i].psfQFperf;

    catalog->measure[i].psfNdof    = ps1data[i].psfNdof;
    catalog->measure[i].psfNpix    = ps1data[i].psfNpix;
    catalog->measure[i].extNsigma  = ps1data[i].extNsigma;

    catalog->measure[i].FWx        = ToShortPixels(ps1data[i].fx);
    catalog->measure[i].FWy        = ToShortPixels(ps1data[i].fy);
    catalog->measure[i].theta      = ToShortDegrees(ps1data[i].df);

    catalog->measure[i].Mxx        = ToShortPixels(ps1data[i].Mxx);
    catalog->measure[i].Mxy        = ToShortPixels(ps1data[i].Mxy);
    catalog->measure[i].Myy        = ToShortPixels(ps1data[i].Myy);
                        
    catalog->measure[i].photFlags  = ps1data[i].flags;
    catalog->measure[i].photFlags2 = ps1data[i].flags2;

    // this is may optionally be replaced by the internal sequence (see FilterStars.c)
    catalog->measure[i].detID      = ps1data[i].detID; 

    // the Average fields and the following Measure fields are set in FilterStars after
    // the image metadata is in hand:  dR, dD, Mcal, dt, airmass, az, t, imageID, extID.

    // averef is set in find_matches

    // dbFlags is zero on ingest.

    // the following fields are currently not being set anywhere: t_msec
  }    
  return catalog;
}

Catalog *Convert_PS1_V5_R0_Lensing (FTable *table) {

  off_t Nstars; 
  unsigned int i;
  double ZeroPt;
  CMF_PS1_V5_R0_Lensing *ps1data;

  ps1data = gfits_table_get_CMF_PS1_V5_R0_Lensing (table, &Nstars, NULL);
  if (!ps1data) {
    fprintf (stderr, "skipping inconsistent entry\n");
    return (NULL);
  }
  ZeroPt = GetZeroPoint();

  Catalog *catalog = addstar_catalog_init (Nstars);
  ALLOCATE (catalog->lensing, Lensing, Nstars);
  catalog->Nlensing = Nstars;

  for (i = 0; i < Nstars; i++) {
    catalog->measure[i].Xccd       = ps1data[i].X;
    catalog->measure[i].Yccd       = ps1data[i].Y;
    catalog->measure[i].dXccd      = ToShortPixels(ps1data[i].dX);
    catalog->measure[i].dYccd      = ToShortPixels(ps1data[i].dY);

    catalog->measure[i].posangle   = ToShortDegrees(ps1data[i].posangle);
    catalog->measure[i].pltscale   = ps1data[i].pltscale;

    if ((ps1data[i].M >= 0.0) || isnan(ps1data[i].M)) {
      catalog->measure[i].M      = NAN;
    } else {
      catalog->measure[i].M      = ps1data[i].M + ZeroPt;
    }
    catalog->measure[i].dM         = ps1data[i].dM;
    catalog->measure[i].dMcal      = ps1data[i].dMcal;
    catalog->measure[i].Map        = ps1data[i].Map + ZeroPt;
    catalog->measure[i].dMap       = (ps1data[i].apFlux > 0.0) ? fabs(ps1data[i].apFluxErr / ps1data[i].apFlux) : NAN;
                        
    catalog->measure[i].Mkron      = (ps1data[i].kronFlux > 0.0) ? -2.5*log10(ps1data[i].kronFlux) + ZeroPt : NAN;
    catalog->measure[i].dMkron     = (ps1data[i].kronFlux > 0.0) ? ps1data[i].kronFluxErr / ps1data[i].kronFlux : NAN;
                        
    // these fluxes are converted from counts to counts/sec in FilterStars.c
    catalog->measure[i].FluxPSF    = GetFluxFromFluxOrMag (ps1data[i].Flux, ps1data[i].M); 
    catalog->measure[i].dFluxPSF   = GetFluxErrFromFluxOrMag (ps1data[i].dFlux, catalog->measure[i].FluxPSF, ps1data[i].dM);
    catalog->measure[i].FluxKron   = ps1data[i].kronFlux;
    catalog->measure[i].dFluxKron  = ps1data[i].kronFluxErr;
    catalog->measure[i].FluxAp     = GetFluxFromFluxOrMag (ps1data[i].apFlux, ps1data[i].Map); 
    catalog->measure[i].dFluxAp    = GetFluxErrFromFluxOrMag (ps1data[i].apFluxErr, catalog->measure[i].FluxAp, catalog->measure[i].dMap);

    catalog->measure[i].Sky        = ps1data[i].sky;
    catalog->measure[i].dSky       = ps1data[i].dSky;
                        
    catalog->measure[i].psfChisq   = ps1data[i].psfChisq;
    catalog->measure[i].psfQF      = ps1data[i].psfQF;
    catalog->measure[i].psfQFperf  = ps1data[i].psfQFperf;

    catalog->measure[i].psfNdof    = ps1data[i].psfNdof;
    catalog->measure[i].psfNpix    = ps1data[i].psfNpix;
    catalog->measure[i].extNsigma  = ps1data[i].extNsigma;

    catalog->measure[i].FWx        = ToShortPixels(ps1data[i].fx);
    catalog->measure[i].FWy        = ToShortPixels(ps1data[i].fy);
    catalog->measure[i].theta      = ToShortDegrees(ps1data[i].df);

    catalog->measure[i].Mxx        = ToShortPixels(ps1data[i].Mxx);
    catalog->measure[i].Mxy        = ToShortPixels(ps1data[i].Mxy);
    catalog->measure[i].Myy        = ToShortPixels(ps1data[i].Myy);
                        
    catalog->measure[i].photFlags  = ps1data[i].flags;
    catalog->measure[i].photFlags2 = ps1data[i].flags2;

    // this is may optionally be replaced by the internal sequence (see FilterStars.c)
    catalog->measure[i].detID      = ps1data[i].detID; 

    dvo_lensing_init (&catalog->lensing[i]);

    catalog->lensing[i].X11_sm_obj  = ps1data[i].X11_sm_obj;
    catalog->lensing[i].X12_sm_obj  = ps1data[i].X12_sm_obj;
    catalog->lensing[i].X22_sm_obj  = ps1data[i].X22_sm_obj;
    catalog->lensing[i].E1_sm_obj   = ps1data[i].E1_sm_obj;
    catalog->lensing[i].E2_sm_obj   = ps1data[i].E2_sm_obj;

    catalog->lensing[i].X11_sh_obj  = ps1data[i].X11_sh_obj;
    catalog->lensing[i].X12_sh_obj  = ps1data[i].X12_sh_obj;
    catalog->lensing[i].X22_sh_obj  = ps1data[i].X22_sh_obj;
    catalog->lensing[i].E1_sh_obj   = ps1data[i].E1_sh_obj;
    catalog->lensing[i].E2_sh_obj   = ps1data[i].E2_sh_obj;

    catalog->lensing[i].X11_sm_psf  = ps1data[i].X11_sm_psf;
    catalog->lensing[i].X12_sm_psf  = ps1data[i].X12_sm_psf;
    catalog->lensing[i].X22_sm_psf  = ps1data[i].X22_sm_psf;
    catalog->lensing[i].E1_sm_psf   = ps1data[i].E1_sm_psf;
    catalog->lensing[i].E2_sm_psf   = ps1data[i].E2_sm_psf;

    catalog->lensing[i].X11_sh_psf  = ps1data[i].X11_sh_psf;
    catalog->lensing[i].X12_sh_psf  = ps1data[i].X12_sh_psf;
    catalog->lensing[i].X22_sh_psf  = ps1data[i].X22_sh_psf;
    catalog->lensing[i].E1_sh_psf   = ps1data[i].E1_sh_psf;
    catalog->lensing[i].E2_sh_psf   = ps1data[i].E2_sh_psf;

    // catalog->lensing[i].F_ApR5    = ps1data[i].F_ApR5;
    // catalog->lensing[i].dF_ApR5   = ps1data[i].dF_ApR5;
    // catalog->lensing[i].sF_ApR5   = ps1data[i].sF_ApR5;
    // catalog->lensing[i].fF_ApR5   = ps1data[i].fF_ApR5;
    // 
    // catalog->lensing[i].F_ApR6    = ps1data[i].F_ApR6;
    // catalog->lensing[i].dF_ApR6   = ps1data[i].dF_ApR6;
    // catalog->lensing[i].sF_ApR6   = ps1data[i].sF_ApR6;
    // catalog->lensing[i].fF_ApR6   = ps1data[i].fF_ApR6;

    // this is may optionally be replaced by the internal sequence (see FilterStars.c)
    catalog->lensing[i].detID      = ps1data[i].detID; 

    // the Average fields and the following Measure fields are set in FilterStars after
    // the image metadata is in hand:  dR, dD, Mcal, dt, airmass, az, t, imageID, extID.

    // averef is set in find_matches

    // dbFlags is zero on ingest.

    // the following fields are currently not being set anywhere: t_msec
  }    
  return catalog;
}

Catalog *Convert_PS1_V5_R1_Lensing (FTable *table) {

  off_t Nstars; 
  unsigned int i;
  double ZeroPt;
  CMF_PS1_V5_R1_Lensing *ps1data;

  ps1data = gfits_table_get_CMF_PS1_V5_R1_Lensing (table, &Nstars, NULL);
  if (!ps1data) {
    fprintf (stderr, "skipping inconsistent entry\n");
    return (NULL);
  }
  ZeroPt = GetZeroPoint();

  Catalog *catalog = addstar_catalog_init (Nstars);
  ALLOCATE (catalog->lensing, Lensing, Nstars);
  catalog->Nlensing = Nstars;

  for (i = 0; i < Nstars; i++) {
    catalog->measure[i].Xccd       = ps1data[i].X;
    catalog->measure[i].Yccd       = ps1data[i].Y;
    catalog->measure[i].dXccd      = ToShortPixels(ps1data[i].dX);
    catalog->measure[i].dYccd      = ToShortPixels(ps1data[i].dY);

    catalog->measure[i].posangle   = ToShortDegrees(ps1data[i].posangle);
    catalog->measure[i].pltscale   = ps1data[i].pltscale;

    if ((ps1data[i].M >= 0.0) || isnan(ps1data[i].M)) {
      catalog->measure[i].M      = NAN;
    } else {
      catalog->measure[i].M      = ps1data[i].M + ZeroPt;
    }
    catalog->measure[i].dM         = ps1data[i].dM;
    catalog->measure[i].dMcal      = ps1data[i].dMcal;
    catalog->measure[i].Map        = ps1data[i].Map + ZeroPt;
    catalog->measure[i].dMap       = (ps1data[i].apFlux > 0.0) ? fabs(ps1data[i].apFluxErr / ps1data[i].apFlux) : NAN;
                        
    catalog->measure[i].Mkron      = (ps1data[i].kronFlux > 0.0) ? -2.5*log10(ps1data[i].kronFlux) + ZeroPt : NAN;
    catalog->measure[i].dMkron     = (ps1data[i].kronFlux > 0.0) ? ps1data[i].kronFluxErr / ps1data[i].kronFlux : NAN;
                        
    // these fluxes are converted from counts to counts/sec in FilterStars.c
    catalog->measure[i].FluxPSF    = GetFluxFromFluxOrMag (ps1data[i].Flux, ps1data[i].M); 
    catalog->measure[i].dFluxPSF   = GetFluxErrFromFluxOrMag (ps1data[i].dFlux, catalog->measure[i].FluxPSF, ps1data[i].dM);
    catalog->measure[i].FluxKron   = ps1data[i].kronFlux;
    catalog->measure[i].dFluxKron  = ps1data[i].kronFluxErr;
    catalog->measure[i].FluxAp     = GetFluxFromFluxOrMag (ps1data[i].apFlux, ps1data[i].Map); 
    catalog->measure[i].dFluxAp    = GetFluxErrFromFluxOrMag (ps1data[i].apFluxErr, catalog->measure[i].FluxAp, catalog->measure[i].dMap);

    catalog->measure[i].Sky        = ps1data[i].sky;
    catalog->measure[i].dSky       = ps1data[i].dSky;
                        
    catalog->measure[i].psfChisq   = ps1data[i].psfChisq;
    catalog->measure[i].psfQF      = ps1data[i].psfQF;
    catalog->measure[i].psfQFperf  = ps1data[i].psfQFperf;

    catalog->measure[i].psfNdof    = ps1data[i].psfNdof;
    catalog->measure[i].psfNpix    = ps1data[i].psfNpix;
    catalog->measure[i].extNsigma  = ps1data[i].extNsigma;

    catalog->measure[i].FWx        = ToShortPixels(ps1data[i].fx);
    catalog->measure[i].FWy        = ToShortPixels(ps1data[i].fy);
    catalog->measure[i].theta      = ToShortDegrees(ps1data[i].df);

    catalog->measure[i].Mxx        = ToShortPixels(ps1data[i].Mxx);
    catalog->measure[i].Mxy        = ToShortPixels(ps1data[i].Mxy);
    catalog->measure[i].Myy        = ToShortPixels(ps1data[i].Myy);
                        
    catalog->measure[i].photFlags  = ps1data[i].flags;
    catalog->measure[i].photFlags2 = ps1data[i].flags2;

    // this is may optionally be replaced by the internal sequence (see FilterStars.c)
    catalog->measure[i].detID      = ps1data[i].detID; 

    dvo_lensing_init (&catalog->lensing[i]);

    catalog->lensing[i].X11_sm_obj  = ps1data[i].X11_sm_obj;
    catalog->lensing[i].X12_sm_obj  = ps1data[i].X12_sm_obj;
    catalog->lensing[i].X22_sm_obj  = ps1data[i].X22_sm_obj;
    catalog->lensing[i].E1_sm_obj   = ps1data[i].E1_sm_obj;
    catalog->lensing[i].E2_sm_obj   = ps1data[i].E2_sm_obj;

    catalog->lensing[i].X11_sh_obj  = ps1data[i].X11_sh_obj;
    catalog->lensing[i].X12_sh_obj  = ps1data[i].X12_sh_obj;
    catalog->lensing[i].X22_sh_obj  = ps1data[i].X22_sh_obj;
    catalog->lensing[i].E1_sh_obj   = ps1data[i].E1_sh_obj;
    catalog->lensing[i].E2_sh_obj   = ps1data[i].E2_sh_obj;

    catalog->lensing[i].X11_sm_psf  = ps1data[i].X11_sm_psf;
    catalog->lensing[i].X12_sm_psf  = ps1data[i].X12_sm_psf;
    catalog->lensing[i].X22_sm_psf  = ps1data[i].X22_sm_psf;
    catalog->lensing[i].E1_sm_psf   = ps1data[i].E1_sm_psf;
    catalog->lensing[i].E2_sm_psf   = ps1data[i].E2_sm_psf;

    catalog->lensing[i].X11_sh_psf  = ps1data[i].X11_sh_psf;
    catalog->lensing[i].X12_sh_psf  = ps1data[i].X12_sh_psf;
    catalog->lensing[i].X22_sh_psf  = ps1data[i].X22_sh_psf;
    catalog->lensing[i].E1_sh_psf   = ps1data[i].E1_sh_psf;
    catalog->lensing[i].E2_sh_psf   = ps1data[i].E2_sh_psf;

    // catalog->lensing[i].F_ApR5    = ps1data[i].F_ApR5;
    // catalog->lensing[i].dF_ApR5   = ps1data[i].dF_ApR5;
    // catalog->lensing[i].sF_ApR5   = ps1data[i].sF_ApR5;
    // catalog->lensing[i].fF_ApR5   = ps1data[i].fF_ApR5;
    // 
    // catalog->lensing[i].F_ApR6    = ps1data[i].F_ApR6;
    // catalog->lensing[i].dF_ApR6   = ps1data[i].dF_ApR6;
    // catalog->lensing[i].sF_ApR6   = ps1data[i].sF_ApR6;
    // catalog->lensing[i].fF_ApR6   = ps1data[i].fF_ApR6;

    // this is may optionally be replaced by the internal sequence (see FilterStars.c)
    catalog->lensing[i].detID      = ps1data[i].detID; 

    // the Average fields and the following Measure fields are set in FilterStars after
    // the image metadata is in hand:  dR, dD, Mcal, dt, airmass, az, t, imageID, extID.

    // averef is set in find_matches

    // dbFlags is zero on ingest.

    // the following fields are currently not being set anywhere: t_msec
  }    
  return catalog;
}

Catalog *Convert_PS1_V5_R2_Lensing (FTable *table) {

  off_t Nstars; 
  unsigned int i;
  double ZeroPt;
  CMF_PS1_V5_R2_Lensing *ps1data;

  // this code expects the output cmf files to have a fixed column order
  // unfortunatly, psphot is not consistent for the obj lensing values:
  // if the first object in the list is not a valid object for the lensing measurement, then
  // the lensingOBJ structure is not allocated for that source and the corresponding rows
  // are not added to the output metadata until an object which does include the
  // measurement.

  char field[128];
  gfits_scan (table->header, "TTYPE45", "%s", 1, field);

  int mode = 0;
  if (!strcmp (field, "LENS_X11_SM_OBJ")) { mode = 1; }
  if (!strcmp (field, "LENS_X11_SM_PSF")) { mode = 2; }
  myAssert (mode, "invalid table layout\n");

  if (mode == 1) {
    ps1data = gfits_table_get_CMF_PS1_V5_R2_Lensing (table, &Nstars, NULL);
    fprintf (stderr, "PS1_V5_R2_Lensing mode 1\n");
  } 
  if (mode == 2) {
    ps1data = gfits_table_get_CMF_PS1_V5_R2x_Lensing (table, &Nstars, NULL);
    fprintf (stderr, "PS1_V5_R2_Lensing mode 2\n");
  } 

  if (!ps1data) {
    fprintf (stderr, "skipping inconsistent entry\n");
    return (NULL);
  }
  ZeroPt = GetZeroPoint();

  Catalog *catalog = addstar_catalog_init (Nstars);
  ALLOCATE (catalog->lensing, Lensing, Nstars);
  catalog->Nlensing = Nstars;

  for (i = 0; i < Nstars; i++) {
    catalog->measure[i].Xccd       = ps1data[i].X;
    catalog->measure[i].Yccd       = ps1data[i].Y;
    catalog->measure[i].dXccd      = ToShortPixels(ps1data[i].dX);
    catalog->measure[i].dYccd      = ToShortPixels(ps1data[i].dY);

    catalog->measure[i].posangle   = ToShortDegrees(ps1data[i].posangle);
    catalog->measure[i].pltscale   = ps1data[i].pltscale;

    if ((ps1data[i].M >= 0.0) || isnan(ps1data[i].M)) {
      catalog->measure[i].M      = NAN;
    } else {
      catalog->measure[i].M      = ps1data[i].M + ZeroPt;
    }
    catalog->measure[i].dM         = ps1data[i].dM;
    catalog->measure[i].dMcal      = ps1data[i].dMcal;
    catalog->measure[i].Map        = ps1data[i].Map + ZeroPt;
    catalog->measure[i].dMap       = (ps1data[i].apFlux > 0.0) ? fabs(ps1data[i].apFluxErr / ps1data[i].apFlux) : NAN;
                        
    catalog->measure[i].Mkron      = (ps1data[i].kronFlux > 0.0) ? -2.5*log10(ps1data[i].kronFlux) + ZeroPt : NAN;
    catalog->measure[i].dMkron     = (ps1data[i].kronFlux > 0.0) ? ps1data[i].kronFluxErr / ps1data[i].kronFlux : NAN;
                        
    // these fluxes are converted from counts to counts/sec in FilterStars.c
    catalog->measure[i].FluxPSF    = GetFluxFromFluxOrMag (ps1data[i].Flux, ps1data[i].M); 
    catalog->measure[i].dFluxPSF   = GetFluxErrFromFluxOrMag (ps1data[i].dFlux, catalog->measure[i].FluxPSF, ps1data[i].dM);
    catalog->measure[i].FluxKron   = ps1data[i].kronFlux;
    catalog->measure[i].dFluxKron  = ps1data[i].kronFluxErr;
    catalog->measure[i].FluxAp     = GetFluxFromFluxOrMag (ps1data[i].apFlux, ps1data[i].Map); 
    catalog->measure[i].dFluxAp    = GetFluxErrFromFluxOrMag (ps1data[i].apFluxErr, catalog->measure[i].FluxAp, catalog->measure[i].dMap);

    catalog->measure[i].Sky        = ps1data[i].sky;
    catalog->measure[i].dSky       = ps1data[i].dSky;
                        
    catalog->measure[i].psfChisq   = ps1data[i].psfChisq;
    catalog->measure[i].psfQF      = ps1data[i].psfQF;
    catalog->measure[i].psfQFperf  = ps1data[i].psfQFperf;

    catalog->measure[i].psfNdof    = ps1data[i].psfNdof;
    catalog->measure[i].psfNpix    = ps1data[i].psfNpix;
    catalog->measure[i].extNsigma  = ps1data[i].extNsigma;

    catalog->measure[i].FWx        = ToShortPixels(ps1data[i].fx);
    catalog->measure[i].FWy        = ToShortPixels(ps1data[i].fy);
    catalog->measure[i].theta      = ToShortDegrees(ps1data[i].df);

    catalog->measure[i].Mxx        = ToShortPixels(ps1data[i].Mxx);
    catalog->measure[i].Mxy        = ToShortPixels(ps1data[i].Mxy);
    catalog->measure[i].Myy        = ToShortPixels(ps1data[i].Myy);
                        
    catalog->measure[i].photFlags  = ps1data[i].flags;
    catalog->measure[i].photFlags2 = ps1data[i].flags2;

    // this is may optionally be replaced by the internal sequence (see FilterStars.c)
    catalog->measure[i].detID      = ps1data[i].detID; 

    dvo_lensing_init (&catalog->lensing[i]);

    catalog->lensing[i].X11_sm_obj  = ps1data[i].X11_sm_obj;
    catalog->lensing[i].X12_sm_obj  = ps1data[i].X12_sm_obj;
    catalog->lensing[i].X22_sm_obj  = ps1data[i].X22_sm_obj;
    catalog->lensing[i].E1_sm_obj   = ps1data[i].E1_sm_obj;
    catalog->lensing[i].E2_sm_obj   = ps1data[i].E2_sm_obj;

    catalog->lensing[i].X11_sh_obj  = ps1data[i].X11_sh_obj;
    catalog->lensing[i].X12_sh_obj  = ps1data[i].X12_sh_obj;
    catalog->lensing[i].X22_sh_obj  = ps1data[i].X22_sh_obj;
    catalog->lensing[i].E1_sh_obj   = ps1data[i].E1_sh_obj;
    catalog->lensing[i].E2_sh_obj   = ps1data[i].E2_sh_obj;

    catalog->lensing[i].X11_sm_psf  = ps1data[i].X11_sm_psf;
    catalog->lensing[i].X12_sm_psf  = ps1data[i].X12_sm_psf;
    catalog->lensing[i].X22_sm_psf  = ps1data[i].X22_sm_psf;
    catalog->lensing[i].E1_sm_psf   = ps1data[i].E1_sm_psf;
    catalog->lensing[i].E2_sm_psf   = ps1data[i].E2_sm_psf;

    catalog->lensing[i].X11_sh_psf  = ps1data[i].X11_sh_psf;
    catalog->lensing[i].X12_sh_psf  = ps1data[i].X12_sh_psf;
    catalog->lensing[i].X22_sh_psf  = ps1data[i].X22_sh_psf;
    catalog->lensing[i].E1_sh_psf   = ps1data[i].E1_sh_psf;
    catalog->lensing[i].E2_sh_psf   = ps1data[i].E2_sh_psf;

    catalog->lensing[i].E1_psf      = ps1data[i].E1_psf;
    catalog->lensing[i].E2_psf      = ps1data[i].E2_psf;

    // this is may optionally be replaced by the internal sequence (see FilterStars.c)
    catalog->lensing[i].detID       = ps1data[i].detID; 

    // the Average fields and the following Measure fields are set in FilterStars after
    // the image metadata is in hand:  dR, dD, Mcal, dt, airmass, az, t, imageID, extID.

    // averef is set in find_matches

    // dbFlags is zero on ingest.

    // the following fields are currently not being set anywhere: t_msec
  }    
  return catalog;
}

Catalog *Convert_PS1_SV1 (FTable *table) {

  off_t Nstars; 
  unsigned int i;
  double ZeroPt;
  CMF_PS1_SV1 *ps1data;

  if (table[0].header[0].Naxis[0] == 196) {
    Catalog *catalog = Convert_PS1_SV1_Alt (table);
    return catalog;
  }

  ps1data = gfits_table_get_CMF_PS1_SV1 (table, &Nstars, NULL, NULL);
  if (!ps1data) {
    fprintf (stderr, "skipping inconsistent entry\n");
    return (NULL);
  }
  ZeroPt = GetZeroPoint();

  Catalog *catalog = addstar_catalog_init (Nstars);

  for (i = 0; i < Nstars; i++) {
    catalog->measure[i].Xccd       = ps1data[i].X;
    catalog->measure[i].Yccd       = ps1data[i].Y;
    catalog->measure[i].dXccd      = ToShortPixels(ps1data[i].dX);
    catalog->measure[i].dYccd      = ToShortPixels(ps1data[i].dY);

    catalog->measure[i].posangle   = ToShortDegrees(ps1data[i].posangle);
    catalog->measure[i].pltscale   = ps1data[i].pltscale;

    if ((ps1data[i].M >= 0.0) || isnan(ps1data[i].M)) {
      catalog->measure[i].M      = NAN;
    } else {
      catalog->measure[i].M      = ps1data[i].M + ZeroPt;
    }
    catalog->measure[i].dM         = ps1data[i].dM;
    catalog->measure[i].dMcal      = ps1data[i].dMcal;
    catalog->measure[i].Map        = ps1data[i].Map + ZeroPt;
    catalog->measure[i].dMap       = ps1data[i].dM; // a proxy measure
                        
    catalog->measure[i].Mkron      = (ps1data[i].kronFlux > 0.0) ? -2.5*log10(ps1data[i].kronFlux) + ZeroPt : NAN;
    catalog->measure[i].dMkron     = (ps1data[i].kronFlux > 0.0) ? ps1data[i].kronFluxErr / ps1data[i].kronFlux : NAN;

    // these fluxes are converted from counts to counts/sec in FilterStars.c
    catalog->measure[i].FluxPSF    = GetFluxFromFluxOrMag (ps1data[i].Flux, ps1data[i].M); 
    catalog->measure[i].dFluxPSF   = GetFluxErrFromFluxOrMag (ps1data[i].dFlux, catalog->measure[i].FluxPSF, ps1data[i].dM);
    catalog->measure[i].FluxKron   = ps1data[i].kronFlux;
    catalog->measure[i].dFluxKron  = ps1data[i].kronFluxErr;
    catalog->measure[i].FluxAp     = GetFluxFromFluxOrMag (NAN, ps1data[i].Map); 
    catalog->measure[i].dFluxAp    = GetFluxErrFromFluxOrMag (NAN, catalog->measure[i].FluxAp, catalog->measure[i].dMap);

    catalog->measure[i].Sky        = ps1data[i].sky;
    catalog->measure[i].dSky       = ps1data[i].dSky;
                        
    catalog->measure[i].psfChisq   = ps1data[i].psfChisq;
    catalog->measure[i].psfQF      = ps1data[i].psfQF;
    catalog->measure[i].psfQFperf  = ps1data[i].psfQFperf;

    catalog->measure[i].psfNdof    = ps1data[i].psfNdof;
    catalog->measure[i].psfNpix    = ps1data[i].psfNpix;
    catalog->measure[i].extNsigma  = ps1data[i].extNsigma;

    catalog->measure[i].FWx        = ToShortPixels(ps1data[i].fx);
    catalog->measure[i].FWy        = ToShortPixels(ps1data[i].fy);
    catalog->measure[i].theta      = ToShortDegrees(ps1data[i].df);

    catalog->measure[i].Mxx        = ToShortPixels(ps1data[i].Mxx);
    catalog->measure[i].Mxy        = ToShortPixels(ps1data[i].Mxy);
    catalog->measure[i].Myy        = ToShortPixels(ps1data[i].Myy);
                        
    catalog->measure[i].photFlags  = ps1data[i].flags;
    catalog->measure[i].photFlags2 = ps1data[i].flags2;

    // this is may optionally be replaced by the internal sequence (see FilterStars.c)
    catalog->measure[i].detID      = ps1data[i].detID; 

    // the Average fields and the following Measure fields are set in FilterStars after
    // the image metadata is in hand:  dR, dD, Mcal, dt, airmass, az, t, imageID, extID, 
    // averef is set in find_matches, dbFlags is zero on ingest.

    // the following fields are currently not being set anywhere: t_msec
  }    
  return catalog;
}

Catalog *Convert_PS1_SV1_Alt (FTable *table) {

  off_t Nstars; 
  unsigned int i;
  double ZeroPt;
  CMF_PS1_SV1 *ps1data;

  // some test output files were produced called CMF_PS1_SV1 but with mismatch byte boundaries
  ps1data = gfits_table_get_CMF_PS1_SV1_Alt (table, &Nstars, NULL);
  if (!ps1data) {
    fprintf (stderr, "skipping inconsistent entry\n");
    return (NULL);
  }
  ZeroPt = GetZeroPoint();

  Catalog *catalog = addstar_catalog_init (Nstars);

  for (i = 0; i < Nstars; i++) {
    catalog->measure[i].Xccd       = ps1data[i].X;
    catalog->measure[i].Yccd       = ps1data[i].Y;
    catalog->measure[i].dXccd      = ToShortPixels(ps1data[i].dX);
    catalog->measure[i].dYccd      = ToShortPixels(ps1data[i].dY);

    catalog->measure[i].posangle   = ToShortDegrees(ps1data[i].posangle);
    catalog->measure[i].pltscale   = ps1data[i].pltscale;

    if ((ps1data[i].M >= 0.0) || isnan(ps1data[i].M)) {
      catalog->measure[i].M      = NAN;
    } else {
      catalog->measure[i].M      = ps1data[i].M + ZeroPt;
    }
    catalog->measure[i].dM         = ps1data[i].dM;
    catalog->measure[i].dMcal      = ps1data[i].dMcal;
    catalog->measure[i].Map        = ps1data[i].Map + ZeroPt;
    catalog->measure[i].dMap       = ps1data[i].dM; // a proxy measure
                        
    catalog->measure[i].Mkron      = (ps1data[i].kronFlux > 0.0) ? -2.5*log10(ps1data[i].kronFlux) + ZeroPt : NAN;
    catalog->measure[i].dMkron     = (ps1data[i].kronFlux > 0.0) ? ps1data[i].kronFluxErr / ps1data[i].kronFlux : NAN;

    // these fluxes are converted from counts to counts/sec in FilterStars.c
    catalog->measure[i].FluxPSF    = GetFluxFromFluxOrMag (ps1data[i].Flux, ps1data[i].M); 
    catalog->measure[i].dFluxPSF   = GetFluxErrFromFluxOrMag (ps1data[i].dFlux, catalog->measure[i].FluxPSF, ps1data[i].dM);
    catalog->measure[i].FluxKron   = ps1data[i].kronFlux;
    catalog->measure[i].dFluxKron  = ps1data[i].kronFluxErr;
    catalog->measure[i].FluxAp     = GetFluxFromFluxOrMag (NAN, ps1data[i].Map); 
    catalog->measure[i].dFluxAp    = GetFluxErrFromFluxOrMag (NAN, catalog->measure[i].FluxAp, catalog->measure[i].dMap);

    catalog->measure[i].Sky        = ps1data[i].sky;
    catalog->measure[i].dSky       = ps1data[i].dSky;
                        
    catalog->measure[i].psfChisq   = ps1data[i].psfChisq;
    catalog->measure[i].psfQF      = ps1data[i].psfQF;
    catalog->measure[i].psfNdof    = ps1data[i].psfNdof;
    catalog->measure[i].psfNpix    = ps1data[i].psfNpix;
    catalog->measure[i].extNsigma  = ps1data[i].extNsigma;

    catalog->measure[i].FWx        = ToShortPixels(ps1data[i].fx);
    catalog->measure[i].FWy        = ToShortPixels(ps1data[i].fy);
    catalog->measure[i].theta      = ToShortDegrees(ps1data[i].df);

    catalog->measure[i].Mxx        = ToShortPixels(ps1data[i].Mxx);
    catalog->measure[i].Mxy        = ToShortPixels(ps1data[i].Mxy);
    catalog->measure[i].Myy        = ToShortPixels(ps1data[i].Myy);
                        
    catalog->measure[i].photFlags  = ps1data[i].flags;
    catalog->measure[i].photFlags2 = ps1data[i].flags2;

    // this is may optionally be replaced by the internal sequence (see FilterStars.c)
    catalog->measure[i].detID      = ps1data[i].detID; 

    // the Average fields and the following Measure fields are set in FilterStars after
    // the image metadata is in hand:  dR, dD, Mcal, dt, airmass, az, t, imageID, extID, 
    // averef is set in find_matches, dbFlags is zero on ingest.

    // the following fields are currently not being set anywhere: t_msec
  }    
  return catalog;
}

Catalog *Convert_PS1_SV2 (FTable *table) {

  off_t Nstars; 
  unsigned int i;
  double ZeroPt;
  CMF_PS1_SV2 *ps1data;

  ps1data = gfits_table_get_CMF_PS1_SV2 (table, &Nstars, NULL, NULL);
  if (!ps1data) {
    fprintf (stderr, "skipping inconsistent entry\n");
    return (NULL);
  }
  ZeroPt = GetZeroPoint();

  Catalog *catalog = addstar_catalog_init (Nstars);

  for (i = 0; i < Nstars; i++) {
    catalog->measure[i].Xccd       = ps1data[i].X;
    catalog->measure[i].Yccd       = ps1data[i].Y;
    catalog->measure[i].dXccd      = ToShortPixels(ps1data[i].dX);
    catalog->measure[i].dYccd      = ToShortPixels(ps1data[i].dY);

    catalog->measure[i].posangle   = ToShortDegrees(ps1data[i].posangle);
    catalog->measure[i].pltscale   = ps1data[i].pltscale;

    if ((ps1data[i].M >= 0.0) || isnan(ps1data[i].M)) {
      catalog->measure[i].M      = NAN;
    } else {
      catalog->measure[i].M      = ps1data[i].M + ZeroPt;
    }
    catalog->measure[i].dM         = ps1data[i].dM;
    catalog->measure[i].dMcal      = ps1data[i].dMcal;
    catalog->measure[i].Map        = ps1data[i].Map + ZeroPt;
    catalog->measure[i].dMap       = (ps1data[i].apFlux > 0.0) ? fabs(ps1data[i].apFluxErr / ps1data[i].apFlux) : NAN;
                        
    catalog->measure[i].Mkron      = (ps1data[i].kronFlux > 0.0) ? -2.5*log10(ps1data[i].kronFlux) + ZeroPt : NAN;
    catalog->measure[i].dMkron     = (ps1data[i].kronFlux > 0.0) ? ps1data[i].kronFluxErr / ps1data[i].kronFlux : NAN;

    // these fluxes are converted from counts to counts/sec in FilterStars.c
    catalog->measure[i].FluxPSF    = GetFluxFromFluxOrMag (ps1data[i].Flux, ps1data[i].M); 
    catalog->measure[i].dFluxPSF   = GetFluxErrFromFluxOrMag (ps1data[i].dFlux, catalog->measure[i].FluxPSF, ps1data[i].dM);
    catalog->measure[i].FluxKron   = ps1data[i].kronFlux;
    catalog->measure[i].dFluxKron  = ps1data[i].kronFluxErr;
    catalog->measure[i].FluxAp     = GetFluxFromFluxOrMag (ps1data[i].apFlux, ps1data[i].Map); 
    catalog->measure[i].dFluxAp    = GetFluxErrFromFluxOrMag (ps1data[i].apFluxErr, catalog->measure[i].FluxAp, catalog->measure[i].dMap);

    catalog->measure[i].Sky        = ps1data[i].sky;
    catalog->measure[i].dSky       = ps1data[i].dSky;
                        
    catalog->measure[i].psfChisq   = ps1data[i].psfChisq;
    catalog->measure[i].psfQF      = ps1data[i].psfQF;
    catalog->measure[i].psfQFperf  = ps1data[i].psfQFperf;

    catalog->measure[i].psfNdof    = ps1data[i].psfNdof;
    catalog->measure[i].psfNpix    = ps1data[i].psfNpix;
    catalog->measure[i].extNsigma  = ps1data[i].extNsigma;

    catalog->measure[i].FWx        = ToShortPixels(ps1data[i].fx);
    catalog->measure[i].FWy        = ToShortPixels(ps1data[i].fy);
    catalog->measure[i].theta      = ToShortDegrees(ps1data[i].df);

    catalog->measure[i].Mxx        = ToShortPixels(ps1data[i].Mxx);
    catalog->measure[i].Mxy        = ToShortPixels(ps1data[i].Mxy);
    catalog->measure[i].Myy        = ToShortPixels(ps1data[i].Myy);
                        
    catalog->measure[i].photFlags  = ps1data[i].flags;
    catalog->measure[i].photFlags2 = ps1data[i].flags2;

    // this is may optionally be replaced by the internal sequence (see FilterStars.c)
    catalog->measure[i].detID      = ps1data[i].detID; 

    // the Average fields and the following Measure fields are set in FilterStars after
    // the image metadata is in hand:  dR, dD, Mcal, dt, airmass, az, t, imageID, extID, 
    // averef is set in find_matches, dbFlags is zero on ingest.

    // the following fields are currently not being set anywhere: t_msec
  }    
  return catalog;
}

Catalog *Convert_PS1_SV3 (FTable *table) {

  off_t Nstars; 
  unsigned int i;
  double ZeroPt;
  CMF_PS1_SV3 *ps1data;

  ps1data = gfits_table_get_CMF_PS1_SV3 (table, &Nstars, NULL);
  if (!ps1data) {
    fprintf (stderr, "skipping inconsistent entry\n");
    return (NULL);
  }
  ZeroPt = GetZeroPoint();

  Catalog *catalog = addstar_catalog_init (Nstars);

  for (i = 0; i < Nstars; i++) {
    catalog->measure[i].Xccd       = ps1data[i].X;
    catalog->measure[i].Yccd       = ps1data[i].Y;
    catalog->measure[i].dXccd      = ToShortPixels(ps1data[i].dX);
    catalog->measure[i].dYccd      = ToShortPixels(ps1data[i].dY);

    catalog->measure[i].posangle   = ToShortDegrees(ps1data[i].posangle);
    catalog->measure[i].pltscale   = ps1data[i].pltscale;

    if ((ps1data[i].M >= 0.0) || isnan(ps1data[i].M)) {
      catalog->measure[i].M      = NAN;
    } else {
      catalog->measure[i].M      = ps1data[i].M + ZeroPt;
    }
    catalog->measure[i].dM         = ps1data[i].dM;
    catalog->measure[i].dMcal      = ps1data[i].dMcal;
    catalog->measure[i].Map        = ps1data[i].Map + ZeroPt;
    catalog->measure[i].dMap       = (ps1data[i].apFlux > 0.0) ? fabs(ps1data[i].apFluxErr / ps1data[i].apFlux) : NAN;
                        
    catalog->measure[i].Mkron      = (ps1data[i].kronFlux > 0.0) ? -2.5*log10(ps1data[i].kronFlux) + ZeroPt : NAN;
    catalog->measure[i].dMkron     = (ps1data[i].kronFlux > 0.0) ? ps1data[i].kronFluxErr / ps1data[i].kronFlux : NAN;

    // these fluxes are converted from counts to counts/sec in FilterStars.c
    catalog->measure[i].FluxPSF    = GetFluxFromFluxOrMag (ps1data[i].Flux, ps1data[i].M); 
    catalog->measure[i].dFluxPSF   = GetFluxErrFromFluxOrMag (ps1data[i].dFlux, catalog->measure[i].FluxPSF, ps1data[i].dM);
    catalog->measure[i].FluxKron   = ps1data[i].kronFlux;
    catalog->measure[i].dFluxKron  = ps1data[i].kronFluxErr;
    catalog->measure[i].FluxAp     = GetFluxFromFluxOrMag (ps1data[i].apFlux, ps1data[i].Map); 
    catalog->measure[i].dFluxAp    = GetFluxErrFromFluxOrMag (ps1data[i].apFluxErr, catalog->measure[i].FluxAp, catalog->measure[i].dMap);

    catalog->measure[i].Sky        = ps1data[i].sky;
    catalog->measure[i].dSky       = ps1data[i].dSky;
                        
    catalog->measure[i].psfChisq   = ps1data[i].psfChisq;
    catalog->measure[i].psfQF      = ps1data[i].psfQF;
    catalog->measure[i].psfQFperf  = ps1data[i].psfQFperf;

    catalog->measure[i].psfNdof    = ps1data[i].psfNdof;
    catalog->measure[i].psfNpix    = ps1data[i].psfNpix;
    catalog->measure[i].extNsigma  = ps1data[i].extNsigma;

    catalog->measure[i].FWx        = ToShortPixels(ps1data[i].fx);
    catalog->measure[i].FWy        = ToShortPixels(ps1data[i].fy);
    catalog->measure[i].theta      = ToShortDegrees(ps1data[i].df);

    catalog->measure[i].Mxx        = ToShortPixels(ps1data[i].Mxx);
    catalog->measure[i].Mxy        = ToShortPixels(ps1data[i].Mxy);
    catalog->measure[i].Myy        = ToShortPixels(ps1data[i].Myy);
                        
    catalog->measure[i].photFlags  = ps1data[i].flags;
    catalog->measure[i].photFlags2 = ps1data[i].flags2;

    // this is may optionally be replaced by the internal sequence (see FilterStars.c)
    catalog->measure[i].detID      = ps1data[i].detID; 

    // the Average fields and the following Measure fields are set in FilterStars after
    // the image metadata is in hand:  dR, dD, Mcal, dt, airmass, az, t, imageID, extID, 
    // averef is set in find_matches, dbFlags is zero on ingest.

    // the following fields are currently not being set anywhere: t_msec
  }    
  return catalog;
}

Catalog *Convert_PS1_SV4 (FTable *table) {

  off_t Nstars; 
  unsigned int i;
  double ZeroPt;
  CMF_PS1_SV4 *ps1data;

  ps1data = gfits_table_get_CMF_PS1_SV4 (table, &Nstars, NULL);
  if (!ps1data) {
    fprintf (stderr, "skipping inconsistent entry\n");
    return (NULL);
  }
  ZeroPt = GetZeroPoint();

  Catalog *catalog = addstar_catalog_init (Nstars);

  for (i = 0; i < Nstars; i++) {
    catalog->measure[i].Xccd       = ps1data[i].X;
    catalog->measure[i].Yccd       = ps1data[i].Y;
    catalog->measure[i].dXccd      = ToShortPixels(ps1data[i].dX);
    catalog->measure[i].dYccd      = ToShortPixels(ps1data[i].dY);

    catalog->measure[i].posangle   = ToShortDegrees(ps1data[i].posangle);
    catalog->measure[i].pltscale   = ps1data[i].pltscale;

    if ((ps1data[i].M >= 0.0) || isnan(ps1data[i].M)) {
      catalog->measure[i].M      = NAN;
    } else {
      catalog->measure[i].M      = ps1data[i].M + ZeroPt;
    }
    catalog->measure[i].dM         = ps1data[i].dM;
    catalog->measure[i].dMcal      = ps1data[i].dMcal;
    catalog->measure[i].Map        = ps1data[i].Map + ZeroPt;
    catalog->measure[i].dMap       = (ps1data[i].apFlux > 0.0) ? fabs(ps1data[i].apFluxErr / ps1data[i].apFlux) : NAN;
                        
    catalog->measure[i].Mkron      = (ps1data[i].kronFlux > 0.0) ? -2.5*log10(ps1data[i].kronFlux) + ZeroPt : NAN;
    catalog->measure[i].dMkron     = (ps1data[i].kronFlux > 0.0) ? ps1data[i].kronFluxErr / ps1data[i].kronFlux : NAN;

    // these fluxes are converted from counts to counts/sec in FilterStars.c
    catalog->measure[i].FluxPSF    = GetFluxFromFluxOrMag (ps1data[i].Flux, ps1data[i].M); 
    catalog->measure[i].dFluxPSF   = GetFluxErrFromFluxOrMag (ps1data[i].dFlux, catalog->measure[i].FluxPSF, ps1data[i].dM);
    catalog->measure[i].FluxKron   = ps1data[i].kronFlux;
    catalog->measure[i].dFluxKron  = ps1data[i].kronFluxErr;
    catalog->measure[i].FluxAp     = GetFluxFromFluxOrMag (ps1data[i].apFlux, ps1data[i].Map); 
    catalog->measure[i].dFluxAp    = GetFluxErrFromFluxOrMag (ps1data[i].apFluxErr, catalog->measure[i].FluxAp, catalog->measure[i].dMap);

    catalog->measure[i].Sky        = ps1data[i].sky;
    catalog->measure[i].dSky       = ps1data[i].dSky;
                        
    catalog->measure[i].psfChisq   = ps1data[i].psfChisq;
    catalog->measure[i].psfQF      = ps1data[i].psfQF;
    catalog->measure[i].psfQFperf  = ps1data[i].psfQFperf;

    catalog->measure[i].psfNdof    = ps1data[i].psfNdof;
    catalog->measure[i].psfNpix    = ps1data[i].psfNpix;
    catalog->measure[i].extNsigma  = ps1data[i].extNsigma;

    catalog->measure[i].FWx        = ToShortPixels(ps1data[i].fx);
    catalog->measure[i].FWy        = ToShortPixels(ps1data[i].fy);
    catalog->measure[i].theta      = ToShortDegrees(ps1data[i].df);

    catalog->measure[i].Mxx        = ToShortPixels(ps1data[i].Mxx);
    catalog->measure[i].Mxy        = ToShortPixels(ps1data[i].Mxy);
    catalog->measure[i].Myy        = ToShortPixels(ps1data[i].Myy);
                        
    catalog->measure[i].photFlags  = ps1data[i].flags;
    catalog->measure[i].photFlags2 = ps1data[i].flags2;

    // this is may optionally be replaced by the internal sequence (see FilterStars.c)
    catalog->measure[i].detID      = ps1data[i].detID; 

    // the Average fields and the following Measure fields are set in FilterStars after
    // the image metadata is in hand:  dR, dD, Mcal, dt, airmass, az, t, imageID, extID, 
    // averef is set in find_matches, dbFlags is zero on ingest.

    // the following fields are currently not being set anywhere: t_msec
  }    
  return catalog;
}

Catalog *Convert_PS1_DV3 (FTable *table) {

  off_t Nstars; 
  unsigned int i;
  double ZeroPt;
  CMF_PS1_DV3 *ps1data;

  ps1data = gfits_table_get_CMF_PS1_DV3 (table, &Nstars, NULL);
  if (!ps1data) {
    fprintf (stderr, "skipping inconsistent entry\n");
    return (NULL);
  }
  ZeroPt = GetZeroPoint();

  Catalog *catalog = addstar_catalog_init (Nstars);

  for (i = 0; i < Nstars; i++) {
    catalog->measure[i].Xccd       = ps1data[i].X;
    catalog->measure[i].Yccd       = ps1data[i].Y;
    catalog->measure[i].dXccd      = ToShortPixels(ps1data[i].dX);
    catalog->measure[i].dYccd      = ToShortPixels(ps1data[i].dY);

    catalog->measure[i].posangle   = ToShortDegrees(ps1data[i].posangle);
    catalog->measure[i].pltscale   = ps1data[i].pltscale;

    if ((ps1data[i].M >= 0.0) || isnan(ps1data[i].M)) {
      catalog->measure[i].M      = NAN;
    } else {
      catalog->measure[i].M      = ps1data[i].M + ZeroPt;
    }
    catalog->measure[i].dM         = ps1data[i].dM;
    catalog->measure[i].dMcal      = ps1data[i].dMcal;
    catalog->measure[i].Map        = ps1data[i].Map + ZeroPt;
    catalog->measure[i].dMap       = (ps1data[i].apFlux > 0.0) ? fabs(ps1data[i].apFluxErr / ps1data[i].apFlux) : NAN;
                        
    catalog->measure[i].Mkron      = (ps1data[i].kronFlux > 0.0) ? -2.5*log10(ps1data[i].kronFlux) + ZeroPt : NAN;
    catalog->measure[i].dMkron     = (ps1data[i].kronFlux > 0.0) ? ps1data[i].kronFluxErr / ps1data[i].kronFlux : NAN;
                        
    // these fluxes are converted from counts to counts/sec in FilterStars.c
    catalog->measure[i].FluxPSF    = GetFluxFromFluxOrMag (ps1data[i].Flux, ps1data[i].M); 
    catalog->measure[i].dFluxPSF   = GetFluxErrFromFluxOrMag (ps1data[i].dFlux, catalog->measure[i].FluxPSF, ps1data[i].dM);
    catalog->measure[i].FluxKron   = ps1data[i].kronFlux;
    catalog->measure[i].dFluxKron  = ps1data[i].kronFluxErr;
    catalog->measure[i].FluxAp     = GetFluxFromFluxOrMag (ps1data[i].apFlux, ps1data[i].Map); 
    catalog->measure[i].dFluxAp    = GetFluxErrFromFluxOrMag (ps1data[i].apFluxErr, catalog->measure[i].FluxAp, catalog->measure[i].dMap);

    catalog->measure[i].Sky        = ps1data[i].sky;
    catalog->measure[i].dSky       = ps1data[i].dSky;
                        
    catalog->measure[i].psfChisq   = ps1data[i].psfChisq;
    catalog->measure[i].psfQF      = ps1data[i].psfQF;
    catalog->measure[i].psfQFperf  = ps1data[i].psfQFperf;
    catalog->measure[i].psfNdof    = ps1data[i].psfNdof;
    catalog->measure[i].psfNpix    = ps1data[i].psfNpix;
    catalog->measure[i].extNsigma  = ps1data[i].extNsigma;

    catalog->measure[i].FWx        = ToShortPixels(ps1data[i].fx);
    catalog->measure[i].FWy        = ToShortPixels(ps1data[i].fy);
    catalog->measure[i].theta      = ToShortDegrees(ps1data[i].df);

    catalog->measure[i].Mxx        = ToShortPixels(ps1data[i].Mxx);
    catalog->measure[i].Mxy        = ToShortPixels(ps1data[i].Mxy);
    catalog->measure[i].Myy        = ToShortPixels(ps1data[i].Myy);
                        
    catalog->measure[i].photFlags  = ps1data[i].flags;
    catalog->measure[i].photFlags2 = ps1data[i].flags2;

    // this is may optionally be replaced by the internal sequence (see FilterStars.c)
    catalog->measure[i].detID      = ps1data[i].detID; 

    // the Average fields and the following Measure fields are set in FilterStars after
    // the image metadata is in hand:  dR, dD, Mcal, dt, airmass, az, t, imageID, extID.

    // averef is set in find_matches

    // dbFlags is zero on ingest.

    // the following fields are currently not being set anywhere: t_msec
  }    
  return catalog;
}

Catalog *Convert_PS1_DV4 (FTable *table) {

  off_t Nstars; 
  unsigned int i;
  double ZeroPt;
  CMF_PS1_DV4 *ps1data;

  ps1data = gfits_table_get_CMF_PS1_DV4 (table, &Nstars, NULL, NULL);
  if (!ps1data) {
    fprintf (stderr, "skipping inconsistent entry\n");
    return (NULL);
  }
  ZeroPt = GetZeroPoint();

  fprintf (stderr, "WARNING: Convert_PS1_DV4 not yet updated to match real format\n");

  Catalog *catalog = addstar_catalog_init (Nstars);

  for (i = 0; i < Nstars; i++) {
    catalog->measure[i].Xccd       = ps1data[i].X;
    catalog->measure[i].Yccd       = ps1data[i].Y;
    catalog->measure[i].dXccd      = ToShortPixels(ps1data[i].dX);
    catalog->measure[i].dYccd      = ToShortPixels(ps1data[i].dY);

    catalog->measure[i].posangle   = ToShortDegrees(ps1data[i].posangle);
    catalog->measure[i].pltscale   = ps1data[i].pltscale;

    if ((ps1data[i].M >= 0.0) || isnan(ps1data[i].M)) {
      catalog->measure[i].M      = NAN;
    } else {
      catalog->measure[i].M      = ps1data[i].M + ZeroPt;
    }
    catalog->measure[i].dM         = ps1data[i].dM;
    catalog->measure[i].dMcal      = ps1data[i].dMcal;
    catalog->measure[i].Map        = ps1data[i].Map + ZeroPt;
    catalog->measure[i].dMap       = (ps1data[i].apFlux > 0.0) ? fabs(ps1data[i].apFluxErr / ps1data[i].apFlux) : NAN;
                        
    catalog->measure[i].Mkron      = (ps1data[i].kronFlux > 0.0) ? -2.5*log10(ps1data[i].kronFlux) + ZeroPt : NAN;
    catalog->measure[i].dMkron     = (ps1data[i].kronFlux > 0.0) ? ps1data[i].kronFluxErr / ps1data[i].kronFlux : NAN;
                        
    // these fluxes are converted from counts to counts/sec in FilterStars.c
    catalog->measure[i].FluxPSF    = GetFluxFromFluxOrMag (ps1data[i].Flux, ps1data[i].M); 
    catalog->measure[i].dFluxPSF   = GetFluxErrFromFluxOrMag (ps1data[i].dFlux, catalog->measure[i].FluxPSF, ps1data[i].dM);
    catalog->measure[i].FluxKron   = ps1data[i].kronFlux;
    catalog->measure[i].dFluxKron  = ps1data[i].kronFluxErr;
    catalog->measure[i].FluxAp     = GetFluxFromFluxOrMag (ps1data[i].apFlux, ps1data[i].Map); 
    catalog->measure[i].dFluxAp    = GetFluxErrFromFluxOrMag (ps1data[i].apFluxErr, catalog->measure[i].FluxAp, catalog->measure[i].dMap);

    catalog->measure[i].Sky        = ps1data[i].sky;
    catalog->measure[i].dSky       = ps1data[i].dSky;
                        
    catalog->measure[i].psfChisq   = ps1data[i].psfChisq;
    catalog->measure[i].psfQF      = ps1data[i].psfQF;
    catalog->measure[i].psfQFperf  = ps1data[i].psfQFperf;
    catalog->measure[i].psfNdof    = ps1data[i].psfNdof;
    catalog->measure[i].psfNpix    = ps1data[i].psfNpix;
    catalog->measure[i].extNsigma  = ps1data[i].extNsigma;

    catalog->measure[i].FWx        = ToShortPixels(ps1data[i].fx);
    catalog->measure[i].FWy        = ToShortPixels(ps1data[i].fy);
    catalog->measure[i].theta      = ToShortDegrees(ps1data[i].df);

    catalog->measure[i].Mxx        = ToShortPixels(ps1data[i].Mxx);
    catalog->measure[i].Mxy        = ToShortPixels(ps1data[i].Mxy);
    catalog->measure[i].Myy        = ToShortPixels(ps1data[i].Myy);
                        
    catalog->measure[i].photFlags  = ps1data[i].flags;
    catalog->measure[i].photFlags2 = ps1data[i].flags2;

    // this is may optionally be replaced by the internal sequence (see FilterStars.c)
    catalog->measure[i].detID      = ps1data[i].detID; 

    // the Average fields and the following Measure fields are set in FilterStars after
    // the image metadata is in hand:  dR, dD, Mcal, dt, airmass, az, t, imageID, extID.

    // averef is set in find_matches

    // dbFlags is zero on ingest.

    // the following fields are currently not being set anywhere: t_msec
  }    
  return catalog;
}

Catalog *Convert_PS1_DV5 (FTable *table) {

  off_t Nstars; 
  unsigned int i;
  double ZeroPt;
  CMF_PS1_DV5 *ps1data;

  ps1data = gfits_table_get_CMF_PS1_DV5 (table, &Nstars, NULL, NULL);
  if (!ps1data) {
    fprintf (stderr, "skipping inconsistent entry\n");
    return (NULL);
  }
  ZeroPt = GetZeroPoint();

  Catalog *catalog = addstar_catalog_init (Nstars);

  for (i = 0; i < Nstars; i++) {
    catalog->measure[i].Xccd       = ps1data[i].X;
    catalog->measure[i].Yccd       = ps1data[i].Y;
    catalog->measure[i].dXccd      = ToShortPixels(ps1data[i].dX);
    catalog->measure[i].dYccd      = ToShortPixels(ps1data[i].dY);

    catalog->measure[i].posangle   = ToShortDegrees(ps1data[i].posangle);
    catalog->measure[i].pltscale   = ps1data[i].pltscale;

    if ((ps1data[i].M >= 0.0) || isnan(ps1data[i].M)) {
      catalog->measure[i].M      = NAN;
    } else {
      catalog->measure[i].M      = ps1data[i].M + ZeroPt;
    }
    catalog->measure[i].dM         = ps1data[i].dM;
    catalog->measure[i].dMcal      = ps1data[i].dMcal;
    catalog->measure[i].Map        = ps1data[i].Map + ZeroPt;
    catalog->measure[i].dMap       = (ps1data[i].apFlux > 0.0) ? fabs(ps1data[i].apFluxErr / ps1data[i].apFlux) : NAN;
                        
    catalog->measure[i].Mkron      = (ps1data[i].kronFlux > 0.0) ? -2.5*log10(ps1data[i].kronFlux) + ZeroPt : NAN;
    catalog->measure[i].dMkron     = (ps1data[i].kronFlux > 0.0) ? ps1data[i].kronFluxErr / ps1data[i].kronFlux : NAN;
                        
    // these fluxes are converted from counts to counts/sec in FilterStars.c
    catalog->measure[i].FluxPSF    = GetFluxFromFluxOrMag (ps1data[i].Flux, ps1data[i].M); 
    catalog->measure[i].dFluxPSF   = GetFluxErrFromFluxOrMag (ps1data[i].dFlux, catalog->measure[i].FluxPSF, ps1data[i].dM);
    catalog->measure[i].FluxKron   = ps1data[i].kronFlux;
    catalog->measure[i].dFluxKron  = ps1data[i].kronFluxErr;
    catalog->measure[i].FluxAp     = GetFluxFromFluxOrMag (ps1data[i].apFlux, ps1data[i].Map); 
    catalog->measure[i].dFluxAp    = GetFluxErrFromFluxOrMag (ps1data[i].apFluxErr, catalog->measure[i].FluxAp, catalog->measure[i].dMap);

    catalog->measure[i].Sky        = ps1data[i].sky;
    catalog->measure[i].dSky       = ps1data[i].dSky;
                        
    catalog->measure[i].psfChisq   = ps1data[i].psfChisq;
    catalog->measure[i].psfQF      = ps1data[i].psfQF;
    catalog->measure[i].psfQFperf  = ps1data[i].psfQFperf;
    catalog->measure[i].psfNdof    = ps1data[i].psfNdof;
    catalog->measure[i].psfNpix    = ps1data[i].psfNpix;
    catalog->measure[i].extNsigma  = ps1data[i].extNsigma;

    catalog->measure[i].FWx        = ToShortPixels(ps1data[i].fx);
    catalog->measure[i].FWy        = ToShortPixels(ps1data[i].fy);
    catalog->measure[i].theta      = ToShortDegrees(ps1data[i].df);

    catalog->measure[i].Mxx        = ToShortPixels(ps1data[i].Mxx);
    catalog->measure[i].Mxy        = ToShortPixels(ps1data[i].Mxy);
    catalog->measure[i].Myy        = ToShortPixels(ps1data[i].Myy);
                        
    catalog->measure[i].photFlags  = ps1data[i].flags;
    catalog->measure[i].photFlags2 = ps1data[i].flags2;

    // this is may optionally be replaced by the internal sequence (see FilterStars.c)
    catalog->measure[i].detID      = ps1data[i].detID; 

    // the Average fields and the following Measure fields are set in FilterStars after
    // the image metadata is in hand:  dR, dD, Mcal, dt, airmass, az, t, imageID, extID.

    // averef is set in find_matches

    // dbFlags is zero on ingest.

    // the following fields are currently not being set anywhere: t_msec
  }    
  return catalog;
}
