# include "relastro.h"

// STATUS is value expected for success
# define CHECK_STATUS(STATUS,MSG,...)					\
  if (!(STATUS)) {							\
    fprintf (stderr, MSG, __VA_ARGS__);					\
    return FALSE;							\
  }

// the frame correction consists of N image correction maps
// there are local maps and AITOFF maps
FrameCorrectionSet *FrameCorrectionSetLoad(char *filename) {

  Header header;
  Matrix matrix;

  FILE *f = fopen (filename, "r");
  if (!f) {
    fprintf (stderr, "ERROR: cannot open image subset file %s\n", filename);
    return NULL;
  }

  // FrameCorrectionSet consists:
  // PHU header listing the contents:
  // SHMAP_dR : EXTNAME or NONE
  // SHMAP_dD : EXTNAME or NONE
  // LOCAL_dR : EXTNAME or NONE
  // LOCAL_dD : EXTNAME or NONE

  if (!gfits_fread_header (f, &header)) {
    if (VERBOSE) fprintf (stderr, "can't read image subset header\n");
    fclose (f);
    return NULL;
  }

  char SHMAP_DR[80], SHMAP_DD[80], LOCAL_DR[80], LOCAL_DD[80];
  if (!gfits_scan (&header, "SHMAP_DR", "%s", 1, SHMAP_DR)) {
    fprintf (stderr, "ERROR: FrameCorrectionSet %s missing required field SHMAP_DR\n", filename);
    exit (2);
  }
  if (!gfits_scan (&header, "SHMAP_DD", "%s", 1, SHMAP_DD)) {
    fprintf (stderr, "ERROR: FrameCorrectionSet %s missing required field SHMAP_DD\n", filename);
    exit (2);
  }
  if (!gfits_scan (&header, "LOCAL_DR", "%s", 1, LOCAL_DR)) {
    fprintf (stderr, "ERROR: FrameCorrectionSet %s missing required field LOCAL_DR\n", filename);
    exit (2);
  }
  if (!gfits_scan (&header, "LOCAL_DD", "%s", 1, LOCAL_DD)) {
    fprintf (stderr, "ERROR: FrameCorrectionSet %s missing required field LOCAL_DD\n", filename);
    exit (2);
  }
  gfits_free_header (&header);
    
  FrameCorrectionSet *set = FrameCorrectionSetInit ();

  if (strcmp(SHMAP_DR, "NONE")) {
    if (!strcmp (SHMAP_DD, "NONE")) {
      fprintf (stderr, "ERROR: FrameCorrectionSet %s has defined SHMAP_DR but not SHMAP_DD\n", filename);
      exit (2);
    }
    if (!gfits_find_Xheader(f, &header, SHMAP_DR)) {
      fprintf (stderr, "ERROR: FrameCorrectionSet %s missing SHMAP_DR extension %s\n", filename, SHMAP_DR);
      exit (2);
    }
    if (!gfits_fread_matrix (f, &matrix, &header)) {
      if (VERBOSE) fprintf (stderr, "can't read SHMAP_DR extension image\n");
      gfits_free_header (&header);
      fclose (f);
      return NULL;
    }
    // convert image to frame
    set->frame = FrameCorrectionImageToSH (&header, &matrix, NULL, TRUE);
    gfits_free_header (&header);
    gfits_free_matrix (&matrix);

    if (!gfits_find_Xheader(f, &header, SHMAP_DD)) {
      fprintf (stderr, "ERROR: FrameCorrectionSet %s missing SHMAP_DD extension %s\n", filename, SHMAP_DD);
      exit (2);
    }
    if (!gfits_fread_matrix (f, &matrix, &header)) {
      if (VERBOSE) fprintf (stderr, "can't read SHMAP_DD extension image\n");
      gfits_free_header (&header);
      fclose (f);
      return NULL;
    }
    set->frame = FrameCorrectionImageToSH (&header, &matrix, set->frame, TRUE);
    gfits_free_header (&header);
    gfits_free_matrix (&matrix);
  } else {
    if (strcmp (SHMAP_DD, "NONE")) {
      fprintf (stderr, "ERROR: FrameCorrectionSet %s has defined SHMAP_DD but not SHMAP_DR\n", filename);
      exit (2);
    }
  }

  if (strcmp(LOCAL_DR, "NONE")) {
    if (!strcmp (LOCAL_DD, "NONE")) {
      fprintf (stderr, "ERROR: FrameCorrectionSet %s has defined LOCAL_DR but not LOCAL_DD\n", filename);
      exit (2);
    }
    if (!gfits_find_Xheader(f, &header, LOCAL_DR)) {
      fprintf (stderr, "ERROR: FrameCorrectionSet %s missing LOCAL_DR extension %s\n", filename, LOCAL_DR);
      exit (2);
    }
    if (!gfits_fread_matrix (f, &matrix, &header)) {
      if (VERBOSE) fprintf (stderr, "can't read LOCAL_DR extension image\n");
      gfits_free_header (&header);
      fclose (f);
      return NULL;
    }
    // convert image to frame
    set->coords = FrameCorrectionImageToMap (&header, &matrix, NULL, TRUE);
    gfits_free_header (&header);
    gfits_free_matrix (&matrix);

    if (!gfits_find_Xheader(f, &header, LOCAL_DD)) {
      fprintf (stderr, "ERROR: FrameCorrectionSet %s missing LOCAL_DD extension %s\n", filename, LOCAL_DD);
      exit (2);
    }
    if (!gfits_fread_matrix (f, &matrix, &header)) {
      if (VERBOSE) fprintf (stderr, "can't read LOCAL_DD extension image\n");
      gfits_free_header (&header);
      fclose (f);
      return NULL;
    }
    set->coords = FrameCorrectionImageToMap (&header, &matrix, set->coords, FALSE);
    gfits_free_header (&header);
    gfits_free_matrix (&matrix);
  } else {
    if (strcmp (LOCAL_DD, "NONE")) {
      fprintf (stderr, "ERROR: FrameCorrectionSet %s has defined LOCAL_DD but not LOCAL_DR\n", filename);
      exit (2);
    }
  }
  fclose (f);
  return set;
}

int FrameCorrectionSetSave(char *filename, FrameCorrectionSet *set) {

  int status;
  Header header;
  Matrix matrix;

  FILE *f = fopen (filename, "w");
  if (!f) {
    fprintf (stderr, "ERROR: cannot open meanmag file for output %s\n", filename);
    return FALSE;
  }

  gfits_init_header (&header);
  header.extend = TRUE;
  gfits_create_header (&header);
  gfits_create_matrix (&header, &matrix);

  // add entries to the PHU
  if (set->frame) {
    gfits_modify (&header, "SHMAP_DR", "%s", 1, "SHMAP_DR");
    gfits_modify (&header, "SHMAP_DD", "%s", 1, "SHMAP_DD");
  } else {
    gfits_modify (&header, "SHMAP_DR", "%s", 1, "NONE");
    gfits_modify (&header, "SHMAP_DD", "%s", 1, "NONE");
  }
  if (set->coords) {
    gfits_modify (&header, "LOCAL_DR", "%s", 1, "LOCAL_DR");
    gfits_modify (&header, "LOCAL_DD", "%s", 1, "LOCAL_DD");
  } else {
    gfits_modify (&header, "LOCAL_DR", "%s", 1, "NONE");
    gfits_modify (&header, "LOCAL_DD", "%s", 1, "NONE");
  }

  // write the PHU header (& empty matrix) to disk 
  status = gfits_fwrite_header  (f, &header);
  CHECK_STATUS (status, "ERROR: cannot write header for frame correction %s\n", filename);

  status = gfits_fwrite_matrix  (f, &matrix);
  CHECK_STATUS (status, "ERROR: cannot write matrix for correction %s\n", filename);

  gfits_free_header (&header);
  gfits_free_matrix (&matrix);

  if (set->frame) {
    // create and save the RA correction
    FrameCorrectionSHtoImage (&header, &matrix, set->frame, TRUE);
    gfits_modify (&header, "EXTNAME", "%s", 1, "SHMAP_DR");

    status = gfits_fwrite_header  (f, &header);
    CHECK_STATUS (status, "ERROR: cannot write header for SHMAP_DR %s\n", filename);

    status = gfits_fwrite_matrix  (f, &matrix);
    CHECK_STATUS (status, "ERROR: cannot write matrix for SHMAP_DR %s\n", filename);

    gfits_free_header (&header);
    gfits_free_matrix (&matrix);

    // create and save the DEC correction
    FrameCorrectionSHtoImage (&header, &matrix, set->frame, FALSE);
    gfits_modify (&header, "EXTNAME", "%s", 1, "SHMAP_DD");

    status = gfits_fwrite_header  (f, &header);
    CHECK_STATUS (status, "ERROR: cannot write header for SHMAP_DD %s\n", filename);

    status = gfits_fwrite_matrix  (f, &matrix);
    CHECK_STATUS (status, "ERROR: cannot write matrix for SHMAP_DD %s\n", filename);

    gfits_free_header (&header);
    gfits_free_matrix (&matrix);
  }

  if (set->coords) {
    // create and save the RA correction
    FrameCorrectionMapToImage (&header, &matrix, set->coords, TRUE);
    gfits_modify (&header, "EXTNAME", "%s", 1, "LOCAL_DR");

    status = gfits_fwrite_header  (f, &header);
    CHECK_STATUS (status, "ERROR: cannot write header for meanpos %s\n", filename);

    status = gfits_fwrite_matrix  (f, &matrix);
    CHECK_STATUS (status, "ERROR: cannot write matrix for meanpos %s\n", filename);

    gfits_free_header (&header);
    gfits_free_matrix (&matrix);

    // create and save the DEC correction
    FrameCorrectionMapToImage (&header, &matrix, set->coords, FALSE);
    gfits_modify (&header, "EXTNAME", "%s", 1, "LOCAL_DD");

    status = gfits_fwrite_header  (f, &header);
    CHECK_STATUS (status, "ERROR: cannot write header for meanpos %s\n", filename);

    status = gfits_fwrite_matrix  (f, &matrix);
    CHECK_STATUS (status, "ERROR: cannot write matrix for meanpos %s\n", filename);

    gfits_free_header (&header);
    gfits_free_matrix (&matrix);
  }

  int fd = fileno (f);

  status = fflush (f);
  CHECK_STATUS (!status, "ERROR: cannot flush file meanpos %s\n", filename);

  status = fsync (fd);
  CHECK_STATUS (!status, "ERROR: cannot flush file meanpos %s\n", filename);

  status = fclose (f);
  CHECK_STATUS (!status, "ERROR: problem closing meanpos file %s\n", filename);

  return TRUE;
}
