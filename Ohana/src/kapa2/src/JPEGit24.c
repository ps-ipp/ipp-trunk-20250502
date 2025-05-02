# include "Ximage.h"
# include "jpeglib.h"

# define MY_SWAP_INT(A,B) { int tmp; tmp = A; A = B; B = tmp; }

# define WHITE_R 255
# define WHITE_G 255
# define WHITE_B 255

int JPEGcommand (int sock) {

  int status;
  char filename[1024];

  KiiScanMessage (sock, "%s", filename);
  status = JPEGit24 (filename);
  return (status);
}

// XXX this currently writes out the jpeg for just the active image
int JPEGit24 (char *filename) {

  struct jpeg_compress_struct cinfo;
  struct jpeg_error_mgr jerr;
  JSAMPROW row_pointer[1];	/* pointer to JSAMPLE row[s] */
  JSAMPLE *image_buffer;	/* Points to data for current line */
  JSAMPLE *line_buffer;	        /* Points to data for current line */
  Section *section;
  Graphic *graphic;
  KapaImageWidget *image;

  int ii, i, j;
  int i_start, i_end, j_start, j_end;
  int I_start, J_start;
  int dx, dy, DX, inDX, inDY;
  int quality;
  int expand_in, expand_out;
  double Ix, Iy;
  unsigned char *out_pix;
  unsigned short *in_pix, *in_pix_ref;
  unsigned char *pixel1, *pixel2, *pixel3;
  FILE *f;

  graphic = GetGraphic();
  section = GetActiveSection();
  image   = section->image;
  if (image == NULL) return (TRUE);

  /***** JPEG init calls */
  cinfo.err = jpeg_std_error (&jerr);
  jpeg_create_compress (&cinfo);

  f = fopen (filename, "w");
  if (f == (FILE *) NULL) {
    fprintf (stderr, "Kii: failed to open %s for output\n", filename);
    return (TRUE);
  }
  jpeg_stdio_dest(&cinfo, f);
  
  quality = 75;
  cinfo.image_width = image[0].picture.dx; 	/* image width and height, in pixels */
  cinfo.image_height = image[0].picture.dy;
# ifdef GREYSCALE
  cinfo.input_components = 1;		        /* # of color components per pixel */
  cinfo.in_color_space = JCS_GRAYSCALE; 	/* colorspace of input image */
# else 
  cinfo.input_components = 3;		        
  cinfo.in_color_space = JCS_RGB; 	
# endif
  jpeg_set_defaults (&cinfo);
  jpeg_set_quality (&cinfo, quality, TRUE       /* limit to baseline-JPEG values */);
  jpeg_start_compress (&cinfo, TRUE);

  ALLOCATE (pixel1, unsigned char, graphic[0].Npixels);
  ALLOCATE (pixel2, unsigned char, graphic[0].Npixels);
  ALLOCATE (pixel3, unsigned char, graphic[0].Npixels);

  /** cmap[i].pixel must be defined even if X is not used **/
  for (i = 0; i < graphic[0].Npixels; i++) { /* set up pixel array */
    pixel1[i] = graphic[0].cmap[i].red >> 8;
    pixel2[i] = graphic[0].cmap[i].green >> 8;
    pixel3[i] = graphic[0].cmap[i].blue >> 8;
  }

  assert ((image[0].picture.expand >= 1) || (image[0].picture.expand <= -2));
  expand_in = expand_out = 1.0;
  if (image[0].picture.expand > 0) {
    expand_out = image[0].picture.expand;
    expand_in  = 1;
  }
  if (image[0].picture.expand < 0) {
    expand_out = 1;
    expand_in  = -image[0].picture.expand;
  }

  dx = image[0].picture.dx;
  dy = image[0].picture.dy;
  DX = image[0].image[0].matrix.Naxis[0];
  // DY = image[0].image[0].matrix.Naxis[1];

  // i_start, j_start are the closest lit screen pixel to 0,0
  // I_start, J_start are the image pixel corresponding to i_start, j_start
  Picture_Lower (&i_start, &j_start, &I_start, &J_start, &image[0].image[0].matrix, &image[0].picture);

  // i_end, j_end are the closest lit screen pixel to dx, dy
  // I_end, J_end are the image pixel corresponding to i_end, j_end
  Picture_Upper (&i_end, &j_end, i_start, j_start, &image[0].image[0].matrix, &image[0].picture);

  assert (i_start <= i_end);
  assert (j_start <= j_end);

  Ix = image[0].picture.flipx ? I_start - 1 : I_start;
  Iy = image[0].picture.flipy ? J_start - 1 : J_start;

  inDX = image[0].picture.flipx ? -1 : +1;
  inDY = image[0].picture.flipy ? -1 : +1;

  /* output line buffer */
  ALLOCATE (image_buffer, JSAMPLE, 3*dx*dy);
  ALLOCATE (line_buffer, JSAMPLE, 3*dx);

  in_pix_ref  = &image[0].pixmap[DX*(int)MAX(Iy,0) + (int)MAX(Ix,0)];

  /********** below we do the mapping from buffer pixels (in) to picture pixels (out) **********/

  /**** fill in bottom area ****/
  out_pix = line_buffer;
  for (i = 0; i < dx; i++, out_pix+=3) {
    out_pix[0] = WHITE_R;
    out_pix[1] = WHITE_G;
    out_pix[2] = WHITE_B;
  }
  for (j = 0; j < j_start; j++) {
    memcpy (&image_buffer[j*3*dx], line_buffer, 3*dx);
  }
  
  /*** fill in the image data region ***/
  for (j = j_start; j < j_end; j+= expand_out, in_pix_ref += inDY*expand_in*DX) {
    
    /* create one output image line */
    in_pix = in_pix_ref;
    out_pix = line_buffer;

    /**** fill in area to the left of the picture ****/
    for (i = 0; i < i_start; i++, out_pix+=3) {
      out_pix[0] = WHITE_R;
      out_pix[1] = WHITE_G;
      out_pix[2] = WHITE_B;
    }
    
    /*** fill in the picture region ***/
    for (i = i_start; i < i_end; i+=expand_out, in_pix += inDX*expand_in) {
      for (ii = 0; ii < expand_out; ii++, out_pix+=3) {
	out_pix[0] = pixel1[*in_pix];
	out_pix[1] = pixel2[*in_pix];
	out_pix[2] = pixel3[*in_pix];
      }
    }
    
    /**** fill in area to the right of the picture ****/
    for (i = i_end; i < dx; i++, out_pix+=3) {
      out_pix[0] = WHITE_R;
      out_pix[1] = WHITE_G;
      out_pix[2] = WHITE_B;
    }

    /* write out the image line expand_out times */
    for (i = 0; i < expand_out; i++) {
      memcpy (&image_buffer[(j + i)*3*dx], line_buffer, 3*dx);
    }
  }

  /**** fill in top area ****/
  out_pix = line_buffer;
  for (i = 0; i < dx; i++, out_pix+=3) { 
    out_pix[0] = WHITE_R;
    out_pix[1] = WHITE_G;
    out_pix[2] = WHITE_B;
  }
  for (j = j_end; j < dy; j++) {
    memcpy (&image_buffer[j*3*dx], line_buffer, 3*dx);
  }


  /* I need to write the overlay objects on the jpeg image.
     if i can write / overwrite data in jpeg buffer, then do it here,
     otherwise i need to create a temporary image buffer, then write the 
     scanlines to that buffer */

  {
    int Npalette;
    png_color *palette;
    bDrawColor white, color;
    bDrawBuffer *buffer;

    palette = KapaPNGPalette (&Npalette);

    buffer = bDrawBufferCreate (dx, dy, 1, palette, Npalette);
    for (i = 0; i < NOVERLAYS; i++) {
      if (image[0].overlay[i].active) bDrawOverlay (buffer, image, i);
    }

    white = KapaColorByName ("white");
    for (j = 0; j < dy; j++) {
      for (i = 0; i < dx; i++) {
	color = buffer[0].pixels[j][i];
	if (color == white) continue;
	image_buffer[j*3*dx + 3*i + 0] = palette[color].red;
	image_buffer[j*3*dx + 3*i + 1] = palette[color].green;
	image_buffer[j*3*dx + 3*i + 2] = palette[color].blue;
      }
    }
    bDrawBufferFree (buffer);
    free (palette);
  }

  for (i = 0; i < dy; i++) {
    row_pointer[0] = &image_buffer[i*3*dx];
    (void) jpeg_write_scanlines (&cinfo, row_pointer, 1);
  }

  jpeg_finish_compress (&cinfo);
  fclose (f);
  jpeg_destroy_compress (&cinfo);

  free (pixel1);
  free (pixel2);
  free (pixel3);
  free (line_buffer);
  free (image_buffer);

  return (TRUE);
}
