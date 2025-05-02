# include "uniphot.h"

static char *timeref  = "2000/01/01,00:00:00";
static char *timeunit = "days";

int main (int argc, char **argv) {

  int i, Ntgroup, Nsgroup, status;
  Group *tgroup, *sgroup;
  ImageLink *imlinks;
  FITS_DB db;

  // set up time format stuff
  time_t TimeReference = GetTimeReference (timeref);
  int TimeUnits = GetTimeUnits (timeunit);

  /* get configuration info, args, lockfile */
  SetSignals ();
  initialize_uniphot (argc, argv);

  set_db (&db);
  gfits_db_init (&db);
  status = dvo_image_lock (&db, ImageCat, 60.0, (UPDATE ? LCK_XCLD : LCK_SOFT));
  if (!status) Shutdown ("ERROR: failure to lock image catalog %s", db.filename);
  if (db.dbstate == LCK_EMPTY) Shutdown ("ERROR: No images in catalog %s (1)", db.filename);

  /* load images */
  load_images_uniphot (&db);
  if (!UPDATE) dvo_image_unlock (&db); 
  
  /* filter image list by selection */
  subset_images (&db);

  tgroup = find_image_tgroups (&db, &imlinks, &Ntgroup);
  sgroup = find_image_sgroups (&db, &imlinks, &Nsgroup);

  /* determine fit values */
  for (i = 0; i < NLOOP; i++) {

    fit_tgroup (tgroup, Ntgroup);
    fit_sgroup (sgroup, Nsgroup); 

  }    

  fprintf (stdout, "# uniphot results for filter %s\n", photcode[0].name);
  fprintf (stdout, "# STATMODE: %s\n", STATMODE);
  fprintf (stdout, "# NLOOP: %d\n", NLOOP);
  fprintf (stdout, "# time groups : %d\n", Ntgroup);
  fprintf (stdout, "# TIMEREF : %s\n", timeref);
  fprintf (stdout, "# TIMEFORMAT : %s\n", timeunit);
  for (i = 0; i < Ntgroup; i++) {

    double tstart = NAN;
    double tstop = NAN;

    time_t time;
    if (ohana_str_to_time (tgroup[i].tstart, &time)) { 
      tstart = TimeValue (time, TimeReference, TimeUnits);
    }
    if (ohana_str_to_time (tgroup[i].tstop, &time)) { 
      tstop = TimeValue (time, TimeReference, TimeUnits);
    }

    fprintf (stdout, "%s : %12.6f %12.6f : %5d %5d %7.4f  %7.4f %7.4f\n", 
	     tgroup[i].label, tstart, tstop,
	     tgroup[i].Nimage, tgroup[i].Ngood, 
	     tgroup[i].M, tgroup[i].dM, tgroup[i].dMsub);
  }
  fprintf (stdout, "\n");

  fprintf (stdout, "# space groups : %d\n", Nsgroup);
  for (i = 0; i < Nsgroup; i++) {
    fprintf (stdout, "%s %5d %5d %7.4f  %7.4f %7.4f\n", sgroup[i].label, 
	     sgroup[i].Nimage, sgroup[i].Ngood, sgroup[i].M, sgroup[i].dM, sgroup[i].dMsub);
  }
  if (!UPDATE) exit (0);

  update_dvo_uniphot (&db, sgroup, Nsgroup);
  dvo_image_unlock (&db); 

  exit (0);
}

/* add a mode to check / force consistency between image.Mcal and
   measure.Mcal.

   use the method in delstar to match image with measure (gregions (image), etc)
*/
