# include "Ximage.h"

// XXX for the moment, this function does NOT set the mask bits.
// since we lay graphics on top of images, this is OK for now

# define WHITE_R 255
# define WHITE_G 255
# define WHITE_B 255
# define MY_SWAP_INT(A,B) { int tmp; tmp = A; A = B; B = tmp; }

int bDrawImage (bDrawBuffer *buffer, KapaImageWidget *image, Graphic *graphic) {

  int ii, i, j;
  int i_start, i_end, j_start, j_end;
  int I_start, J_start;
  // int dropback;  /* this is a bit of a kludge... */
  int dx, dy, DX, inDX, inDY, Xs, Ys;
  int expand_in, expand_out;
  double Ix, Iy;
  unsigned char *out_pix;
  unsigned short *in_pix, *in_pix_ref;
  unsigned char *pixel1, *pixel2, *pixel3;

  bDrawColor *line_buffer;

  if (image == NULL) return (TRUE);

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

  // Xs,Ys are in full-frame coords.  can we trim here?
  Xs = image[0].picture.x - graphic[0].xwin;
  Ys = image[0].picture.y - graphic[0].ywin;
  dx = image[0].picture.dx;
  dy = image[0].picture.dy;
  DX = image[0].image[0].matrix.Naxis[0];
  // int DY = image[0].image[0].matrix.Naxis[1];

  // the created buffer is supposed to contain the output windows
  if (Xs < 0) {
    fprintf (stderr, "image display boundaries out of range of window (invalid condition) : fix Kapa\n");
    abort();
  }
  if (buffer[0].Nx < Xs + dx) {
    fprintf (stderr, "image display boundaries out of range of window (invalid condition) : fix Kapa\n");
    abort();
  }
  if (Ys < 0) {
    fprintf (stderr, "image display boundaries out of range of window (invalid condition) : fix Kapa\n");
    abort();
  }
  if (buffer[0].Ny < Ys + dy) {
    fprintf (stderr, "image display boundaries out of range of window (invalid condition) : fix Kapa\n");
    abort();
  }

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

  // dropback = expand_out - (i_end - i_start) % expand_out;
  // if ((i_end - i_start) % expand_out == 0) dropback = 0;

  ALLOCATE (line_buffer, bDrawColor, 3*dx);

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
    memcpy (&buffer[0].pixels[j + Ys][3*Xs], line_buffer, 3*dx);
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
      memcpy (&buffer[0].pixels[j + i + Ys][3*Xs], line_buffer, 3*dx);
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
    memcpy (&buffer[0].pixels[j + Ys][3*Xs], line_buffer, 3*dx);
  }

  free (pixel1);
  free (pixel2);
  free (pixel3);
  free (line_buffer);

  return (TRUE);
}

void bDrawXimage (bDrawBuffer *buffer) {

  Graphic *graphic = GetGraphic ();

  ALLOCATE_PTR (data, char, 4*buffer->Nx*buffer->Ny);

  for (int iy = 0; iy < buffer->Ny; iy++) {
    for (int ix = 0; ix < buffer->Nx; ix++) {
      data[4*(iy*buffer->Nx + ix) + 0] = buffer->pixels[iy][3*ix + 2];
      data[4*(iy*buffer->Nx + ix) + 1] = buffer->pixels[iy][3*ix + 1];
      data[4*(iy*buffer->Nx + ix) + 2] = buffer->pixels[iy][3*ix + 0];
      data[4*(iy*buffer->Nx + ix) + 3] = 0;
    }
  }

  XImage *pix = XCreateImage (graphic[0].display, graphic[0].visual, graphic[0].depth, ZPixmap, 0, 
			      data, buffer->Nx, buffer->Ny, 32, 0);

  XPutImage (graphic[0].display, graphic[0].window, graphic[0].gc, pix, 0, 0, 1, 1, buffer->Nx, buffer->Ny);

  free (data);
}

# if (0)

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

    buffer = bDrawBufferCreate (dx, dy, 1);
    for (i = 0; i < NOVERLAYS; i++) {
      if (image[i].overlay[i].active) bDrawOverlay (image, i);
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

# endif
