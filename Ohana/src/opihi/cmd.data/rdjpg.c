# include "data.h"
# include "jpeglib.h"
# include <setjmp.h>

struct my_error_mgr {
  struct jpeg_error_mgr pub;	/* "public" fields */
  jmp_buf setjmp_buffer;	/* for return to caller */
};

typedef struct my_error_mgr * my_error_ptr;

METHODDEF(void) my_error_exit (j_common_ptr cinfo)
{
  fprintf (stderr, "got an error\n");

  /* cinfo->err really points to a my_error_mgr struct, so coerce pointer */
  my_error_ptr myerr = (my_error_ptr) cinfo->err;

  /* Always display the message. */
  /* We could postpone this until after returning, if we chose. */
  (*cinfo->err->output_message) (cinfo);

  /* Return control to the setjmp point */
  longjmp(myerr->setjmp_buffer, 1);
}

int rdjpg (int argc, char **argv) {
  
  int N;
  Buffer *buf;

  struct jpeg_decompress_struct cinfo;
  struct my_error_mgr jerr;

  int VERBOSE = FALSE;
  if ((N = get_argument (argc, argv, "-v"))) {
    remove_argument (N, &argc, argv);
    VERBOSE = TRUE;
  }

  if (argc != 3) {
    gprint (GP_ERR, "USAGE: rdjpg <buffer> <filename>\n");
    return (FALSE);
  }

  // open input file for read
  FILE *f = fopen (argv[2], "r");
  if (f == (FILE *) NULL) {
    gprint (GP_ERR, "file %s not found\n", argv[2]);
    return (FALSE);
  }

  // find matrix, free old data
  if ((buf = SelectBuffer (argv[1], ANYBUFFER, TRUE)) == NULL) {
    fclose (f);
    return (FALSE);
  }
  gfits_free_matrix (&buf[0].matrix);
  gfits_free_header (&buf[0].header);

  // save file name in buffer info
  char *filename = filebasename (argv[2]);
  strcpy (buf[0].file, filename);
  free (filename);

  // the setup for the error handling seems a bit circular, but this
  // example from the libjpeg example.c seems to work?
  cinfo.err = jpeg_std_error(&jerr.pub);
  jerr.pub.error_exit = my_error_exit;

  // an error in the jpeg code will jump (goto) here:
  if (setjmp(jerr.setjmp_buffer)) {
    /* If we get here, the JPEG code has signaled an error.
     * We need to clean up the JPEG object, close the input file, and return.
     */
    jpeg_destroy_decompress(&cinfo);
    fclose(f);
    return FALSE;
  }

  /* Now we can initialize the JPEG decompression object. */
  jpeg_create_decompress(&cinfo);
  jpeg_stdio_src(&cinfo, f);
  jpeg_read_header(&cinfo, TRUE);
  jpeg_start_decompress(&cinfo);

  /* JSAMPLEs per row in output buffer */
  int Nx = cinfo.output_width;
  int Ny = cinfo.output_height;
  int Npix_row = Nx * cinfo.output_components;

  CreateBuffer (buf, Nx, Ny, -32, 0.0, 1.0);
  float *Vout = (float *)buf[0].matrix.buffer;

  /* Make a one-row-high sample array that will go away when done with image */
  JSAMPARRAY buffer = (*cinfo.mem->alloc_sarray) ((j_common_ptr) &cinfo, JPOOL_IMAGE, Npix_row, 1);

  // read each of the scanlines
  for (int iy = 0; iy < Ny; iy++) {
    jpeg_read_scanlines(&cinfo, buffer, 1);

    // for now just copy R + G + B as the total value:
    for (int ix = 0; ix < Nx; ix ++) {
      unsigned char Rpix = buffer[0][3*ix + 0];
      unsigned char Gpix = buffer[0][3*ix + 1];
      unsigned char Bpix = buffer[0][3*ix + 2];
      float value = Rpix + Gpix + Bpix;
      Vout[Nx*iy + ix] = value;
    }
  }
    
  jpeg_finish_decompress(&cinfo);
  jpeg_destroy_decompress(&cinfo);
  
  fclose (f);
  
  buf[0].bitpix = buf[0].header.bitpix;    /* store the original values */
  buf[0].bscale = buf[0].header.bscale;    /* store the original values */
  buf[0].bzero  = buf[0].header.bzero;     /* store the original values */
  buf[0].unsign = buf[0].header.unsign;

  if (VERBOSE) gprint (GP_LOG, "read "OFF_T_FMT" bytes from %s into buffer %s\n", buf[0].header.datasize + buf[0].matrix.datasize, argv[2], argv[1]);

  return (TRUE);
}
