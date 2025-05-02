# include "addstar.h"
# include "ukirt_uhs.h"

// one pass reads a single file at a time
int loadukirt_uhs_table (SkyList *skylistInput, char *filename, AddstarClientOptions *options) {
  
  // open the input file here and pass the pointer below
  FILE *f = fopen (filename, "r");
  if (f == NULL) Shutdown ("can't read ukirt_uhs file: %s", filename);

  // we allocate one extra byte into which we never read so there will always be a NULL terminating the string
  ALLOCATE_PTR (buffer, char, BUFFER_SIZE + 1);
  bzero (buffer, BUFFER_SIZE + 1);

  // starting point of valid data in buffer; this is updated on each pass to readstars
  int Nstart = 0;

  while (1) {

    // read the chunk from the file
    fprintf (stderr, "loading %s\n", filename);

    // on each pass, we read a new chunk of data and receive Nstars

    int Nstars = 0;
    UKIRT_Stars *stars = loadukirt_uhs_readstars (f, buffer, &Nstart, options, &Nstars);
    if (!Nstars) {
      fclose (f);
      free (buffer);
      free (stars);
      return TRUE; // end of file reached
    }

    fprintf (stderr, "writing %d stars to dvo\n", Nstars);

    // sort the stars by RA (Nmeasure agnostic)
    loadukirt_uhs_sortStars (stars, Nstars);

    // scan through the stars, loading the containing catalogs
    // skip through table for unsaved stars
    for (int i = 0; i < Nstars; i++) {
      if (stars[i].flag) continue;

      // scan forward until we read the UserPatch
      if (stars[i].average.R < UserPatch.Rmin) continue;
      if (stars[i].average.R > UserPatch.Rmax) break;
      if (stars[i].average.D < UserPatch.Dmin) continue;
      if (stars[i].average.D > UserPatch.Dmax) continue;

      // identify the relevant catalog
      SkyList *skylist = SkyRegionByPoint_List (skylistInput, -1, stars[i].average.R, stars[i].average.D);
      if (skylist[0].Nregions == 0) {
	SkyListFree (skylist);
	continue;
      }
      SkyRegion *region = skylist[0].regions[0];

      // select stars matching this region (Nmeasure agnostic)
      int Nsubset;
      UKIRT_Stars *subset = loadukirt_uhs_make_subset (stars, Nstars, i, region, &Nsubset);

      // In parallel mode, write out the subset to a disk file.  Block until a remote host
      // is available.  In serial mode, just match against the appropriate region and save
      // NOTE: disable parallel mode for now: 
      // loadukirt_uhs_save_remote (subset, Nsubset, hosts, region, skylist[0].filename[0], options);
      // loadukirt_uhs_catalog (Nmeasure agnostic)
      loadukirt_uhs_catalog (subset, Nsubset, region, skylist[0].filename[0], options);
      free (subset);
      SkyListFree (skylist);
    }
    for (int i = 0; i < Nstars; i++) {
      free (stars[i].measure);
    }
    free (stars);
  }

  // wait for last remote clients to finish
  // NOTE: disable parallel mode for now: 
  // harvest_all ();

  // we should not actually reach this point
  return FALSE;
}

/* modifying to read in smaller blocks of the input file at a time

   each pass on readstars needs to ...

*/

