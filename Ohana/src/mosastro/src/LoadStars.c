# include "mosastro.h"

int LoadStars (int Nfile, char **file) {

  int i, itmp, status, extend;
  time_t tsval;
  struct tm *tmval;

  ALLOCATE (chip, Chip, Nfile);

  /* Nchip is number of valid chips */

  Nchip = 0;
  for (i = 0; i < Nfile; i++) {
    chip[Nchip].file = strcreate (file[i]);

    /* load header */
    if (!gfits_read_header (chip[Nchip].file, &chip[Nchip].header)) {
      fprintf (stderr, "ERROR: can't read header for %s\n", file[i]);
      free (chip[Nchip].file);
      continue;
    }

    /* get image dimensions */
    gfits_scan (&chip[Nchip].header, "NAXIS1",   "%d", 1, &chip[Nchip].NX);
    gfits_scan (&chip[Nchip].header, "NAXIS2",   "%d", 1, &chip[Nchip].NY);

    /* get astrometry information */
    if (!GetCoords (&chip[Nchip].coords, &chip[Nchip].header)) {
      fprintf (stderr, "ERROR: no astrometric solution in header\n");
      free (chip[Nchip].file);
      gfits_free_header (&chip[Nchip].header);
      continue;
    }
    chip[Nchip].coords.crval1 = ohana_normalize_angle (chip[Nchip].coords.crval1);

    itmp = 0;
    gfits_scan (&chip[Nchip].header, "NASTRO",   "%d", 1, &itmp);
    if (itmp == 0) {
      fprintf (stderr, "ERROR: bad astrometric solution in header %s\n", file[i]);
      free (chip[Nchip].file);
      gfits_free_header (&chip[Nchip].header);
      continue;
    }

    /* get time info */
    tsval = parse_time (&chip[Nchip].header);
    tmval = gmtime (&tsval);
    Year = tmval[0].tm_year;

    extend = FALSE;
    gfits_scan_alt (&chip[Nchip].header, "EXTEND",  "%t", 1, &extend);
    if (extend) {
      status = rfits (&chip[Nchip]);
    } else {
      status = rtext (&chip[Nchip]);
    }
    if (!status) {
      /* skip on failure */
      free (chip[Nchip].file);
      gfits_free_header (&chip[Nchip].header);
      continue;
    }

    if (VERBOSE) fprintf (stderr, "loaded %d stars from %s\n", chip[Nchip].Nstars, chip[Nchip].file);

    Nchip ++;
  }
 
  if (Nchip == 0) {
    fprintf (stderr, "no valid data in chip files, exiting\n");
    exit (1);
  }

  return (TRUE);
}
