/** @file  pmKHcorrect.c
 *  @brief Functions to read (and write?) Koppenhoefer correction file
 *
 *  The Koppenhoefer correction is needed for some chips of gpc1 before the camera voltages were adjusted 2011/05/11.
 *  The correction is a modification of the X (and possibly Y) coordinate of a star which depends on the instrumental 
 *  surface brightness, defined as -2.5 log_10 (DN) + 5.0 log_10 fwhm_maj [XXX be careful about the definition of fwhm_maj]
 *
 *  @ingroup AstroImage
 *  @author EAM, IfA
 *
 *  Copyright 2014 Institute for Astronomy, University of Hawaii
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

/******************************************************************************/
/*  INCLUDE FILES                                                             */
/******************************************************************************/
#include <stdio.h>
#include <strings.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include <unistd.h>   // for unlink
#include <pslib.h>

#include "pmConfig.h"
#include "pmDetrendDB.h"
#include "pmHDU.h"
#include "pmFPA.h"
#include "pmFPALevel.h"
#include "pmFPAview.h"
#include "pmFPAfile.h"
#include "pmFPAExtent.h"
#include "pmFPAfileFitsIO.h"
#include "pmConcepts.h"
#include "pmKHcorrect.h"

static void KHcorrectDataFree (KHcorrectData *spline) {

    if (!spline) return;

    psFree (spline->xk);
    psFree (spline->yk);
    psFree (spline->y2);
    return;
}

KHcorrectData *KHcorrectDataAlloc (int Nrow) {

    // allocate xk[Nrow], etc

    KHcorrectData *spline = (KHcorrectData *) psAlloc(sizeof(KHcorrectData));
    psMemSetDeallocator(spline, (psFreeFunc) KHcorrectDataFree);

    spline->N  = Nrow;
    spline->xk = (float *) psAlloc(Nrow*sizeof(float));
    spline->yk = (float *) psAlloc(Nrow*sizeof(float));
    spline->y2 = (float *) psAlloc(Nrow*sizeof(float));

    return spline;
}

// the KH correction is a function of the instrumental surface brightness, SBinst
float KHcorrectApply (KHcorrectData *spline, float X) {

    int N = spline->N;

    // saturate correction at high and low ends
    if (X < spline->xk[  0]) return spline->yk[  0];
    if (X > spline->xk[N-1]) return spline->yk[N-1];

    float *xk = spline->xk;
    float *yk = spline->yk;
    float *y2 = spline->y2;

    /* find correct element in array (x must be sorted) */
    int lo = 0;
    int hi = N-1;
    while (hi - lo > 1) {
	int i = 0.5*(hi+lo);
	if (xk[i] > X) {
	    hi = i;
	} else {
	    lo = i;
	}
    }

    /* error condition: duplicate abssisca */
    float dx = xk[hi] - xk[lo];
    if (dx == 0.0) {
	return (0.0);
    }

    /* evaluate spline */
    float a = (xk[hi] - X) / dx;
    float b = (X - xk[lo]) / dx;

    float value = a*yk[lo] + b*yk[hi] + ((a*a - 1.0)*a*y2[lo] + (b*b - 1.0)*b*y2[hi])*(dx*dx) / 6.0;
    return (value);
}

/********************* CheckDataStatus functions *****************************/

bool pmKHcorrectCheckDataStatusForView (const pmFPAview *view, pmFPAfile *file) {
    psError(PS_ERR_IO, false, "Check Data Status not defined");
    return false;
}

bool pmKHcorrectCheckDataStatusForFPA (const pmFPA *fpa) {
    psError(PS_ERR_IO, false, "Check Data Status not defined");
    return false;
}

bool pmKHcorrectCheckDataStatusForChip (const pmChip *chip) {
    psError(PS_ERR_IO, false, "Check Data Status not defined");
    return false;
}

/********************* Write Data functions *****************************/

// NOTE : these are not exposed because I don't think we need them (we do not create KHcorrect
// in psModules based tool, but in DVO.

bool pmKHcorrectWriteFPA (pmFPAfile *file, const pmFPA *fpa);
bool pmKHcorrectWriteForView (const pmFPAview *view, pmFPAfile *file, pmConfig *config)
{
    // write the full model in one pass: require the level to be FPA
    if (view->chip != -1) {
        psError(PS_ERR_IO, false, "Koppenhoefer Correction must be written at the FPA level");
        return false;
    }

    pmFPA *fpa = pmFPAfileSuitableFPA(file, view, config, false); // Suitable FPA for writing

    if (!pmKHcorrectWriteFPA(file, fpa)) {
        psError(PS_ERR_IO, false, "Failed to write KH Correction for fpa");
        psFree(fpa);
        return false;
    }

    psFree(fpa);

    return true;
}

// write out all chip-level KH Correction data for this FPA
bool pmKHcorrectWriteFPA (pmFPAfile *file, const pmFPA *fpa)
{
    psError(PS_ERR_IO, false, "output for KH Correction is not defined");
    return false;
}

/********************* Read Data functions *****************************/

bool pmKHcorrectReadForView (const pmFPAview *view, pmFPAfile *file, const pmConfig *config)
    {
        // read the full model in one pass: require the level to be FPA
        if (view->chip != -1) {
            psError(PS_ERR_IO, false, "KH Correction must be read at the FPA level");
            return false;
        }

        if (!pmKHcorrectReadFPA (file)) {
            psError(PS_ERR_IO, false, "Failed to read KH Correction for fpa");
            return false;
        }
        return true;
    }

// read in all chip-level KH Correction data for this FPA
bool pmKHcorrectReadFPA (pmFPAfile *file) {

    if (!pmKHcorrectReadChips (file)) {
        psError(PS_ERR_IO, false, "Failed to read KH Correction for chips");
        return false;
    }

    return true;
}

// read the set of tables, one for each chip
bool pmKHcorrectReadChips (pmFPAfile *file) {

    bool haveData, status;

    // loop over the extensions
    // for each extension, use the extname (eg, XY01.DX.T0) to assign to a chip

    // move to the start of the file
    haveData = psFitsMoveExtNum (file->fits, 1, false);
    if (!haveData) {
        psError(PS_ERR_IO, false, "Failed to read even the first extension?");
        return false;
    }

    while (haveData) {

	// load the header
	psMetadata *header = psFitsReadHeader(NULL, file->fits); // The FITS header
	if (!header) psAbort("cannot read model header");

	// load the full model in one shot
	psArray *model = psFitsReadTable (file->fits);
	if (!model) psAbort("cannot read model");
	
	// determine the chip:
	char *extname = psMetadataLookupStr (&status, header, "EXTNAME");
	psLogMsg ("psModules.astrom", 4, "read %ld rows from Koppenhoefer correction file, extname %s\n", model->n, extname);

	// I expect to find a name of the form: chipName.dir.tset (eg, XY01.DX.T0)
	// where chipName like 'XY01'
	// dir = 'DX' 
	// tset = 'T0'
	psAssert (strlen(extname) == 10, "invalid extension %s", extname);
	psAssert (extname[5] == 'D', "invalid extension %s", extname);
	psAssert (extname[6] == 'X', "invalid extension %s", extname);
	psAssert (extname[8] == 'T', "invalid extension %s", extname);
	psAssert (extname[9] == '0', "invalid extension %s", extname);

	char chipName[5];
	strncpy (chipName, extname, 4);
	chipName[4] = 0;

	pmChip *chip = pmConceptsChipFromName (file->fpa, chipName);
	if (!chip) psAbort ("invalid chip?");

	KHcorrectData *spline = KHcorrectDataAlloc (model->n);

	// parse the model entries
	for (int i = 0; i < model->n; i++) {
	    psMetadata *row = model->data[i];

	    spline->xk[i] = psMetadataLookupF32(&status, row, "X_KNOT");
	    spline->yk[i] = psMetadataLookupF32(&status, row, "Y_KNOT");
	    spline->y2[i] = psMetadataLookupF32(&status, row, "DY2_DX");
	}
	psMetadataAddUnknown (chip->analysis, PS_LIST_TAIL, "KH.CORRECT", PS_META_REPLACE, "", spline);
	psFree (spline);

	psFree (model);
	psFree (header);

	// move to the next extension
	haveData = psFitsMoveExtNum (file->fits, 1, true);
    }

    return true;
}

