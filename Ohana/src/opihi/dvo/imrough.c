# include "dvoshell.h"
RegImage *load_imreg (char *DataBase, off_t *nimage);

# define NVALUE 22
enum {V_NONE, V_EXPTIME, V_CCDN, V_SKY, V_BIAS, V_FILTER, V_FWHM, V_AIRM, V_TIME, V_TEMP0, V_TEMP1, V_TEMP2, V_TEMP3, V_TELFOCUS, V_XPROBE, V_YPROBE, V_ZPROBE, V_RA, V_DEC, V_DETTEMP, V_ROTANGLE, V_REGTIME};
static char valuename[NVALUE][32] = {"none", "exptime", "ccd", "sky", "bias", "filter", "fwhm", "airmass", "time", "temp0", "temp1", "temp2", "temp3", "telfocus", "xprobe", "yprobe", "zprobe", "ra", "dec", "dettemp", "rotangle", "regtime"};

int imrough (int argc, char **argv) {
 
  off_t i, Nimage;
  int N, TimeSelect;
  int ModeSelect, TypeSelect, CCDSelect, FilterSelect, TimeFormat;
  int type, value, mode, CCD, NVEC;
  char DataBase[256], *Filter;
  double trange;
  opihi_flt *Vec;
  time_t tzero, tend, TimeReference;
  RegImage *image;
  Vector *vec;

  // XXX this function is only valid for the Elixir db system, not the IPP db system
  // this function should exit gracefully if the REGISTRATION_DATABASE is missing
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
 
  type = T_UNDEF;
  TypeSelect = FALSE;
  if ((N = get_argument (argc, argv, "-type"))) {
    TypeSelect = TRUE;
    remove_argument (N, &argc, argv);
    type = get_image_type (argv[N]);
    if (type == T_UNDEF) {
      gprint (GP_ERR, "ERROR: invalid image type %s\n", argv[N]);
      return (FALSE);
    }
    remove_argument (N, &argc, argv);
  }

  mode = M_NONE;
  ModeSelect = FALSE;
  if ((N = get_argument (argc, argv, "-mode"))) {
    ModeSelect = TRUE;
    remove_argument (N, &argc, argv);
    mode = get_image_mode (argv[N]);
    if (mode == M_UNDEF) {
      gprint (GP_ERR, "ERROR: invalid image mode %s\n", argv[N]);
      return (FALSE);
    }
    remove_argument (N, &argc, argv);
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
    gprint (GP_ERR, "       value options:\n");
    for (i = 1; i < NVALUE; i++) {
      gprint (GP_ERR, "%s\n", valuename[i]);
    }
    return (FALSE);
  }
 
  /* identify selection */
  value = V_NONE;
  for (i = 0; (i < NVALUE) && (value == V_NONE); i++) {
    if (!strncasecmp (argv[1], valuename[i], strlen(argv[1]))) value = i;
  }
  if (value == V_NONE) {
    gprint (GP_ERR, "ERROR: invalid image value %s\n", argv[1]);
    return (FALSE);
  }

  if ((vec = SelectVector (argv[1], ANYVECTOR, TRUE)) == NULL) return (FALSE);

  image = load_imreg (DataBase, &Nimage);
  if (image == (RegImage *) NULL) return (FALSE);

  N = 0;
  NVEC = 1000;
  ResetVector (vec, OPIHI_FLT, NVEC);
  Vec = vec[0].elements.Flt;

  GetTimeFormat (&TimeReference, &TimeFormat);

  /* get data */
  for (i = 0; i < Nimage; i++) {
    /* skip unmatched selections */
    if (TimeSelect && ((image[i].obstime < tzero) || (image[i].obstime > tzero + trange))) continue;
    if (TimeSelect && ((image[i].obstime < tzero) || (image[i].obstime > tzero + trange))) continue;
    if (FilterSelect && (strcasecmp (image[i].filter, Filter))) continue;
    if (CCDSelect && (image[i].ccd != CCD)) continue;
    if (TypeSelect && (image[i].type != type)) continue;
    if (ModeSelect && (image[i].mode != mode)) continue;

    /* assign correct value */
    switch (value) {
    case (V_EXPTIME):
      Vec[N] = image[i].exptime;
      break;
    case (V_CCDN):
      Vec[N] = image[i].ccd;
      break;
    case (V_SKY):
      Vec[N] = image[i].sky;
      break;
    case (V_BIAS):
      Vec[N] = image[i].bias;
      break;
    case (V_FILTER):
      Vec[N] = image[i].filter[0];
      break;
    case (V_FWHM):
      Vec[N] = image[i].fwhm;
      break;
    case (V_AIRM):
      Vec[N] = image[i].airmass;
      break;
    case (V_TIME):
      Vec[N] = TimeValue (image[i].obstime, TimeReference, TimeFormat);
      break;
    case (V_TEMP0):
      Vec[N] = image[i].teltemp_0;
      break;
    case (V_TEMP1):
      Vec[N] = image[i].teltemp_1;
      break;
    case (V_TEMP2):
      Vec[N] = image[i].teltemp_2;
      break;
    case (V_TEMP3):
      Vec[N] = image[i].teltemp_3;
      break;
    case (V_TELFOCUS):
      Vec[N] = image[i].telfocus;
      break;
    case (V_XPROBE):
      Vec[N] = image[i].xprobe;
      break;
    case (V_YPROBE):
      Vec[N] = image[i].yprobe;
      break;
    case (V_ZPROBE):
      Vec[N] = image[i].zprobe;
      break;
    case (V_RA):
      Vec[N] = image[i].ra;
      break;
    case (V_DEC):
      Vec[N] = image[i].dec;
      break;
    case (V_DETTEMP):
      Vec[N] = image[i].dettemp;
      break;
    case (V_ROTANGLE):
      Vec[N] = image[i].rotangle;
      break;
    case (V_REGTIME):
      Vec[N] = TimeValue (image[i].regtime, TimeReference, TimeFormat);
      break;
    }
    N++;
    if (N >= NVEC - 1) {
      NVEC += 1000;
      REALLOCATE (vec[0].elements.Flt, opihi_flt, NVEC);
      Vec = vec[0].elements.Flt;
    }
  }

  REALLOCATE (vec[0].elements.Flt, opihi_flt, MAX (1,N));
  vec[0].Nelements = N;

  free (image);
  return (TRUE);

}

RegImage *load_imreg (char *DataBase, off_t *nimage) {

  off_t Nimage;
  int status;
  char line[80];
  FILE *f;
  Header header, theader;
  Matrix matrix;
  FTable table;
  RegImage *image;

  *nimage = 0;

  /* open database */
  f = fopen (DataBase, "r");
  if (f == (FILE *) NULL) {
    gprint (GP_ERR, "ERROR: can't open Registration Database\n");
    return ((RegImage *) NULL);
  }

  /* load in database header */
  if (!gfits_fread_header (f, &header)) {
    fclose (f);
    gfits_free_header (&header);
    gprint (GP_ERR, "ERROR: trouble reading database header\n");
    return ((RegImage *) NULL);
  }

  /* check for database v1, v2 */
  gfits_scan (&header, "ORIGIN", "%s", 1, line);
  if (!strcmp (line, "MDM Observatory")) {

    fseeko (f, header.datasize, SEEK_SET);
    
    /* load existing data from database */
    gfits_scan (&header, "NIMAGES", OFF_T_FMT, 1,  &Nimage);
    ALLOCATE (image, RegImage, Nimage);
    status = fread (image, sizeof(RegImage), Nimage, f);
    fclose (f);
    
    if (status != Nimage) {
      gprint (GP_ERR, "ERROR: header and data in dB don't match ("OFF_T_FMT" vs %d)\n",  Nimage, status);
      gfits_free_header (&header);
      free (image);
      return ((RegImage *) NULL);
    }
    gfits_convert_RegImage (image, sizeof (RegImage), Nimage);

    *nimage = Nimage;
    return (image);
  }

  /* we probably have v3 */
  if (!gfits_fread_matrix (f, &matrix, &header)) {
    fclose (f);
    gfits_free_header (&header);
    gfits_free_matrix (&matrix);
    gprint (GP_ERR, "ERROR: trouble reading database matrix\n");
    return ((RegImage *) NULL);
  }

  table.header = &theader;
  if (!gfits_fread_ftable  (f, &table, "IMAGE_DATABASE")) {
    fclose (f);
    gfits_free_header (&header);
    gfits_free_matrix (&matrix);
    gprint (GP_ERR, "ERROR: trouble reading database table\n");
    return ((RegImage *) NULL);
  }

  /* convert to internal format */
  image = (RegImage *) table.buffer;
  gfits_scan (table.header, "NAXIS2", OFF_T_FMT, 1,  &Nimage);
  gfits_convert_RegImage (image, sizeof (RegImage), Nimage);

  gfits_free_header (&header);
  gfits_free_matrix (&matrix);

  *nimage = Nimage;
  return (image);
}
