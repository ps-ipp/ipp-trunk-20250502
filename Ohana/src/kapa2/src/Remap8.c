# include "Ximage.h"
# define OUT_TYPE unsigned char

void Remap8 (Graphic *graphic, KapaImageWidget *image, Picture *picture, Matrix *matrix) {

  int i, j, ii, jj;
  int i_start, i_end, j_start, j_end;
  int I_start, J_start;
  int dropback, inDX, inDY;
  int dx, dy, DX;
  double Ix, Iy;
  int expand_in, expand_out;
  OUT_TYPE *out_pix, *out_pix2, *data;
  unsigned short *in_pix, *in_pix2;
  OUT_TYPE *pixel, pixvalue;
  OUT_TYPE back;

  // just skip if there is no data
  if (matrix[0].Naxes == 0) return;
  if (matrix[0].Naxis[0] == 0) return;
  if (matrix[0].Naxis[1] == 0) return;

  ALLOCATE (pixel, OUT_TYPE, graphic[0].Npixels);

  // local array for pixel values
  for (i = 0; i < graphic[0].Npixels; i++) {
    pixel[i] = 0xff & graphic[0].cmap[i].pixel;
  }
  back = 0xff & graphic[0].back;

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

  // define the image boundaries
  dx = picture[0].dx;
  dy = picture[0].dy;
  DX = matrix[0].Naxis[0];
  // int DY = matrix[0].Naxis[1];

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

  out_pix = data = (OUT_TYPE *) picture[0].data;
  in_pix  = &image[0].pixmap[DX*(int)MAX(Iy,0) + (int)MAX(Ix,0)];

  /********** below we do the mapping from buffer pixels (in) to picture pixels (out) **********/

  /**** fill in bottom area ****/
  for (j = 0; j < dx*j_start; j++, out_pix++) {
    *out_pix = back;
  }
  
  for (j = j_start; j < j_end; j+= expand_out, in_pix += inDY*expand_in*DX) {
    out_pix = &data[j*dx];

    /**** fill in area to the left of the picture ****/
    for (jj = 0; (i_start > 0) && (jj < expand_out) && (j + jj < dy); jj++) { 
      out_pix2 = out_pix + jj*dx;
      for (i = 0; i < i_start; i++, out_pix2++) {
	*out_pix2 = back;
      }
    }
    out_pix += i_start;
    
    /*** fill in the picture region ***/
    in_pix2 = in_pix;
    if (expand_out == 1) {
      for (i = i_start; i < i_end; i++, in_pix2 += inDX*expand_in, out_pix++) {
	*out_pix = pixel[*in_pix2];
      }
    } else {
      for (i = i_start; i < i_end; i+= expand_out, in_pix2 += inDX, out_pix+= expand_out) { 
	pixvalue = pixel[*in_pix2];
	out_pix2 = out_pix;
	for (jj = 0; (jj < expand_out) && (j + jj < dy); jj++, out_pix2+=(dx-expand_out)) {
	  for (ii = 0; ii < expand_out; ii++, out_pix2++) {
	    *out_pix2 = pixvalue;
	  }
	}
      }
    }
    out_pix -= dropback;
    
    /**** fill in area to the right of the picture ****/
    for (jj = 0; (jj < expand_out) && (j + jj < dy); jj++) {
      out_pix2 = out_pix + jj*dx;
      for (i = i_end; i < dx; i++, out_pix2++) {
	*out_pix2 = back;
      }
    }
  } 
  
  /**** fill in top area ****/
  out_pix = &data[j_end*dx];
  for (j = 0; j < dy - j_end; j++) {
    for (i = 0; i < dx; i++, out_pix++) {
      *out_pix = back;
    }
  }
  picture[0].pix = XCreateImage (graphic[0].display, graphic[0].visual, graphic[0].depth, ZPixmap, 0, 
				 picture[0].data, picture[0].dx, picture[0].dy, 8, 0);
  free (pixel);
}
