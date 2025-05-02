# include "addstar.h"

int load2mass_catalog (Catalog *catalog, Stars *stars, int Nstars) {

  int i, j, Nsec, Nmeas, Nave, NMEAS, NAVE;

  Nsec = GetPhotcodeNsecfilt ();
  Nave = catalog[0].Naverage;
  Nmeas = catalog[0].Nmeasure;
   
  NAVE = Nave + 100;
  NMEAS = Nmeas + 100;
  REALLOCATE (catalog[0].average, Average, NAVE);
  REALLOCATE (catalog[0].secfilt, SecFilt, NAVE*Nsec);
  REALLOCATE (catalog[0].measure, Measure, NMEAS);

  for (i = 0; i < Nstars; i+=3) {

    double R = stars[i].average.R;
    double D = stars[i].average.D;

    // construct an average object for this object
    // XXX for now, the output objects will have limited astrometric interpretation...
    // XXX every 3 stars represents 3 measurements and 1 average
    dvo_average_init (&catalog[0].average[Nave]);
    catalog[0].average[Nave].R     = R;
    catalog[0].average[Nave].D     = D;
    catalog[0].average[Nave].measureOffset = Nmeas;

    for (j = 0; j < Nsec; j++) {
      dvo_secfilt_init (&catalog[0].secfilt[Nave*Nsec+j], SECFILT_RESET_ALL);
    }

    // we now have the min chisq row. use this to supply the other filter values....
    for (j = 0; j < 3; j++) {
      catalog[0].measure[Nmeas]           = stars[i+j].measure;

      catalog[0].measure[Nmeas].R         = R;
      catalog[0].measure[Nmeas].D         = D;
      catalog[0].measure[Nmeas].dt        = NAN_S_SHORT;

      // XXX what about averef?

      catalog[0].average[Nave].Nmeasure++;
      Nmeas ++;
      CHECK_REALLOCATE (catalog[0].measure, Measure, NMEAS, Nmeas, 100);
    }

    Nave ++;
    if (Nave >= NAVE) {
      NAVE += 100;
      REALLOCATE (catalog[0].average, Average, NAVE);
      REALLOCATE (catalog[0].secfilt, SecFilt, NAVE*Nsec);
    }
  }
  catalog[0].Naverage = Nave;
  catalog[0].Nmeasure = Nmeas;
  catalog[0].Nsecfilt_mem = Nave*Nsec;
  return (TRUE);
}
