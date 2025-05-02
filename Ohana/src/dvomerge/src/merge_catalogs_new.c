# include "dvomerge.h"

# define IN_REGION(R,D) ( \
((D) >= region[0].Dmin) && ((D) < region[0].Dmax) && \
((R) >= region[0].Rmin)  && ((R) < region[0].Rmax))

// merge the input data into the output catalog
// input entries always define new objects

int merge_catalogs_new (SkyRegion *region, Catalog *output, Catalog *input, int *secfiltMap) {
  
  off_t i, j, offset;
  off_t NAVERAGE, NMEASURE, NLENSING, NSTARPAR, NGALPHOT, Naverage, Nmeasure, Nlensing, Nstarpar, Ngalphot, NsecfiltIn, NsecfiltOut, Nm;

  Naverage = output[0].Naverage;
  Nmeasure = output[0].Nmeasure;
  Nlensing = output[0].Nlensing;
  Nstarpar = output[0].Nstarpar;
  Ngalphot = output[0].Ngalphot;

  NsecfiltOut = output[0].Nsecfilt;
  NsecfiltIn  = input[0].Nsecfilt;

  NAVERAGE = Naverage + 100;
  NMEASURE = Nmeasure + 1000;
  NLENSING = Nlensing + 1000;
  NSTARPAR = Nstarpar + 1000;
  NGALPHOT = Ngalphot + 1000;

  REALLOCATE (output[0].average, Average, NAVERAGE);
  REALLOCATE (output[0].secfilt, SecFilt, NAVERAGE*NsecfiltOut);
  REALLOCATE (output[0].measure, Measure, NMEASURE);
  REALLOCATE (output[0].lensing, Lensing, NLENSING);
  REALLOCATE (output[0].starpar, StarPar, NSTARPAR);
  REALLOCATE (output[0].galphot, GalPhot, NGALPHOT);

  // add all of these entries to the existing catalog
  for (i = 0; i < input[0].Naverage; i++) {
    if (!IN_REGION (input[0].average[i].R, input[0].average[i].D)) continue;

      output[0].average[Naverage] = input[0].average[i];
      output[0].average[Naverage].measureOffset = Nmeasure;

      if (secfiltMap) {
        // the output secfilt photcodes may be in a different place in the output table
        assert (output[0].Nsecfilt >= input[0].Nsecfilt);
        for (j = 0; j < NsecfiltIn; j++) {
          output[0].secfilt[NsecfiltOut*Naverage + secfiltMap[j]] = input[0].secfilt[NsecfiltIn*i+j];
        }
      } else {
        assert (output[0].Nsecfilt == input[0].Nsecfilt);
        for (j = 0; j < NsecfiltIn; j++) {
            output[0].secfilt[NsecfiltOut*Naverage+j] = input[0].secfilt[NsecfiltIn*i+j];
        }
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

      Nm = 0;
      for (j = 0; j < input[0].average[i].Nlensing; j++) {
	  offset = input[0].average[i].lensingOffset + j;

	  output[0].lensing[Nlensing] = input[0].lensing[offset];
	  output[0].lensing[Nlensing].averef = Naverage;

	  Nlensing ++;
	  Nm ++;
	  if (Nlensing == NLENSING) {
	      NLENSING += 1000;
	      REALLOCATE (output[0].lensing, Lensing, NLENSING);
	  }
      }
      output[0].average[Naverage].Nlensing = Nm;

      Nm = 0;
      for (j = 0; j < input[0].average[i].Nstarpar; j++) {
	  offset = input[0].average[i].starparOffset + j;

	  output[0].starpar[Nstarpar] = input[0].starpar[offset];
	  output[0].starpar[Nstarpar].averef = Naverage;

	  Nstarpar ++;
	  Nm ++;
	  if (Nstarpar == NSTARPAR) {
	      NSTARPAR += 1000;
	      REALLOCATE (output[0].starpar, StarPar, NSTARPAR);
	  }
      }
      output[0].average[Naverage].Nstarpar = Nm;

      Nm = 0;
      for (j = 0; j < input[0].average[i].Ngalphot; j++) {
	  offset = input[0].average[i].galphotOffset + j;

	  output[0].galphot[Ngalphot] = input[0].galphot[offset];
	  output[0].galphot[Ngalphot].averef = Naverage;

	  Ngalphot ++;
	  Nm ++;
	  if (Ngalphot == NGALPHOT) {
	      NGALPHOT += 1000;
	      REALLOCATE (output[0].galphot, GalPhot, NGALPHOT);
	  }
      }
      output[0].average[Naverage].Ngalphot = Nm;

      Naverage ++;
      if (Naverage == NAVERAGE) {
	  NAVERAGE += 50;
	  REALLOCATE (output[0].average, Average, NAVERAGE);
	  REALLOCATE (output[0].secfilt, SecFilt, NAVERAGE*NsecfiltOut);
      }
  }
  REALLOCATE (output[0].average, Average, MAX (Naverage, 1));
  REALLOCATE (output[0].measure, Measure, MAX (Nmeasure, 1));
  REALLOCATE (output[0].lensing, Lensing, MAX (Nlensing, 1));
  REALLOCATE (output[0].starpar, StarPar, MAX (Nstarpar, 1));
  REALLOCATE (output[0].galphot, GalPhot, MAX (Ngalphot, 1));
  REALLOCATE (output[0].secfilt, SecFilt, NsecfiltOut*MAX (Naverage, 1));
  output[0].Naverage = Naverage;
  output[0].Nlensing = Nlensing;
  output[0].Nstarpar = Nstarpar;
  output[0].Ngalphot = Ngalphot;
  output[0].Nsecfilt_mem = Naverage * NsecfiltOut;

  // If we are using dvomergeCreate to split an existing catalog, then the max objID in
  // each of the new subdivisions of the original catalog is the max objID of the original
  // catalog.  It is not allowed to use dvomerge to unify catalogs to a lower-resolution
  // partition.
  output[0].objID = input[0].objID; // new max value, save on catalog close
  
  if (VERBOSE) {
      fprintf (stderr, OFF_T_FMT": using "OFF_T_FMT" stars ("OFF_T_FMT" measures, "OFF_T_FMT" lensing, "OFF_T_FMT" starpar, "OFF_T_FMT" galphot) for catalog\n", 
	        i, 
	        output[0].Naverage, 
	        output[0].Nmeasure, 
	        output[0].Nlensing, 
	        output[0].Nstarpar, 
	        output[0].Ngalphot);
  }
  return (TRUE);
}
