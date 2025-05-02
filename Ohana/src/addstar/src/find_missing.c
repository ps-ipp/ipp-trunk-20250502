
/* XXX I have dropped the -missed capability of addstar. This is an expensive operation which
 * is only rarely needed.  It is more efficient to perform this operation as a crawler like
 * relphot.  I am saving in this file the code which was used in find_matches to perform the
 * missing matches.
 */

/** code to add references from all previous non-detection observations of this spot on the sky */
for (j = 0; (j < Noverlap) && !options.skip_missed; j++) {
  /* make sure there is space for next entry */
  if (Nmiss >= NMISS) {
    NMISS = Nmiss + 1000;
    REALLOCATE (next_miss, int, NMISS);
    REALLOCATE (catalog[0].missing, Missing, NMISS);
  }
  if (!in_image (catalog[0].average[Nave].R, catalog[0].average[Nave].D, &overlap[j])) continue;
  add_miss_link (&catalog[0].average[Nave], next_miss, Nmiss);

  /* get time of exposure of this portion of the image */
  RD_to_XY (&X, &Y, catalog[0].average[Nave].R, catalog[0].average[Nave].D, &overlap[j].coords);	  
  catalog[0].missing[Nmiss].t  = overlap[j].tzero + 1e-4*Y*overlap[j].trate;  /* rough guess at time */
  catalog[0].average[Nave].Nn ++;
  Nmiss ++;
}

/* add reference for undetected catalog stars */
/* XXX allow this option only for single images? */
if (!strcmp (&image[0].coords.ctype[4], "-WRP")) image[0].coords = mosaic;
for (j = 0; (j < Nave) && !options.skip_missed; j++) {
  n = N2[j];
  if (catalog[0].found[n] < 0) { 
    /* make sure there is space for next entry */
    if (Nmiss >= NMISS) {
      NMISS = Nmiss + 1000;
      REALLOCATE (next_miss, int, NMISS);
      REALLOCATE (catalog[0].missing, Missing, NMISS);
    }

    /* should the catalog star be on this image? project into image coords */
    if (!in_image (catalog[0].average[n].R, catalog[0].average[n].D, image)) continue;
    add_miss_link (&catalog[0].average[n], next_miss, Nmiss);

    /* calculate time of exposure for this coordinate in the image */
    RD_to_XY (&X, &Y, catalog[0].average[n].R, catalog[0].average[n].D, &image[0].coords);	  
    catalog[0].missing[Nmiss].t  = image[0].tzero + 1e-4*Y*image[0].trate;  /* trate is in 0.1 msec / row */
    catalog[0].average[n].Nn ++;
    Nmiss ++;
  }
}
  catalog[0].missing = sort_missing (catalog[0].average, Nave, catalog[0].missing, Nmiss, next_miss);

