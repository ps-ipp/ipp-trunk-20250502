#define DIRTY_BUG_FIX 0

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <pslib.h>

#include "ppMops.h"

static void addDummyValues(psMetadata* md, long size, int version);
static void replaceDummyValuesF32(const char* colName, psMetadata* source, psMetadata* target, psVector* indexes);
static void replaceDummyValuesS32(const char* colName, psMetadata* source, psMetadata* target, psVector* indexes);
static void renameDummyValuesF32(const char* colNameSource, psMetadata* source, const char* colNameTarget, psMetadata* target, psVector* indexes);

/*
  ppMopsRead possibly modifies the args->version if the user did not
  set it explicitely.
*/
psArray *ppMopsRead(ppMopsArguments *args)
{
  psTrace("ppMops.read", 1, "Reading input detections\n");

  psArray *inNames = args->input;          // Input names
  long num = inNames->n;                   // Number of inputs
  psArray *detections = psArrayAlloc(num); // Array of detections, to return
  for (int i = 0; i < num; i++) {
    const char *name = inNames->data[i];
    printf("Input filename:\n%s\n", name);

    psFits *fits = psFitsOpen(name,  "r"); // FITS file
    if (!fits) {
      psError(PS_ERR_IO, false, "Unable to open input %d", i);
      return false;
    }

    psMetadata *header = psFitsReadHeader(NULL, fits); // Primary header
    if (!header) {
      psError(PS_ERR_IO, false, "Unable to read header %d", i);
      return false;
    }

    psS64 diffSkyfileId = psMetadataLookupS64(NULL, header, "IMAGEID"); // Identifier for image
    if (diffSkyfileId == 0) {
      psError(PS_ERR_UNEXPECTED_NULL, false, "Unable to find identifier for image %d", i);
      return false;
    }

    if (!psFitsMoveExtName(fits, "SkyChip.psf")) {
      psError(PS_ERR_IO, false, "Unable to move to HDU with detections");
      return false;
    }
    int skyChipPsfVersion = ppMopsGetSkyChipPsfVersion(fits);
    if (args->version == 0) {
      psTrace("ppMops.read", 1, "Changing args->version to %d\n", skyChipPsfVersion);
      args->version = (unsigned short) skyChipPsfVersion;
    }
    if (skyChipPsfVersion == 0) {
      // Try to read with the user specified version?
      skyChipPsfVersion = args->version;
    }
    /* Display a warning message if there are version
       inconsistencies between the file and the flag (note that
       those inconsistencies might be wanted) */
    if (skyChipPsfVersion != args->version) {
      if (skyChipPsfVersion > args->version) {
        psWarning("The FITS data will be downgraded from PS1_DV%d to PS1_DV%d\n",
                  skyChipPsfVersion, args->version);
      } else { // Necessarily: skyChipPsfVersion > args->version
        psWarning("The FITS data will be upgraded from PS1_DV%d to PS1_DV%d (new values set to default 0, NaN...)\n",
                  skyChipPsfVersion, args->version);
      }
    }

    long size = psFitsTableSize(fits); // Size of table
    if (size <= 0) {
      psErrorStackPrint(stderr, "Unable to determine size of table %d", i);
      psErrorClear();
      psWarning("Ignoring input %d", i);
      psFree(header);
      psFitsClose(fits);
      continue;
    }
    ppMopsDetections *det = ppMopsDetectionsAlloc();
    det->platescale = NAN;
    detections->data[i] = det;
    det->component = psStringNCopy(name, strrchr(name, '.') - name); // Strip off extension
    det->num = size;
    det->diffSkyfileId = diffSkyfileId;

    psTrace("ppMops.read", 3, "Reading %ld rows from %s\n", size, (const char*)inNames->data[i]);

    det->raBoresight = psMemIncrRefCounter(psMetadataLookupStr(NULL, header, "FPA.RA"));
    det->decBoresight = psMemIncrRefCounter(psMetadataLookupStr(NULL, header, "FPA.DEC"));
    det->filter = psMemIncrRefCounter(psMetadataLookupStr(NULL, header, "FPA.FILTER"));
    det->airmass = psMetadataLookupF32(NULL, header, "AIRMASS");
    det->exptime = psMetadataLookupF32(NULL, header, "EXPTIME");
    det->posangle = psMetadataLookupF64(NULL, header, "FPA.POSANGLE");
    det->alt = psMetadataLookupF64(NULL, header, "FPA.ALT");
    det->az = psMetadataLookupF64(NULL, header, "FPA.AZ");
    det->mjd = psMetadataLookupF64(NULL, header, "MJD-OBS") + det->exptime / 2.0 / 3600 / 24;
    //The global SEEING is the mean of all defined FWHM_MAJ
    det->seeing = psMetadataLookupF32(NULL, header, "FWHM_MAJ");
    det->naxis1 = psMetadataLookupS32(NULL, header, "IMNAXIS1"); // Number of columns
    det->naxis2 = psMetadataLookupS32(NULL, header, "IMNAXIS2"); // Number of rows
    det->fpashutoutc = psMemIncrRefCounter(psMetadataLookupStr(NULL, header, "FPA.SHUTOUTC"));
    det->fpashutcutc = psMemIncrRefCounter(psMetadataLookupStr(NULL, header, "FPA.SHUTCUTC"));
    det->fpashmdoutc = psMemIncrRefCounter(psMetadataLookupStr(NULL, header, "FPA.SHMDOUTC"));
    det->fpashmdcutc = psMemIncrRefCounter(psMetadataLookupStr(NULL, header, "FPA.SHMDCUTC"));
    det->refcat = psMemIncrRefCounter(psMetadataLookupStr(NULL, header, "PSREFCAT"));
    psFree(header);

    psMetadata *table = psFitsReadTableAllColumns(fits); // Table of interest
    if (!table) {
      psError(PS_ERR_IO, false, "Unable to read table %d", i);
      return NULL;
    }
    det->table = table;
    psTrace("ppMops.read", 19, "PSF table:\n%s\n", psMetadataConfigFormat(det->table));
    psTrace("ppMops.read", 19, "End of PSF table\n");

    //fittedTrail extension (Supported for releases 2 and above)
    if (skyChipPsfVersion >= 2) {
      //First: append all the new columns that we want to the existing table
      addDummyValues(det->table, size, args->version);
      if (!psFitsMoveExtName(fits, "SkyChip.xfit")) {
        psTrace("ppMops.read", 3, "No fitted trails extension");
      } else {
        psTrace("ppMops.read", 3, "Fitted trails extension found\n");
        psTrace("ppMops.read", 10, "Getting size?\n");
        int fittedTrailsSize = psFitsTableSize(fits);
        psTrace("ppMops.read", 10, "size = %d\n", fittedTrailsSize);
        if (fittedTrailsSize <= 0) {
          psErrorStackPrint(stderr, "Unable to determine size of fitted trails extension table %d", i);
          psTrace("ppMops.read", 3, "No entry in fitted trails extension!!!!\n");
          psErrorClear();
        } else {
          psTrace("ppMops.read", 10, "Reading table of interest?\n");
          psMetadata* fittedTrails = psFitsReadTableAllColumns(fits); // Table of interest
          psTrace("ppMops.read", 10, "OK for table of interest\n");
          if (!fittedTrails) {
            psError(PS_ERR_IO, false, "Unable to read fittedTrails table in file %d", i);
            return NULL;
          }
          //Iterate on the different names and types expected in the fittedTrails parameters
          psTrace("ppMops.read", 10, "Getting IPP_IDET\n");
          psVector* idet = psMetadataLookupVector(NULL, fittedTrails, "IPP_IDET");
          replaceDummyValuesF32("X_EXT", fittedTrails, det->table, idet);
          replaceDummyValuesF32("Y_EXT", fittedTrails, det->table, idet);
          replaceDummyValuesF32("X_EXT_SIG", fittedTrails, det->table, idet);
          replaceDummyValuesF32("Y_EXT_SIG", fittedTrails, det->table, idet);
          replaceDummyValuesF32("EXT_INST_MAG",  fittedTrails, det->table, idet);
          replaceDummyValuesF32("EXT_INST_MAG_SIG",  fittedTrails, det->table, idet);
          replaceDummyValuesS32("NPARAMS",  fittedTrails, det->table, idet);
          replaceDummyValuesF32("EXT_WIDTH_MAJ",  fittedTrails, det->table, idet);
          replaceDummyValuesF32("EXT_WIDTH_MIN",  fittedTrails, det->table, idet);
          replaceDummyValuesF32("EXT_THETA",  fittedTrails, det->table, idet);
          replaceDummyValuesF32("EXT_WIDTH_MAJ_ERR",  fittedTrails, det->table, idet);
          // EXT_WIDTH_MIN_ERR is actually undefined but set to 0. in CMF files
          // We explicitely let it set to NaN, hence the commented out
          // following line
          //replaceDummyValuesF32("EXT_WIDTH_MIN_ERR",  fittedTrails, det->table, idet);
          replaceDummyValuesF32("EXT_THETA_ERR",  fittedTrails, det->table, idet);
          psTrace("ppMops.read", 10, "Got all version 2 values\n");
          if (skyChipPsfVersion >= 3) {
            psTrace("ppMops.read", 10, "Getting PS1_DV3 data\n");
            replaceDummyValuesF32("RA_EXT",  fittedTrails, det->table, idet);
            // Note: RA_EXT_SIGMA is computed (see det->raExtErr)
            replaceDummyValuesF32("DEC_EXT",  fittedTrails, det->table, idet);
            // Note: DEC_EXT_SIGMA is computed (see det->decExtErr)
            //Note: POSANG_EXT is POSANGLE in xfit
            renameDummyValuesF32("POSANGLE",  fittedTrails, "POSANG_EXT", det->table, idet);
            //Not written anymore but still used in computation
            renameDummyValuesF32("PLTSCALE", fittedTrails, "PLTSCALE_EXT", det->table, idet);
            renameDummyValuesF32("EXT_INST_FLUX", fittedTrails, "EXT_FLUX", det->table, idet);
            replaceDummyValuesF32("EXT_CAL_MAG", fittedTrails, det->table, idet);
            renameDummyValuesF32("EXT_INST_MAG_SIG", fittedTrails, "EXT_MAG_SIG", det->table, idet);
            replaceDummyValuesF32("EXT_CHISQ", fittedTrails, det->table, idet);
            replaceDummyValuesS32("EXT_NDOF", fittedTrails, det->table, idet);
            psTrace("ppMops.read", 10, "Got all version 3 values\n");
          }
        }
      }
    }
    psFitsClose(fits);

    if (args->version == 0) {
      if (skyChipPsfVersion < 2) {
        // XXX: TODO: Do we need to add dummy vectors for the missing columns?
      }
    }

    psVector *ra = psMetadataLookupVector(NULL, table, "RA_PSF");
    psVector *dec = psMetadataLookupVector(NULL, table, "DEC_PSF");
    det->raErr = psVectorAlloc(size, PS_TYPE_F64);
    det->decErr = psVectorAlloc(size, PS_TYPE_F64);
    if (skyChipPsfVersion >= 3) {
      det->raExtErr = psVectorAlloc(size, PS_TYPE_F64);
      det->decExtErr = psVectorAlloc(size, PS_TYPE_F64);
    } else {
      det->raExtErr = NULL;
      det->decExtErr = NULL;
    }
    det->mask = psVectorAlloc(size, PS_TYPE_U8);

    // convert ra and dec to radians for use in the purge duplicates function
    det->ra = (psVector*)psBinaryOp(NULL, ra, "*", psScalarAlloc(DEG_TO_RAD(1.0), PS_TYPE_F64));
    det->dec = (psVector*)psBinaryOp(NULL, dec, "*", psScalarAlloc(DEG_TO_RAD(1.0), PS_TYPE_F64));
    det->x = psMemIncrRefCounter(psMetadataLookupVector(NULL, table, "X_PSF"));
    det->y = psMemIncrRefCounter(psMetadataLookupVector(NULL, table, "Y_PSF"));
    if (!det->ra || !det->dec || !det->x || !det->y) {
      psError(PS_ERR_UNEXPECTED_NULL, true, "Unable to find all of RA, Dec, X and Y columns");
      return NULL;
    }

    // Add our new vectors to the table so that duplicates and masked items may be purged
    psMetadataAddVector(table, PS_LIST_HEAD, "RA_ERR", 0, NULL, det->raErr);
    psMetadataAddVector(table, PS_LIST_HEAD, "DEC_ERR", 0, NULL, det->decErr);
    if (skyChipPsfVersion >= 3) {
      psMetadataAddVector(table, PS_LIST_HEAD, "RA_EXT_SIGMA", 0, NULL, det->raExtErr);
      psMetadataAddVector(table, PS_LIST_HEAD, "DEC_EXT_SIGMA", 0, NULL, det->decExtErr);
    }

    psTrace("ppMops.read", 2, "Read %ld rows from %s\n", det->num, det->component);

    psVector *mag    = psMetadataLookupVector(NULL, table, "PSF_INST_MAG");
    psVector *magErr = psMetadataLookupVector(NULL, table, "PSF_INST_MAG_SIG");
    psVector *xErrV  = psMetadataLookupVector(NULL, table, "X_PSF_SIG");
    psVector *yErrV  = psMetadataLookupVector(NULL, table, "Y_PSF_SIG");
    psVector *scaleV = psMetadataLookupVector(NULL, table, "PLTSCALE");
    psVector *angleV = psMetadataLookupVector(NULL, table, "POSANGLE");
    psVector *flagsV = psMetadataLookupVector(NULL, table, "FLAGS");
    psVector *flags2V = psMetadataLookupVector(NULL, table, "FLAGS2");
    psVector *xExtErrV = NULL;
    psVector *yExtErrV = NULL;
    psVector *scaleExtV = NULL;
    psVector *angleExtV = NULL;
    if (skyChipPsfVersion >= 3) {
      xExtErrV = psMetadataLookupVector(NULL, table, "X_EXT_SIG");
      yExtErrV = psMetadataLookupVector(NULL, table, "Y_EXT_SIG");
      scaleExtV = psMetadataLookupVector(NULL, table, "PLTSCALE_EXT");
      angleExtV = psMetadataLookupVector(NULL, table, "POSANG_EXT");
    }

    //MEH -- diff params, may need skyChipPsfVersion 
    // -- unclear for npos, remove for now
    //psVector *npos = psMetadataLookupVector(NULL, table, "DIFF_NPOS");
    psVector *fpos = psMetadataLookupVector(NULL, table, "DIFF_FRATIO");
    psVector *rbad = psMetadataLookupVector(NULL, table, "DIFF_NRATIO_BAD");
    psVector *rmask = psMetadataLookupVector(NULL, table, "DIFF_NRATIO_MASK");
    psVector *rall = psMetadataLookupVector(NULL, table, "DIFF_NRATIO_ALL");

    psVector *rp = psMetadataLookupVector(NULL, table, "DIFF_R_P");
    psVector *rm = psMetadataLookupVector(NULL, table, "DIFF_R_M");
    psVector *snp = psMetadataLookupVector(NULL, table, "DIFF_SN_P");
    psVector *snm = psMetadataLookupVector(NULL, table, "DIFF_SN_M");

    psVector *mxx = psMetadataLookupVector(NULL, table, "MOMENTS_XX");
    psVector *myy = psMetadataLookupVector(NULL, table, "MOMENTS_YY");
    //MEH

    double plateScale = 0.0;        // Plate scale
    long numGood = 0;               // Number of good rows
    for (long row = 0; row < size; row++) {
      //psU32 flags = flagsV->data.U32[row]; // psFitsTableGetU32(NULL, table, row, "FLAGS");
      psU64 flags = flagsV->data.U64[row]; // table reads in as U64 
      if (flags & SOURCE_MASK) {
        psTrace("ppMops.read", 10, "Discarding row %ld from input %d because of flags: %lud", row, i, flags);
        det->mask->data.U8[row] = 0xFF;
        continue;
      }

      //MEH -- section off flags2 and diffstats cuts to just be for WSdiffs -- 
      // -- need to test similarity/differences for WW/WS -- have separate block for WW
      // -- may need to move block
      // -- block should set vars and then run rej afterwards -- not clear want all for both -- 

//      if (!strcasecmp(args->difftype, "WW")) {
//        // cut non-physical moments for PS -- <2.0? -- need to modify w/ mask level for edges
//        if (!isfinite(mxx->data.F32[row]) || !isfinite(myy->data.F32[row])) {
//          psTrace("ppMops.read", 10, "Discarding row %ld from input %d because of NAN MOMENTS_XX/YY params: < 2 2:  %f %f ", row, i, mxx->data.F32[row],myy->data.F32[row]);
//          det->mask->data.U8[row] = 0xFF;
//          continue;
//        }
//        if ( (mxx->data.F32[row]<2.0 || myy->data.F32[row]<2.0) && rmask->data.F32[row]>0.2 ) {
//          psTrace("ppMops.read", 10, "Discarding row %ld from input %d because of non-physical MOMENTS_XX/YY params: < 2 2:  %f %f ", row, i, mxx->data.F32[row],myy->data.F32[row]);
//          det->mask->data.U8[row] = 0xFF;
//          continue;
//        }
//      }

      if (!strcasecmp(args->difftype, "WS")) {
//      if (!strcasecmp(args->difftype, "WS") && (!strcasecmp(args->camera, "GPC2")) {
        psU64 flags2 = flags2V->data.U64[row]; // table reads in as U64 
        if (flags2 & SOURCE_MASK2) {
          psTrace("ppMops.read", 10, "Discarding row %ld from input %d because of flags2: %lud", row, i, flags2);
          det->mask->data.U8[row] = 0xFF;
          continue;
        }

	// MEH -- filter on same source P/M in diff (movers shouldn't have) -- WS deeper and may have M
        // cut P/M sources at same position (stationary) -- NULL is issue when isn't one, lower s/n larger centroid error
        if ( (isfinite(rp->data.F32[row]) && isfinite(snp->data.F32[row])) && (rp->data.F32[row]>4.0 && snp->data.F32[row]>5.0) ) { 
            psTrace("ppMops.read", 10, "Discarding row %ld from input %d because of NAN R_P/SN_P or R_P/SN_P params: >4 >5 : %f %f ", row, i, rp->data.F32[row],snp->data.F32[row]);
          det->mask->data.U8[row] = 0xFF;
          continue;
        }

        if ( (isfinite(rm->data.F32[row]) && isfinite(snm->data.F32[row])) && (rm->data.F32[row]<4.0 && snm->data.F32[row]>5.0) ) { 
            psTrace("ppMops.read", 10, "Discarding row %ld from input %d because of NAN R_M/SN_M or R_M/SN_M params: <4 >5 : %f %f ", row, i, rm->data.F32[row],snm->data.F32[row]);
          det->mask->data.U8[row] = 0xFF;
          continue;
        }

        // MEH -- add diff params to discard -- needs to be in config.. -- WSdiff only?
        // -- needs to be finite to test but unclear if should reject (probably)
        // -- using Napoli 2013 to start with
        // DIFF_NPOS>3
        // DIFF_FRATIO>0.6
        // DIFF_NRATIO_BAD>0.4
        // DIFF_NRATIO_MASK>0.4
        // DIFF_NRATIO_ALL>0.3
	// -- increase the cuts for gpc2 to exclude the poor masked edges more
        // DIFF_FRATIO>0.6 -- losing some fainter sources @0.7
        // DIFF_NRATIO_BAD>0.5
        // DIFF_NRATIO_MASK>0.6
        // DIFF_NRATIO_ALL>0.5
        // -- unclear about npos still -- so remove from check 
        //if (npos->data.S32[row]<=3 ||
        if (!isfinite(fpos->data.F32[row]) || !isfinite(rbad->data.F32[row]) || !isfinite(rmask->data.F32[row]) || !isfinite(rall->data.F32[row])) {
           //psTrace("ppMops.read", 10, "Discarding row %ld from input %d because of NAN diff params: 3, 0.6, 0.4, 0.4, 0.3: %d %g %f %f %f ", row, i, npos->data.S32[row],fpos->data.F32[row],rbad->data.F32[row],rmask->data.F32[row],rall->data.F32[row]);
          psTrace("ppMops.read", 10, "Discarding row %ld from input %d because of NAN diff params: 3, 0.6, 0.4, 0.4, 0.3:  %g %f %f %f ", row, i, fpos->data.F32[row],rbad->data.F32[row],rmask->data.F32[row],rall->data.F32[row]);
          det->mask->data.U8[row] = 0xFF;
          continue;
        }
        if (fpos->data.F32[row]<=0.6 || rbad->data.F32[row]<=0.5 || rmask->data.F32[row]<=0.6 || rall->data.F32[row]<=0.5) {
          //psTrace("ppMops.read", 10, "Discarding row %ld from input %d because of diff params: 3, 0.6, 0.4, 0.4, 0.3: %d %g %f %f %f ", row, i, npos->data.S32[row], fpos->data.F32[row], rbad->data.F32[row], rmask->data.F32[row], rall->data.F32[row]);
          psTrace("ppMops.read", 10, "Discarding row %ld from input %d because of diff params: 3, 0.6, 0.4, 0.4, 0.3: %g %f %f %f ", row, i, fpos->data.F32[row], rbad->data.F32[row], rmask->data.F32[row], rall->data.F32[row]);
	  det->mask->data.U8[row] = 0xFF;
	  continue;
        }

	// cut non-physical moments for PS -- <2.0? -- need to modify w/ mask level for edges
        if (!isfinite(mxx->data.F32[row]) || !isfinite(myy->data.F32[row])) {
	    psTrace("ppMops.read", 10, "Discarding row %ld from input %d because of NAN MOMENTS_XX/YY params: < 2 2:  %f %f ", row, i, mxx->data.F32[row],myy->data.F32[row]);
          det->mask->data.U8[row] = 0xFF;
          continue;
        }
        if ( (mxx->data.F32[row]<=2.0 || myy->data.F32[row]<=2.0) && rmask->data.F32[row]>0.2 ) {
	    psTrace("ppMops.read", 10, "Discarding row %ld from input %d because of non-physical MOMENTS_XX/YY params: < 2 2:  %f %f ", row, i, mxx->data.F32[row],myy->data.F32[row]);
          det->mask->data.U8[row] = 0xFF;
	  continue;
        }

      } 

      // Calculate error in RA, Dec
      double xErr = xErrV->data.F32[row];
      double yErr = yErrV->data.F32[row];
      double scale = scaleV->data.F32[row];
      double angle = angleV->data.F32[row];
      if (!isfinite(det->x->data.F32[row]) || !isfinite(det->y->data.F32[row]) ||
          !isfinite(det->ra->data.F64[row]) || !isfinite(det->dec->data.F64[row]) ||
          !isfinite(mag->data.F32[row]) || !isfinite(magErr->data.F32[row]) ||
          !isfinite(xErr) || !isfinite(yErr) || !isfinite(scale) || !isfinite(angle)) {
        psTrace("ppMops.read", 10,
                "Discarding row %ld from input %d because of non-finite values: "
                "%f %f %lf %lf %f %f %f %f %f %f",
                row, i,
                det->x->data.F32[row], det->y->data.F32[row],
                det->ra->data.F64[row], det->dec->data.F64[row],
                mag->data.F32[row], magErr->data.F32[row],
                xErr, yErr, scale, angle);
        det->mask->data.U8[row] = 0xFF;
        continue;
      }

      // XXX Not at all sure I've got the angles around the right way here...
      double cosAngle = cos(angle), sinAngle = sin(angle);
      double cosAngle2 = PS_SQR(cosAngle), sinAngle2 = PS_SQR(sinAngle);
      double xErr2 = PS_SQR(xErr), yErr2 = PS_SQR(yErr);
      double errScale = scale / 3600.0;
      det->raErr->data.F64[row] = errScale * sqrt(cosAngle2 * xErr2 + sinAngle2 * yErr2);
      det->decErr->data.F64[row] = errScale * sqrt(sinAngle2 * xErr2 + cosAngle2 * yErr2);
      det->mask->data.U8[row] = 0;
      plateScale += scale;
      //Update the platescale value (should be a constant)
      if (isfinite(scale)) {
          det->platescale = scale;
          //printf("platescale = %g\n", scale);
      }
      numGood++;

      // Same for EXT data if version permits
      if (skyChipPsfVersion >= 3) {
        xErr = xExtErrV->data.F32[row];
        yErr = yExtErrV->data.F32[row];
        scale = scaleExtV->data.F32[row]/3600.;
        angle = angleExtV->data.F32[row];
        cosAngle = cos(angle);
        sinAngle = sin(angle);
        cosAngle2 = PS_SQR(cosAngle);
        sinAngle2 = PS_SQR(sinAngle);
        xErr2 = PS_SQR(xErr);
        yErr2 = PS_SQR(yErr);
        errScale = scale / 3600.0;
        det->raExtErr->data.F64[row] = errScale * sqrt(cosAngle2 * xErr2 + sinAngle2 * yErr2);
        det->decExtErr->data.F64[row] = errScale * sqrt(sinAngle2 * xErr2 + cosAngle2 * yErr2);
      }
    }
    det->numGood = numGood;

    if (isfinite(args->zp) && numGood > 0) {
      psBinaryOp(mag, mag, "+", psScalarAlloc(args->zp, PS_TYPE_F32));
    }

    printf("Detection platescale = %g\n", det->platescale);
    psTrace("ppMops.read", 2, "Read %ld good rows from %s\n", numGood, (const char*)name);
  }

  psTrace("ppMops.read", 1, "Done reading input detections\n");

  return detections;
}

static psVector* createDummyF32(long size) {
  psVector* dummy = psVectorAlloc(size, PS_TYPE_F32);
  psVectorInit(dummy, NAN);
  return dummy;
}

static psVector* createDummyS32(long size) {
  psVector* dummy = psVectorAlloc(size, PS_TYPE_S32);
  psVectorInit(dummy, 0);
  return dummy;
}

static void addDummyValues(psMetadata* md, long size, int version) {
  psMetadataAdd(md, PS_LIST_TAIL, "X_EXT", PS_DATA_VECTOR, "EXT model x coordinate", createDummyF32(size));
  psMetadataAdd(md, PS_LIST_TAIL, "Y_EXT", PS_DATA_VECTOR, "EXT model y coordinate", createDummyF32(size));
  psMetadataAdd(md, PS_LIST_TAIL, "X_EXT_SIG", PS_DATA_VECTOR, "Sigma in EXT x coordinate", createDummyF32(size));
  psMetadataAdd(md, PS_LIST_TAIL, "Y_EXT_SIG", PS_DATA_VECTOR, "Sigma in EXT y coordinate", createDummyF32(size));
  psMetadataAdd(md, PS_LIST_TAIL, "EXT_INST_MAG",  PS_DATA_VECTOR, "EXT fit instrumental magnitude", createDummyF32(size));
  psMetadataAdd(md, PS_LIST_TAIL, "EXT_INST_MAG_SIG",  PS_DATA_VECTOR, "Sigma of PSF instrumental magnitude", createDummyF32(size));
  psMetadataAdd(md, PS_LIST_TAIL, "NPARAMS",  PS_DATA_VECTOR, "Number of model parameters", createDummyS32(size));
  psMetadataAdd(md, PS_LIST_TAIL, "EXT_WIDTH_MAJ",  PS_DATA_VECTOR, "EXT width (major axis), length for trail", createDummyF32(size));
  psMetadataAdd(md, PS_LIST_TAIL, "EXT_WIDTH_MIN",  PS_DATA_VECTOR, "EXT width (minor axis), sigma for trail", createDummyF32(size));
  psMetadataAdd(md, PS_LIST_TAIL, "EXT_THETA",  PS_DATA_VECTOR, "EXT orientation angle", createDummyF32(size));
  psMetadataAdd(md, PS_LIST_TAIL, "EXT_WIDTH_MAJ_ERR",  PS_DATA_VECTOR, "EXT width error (major axis)", createDummyF32(size));
  psMetadataAdd(md, PS_LIST_TAIL, "EXT_WIDTH_MIN_ERR",  PS_DATA_VECTOR, "EXT width error (minor axis)", createDummyF32(size));
  psMetadataAdd(md, PS_LIST_TAIL, "EXT_THETA_ERR",  PS_DATA_VECTOR, "EXT orientation angle (error)", createDummyF32(size));
  if (version >= 3) {
    psTrace("ppMops.read", 10, "Adding columns for version PS1_DV%d\n", version);
    psMetadataAdd(md, PS_LIST_TAIL, "RA_EXT",  PS_DATA_VECTOR, "Fitted centroid RA", createDummyF32(size));
/*     psMetadataAdd(md, PS_LIST_TAIL, "RA_EXT_SIGMA",  PS_DATA_VECTOR, "Fitted RA sigma", createDummyF32(size)); *\/ */
    psMetadataAdd(md, PS_LIST_TAIL, "DEC_EXT",  PS_DATA_VECTOR, "Fitted centroid DEC", createDummyF32(size));
/*     psMetadataAdd(md, PS_LIST_TAIL, "DEC_EXT_SIGMA",  PS_DATA_VECTOR, "Fitted DEC sigma", createDummyF32(size)); *\/ */
    psMetadataAdd(md, PS_LIST_TAIL, "POSANG_EXT",  PS_DATA_VECTOR, "Fitted position angle", createDummyF32(size));
    psMetadataAdd(md, PS_LIST_TAIL, "PLTSCALE_EXT",  PS_DATA_VECTOR, "Plate scale at centroid", createDummyF32(size));
    psMetadataAdd(md, PS_LIST_TAIL, "EXT_FLUX",  PS_DATA_VECTOR, "Fitted flux", createDummyF32(size));
    psMetadataAdd(md, PS_LIST_TAIL, "EXT_CAL_MAG",  PS_DATA_VECTOR, "Calibrated mag", createDummyF32(size));
    psMetadataAdd(md, PS_LIST_TAIL, "EXT_MAG_SIG",  PS_DATA_VECTOR, "Mag sigma", createDummyF32(size));
    psMetadataAdd(md, PS_LIST_TAIL, "EXT_CHISQ",  PS_DATA_VECTOR, "Chi^2 of fit", createDummyF32(size));
    psMetadataAdd(md, PS_LIST_TAIL, "EXT_NDOF",  PS_DATA_VECTOR, "Fit degrees of freedom", createDummyS32(size));
  }
}

static void replaceDummyValuesF32(const char* colName, psMetadata* source, psMetadata* target, psVector* indexes) {
  psTrace("ppMops.read", 10, "Trying to get column %s\n", colName);
  psVector* source_vector = psMetadataLookupVector(NULL, source, colName);
  psVector* target_vector = psMetadataLookupVector(NULL, target, colName);
  for (long index = 0; index<indexes->n; ++index) {
    target_vector->data.F32[indexes->data.S64[index]] = source_vector->data.F32[index];
  }
  psTrace("ppMops.read", 10, "OK for %s\n", colName);
}

static void replaceDummyValuesS32(const char* colName, psMetadata* source, psMetadata* target, psVector* indexes) {
  psTrace("ppMops.read", 10, "Trying to get column %s\n", colName);
  psVector* source_vector = psMetadataLookupVector(NULL, source, colName);
  psVector* target_vector = psMetadataLookupVector(NULL, target, colName);
  for (long index = 0; index<indexes->n; ++index) {
    target_vector->data.S32[indexes->data.S64[index]] = source_vector->data.S32[index];
  }
  psTrace("ppMops.read", 10, "OK for %s\n", colName);
}

static void renameDummyValuesF32(const char* colNameSource, psMetadata* source, const char* colNameTarget, psMetadata* target, psVector* indexes) {
  psTrace("ppMops.read", 10, "Trying to rename column %s to %s\n", colNameSource, colNameTarget);
  psVector* source_vector = psMetadataLookupVector(NULL, source, colNameSource);
  psVector* target_vector = psMetadataLookupVector(NULL, target, colNameTarget);
  for (long index = 0; index<indexes->n; ++index) {
    target_vector->data.F32[indexes->data.S64[index]] = source_vector->data.F32[index];
  }
  psTrace("ppMops.read", 10, "OK for %s (renamed by %s)\n", colNameSource, colNameTarget);
}
