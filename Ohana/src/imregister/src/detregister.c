# include "imregister.h"
# include "detrend.h"
static char *version = "detregister $Revision: 3.5 $";

int main (int argc, char **argv) {
 
  char *dBFile;
  DetReg newdata;
  Descriptor descriptor;
  FITS_DB db;
  
  get_version (argc, argv, version);
  regargs (argc, argv, &descriptor);
  DefineImage (argv[1], &descriptor);

  newdata = DefineEntry (descriptor);
  SaveEntry (argv[1], &newdata, descriptor.imageID);

  gfits_db_init (&db);
  db.lockstate = LCK_HARD;
  db.timeout   = 300.0;

  dBFile = set_dBFile ();
  if (!gfits_db_lock (&db, dBFile)) {
    fprintf (stderr, "ERROR: failure to lock db\n");
    gfits_db_close (&db);
    exit (1);
  }
  if (db.dbstate == LCK_EMPTY) {
    gfits_db_create (&db);    
    gfits_table_set_DetReg (&db.ftable, NULL, 0, TRUE);
  } else {  
    if (!gfits_db_load (&db)) {
      fprintf (stderr, "ERROR: failure to load db\n");
      gfits_db_close (&db);
      exit (1);
    }
  }

  gfits_convert_DetReg (&newdata, sizeof (DetReg), 1);
  gfits_table_to_vtable (&db.ftable, &db.vtable, 0, 0);
  gfits_vadd_rows (&db.vtable, (char *) &newdata, 1, sizeof(DetReg));

  gfits_db_update (&db);
  gfits_db_close (&db);
  gfits_db_free (&db);

  fprintf (stderr, "SUCCESS\n");
  exit (0);
}

/* detrend files created by mkdetrend have names of the following form:
   
   TYPE.CCD.FILTER.CRUN.NUMBER.fits 
   
   ie:
   
   bias.02.0.01Ak3.0.fits
   
   When they are registered, the NUMBER component needs to be
   updated to match the next available number.
   
   This is a bit tricky.  We can easily identify 
   TYPE, CCD, FILTER, and extrapolate to the NUMBER, but 
   unambiguously identifying the CRUN is harder.  We could make it a 
   required keyword on the command line, or insert it in the
   image header and request it if it doesn't exist?

    required fields:

    tstart
    tstop
    type
    ccd

    filename (automatic)
    treg (automatic)

    case type 
      flat:
      ring:
      scat:
        filter is required
      default:
        filter is none

    case type
      dark:
        exptime is required
      bias:
        exptime is 0.0
      default:
        exptime is NaN

*/
