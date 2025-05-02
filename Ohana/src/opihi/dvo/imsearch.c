# include "dvoshell.h"

int imsearch (int argc, char **argv) {
 
  char DataBase[256], name[64];
  FILE *f;
  Header header;
  RegImage *pimage;
  off_t i, Nimage, status;
  int N, TimeSelect, SaveNames;
  int ModeSelect, TypeSelect, CCDSelect, FilterSelect;
  char *Filter, *obstime;
  int Type, Mode, CCD;
  time_t tzero, obstime_sec;
  double trange;
   
  VarConfig ("REGISTRATION_DATABASE", "%s", DataBase);

  SaveNames = FALSE;
  if ((N = get_argument (argc, argv, "-save"))) {
    remove_argument (N, &argc, argv);
    SaveNames = TRUE;
  }
 
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
 
  Mode = 0;
  ModeSelect = FALSE;
  if ((N = get_argument (argc, argv, "-mode"))) {
    remove_argument (N, &argc, argv);
    Mode = get_image_mode (argv[N]);
    if (Mode == M_UNDEF) {
      gprint (GP_ERR, "ERROR: invalid image mode %s\n", argv[N]);
      exit (1);
    }
    remove_argument (N, &argc, argv);
    ModeSelect = TRUE;
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

  if (argc != 1) {
    gprint (GP_ERR, "USAGE: imsearch [-time start range] [-type type] [-mode mode] [-ccd N] [-filter name]\n");
    exit (1);
  }

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

  /* print out all data */
  N = 0;
  for (i = 0; i < Nimage; i++) {
    if (TimeSelect && ((pimage[i].obstime < tzero) || (pimage[i].obstime > tzero + trange))) continue;
    if (FilterSelect && (strcasecmp (pimage[i].filter, Filter))) continue;
    if (ModeSelect && (pimage[i].mode != Mode)) continue;
    if (CCDSelect && (pimage[i].ccd != CCD)) continue;
    if (TypeSelect && (pimage[i].type != Type)) continue;

    obstime_sec = (time_t) pimage[i].obstime;
    obstime = ctime (&obstime_sec);
    obstime[strlen(obstime)-1] = 0;

    gprint (GP_LOG, OFF_T_FMT" %6s %6s %2d %2d   ",  i, get_type_name(pimage[i].type), get_mode_name(pimage[i].mode), pimage[i].ccd, pimage[i].type);
    gprint (GP_LOG, "%s %s  ", pimage[i].pathname, pimage[i].filename);
    gprint (GP_LOG, "%s %s %f %s\n", pimage[i].filter, pimage[i].instrument, pimage[i].exptime, obstime);

    if (SaveNames) {
      sprintf (name, "IMAGEpath:%d", N);
      set_str_variable (name, pimage[i].pathname);
      sprintf (name, "IMAGEfile:%d", N);
      set_str_variable (name, pimage[i].filename);
      sprintf (name, "IMAGEmode:%d", N);
      set_int_variable (name, pimage[i].mode);
    }
    N++;
  }
  if (SaveNames) {
    set_int_variable ("IMAGEpath:n", N);
    set_int_variable ("IMAGEfile:n", N);
  }

  free (pimage);
  gfits_free_header (&header);
  return (TRUE);
}
