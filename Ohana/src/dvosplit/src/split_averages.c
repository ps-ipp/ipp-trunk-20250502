# include "dvosplit.h"
# define NROWS 1000000 /* ~10MB per block for measures */
# define DNOUT 1000

/* used in find_matches, find_matches_refstars */
# define IN_REGION(REG,R,D) (			     \
((D) >= REG[0].Dmin) && ((D) < REG[0].Dmax) && \
((R) >= REG[0].Rmin) && ((R) < REG[0].Rmax))

// incatalog has already been loaded; we now need to split out the entries to the subcatalogs
// outcatalog[] have already been opened (and are starting at empty)

// this version requires the entire catalog in memory at once
int split_averages (Catalog *incatalog, SkyList *outlist, Catalog *outcatalogs) {

  off_t Nm; // used to track number of values for a given object in a table

  // we need a list of currently-allocated elements for each data type:
  ALLOCATE_PTR (NAVERAGE, int, outlist[0].Nregions);
  ALLOCATE_PTR (NMEASURE, int, outlist[0].Nregions);
  ALLOCATE_PTR (NLENSING, int, outlist[0].Nregions);
  ALLOCATE_PTR (NSTARPAR, int, outlist[0].Nregions);
  ALLOCATE_PTR (NGALPHOT, int, outlist[0].Nregions);

  int Nsecfilt = GetPhotcodeNsecfilt ();

  for (int cat = 0; cat < outlist[0].Nregions; cat++) {
    NAVERAGE[cat] = DNOUT;  REALLOCATE (outcatalogs[cat].average, Average, NAVERAGE[cat]);
    NMEASURE[cat] = DNOUT;  REALLOCATE (outcatalogs[cat].measure, Measure, NMEASURE[cat]);
    NLENSING[cat] = DNOUT;  REALLOCATE (outcatalogs[cat].lensing, Lensing, NLENSING[cat]);
    NSTARPAR[cat] = DNOUT;  REALLOCATE (outcatalogs[cat].starpar, StarPar, NSTARPAR[cat]);
    NGALPHOT[cat] = DNOUT;  REALLOCATE (outcatalogs[cat].galphot, GalPhot, NGALPHOT[cat]);

    REALLOCATE (outcatalogs[cat].secfilt, SecFilt, NAVERAGE[cat]*Nsecfilt);
  }

  // distribute data to the output catalogs
  for (off_t ave = 0; ave < incatalog[0].Naverage; ave++) {
    double inR = incatalog[0].average[ave].R;
    double inD = incatalog[0].average[ave].D;

    // which of the outcatalogs contains this coordinate?
    int Ncat = -1;
    for (int cat = 0; cat < outlist[0].Nregions; cat++) {
      if (!IN_REGION(outlist[0].regions[cat], inR, inD)) continue;
      Ncat = cat;
      break;
    }

    // NO outcatalogs contains this coordinate?
    if (Ncat == -1) {
      fprintf (stderr, "WARNING: missed "OFF_T_FMT" (%f, %f)\n", ave, inR, inD);
      continue;
    }
    
    // these values are the current (next) entry for the table
    off_t averageOut = outcatalogs[Ncat].Naverage;
    off_t measureOut = outcatalogs[Ncat].Nmeasure;
    off_t lensingOut = outcatalogs[Ncat].Nlensing;
    off_t starparOut = outcatalogs[Ncat].Nstarpar;
    off_t galphotOut = outcatalogs[Ncat].Ngalphot;

    // assign the value to the next element of the output catalog
    outcatalogs[Ncat].average[averageOut] = incatalog[0].average[ave];

    // fprintf (stderr, "catalog %s: aveOut: %d m

    // these values track the start of the values for the table for this object
    outcatalogs[Ncat].average[averageOut].measureOffset = measureOut;
    outcatalogs[Ncat].average[averageOut].lensingOffset = lensingOut;
    outcatalogs[Ncat].average[averageOut].starparOffset = starparOut;
    outcatalogs[Ncat].average[averageOut].galphotOffset = galphotOut;
    
    // update secfilt at the same time
    for (int n = 0; n < Nsecfilt; n++) {
      outcatalogs[Ncat].secfilt[averageOut*Nsecfilt + n] = incatalog[0].secfilt[ave*Nsecfilt + n];
    }
    outcatalogs[Ncat].Naverage ++;
    
    if (outcatalogs[Ncat].Naverage >= NAVERAGE[Ncat]) {
      NAVERAGE[Ncat] += DNOUT;
      REALLOCATE (outcatalogs[Ncat].average, Average, NAVERAGE[Ncat]);
      REALLOCATE (outcatalogs[Ncat].secfilt, SecFilt, NAVERAGE[Ncat]*Nsecfilt);
    }

    // assign the Measure values for this object to the output catalog
    Nm = 0;
    for (int j = 0; j < incatalog[0].average[ave].Nmeasure; j++) {
      off_t offset = incatalog[0].average[ave].measureOffset + j;

      outcatalogs[Ncat].measure[measureOut] = incatalog[0].measure[offset];
      outcatalogs[Ncat].measure[measureOut].averef = averageOut;

      measureOut ++;
      Nm ++;
      if (measureOut >= NMEASURE[Ncat]) {
	NMEASURE[Ncat] += DNOUT;
	REALLOCATE (outcatalogs[Ncat].measure, Measure, NMEASURE[Ncat]);
      }
    }
    outcatalogs[Ncat].average[averageOut].Nmeasure = Nm;

    Nm = 0;
    for (int j = 0; j < incatalog[0].average[ave].Nlensing; j++) {
      off_t offset = incatalog[0].average[ave].lensingOffset + j;

      outcatalogs[Ncat].lensing[lensingOut] = incatalog[0].lensing[offset];
      outcatalogs[Ncat].lensing[lensingOut].averef = averageOut;

      lensingOut ++;
      Nm ++;
      if (lensingOut >= NLENSING[Ncat]) {
	NLENSING[Ncat] += DNOUT;
	REALLOCATE (outcatalogs[Ncat].lensing, Lensing, NLENSING[Ncat]);
      }
    }
    outcatalogs[Ncat].average[averageOut].Nlensing = Nm;

    Nm = 0;
    for (int j = 0; j < incatalog[0].average[ave].Nstarpar; j++) {
      off_t offset = incatalog[0].average[ave].starparOffset + j;

      outcatalogs[Ncat].starpar[starparOut] = incatalog[0].starpar[offset];
      outcatalogs[Ncat].starpar[starparOut].averef = averageOut;

      starparOut ++;
      Nm ++;
      if (starparOut == NSTARPAR[Ncat]) {
	NSTARPAR[Ncat] += DNOUT;
	REALLOCATE (outcatalogs[Ncat].starpar, StarPar, NSTARPAR[Ncat]);
      }
    }
    outcatalogs[Ncat].average[averageOut].Nstarpar = Nm;

    Nm = 0;
    for (int j = 0; j < incatalog[0].average[ave].Ngalphot; j++) {
      off_t offset = incatalog[0].average[ave].galphotOffset + j;

      outcatalogs[Ncat].galphot[galphotOut] = incatalog[0].galphot[offset];
      outcatalogs[Ncat].galphot[galphotOut].averef = averageOut;

      galphotOut ++;
      Nm ++;
      if (galphotOut == NGALPHOT[Ncat]) {
	NGALPHOT[Ncat] += DNOUT;
	REALLOCATE (outcatalogs[Ncat].galphot, GalPhot, NGALPHOT[Ncat]);
      }
    }
    outcatalogs[Ncat].average[averageOut].Ngalphot = Nm;

    outcatalogs[Ncat].Nmeasure = measureOut;
    outcatalogs[Ncat].Nlensing = lensingOut;
    outcatalogs[Ncat].Nstarpar = starparOut;
    outcatalogs[Ncat].Ngalphot = galphotOut;

  }

  free (NAVERAGE);
  free (NMEASURE);
  free (NLENSING);
  free (NSTARPAR);
  free (NGALPHOT);

  return TRUE;
}
