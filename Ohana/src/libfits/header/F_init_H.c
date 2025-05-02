# include <ohana.h>
# include <gfitsio.h>
static int FT_UNSIGN_MODE = FALSE;

/*********************** fits read header ***********************************/
int gfits_init_header (Header *header) {

  int i;

  header[0].simple = TRUE;
  header[0].extend = FALSE;
  header[0].unsign = FALSE;
  header[0].pcount = 0;
  header[0].gcount = 1;
  header[0].bscale = 1.0;
  header[0].bzero  = 0.0;
  header[0].bitpix = 8;
  header[0].Naxes  = 0;
  header[0].buffer = NULL;
  for (i = 0; i < FT_MAX_NAXES; i++) header[0].Naxis[i] = 0;
  header[0].datasize = 0;

  return (TRUE);
}

int gfits_set_unsign_mode (int mode) {
  int oldmode;
  
  oldmode = FT_UNSIGN_MODE;
  FT_UNSIGN_MODE = mode;
  return (oldmode);
}

int gfits_get_unsign_mode () {
  return (FT_UNSIGN_MODE);
}

Header *gfits_alloc_header (void) {

  Header *header;
  ALLOCATE (header, Header, 1);
  gfits_init_header (header);
  return header;
}
