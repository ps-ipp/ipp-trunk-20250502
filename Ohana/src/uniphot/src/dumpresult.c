# include "uniphot.h"

void dumpresult () {
  float Mcal, Mgrp, Mset;
  FILE *f;
  char outfile[64];
  Group *tgrp;

  for (i = 0; i < Nsgroup; i++) {
    sprintf (outfile, "test.%02d.dat", i);
    f = fopen (outfile, "w");
    for (j = 0; j < sgroup[i].Nimage; j++) {
      if (sgroup[i].image[j][0].code & IMAGE_BAD) continue;
      tgrp = (Group *) sgroup[i].imlink[j][0].tgroup;
      Mcal = sgroup[i].image[j][0].McalPSF;
      Mset = sgroup[i].M;
      Mgrp = tgrp[0].M;
      fprintf (f, "%7.4f %7.4f %7.4f %7.4f   %10.6f %10.6f  %f %s\n", 
	       Mcal, Mgrp, Mset, sgroup[i].image[j][0].dMcal, 
	       sgroup[i].image[j][0].coords.crval1, sgroup[i].image[j][0].coords.crval2, (sgroup[i].image[j][0].tzero-915148800)/86400.0, tgrp[0].label);
    }
    fclose (f);
  }
}

