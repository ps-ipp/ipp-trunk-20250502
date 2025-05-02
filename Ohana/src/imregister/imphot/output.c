# include "imregister.h"
# include "imphot.h"

int output (Image *image, off_t *match, off_t Nmatch) {

  /* output the selected entries */
  if (options.table != (char *) NULL) {
    DumpFitsTable (options.table, image, match, Nmatch);
    return (TRUE);
  } 

  /* output the selected entries */
  if (options.bintable != (char *) NULL) {
    DumpFitsBintable (options.bintable, image, match, Nmatch);
    return (TRUE);
  } 

  PrintSubset (image, match, Nmatch);
  return (TRUE);
}

int PrintSubset (Image *image, off_t *match, off_t Nmatch) {

  off_t i, j;
  char *timestr, *photstr;
  static char PhotError[] = "unknown";

  for (j = 0; j < Nmatch; j++) {
    i = match[j];
      
    /* convert UNIX time to Elixir-style date string */
    timestr = ohana_sec_to_date (image[i].tzero);
      
    /* convert photcode to filter name */
    photstr = GetPhotcodeNamebyCode (image[i].photcode);
    if (photstr == (char *) NULL) photstr = PhotError;
      
    fprintf (stdout, "%s %s %s  %7.4f %7.4f  %7.4f %5d %02x\n", image[i].name, photstr, timestr, 
	     image[i].McalPSF, image[i].dMcal, image[i].secz, image[i].nstar, image[i].flags); 
    free (timestr);
  }
  return (TRUE);
}
