# include "dvoshell.h"

/* add others as needed */
enum {F_NONE, F_RA, F_DEC, F_X, F_Y, F_MAG, F_DMAG, F_TYPE, F_SKY, F_FX, F_FY, F_APMAG, F_GALMAG};

int cmpread (int argc, char **argv) {
  
  off_t i, Nbytes, Nstars;
  int field, Naxis;
  double tR, tD;
  float value;
  FILE *f;
  Vector *vec;
  Header header;
  Coords coords;
  CMPstars *stars;

  if (argc != 3) {
    gprint (GP_ERR, "USAGE: cmpread field <filename>\n");
    return (FALSE);
  }

  field = F_NONE;
  if (!strcasecmp (argv[1], "ra"))   field = F_RA;
  if (!strcasecmp (argv[1], "dec"))  field = F_DEC;
  if (!strcasecmp (argv[1], "mag"))  field = F_MAG;
  if (!strcasecmp (argv[1], "dmag")) field = F_DMAG;
  if (!strcasecmp (argv[1], "x"))    field = F_X;
  if (!strcasecmp (argv[1], "y"))    field = F_Y;
  if (!strcasecmp (argv[1], "type")) field = F_TYPE;
  if (!strcasecmp (argv[1], "sky"))  field = F_SKY;
  if (!strcasecmp (argv[1], "fx"))   field = F_FX;
  if (!strcasecmp (argv[1], "fy"))   field = F_FY;
  if (!strcasecmp (argv[1], "apmag"))  field = F_APMAG;
  if (!strcasecmp (argv[1], "galmag"))  field = F_GALMAG;
  if (field == F_NONE) {
    gprint (GP_ERR, "invalid cmp field: %s\n", argv[1]);
    return (FALSE);
  }

  if ((vec = SelectVector (argv[1],  ANYVECTOR, TRUE)) == NULL) return (FALSE);

  /* load FITS header */
  if (!gfits_read_header (argv[2], &header)) {
    gprint (GP_ERR, "ERROR: can't read header for %s\n", argv[2]);
    return (FALSE);
  }

  if ((field == F_RA) || (field == F_DEC)) {
    if (!GetCoords (&coords, &header)) {
      gprint (GP_ERR, "can't get WCS info from header\n");
      gfits_free_header (&header);
      return (FALSE);
    }
  }

  /* re-open file to load data */
  f = fopen (argv[2], "r");
  if (f == NULL) {
    gprint (GP_ERR, "ERROR: can't read data from %s\n", argv[2]);
    gfits_free_header (&header);
    return (FALSE);
  }
  fseeko (f, header.datasize, SEEK_SET); 

  /* find expected number of stars */
  if (!gfits_scan (&header, "NSTARS", OFF_T_FMT, 1,  &Nstars)) {
    gprint (GP_ERR, "ERROR: can't get NSTARS from header\n");
    gfits_free_header (&header);
    return (FALSE);
  }

  /* read from FITS table or from text table */
  Naxis = 0;
  gfits_scan (&header, "NAXIS",  "%d", 1, &Naxis);
  if (Naxis == 2) {
    /* allocate space for stars */
    gprint (GP_ERR, "reading from TEXT cmp file %s\n", argv[2]);
    if (!gfits_scan (&header, "NSTARS", OFF_T_FMT, 1,  &Nstars)) {
      gprint (GP_ERR, "ERROR: failed to find NSTARS\n");
      exit (1);
    }
    stars = cmpReadText (f, &Nstars);
  } else {
    gprint (GP_ERR, "reading from FITS cmp file %s\n", argv[2]);
    Nbytes = gfits_data_size (&header);
    fseeko (f, Nbytes, SEEK_CUR); 
    stars = cmpReadFits (f, &Nstars);
  }
  fclose (f);

  ResetVector (vec, OPIHI_FLT, Nstars);
  bzero (vec[0].elements.Flt, Nstars*sizeof(opihi_flt));

  value = 0;
  for (i = 0; i < Nstars; i++) {
    switch (field) {
      case F_RA:
	XY_to_RD (&tR, &tD, stars[i].X, stars[i].Y, &coords);
	value = tR;
	break;
      case F_DEC:
	XY_to_RD (&tR, &tD, stars[i].X, stars[i].Y, &coords);
	value = tD;
	break;
      case F_X:
	value = stars[i].X;
	break;
      case F_Y:
	value = stars[i].Y;
	break;
      case F_MAG:
	value = stars[i].M;
	break;
      case F_APMAG:
	value = stars[i].Map;
	break;
      case F_GALMAG:
	value = stars[i].Mgal;
	break;
      case F_DMAG:
	value = stars[i].dM;
	break;
      case F_TYPE:
	value = stars[i].dophot;
	break;
      case F_SKY:
	value = stars[i].sky;
	break;
      case F_FX:
	value = stars[i].fx;
	break;
      case F_FY:
	value = stars[i].fy;
	break;
    }
    vec[0].elements.Flt[i] = value;
  }      
  free (stars);
  gfits_free_header (&header);
  gprint (GP_ERR, "loaded "OFF_T_FMT" objects\n",  Nstars);
  return (TRUE);
}
