# include "addstar.h"
# include "2mass.h"

int main (int argc, char **argv) {

  int i, N, Nrefcat;
  Stars *refcat;
  SkyRegion *regions, accregion;

  ConfigInit (&argc, argv);

  /* override any header PHOTCODE values */
  thiscode = NULL;
  if ((N = get_argument (argc, argv, "-p"))) {
    remove_argument (N, &argc, argv);
    thiscode = GetPhotcodebyName (argv[N]);
    remove_argument (N, &argc, argv);
  }

  if (argc != 2) {
    fprintf (stderr, "USAGE: mkacc.2mass (file)\n");
    exit (2);
  }

  NAMED_PHOTCODE (TM_J, "2MASS_J");
  NAMED_PHOTCODE (TM_H, "2MASS_H");
  NAMED_PHOTCODE (TM_K, "2MASS_K");
  if (thiscode == NULL) Shutdown ("photcode not specified");
  if (thiscode[0].code == TM_J) goto good_code;
  if (thiscode[0].code == TM_H) goto good_code;
  if (thiscode[0].code == TM_K) goto good_code;
  Shutdown ("2MASS photcode not specified");

good_code:
  UserPatch.RAmin = 0;
  UserPatch.RAmax = 360;
  UserPatch.DECmin = -90;
  UserPatch.DECmax = +90;

  ALLOCATE (regions, SkyRegion, 1);
  strcpy (regions[0].filename, argv[1]);
  regions[0].RAmin = 0;
  regions[0].RAmax = 360;
  regions[0].DECmin = -90;
  regions[0].DECmax = +90;

  refcat = get2mass_AS_data (regions, &UserPatch, &Nrefcat);

  /* find upper and lower file limits in RA and DEC */

  strcpy (accregion.filename, regions[0].filename);
  accregion.RAmin  = 360;
  accregion.RAmax  =   0;
  accregion.DECmin = +90;
  accregion.DECmax = -90;

  for (i = 0; i < Nrefcat; i++) {
    accregion.RAmin  = MIN (refcat[i].R, accregion.RAmin);
    accregion.RAmax  = MAX (refcat[i].R, accregion.RAmax);
    accregion.DECmin = MIN (refcat[i].D, accregion.DECmin);
    accregion.DECmax = MAX (refcat[i].D, accregion.DECmax);
  }

  fprintf (stderr, "%s %10.6f %10.6f  %10.6f %10.6f  %d\n", 
	   accregion.filename, accregion.RAmin/15.0, accregion.RAmax/15.0, accregion.DECmin, accregion.DECmax, Nrefcat);

  exit (0);
}

/* XXX update this function to create an additional accelerator file for each 2mass file
   each file: one row per DEC band
   each row: Rmin, Rmax, Dmin, Dmax, Nbyte(i)
   where Nbyte(i) = byte for each Rmin + i*30 deg transition
*/
