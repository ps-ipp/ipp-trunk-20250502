# include "dimm.h"

int simsignal (int argc, char **argv) {
  
  int Nvect, Nbin, Nbit, i, ivalue, jvalue, Nshift, mask, scale;
  float *buf;
  double cvalue, dvalue, sigma, SN, period;

  if (argc != 5) {
    gprint (GP_ERR, "USAGE: simsignal (vector) Nbin Nbits period\n");
    return (FALSE);
  }

  if (!SelectVector (&Nvect, argv[1], ANYVECTOR)) return (FALSE);
  Nbin = atof (argv[2]);
  Nbit = atof (argv[3]);
  /* SN = atof (argv[4]); */
  period = atof (argv[4]);

  vectors[Nvect].Nelements = Nbin;
  REALLOCATE (vectors[Nvect].elements, float, vectors[Nvect].Nelements);

  scale = (0x1 << Nbit) - 1;

  buf = vectors[Nvect].elements;
  for (i = 0; i < Nbin; i++, buf++) {
    ivalue = scale * 0.5 * (sin (i*2*M_PI/period) + 1) + 0.5;
    /*
    dvalue = ohana_gaussdev_rnd (cvalue, sigma);
    cvalue = (dvalue + range) / (2.0*range);
    dvalue = MAX (0, MIN (0.99999, cvalue));
    ivalue = scale * dvalue;
    *buf = (2.0 * range * ((double) ivalue + 0.5)) / scale - range; 
    */
    *buf = ivalue;
  }

  return (TRUE);
}

/* 

8 bit = 2^8

  ohana_gaussdev_init ();
  sigma = 2.0 / SN;
  range = 1 + 5*sigma;

*/
