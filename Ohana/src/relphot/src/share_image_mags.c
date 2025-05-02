# include "relphot.h"

// we are sharing image calibrations mags for all images which (a) I own and (b) which have unowned detections

# define D_NIMAGEMAGS 1000
int share_image_mags (RegionHostTable *regionHosts, int nloop) {

  off_t i, Nimages;
  Image *images = getimages (&Nimages, NULL);

  off_t Nimage_mags = 0;
  off_t NIMAGE_MAGS = D_NIMAGEMAGS;
  
  ImageMag *image_mags = NULL;
  ALLOCATE (image_mags, ImageMag, NIMAGE_MAGS);

  for (i = 0; i < Nimages; i++) {
    // XXX does this image have missing detections (does someone else need it?)
    // XXX : NOTE NEED TO FIX THIS: if (imageExtra[i].Nmiss == 0) continue;
    
    set_image_mags (&image_mags[Nimage_mags], &images[i]);
    Nimage_mags ++;

    CHECK_REALLOCATE (image_mags, ImageMag, NIMAGE_MAGS, Nimage_mags, D_NIMAGEMAGS);
  }

  // write out the image_mag fits table AND write state in some file
  int myHost = regionHosts->index[REGION_HOST_ID];
  char *hostname = regionHosts->hosts[myHost].hostname;

  char *imagfile = make_filename (CATDIR, hostname, REGION_HOST_ID, "imagemags.fits");
  ImageMagSave (imagfile, image_mags, Nimage_mags);
  free (image_mags);
  free (imagfile);

  char *syncfile = make_filename (CATDIR, hostname, REGION_HOST_ID, "imagemags.sync");
  update_sync_file (syncfile, nloop);
  free (syncfile);

  return TRUE;
}

int slurp_image_mags (RegionHostTable *regionHosts, int nloop) {

  off_t Nimage, i;
  Image *images = getimages (&Nimage, NULL);

  int Nimage_mags = 0;
  ImageMag *image_mags = NULL;
  ALLOCATE (image_mags, ImageMag, 1);

  fprintf (stderr, "grabbing image mags from other hosts...\n");

  for (i = 0; i < regionHosts->Nhosts; i++) {
    if (regionHosts->hosts[i].hostID == REGION_HOST_ID) continue;
    if (REGION_HOST_ID && !regionHosts->hosts[i].isNeighbor) continue;

    char *syncfile = make_filename (CATDIR, regionHosts->hosts[i].hostname, regionHosts->hosts[i].hostID, "imagemags.sync");
    check_sync_file (syncfile, nloop);
    free (syncfile);
    
    off_t Nsubset;
    char *imagfile = make_filename (CATDIR, regionHosts->hosts[i].hostname, regionHosts->hosts[i].hostID, "imagemags.fits");
    ImageMag *image_mags_subset = ImageMagLoad (imagfile, &Nsubset);
    free (imagfile);

    image_mags = merge_image_mags (image_mags, &Nimage_mags, image_mags_subset, Nsubset);
  }

  for (i = 0; i < Nimage_mags; i++) {
    off_t seq = getImageByID (image_mags[i].imageID);
    if (seq < 0) {
      // XXX is this a problem? (no, other hosts don't know which images I own)
      continue;
    }
    images[seq].McalPSF     = image_mags[i].McalPSF;
    images[seq].McalAPER    = image_mags[i].McalAPER;
    images[seq].dMcal  	    = image_mags[i].dMcal;
    images[seq].dMagSys	    = image_mags[i].dMagSys;
    images[seq].McalChiSq   = image_mags[i].McalChiSq;
    images[seq].nFitPhotom  = image_mags[i].nFitPhotom;
    images[seq].flags 	    = image_mags[i].flags;
    images[seq].ubercalDist = image_mags[i].ubercalDist;
  }
  free (image_mags);

  fprintf (stderr, "DONE grabbing image mags from other hosts\n");

  return TRUE;
}

int set_image_mags (ImageMag *image_mags, Image *image) {

  image_mags->McalPSF  	  = image->McalPSF;
  image_mags->McalAPER 	  = image->McalAPER;
  image_mags->dMcal  	  = image->dMcal;
  image_mags->dMagSys	  = image->dMagSys;
  image_mags->McalChiSq	  = image->McalChiSq;
  image_mags->nFitPhotom  = image->nFitPhotom;
  image_mags->flags 	  = image->flags;
  image_mags->ubercalDist = image->ubercalDist;
  image_mags->imageID     = image->imageID;

  return TRUE;
}

ImageMag *merge_image_mags (ImageMag *target, int *ntarget, ImageMag *source, int Nsource) {

  off_t i;

  REALLOCATE (target, ImageMag, *ntarget + Nsource);
  for (i = 0; i < Nsource; i++) {
    off_t n = i + *ntarget;
    target[n] = source[i];
  }
  
  free (source);

  *ntarget += Nsource;
  return (target);
}

