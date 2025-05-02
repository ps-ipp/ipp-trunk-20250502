# include <ohana.h>
# include <gfitsio.h>
# define NBYTES 2880

/******************** fits create header ***********************************/
int gfits_create_header (Header *header) {

  int i;
  char axis[10];
  
  header[0].datasize = NBYTES;

  ALLOCATE (header[0].buffer, char, NBYTES);
  bzero (header[0].buffer, NBYTES);

  for (i = 0; i < NBYTES; i++) header[0].buffer[i] = ' ';
  header[0].buffer[0] = 'E';
  header[0].buffer[1] = 'N';
  header[0].buffer[2] = 'D';

  gfits_modify_alt (header, "SIMPLE", "%t", 1, header[0].simple);
  gfits_modify (header, "BITPIX", "%d", 1, header[0].bitpix);
  gfits_modify (header, "NAXIS",  "%d", 1, header[0].Naxes);
				       
  for (i = 0; i < header[0].Naxes; i++) {
    snprintf_nowarn (axis, 10, "NAXIS%d", i + 1);
    gfits_modify (header,  axis, OFF_T_FMT, 1,  header[0].Naxis[i]);
  }

  // gfits_modify (header, "PCOUNT", "%d",  1, header[0].pcount);
  // gfits_modify (header, "GCOUNT", "%d",  1, header[0].gcount);
  gfits_modify (header, "BSCALE", "%lf", 1, header[0].bscale);
  gfits_modify (header, "BZERO",  "%lf", 1, header[0].bzero);
  gfits_modify_alt (header, "EXTEND", "%t",  1, header[0].extend);
  return (TRUE);

}	


