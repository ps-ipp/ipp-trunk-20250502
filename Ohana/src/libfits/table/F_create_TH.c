# include <ohana.h>
# include <gfitsio.h>
# define NBYTES 2880

/* a basic table header (extension) is different from a primary header
   in that it has the word XTENSION and a type for the first line,
   instead of SIMPLE */

/*********************** fits create Theader *********************************/
int gfits_create_Theader (Header *header, char *type) {
  
  int i;
  char axis[128];
  
  header[0].datasize = NBYTES;

  myAssert (!header->buffer, "failed to init header or free buffer?");
  ALLOCATE (header[0].buffer, char, NBYTES);
  bzero (header[0].buffer, NBYTES);
  
  for (i = 0; i < NBYTES; i++) header[0].buffer[i] = ' ';
  header[0].buffer[0] = 'E';
  header[0].buffer[1] = 'N';
  header[0].buffer[2] = 'D';
  
  gfits_modify (header, "XTENSION", "%s", 1, type);
  gfits_modify (header, "BITPIX",   "%d", 1, header[0].bitpix);
  gfits_modify (header, "NAXIS",    "%d", 1, header[0].Naxes);
  
  for (i = 0; i < header[0].Naxes; i++) {
    snprintf (axis, 64, "NAXIS%d", i + 1);
    gfits_modify (header,  axis, OFF_T_FMT, 1,  header[0].Naxis[i]);
  }
  
  gfits_modify (header, "PCOUNT", OFF_T_FMT, 1, header[0].pcount);
  gfits_modify (header, "GCOUNT", "%d", 1, header[0].gcount);
  if (!strcmp (type, "IMAGE")) {
    gfits_modify (header, "BSCALE", "%lf", 1, header[0].bscale);
    gfits_modify (header, "BZERO",  "%lf", 1, header[0].bzero);
  }

  return (TRUE);
}	

/*********************** fits create table header *********************************/
int gfits_create_table_header (Header *header, char *type, char *extname) {
  
  int i, valid;

  /* check valid table types */
  valid = FALSE;
  valid |= !strcmp (type, "TABLE");
  valid |= !strcmp (type, "BINTABLE");
  if (!valid) return (FALSE);

  gfits_init_header (header);

  header[0].simple = FALSE;
  header[0].Naxes  = 2;
  for (i = 0; i < FT_MAX_NAXES; i++)
    header[0].Naxis[i] = 0;

  gfits_create_Theader (header, type);

  gfits_modify (header, "TFIELDS", "%d", 1, 0);
  gfits_modify (header, "EXTNAME", "%s", 1, extname);
  
  return (TRUE);
}	
