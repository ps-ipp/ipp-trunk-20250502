# include "relphot.h"

// merge the input data into the output catalog
// input entries always define new objects
int merge_catalogs_new (Catalog *output, Catalog *input) {
  
  off_t i, j, offset;
  off_t NAVERAGE, NMEASURE, Naverage, Nmeasure, Nsecfilt, Nm;

  Naverage = output[0].Naverage;
  Nmeasure = output[0].Nmeasure;
  Nsecfilt = output[0].Nsecfilt;
  assert (output[0].Nsecfilt == input[0].Nsecfilt);

  NAVERAGE = Naverage + 100;
  NMEASURE = Nmeasure + 1000;

  REALLOCATE (output[0].average, Average, NAVERAGE);
  REALLOCATE (output[0].secfilt, SecFilt, NAVERAGE*PhotNsec);
  REALLOCATE (output[0].measure, Measure, NMEASURE);

  // add all of these entries to the existing catalog
  for (i = 0; i < input[0].Naverage; i++) {
      output[0].average[Naverage] = input[0].average[i];
      output[0].average[Naverage].measureOffset = Nmeasure;

      // XXX input[0].Nsecfilt == output[0].Nsecfilt ???
      for (j = 0; j < Nsecfilt; j++) {
	  output[0].secfilt[Nsecfilt*Naverage+j] = input[0].secfilt[Nsecfilt*i+j];
      }

      Nm = 0;
      for (j = 0; j < input[0].average[i].Nmeasure; j++) {
	  offset = input[0].average[i].measureOffset + j;

	  output[0].measure[Nmeasure] = input[0].measure[offset];
	  output[0].measure[Nmeasure].averef = Naverage;

	  Nmeasure ++;
	  Nm ++;
	  if (Nmeasure == NMEASURE) {
	      NMEASURE += 1000;
	      REALLOCATE (output[0].measure, Measure, NMEASURE);
	  }
      }
      output[0].average[Naverage].Nmeasure = Nm;
      Naverage ++;
      if (Naverage == NAVERAGE) {
	  NAVERAGE += 50;
	  REALLOCATE (output[0].average, Average, NAVERAGE);
	  REALLOCATE (output[0].secfilt, SecFilt, NAVERAGE*Nsecfilt);
      }
  }
  REALLOCATE (output[0].average, Average, MAX (Naverage, 1));
  REALLOCATE (output[0].measure, Measure, MAX (Nmeasure, 1));
  REALLOCATE (output[0].secfilt, SecFilt, Nsecfilt*MAX (Naverage, 1));
  output[0].Naverage = Naverage;
  output[0].Nmeasure = Nmeasure;
  output[0].Nsecfilt_mem = Naverage * Nsecfilt;
  
  if (VERBOSE) {
      fprintf (stderr, "%d: using %d stars (%d measures) for catalog\n", i, 
	       output[0].Naverage, output[0].Nmeasure);
  }
  return (TRUE);
}
