# include "relphot.h"

// STATUS is value expected for success
# define CHECK_STATUS(STATUS,MSG,...)					\
  if (!(STATUS)) {							\
    fprintf (stderr, MSG, __VA_ARGS__);					\
    return FALSE;							\
  }

typedef struct {
  float  Mresid;
  float dMresid;
  float  Mchisq;
  float  Mmedian;
  float  Maltsig;
  int    Nfit;
} MagResidResult;

MagResidResult MagResidCalc (Catalog *catalog, Image *image, FitDataSet *psfStars, int Nsecfilt, int N_onImage, IDX_T *ImageToMeasure, IDX_T *ImageToCatalog);

off_t *get_N_onImage();
IDX_T **get_ImageToCatalog();
IDX_T **get_ImageToMeasure();

int MagResidSave(char *filename, Catalog *catalog) {

  int i;
  Header header;
  Header theader;
  Matrix matrix;
  FTable ftable;

  off_t Nimage;
  Image *image = getimages (&Nimage, NULL);
  off_t *N_onImage = get_N_onImage();

  // find the maximum number of stars (needed to allocate the psfStars structure)
  off_t Nmax = 0;
  for (i = 0; i < Nimage; i++) {
    Nmax = MAX (Nmax, N_onImage[i]);
  }

  // we are making a 0-order fit and not doing bootstrap analysis
  FitDataSet psfStars;
  FitDataSetAlloc (&psfStars, Nmax, 0, 0);
  // By default, we use IRLS; if MaxIterations = 0, then we use OLS
  // psfStars.MaxIterations = 0;

  gfits_init_header (&header);
  header.extend = TRUE;
  gfits_create_header (&header);
  gfits_create_matrix (&header, &matrix);

  gfits_create_table_header (&theader, "BINTABLE", "MAG_PSF_RESID");

  gfits_define_bintable_column (&theader, "D", "RA",         "ra",                           "degree",     1.0, 0.0);
  gfits_define_bintable_column (&theader, "D", "DEC",        "dec",                          "degree",     1.0, 0.0);
  gfits_define_bintable_column (&theader, "E", "MAG_RESID",  "magnitude residual",           "magnitudes", 1.0, 0.0);
  gfits_define_bintable_column (&theader, "E", "MAG_ERROR",  "magnitude error",              "magnitudes", 1.0, 0.0);
  gfits_define_bintable_column (&theader, "E", "MAG_CHISQ",  "magnitude resid chisq",        "unitless",   1.0, 0.0);
  gfits_define_bintable_column (&theader, "E", "MAG_MEDIAN", "median magnitude residual",    "magnitudes", 1.0, 0.0);
  gfits_define_bintable_column (&theader, "E", "MAG_ALTSIG", "68 percentile range as sigma", "magnitudes", 1.0, 0.0);
  gfits_define_bintable_column (&theader, "E", "AIRMASS",    "",                             "unitless",   1.0, 0.0);
  gfits_define_bintable_column (&theader, "D", "MJD",        "",                             "unitless",   1.0, 0.0);
  gfits_define_bintable_column (&theader, "E", "EXPTIME",    "",                             "unitless",   1.0, 0.0);
  gfits_define_bintable_column (&theader, "J", "PHOTCODE",   "",                             "unitless",   1.0, 0.0);
  gfits_define_bintable_column (&theader, "J", "FLAGS",      "image flags",                  "unitless",   1.0, 0.0);
  gfits_define_bintable_column (&theader, "J", "NFIT",       "number of stars fitted",       NULL,         1.0, 0.0);
  gfits_define_bintable_column (&theader, "J", "NMEAS",      "number of stars on image",     NULL,         1.0, 0.0);

  // generate the output array that carries the data
  gfits_create_table (&theader, &ftable);

  // create intermediate storage arrays
  ALLOCATE_PTR (Rs,       double, Nimage);
  ALLOCATE_PTR (Ds,       double, Nimage);

  ALLOCATE_PTR (Mresid,   float,  Nimage);
  ALLOCATE_PTR (dMresid,  float,  Nimage);
  ALLOCATE_PTR (Mchisq,   float,  Nimage);
  ALLOCATE_PTR (Mmedian,  float,  Nimage);
  ALLOCATE_PTR (Maltsig,  float,  Nimage);
  ALLOCATE_PTR (airmass,  float,  Nimage);
  ALLOCATE_PTR (mjd,      double, Nimage);
  ALLOCATE_PTR (exptime,  float,  Nimage);
  ALLOCATE_PTR (photcode, int,    Nimage);
  ALLOCATE_PTR (flags,    int,    Nimage);
  ALLOCATE_PTR (Nfit,     int,    Nimage);
  ALLOCATE_PTR (Nmeas,    int,    Nimage);

  // XXXX fill in the vectors here
  int Nsecfilt = GetPhotcodeNsecfilt ();
  IDX_T **ImageToCatalog = get_ImageToCatalog();
  IDX_T **ImageToMeasure = get_ImageToMeasure();

  for (i = 0; i < Nimage; i++) {
    double Ro, Do;
    XY_to_RD (&Ro, &Do, (double) 0.5*image[i].NX, (double) 0.5*image[i].NY, &image[i].coords);

    Rs[i]       = Ro;
    Ds[i]       = Do;
    Mresid[i]   = NAN;
    Mchisq[i]   = NAN;
    dMresid[i]  = NAN;
    Mmedian[i]  = NAN;
    Maltsig[i]  = NAN;
    Nfit[i]     = 0;
    Nmeas[i]    = N_onImage[i];
    airmass[i]  = image[i].secz;
    mjd[i]      = ohana_sec_to_mjd (image[i].tzero);
    exptime[i]  = image[i].exptime;
    photcode[i] = image[i].photcode;
    flags[i]    = image[i].flags;

    if (image[i].photcode == 0) continue; // skip the PHU images
    if (N_onImage[i] == 0) continue;

    MagResidResult result = MagResidCalc (catalog, &image[i], &psfStars, Nsecfilt, N_onImage[i], ImageToMeasure[i], ImageToCatalog[i]);
    
    Mresid[i]   = result.Mresid;
    Mchisq[i]   = result.Mchisq;
    dMresid[i]  = result.dMresid;
    Mmedian[i]  = result.Mmedian;
    Maltsig[i]  = result.Maltsig;
    Nfit[i]     = result.Nfit;
  }

  // add the columns to the output array
  gfits_set_bintable_column (&theader, &ftable, "RA",        Rs,       Nimage);
  gfits_set_bintable_column (&theader, &ftable, "DEC",       Ds,       Nimage);
  gfits_set_bintable_column (&theader, &ftable, "MAG_RESID", Mresid,   Nimage);
  gfits_set_bintable_column (&theader, &ftable, "MAG_ERROR", dMresid,  Nimage);
  gfits_set_bintable_column (&theader, &ftable, "MAG_CHISQ", Mchisq,   Nimage);
  gfits_set_bintable_column (&theader, &ftable, "MAG_MEDIAN", Mmedian, Nimage);
  gfits_set_bintable_column (&theader, &ftable, "MAG_ALTSIG", Maltsig, Nimage);
  gfits_set_bintable_column (&theader, &ftable, "AIRMASS",   airmass,  Nimage);
  gfits_set_bintable_column (&theader, &ftable, "MJD",       mjd,      Nimage);
  gfits_set_bintable_column (&theader, &ftable, "EXPTIME",   exptime,  Nimage);
  gfits_set_bintable_column (&theader, &ftable, "PHOTCODE",  photcode, Nimage);
  gfits_set_bintable_column (&theader, &ftable, "FLAGS",     flags,    Nimage);
  gfits_set_bintable_column (&theader, &ftable, "NFIT",      Nfit,     Nimage);
  gfits_set_bintable_column (&theader, &ftable, "NMEAS",     Nmeas,    Nimage);

  free (Rs);
  free (Ds);
  free (Mresid  );
  free (dMresid );
  free (Mchisq  );
  free (Mmedian );
  free (Maltsig );
  free (airmass );
  free (mjd     );
  free (exptime );
  free (photcode);
  free (flags   );
  free (Nfit    );
  free (Nmeas   );

  FILE *f = fopen (filename, "w");
  if (!f) {
    fprintf (stderr, "ERROR: cannot open meanmag file for output %s\n", filename);
    return FALSE;
  }

  int status;
  status = gfits_fwrite_header  (f, &header);
  CHECK_STATUS (status, "ERROR: cannot write header for meanmags %s\n", filename);

  status = gfits_fwrite_matrix  (f, &matrix);
  CHECK_STATUS (status, "ERROR: cannot write matrix for meanmags %s\n", filename);

  status = gfits_fwrite_Theader (f, &theader);
  CHECK_STATUS (status, "ERROR: cannot write table header for meanmags %s\n", filename);

  status = gfits_fwrite_table  (f, &ftable);
  CHECK_STATUS (status, "ERROR: cannot write table data for meanmags %s\n", filename);

  gfits_free_header (&header);
  gfits_free_matrix (&matrix);
  gfits_free_header (&theader);
  gfits_free_table (&ftable);

  int fd = fileno (f);

  status = fflush (f);
  CHECK_STATUS (!status, "ERROR: cannot flush file meanmags %s\n", filename);

  status = fsync (fd);
  CHECK_STATUS (!status, "ERROR: cannot flush file meanmags %s\n", filename);

  status = fclose (f);
  CHECK_STATUS (!status, "ERROR: problem closing meanmags file %s\n", filename);

  FitDataSetFree (&psfStars);

  return TRUE;
}

// determine Mag residual for this image
MagResidResult MagResidCalc (Catalog *catalog, Image *image, FitDataSet *psfStars, int Nsecfilt, int N_onImage, IDX_T *ImageToMeasure, IDX_T *ImageToCatalog) {

  off_t Nref = 0;    // number of stars used to measure McalPSF

  MagResidResult result;
  result.Mresid  = NAN;
  result.dMresid = NAN;
  result.Mchisq  = NAN;
  result.Mmedian = NAN;
  result.Maltsig = NAN;
  result.Nfit    = 0;

  for (off_t j = 0; j < N_onImage; j++) {
      
    off_t m = ImageToMeasure[j];
    off_t c = ImageToCatalog[j];
      
    if (catalog[c].measureT[m].dbFlags & MEAS_BAD) continue;

    float Mcal  = getMcal  (m, c, MAG_CLASS_PSF);
    if (isnan(Mcal)) continue;

    float Mmos  = getMmos  (m, c);
    if (isnan(Mmos)) continue;

    float Mgrp  = getMgrp  (m, c, catalog[c].measureT[m].airmass, NULL); // ignore error for now?
    if (isnan(Mgrp)) continue;

    float Mgrid = getMgridTiny (&catalog[c].measureT[m]);
    if (isnan(Mgrid)) continue;

    // image.Mcal is not supposed to include the flat-field correction, so we need to
    // apply that offset as well here for this image (in other words, each detection is
    // being compared to the model, excluding the zero point, Mcal.  The model includes
    // the flat-correction.  NOTE the sign of Mflat (Image.Mcal = Measure.Mcal + Mflat)

    float Mflat = getMflat (m, c, catalog);

    // MrelPSF is the average magnitude for this star.  
    float MrelPSF  = getMrel  (catalog, m, c, MAG_CLASS_PSF, MAG_SRC_CHP);
    if (isnan(MrelPSF)) continue;
      
    // get the PSF magnitude for thie measurement, with airmass slope applied
    off_t n = catalog[c].measureT[m].averef;

    // MsysPSF is the specific measurement for this star on this image
    float MsysPSF = PhotSysTiny (&catalog[c].measureT[m], &catalog[c].averageT[n], &catalog[c].secfilt[n*Nsecfilt], MAG_CLASS_PSF);
    if (isnan(MsysPSF)) continue;

    float Moff = Mcal + Mmos + Mgrp + Mgrid + Mflat;

    // Mrel is the true average apparent magnitude of this star
    // Msys is the observed apparent magnitude, with nominal corrections, ie, the instrumental magnitude plus a constant
    // yVector (dMag) = Msys - Mrel - Moff
    // as the clouds come and go, Mrel is constant.  As the clouds increase, the observed
    // star is fainter, so Msys gets larger, and the value of Moff also gets larger to compensate
    // in other words:
    // Mapp = Minst + ZP
    // is equivalent to:
    // Mrel = Msys - Moff, so a larger Moff means a smaller ZP and increase clouds (decreased transmission)
    psfStars->alldata-> yVector[Nref] = MsysPSF - MrelPSF - Moff;
    psfStars->alldata->dyVector[Nref] = MAX (catalog[c].measureT[m].dM, MIN_ERROR);
    Nref++;
  }

  if (Nref < 5) {
    result.Mresid  = NAN;
    result.dMresid = NAN;
    result.Mchisq  = NAN;
    result.Mmedian = NAN;
    result.Maltsig = NAN;
    result.Nfit    = Nref;
    return result;
  }

  StatType stats = FitDataSetSoften (psfStars, Nref);
  double altSigma = (stats.Upper80 - stats.Lower20) / 1.6;

  // no additional weight modification (we treat all stars on an image equally -- note an image is either ubercal-tied or not)
  fit1d_irls (psfStars, Nref);
  result.Mresid  = psfStars->bSaveArray[0][0];
  result.dMresid = psfStars->bSigma[0];
  result.Nfit    = psfStars->Nmeas;
  result.Mchisq  = psfStars->chisq;
  result.Mmedian = stats.median;
  result.Maltsig = altSigma;

  return result;
}
