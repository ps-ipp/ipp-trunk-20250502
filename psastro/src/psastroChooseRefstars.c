/** @file psastroChooseRefstars.c
 *
 *  @brief 
 *
 *  @ingroup libpsastro
 *
 *  @author IfA
 *  @version $Revision: 1.21 $
 *  @date $Date: 2009-02-07 02:03:34 $
 *  Copyright 2009 Institute for Astronomy, University of Hawaii
 */

# include "psastroInternal.h"

# define ESCAPE { \
  psError(PS_ERR_UNKNOWN, false, "Failure in psastroChooseRefstars"); \
  psFree (index); \
  psFree (view); \
  return false; \
}

bool psastroChooseRefstars (pmConfig *config, psArray *refs, const char *source, bool saveExistingMatchedRefs) {

    bool status;
    pmChip *chip = NULL;
    pmCell *cell = NULL;
    pmReadout *readout = NULL;

    // select the current recipe
    psMetadata *recipe  = psMetadataLookupPtr (NULL, config->recipes, PSASTRO_RECIPE);
    if (!recipe) {
        psError(PSASTRO_ERR_CONFIG, true, "Can't find PSASTRO recipe!\n");
        return false;
    }

    // select the input data sources
    pmFPAfile *input = psMetadataLookupPtr (NULL, config->files, source);
    if (!input) {
        psError(PSASTRO_ERR_CONFIG, true, "Can't find input data!\n");
        return false;
    }

    // extra field fraction to add
    double fieldPadding = psMetadataLookupF32 (&status, recipe, "PSASTRO.FIELD.PADDING");
    if (!status) fieldPadding = 0.0;

    bool matchLumFunc = psMetadataLookupBool (&status, recipe, "PSASTRO.MATCH.LUMFUNC");

    int nIter = psMetadataLookupS32 (&status, recipe, "PSASTRO.REFSTAR.CLUMP.NITER");
    if (!status) nIter = 3;

    psF32 clumpScale = psMetadataLookupS32 (&status, recipe, "PSASTRO.REFSTAR.CLUMP.SCALE");
    if (!status) clumpScale = 150;

    pmFPAview *view = pmFPAviewAlloc (0);
    pmFPA *fpa = input->fpa;

    // XXX kind of a hack -- think this through a bit more clearly:
    if (psMetadataLookupPtr (&status, fpa->analysis, "MATCHED_REFS")) {
	// we loaded a set of matched references from an earlier astrometry
	// analysis.  however, we are re-doing the astrometry here, so remove
	// that prior set of matched references
        if (!saveExistingMatchedRefs) {
            psMetadataRemoveKey (fpa->analysis, "MATCHED_REFS");
        }
    }

    // sort by mag
    psVector *index = psArraySortIndex (NULL, refs, psastroSortByMag);

    int nMax = psMetadataLookupS32 (&status, recipe, "PSASTRO.MAX.NREF");
    psF32 clampMagMin = psMetadataLookupF32 (&status, recipe, "REFSTAR_CLAMP_MAG_MIN");
    if (!status) clampMagMin = -5.0;
    //MEH adding max option as well -- really should not be hardcoded..
    psF32 clampMagMax = psMetadataLookupF32 (&status, recipe, "REFSTAR_CLAMP_MAG_MAX");
    if (!status) clampMagMax = 30.0;
    psWarning("Will skip/clamp refstar with brighter/fainter magnitude %f/%f\n",clampMagMin,clampMagMax); 


    // de-activate all files except PSASTRO.REFSTARS
    pmFPAfileActivate (config->files, false, NULL);
    pmFPAfileActivate (config->files, true, "PSASTRO.OUT.REFSTARS");

    // this loop selects the matched stars for all chips
    while ((chip = pmFPAviewNextChip (view, fpa, 1)) != NULL) {
        psTrace ("psastro", 4, "Chip %d: %x %x\n", view->chip, chip->file_exists, chip->process);
        if (!chip->process || !chip->file_exists) { continue; }
	if (!chip->fromFPA) { continue; }

        while ((cell = pmFPAviewNextCell (view, fpa, 1)) != NULL) {
            psTrace ("psastro", 4, "Cell %d: %x %x\n", view->cell, cell->file_exists, cell->process);
            if (!cell->process || !cell->file_exists) { continue; }

            // process each of the readouts
            // XXX there can only be one readout per chip in astrometry, right?
            while ((readout = pmFPAviewNextReadout (view, fpa, 1)) != NULL) {
                if (! readout->data_exists) { continue; }

		psRegion *extent = pmReadoutExtent (readout);
		if (!extent) {
		    psError(PSASTRO_ERR_CONFIG, true, "Can't find readout size!\n");
		    return NULL;
		}

		int Nx = abs(extent->x1 - extent->x0);
		int Ny = abs(extent->y1 - extent->y0);

                float minX = -fieldPadding*Nx;
                float maxX = (1+fieldPadding)*Nx;
                float minY = -fieldPadding*Ny;
                float maxY = (1+fieldPadding)*Ny;

                // the refstars is a subset within range of this chip
                psArray *refstars = psArrayAllocEmpty (100);

                // select the reference objects within range of this readout
                // project the reference objects to this chip
                for (int i = 0; i < refs->n; i++) {
                    pmAstromObj *ref = pmAstromObjCopy(refs->data[index->data.S32[i]]);

                    if (ref->Mag < clampMagMin) {
                        //psWarning("Skipping refstar with abusrd magnitude %f ra: %f dec: %f\n",
                        //    ref->Mag, RAD_TO_DEG(ref->sky->r), RAD_TO_DEG(ref->sky->d));
                        psTrace ("psastro", 6, "Skipping by clamp refstar with bright magnitude %f ra: %f dec: %f\n",
			         ref->Mag, RAD_TO_DEG(ref->sky->r), RAD_TO_DEG(ref->sky->d));
                        goto skip;
                    }
		    //MEH option for max mag clamp also -- need to change psWarning (to log) to trace and above for bright..
                    if (ref->Mag > clampMagMax) {
                        //psWarning("Skipping by clamp refstar with faint magnitude %f ra: %f dec: %f\n",
                        //    ref->Mag, RAD_TO_DEG(ref->sky->r), RAD_TO_DEG(ref->sky->d));
			psTrace ("psastro", 6, "Skipping by clamp refstar with faint magnitude %f ra: %f dec: %f\n",
			         ref->Mag, RAD_TO_DEG(ref->sky->r), RAD_TO_DEG(ref->sky->d));
                        goto skip;
                    }

                    psProject (ref->TP, ref->sky, fpa->toSky);
                    psPlaneTransformApply (ref->FP, fpa->fromTPA, ref->TP);
                    psPlaneTransformApply (ref->chip, chip->fromFPA, ref->FP);

                    // limit the X,Y range of the refs to the selected chip
                    if (ref->chip->x < minX) goto skip;
                    if (ref->chip->x > maxX) goto skip;
                    if (ref->chip->y < minY) goto skip;
                    if (ref->chip->y > maxY) goto skip;

                    psArrayAdd (refstars, 100, ref);
                skip:
                    psFree (ref);

		    if (nMax && (refstars->n >= nMax)) break;
                }
                psTrace ("psastro", 4, "Added %ld refstars\n", refstars->n);

# if (0)
		if (1) {
		  // test output block, not used in normal ops
		  char filename[64];
		  snprintf (filename, 64, "refstars.%02d.dat", Nchip);
		  FILE *outfile = fopen (filename, "w");
		  assert (outfile);
		  for (int nn = 0; nn < refstars->n; nn++) {
		    pmAstromObj *ref = refstars->data[nn];
		    fprintf (outfile, "%lf %lf\n", ref->sky->r*PS_DEG_RAD, ref->sky->d*PS_DEG_RAD);
		  }
		  fclose (outfile);
		}
		if (1) {
		  // test output block, not used in normal ops
		  char filename[64];
		  snprintf (filename, 64, "refstars.%02d.dat", Nchip);
		  FILE *outfile = fopen (filename, "w");
		  assert (outfile);
		  for (int nn = 0; nn < refstars->n; nn++) {
		    pmAstromObj *ref = refstars->data[nn];
		    fprintf (outfile, "%lf %lf  %lf %lf  %lf %lf\n", ref->sky->r*PS_DEG_RAD, ref->sky->d*PS_DEG_RAD, );
		  }
		  fclose (outfile);
		}
# endif

		psMetadataAdd (readout->analysis, PS_LIST_TAIL, "PSASTRO.REFSTARS", PS_DATA_ARRAY, "astrometry matches", refstars);

		// generate a reduced subset excluding the clumps
		// XXX do we need both REFSTARS and SUBSET? 
		psArray *subset = psastroRemoveClumpsIterate(refstars, clumpScale, nIter);
		psMetadataAdd (readout->analysis, PS_LIST_TAIL, "PSASTRO.REFSTARS.SUBSET", PS_DATA_ARRAY, "astrometry objects", subset);

		psFree (refstars);
		psFree (subset);
		psFree (extent);

		if (matchLumFunc) {
		    // limit the total magnitude range of PSASTRO.REFSTARS.SUBSET based on
		    // overlapping luminosity functions
		    if (!psastroRefstarSubset (readout)) {
			psError(PSASTRO_ERR_DATA, false, "Can't determine an appropriate refstar subset\n");
			psFree (index);
			psFree (view);
			return false;
		    }
		}
            }
        }
	if (!pmFPAfileIOChecks (config, view, PM_FPA_AFTER)) ESCAPE;
    }
    if (!pmFPAfileIOChecks (config, view, PM_FPA_AFTER)) ESCAPE;

    // activate all files except PSASTRO.OUTPUT
    pmFPAfileActivate (config->files, true, NULL);
    pmFPAfileActivate (config->files, false, "PSASTRO.OUT.REFSTARS");

    bool onlyRefstars = psMetadataLookupBool (&status, recipe, "PSASTRO.ONLY.REFSTARS");
    if (onlyRefstars) exit (0);

    psFree (index);
    psFree (view);
    return true;
}
