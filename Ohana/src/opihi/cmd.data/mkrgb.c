# include "data.h"
# include "jpeglib.h"

int mkrgb (int argc, char **argv) {
 
  int i, j, Nx, Ny;
  FILE *f;
  Buffer *red, *green, *blue;
  float *Vr, *Vg, *Vb;

  struct jpeg_compress_struct cinfo;
  struct jpeg_error_mgr jerr;
  JSAMPROW row_pointer[1];	/* pointer to JSAMPLE row[s] */
  JSAMPLE *image_buffer;	/* Points to data for current line */

  if (argc != 5) {
    gprint (GP_ERR, "USAGE: mkrgb (red) (green) (blue) (output)\n");
    return (FALSE);
  }

  // define the input buffer and examine the shift
  if ((red    = SelectBuffer (argv[1], OLDBUFFER, TRUE)) == NULL) return (FALSE);
  if ((green  = SelectBuffer (argv[2], OLDBUFFER, TRUE)) == NULL) return (FALSE);
  if ((blue   = SelectBuffer (argv[3], OLDBUFFER, TRUE)) == NULL) return (FALSE);

  Nx = red[0].matrix.Naxis[0];
  Ny = red[0].matrix.Naxis[1];
  if (Nx != blue[0].matrix.Naxis[0]) return (FALSE);
  if (Ny != blue[0].matrix.Naxis[1]) return (FALSE);
  if (Nx != green[0].matrix.Naxis[0]) return (FALSE);
  if (Ny != green[0].matrix.Naxis[1]) return (FALSE);

  f = fopen (argv[4], "w");
  if (f == (FILE *) NULL) {
    fprintf (stderr, "failed to open %s for output\n", argv[4]);
    return (FALSE);
  }

  /* set up the error handler , initialize the JPEG compression object. */
  cinfo.err = jpeg_std_error (&jerr);
  jpeg_create_compress (&cinfo);
  jpeg_stdio_dest(&cinfo, f);
  
  // set up the basic jpeg output file
  cinfo.image_width = Nx; 	/* image width and height, in pixels */
  cinfo.image_height = Ny;
  cinfo.input_components = 3;		        
  cinfo.in_color_space = JCS_RGB; 	
  jpeg_set_defaults (&cinfo);
  jpeg_set_quality (&cinfo, 75, TRUE       /* limit to baseline-JPEG values */);
  jpeg_start_compress (&cinfo, TRUE);

  ALLOCATE (image_buffer, JSAMPLE, 3*Nx);

  // ??
  // && (cinfo.next_scanline < cinfo.image_height)

  Vr = (float *) red[0].matrix.buffer;
  Vg = (float *) green[0].matrix.buffer;
  Vb = (float *) blue[0].matrix.buffer;

  for (j = 0; j < Ny; j++) {
    for (i = 0; i < Nx; i++, Vr++, Vg++, Vb++) {
      image_buffer[3*i+0] = MAX (0.0, MIN (255.0, *Vr));
      image_buffer[3*i+1] = MAX (0.0, MIN (255.0, *Vg));
      image_buffer[3*i+2] = MAX (0.0, MIN (255.0, *Vb));
    }
    row_pointer[0] = image_buffer;
    (void) jpeg_write_scanlines (&cinfo, row_pointer, 1);
  }

  jpeg_finish_compress (&cinfo);
  fclose (f);

  jpeg_destroy_compress (&cinfo);

  return (TRUE);
}
