# include "delstar.h"

/* load information about image from image header
 * this should be the same as addstar/gstars, but it is not...
 */

Image *gimages (char *filename) {
 
  FILE *f;
  Header header;
  char photcode[64], *c;
  double tmp;
  Image *image;
  int Nc, haveNx, haveNy;

  ALLOCATE (image, Image, 1);
  /* load header */
  if (!gfits_read_header (filename, &header)) {
    Shutdown ("ERROR: can't find image file %s", filename);
  }

  /* open file */
  f = fopen (filename, "r");
  if (f == NULL) {
    Shutdown ("ERROR: can't find data file %s", filename);
  }
  fseeko (f, header.datasize, SEEK_SET); 

  /* add file name to image structure */
  c = strrchr (filename, 0x2f);
  if (c == (char *) NULL) {
    strcpy (image[0].name, filename);
  } else { 
    strcpy (image[0].name, (c+1));
  }

  /* get astrometry information */
  if (!GetCoords (&image[0].coords, &header)) {
    Shutdown ("ERROR: no astrometric solution in header");
  }
  
  image[0].coords.crval1 = ohana_normalize_angle (image[0].coords.crval1);

  /* CERROR in data file is in pixels, convert to 20*arcsec */
  image[0].cerror = tmp * 50.0 * image[0].coords.cdelt1 * 3600.0;
 
  /* get other header info */
  haveNx = gfits_scan (&header, "NAXIS1",   "%hu", 1, &image[0].NX); 
  haveNy = gfits_scan (&header, "NAXIS2",   "%hu", 1, &image[0].NY);
  if (!haveNx && !haveNy) {
      haveNx = gfits_scan (&header, "IMNAXIS1",   "%hu", 1, &image[0].NX); 
      haveNy = gfits_scan (&header, "IMNAXIS2",   "%hu", 1, &image[0].NY);
  }      
  if (!haveNx && !haveNy) {
      haveNx = gfits_scan (&header, "ZNAXIS1",   "%hu", 1, &image[0].NX);
      haveNy = gfits_scan (&header, "ZNAXIS2",   "%hu", 1, &image[0].NY);
  }
  if (!haveNx || !haveNy) {
      Shutdown ("ERROR: missing image dimensions in header");
  }

  gfits_scan (&header, "PHOTCODE", "%s", 1, photcode);
  Nc = GetPhotcodeCodebyName (photcode);
  if (!Nc) {
    Shutdown ("ERROR: photcode %s not found in photcode table", photcode);
  }
  image[0].photcode = Nc;

  tmp = 0;
  gfits_scan (&header, "FLIMIT",   "%lf", 1, &tmp);
  image[0].detection_limit = tmp * 10.0;

  tmp = 0;
  gfits_scan (&header, "FSATUR",   "%lf", 1, &tmp);
  image[0].saturation_limit = tmp * 10.0;

  if (!gfits_scan (&header, "TZERO",   "%d",  1, &image[0].tzero)) {
    image[0].tzero = parse_time (&header);
  }

  tmp = 0;
  gfits_scan (&header, "TRATE",   "%lf", 1, &tmp);
  image[0].trate = 10000 * tmp;

  tmp = 0;
  gfits_scan (&header, "AIRMASS", "%lf", 1, &tmp);
  image[0].secz = tmp;

  /* secz is in units milli-airmass */
  image[0].McalPSF   = ALPHA*(image[0].secz - 1.000);
  image[0].McalAPER  = ALPHA*(image[0].secz - 1.000);
  image[0].McalChiSq = NAN_S_SHORT;

  free (header.buffer);
 
  return (image);
}
