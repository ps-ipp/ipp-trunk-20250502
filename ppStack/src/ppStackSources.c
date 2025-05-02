#include "ppStack.h"

//#define TESTING                         // Enable debugging output

//#define ASTROMETRY                    // Correct astrometry?

#ifdef TESTING
// Size of fake image; set by hand because it's trouble to get it from other places
#define FAKE_COLS 4861
#define FAKE_ROWS 4913
#endif

// #define TESTING_CZW

#ifdef TESTING_CZW
// Dump matches to a file
static void dumpMatches(const char *filename, // File to which to dump
                        int num,        // Number of inputs
                        psArray *matches, // Star matches
                        psVector *zp,   // Zero points
                        psVector *trans // Transparencies
                        )
{
    FILE *outMatches = fopen(filename, "w"); // Output matches
    psVector *mag = psVectorAlloc(num, PS_TYPE_F32); // Magnitudes for each star
    psVector *magErr = psVectorAlloc(num, PS_TYPE_F32); // Errors for each star
    for (int i = 0; i < matches->n; i++) {
        pmSourceMatch *match = matches->data[i]; // Match of interest
        psVectorInit(mag, NAN);
        psVectorInit(magErr, NAN);
        for (int j = 0; j < match->num; j++) {
	  if (match->mask) {
            if (match->mask->data.PS_TYPE_VECTOR_MASK_DATA[j]) {
	      continue;
            }
	  }
            int index = match->image->data.U32[j]; // Image index
            mag->data.F32[index] = match->mag->data.F32[j] - zp->data.F32[index];
            if (trans) {
                mag->data.F32[index] -= trans->data.F32[index];
            }
            magErr->data.F32[index] = match->magErr->data.F32[j];
        }
        for (int j = 0; j < num; j++) {
	  fprintf(outMatches, "%f %f %f %f %f %f\t",
		  match->mag->data.F32[j],
		  zp->data.F32[j],
		  trans ? trans->data.F32[j] : NAN,
		  magErr->data.F32[j],
		  match->x->data.F32[j],
		  match->y->data.F32[j]);
        }
        fprintf(outMatches, "\n");
    }
    psFree(mag);
    psFree(magErr);
    fclose(outMatches);
    return;
}
#endif


bool ppStackSourcesTransparency(ppStackOptions *options, const pmFPAview *view, const pmConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(options, false);
    PS_ASSERT_PTR_NON_NULL(view, false);
    PS_ASSERT_PTR_NON_NULL(config, false);

    if (!options->matchZPs && !options->photometry) {
        options->norm = psVectorAlloc(options->num, PS_TYPE_F32);
        psVectorInit(options->norm, 0.0);
        return true;
    }

    psArray *sourceLists = options->sourceLists; // Source lists for each input
    psVector *inputMask = options->inputMask; // Mask for inputs

    PS_ASSERT_ARRAY_NON_NULL(sourceLists, false);
    PS_ASSERT_VECTOR_NON_NULL(inputMask, false);
    PS_ASSERT_VECTOR_TYPE(inputMask, PS_TYPE_U8, false);
    PS_ASSERT_VECTOR_SIZE(inputMask, sourceLists->n, false);

#if defined(TESTING) && 1
    {
        // Deliberately induce a major transparency difference
        psArray *sources = sourceLists->data[0]; // Sources to correct
        for (int i = 0; i < sources->n; i++) {
            pmSource *source = sources->data[i]; // Source of interest
            if (!source) {
                continue;
            }
            source->psfMag += 1.0; // modified only for a test
#ifdef ASTROMETRY
            if (source->modelPSF) {
                source->modelPSF->params->data.F32[PM_PAR_XPOS] += 1.0;
                source->modelPSF->params->data.F32[PM_PAR_YPOS] += 1.0;
            }
            if (source->peak) {
                source->peak->xf += 1.0;
                source->peak->yf += 1.0;
            }
#endif
        }
    }
#endif

    psMetadata *recipe = psMetadataLookupMetadata(NULL, config->recipes, PPSTACK_RECIPE); // ppStack recipe
    psAssert(recipe, "We've thrown an error on this before.");

    float radius = psMetadataLookupF32(NULL, recipe, "ZP.RADIUS"); // Radius (pixels) for matching sources
    int iter1 = psMetadataLookupS32(NULL, recipe, "ZP.ITER.1"); // Maximum iterations for pass 1
    int iter2 = psMetadataLookupS32(NULL, recipe, "ZP.ITER.2"); // Maximum iterations for pass 2
    float tol = psMetadataLookupF32(NULL, recipe, "ZP.TOL"); // Tolerance for zero point iterations
    int transIter = psMetadataLookupS32(NULL, recipe, "ZP.TRANS.ITER"); // Iterations for transparency
    float transRej = psMetadataLookupF32(NULL, recipe, "ZP.TRANS.REJ");// Rejection threshold for transparency
    float transThresh = psMetadataLookupF32(NULL, recipe, "ZP.TRANS.THRESH"); // Threshold for transparency

    float starRej1 = psMetadataLookupF32(NULL, recipe, "ZP.STAR.REJ.1"); // Rejection threshold for stars
    float starSys1 = psMetadataLookupF32(NULL, recipe, "ZP.STAR.SYS.1"); // Estimated systematic error
    float starRej2 = psMetadataLookupF32(NULL, recipe, "ZP.STAR.REJ.2"); // Rejection threshold for stars
    float starSys2 = psMetadataLookupF32(NULL, recipe, "ZP.STAR.SYS.2"); // Estimated systematic error

    float starLimit = psMetadataLookupF32(NULL, recipe, "ZP.STAR.LIMIT"); // Limit on star rejection fraction

    float fracMatch = psMetadataLookupF32(NULL, recipe, "ZP.MATCH"); // Fraction of images to match for star

    bool mdok = false;
    float airmassTarget = psMetadataLookupF32(&mdok, recipe, "ZP.AIRMASS.TARGET"); // output airmass value 
    if (!mdok) {
	airmassTarget = 1.0;
    }

    psMetadata *airmassZP = psMetadataLookupMetadata(NULL, recipe, "ZP.AIRMASS"); // Airmass terms (slopes) by filter
    if (!airmassZP) {
        psError(PPSTACK_ERR_CONFIG, false, "Unable to find ZP.AIRMASS in recipe.");
        return false;
    }
    psMetadata *zpTargetMenu = psMetadataLookupMetadata(NULL, recipe, "ZP.TARGET"); // Target zero point terms
    if (!zpTargetMenu) {
        psError(PPSTACK_ERR_CONFIG, false, "Unable to find ZP.TARGET in recipe.");
        return false;
    }

    int num = options->num;             // Number of inputs
    psAssert(num == sourceLists->n, "Wrong number of source lists: %ld\n", sourceLists->n);

    // vectors to store these inputs so they may be recorded in the output headers
    options->zpInput      = psVectorAlloc(num, PS_TYPE_F32);
    options->expTimeInput = psVectorAlloc(num, PS_TYPE_F32);
    options->airmassInput = psVectorAlloc(num, PS_TYPE_F32);

    psVector *zp = psVectorAlloc(num, PS_TYPE_F32); // Relative zero points for each image
    psVector *zpExp = psVectorAlloc(num, PS_TYPE_F32); // Measured zero points for each image (maybe)
    int zpExpNum = 0;                                  // Number of measured zero points
    const char *filter = NULL;          // Filter name
    float airmassTerm = NAN;            // Airmass term
    float zpTarget = NAN;               // Target zero point
    int numGoodImages = 0;              // Number of good images
    for (int i = 0; i < num; i++) {
        psArray *sources = sourceLists->data[i]; // Source list
        if (!sources || sources->n == 0) {
            psLogMsg("ppStack", PS_LOG_WARN, "Image %d has no sources for transparency measurement.", i);
            options->inputMask->data.PS_TYPE_VECTOR_MASK_DATA[i] |= PPSTACK_MASK_CAL;
            zp->data.F32[i] = NAN;
            continue;
        }
        numGoodImages++;

        pmFPAfile *file = pmFPAfileSelectSingle(config->files, "PPSTACK.INPUT", i); // File of interest

#if defined(TESTING) && 0
        pmReadout *fake = pmReadoutAlloc(NULL); // Fake readout
        pmPSF *psf = psMetadataLookupPtr(NULL, config->arguments, "PSF.TARGET"); // PSF for fake image
        pmReadoutFakeFromSources(fake, FAKE_COLS, FAKE_ROWS, sourceLists->data[i], 0,
                                 NULL, NULL, psf, 5, 0, false, true);
        psString name = NULL;
        psStringAppend(&name, "start_%03d.fits", i);
        pmStackVisualPlotTestImage(fake->image, name);
        psFits *fits = psFitsOpen(name, "w");
        psFree(name);
        psFitsWriteImage(fits, NULL, fake->image, 0, NULL);
        psFitsClose(fits);
        psFree(fake);
#endif

        float exptime = options->exposures->data.F32[i]; // Exposure time
        float airmass = psMetadataLookupF32(NULL, file->fpa->concepts, "FPA.AIRMASS"); // Airmass
        const char *expFilter = psMetadataLookupStr(NULL, file->fpa->concepts, "FPA.FILTER"); // Filter name
        zpExp->data.F32[i] = psMetadataLookupF32(NULL, file->fpa->concepts, "FPA.ZP"); // Exposure zero point
	// XXX need to get the zero point error values and propagate to get the FPA.ZP.ERR value

	options->zpInput->data.F32[i] = zpExp->data.F32[i]; // NOTE zpExp may be re-assigned below using relative photometry
	options->expTimeInput->data.F32[i] = exptime;
	options->airmassInput->data.F32[i] = airmass;

        psLogMsg("ppStack", PS_LOG_INFO,
                 "Image %d: %.2f sec exposure in %s at airmass %.2f with zero point %.2f",
                 i, exptime, expFilter, airmass, zpExp->data.F32[i]);
        if (!isfinite(exptime) || exptime == 0 || !isfinite(airmass) || airmass == 0 ||
            !expFilter || strlen(expFilter) == 0) {
            psError(PPSTACK_ERR_CONFIG, false,
                    "Unable to find exposure time (%f), airmass (%f) or filter (%s)",
                    exptime, airmass, expFilter);
            psFree(zp);
            return false;
        }
        if (isfinite(zpExp->data.F32[i])) {
            zpExp->data.F32[i] += 2.5 * log10(exptime);
            zpExpNum++;
        }

        if (!filter) {
            filter = expFilter;
            airmassTerm = psMetadataLookupF32(&mdok, airmassZP, filter);
            if (!mdok || !isfinite(airmassTerm)) {
                psError(PPSTACK_ERR_CONFIG, false,
                        "Unable to find airmass term (ZP.AIRMASS) for filter %s", filter);
                psFree(zp);
                return false;
            }
            zpTarget = psMetadataLookupF32(&mdok, zpTargetMenu, filter);
            if (!mdok || !isfinite(zpTarget)) {
                psError(PPSTACK_ERR_CONFIG, false,
                        "Unable to find target zero point (ZP.TARGET) for filter %s", filter);
                psFree(zp);
                return false;
            }
        } else if (strcmp(filter, expFilter) != 0) {
	    psWarning("Filters don't match: %s vs %s", filter, expFilter);
        }

	// XXX this is wrong, or at least inconsistent with the above: this needs to include 
	// a value for the nominal system zero point to be consistent with zpExp
        zp->data.F32[i] = airmassTerm * airmass + 2.5 * log10(exptime);
    }

    if (numGoodImages == 0) {
        psLogMsg("ppStack", PS_LOG_WARN, "No images with sources to measure transparency.");
        options->quality = PPSTACK_ERR_REJECTED;
        psFree(zp);
        psFree(zpExp);
        return true;
    }
    if (numGoodImages == 1) {
        psArray *sources = NULL;        // Sources
        for (int i = 0; i < num && !sources; i++) {
            if (options->inputMask->data.PS_TYPE_VECTOR_MASK_DATA[i]) {
                continue;
            }
            sources = sourceLists->data[i];
        }
        options->quality = PPSTACK_ERR_REJECTED;
        options->sources = psMemIncrRefCounter(sources);
        options->norm = psVectorAlloc(num, PS_TYPE_F32);
        psVectorInit(options->norm, 1.0);
        options->zp = NAN;
        psLogMsg("ppStack", PS_LOG_WARN, "Single image with sources --- cannot match transparency.");
        psFree(zp);
        psFree(zpExp);
        return true;
    }

    if (zpExpNum == numGoodImages) {
	psLogMsg ("ppStack", PS_LOG_INFO, "all zero points are finite; using reported zero points listed above");
        for (int i = 0; i < num; i++) {
            zp->data.F32[i] = zpExp->data.F32[i];
        }
    } else {
	psLogMsg ("ppStack", PS_LOG_INFO, "missing some zero points; using guess values:");
        for (int i = 0; i < num; i++) {
	    psLogMsg("ppStack", PS_LOG_INFO, "Image %d: %.2f sec exposure with zero point %.2f", i, options->exposures->data.F32[i], zp->data.F32[i]);
        }
    }

    psArray *matches = pmSourceMatchSources(sourceLists, radius, true); // List of matches
    if (!matches) {
        psError(PPSTACK_ERR_DATA, false, "Unable to match sources");
        psFree(zp);
        return false;
    }
    options->sources = psMemIncrRefCounter(matches);
    psArray *matchedSources = psArrayAlloc(0);

#ifdef TESTING_CZW
    dumpMatches("source_match.dat", num, matches, zp, NULL);
#endif

    if (options->matchZPs) {

      psVector *trans = pmSourceMatchRelphot(matches, matchedSources, zp, tol, iter1, starRej1, starSys1,
                                               iter2, starRej2, starSys2, starLimit,
                                               transIter, transRej, transThresh); // Transparencies per image
        if (!trans) {
            psError(PPSTACK_ERR_DATA, false, "Unable to measure transparencies");
            return false;
        }
        for (int i = 0; i < trans->n; i++) {
            if (!isfinite(trans->data.F32[i])) {
                inputMask->data.U8[i] |= PPSTACK_MASK_CAL;
            }
        }

	// EAM : the discussion here was not quite right (or at least sloppy).  Here is a replacement explanation:

	// For any star, the observed instrumental magnitude on an image and the apparent magnitude are related by:
	// M_app = m_inst + zp + c1 * airmass + 2.5log(t) - transparency
	// NOTE the sign of 'transparency'  this must agree with the definition in pmSourceMatch.c. see, eg, line 457 where 
	// transparency = m_inst + zp + c1 * airmass + 2.5log(t) - M_app 

	// we want to adjust the input images to be in a consistent flux system so that the
	// final stack can be generated with a specific target zero point.  Any adjustment to
	// the flux scale of the image must be made in coordination with the resulting
	// zeropoint, exposure time, and airmass such that the above relationship yields the
	// same apparent magnitude for a given star:

	// m_inst_i : instrumental mags on input image (in)
	// m_inst_o : instrumental mags on re-normalized image (out)

	// m_inst_o + zp_o + c1 * airmass_o + 2.5log(t_o) - trans_o = m_inst_i + zp_i + c1 * airmass_i + 2.5log(t_i) - trans_i

	// m_inst_o = m_inst_i + (zp_i - zp_o) + c1 * (airmass_i - airmass_o) + 2.5log(t_i) - 2.5log(t_o) - trans_i + trans_o

	// zp_i, airmass_i, t_i, trans_i : reported or measured for input image

	// zp_o      = zpTarget      (from recipe)
	// airmass_o = airmassTarget (from recipe)
	// t_o       = sumExpTime    [sum of input exposure times: once images are scale to this time, they can be avereaged]
	// trans_o   = 0.0           [obviously!]

	// we have 2 cases: (a) all reported ZPs are good or (b) some are bad:
	// (a) FPA.ZP = zp_i + c1 * airmass_i
	//  --> zp[i] = zp_i + c1 * airmass_i + 2.5log(exptime_i)
	// (b)  zp[i] = c1 * airmass_i + 2.5log(exptime_i)
	// NOTE: in case (b), the current code is equating the TARGET zp with the NOMINAL zp, which is wrong.

	// m_inst_o - m_inst_i = zp[i] - zpTarget - c1 * airmassTarget - 2.5log(sumExpTime) - trans_i
#ifdef TESTING_CZW
    dumpMatches("source_match2.dat", num, matches, zp, trans);
#endif
        if (options->matchZPs) {
            options->norm = psVectorAlloc(num, PS_TYPE_F32);
            for (int i = 0; i < num; i++) {
		if (!isfinite(trans->data.F32[i])) {
		    psLogMsg("ppStack", PS_LOG_INFO, "Non-finite transparency, skipping correction for image %d: %f mag (%f) (with inputMask value %d)\n",
			     i, NAN, trans->data.F32[i], inputMask->data.U8[i]);
		    continue;
		}
                psArray *sources = sourceLists->data[i]; // Sources of interest
                float magCorr = zp->data.F32[i] - trans->data.F32[i] - 2.5*log10(options->sumExposure) - airmassTerm * airmassTarget;
                if (zpExpNum == numGoodImages) { // case (a)
                    // Using measured zero points, so attempt to set target zero point
		    // XXX see NOTE above regarding case (b) : this is wrong.  the code should load a nominal zero point and supply it above
		    // 
                    magCorr -= zpTarget;
                }
                options->norm->data.F32[i] = magCorr;
                psLogMsg("ppStack", PS_LOG_INFO,
                         "Applying scale correction to image %d: %f mag (%f) (with inputMask value %d)\n",
                         i, magCorr, trans->data.F32[i],inputMask->data.U8[i]);

                for (int j = 0; j < sources->n; j++) {
                    pmSource *source = sources->data[j]; // Source of interest
		    source->psfMag += magCorr;
		    source->apMag += magCorr;

                    if (!source) {
                        continue;
                    }
		    // XXX need to apply to apMag as well
		    psTrace("ppStack",5,"Source corrections: %d %d %f %f %f %f %f %f %f %f\n",i,j,
			    source->peak->xf,source->peak->yf,
			    source->psfMag,source->psfMagErr,
			    airmassTerm,airmassTarget,2.5 * log10(options->sumExposure),
			    source->psfMag + magCorr);
                }
            }
        }
	matches = pmSourceMatchSources(sourceLists, radius, true); // List of matches

        if (zpExpNum == numGoodImages) {
            // Producing image with target zero point
            options->zp = zpTarget;
            options->airmass = airmassTarget;
            options->airmassSlope = airmassTerm;
        } else {
            options->zp = NAN;
        }


        psFree(trans);

#ifdef TESTING_CZW
        // Double check: all transparencies should be zero
        {
            psArray *matches = pmSourceMatchSources(sourceLists, radius, true); // List of matches
	    psArray *matchedSources = psArrayAlloc(matches->n);
            if (!matches) {
                psError(PPSTACK_ERR_DATA, false, "Unable to match sources");
                psFree(zp);
                return false;
            }
            psVector *trans = pmSourceMatchRelphot(matches, matchedSources, zp, tol, iter1, starRej1, starSys1,
                                                   iter2, starRej2, starSys2, starLimit,
                                                   transIter, transRej, transThresh); // Transparencies
            for (int i = 0; i < num; i++) {
                fprintf(stderr, "Transparency of image %d: %f\n", i, trans->data.F32[i]);
            }

            psFree(trans);
            psFree(matches);
	    psFree(matchedSources);
        }
#endif
    }


    psFree(zpExp);

#ifdef ASTROMETRY
    // CZW: This is off by default. 
    // Position offsets
    {
        psArray *offsets = pmSourceMatchRelastro(matches, num, tol, iter1, starRej1,
                                                  iter2, starRej2, starLimit); // Shifts for each image
        if (!offsets) {
            psError(PPSTACK_ERR_DATA, false, "Unable to measure offsets");
            return false;
        }
        for (int i = 0; i < num; i++) {
            if (options->inputMask->data.U8[i]) {
                continue;
            }
            psArray *sources = sourceLists->data[i]; // Sources of interest
            psVector *offset = offsets->data[i];                      // Offsets for image
            float dx = offset->data.F32[0], dy = offset->data.F32[1]; // Offsets to apply
            if (!isfinite(dx) || !isfinite(dy)) {
                continue;
            }
            psLogMsg("ppStack", PS_LOG_INFO, "Applying astrometric correction to image %d: %f,%f\n",
                     i, dx, dy);
            for (int j = 0; j < sources->n; j++) {
                pmSource *source = sources->data[j]; // Source of interest
                if (!source) {
                    continue;
                }
                if (source->modelPSF) {
                    source->modelPSF->params->data.F32[PM_PAR_XPOS] -= dx;
                    source->modelPSF->params->data.F32[PM_PAR_YPOS] -= dy;
                }
                if (source->peak) {
                    source->peak->xf -= dx;
                    source->peak->yf -= dy;
                }
            }
        }
        psFree(offsets);
    }
#endif

#if (defined TESTING && defined ASTROMETRY)
    // CZW: This is off by default.
        // Double check: all offsets should be zero
        {
            psArray *matches = pmSourceMatchSources(sourceLists, radius, true); // List of matches
            if (!matches) {
                psError(PPSTACK_ERR_DATA, false, "Unable to match sources");
                psFree(zp);
                return false;
            }
            psArray *offsets = pmSourceMatchRelastro(matches, num, tol, iter1, starRej1,
                                                     iter2, starRej2, starLimit); // Shifts for each image
            for (int i = 0; i < num; i++) {
                psVector *offset = offsets->data[i]; // Offsets for image
                fprintf(stderr, "Offset of image %d: %f,%f\n", i, offset->data.F32[0], offset->data.F32[1]);
            }
            psFree(offsets);
            psFree(matches);
        }
#endif


    if (options->photometry) {
        // Save best matches for future photometry
      psArray *sourcesBest = options->sources = psArrayAllocEmpty(matches->n);
        // XXX something of a hack: require at least 2 detections or the nominated fraction of the max possible
        //int minMatches = PS_MAX(2, fracMatch * num);// Minimum number of matches required
        //MEH - if many poor images, will hit oddity with num fraction. use numGoodImages instead
      int minMatches = PS_MAX(2, fracMatch * numGoodImages);// Minimum number of matches required
      
      int good0 = 0;
      for (int i = 0; i < num; i++) {
	if (inputMask->data.U8[i]) {
	  continue;
	}
	good0 = i;
	break;
      }
      
      for (int i = 0; i < matches->n; i++) {
	pmSourceMatch *match = matches->data[i]; // Match of interest
	 if (match->num < minMatches) {
	   continue;
	 }

	 // We need to grab a single instance of this source: just take the first available
	 // MEH - if first available is a rejected image, then problem. so take first non-rejected instance above
	 //	 int image = match->image->data.S32[0]; // Index of image
	 // int index = match->index->data.S32[0]; // Index of source within image
	 int image = match->image->data.S32[good0]; // Index of image
	 int index = match->index->data.S32[good0]; // Index of source within image
	 psArray *sources = sourceLists->data[image]; // Sources for image
	 pmSource *source = sources->data[index]; // Source of interest

	 // CZW: Accept the first source for all properties, except for the magnitudes.
	 //      We have average magnitudes from relphot, so we should use those
	 pmSource *photsource = matchedSources->data[i];
	 if ((photsource)&&(isfinite(photsource->psfMag))) {
	   // CZW: photsource is the average relphot mag, which is equal to <m_i + zpIN_j>
	   //      (primed are averaged values for this discussion)
	   //      Therefore, the correct magnitude for psfMag' is:
	   //         psfMag' = (photsource - zpIN_j + magCorr)
	   //      apMag - psfMag should be retained, so:
	   //         apMag - psfMag = apMag' - psfMag'
	   //         apMag' = apMag + psfMag' - psfMag
	   source->apMag  = source->apMag + (photsource->psfMag + options->norm->data.F32[image] - zp->data.F32[image] - source->psfMag);
	   source->psfMag = photsource->psfMag + options->norm->data.F32[image] - zp->data.F32[image];
	   
	   psTrace("ppStack", 5, "Best Source %d/%ld @ %f %f %f %f %f %d",i,sourcesBest->n,source->peak->xf,source->peak->yf,
		   source->psfMag,
		   photsource->psfMag + options->norm->data.F32[image] - zp->data.F32[image] ,
		   source->psfMagErr,-1);
	   psArrayAdd(sourcesBest, sourcesBest->n, source);
	 }
        }
      psLogMsg("ppStack", PS_LOG_INFO, "Selected %ld sources for photometry analysis", sourcesBest->n);

    }
    psFree(zp);
    psFree(matches);

    return true;
}
