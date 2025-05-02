# include "imregister.h"
# include "imreg.h"
static char *version = "showiminfo $Revision: 3.2 $";

int main (int argc, char **argv) {
 
  char *obstime, *regtime;
  RegImage *image, im;

  get_version (argc, argv, version);
  args (argc, argv);
  image = iminfo (argv[1]);
  im = image[0];
  
  fprintf (stderr, "\n");
  fprintf (stderr, "filename: %s\n",   im.filename);
  fprintf (stderr, "pathname: %s\n",   im.pathname);
  fprintf (stderr, "filter: %s\n",     im.filter);
  fprintf (stderr, "instrument: %s\n\n", im.instrument);

  fprintf (stderr, "ccd: %d, mode: %d, type: %d\n\n", im.ccd, im.mode, im.type);

  fprintf (stderr, "exptime: %f, airmass: %f, telfocus: %f\n", im.exptime, im.airmass, im.telfocus);
  fprintf (stderr, "xprobe: %f, yprobe: %f, zprobe: %f\n", im.xprobe, im.yprobe, im.zprobe);
  fprintf (stderr, "dettemp: %f, temp0: %f temp1: %f, temp2: %f, temp3: %f\n\n", 
	   im.dettemp, im.teltemp_0, im.teltemp_1, im.teltemp_2, im.teltemp_3);

  fprintf (stderr, "ra: %f, dec: %f, rotangle: %f\n", im.ra, im.dec, im.rotangle);

  obstime = ohana_sec_to_date (im.obstime);
  regtime = ohana_sec_to_date (im.regtime);

  fprintf (stderr, "obstime: %s\n", obstime);
  fprintf (stderr, "regtime: %s\n", regtime);

  exit (0);
}
