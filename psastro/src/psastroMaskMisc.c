/** storage : I don't really need these print functions any more ***/
# if (0)
    bool REFSTAR_MASK_REGIONS              = psMetadataLookupBool (&status, recipe, "REFSTAR_MASK_REGIONS");

        // text region files for testing
        FILE *f = NULL;
        if (REFSTAR_MASK_REGIONS) {
          char *filename = NULL;
          char *chipname = psMetadataLookupStr (&status, chip->concepts, "CHIP.NAME");
          psStringAppend (&filename, "refstars.mask.%s.dat", chipname);
          FILE *f = fopen (filename, "w");
          if (!f) {
            psWarning ("cannot create refstar mask file %s\n", filename);
            continue;
          }
          psFree (filename);
        }

                    if (REFSTAR_MASK_REGIONS) {
                      fprintf (f, "CIRCLE %f %f  %f %f\n", ref->chip->x, ref->chip->y, radius, radius);
                    }

                        if (REFSTAR_MASK_REGIONS) {
                          // lower side
                          x0 = ref->chip->x + spikeWidth*sin(Theta);
                          y0 = ref->chip->y - spikeWidth*cos(Theta);
                          x1 = ref->chip->x + spikeLength*cos(Theta) + spikeWidth*sin(Theta);
                          y1 = ref->chip->y + spikeLength*sin(Theta) - spikeWidth*cos(Theta);
                          dx = x1 - x0;
                          dy = y1 - y0;

                          fprintf (f, "LINE %f %f  %f %f\n", x0, y0, dx, dy);

                          // upper side
                          x0 = ref->chip->x - spikeWidth*sin(Theta);
                          y0 = ref->chip->y + spikeWidth*cos(Theta);
                          x1 = ref->chip->x + spikeLength*cos(Theta) - spikeWidth*sin(Theta);
                          y1 = ref->chip->y + spikeLength*sin(Theta) + spikeWidth*cos(Theta);
                          dx = x1 - x0;
                          dy = y1 - y0;
                          fprintf (f, "LINE %f %f  %f %f\n", x0, y0, dx, dy);
                        }

			if (REFSTAR_MASK_REGIONS) {
			    fprintf (f, "LINE %f %f  %f %f\n", ref->chip->x, ref->chip->y, 0.0, -100.0);
			}

        if (REFSTAR_MASK_REGIONS) {
          fclose (f);
        }
# endif
