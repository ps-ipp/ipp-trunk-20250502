# include "imregister.h"
# include "imreg.h"
static char *version = "convertimreg $Revision: 3.4 $";

int main (int argc, char **argv) {
 
  FILE *f;
  Header header;
  RegImage *pimage;
  off_t i, Nimage, *match, status;
  int lockstate, dbstate;
  
  get_version (argc, argv, version);
  ConfigInit (&argc, argv);
  ConfigCamera ();
  ConfigFilter ();
  
  if (argc != 3) {
    fprintf (stderr, "USAGE: convertregdb (input) (output)\n");
    fprintf (stderr, "       convert old pseudo-FITS table imreg db to FITS table\n");
    exit (1);
  }
  
  /* load old-style pseudo-FITS table db */
  lockstate = LCK_SOFT;

  /* lock database (soft) */
  f = fsetlockfile (argv[1], 300.0, lockstate, &dbstate);
  if (f == (FILE *) NULL) {
    fprintf (stderr, "ERROR: can't open db\n");
    exit (1);
  }
  Fseek (f, 0, SEEK_SET);

  /* load in database header */
  if (!gfits_load_header (f, &header)) {
    fprintf (stderr, "ERROR: trouble reading database header\n");
    fclearlockfile (argv[1], f, lockstate, &dbstate);
    exit (1);
  }

  /* load existing data from database */
  gfits_scan (&header, "NIMAGES", OFF_T_FMT, 1,  &Nimage);
  ALLOCATE (pimage, RegImage, Nimage);
  status = fread (pimage, sizeof(RegImage), Nimage, f);
  if (status != Nimage) {
    fprintf (stderr, "ERROR: header and data in dB don't match ("OFF_T_FMT" vs "OFF_T_FMT")\n",  Nimage,  status);
    fclearlockfile (argv[1], f, lockstate, &dbstate);
    exit (1);
  }
  fclearlockfile (argv[1], f, lockstate, &dbstate);
  gfits_convert_RegImage (pimage, sizeof (RegImage), Nimage);

  /* create complete subset */
  ALLOCATE (match, off_t, Nimage);
  for (i = 0; i < Nimage; i++) match[i] = i;

  DumpFitsBintable (argv[2], pimage, match, Nimage);
  exit (0);
}
