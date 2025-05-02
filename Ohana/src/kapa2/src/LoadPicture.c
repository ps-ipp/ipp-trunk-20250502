# include "Ximage.h"

int LoadPicture (int sock) {

  Header header;
  char *buff;
  off_t status, bytes_left;
  Section *section;
  KapaImageWidget *image;
  Graphic *graphic;
  double Xoffset, Yoffset, wx;

  graphic = GetGraphic ();
  section = GetActiveSection();
  if (section->image == NULL) {
    section->image = InitImageWidget ();
    SetSectionSizes (section);
  }
  image = section->image;
  
  KiiSendMessage (sock, "%d", graphic->Npixels);

  // when we load a new picture, use the same orientation as the old picture
  Xoffset = 0.0;
  Yoffset = 0.0;
  if (image[0].image[0].matrix.datasize) {
    // XXX enforce int center here?
    Xoffset = image[0].picture.Xc - 0.5*image[0].image[0].matrix.Naxis[0];
    Yoffset = image[0].picture.Yc - 0.5*image[0].image[0].matrix.Naxis[1];
  }

  gfits_init_header (&header);
  header.Naxes = 2;
  KiiScanMessage (sock, OFF_T_FMT" "OFF_T_FMT,  &header.Naxis[0],  &header.Naxis[1]);

  // internal image are 32 bit floats; sender must send in this format
  header.bitpix = -32;
  header.unsign = 1;
  header.bzero = 0.0;
  header.bscale = 1.0;

  KiiScanMessage (sock, "%lf %lf %s %s",  &image[0].image[0].zero, &image[0].image[0].range, image[0].image[0].name, image[0].image[0].file);
  KiiScanMessage (sock, "%lf %lf "OFF_T_FMT, &image[0].image[0].min,  &image[0].image[0].max,  &header.datasize);
  KiiScanMessage (sock, "%lf %f %f %f %f", &image[0].image[0].coords.crval1, &image[0].image[0].coords.crpix1, &image[0].image[0].coords.cdelt1, &image[0].image[0].coords.pc1_1, &image[0].image[0].coords.pc1_2);
  KiiScanMessage (sock, "%lf %f %f %f %f", &image[0].image[0].coords.crval2, &image[0].image[0].coords.crpix2, &image[0].image[0].coords.cdelt2, &image[0].image[0].coords.pc2_1, &image[0].image[0].coords.pc2_2);
  KiiScanMessage (sock, "%s", image[0].image[0].coords.ctype);

  gfits_free_matrix (&image[0].image[0].matrix);
  gfits_create_matrix (&header, &image[0].image[0].matrix);

  // reference point for image is the center pixel
  image[0].picture.Xc = 0.5*header.Naxis[0] + Xoffset;
  image[0].picture.Yc = 0.5*header.Naxis[1] + Yoffset;

  // choose expand for wide to guarantee we fit:
  wx = MAX ((header.Naxis[0] / (float) image[0].wide.dx), (header.Naxis[1] / (float) image[0].wide.dy));

  // -4.002 -> -5
  // image[0].wide.expand = (wx > 1.0) ? ceil (-wx) : floor (1.0 / wx);
  image[0].wide.expand = (wx > 1.0) ? floor (-wx) : ceil (1.0 / wx);
  // fprintf (stderr, "%d : %f\n", image[0].wide.expand, wx);
  image[0].wide.expand = (wx > 1.0) ? ceil (-wx) : floor (1.0 / wx);

  image[0].wide.Xc = 0.5*header.Naxis[0];
  image[0].wide.Yc = 0.5*header.Naxis[1];

  fcntl (sock, F_SETFL, O_NONBLOCK);  

  status = 1;
  buff = image[0].image[0].matrix.buffer;
  bytes_left = header.datasize;
  image[0].image[0].matrix.datasize = 0;
  while (bytes_left > 0) {
    status = read (sock, buff, bytes_left);
    if (status == 0) {  /* No more pipe */
      fprintf (stderr, "error: pipe closed\n");
      fcntl (sock, F_SETFL, !O_NONBLOCK);  
      return (FALSE);
    }
    if (status != -1) { /* pipe has data */
      image[0].image[0].matrix.datasize += status;
      bytes_left -= status;
      buff = (char *)(buff + status);
    }
  }

  fcntl (sock, F_SETFL, !O_NONBLOCK);  

  if (DEBUG) fprintf (stderr, "read "OFF_T_FMT" bytes\n",  image[0].image[0].matrix.datasize);
  /* it it not obvious this condition should kill kapa, but ... */
  if (image[0].image[0].matrix.datasize != header.datasize) {  
    fprintf (stderr, "error: expected "OFF_T_FMT" bytes, but got only "OFF_T_FMT"\n",  header.datasize,  image[0].image[0].matrix.datasize);
    return (FALSE);
  }
  SetColorScale (graphic, image);

  if (!USE_XWINDOW) return (TRUE);

  Remap (graphic, image);
  CreateWide (graphic, image);
  if (DEBUG) fprintf (stderr, "remapped image\n");
  Refresh ();
  if (DEBUG) fprintf (stderr, "refreshed\n");
  XFlush (graphic[0].display);

  return (TRUE);
}
