# include "Ximage.h"

void PSPixmap8 (Graphic *graphic, KapaImageWidget *image, FILE *f) {

  int i, k, m, val;
  double Nchar, Npix, start, slope, frac;
  unsigned char *buff;
  unsigned long back;

  Nchar = 255.0;
  Npix = graphic[0].Npixels;
  frac = Nchar / Npix;
  /* start at the last line, print lines in decending order */
  buff = (unsigned char *)image[0].picture.data + image[0].picture.dx*(image[0].picture.dy - 1);
  slope = image[0].image[0].slope;
  start = image[0].image[0].start;
  back  = graphic[0].back;

  for (i = 0; i < image[0].picture.dy; i++) {
    for (k = 0; k < image[0].picture.dx; k++, buff++) {
      if (*buff == back) 
	val = Nchar;
      else {
	for (m = 0; (graphic[0].cmap[m].pixel != *buff) && (m < Npix); m++);
	val = Nchar - frac * MIN (MAX (start + m * slope, 0), Npix);
      }
      fprintf (f, "%02x", val);
      if (!((k+1) % 40)) fprintf (f, "\n"); 
    }
    fprintf (f, "\n");
    buff -= 2*image[0].picture.dx;
  }
  return;
}

void PSPixmap16 (Graphic *graphic, KapaImageWidget *image, FILE *f) {

  int i, k, m, val;
  double Nchar, Npix, start, slope, frac;
  unsigned short *buff;
  unsigned long back;

  Nchar = 255.0;
  Npix = graphic[0].Npixels;
  frac = Nchar / Npix;
  /* start at the last line, print lines in decending order */
  buff = (unsigned short *)image[0].picture.data + image[0].picture.dx*(image[0].picture.dy - 1);
  slope = image[0].image[0].slope;
  start = image[0].image[0].start;
  back  = graphic[0].back;

  for (i = 0; i < image[0].picture.dy; i++) {
    for (k = 0; k < image[0].picture.dx; k++, buff++) {
      if (*buff == back) 
	val = Nchar;
      else {
	for (m = 0; (graphic[0].cmap[m].pixel != *buff) && (m < Npix); m++);
	val = Nchar - frac * MIN (MAX (start + m * slope, 0), Npix);
      }
      fprintf (f, "%02x", val);
      if (!((k+1) % 40)) fprintf (f, "\n"); 
    }
    fprintf (f, "\n");
    buff -= 2*image[0].picture.dx;
  }
  return;
}

void PSPixmap24 (Graphic *graphic, KapaImageWidget *image, FILE *f) {

  int i, k, m, dx, dy, val, extra;
  unsigned char *buff;
  unsigned long color, byte;
  double Nchar, Npix, start, slope, frac;
  unsigned long back;

  Nchar = 255.0;
  Npix = graphic[0].Npixels;
  frac = Nchar / Npix;
  dx = image[0].picture.dx;
  dy = image[0].picture.dy;
  extra = 4 - (dx * 3) % 4;
  /* start at the last line, print lines in decending order */
  buff = (unsigned char *)&image[0].picture.data[(dy - 1)*(3*dx + extra)];
  slope = image[0].image[0].slope;
  start = image[0].image[0].start;
  back  = graphic[0].back;

  for (i = 0; i < dy; i++) {
    for (k = 0; k < dx; k++, buff+=3) {
      if (*buff == back) {
	val = Nchar;
      } else {
	color = 0;
	byte = buff[2];
	color = (byte << 16);
	byte = buff[1];
	color |= (byte << 8);
	byte = buff[0];
	color |= byte;
	for (m = 0; (graphic[0].cmap[m].pixel != color) && (m < Npix); m++);
	val = Nchar - frac * MIN (MAX (start + m * slope, 0), Npix);
      }
      fprintf (f, "%02x", val);
      if (!((k+1) % 40)) fprintf (f, "\n"); 
    }
    fprintf (f, "\n");
    buff -= 2*3*dx + extra;
  }
  return;
}

void PSPixmap32 (Graphic *graphic, KapaImageWidget *image, FILE *f) {

  int i, k, m, val;
  double Nchar, Npix, start, slope, frac;
  unsigned int *buff;
  unsigned long back;

  Nchar = 255.0;
  Npix = graphic[0].Npixels;
  frac = Nchar / Npix;
  /* start at the last line, print lines in decending order */
  buff = (unsigned int *)image[0].picture.data + image[0].picture.dx*(image[0].picture.dy - 1);
  slope = image[0].image[0].slope;
  start = image[0].image[0].start;
  back  = graphic[0].back;

  for (i = 0; i < image[0].picture.dy; i++) {
    for (k = 0; k < image[0].picture.dx; k++, buff++) {
      if (*buff == back) 
	val = Nchar;
      else {
	for (m = 0; (graphic[0].cmap[m].pixel != *buff) && (m < Npix); m++);
	val = Nchar - frac * MIN (MAX (start + m * slope, 0), Npix);
      }
      fprintf (f, "%02x", val);
      if (!((k+1) % 40)) fprintf (f, "\n"); 
    }
    fprintf (f, "\n");
    buff -= 2*image[0].picture.dx;
  }
  return;
}

# define WHITE_R 255
# define WHITE_G 255
# define WHITE_B 255

void PSPixmap_3byte (Graphic *graphic, KapaImageWidget *image, FILE *f) {

  int i, j, ii, jj;
  int i_start, i_end, j_start, j_end;
  int I_start, J_start;
  int dx, dy, DX, inDX, inDY;
  int expand_in, expand_out;
  double Ix, Iy;
  unsigned short *in_pix, *in_pix_ref;
  unsigned char *pixel1, *pixel2, *pixel3;

  if (image == NULL) return;

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

  in_pix_ref  = &image[0].pixmap[DX*(int)MAX(Iy,0) + (int)MAX(Ix,0)];

  /********** below we do the mapping from buffer pixels (in) to picture pixels (out) **********/

  // add in occasional return chars

  /**** fill in bottom area ****/
  for (j = 0; j < j_start; j++) {
    for (i = 0; i < dx; i++) {
      fprintf (f, "%02x%02x%02x", WHITE_R, WHITE_G, WHITE_B);
    }
  }
  
  // probably could do this all smarter with scale operations in PS...

  /*** fill in the image data region ***/
  for (j = j_start; j < j_end; j+= expand_out, in_pix_ref += inDY*expand_in*DX) {
    
    // repeat the section below 'expand_out' times
    for (jj = 0; jj < expand_out; jj++) {

      /* create one output image line */
      in_pix = in_pix_ref;

      /**** fill in area to the left of the picture ****/
      for (i = 0; i < i_start; i++) {
	fprintf (f, "%02x%02x%02x", WHITE_R, WHITE_G, WHITE_B);
      }
    
      /*** fill in the picture region ***/
      for (i = i_start; i < i_end; i+=expand_out, in_pix += inDX*expand_in) {
	for (ii = 0; ii < expand_out; ii++) {
	  fprintf (f, "%02x%02x%02x", pixel1[*in_pix], pixel2[*in_pix], pixel3[*in_pix]);
	}
      }
    
      /**** fill in area to the right of the picture ****/
      for (i = i_end; i < dx; i++) {
	fprintf (f, "%02x%02x%02x", WHITE_R, WHITE_G, WHITE_B);
      }
    }
  }

  /**** fill in top area ****/
  for (j = j_end; j < dy; j++) {
    for (i = 0; i < dx; i++) { 
      fprintf (f, "%02x%02x%02x", WHITE_R, WHITE_G, WHITE_B);
    }
  }

  free (pixel1);
  free (pixel2);
  free (pixel3);

  return;
}
