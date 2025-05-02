/** @file psastroModelAnalysis.c
 *
 *  @brief
 *
 *  @ingroup psastroModel
 *
 *  @author IfA
 *  @version $Revision: 1.7 $ $Name: not supported by cvs2svn $
 *  @date $Date: 2009-02-07 02:03:34 $
 *  Copyright 2009 Institute for Astronomy, University of Hawaii
 */

# include "psastroStandAlone.h"
# define NONLIN_TOL 0.001

bool psastroModelAnalysis (pmConfig *config) {

    bool status;

    // select the current recipe
    psMetadata *recipe  = psMetadataLookupPtr (&status, config->recipes, PSASTRO_RECIPE);
    if (!recipe) {
	psError(PSASTRO_ERR_CONFIG, true, "Can't find PSASTRO recipe");
	return false;
    }

    // reference chip for boresite parameters
    char *refChip = psMetadataLookupStr (&status, recipe, "PSASTRO.MODEL.REF.CHIP");
    if (!refChip) {
	psError(PS_ERR_IO, true, "reference chip is missing from recipe"); 
	return false; 
    } 

    pmFPAfile *output = psMetadataLookupPtr (&status, config->files, "PSASTRO.OUT.MODEL");
    if (!status) psAbort ("Can't find output pmFPAfile PSASTRO.OUT.MODEL");

    // measure the boresite position from a rotation sequence?
    bool fitBoresite = psMetadataLookupBool (&status, recipe, "PSASTRO.MODEL.FIT.BORESITE");
    if (!fitBoresite) {
	psMetadataAddF32 (output->fpa->concepts, PS_LIST_TAIL, "FPA.BORE.X0", PS_META_REPLACE, "boresite parameter", 0.0); 
	psMetadataAddF32 (output->fpa->concepts, PS_LIST_TAIL, "FPA.BORE.Y0", PS_META_REPLACE, "boresite parameter", 0.0); 
	psMetadataAddF32 (output->fpa->concepts, PS_LIST_TAIL, "FPA.BORE.RX", PS_META_REPLACE, "boresite parameter", 0.0); 
	psMetadataAddF32 (output->fpa->concepts, PS_LIST_TAIL, "FPA.BORE.RY", PS_META_REPLACE, "boresite parameter", 0.0); 
	psMetadataAddF32 (output->fpa->concepts, PS_LIST_TAIL, "FPA.BORE.T0", PS_META_REPLACE, "boresite parameter", 0.0); 
	psMetadataAddF32 (output->fpa->concepts, PS_LIST_TAIL, "FPA.BORE.P0", PS_META_REPLACE, "boresite parameter", 0.0); 
	psMetadataAddStr (output->fpa->concepts, PS_LIST_TAIL, "FPA.REF.CHIP", PS_META_REPLACE, "boresite parameter", refChip);
	return true; 
    } 

    char *outroot = psMetadataLookupStr (&status, config->arguments, "OUTPUT");
    if (!status || !outroot) psAbort ("Can't find outroot on config->arguments");

    /* model analysis:
     *
     * determine POS_ZERO via comparison of measured and reported posangles
     * POS_ZERO = FPA.POSANGLE - posangle
     *
     * determine boresite model:
     * X = Xo + R_X cos(FPA.POSANGLE - T_0) cos(P_0) + R_Y sin(FPA.POSANGLE - T_0) sin(P_0) 
     * Y = Yo + R_Y sin(FPA.POSANGLE - T_0) cos(P_0) - R_X cos(FPA.POSANGLE - T_0) sin(P_0) 
     * position of reported boresite in reference chip pixels
     * Xo, Yo : true coordinate of boresite (rotator center) in reference chip pixels
     * R_X, R_Y : amplitude of boresite offset
     * T_0 : reference angle for rotator
     * P_0 : orientation of boresite ellipse
     *
     */

    // select the input pmFPAfile pointers
    psMetadataItem *item = psMetadataLookup (config->files, "PSASTRO.WCS");
    if (item == NULL) psAbort("missing PSASTRO.WCS entries in config->files");
    if (item->type != PS_DATA_METADATA_MULTI) psAbort("unexpected type for PSASTRO.WCS");
    psArray *files = psListToArray (item->data.list);

    // data storage vectors for measurements
    psVector *posZero  = psVectorAlloc (files->n, PS_TYPE_F32);
    psVector *Po       = psVectorAlloc (files->n, PS_TYPE_F32);
    psVector *Xo       = psVectorAlloc (files->n, PS_TYPE_F32);
    psVector *Yo       = psVectorAlloc (files->n, PS_TYPE_F32);

    // counter for accepted measured values
    int n = 0;

    // Re-select the output fpa.  The output fpa needs to have valid astrometry for the
    // refChip.  output->fpa is a copy of the pointer to one of the input->fpa, but the choice
    // is arbitrary.  select a new one that has an existing ref chip
    psFree (output->fpa);
    output->fpa = NULL;

    char filename[256];
    snprintf (filename, 256, "%s.bore", outroot);
    FILE *outfile = fopen (filename, "w");
    if (!outfile) {
        psError(PSASTRO_ERR_ARGUMENTS, true, "cannot open %s for output", filename);
	return false;
    }

    float posBoundary = 0.0;

    // extract the relevant measured and reported values from the reference chip
    for (int i = 0; i < files->n; i++) {
	psMetadataItem *file = files->data[i];
	pmFPAfile *input = file->data.V;

	// reported rotator position angle
	double POSANGLE = psMetadataLookupF64 (&status, input->fpa->concepts, "FPA.POSANGLE"); 
	if (!status) psAbort ("missing FPA.POSANGLE");

    	// get reference chip from name
	pmChip *chip = pmConceptsChipFromName (input->fpa, refChip);
	if (!chip) psAbort ("invalid chip name for reference");

	if (!chip->toFPA) continue;

	if (!output->fpa) {
	    // this one matches
	    output->fpa = psMemIncrRefCounter(input->fpa);
	}

	// we have two measurements of the posangle (may be parity flipped to different quadrants)
	// atan2 returns values in the range 0-2pi
	// all posZero values should be clustered in some region, but we need to flip over the 0,360 boundary correctly.
	// push all to one side or the other
	float chipAngle = PM_DEG_RAD * atan2 (chip->toFPA->y->coeff[1][0], chip->toFPA->x->coeff[1][0]);
	float fpaAngle = PM_DEG_RAD * atan2 (input->fpa->toTPA->y->coeff[1][0], input->fpa->toTPA->x->coeff[1][0]);

	posZero->data.F32[n] = POSANGLE - chipAngle - fpaAngle;
	if (n == 0) {
	    posBoundary = posZero->data.F32[n] + 180.0;
	} else {
	    while (posZero->data.F32[n] > posBoundary) posZero->data.F32[n] -= 360.0;
	    while (posZero->data.F32[n] < posBoundary - 360.0) posZero->data.F32[n] += 360.0;
	}

	Po->data.F32[n] = POSANGLE * PM_RAD_DEG; // reported position angle
	float xc = chip->fromFPA->x->coeff[0][0]; // reported boresite x position in ref chip coordinates
	float yc = chip->fromFPA->y->coeff[0][0]; // reported boresite y position in ref chip coordinates
	// XXX this can also be derived from toFPA via GetCenter....
	
	psPlane *PT = psPlaneTransformGetCenter (chip->toFPA, NONLIN_TOL);
	Xo->data.F32[n] = PT->x; // reported boresite x position in ref chip coordinates
	Yo->data.F32[n] = PT->y; // reported boresite y position in ref chip coordinates
	psFree (PT);

	fprintf (outfile, "%d : %f %f : %f = %f - %f - %f | %f %f\n", i, Xo->data.F32[n], Yo->data.F32[n], posZero->data.F32[n], POSANGLE, chipAngle, fpaAngle, xc, yc);
	n ++;
    }
      
    Xo->n = n;
    Yo->n = n;
    Po->n = n;
    posZero->n = n;

    psStats *stats = psStatsAlloc (PS_STAT_SAMPLE_MEDIAN | PS_STAT_SAMPLE_STDEV);
    if (!psVectorStats (stats, posZero, NULL, NULL, 0)) {
	psError(PS_ERR_UNKNOWN, false, "failure to measure stats");
	return false;
    }

    fprintf (outfile, "# pos zero %f +/- %f\n", stats->sampleMedian, stats->sampleStdev);
    psMetadataAddF32 (output->fpa->concepts, PS_LIST_TAIL, "FPA.POS_ZERO", PS_META_REPLACE, "offset between obs and meas posangle", stats->sampleMedian); 
    fclose (outfile);

    psVector *params = psastroModelFitBoresite (Xo, Yo, Po, outroot);
    if (params->n != 6) psAbort ("error");


    psMetadataAddF32 (output->fpa->concepts, PS_LIST_TAIL, "FPA.BORE.X0", PS_META_REPLACE, "boresite parameter", params->data.F32[PAR_X0]); 
    psMetadataAddF32 (output->fpa->concepts, PS_LIST_TAIL, "FPA.BORE.Y0", PS_META_REPLACE, "boresite parameter", params->data.F32[PAR_Y0]); 
    psMetadataAddF32 (output->fpa->concepts, PS_LIST_TAIL, "FPA.BORE.RX", PS_META_REPLACE, "boresite parameter", params->data.F32[PAR_RX]); 
    psMetadataAddF32 (output->fpa->concepts, PS_LIST_TAIL, "FPA.BORE.RY", PS_META_REPLACE, "boresite parameter", params->data.F32[PAR_RY]); 
    psMetadataAddF32 (output->fpa->concepts, PS_LIST_TAIL, "FPA.BORE.T0", PS_META_REPLACE, "boresite parameter", params->data.F32[PAR_T0]); 
    psMetadataAddF32 (output->fpa->concepts, PS_LIST_TAIL, "FPA.BORE.P0", PS_META_REPLACE, "boresite parameter", params->data.F32[PAR_P0]); 
    psMetadataAddStr (output->fpa->concepts, PS_LIST_TAIL, "FPA.REF.CHIP", PS_META_REPLACE, "boresite parameter", refChip);

    return true;
}
