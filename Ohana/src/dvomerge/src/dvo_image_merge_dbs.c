# include "dvomerge.h"

// utility functions
void sort_IDmap (IDmapType *IDmap);

void SortTgtByTimes (e_time *S, off_t *I, short *C, off_t N);
void SortTgtByExtID (unsigned int *S, off_t *I, off_t N);

off_t getTgtIndexByExtID (unsigned int extID, off_t *TgtIndex, unsigned int *TgtExtID, off_t NimagesTgt);
off_t getTgtIndexByTimeAndPhotcode (e_time start, e_time stop, short photcode, off_t *TgtIndex, e_time *TgtTimes, short *TgtCodes, off_t NimagesTgt);

void dvo_image_map_init (IDmapType *IDmap) {
  // we don't own the IDmap, just the elements
  IDmap->old = NULL;
  IDmap->new = NULL;
  IDmap->notFoundMeasure = NULL;
  IDmap->notFoundLensing = NULL;
  IDmap->Nmap = 0;
  IDmap->oldIDmax = 0;
}

void dvo_image_map_free (IDmapType *IDmap) {
  // we don't own the IDmap, just the elements
  FREE (IDmap->old);
  FREE (IDmap->new);
  FREE (IDmap->notFoundMeasure);
  FREE (IDmap->notFoundLensing);
}

// we have two tables; 'tgt' contains some exposures from 'src' : find them and match a map
int dvo_image_match_dbs_by_extern_id (IDmapType *IDmap, FITS_DB *tgt, FITS_DB *src) {

  Image *imagesSrc, *imagesTgt;
  off_t NimagesSrc, NimagesTgt;
  off_t iSrc, iTgt;
  off_t  *TgtIndex;
  unsigned int *TgtExtID;
 
  imagesSrc = gfits_table_get_Image (&src[0].ftable, &NimagesSrc, &src[0].scaledValue, &src[0].nativeOrder);
  if (!imagesSrc) {
    fprintf (stderr, "ERROR: failed to read images from src\n");
    exit (2);
  }

  imagesTgt = gfits_table_get_Image (&tgt[0].ftable, &NimagesTgt, &tgt[0].scaledValue, &tgt[0].nativeOrder);
  if (!imagesTgt) {
    fprintf (stderr, "ERROR: failed to read images from tgt\n");
    exit (2);
  }

  ALLOCATE (IDmap->old, unsigned int, NimagesSrc);
  ALLOCATE (IDmap->new, unsigned int, NimagesSrc);
  IDmap->Nmap = NimagesSrc;

  // match by time and photcode
  ALLOCATE (TgtIndex, off_t,        NimagesTgt);
  ALLOCATE (TgtExtID, unsigned int, NimagesTgt);

  // save the Index, Times, Codes for all TGT images
  for (iTgt = 0; iTgt < NimagesTgt; iTgt++) {
    TgtIndex[iTgt] = iTgt;
    TgtExtID[iTgt] = imagesTgt[iTgt].externID;
  }

  // sort the index, start, and stop by the start times:
  SortTgtByExtID (TgtExtID, TgtIndex, NimagesTgt);

  for (iSrc = 0; iSrc < NimagesSrc; iSrc++) {
    iTgt = getTgtIndexByExtID (imagesSrc[iSrc].externID, TgtIndex, TgtExtID, NimagesTgt);
    if ((iTgt < 0) && (!ALLOW_MISSING_INPUT_IMAGES)) {
      Shutdown ("failure to find matching image: %s\n", imagesSrc[iSrc].name);
    }
    IDmap[0].old[iSrc] = imagesSrc[iSrc].imageID;
    if (iTgt < 0) {
      IDmap[0].new[iSrc] = 0;
    } else {
      IDmap[0].new[iSrc] = imagesTgt[iTgt].imageID;
    }
  }

  create_IDmap_lookup (IDmap);

  FREE(TgtIndex);
  FREE(TgtExtID);

  // sort IDmap->old,new on the basis of IDmap->old:
  sort_IDmap (IDmap);

  return TRUE;
}

// we have two tables; 'tgt' contains some exposures from 'src' : find them and match a map
int dvo_image_match_dbs_by_time_and_photcode (IDmapType *IDmap, FITS_DB *tgt, FITS_DB *src) {

  Image *imagesSrc, *imagesTgt;
  off_t NimagesSrc, NimagesTgt;
  off_t iSrc, iTgt;
  off_t  *TgtIndex;
  e_time *TgtTimes;
  short  *TgtCodes;
 
  imagesSrc = gfits_table_get_Image (&src[0].ftable, &NimagesSrc, &src[0].scaledValue, &src[0].nativeOrder);
  if (!imagesSrc) {
    fprintf (stderr, "ERROR: failed to read images from src\n");
    exit (2);
  }

  imagesTgt = gfits_table_get_Image (&tgt[0].ftable, &NimagesTgt, &tgt[0].scaledValue, &tgt[0].nativeOrder);
  if (!imagesTgt) {
    fprintf (stderr, "ERROR: failed to read images from tgt\n");
    exit (2);
  }

  ALLOCATE (IDmap->old, unsigned int, NimagesSrc);
  ALLOCATE (IDmap->new, unsigned int, NimagesSrc);
  IDmap->Nmap = NimagesSrc;

  // match by time and photcode
  ALLOCATE (TgtIndex, off_t,  NimagesTgt);
  ALLOCATE (TgtTimes, e_time, NimagesTgt);
  ALLOCATE (TgtCodes, short,  NimagesTgt);

  // save the Index, Times, Codes for all TGT images
  for (iTgt = 0; iTgt < NimagesTgt; iTgt++) {
    TgtIndex[iTgt] = iTgt;
    TgtTimes[iTgt] = imagesTgt[iTgt].tzero;
    TgtCodes[iTgt] = imagesTgt[iTgt].photcode;
  }

  // sort the index, start, and stop by the start times:
  SortTgtByTimes (TgtTimes, TgtIndex, TgtCodes, NimagesTgt);

  for (iSrc = 0; iSrc < NimagesSrc; iSrc++) {
    iTgt = getTgtIndexByTimeAndPhotcode (imagesSrc[iSrc].tzero, imagesSrc[iSrc].tzero + (int) imagesSrc[iSrc].exptime, imagesSrc[iSrc].photcode, TgtIndex, TgtTimes, TgtCodes, NimagesTgt);
    if ((iTgt < 0) && (!ALLOW_MISSING_INPUT_IMAGES)) {
      Shutdown ("failure to find matching image: %s\n", imagesSrc[iSrc].name);
    }
    IDmap[0].old[iSrc] = imagesSrc[iSrc].imageID;
    IDmap[0].new[iSrc] = imagesTgt[iTgt].imageID;
  }

  create_IDmap_lookup (IDmap);

  FREE(TgtIndex);
  FREE(TgtTimes);
  FREE(TgtCodes);

  // sort IDmap->old,new on the basis of IDmap->old:
  sort_IDmap (IDmap);

  return TRUE;
}

// merge db2 into db1
int dvo_image_merge_dbs (IDmapType *IDmap, FITS_DB *out, FITS_DB *in) {

  Image *images;
  off_t Nimages;
  off_t Nout;
  off_t i, IDstart;
  int status;
 
  IDmap->old = NULL;
  IDmap->new = NULL;

  // it is OK if there are no images in the database, but there should be no imageIDs to map...
  if (in->dbstate == LCK_EMPTY) {
    return TRUE;
  }

  images = gfits_table_get_Image (&in[0].ftable, &Nimages, &in[0].scaledValue, &in[0].nativeOrder);
  if (!images) {
    fprintf (stderr, "ERROR: failed to read images\n");
    exit (2);
  }

  ALLOCATE (IDmap->old, unsigned int, Nimages);
  ALLOCATE (IDmap->new, unsigned int, Nimages);
  IDmap->Nmap = Nimages;

 /* adjust header */
  Nout = 0;
  gfits_scan (&out[0].header, "NIMAGES", OFF_T_FMT, 1,  &Nout);
  status = gfits_scan (&out[0].header, "IMAGEID", OFF_T_FMT, 1,  &IDstart);
  if (!status) {
    IDstart = 1;
  }

  for (i = 0; i < Nimages; i++) {
    IDmap[0].old[i] = images[i].imageID;
    images[i].imageID = i + IDstart;
    IDmap[0].new[i] = images[i].imageID;
  }


  // sort IDmap->old,new on the basis of IDmap->old:
  sort_IDmap (IDmap);

  create_IDmap_lookup (IDmap);

  Nout += Nimages;
  IDstart += Nimages;
  gfits_modify (&out[0].header, "NIMAGES", OFF_T_FMT, 1,  Nout);
  gfits_modify (&out[0].header, "IMAGEID", OFF_T_FMT, 1,  IDstart);

  gfits_add_rows (&out[0].ftable, (char *) images, Nimages, sizeof(Image));
  return TRUE;
}

# if (0)
// merge db2 into db1, skipping any from 'src' with the same EXTERN_ID as 'tgt'
int dvo_image_merge_dbs_skip_duplicates (IDmapType *IDmap, FITS_DB *tgt, FITS_DB *stc) {

  off_t NimagesSrc, NimagesTgt;
  off_t Ntgt;
  off_t i, IDstart;
 
  IDmap->old = NULL;
  IDmap->new = NULL;

  // it is OK if there are no images in the database, but there should be no imageIDs to map...
  if (src->dbstate == LCK_EMPTY) {
    return TRUE;
  }

  // load both image tables
  Images *imagesSrc = gfits_table_get_Image (&src[0].ftable, &NimagesSrc, &src[0].scaledValue, &src[0].nativeOrder);
  if (!imagesSrc) {
    fprintf (stderr, "ERROR: failed to read images (src)\n");
    exit (2);
  }

  Images *imagesTgt = gfits_table_get_Image (&tgt[0].ftable, &NimagesTgt, &tgt[0].scaledValue, &tgt[0].nativeOrder);
  if (!imagesTgt) {
    fprintf (stderr, "ERROR: failed to read images (tgt)\n");
    exit (2);
  }

  // generate an index for target extern IDs:
  off_t  *TgtIndex;
  unsigned int *TgtExtID;
  ALLOCATE (TgtIndex, off_t,        NimagesTgt);
  ALLOCATE (TgtExtID, unsigned int, NimagesTgt);

  // save the Index & EXTERN_ID for all TGT images
  off_t iSrc, iTgt;
  for (iTgt = 0; iTgt < NimagesTgt; iTgt++) {
    TgtIndex[iTgt] = iTgt;
    TgtExtID[iTgt] = imagesTgt[iTgt].externID;
  }

  // sort the index, start, and stop by the start times:
  SortTgtByExtID (TgtExtID, TgtIndex, NimagesTgt);

  // identify the src images which are NOT in tgt
  off_t *keepSrc = NULL;
  ALLOCATE (keepSrc, off_t, NimagesSrc);
  memset (keepSrc, 0, NimagesSrc*sizeof(off_t));

  off_t Nkeep = 0;
  for (iSrc = 0; iSrc < NimagesSrc; iSrc++) {
    iTgt = getTgtIndexByExtID (imagesSrc[iSrc].externID, TgtIndex, TgtExtID, NimagesTgt);
    if (iTgt >= 0) continue;
    keepSrc[iSrc] = 1;
    Nkeep ++;
  }

  // append the new image data to the end of the tgt structure, adjusting imageID as we go:
  int status = gfits_scan (&tgt[0].header, "IMAGEID", OFF_T_FMT, 1,  &IDstart);
  if (!status) {
    IDstart = 1;
  }

  off_t jSrc = 0;
  REALLOCATE (imagesTgt, Image, NimagesTgt + Nkeep);

  for (iSrc = 0; iSrc < NimagesSrc; iSrc++) {
    if (!keepSrc[iSrc]) continue; // skip images already in the image table
    imagesTgt[NimagesTgt + jSrc] = imagesSrc[iSrc];
    imagesTgt[NimagesTgt + jSrc].imageID = jSrc + IDstart;
    jSrc ++;
  }
  myAssert (jSrc == Nkeep, "oops");

  Ntgt += Nkeep;
  IDstart += Nkeep;
  gfits_modify (&tgt[0].header, "NIMAGES", OFF_T_FMT, 1,  Ntgt);
  gfits_modify (&tgt[0].header, "IMAGEID", OFF_T_FMT, 1,  IDstart);

  // XXX need to update ftable with the full Image table
  // gfits_add_rows (&tgt[0].ftable, (char *) images, Nimages, sizeof(Image));
  return TRUE;
}
# endif

// map this ID to the new table
// XXX isn't the map just ID_new = ID_old + offset ?? (probably not)
off_t dvo_map_image_ID (IDmapType *IDmap, off_t oldID) {

  // off_t i;
  // 
  // for (i = 0; i < IDmap->Nmap; i++) {
  //   if (IDmap->old[i] != oldID) continue;
  //   return (IDmap->new[i]);
  // }

  off_t Nlo, Nhi, N;

  if (!IDmap->Nmap) return 0;

  // find the a close entry below desired ID
  Nlo = 0; Nhi = IDmap->Nmap;
  while (Nhi - Nlo > 10) {
    N = 0.5*(Nlo + Nhi);
    if (IDmap->old[N] < oldID) {
      Nlo = MAX(N, 0);
    } else {
      Nhi = MIN(N + 1, IDmap->Nmap);
    }
  }

  // search for the desired ID starting from Nlo, give up at Nhi
  for (N = Nlo; N < Nhi; N++) { 
    if (IDmap->old[N] < oldID) continue;
    if (IDmap->old[N] > oldID) return 0;
    return (IDmap->new[N]);
  }
  return 0;
}

// given a table of detections, update their image IDs based on the new map
int dvo_update_image_IDs (IDmapType *IDmap, Catalog *catalog) {

  off_t i, oldID, newID;

  if (!IDmap->old) {
    fprintf (stderr, "input database has image IDs, but no Image table\n");
    return FALSE;
  }

  if (!IDmap->Nmap) {
    fprintf (stderr, "input database has image IDs, but no Image table\n");
    return FALSE;
  }

  off_t lastID = IDmap->old[IDmap->Nmap-1];

  // update measure.imageID
  for (i = 0; i < catalog[0].Nmeasure; i++) {
    oldID = catalog[0].measure[i].imageID;
    if (oldID == 0) continue;

    newID = dvo_map_image_ID (IDmap, oldID);
    if (newID == 0) {
      if (oldID > lastID) {
	// this is a case where the measure->imageIDs are not consistent with the image->imageIDs
	fprintf (stderr, "problem with image IDs (measure) : input out of range\n");
	fprintf (stderr, "old ID: "OFF_T_FMT", last ID: "OFF_T_FMT"\n", oldID, lastID);
	exit (2);
      }
      if (!IDmap->notFoundMeasure[oldID]) {
	// this is a case where the measure->imageIDs are not found in the OUTPUT image->imageIDs
	// once we discover an imageID is not found, record that fact so we do not complain for every detection
	fprintf (stderr, "cannot find image ID "OFF_T_FMT" with at least one measure\n",  oldID);
      }
      IDmap->notFoundMeasure[oldID]++;
      // optionally exit here? or wait until end to report an error?
      // exit (2);
    }
    catalog[0].measure[i].imageID = newID;
  }

  // also update lensing.imageID if lensing exists
  for (i = 0; i < catalog[0].Nlensing; i++) {
    oldID = catalog[0].lensing[i].imageID;
    if (oldID == 0) continue;

    newID = dvo_map_image_ID (IDmap, oldID);
    if (newID == 0) {
      if (oldID > lastID) {
	// this is a case where the measure->imageIDs are not consistent with the image->imageIDs
	fprintf (stderr, "problem with image IDs (lensing) : input out of range\n");
	fprintf (stderr, "old ID: "OFF_T_FMT", last ID: "OFF_T_FMT"\n", oldID, lastID);
	exit (2);
      }
      if (!IDmap->notFoundLensing[oldID]) {
	// once we discover an imageID is not found, record that fact so we do not complain for every detection
	fprintf (stderr, "cannot find image ID "OFF_T_FMT"\n",  oldID);
      }
      IDmap->notFoundLensing[oldID] ++;
      // optionally exit here? or wait until end to report an error?
      // exit (2);
    }
    catalog[0].lensing[i].imageID = newID;
  }

  // NOTE galphot also carries an imageID, but it is actually the EXTERNAL image ID reference.  Do NOT update it.

  return TRUE;
}

void dvo_report_image_IDs (IDmapType *IDmap) {

  off_t i;

  for (i = 0; IDmap->notFoundMeasure && (i < IDmap->oldIDmax + 1); i++) {
    if (!IDmap->notFoundMeasure[i]) continue;
    fprintf (stderr, "SKIP IMAGE MEASURE: "OFF_T_FMT", %d measures\n", i, IDmap->notFoundMeasure[i]);
  }
  for (i = 0; IDmap->notFoundLensing && (i < IDmap->oldIDmax + 1); i++) {
    if (!IDmap->notFoundLensing[i]) continue;
    fprintf (stderr, "SKIP IMAGE LENSING: "OFF_T_FMT", %d lensings\n", i, IDmap->notFoundLensing[i]);
  }
  return;
}

// sort two times vectors and an index by first time vector
void sort_IDmap (IDmapType *IDmap) {

# define SWAPFUNC(A,B){ off_t tmp_old, tmp_new;	   \
  tmp_old = IDmap->old[A]; IDmap->old[A] = IDmap->old[B]; IDmap->old[B] = tmp_old; \
  tmp_new = IDmap->new[A]; IDmap->new[A] = IDmap->new[B]; IDmap->new[B] = tmp_new; \
}
# define COMPARE(A,B)(IDmap->old[A] < IDmap->old[B])

  OHANA_SORT (IDmap->Nmap, COMPARE, SWAPFUNC);

# undef SWAPFUNC
# undef COMPARE

}

off_t getTgtIndexByExtID (unsigned int extID, off_t *TgtIndex, unsigned int *TgtExtID, off_t NimagesTgt) {

  // use bisection to find the starting entry by time

  off_t Nlo, Nhi, N;

  // find the last TGT before start
  Nlo = 0; Nhi = NimagesTgt;
  while (Nhi - Nlo > 10) {
    N = 0.5*(Nlo + Nhi);
    if (TgtExtID[N] < extID) {
      Nlo = MAX(N, 0);
    } else {
      Nhi = MIN(N + 1, NimagesTgt);
    }
  }

  // check for the matched mosaic starting from Nlo 
  // we may have to go much beyond Nlo since stop is not sorted
  // can we use a sorted version of stop to check when we are beyond the valid range??
  for (N = Nlo; N < NimagesTgt; N++) { 
    if (TgtExtID[N] < extID) continue;
    if (TgtExtID[N] > extID) return (-1);
    return (TgtIndex[N]);
  }
  return (-1);
}

off_t getTgtIndexByTimeAndPhotcode (e_time start, e_time stop, short photcode, off_t *TgtIndex, e_time *TgtTimes, short *TgtCodes, off_t NimagesTgt) {

  // use bisection to find the starting entry by time

  off_t Nlo, Nhi, N;

  // find the last TGT before start
  Nlo = 0; Nhi = NimagesTgt;
  while (Nhi - Nlo > 10) {
    N = 0.5*(Nlo + Nhi);
    if (TgtTimes[N] < start) {
      Nlo = MAX(N, 0);
    } else {
      Nhi = MIN(N + 1, NimagesTgt);
    }
  }

  // check for the matched mosaic starting from Nlo 
  // we may have to go much beyond Nlo since stop is not sorted
  // can we use a sorted version of stop to check when we are beyond the valid range??
  for (N = Nlo; N < NimagesTgt; N++) { 
    if (TgtTimes[N] < start) continue;
    if (TgtTimes[N] > stop) return (-1);
    if (TgtCodes[N] != photcode) continue;
    return (TgtIndex[N]);
  }
  return (-1);
}

// sort two times vectors and an index by first time vector
void SortTgtByTimes (e_time *S, off_t *I, short *C, off_t N) {

# define SWAPFUNC(A,B){ e_time tmp_t; off_t tmp_i; short tmp_c; 	\
  tmp_t = S[A]; S[A] = S[B]; S[B] = tmp_t; \
  tmp_i = I[A]; I[A] = I[B]; I[B] = tmp_i; \
  tmp_c = C[A]; C[A] = C[B]; C[B] = tmp_c; \
}
# define COMPARE(A,B)(S[A] < S[B])

  OHANA_SORT (N, COMPARE, SWAPFUNC);

# undef SWAPFUNC
# undef COMPARE

}

// sort two times vectors and an index by first time vector
void SortTgtByExtID (unsigned int *S, off_t *I, off_t N) {

# define SWAPFUNC(A,B){ unsigned int tmp_s; off_t tmp_i; 	\
  tmp_s = S[A]; S[A] = S[B]; S[B] = tmp_s; \
  tmp_i = I[A]; I[A] = I[B]; I[B] = tmp_i; \
}
# define COMPARE(A,B)(S[A] < S[B])

  OHANA_SORT (N, COMPARE, SWAPFUNC);

# undef SWAPFUNC
# undef COMPARE
}

// we need a lookup table of length (oldIDmax + 1) to record if a given oldIDmax has not been found in Tgt
int create_IDmap_lookup (IDmapType *IDmap) {
  
  off_t i;
  unsigned int oldIDmax = 0;

  for (i = 0; i < IDmap->Nmap; i++) {
    if (oldIDmax > IDmap->old[i]) continue;
    oldIDmax = IDmap->old[i];
  }
  IDmap->oldIDmax = oldIDmax;

  ALLOCATE_ZERO (IDmap->notFoundMeasure, int, oldIDmax + 1);
  ALLOCATE_ZERO (IDmap->notFoundLensing, int, oldIDmax + 1);
  return TRUE;
}
