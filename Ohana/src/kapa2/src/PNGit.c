# include "Ximage.h"

int PNGcommand (int sock) {

  int status;
  char filename[1024];

  KiiScanMessage (sock, "%s", filename);
  status = PNGit (filename);
  return (status);
}

int PNGit (char *filename) {

  FILE *f;
  png_structp png_ptr;
  png_infop info_ptr;
  int status, Npalette;
  bDrawBuffer *buffer = NULL;
  Graphic *graphic = NULL;
  png_color *palette = NULL;

  graphic = GetGraphic();

  // limit the png window to the min needed to contain the active graphic regions
  SectionMinBoundary (graphic);

  f = fopen (filename, "w");
  if (f == (FILE *) NULL) {
    fprintf (stderr, "can't open output file %s\n", "Xgraph.png");
    return (TRUE);  /* true because otherwise it quits kapa! */
  }

  png_ptr = png_create_write_struct (PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
  if (!png_ptr) {
    fprintf (stderr, "can't get png structure\n");
    fclose (f);
    return (TRUE);
  }

  info_ptr = png_create_info_struct (png_ptr);
  if (!info_ptr) {
    fprintf (stderr, "can't get png info structure\n");
    png_destroy_write_struct (&png_ptr, NULL);
    fclose (f);
    return (TRUE);
  }

#ifdef png_jmpbuf
  status = setjmp(png_jmpbuf(png_ptr));
# else
  status = setjmp (png_ptr[0].jmpbuf);
# endif

  if (status) {
    fprintf (stderr, "can't get png return\n");
    png_destroy_write_struct (&png_ptr, &info_ptr);
    fclose (f);
    return (TRUE);
  }

  png_init_io (png_ptr, f);

  palette = KapaPNGPalette (&Npalette);

  /* before we allowed colormap-scaled points, we had the option of writing an image using a
     limited palette.  we could get smart and check all objects to see if any of them use
     scaled colors and choose a limited palette if not. but maybe that is not worth the effort.
  */

# if (0)
  // do we have an image in any of the sections?
  int Nsection = GetNumberOfSections ();
  int haveImage = FALSE;
  int i;
  for (i = 0; !haveImage && (i < Nsection); i++) {
    Section *section = GetSectionByNumber (i);
    haveImage = (section->image != NULL);
  }
# else 
  int haveImage = TRUE;
# endif

  /* see docs for write-row-callback to provide progress info */
  if (haveImage) {
    png_set_IHDR (png_ptr, info_ptr, graphic->dxwin, graphic->dywin, 8, PNG_COLOR_TYPE_RGB, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
    png_set_sRGB (png_ptr, info_ptr, PNG_sRGB_INTENT_ABSOLUTE);
  } else {
    png_set_IHDR (png_ptr, info_ptr, graphic->dxwin, graphic->dywin, 8, PNG_COLOR_TYPE_PALETTE, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT); 
    png_set_PLTE (png_ptr, info_ptr, palette, Npalette); 
  }

  /* other useful calls here:
     png_set_tIME(png_ptr, info_ptr, mod_time);
     png_set_text(png_ptr, info_ptr, text_ptr, num_text);
  */
 
  png_write_info (png_ptr, info_ptr);

  if (graphic->smooth_sigma == 0.0) {
    fprintf (stderr, "making PNG without antialias smoothing\n");
    fprintf (stderr, "use 'antialias (value)' to set smoothing scale (0.4 - 0.7 recommended)\n");
  }
    
  if (haveImage) {
    buffer = bDrawIt (palette, Npalette, 3);
  } else {
    buffer = bDrawIt (palette, Npalette, 1);
  }

  png_write_image (png_ptr, buffer[0].pixels);
  png_write_end (png_ptr, info_ptr);
  png_destroy_write_struct (&png_ptr, &info_ptr);
  
  bDrawBufferFree (buffer);
  free (palette);
  
  fclose (f);
  return (TRUE);

}
