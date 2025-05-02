# include <ohana.h>
# include <gfitsio.h>
# include "inttypes.h"

int main (int argc, char **argv) {

  int Ncol;
  off_t Nrow;
  char type[16];

  Header header;
  Header theader;
  Matrix matrix;
  FTable ftable;
  
  FILE *f = fopen (argv[1], "r");
  
  /* load in PHU segment (ignore) */
  gfits_fread_header (f, &header);
  gfits_fread_matrix (f, &matrix, &header);

  ftable.header = &theader;

  // load data for this header 
  gfits_load_header (f, &theader);
  
  // read the fits table bytes
  gfits_fread_ftable_data (f, &ftable, FALSE);
  
  float *RA = gfits_get_bintable_column_data (&theader, &ftable, "RA", type, &Nrow, &Ncol); 
  float *DEC = gfits_get_bintable_column_data (&theader, &ftable, "DEC", type, &Nrow, &Ncol); 
  float *errorbit = gfits_get_bintable_column_data (&theader, &ftable, "Error_bit_flag", type, &Nrow, &Ncol); 

  int *errorint = (int *) errorbit;
  fprintf (stderr, "%f %f %f %x %d\n", RA[1], DEC[1], errorbit[1], errorint[1], errorint[1]);

  exit (0);
}
