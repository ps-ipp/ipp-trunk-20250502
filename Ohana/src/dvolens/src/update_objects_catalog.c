# include "dvolens.h"
# define SCALE 0.001
float MagToFlux (float Mag); // in libdvo, but not exposed?

# define TIMESTAMP(TIME){				\
  gettimeofday (&stopTimer, (void *) NULL);		\
  double dtime = DTIME (stopTimer, startTimer);		\
  TIME += dtime;					\
  gettimeofday (&startTimer, (void *) NULL); }

int update_objects_catalog (Catalog *catalog) {

  off_t i, j, Lj, Mj;

  if (VERBOSE2) fprintf (stderr, "lensobj catalog %d : "OFF_T_FMT" ave, "OFF_T_FMT" meas, "OFF_T_FMT" lensing\n", 
			 catalog->catID, catalog->Naverage, catalog->Nmeasure, catalog->Nlensing);

  // I am making this measurement per filter
  int Nsecfilt = GetPhotcodeNsecfilt ();
  
  ALLOCATE_PTR (Mxx_obj, float, Nsecfilt);
  ALLOCATE_PTR (Mxy_obj, float, Nsecfilt);
  ALLOCATE_PTR (Myy_obj, float, Nsecfilt);

  ALLOCATE_PTR (N5, int, Nsecfilt);
  ALLOCATE_PTR (N6, int, Nsecfilt);
  ALLOCATE_PTR (N7, int, Nsecfilt);

  // I think we have already allocated lensobj
  REALLOCATE (catalog->lensobj, Lensobj, catalog->Naverage * Nsecfilt);

  // Nlensobj tracks how many lensobj entries have I actually generated so far for this
  // catalog This is not necessarily 1-to-1 with secfilt entries, but any average entry
  // will either have 0 or Nsecfilt lensobj entries
  int Nlensobj = 0;

  mySequenceType *measureSeq = mySequenceAlloc();

  int Nfixed = 0;
  int Nfailed = 0;

  INITTIME;
  double time1 = 0.0;
  double time2 = 0.0;
  double time3 = 0.0;
  double time4 = 0.0;

  int NfixLensing = 0;
  int NgoodLensing = 0;
  int NmissLensing = 0;
  int NmissWarps   = 0;

  for (i = 0; i < catalog->Naverage; i++) {
    
    Average *average = &catalog->average[i];
    if (average->Nlensing == 0) continue;

    // NOTE Nlensobj increments by Nsecfilt each time
    Lensobj *lensobj = &catalog->lensobj[Nlensobj];
    for (j = 0; j < Nsecfilt; j++) {
      dvo_lensobj_init (&lensobj[j], TRUE); // init accumulated values to 0
      Mxx_obj[j] = 0.0;
      Mxy_obj[j] = 0.0;
      Myy_obj[j] = 0.0;

      N5[j] = 0;
      N6[j] = 0;
      N7[j] = 0;
    }

    // reset the average values
    off_t Loff = average->lensingOffset;
    off_t Moff = average->measureOffset;

    // repair lensing imageID values by asserting the warp measurements and lensing measurements are in the same order
    if (REPAIR_LENSING_IDS_FROM_WARPS) {
      int Nmatch = 0;
      for (Mj = 0, Lj = 0; (Mj < average->Nmeasure) && (Lj < average->Nlensing); Mj++) {
	Measure *measure = &catalog->measure[Moff + Mj];
	if (!isGPC1warp(measure->photcode)) continue;
	
	Lensing *lensing = &catalog->lensing[Loff + Lj];
	if (lensing->detID == measure->detID) {
	  if (lensing->imageID == measure->imageID) {
	    NgoodLensing ++;
	  } else {
	    lensing->oldImID = lensing->imageID;
	    lensing->imageID = measure->imageID;
	    NfixLensing ++;
	  }
	  Nmatch ++;
	}
	Lj ++;
      }
      if (Nmatch != Lj) { NmissLensing ++; }
      
      while (Mj < average->Nmeasure) {
	Measure *measure = &catalog->measure[Moff + Mj];
	Mj ++;
	if (!isGPC1warp(measure->photcode)) continue;
	NmissWarps ++;
      }
    }      

    // I have Nmeasure entries for this object.  I need to match measure to lensing (by imageID)
    // I could generated a sorted list (imageID, measureSeq) and use bisection to find the desired imageID
    // I could make an index measureSeq[imageID-imageIDmin]
    
    TIMESTAMP (time1);

    mySequenceSetSize (measureSeq, average->Nmeasure);

    // generate an index for these measure entries (based on Mj and imageID)
    for (Mj = 0; Mj < average->Nmeasure; Mj++) {
      mySequenceSetValue (measureSeq, catalog->measure[Moff + Mj].imageID, Mj);
    }

    mySequenceSort (measureSeq);

    TIMESTAMP (time2);

    // loop over the lensing measurements.  for each one, I need to find the corresponding measurement (make an index in lensing?)
    for (Lj = 0; Lj < average->Nlensing; Lj++) {
      
      // find the corresponding measure
      Lensing *lensing = &catalog->lensing[Loff + Lj];
      if (!isfinite(lensing->X11_sm_obj)) continue;

      // pointer from lensing entry to corresponding measure entry (keep updated?)
      // XX ALT int Mseq = lensing->measureSeq;
      // XX ALT myAssert (Mseq < average->Nmeasure, "oops");
      // XX ALT Measure *measure = &catalog->measure[Moff + Mseq];
      // XX ALT myAssert (measure->imageID == lensing->imageID, "oops, deux");

      Mj = mySequenceGetEntry (measureSeq, lensing->imageID);

      // if we are unable to find a match in the measure table, we may have the wrong 
      // imageID.  there was a bug in which we only matched to the correct warp obstime,
      // but got the wrong skycell.  if this is the case, we can try to recover by finding
      // the set of warps which match our obstime, then choosing the one from that set
      // which matches one of our warps.
      if (Mj < 0) {
	if (REPAIR_LENSING_IDS) {
	  Mj = RecoverLensingIndex (average, measureSeq, lensing);
	  if (Mj == -1) {
	    if (Nfailed < 10) fprintf (stderr, "failed %f %f : %d\n", average->R, average->D, lensing->imageID);
	    Nfailed ++;
	    continue;
	  }
	  Nfixed ++;
	} else {
	  Nfailed ++;
	  continue;
	}
      }

      Measure *measure = &catalog->measure[Moff + Mj];
      myAssert (measure->imageID == lensing->imageID, "oops, deux");

# if (0)      
      Measure *measure = NULL;
      int found = FALSE;
      for (Mj = 0; !found && (Mj < average->Nmeasure); Mj++) {
	measure = &catalog->measure[Moff + Mj];
	if (measure->imageID != lensing->imageID) continue;
	found = TRUE;
      }
      if (!found) {
	fprintf (stderr, "error: cannot match measurement with lensing parameter\n");
	fprintf (stderr, "objID: %d, catID: %d, detID: %d, imageID: %d, averef: %d\n",
		 lensing->objID, lensing->catID, lensing->detID, lensing->imageID, lensing->averef); 
	abort();
      }
# endif
      
      // skip measurements that do not match the current photcode
      PhotCode *code = GetPhotcodebyCode (measure->photcode);
      myAssert (code, "missing photcode?");
      myAssert (code->equiv > -1, "photcode not equivalent to secfilt?");
      
      int Nsec = GetPhotcodeNsec (code->equiv);
      myAssert (Nsec > -1, "cannot find Nsec?");
	
      // if (measure->dbFlags & MEAS_BAD) SKIP_THIS_MEAS(Nbad); 
      if (measure->psfQF < 0.85) continue; 
      if (measure->psfQFperf < 0.85) continue; 
      
      // I should probably only use lensing entries which correspond to warps 
      // included in the mean warp flux (setMrelCatalog.c)
      if ((measure->dbFlags & ID_MEAS_WARP_USED) == 0) continue;

      Mxx_obj[Nsec] += measure->Mxx;
      Mxy_obj[Nsec] += measure->Mxy;
      Myy_obj[Nsec] += measure->Myy;
      
      lensobj[Nsec].X11_sm_obj += lensing->X11_sm_obj;
      lensobj[Nsec].X12_sm_obj += lensing->X12_sm_obj;
      lensobj[Nsec].X22_sm_obj += lensing->X22_sm_obj;
      lensobj[Nsec]. E1_sm_obj += lensing-> E1_sm_obj;
      lensobj[Nsec]. E2_sm_obj += lensing-> E2_sm_obj;

      lensobj[Nsec].X11_sh_obj += lensing->X11_sh_obj;
      lensobj[Nsec].X12_sh_obj += lensing->X12_sh_obj;
      lensobj[Nsec].X22_sh_obj += lensing->X22_sh_obj;
      lensobj[Nsec]. E1_sh_obj += lensing-> E1_sh_obj;
      lensobj[Nsec]. E2_sh_obj += lensing-> E2_sh_obj;

      lensobj[Nsec].X11_sm_psf += lensing->X11_sm_psf;
      lensobj[Nsec].X12_sm_psf += lensing->X12_sm_psf;
      lensobj[Nsec].X22_sm_psf += lensing->X22_sm_psf;
      lensobj[Nsec]. E1_sm_psf += lensing-> E1_sm_psf;
      lensobj[Nsec]. E2_sm_psf += lensing-> E2_sm_psf;

      lensobj[Nsec].X11_sh_psf += lensing->X11_sh_psf;
      lensobj[Nsec].X12_sh_psf += lensing->X12_sh_psf;
      lensobj[Nsec].X22_sh_psf += lensing->X22_sh_psf;
      lensobj[Nsec]. E1_sh_psf += lensing-> E1_sh_psf;
      lensobj[Nsec]. E2_sh_psf += lensing-> E2_sh_psf;

      // relphot sets measure->Mcal (setMcalOutput.c, called by setMrelFinal.c)
      // XXX : I'm using McalAPER since these lens measurements are aperture-like, right?
      float Mcal = code[0].K*(measure[0].airmass - 1.000) + SCALE*code[0].C - measure->McalAPER;
      float Fcal = 3630.8 * MagToFlux(Mcal);

      // lensing->F_ApR5, etc need to be in units of DN/sec
      // Fcal * lensing->F_ApR5 is in Jy

      // F_ApR5 is <F_ApR5_i>, the mean of the lensing entries.  we will save stdev of F_ApR5_i, 
      // not the r.m.s. of sF_ApR5

      // stdev = sqrt(F_Ap_R5_i^2 / N - <F_Ap_R5>) 

      if (isfinite(lensing-> F_ApR5)) {
	lensobj[Nsec]. F_ApR5 +=    Fcal * lensing-> F_ApR5;
	lensobj[Nsec].dF_ApR5 += SQ(Fcal * lensing->dF_ApR5);
	lensobj[Nsec].sF_ApR5 += SQ(Fcal * lensing-> F_ApR5);
	lensobj[Nsec].fF_ApR5 +=           lensing->fF_ApR5;
	N5[Nsec] ++;
      }

      if (isfinite(lensing-> F_ApR6)) {
	lensobj[Nsec]. F_ApR6 +=    Fcal * lensing-> F_ApR6;
	lensobj[Nsec].dF_ApR6 += SQ(Fcal * lensing->dF_ApR6);
	lensobj[Nsec].sF_ApR6 += SQ(Fcal * lensing-> F_ApR6);
	lensobj[Nsec].fF_ApR6 +=           lensing->fF_ApR6;
	N6[Nsec] ++;
      }

      if (isfinite(lensing-> F_ApR7)) {
	lensobj[Nsec]. F_ApR7 +=    Fcal * lensing-> F_ApR7;
	lensobj[Nsec].dF_ApR7 += SQ(Fcal * lensing->dF_ApR7);
	lensobj[Nsec].sF_ApR7 += SQ(Fcal * lensing-> F_ApR7);
	lensobj[Nsec].fF_ApR7 +=           lensing->fF_ApR7;
	N7[Nsec] ++;
	// fprintf (stderr, "%f %f %f %f\n", lensing-> F_ApR7, lensing->dF_ApR7, lensing->sF_ApR7, lensing->fF_ApR7);
	// fprintf (stderr, "%f %f %f %f : %d\n", lensobj[Nsec].F_ApR7, lensobj[Nsec].dF_ApR7, lensobj[Nsec].sF_ApR7, lensobj[Nsec].fF_ApR7, N7[Nsec]);
      }

      lensobj[Nsec].Nmeas   ++;
      lensobj[Nsec].photcode = code->equiv;
      lensobj[Nsec].objID = lensing->objID;
      lensobj[Nsec].catID = lensing->catID;
    }
    
    TIMESTAMP (time3);

    for (j = 0; j < Nsecfilt; j++) {
      if (!lensobj[j].Nmeas) {
	dvo_lensobj_init (&lensobj[j], FALSE);
	continue;
      }
      float Nmeas = lensobj[j].Nmeas;
      lensobj[j].X11_sm_obj /= Nmeas;
      lensobj[j].X12_sm_obj /= Nmeas;
      lensobj[j].X22_sm_obj /= Nmeas;
      lensobj[j]. E1_sm_obj /= Nmeas;
      lensobj[j]. E2_sm_obj /= Nmeas;
      lensobj[j].X11_sh_obj /= Nmeas;
      lensobj[j].X12_sh_obj /= Nmeas;
      lensobj[j].X22_sh_obj /= Nmeas;
      lensobj[j]. E1_sh_obj /= Nmeas;
      lensobj[j]. E2_sh_obj /= Nmeas;
      lensobj[j].X11_sm_psf /= Nmeas;
      lensobj[j].X12_sm_psf /= Nmeas;
      lensobj[j].X22_sm_psf /= Nmeas;
      lensobj[j]. E1_sm_psf /= Nmeas;
      lensobj[j]. E2_sm_psf /= Nmeas;
      lensobj[j].X11_sh_psf /= Nmeas;
      lensobj[j].X12_sh_psf /= Nmeas;
      lensobj[j].X22_sh_psf /= Nmeas;
      lensobj[j]. E1_sh_psf /= Nmeas;
      lensobj[j]. E2_sh_psf /= Nmeas;

      if (N5[j]) {
	lensobj[j]. F_ApR5 /= (float) N5[j];
	lensobj[j].fF_ApR5 /= (float) N5[j];
	lensobj[j].dF_ApR5  = sqrt(lensobj[j].dF_ApR5 / (float) N5[j]);

	double S1 = SQ(lensobj[j]. F_ApR5); // <f>^2
	double S2 = lensobj[j].sF_ApR5 / (float) N5[j]; // sum(f^2) / N
	lensobj[j].sF_ApR5  = sqrt(S2 - S1) * (N5[j] / (N5[j] - 1.0)); // correct to sample stdev
      } else {
	lensobj[j]. F_ApR5 = NAN;
	lensobj[j].dF_ApR5 = NAN;
	lensobj[j].sF_ApR5 = NAN;
	lensobj[j].fF_ApR5 = NAN;
      }

      if (N6[j]) {
	lensobj[j]. F_ApR6 /= (float) N6[j];
	lensobj[j].fF_ApR6 /= (float) N6[j];
	lensobj[j].dF_ApR6 = sqrt(lensobj[j].dF_ApR6 / (float) N6[j]);

	double S1 = SQ(lensobj[j]. F_ApR6); // <f>^2
	double S2 = lensobj[j].sF_ApR6 / (float) N6[j]; // sum(f^2) / N
	lensobj[j].sF_ApR6 = sqrt(S2 - S1) * (N6[j] / (N6[j] - 1.0)); // correct to sample stdev
      } else {
	lensobj[j]. F_ApR6 = NAN;
	lensobj[j].dF_ApR6 = NAN;
	lensobj[j].sF_ApR6 = NAN;
	lensobj[j].fF_ApR6 = NAN;
      }

      if (N7[j]) {
	lensobj[j]. F_ApR7 /= (float) N7[j];
	lensobj[j].fF_ApR7 /= (float) N7[j];
	lensobj[j].dF_ApR7 = sqrt(lensobj[j].dF_ApR7 / (float) N7[j]);

	double S1 = SQ(lensobj[j]. F_ApR7); // <f>^2
	double S2 = lensobj[j].sF_ApR7 / (float) N7[j]; // sum(f^2) / N
	lensobj[j].sF_ApR7  = sqrt(S2 - S1) * (N7[j] / (N7[j] - 1.0)); // correct to sample stdev
      } else {
	lensobj[j]. F_ApR7 = NAN;
	lensobj[j].dF_ApR7 = NAN;
	lensobj[j].sF_ApR7 = NAN;
	lensobj[j].fF_ApR7 = NAN;
      }

      float e0 = Mxx_obj[j] + Myy_obj[j];
      lensobj[j].E1 = (Mxx_obj[j] - Myy_obj[j]) / e0;
      lensobj[j].E2 = 2.0 * Mxy_obj[j] / e0;
    }

    TIMESTAMP (time4);

    average->Nlensobj = Nsecfilt;
    average->lensobjOffset = Nlensobj;
    Nlensobj += Nsecfilt;
    // XVERB |= (catalog->average[j].objID == OBJ_ID_SRC) && (catalog->average[j].catID == CAT_ID_SRC);
    // XVERB |= (catalog->average[j].objID == OBJ_ID_DST) && (catalog->average[j].catID == CAT_ID_DST);
  }

  free (Mxx_obj);
  free (Mxy_obj);
  free (Myy_obj);

  free (N5);
  free (N6);
  free (N7);

  mySequenceFree (measureSeq);

  fprintf (stderr, "done with %d aves (%f %f %f %f)\n", (int) catalog->Naverage, time1, time2, time3, time4);

  if (REPAIR_LENSING_IDS) fprintf (stderr, "corrected %d lensing IDs, failed on %d lensing IDs\n", Nfixed, Nfailed);

  if (REPAIR_LENSING_IDS_FROM_WARPS) fprintf (stderr, "Ngood %d Nfixed %d missed %d lensing IDs, %d warps\n", NgoodLensing, NfixLensing, NmissLensing, NmissWarps);

  catalog->Nlensobj = Nlensobj;
  catalog->Nlensobj_disk = 0;
  catalog->Nlensobj_off  = 0;
  REALLOCATE (catalog->lensobj, Lensobj, Nlensobj);

  return (TRUE);
}
