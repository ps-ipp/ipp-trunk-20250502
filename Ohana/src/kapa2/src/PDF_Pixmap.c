# include "Ximage.h"

# define WHITE_R 255
# define WHITE_G 255
# define WHITE_B 255

int WhiteIOBuffer (IOBuffer *buffer);

void PDF_Pixmap (Graphic *graphic, KapaImageWidget *image, IOBuffer *buffer) {

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
      WhiteIOBuffer (buffer);
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
	WhiteIOBuffer (buffer);
      }
    
      /*** fill in the picture region ***/
      for (i = i_start; i < i_end; i+=expand_out, in_pix += inDX*expand_in) {
	for (ii = 0; ii < expand_out; ii++) {
	  char tmpdata[4];
	  tmpdata[0] = pixel1[*in_pix];
	  tmpdata[1] = pixel2[*in_pix];
	  tmpdata[2] = pixel3[*in_pix];
	  WriteToIOBuffer (buffer, tmpdata, 3);
	}
      }
    
      /**** fill in area to the right of the picture ****/
      for (i = i_end; i < dx; i++) {
	WhiteIOBuffer (buffer);
      }
    }
  }

  /**** fill in top area ****/
  for (j = j_end; j < dy; j++) {
    for (i = 0; i < dx; i++) { 
      WhiteIOBuffer (buffer);
    }
  }

  free (pixel1);
  free (pixel2);
  free (pixel3);

  return;
}

/* Set current pixel to white */
int WhiteIOBuffer (IOBuffer *buffer) {

  // extend the buffer if needed
  if (buffer[0].Nbuffer + 4 >= buffer[0].Nalloc) {
    buffer[0].Nalloc = buffer[0].Nbuffer + 64;
    REALLOCATE (buffer[0].buffer, char, buffer[0].Nalloc);
  }

  buffer[0].buffer[buffer[0].Nbuffer + 0] = WHITE_R;
  buffer[0].buffer[buffer[0].Nbuffer + 1] = WHITE_G;
  buffer[0].buffer[buffer[0].Nbuffer + 2] = WHITE_B;
  buffer[0].Nbuffer += 3;

  return (TRUE);
}
  
