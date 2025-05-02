# include "dvoshell.h"

/* qualities to be extracted */
enum {ZERO, DTIME, SKY, BIAS, FWHM, AIRM, TIME, TEMP};

int detrend (int argc, char **argv) {
 
  FILE *f;
  off_t i, Nimage, status;
  int N, TimeSelect;
  char DataBase[256];
  time_t tzero, tend;
  double trange;
  int TypeSelect, CCDSelect, FilterSelect;
  char *Filter;
  int Type, mode, CCD;
  int NVALUE;
  opihi_flt *value;
  time_t TimeReference;
  int TimeFormat;
  Header header;
  RegImage *pimage;
  Vector *vec;

  VarConfig ("REGISTRATION_DATABASE", "%s", DataBase);

  TimeSelect = FALSE;
  if ((N = get_argument (argc, argv, "-time"))) {
    remove_argument (N, &argc, argv);
    if (!ohana_str_to_time (argv[N], &tzero)) { 
      gprint (GP_ERR, "syntax error\n");
      return (FALSE);
    }
    remove_argument (N, &argc, argv);
    if (!ohana_str_to_dtime (argv[N], &trange)) { 
      gprint (GP_ERR, "syntax error\n");
      return (FALSE);
    }
    remove_argument (N, &argc, argv);
    if (trange < 0) {
      trange = fabs (trange);
      tzero -= trange;
    }
    TimeSelect = TRUE;
  }
  if ((N = get_argument (argc, argv, "-trange"))) {
    remove_argument (N, &argc, argv);
    if (!ohana_str_to_time (argv[N], &tzero)) { 
      gprint (GP_ERR, "syntax error\n");
      return (FALSE);
    }
    remove_argument (N, &argc, argv);
    if (!ohana_str_to_time (argv[N], &tend)) { 
      gprint (GP_ERR, "syntax error\n");
      return (FALSE);
    }
    remove_argument (N, &argc, argv);
    trange = tend - tzero;
    if (trange < 0) {
      trange = fabs (trange);
      tzero -= trange;
    }
    TimeSelect = TRUE;
  }
 
  Type = 0;
  TypeSelect = FALSE;
  if ((N = get_argument (argc, argv, "-type"))) {
    remove_argument (N, &argc, argv);
    Type = get_image_type (argv[N]);
    if (Type == T_UNDEF) {
      gprint (GP_ERR, "ERROR: invalid image type %s\n", argv[N]);
      return (FALSE);
    }
    remove_argument (N, &argc, argv);
    TypeSelect = TRUE;
  }

  CCD = 0;
  CCDSelect = FALSE;
  if ((N = get_argument (argc, argv, "-ccd"))) {
    remove_argument (N, &argc, argv);
    CCD = atoi (argv[N]);
    remove_argument (N, &argc, argv);
    CCDSelect = TRUE;
  }
 
  Filter = NULL;
  FilterSelect = FALSE;
  if ((N = get_argument (argc, argv, "-filter"))) {
    remove_argument (N, &argc, argv);
    Filter = strcreate (argv[N]);
    remove_argument (N, &argc, argv);
    FilterSelect = TRUE;
    if (!strcasecmp (Filter, "X")) {
      FilterSelect = FALSE;
    }
  }

  if (argc != 2) {
    gprint (GP_ERR, "USAGE: imrough (value)\n");
    return (FALSE);
  }
  gprint (GP_ERR, "  this function is not well-defined.  re-work and re-code\n");
  return (FALSE);
  
  /* identify selection */
  mode = ZERO;
  if (!strcasecmp (argv[1], "exptime")) mode = DTIME;
  if (!strcasecmp (argv[1], "sky")) mode = SKY;
  if (!strcasecmp (argv[1], "bias")) mode = BIAS;
  if (!strcasecmp (argv[1], "fwhm")) mode = FWHM;
  if (!strcasecmp (argv[1], "airmass")) mode = AIRM;
  if (!strcasecmp (argv[1], "time")) mode = TIME;
  if (!strcasecmp (argv[1], "temp")) mode = TEMP;
  if (mode == ZERO) {
    gprint (GP_ERR, "value may be one of the following:\n");
    gprint (GP_ERR, " exptime, sky, bias, fwhm, airmass, time\n");
    return (FALSE);
  }

  if ((vec = SelectVector (argv[1], ANYVECTOR, TRUE)) == NULL) return (FALSE);

  /* load in database header */
  if (!gfits_read_header (DataBase, &header)) {
    gprint (GP_ERR, "ERROR: trouble reading database header\n");
    return (FALSE);
  }

  /* open database */
  f = fopen (DataBase, "r");
  if (f == (FILE *) NULL) {
    gprint (GP_ERR, "ERROR: can't open Registration Database\n");
    return (FALSE);
  }
  fseeko (f, header.datasize, SEEK_SET);

  /* load existing data from database */
  gfits_scan (&header, "NIMAGES", OFF_T_FMT, 1,  &Nimage);
  ALLOCATE (pimage, RegImage, Nimage);
  status = fread (pimage, sizeof(RegImage), Nimage, f);
  fclose (f);

  if (status != Nimage) {
    gprint (GP_ERR, "ERROR: header and data in dB don't match ("OFF_T_FMT" vs "OFF_T_FMT")\n",  Nimage,  status);
    gfits_free_header (&header);
    free (pimage);
    return (FALSE);
  }
  gfits_convert_RegImage (pimage, sizeof (RegImage), Nimage);

  N = 0;
  NVALUE = 1000;
  ResetVector (vec, OPIHI_FLT, NVALUE);
  value = vec[0].elements.Flt;

  GetTimeFormat (&TimeReference, &TimeFormat);
  gprint (GP_ERR, "%ld %d\n", TimeReference, TimeFormat);

  /* get data */
  for (i = 0; i < Nimage; i++) {
    /* skip unmatched selections */
    if (TimeSelect && ((pimage[i].obstime < tzero) || (pimage[i].obstime > tzero + trange))) continue;
    if (TimeSelect && ((pimage[i].obstime < tzero) || (pimage[i].obstime > tzero + trange))) continue;
    if (FilterSelect && (strcasecmp (pimage[i].filter, Filter))) continue;
    if (CCDSelect && (pimage[i].ccd != CCD)) continue;
    if (TypeSelect && (pimage[i].type != Type)) continue;

    /* assign correct value */
    switch (mode) {
    case (DTIME):
      value[N] = pimage[i].exptime;
      break;
    case (TIME):
      value[N] = TimeValue (pimage[i].obstime, TimeReference, TimeFormat);
      break;
    case (SKY):
      value[N] = pimage[i].sky;
      break;
    case (BIAS):
      value[N] = pimage[i].bias;
      break;
    case (FWHM):
      value[N] = pimage[i].fwhm;
      break;
    case (AIRM):
      value[N] = pimage[i].airmass;
      break;
    case (TEMP):
      value[N] = pimage[i].teltemp_0;
      break;
    }
    N++;
    if (N >= NVALUE - 1) {
      NVALUE += 1000;
      REALLOCATE (vec[0].elements.Flt, opihi_flt, NVALUE);
      value = vec[0].elements.Flt;
    }
  }

  REALLOCATE (vec[0].elements.Flt, opihi_flt, MAX (1,N));
  vec[0].Nelements = N;

  free (pimage);
  gfits_free_header (&header);
  return (TRUE);

}
