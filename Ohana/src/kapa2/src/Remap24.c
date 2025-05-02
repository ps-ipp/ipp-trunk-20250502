# include "Ximage.h"

void Remap24 (Graphic *graphic, KapaImageWidget *image, Picture *picture, Matrix *matrix) {

  int i, j, ii, jj;
  int i_start, i_end, j_start, j_end;
  int I_start, J_start;
  int dropback, extra, inDX, inDY;
  int dx, dy, DX;
  double Ix, Iy;
  int expand_in, expand_out;
  unsigned char *out_pix, *out_pix2, *data;
  unsigned short *in_pix, *in_pix2;
  unsigned char *pixel1, *pixel2, *pixel3;
  unsigned char pixvalue1, pixvalue2, pixvalue3;
  unsigned char back1, back2, back3;

  ALLOCATE (pixel1, unsigned char, graphic[0].Npixels);
  ALLOCATE (pixel2, unsigned char, graphic[0].Npixels);
  ALLOCATE (pixel3, unsigned char, graphic[0].Npixels);

  // local arrays for pixel values
  for (i = 0; i < graphic[0].Npixels; i++) { /* set up pixel array */
    pixel1[i] = 0x0000ff &  graphic[0].cmap[i].pixel;
    pixel2[i] = 0x0000ff & (graphic[0].cmap[i].pixel >> 8);
    pixel3[i] = 0x0000ff & (graphic[0].cmap[i].pixel >> 16);
  }
  back1 = 0x0000ff & (graphic[0].back >>  0);
  back2 = 0x0000ff & (graphic[0].back >>  8);
  back3 = 0x0000ff & (graphic[0].back >> 16);

  // set up expansions
  if (picture[0].expand == -1) picture[0].expand = 1;
  if (picture[0].expand ==  0) picture[0].expand = 1;
  assert ((picture[0].expand >= 1) || (picture[0].expand <= -2));
  expand_in = expand_out = 1.0;
  if (picture[0].expand == 0) /* set up expansions */
    picture[0].expand = 1;
  if (picture[0].expand > 0) {
    expand_out = picture[0].expand;
    expand_in  = 1;
  }
  if (picture[0].expand < 0) {
    expand_out = 1;
    expand_in  = -picture[0].expand;
  }

  dx = picture[0].dx;
  dy = picture[0].dy;
  DX = matrix[0].Naxis[0];
  // int DY = matrix[0].Naxis[1];

  // each row is padded to a 4-byte word
  extra = 4 - (dx * 3) % 4;

  // i_start, j_start are the closest lit screen pixel to 0,0
  // I_start, J_start are the image pixel corresponding to i_start, j_start
  Picture_Lower (&i_start, &j_start, &I_start, &J_start, matrix, picture);

  // i_end, j_end are the closest lit screen pixel to dx, dy
  // I_end, J_end are the image pixel corresponding to i_end, j_end
  Picture_Upper (&i_end, &j_end, i_start, j_start, matrix, picture);

  assert (i_start <= i_end);
  assert (j_start <= j_end);

  Ix = picture[0].flipx ? I_start - 1 : I_start;
  Iy = picture[0].flipy ? J_start - 1 : J_start;

  inDX = picture[0].flipx ? -1 : +1;
  inDY = picture[0].flipy ? -1 : +1;

  dropback = expand_out - (i_end - i_start) % expand_out;
  if ((i_end - i_start) % expand_out == 0) dropback = 0;
  
  data = out_pix = (unsigned char *) picture[0].data;
  in_pix  = &image[0].pixmap[DX*(int)MAX(Iy,0) + (int)MAX(Ix,0)];

  /********** below we do the mapping from buffer pixels (in) to picture pixels (out) **********/

  /**** fill in bottom area ****/
  for (j = 0; j < j_start; j++) {
    for (i = 0; i < dx; i++, out_pix+=3) {
      out_pix[0] = back1;
      out_pix[1] = back2;
      out_pix[2] = back3;
    }
    out_pix += extra;
  }
  
  for (j = j_start; j < j_end; j+= expand_out, in_pix += inDY*expand_in*DX) {
    out_pix = &data[j*(3*dx+extra)];

    /**** fill in area to the left of the picture ****/
    for (jj = 0; (i_start > 0) && (jj < expand_out) && (j + jj < dy); jj++) { 
      out_pix2 = out_pix + jj*(3*dx + extra);
      for (i = 0; i < i_start; i++, out_pix2+=3) {
	out_pix2[0] = back1;
	out_pix2[1] = back2;
	out_pix2[2] = back3;
      }
    }
    out_pix += 3*i_start;
    
    /*** fill in the picture region ***/
    in_pix2 = in_pix;
    if (expand_out == 1) {
      for (i = i_start; i < i_end; i++, in_pix2+= inDX*expand_in, out_pix+=3) {
	out_pix[0] = pixel1[*in_pix2];
	out_pix[1] = pixel2[*in_pix2];
	out_pix[2] = pixel3[*in_pix2];
      }
    } else {
      for (i = i_start; i < i_end; i+= expand_out, in_pix2 += inDX, out_pix+= 3*expand_out) { 
	pixvalue1 = pixel1[*in_pix2];
	pixvalue2 = pixel2[*in_pix2];
	pixvalue3 = pixel3[*in_pix2];
	out_pix2 = out_pix;
	for (jj = 0; (jj < expand_out) && (j + jj < dy); jj++, out_pix2+=3*(dx-expand_out)+extra) {
	  for (ii = 0; ii < expand_out; ii++, out_pix2+=3) {
	    out_pix2[0] = pixvalue1; 
	    out_pix2[1] = pixvalue2; 
	    out_pix2[2] = pixvalue3; 
	  }
	}
      }
    }
    out_pix -= 3*dropback;
    
    /**** fill in area to the right of the picture ****/
    for (jj = 0; (jj < expand_out) && (j + jj < dy); jj++) {
      out_pix2 = out_pix + jj*(3*dx+extra);
      for (i = i_end; i < dx; i++, out_pix2+=3) {
	out_pix2[0] = back1;
	out_pix2[1] = back2;
	out_pix2[2] = back3;
      }
    }
  } 
  
  /**** fill in top area ****/
  out_pix = &data[j_end*(3*dx+extra)];
  for (j = 0; j < (dy - j_end); j++) {
    for (i = 0; i < dx; i++, out_pix+=3) { 
      out_pix[0] = back1;
      out_pix[1] = back2;
      out_pix[2] = back3;
    }
    out_pix+=extra;
  }

  picture[0].pix = XCreateImage (graphic[0].display, graphic[0].visual, graphic[0].depth, ZPixmap, 0, 
				 picture[0].data, picture[0].dx, picture[0].dy, 32, 0);

  free (pixel1);
  free (pixel2);
  free (pixel3);
}
