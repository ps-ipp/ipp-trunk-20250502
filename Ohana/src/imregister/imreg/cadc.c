# include "imregister.h"
# include "imreg.h"

static int REFCCD;

void DumpCADCTable (char *filename, RegImage *image, off_t *match, off_t Nmatch) {
  
  off_t i, Obsid, Nobsid, Nsubset, ref;
  char *datestr, *line, hdrname[99];
  time_t tsecond;
  Header header, theader;
  Matrix matrix;
  FTable table;
  RegImage *row;
  off_t *index, *entry, *obsid;
  MosaicLayout *layout;
  off_t *subset;
  double left, right, center, outer, top, bottom;
  double iqx, iqy, iqr, iqf, iqo;

  /* set REFCCD for use in GetREFIQ */
  REFCCD = MatchCCDName (SeeingREFCCD);
  if (REFCCD == -1) { 
    fprintf (stderr, "ERROR: can't get reference ccd\n");
    exit (1);
  }

  /* assign relevant mosaic layout structure */
  layout = (MosaicLayout *) NULL;
  if (!strcasecmp (Camera, "CFH12K"))  layout = CreateCFH12K ();
  if (!strcasecmp (Camera, "MegaCam")) layout = CreateMegaCam ();
  if (!strcasecmp (Camera, "MegaNorth")) layout = CreateMegaCam ();
  if (layout == (MosaicLayout *) NULL) {
    fprintf (stderr, "ERROR: invalid camera for CADC Table\n");
    exit (1);
  }

  /* create primary header */
  gfits_init_header (&header);    
  header.extend = TRUE;
  gfits_create_header (&header);
  gfits_create_matrix (&header, &matrix);
  gfits_print (&header, "NEXTEND", "%d", 1, 1);
  
  /* create table header */
  gfits_create_table_header (&theader, "TABLE", "CADC_RAW_IMAGES");
      
  /* add current date/time to header */
  ohana_str_to_time ("now", &tsecond);
  datestr = ohana_sec_to_date (tsecond);
  gfits_modify (&header,  "DATE", "%s", 1, datestr);
  gfits_modify (&theader, "DATE", "%s", 1, datestr);

  /* define table layout */
  gfits_define_table_column (&theader, "A99",   "FILENAME",       "filename in db",                  "");
  gfits_define_table_column (&theader, "A99",   "HDR_FILENAME",   "image header filename",           "");
  gfits_define_table_column (&theader, "I10",   "OBSID",          "image ID number",                 "");
  gfits_define_table_column (&theader, "F5.2",  "OBS_IQ_REFCCD",  "image quality on reference chip", "arcsec");
  gfits_define_table_column (&theader, "F5.2",  "OBS_IQ_CENTER",  "image quality center region",     "arcsec");
  gfits_define_table_column (&theader, "F5.2",  "OBS_IQ_R_RATIO", "IQ ratio (outer / center)",       "");
  gfits_define_table_column (&theader, "F5.2",  "OBS_IQ_X_RATIO", "IQ ratio (left / right)",         "");
  gfits_define_table_column (&theader, "F5.2",  "OBS_IQ_Y_RATIO", "IQ ratio (top / bottom)",         "");
  gfits_define_table_column (&theader, "F9.3",  "OBS_BG_VAL",     "background level",                "counts / pixel");

  /* define TNULL, TNVAL values */
  gfits_modify (&theader, "TNULL1",  "%s", 1, "NULL");  /* FILENAME       */
  gfits_modify (&theader, "TNULL2",  "%s", 1, "NULL");  /* HDR_FILENAME   */
  gfits_modify (&theader, "TNULL3",  "%s", 1, "0");     /* OBSID          */
  gfits_modify (&theader, "TNULL4",  "%s", 1, "0.00");  /* OBS_IQ_REFCCD  */
  gfits_modify (&theader, "TNULL5",  "%s", 1, "0.00");  /* OBS_IQ_CENTER  */
  gfits_modify (&theader, "TNULL6",  "%s", 1, "0.00");  /* OBS_IQ_R_RATIO */
  gfits_modify (&theader, "TNULL7",  "%s", 1, "0.00");  /* OBS_IQ_X_RATIO */
  gfits_modify (&theader, "TNULL8",  "%s", 1, "0.00");  /* OBS_IQ_Y_RATIO */
  gfits_modify (&theader, "TNULL9",  "%s", 1, "0.00");  /* OBS_BG_VAL     */

  gfits_modify (&theader, "TNVAL1",  "%s", 1, "NA");    /* FILENAME     */
  gfits_modify (&theader, "TNVAL2",  "%s", 1, "NA");    /* HDR_FILENAME */
  gfits_modify (&theader, "TNVAL3",  "%s", 1, "-1");    /* OBSID        */
  gfits_modify (&theader, "TNVAL4",  "%s", 1, "-1.00"); /* OBS_IQ_REFCCD  */
  gfits_modify (&theader, "TNVAL5",  "%s", 1, "-1.00"); /* OBS_IQ_CENTER  */
  gfits_modify (&theader, "TNVAL6",  "%s", 1, "-1.00"); /* OBS_IQ_R_RATIO */
  gfits_modify (&theader, "TNVAL7",  "%s", 1, "-1.00"); /* OBS_IQ_X_RATIO */
  gfits_modify (&theader, "TNVAL8",  "%s", 1, "-1.00"); /* OBS_IQ_Y_RATIO */
  gfits_modify (&theader, "TNVAL9",  "%s", 1, "-1.00"); /* OBS_BG_VAL   */

  /* create table, add data values */
  gfits_create_table (&theader, &table);
  
  /* prepare indicies to handle table data */
  GetObsIDIndex (image, match, Nmatch, &index, &entry);
  obsid = GetUniqueObsID (image, index, entry, Nmatch, &Nobsid);

  for (i = 0; i < Nobsid; i++) {
    ref = GetREFCCD (image, index, entry, Nmatch, obsid[i]);
    row = &image[ref];
    Obsid = index[obsid[i]];
    sprintf (hdrname, "%s.hdr", row[0].filename);

    subset = GetObsIDSubset (image, obsid[i], index, entry, Nmatch, &Nsubset);
    center = MosaicIQStats (image, subset, Nsubset, &layout[0].center);
    outer  = MosaicIQStats (image, subset, Nsubset, &layout[0].outer );
    top    = MosaicIQStats (image, subset, Nsubset, &layout[0].top   );
    bottom = MosaicIQStats (image, subset, Nsubset, &layout[0].bottom);
    left   = MosaicIQStats (image, subset, Nsubset, &layout[0].left  );
    right  = MosaicIQStats (image, subset, Nsubset, &layout[0].right );
    free (subset);

    iqx = (left   == 0) ? 0 : (right / left);
    iqy = (bottom == 0) ? 0 : (top / bottom);
    iqr = (center == 0) ? 0 : (outer / center);
    iqf = center * ARCSEC_PIXEL;
    iqo = row[0].fwhm*ARCSEC_PIXEL;
    line = gfits_table_print (&table, row[0].filename, hdrname, Obsid, iqo, iqf, iqr, iqx, iqy, row[0].sky);

    gfits_add_rows (&table, line, 1, strlen(line));
    free (line);
  }
  free (obsid);
  free (index);
  free (entry);

  gfits_write_header  (filename, &header);
  gfits_write_matrix  (filename, &matrix);
  gfits_write_Theader (filename, &theader);
  gfits_write_table   (filename, &table);
  exit (0);
}

off_t *GetObsIDSubset (RegImage *image, off_t start, off_t *index, off_t *entry, off_t Nindex, off_t *Nsubset) {

  off_t i, N, NSUBSET;
  off_t *subset;

  /* create output index */
  N = 0;
  NSUBSET = 64;
  ALLOCATE (subset, off_t, NSUBSET);

  /* find unique sequences */
  for (i = start; (i < Nindex) && (index[i] == index[start]); i++) {
    subset[N] = entry[i];
    N++;
    if (N == NSUBSET) {
      NSUBSET += 64;
      REALLOCATE (subset, off_t, NSUBSET);
    }
  }

  *Nsubset = N;
  return (subset);
}

/* return an index list of unique obs id entries at start of sequence */
off_t *GetUniqueObsID (RegImage *image, off_t *index, off_t *entry, off_t Nindex, off_t *Nmatch) {
  
  off_t i, j, N, NMATCH;
  off_t *match;

  /* create output index */
  N = 0;
  NMATCH = 1000;
  ALLOCATE (match, off_t, NMATCH);

  /* find unique sequences */
  for (i = 0; i < Nindex; ) {
    for (j = i + 1; (j < Nindex) && (index[i] == index[j]); j++);

    /* add unique entry to output list */
    match[N] = i;
    N ++;
    if (N == NMATCH) {
      NMATCH += 1000;
      REALLOCATE (match, off_t, NMATCH);
    }

    /* j always points to the next entry */
    i = j;
  }
  *Nmatch = N;
  return (match);
}

/* return seeing for ccd == REFCCD */
off_t GetREFCCD (RegImage *image, off_t *index, off_t *entry, off_t Nindex, off_t start) {
  
  off_t i;

  /* find unique sequences */
  for (i = start; (i < Nindex) && (index[i] == index[start]); i++) {
    if (image[entry[i]].ccd == REFCCD) return (entry[i]);
  }
  return (start);
}

void GetObsIDIndex (RegImage *image, off_t *match, off_t Nmatch, off_t **Index, off_t **Entry) {

  off_t i;
  off_t *index, *entry;

  /* index = OBSID */
  ALLOCATE (index, off_t, Nmatch);
  ALLOCATE (entry, off_t, Nmatch);
  for (i = 0; i < Nmatch; i++) {
    index[i] = atoi (image[match[i]].filename);
    if (index[i] < 400000) fprintf (stderr, "warning: derived obsid < 400000\n");
    entry[i] = match[i];
  }
  llsortpair (index, entry, Nmatch);
  *Index = index;
  *Entry = entry;
}

/* match is a list of image entries with the same obsid */
double MosaicIQStats (RegImage *image, off_t *match, off_t Nmatch, MosaicRegion *region) {

  int i, N, Nccd;
  off_t j;
  double *list, value;

  Nccd = region[0].Nccd;
  ALLOCATE (list, double, Nccd);

  N = 0;
  for (i = 0; i < Nccd; i++) {
    for (j = 0; j < Nmatch; j++) {
      if (image[match[j]].ccd == region[0].ccd[i]) {
	list[N] = image[match[j]].fwhm;
	N++;
	break;
      }
    }
  }

  value = SigmaClipList (list, N);
  free (list);
  return (value);
}

double SigmaClipList (double *list, int N) {

  int i, n;
  double median, sigma3, m1, m2;

  if (N == 0) return (0.0);
  if (N == 1) return (list[0]);
  
  dsort (list, N);
  median = list[(int)(0.5*N)];
  
  m1 = m2 = 0;
  for (i = 0; i < N; i++) { m1 += list[i]; m2 += SQ(list[i]); }
  sigma3 = 3*sqrt (m2/N - m1*m1/N/N);
  
  m1 = n = 0;
  for (i = 0; i < N; i++) { 
    if (abs(list[i] - median) > sigma3) continue;
    m1 += list[i];
    n ++;
  }

  m2 = m1 / n;
  return (m2);
}
  

/* the CADC table is special: we need to report specfic IQ stats 
   which represent the variations in focus across the mosaic. 
   this representation is explicitly dependent on the mosaic,
   and does not make sense for other camera types 
   CADC table option cannot be combined with CCD filtering options
   CADC table forces -unique, needed to make calculation
*/

/* derived CADC parameters:
   OBS_IQ_CENTER - sigma-clip mean value of center region
   OBS_IQ_X      - ratio of left to right regions
   OBS_IQ_Y      - ratio of top to bottom regions
   OBS_IQ_R      - ratio of center to edge regions
*/

/* cfh12k:

   00 01 02 03 04 05
   06 07 08 09 10 11

   center: 02,03,08,09
   outer:  00,01,04,05,06,07,10,11
   top;    00-05
   bottom: 06-11
   right:  00,01,02,06,07,08
   left:   03,04,05,09,10,11

   for each region, calculate median, sigma, reject >3sigma, calculate mean
*/

/* we are guaranteed a unique set of filename / ccd values */
/* index = OBSID = atoi (filename) */

