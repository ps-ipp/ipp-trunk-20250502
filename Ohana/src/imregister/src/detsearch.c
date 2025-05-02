# include "imregister.h"
# include "detrend.h"
static char *version = "detsearch $Revision: 3.8 $";

int main (int argc, char **argv) {
 
  off_t Nmatch, Ndetrend;
  char *dBFile;
  FITS_DB db;
  Match *match;
  DetReg *detrend;

  /* args searches the argument list and generates an array of criteria */
  get_version (argc, argv, version);
  args (argc, argv);

  if (output.Criteria) PrintCriteria ();
	
  gfits_db_init (&db);
  db.lockstate = (output.Modify || output.Delete) ? LCK_HARD : LCK_SOFT;
  db.timeout   = 300.0;

  dBFile = set_dBFile ();
  if (!gfits_db_lock (&db, dBFile)) {
    fprintf (stderr, "ERROR: failure to lock db\n");
    gfits_db_close (&db);
    exit (1);
  }
  
  if (!gfits_db_load (&db)) {
    fprintf (stderr, "ERROR: failure to load db\n");
    gfits_db_close (&db);
    exit (1);
  }

  if (!output.Modify && !output.Delete && !output.Altpath) gfits_db_close (&db);
  detrend = gfits_table_get_DetReg (&db.ftable, &Ndetrend, &db.scaledValue, &db.nativeOrder);
  if (!detrend) {
    fprintf (stderr, "ERROR: failed to read detrend info\n");
    exit (2);
  }
  
  match = MatchCriteria (detrend, Ndetrend, &Nmatch);          /* match basic criteria */
  match = ExptimeCriteria (detrend, Ndetrend, match, &Nmatch); /* reduce matches based on Exptime */
  match = CloseCriteria (detrend, Ndetrend, match, &Nmatch);   /* reduce matches based on closeness */

  if (output.Select) {
    match = UniqueSubset (detrend, Ndetrend, match, &Nmatch);
    if (Nmatch == 0) {
      if (output.verbose) fprintf (stderr, "ERROR: can't find any valid detrend files (%s %s %d)\n", get_type_name(criteria[0].Type), filtername[criteria[0].Filter], criteria[0].CCD);
      gfits_db_close (&db);
      exit (1);
    }
  }

  if (output.Altpath) SetAltpath (&db, detrend, Ndetrend, match, Nmatch);
  if (output.Modify) ModifySubset (&db, detrend, Ndetrend, match, Nmatch);
  if (output.Delete) DeleteSubset (&db, detrend, Ndetrend, match, Nmatch);

  OutputSubset (detrend, Ndetrend, match, Nmatch);
  exit (0);
}

/*

selection options:

CCDSelect
TypeSelect
TimeSelect
FilterSelect
ExptimeSelect
EntrySelect
LabelSelect


 functional modes:

   1) list - list all images that apply to a restrictive set of criteria
   
   2) select - choose the reference image(s)

   3) delete - delete a set of images

   4) modify - alter fields for a set of images

   
   methods to define criteria

   - command line  [-time -type -ccd -filter -exptime]
   - image  -> define time, filter, ccd
   - mosaic -> define time, filter, exptime
   - recipe -> defile type list based on filter

*/
