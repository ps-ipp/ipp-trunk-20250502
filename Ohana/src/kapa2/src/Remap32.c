# include "Ximage.h"
# define OUT_TYPE unsigned int

# define MY_SWAP_INT(A,B) { int tmp; tmp = A; A = B; B = tmp; }

# define MY_SWAP_WORD(W) {			\
    char tmp, *X;				\
    X = (char *) &W;				\
    tmp = X[0]; X[0] = X[3]; X[3] = tmp;	\
    tmp = X[1]; X[1] = X[2]; X[2] = tmp; }

void Remap32 (Graphic *graphic, KapaImageWidget *image, Picture *picture, Matrix *matrix) {

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
  int swap_client, swap_server, swap_bytes;

  // just skip if there is no data
  if (matrix[0].Naxes == 0) return;
  if (matrix[0].Naxis[0] == 0) return;
  if (matrix[0].Naxis[1] == 0) return;

  ALLOCATE (pixel, OUT_TYPE, graphic[0].Npixels);

# ifdef BYTE_SWAP
  swap_client = 1;
# else 
  swap_client = 0;
# endif  
  swap_server = ImageByteOrder (graphic[0].display);
  swap_bytes = !(swap_client ^ swap_server);

  // local array for pixel values
  for (i = 0; i < graphic[0].Npixels; i++) { 
    pixel[i] = graphic[0].cmap[i].pixel;
    if (swap_bytes) MY_SWAP_WORD(pixel[i]);
  }
  back = graphic[0].back;
  if (swap_bytes) MY_SWAP_WORD(back);

  // set up expansions
  if (picture[0].expand == -1) picture[0].expand = 1;
  if (picture[0].expand ==  0) picture[0].expand = 1;
  assert ((picture[0].expand >= 1) || (picture[0].expand <= -2));
  expand_in = expand_out = 1.0;
  if (picture[0].expand > 0) {
    expand_out = picture[0].expand;
    expand_in  = 1;
  }
  if (picture[0].expand < 0) {
    expand_out = 1;
    expand_in  = -picture[0].expand;
  }

  // define the image sizes
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

  unsigned short *in_base = image[0].pixmap;
  OUT_TYPE *out_base = (OUT_TYPE *) picture[0].data;

  int Ninmax = matrix[0].Naxis[0] * matrix[0].Naxis[1];
  int Noutmax = picture[0].dx * picture[0].dy;

  /********** below we do the mapping from buffer pixels (in) to picture pixels (out) **********/

  /**** fill in bottom area ****/
  for (j = 0; j < dx*j_start; j++, out_pix++) {
    myAssert (out_pix - out_base < Noutmax, "too far out");
    *out_pix = back;
  }
  
  for (j = j_start; j < j_end; j+= expand_out, in_pix += inDY*expand_in*DX) {
    out_pix = &data[j*dx];

    /**** fill in area to the left of the picture ****/
    for (jj = 0; (i_start > 0) && (jj < expand_out) && (j + jj < dy); jj++) { 
      out_pix2 = out_pix + jj*dx;
      for (i = 0; i < i_start; i++, out_pix2++) {
	myAssert (out_pix2 - out_base < Noutmax, "too far out");
	*out_pix2 = back;
      }
    }
    out_pix += i_start;
    
    /*** fill in the picture region ***/
    in_pix2 = in_pix;
    if (expand_out == 1) {
      for (i = i_start; i < i_end; i++, in_pix2 += inDX*expand_in, out_pix++) {
	myAssert (out_pix - out_base < Noutmax, "too far out");
	myAssert (in_pix2 - in_base < Ninmax, "too far in");
	*out_pix = pixel[*in_pix2];
      }
    } else {
      for (i = i_start; i < i_end; i+= expand_out, in_pix2 += inDX, out_pix+= expand_out) { 
	myAssert (in_pix2 - in_base < Ninmax, "too far in");
	pixvalue = pixel[*in_pix2];
	out_pix2 = out_pix;
	for (jj = 0; (jj < expand_out) && (j + jj < dy); jj++, out_pix2+=(dx-expand_out)) {
	  for (ii = 0; ii < expand_out; ii++, out_pix2++) {
	    myAssert (out_pix2 - out_base < Noutmax, "too far out");
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
	myAssert (out_pix2 - out_base < Noutmax, "too far out");
	*out_pix2 = back;
      }
    }
  } 

  /**** fill in top area ****/
  out_pix = &data[j_end*dx];
  for (j = 0; j < dy - j_end; j++) {
    for (i = 0; i < dx; i++, out_pix ++) {
      myAssert (out_pix - out_base < Noutmax, "too far out");
      *out_pix = back;
    }
  }

  picture[0].pix = XCreateImage (graphic[0].display, graphic[0].visual, graphic[0].depth, ZPixmap, 0, 
				 picture[0].data, picture[0].dx, picture[0].dy, 32, 0);

  free (pixel);
}
