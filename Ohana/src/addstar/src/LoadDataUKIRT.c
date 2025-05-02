# include "addstar.h"

// examine the header sets and set the Image entries for the the valid images
// UKIRT data has the WCS/image metadata header intermixed with the bintable header

int MergeCatalogs (Catalog *tgt, Catalog *src);

Catalog *LoadDataUKIRT (FILE *f, char *imagename, Image **images, off_t *nimages, Header **headers, off_t *extsize, HeaderSet *headerSets, off_t NheaderSets, SkyRegion *region) {

  off_t Nskip, Nvalid, NVALID;
  int i, j, Nhead, Ndata;

  Catalog *catalog = NULL;
  ALLOCATE (catalog, Catalog, 1);
  dvo_catalog_init (catalog, TRUE);
  ALLOCATE (catalog->average, Average, 1);
  ALLOCATE (catalog->measure, Measure, 1);

  // create or update image table 
  if (images[0] == NULL) {
    Nvalid = 0;
    NVALID = NheaderSets;
    ALLOCATE (images[0], Image, NVALID);
  } else {
    Nvalid = *nimages;
    NVALID = Nvalid + NheaderSets;
    REALLOCATE (images[0], Image, NVALID);
  }    

  // validate the number of headers sets == 4
  for (i = 0; i < NheaderSets; i++) {
    if (VERBOSE) fprintf (stderr, "reading header for %s (%s)\n", headerSets[i].exthead, headerSets[i].extdata);

    // advance the pointer to the start of the corresponding table block
    Nhead = headerSets[i].extnum_head;
    Ndata = headerSets[i].extnum_data;
    Nskip = 0;
    for (j = 0; j < Ndata; j++) {
      Nskip += extsize[j];
    }
    fseeko (f, Nskip, SEEK_SET); 

    Catalog *newcat = ReadStarsUKIRT (f, imagename, headers[Nhead], images[0], &Nvalid, region);
    MergeCatalogs (catalog, newcat);

    dvo_catalog_free (newcat);
    free (newcat);

    *nimages = Nvalid;
  }    
  return (catalog);
}

int MergeCatalogs (Catalog *tgt, Catalog *src) {

  // this function assumes the src and tgt catalog do NOT overlap (otherwise just use find_match_closest) 

  off_t Naverage_tgt = tgt->Naverage;
  off_t Nmeasure_tgt = tgt->Nmeasure;

  tgt->Naverage += src->Naverage;
  tgt->Nmeasure += src->Nmeasure;

  REALLOCATE (tgt->average, Average, tgt->Naverage);
  REALLOCATE (tgt->measure, Measure, tgt->Nmeasure);

  off_t i;
  for (i = 0; i < src->Naverage; i++) {
    tgt->average[i+Naverage_tgt] = src->average[i];
    tgt->average[i+Naverage_tgt].measureOffset += Nmeasure_tgt;
  }

  for (i = 0; i < src->Nmeasure; i++) {
    tgt->measure[i+Nmeasure_tgt] = src->measure[i];
  }

  return TRUE;
}
