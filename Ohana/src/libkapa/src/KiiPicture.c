# include <kapa_internal.h>

int KiiSetChannel (int fd, int channel) {

  KiiSendCommand (fd, 4, "CHAN"); /* tell kapa to look for the incoming image */
  KiiSendMessage (fd, "%1d", channel);
  
  KiiWaitAnswer (fd, "DONE");
  return (TRUE);
}

int KiiSetColormap (int fd, char *colormap) {

  KiiSendCommand (fd, 4, "CMAP"); /* tell kapa to look for the incoming image */
  KiiSendMessage (fd, "%s", colormap);
  
  KiiWaitAnswer (fd, "DONE");
  return (TRUE);
}

int KiiSetNanColor (int fd, int red, int green, int blue) {

  KiiSendCommand (fd, 4, "CNAN"); /* tell kapa to look for the incoming image */
  KiiSendMessage (fd, "%d %d %d ", red, green, blue);
  
  KiiWaitAnswer (fd, "DONE");
  return (TRUE);
}

int KiiNewPicture1D (int fd, KiiImage *image, KapaImageData *data, Coords *coords) {

  int Ncolors;
  off_t Nwrite, Npix, size;
  float min, max;

  Npix = image[0].Nx*image[0].Ny;

  KiiSendCommand (fd, 4, "READ"); /* tell kapa to look for the incoming image */
  KiiScanMessage (fd, "%d", &Ncolors);

  /* these are for a future upgrade */
  min = max = 0.0;
  size = Npix*sizeof(float);

  /* done with the conversion, now send kapa the converted picture */
  KiiSendMessage (fd, "%8d %8d", image[0].Nx, image[0].Ny);
  KiiSendMessage (fd, "%f %f %s %s", data[0].zero, data[0].range, data[0].name, data[0].file);
  KiiSendMessage (fd, "%f %f "OFF_T_FMT" ", min, max, size);
  KiiSendMessage (fd, "%f %f %g %g %g ", coords[0].crval1, coords[0].crpix1, coords[0].cdelt1, coords[0].pc1_1, coords[0].pc1_2);
  KiiSendMessage (fd, "%f %f %g %g %g ", coords[0].crval2, coords[0].crpix2, coords[0].cdelt2, coords[0].pc2_1, coords[0].pc2_2);
  KiiSendMessage (fd, "%s", coords[0].ctype);

  /* send the image data */
  off_t bytes_left = size;
  while (bytes_left > 0) {
    Nwrite = write (fd, image[0].data1d, bytes_left);
    if (Nwrite == 0) {
      fprintf (stderr, "unable to send more data to kapa?\n");
      return FALSE;
    }
    if (Nwrite < 0) {
      if (errno == EAGAIN) continue;
      if (errno == EWOULDBLOCK) continue;
      perror ("KiiNewPicture1D:");
      return FALSE;
    }
    bytes_left -= Nwrite;
  }

  KiiWaitAnswer (fd, "DONE");
  return (TRUE);
}

int KiiNewPicture1D_8bit (int fd, KiiImage *image, KapaImageData *data, Coords *coords) {

  int i;
  int Nwrite, Npix, Ncolors, NNcolors, size;
  float *in, min, max;
  char *out, *outbuffer;
  double a1, a2;

  Npix = image[0].Nx*image[0].Ny;

  KiiSendCommand (fd, 4, "READ"); /* tell kapa to look for the incoming image */
  KiiScanMessage (fd, "%d", &Ncolors);

  ALLOCATE (outbuffer, char, Npix);
  out = outbuffer;
  in = image[0].data1d;

  /* need to invert the logic if range < 0 */

  /* define color table, */
  NNcolors = Ncolors - 1;
  if (data[0].logflux) {
    data[0].range = MAX (2, data[0].range);
    a1 = Ncolors / log10 (data[0].range);
    a2 = data[0].zero;
    for (i = 0; i < Npix; i++, in++, out++) {
      *out = (char) MIN (a1 * log10 (MAX (*in - a2, 1.0)), NNcolors);
    }
  } else {
    a1 = Ncolors / data[0].range;
    a2 = Ncolors * data[0].zero / data[0].range;
    for (i = 0; i < Npix; i++, in++, out++) {
      *out = (char) MIN (MAX (a1 * *in - a2, 0), NNcolors);
    }
  }
  
  /* these are for a future upgrade */
  min = max = 0.0;
  size = Npix*sizeof(char);

  /* done with the conversion, now send kapa the converted picture */
  KiiSendMessage (fd, "%8d %8d", image[0].Nx, image[0].Ny);
  KiiSendMessage (fd, "%f %f %s %s", data[0].zero, data[0].range, data[0].name, data[0].file);
  KiiSendMessage (fd, "%f %f %d ", min, max, size);
  KiiSendMessage (fd, "%f %f %g %g %g ", coords[0].crval1, coords[0].crpix1, coords[0].cdelt1, coords[0].pc1_1, coords[0].pc1_2);
  KiiSendMessage (fd, "%f %f %g %g %g ", coords[0].crval2, coords[0].crpix2, coords[0].cdelt2, coords[0].pc2_1, coords[0].pc2_2);
  KiiSendMessage (fd, "%s", coords[0].ctype);

  /* send the image data */
  Nwrite = write (fd, outbuffer, size);
  if (Nwrite != size) {
    fprintf (stderr, "error sending picture to kapa\n");
    return (FALSE);
  }
  free (outbuffer);

  KiiWaitAnswer (fd, "DONE");
  return (TRUE);
}

int KiiNewPicture2D (int fd, KiiImage *image, KapaImageData *data, Coords *coords) {

  int Ncolors;
  off_t j, Nwrite, Nbytes, Npix, size;
  float min, max;

  Npix = image[0].Nx*image[0].Ny;

  KiiSendCommand (fd, 4, "READ"); /* tell kapa to look for the incoming image */
  KiiScanMessage (fd, "%d", &Ncolors);

  /* these are for a future upgrade */
  min = max = 0.0;
  size = Npix*sizeof(float);

  /* done with the conversion, now send kapa the converted picture */
  KiiSendMessage (fd, "%8d %8d", image[0].Nx, image[0].Ny);
  KiiSendMessage (fd, "%f %f %s %s", data[0].zero, data[0].range, data[0].name, data[0].file);
  KiiSendMessage (fd, "%f %f "OFF_T_FMT" ", min, max, size);
  KiiSendMessage (fd, "%f %f %g %g %g ", coords[0].crval1, coords[0].crpix1, coords[0].cdelt1, coords[0].pc1_1, coords[0].pc1_2);
  KiiSendMessage (fd, "%f %f %g %g %g ", coords[0].crval2, coords[0].crpix2, coords[0].cdelt2, coords[0].pc2_1, coords[0].pc2_2);
  KiiSendMessage (fd, "%s", coords[0].ctype);

  /* send the image data */

  for (j = 0; j < image[0].Ny; j++) {
    Nbytes = image[0].Nx*sizeof(float);
    Nwrite = write (fd, image[0].data2d[j], Nbytes);
    if (Nwrite != Nbytes) {
      fprintf (stderr, "error sending picture to kapa\n");
      return (FALSE);
    }
  }

  KiiWaitAnswer (fd, "DONE");
  return (TRUE);
}

int KapaSetImageCoords (int fd, Coords *coords) {

  /* tell kapa to look for the incoming image */
  KiiSendCommand (fd, 4, "SIMC"); 
  
  KiiSendMessage (fd, "%g %g %g %g", 
		  coords[0].pc1_1, coords[0].pc2_2,
		  coords[0].pc1_2, coords[0].pc2_1);

  KiiSendMessage (fd, "%s", coords[0].ctype);

  KiiSendMessage (fd, "%g %g %g %g %g %g", 
		  coords[0].crval1,
		  coords[0].crval2,
		  coords[0].crpix1,
		  coords[0].crpix2,
		  coords[0].cdelt1,
		  coords[0].cdelt2);

  KiiWaitAnswer (fd, "DONE");
  return (TRUE);
}

int KapaGetImageCoords (int fd, Coords *coords) {

  /* tell kapa to look for the incoming image */
  KiiSendCommand (fd, 4, "GIMC"); 
  
  KiiScanMessage (fd, "%f %f %f %f", 
		  &coords[0].pc1_1, &coords[0].pc2_2,
		  &coords[0].pc1_2, &coords[0].pc2_1);

  KiiScanMessage (fd, "%s", coords[0].ctype);

  KiiScanMessage (fd, "%lf %lf %f %f %f %f", 
		  &coords[0].crval1,
		  &coords[0].crval2,
		  &coords[0].crpix1,
		  &coords[0].crpix2,
		  &coords[0].cdelt1,
		  &coords[0].cdelt2);

  // XXX at some point, we need to add polynomials and 2-level mosaic
  // astrometry here.

  coords[0].Npolyterms = 0;

  KiiWaitAnswer (fd, "DONE");
  return (TRUE);
}

int KapaGetImageRange (int fd, double *Xmin, double *Xmax, double *Ymin, double *Ymax, int *dX, int *dY) {

  /* tell kapa to look for the incoming image */
  KiiSendCommand (fd, 4, "GIMR"); 
  
  KiiScanMessage (fd, "%lf %lf %lf %lf %d %d", Xmin, Xmax, Ymin, Ymax, dX, dY);

  KiiWaitAnswer (fd, "DONE");
  return (TRUE);
}


/* this function should be broken into pieces: 
   KiiSendImage
   KiiSendCoords (default to 0 otherwise)
   KiiSendFilename
*/
