/** @file psastroRefstarSubset.c
 *
 *  @brief 
 *
 *  @ingroup libpsastro
 *
 *  @author IfA
 *  @version $Revision: 1.13 $ $Name: not supported by cvs2svn $
 *  @date $Date: 2009-02-07 02:03:34 $
 *  Copyright 2009 Institute for Astronomy, University of Hawaii
 */

# include "psastroInternal.h"

bool psastroRefstarSubset (pmReadout *readout) {

  // select the raw objects for this readout
  psArray *rawstars = psMetadataLookupPtr (NULL, readout->analysis, "PSASTRO.RAWSTARS.SUBSET");
  if (rawstars == NULL)  {
    psError(PSASTRO_ERR_DATA, false, "missing rawstars in psastroRefstarSubset\n");
    return false;
  }

  // select the raw objects for this readout
  psArray *refstars = psMetadataLookupPtr (NULL, readout->analysis, "PSASTRO.REFSTARS.SUBSET");
  if (refstars == NULL)  {
    psError(PSASTRO_ERR_DATA, false, "missing refstars in psastroRefstarSubset\n");
    return false;
  }

  // calculate luminosity functions for rawstars and refstars
  // the samples cover the same area (the chip), so no area correction
  // is needed...
  psLogMsg ("psastro", 4, "measuring luminosity function for rawstars\n");
  pmLumFunc *rawfunc = psastroLuminosityFunction (rawstars, NULL);
  if (rawfunc == NULL) {
    psLogMsg ("psastro", 4, "giving up on rawstars for this readout\n");
    return true;
  }
  psLogMsg ("psastro", 4, "measuring luminosity function for refstars\n");
  pmLumFunc *reffunc = psastroLuminosityFunction (refstars, rawfunc);
  if (reffunc == NULL) {
    psLogMsg ("psastro", 4, "giving up on refstars for this readout\n");
    return true;
  }

// XXX code to better deal with mismatches in the fitted lum function
# if (0)
  // if the fitted slopes differ by too much, give up and just try to match the peak bin
  fSlope = (reffunc->slope / rawfunc->slope);
  if ((fSlope > 1.3) || (fSlope < 0.77)) {
      // XXX do something here (choose the peak of the smaller set, generate a histogram for
      // the other set, then choose the bin from the larger set which has nBin = nPeak
  }
# endif

  // what is the offset between the two lines at the average magnitude?
  double mRef = 0.5*(reffunc->mMin + reffunc->mMax);
  double logRho = mRef * reffunc->slope + reffunc->offset;
  double mRaw = (logRho - rawfunc->offset) / rawfunc->slope;

  psLogMsg ("psastro", 4, "mRef: %f, logRho: %f, mRaw: %f\n", mRef, logRho, mRaw);

  double mRefMax = rawfunc->mMax - mRaw + mRef;
  psLogMsg ("psastro", 4, "clipping stars fainter than %f\n", mRefMax);

  psArray *subset = psArrayAllocEmpty (100);
  for (int i = 0; i < refstars->n; i++) {
    pmAstromObj *ref = refstars->data[i];
    if (ref->Mag > mRefMax) continue;
    psArrayAdd (subset, 100, ref);
  }

  psLogMsg ("psastro", 4, "keeping %ld of %ld reference stars\n", subset->n, refstars->n);

  psMetadataRemoveKey (readout->analysis, "PSASTRO.REFSTARS.SUBSET");
  psMetadataAdd (readout->analysis, PS_LIST_TAIL, "PSASTRO.REFSTARS.SUBSET", PS_DATA_ARRAY, "astrometry matches", subset);

  if (psTraceGetLevel("psastro.dump") > 0) {
      pmChip *chip = readout->parent->parent;

      char *filename = NULL;
      char *chipname = psMetadataLookupStr (NULL, chip->concepts, "CHIP.NAME");
      psStringAppend (&filename, "refstars.%s.dat", chipname);
      psastroDumpRefstars (subset, filename);
      psFree (filename);
  }

  psFree (rawfunc);
  psFree (reffunc);
  psFree (subset);

  return true;
}

/* this test is a bit sensitive to the total number of refstars or rawstars available
   watch out if:
   - the fitted slopes are extremely different
   - the average number of stars per bin is ~1

   skip the cut in these cases?
*/
