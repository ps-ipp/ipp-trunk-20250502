# include "relphot.h"
# include "lensing.h"

# define SCALE 0.001
static float MagToFlux (float Mag) {
  float Flux = pow(10.0, -0.4*(Mag));
  return (Flux);
}

// This function simultaneously generates the lensobj table entries for each object
// with lensing measurements and calculates the average values
// NOTE: 'lensing' and 'lensobj' names are archaic : they really hold xrad values

int setXradAverages (Catalog *catalog) {

  if (VERBOSE2) fprintf (stderr, "lensobj catalog %d : "OFF_T_FMT" ave, "OFF_T_FMT" meas, "OFF_T_FMT" lensing\n", catalog->catID, catalog->Naverage, catalog->Nmeasure, catalog->Nlensing);

  int Nsecfilt = GetPhotcodeNsecfilt ();
  
  // save the photcodes for the Nsec values (assigned to lensobj below)
  ALLOCATE_PTR (photcodeVals, int, Nsecfilt);
  for (int Nsec = 0; Nsec < Nsecfilt; Nsec++) {
    PhotCode *code = GetPhotcodebyNsec (Nsec);
    photcodeVals[Nsec] = code->code;
  }

  // myLensobj[] is an intermediate accumulation structure
  ALLOCATE_PTR (myLensobj, Lensobj, Nsecfilt);

  // myLensctr holds counters for each of the radial apertures / types
  Lensctr *myLensctr = dvo_lensctr_init (Nsecfilt);

  // I think we have already allocated lensobj
  int NLENSOBJ = 10000;
  REALLOCATE (catalog->lensobj, Lensobj, NLENSOBJ);

  // Nlensobj tracks how many lensobj entries have I actually generated so far for this
  // catalog.  An object may have 0, 1, or more lensobj entries (max = Nsecfilt).  In
  // UNIONS DR3, I have lensobj values only for the i-band, but Nsecfilt = 6

  int Nlensobj = 0;

  // In a sorted database, the lensing and measure values for a single object are all in the range:
  // average->measureOffset : average->measureOffset + average->Nmeasure
  // average->lensingOffset : average->lensingOffset + average->Nlensing
  // NOTE: all lensing entries must have a matching measurement entry (same detID, same imageID)
  // but not all measurement entries must have a lensing entry
  
  // For each object:
  // 1) I need to examine the lensing entries and match them to their corresponding measure entries
  // 2) determine the set of unique photcodes in the lensing set (XXX in future, overload photcode in Lensing).

  for (off_t i = 0; i < catalog->Naverage; i++) {
    
    Average *average = &catalog->average[i];
    if (average->Nlensing == 0) continue;

    // start of the measure and lensing sequences
    off_t Loff = average->lensingOffset;
    off_t Moff = average->measureOffset;

    // reset the myLensobj accumulators
    for (int Nsec = 0; Nsec < Nsecfilt; Nsec ++) {
      dvo_lensobj_init (&myLensobj[Nsec], TRUE); // init accumulated values to 0
      myLensobj[Nsec].photcode = photcodeVals[Nsec]; // set the photcodes for the accumulators
    }
    dvo_lensctr_reset (myLensctr, Nsecfilt); // init counters to 0

    // assign the lensing values to the appropriate lensobj
    for (int Lj = 0; Lj < average->Nlensing; Lj ++) {
      Lensing *lensing = &catalog->lensing[Loff + Lj];
      if (lensing->detID < 0) continue;  // XXX some invalid lensing entries?

      // need to find the corresponding Measure to get the photcode
      int foundMeasure = FALSE;
      for (int Mj = 0; !foundMeasure && (Mj < average->Nmeasure); Mj ++) {
	Measure *measure = &catalog->measure[Moff + Mj];

	// skip the mismatched entries
	if (lensing->detID   != measure->detID) continue;
	if (lensing->imageID != measure->imageID) continue;

	// ** this is the matched entry
	foundMeasure = TRUE;
	lensing->oldImID = measure->photcode; // XXX save the photcode on the lensing structure (unused element)

	PhotCode *code = GetPhotcodebyCode (measure->photcode);
	myAssert (code, "missing photcode?");
	myAssert (code->equiv > -1, "photcode not equivalent to secfilt?");
	  
	int Nsec = GetPhotcodeNsec (code->equiv);
	myAssert (Nsec > -1, "cannot find Nsec?");

	// relphot sets measure->Mcal (setMcalOutput.c, called by setMrelFinal.c)
	// XXX : I'm using McalAPER since these lens measurements are aperture-like, right?
	float Mcal = code[0].K*(measure->airmass - 1.000) + SCALE*code->C - measure->McalAPER;
	float Fcal = 3630.8 * MagToFlux(Mcal);
	  
	// lensing->F_ApR5, etc are in units of DN/sec
	// Fcal * lensing->F_ApR5 is in Jy
	  
	// F_ApR5 is <F_ApR5_i>, the mean of the lensing entries.  we will save stdev of F_ApR5_i, 
	// not the r.m.s. of sF_ApR5
	  
	// stdev = sqrt(F_Ap_R5_i^2 / N - <F_Ap_R5>) 
	  
	// accumulate valid values
	dvo_lensing_accum (&myLensobj[Nsec], &myLensctr[Nsec], lensing, Fcal);

	// XXX TEST:
	if (average->objID == 1702) {
	  fprintf (stderr, "%e : %e : %e\n", Fcal, lensing->F_ApR5, myLensobj[Nsec].F_ApR5);
	}
      }
      myAssert (foundMeasure, "oops, unmatched lensing entry");
    }

    average->Nlensobj = 0;

    // now loop over the Nsec values and calculate averages for each
    for (int Nsec = 0; Nsec < Nsecfilt; Nsec ++) {
      if (!dvo_lensctr_has_values (&myLensctr[Nsec])) continue;
      
      // calculate averages
      dvo_lensobj_aves (&myLensobj[Nsec], &myLensctr[Nsec]);

      // copy into the output array
      catalog->lensobj[Nlensobj] = myLensobj[Nsec];
      catalog->lensobj[Nlensobj].objID = average->objID;
      catalog->lensobj[Nlensobj].catID = average->catID;

      if (!average->Nlensobj) {
	average->lensobjOffset = Nlensobj;
      }
      average->Nlensobj ++;
      Nlensobj ++;

      CHECK_REALLOCATE (catalog->lensobj, Lensobj, NLENSOBJ, Nlensobj, 1000);
    }
  }

  catalog->Nlensobj = Nlensobj;
  catalog->Nlensobj_disk = 0;
  catalog->Nlensobj_off  = 0;
  REALLOCATE (catalog->lensobj, Lensobj, Nlensobj);

  free (myLensobj);
  free (myLensctr);
  free (photcodeVals);

  return TRUE;
}

