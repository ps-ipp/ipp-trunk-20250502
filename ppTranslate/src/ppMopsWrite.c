#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <pslib.h>

#include "ppMops.h"
#include "ppTranslateVersion.h"

static bool addOutputColumn(psMetadata *table, const psArray *detections, long total, char *outColName, char *inColName, bool convertTo32);
static bool addSkyfileIDColumn(psMetadata *table, const psArray *detections, long total, char *colName);

bool ppMopsWrite(const psArray *detections, const ppMopsArguments *args)
{
  psFits *fits = psFitsOpen(args->output, "w"); // FITS file
  if (!fits) {
    psError(PS_ERR_IO, false, "Unable to open output file.");
    return false;
  }

  psMetadata *header = psMetadataAlloc(); // Header to write
  psString source = ppTranslateSource(), version = ppTranslateVersion();
  psMetadataAddStr(header, PS_LIST_TAIL, "SWSOURCE", 0, "Software source", source);
  psMetadataAddStr(header, PS_LIST_TAIL, "SWVERSN", 0, "Software version", version);
  ppTranslateVersionHeader(header);
  psFree(source);
  psFree(version);

  psMetadataAddStr(header, PS_LIST_TAIL, "EXP_NAME", 0, "Exposure name", args->exp_name);
  psMetadataAddS64(header, PS_LIST_TAIL, "EXP_ID", 0, "Exposure identifier", args->exp_id);
  psMetadataAddS64(header, PS_LIST_TAIL, "CHIP_ID", 0, "Chip stage identifier", args->chip_id);
  psMetadataAddS64(header, PS_LIST_TAIL, "CAM_ID", 0, "Cam stage identifier", args->cam_id);
  psMetadataAddS64(header, PS_LIST_TAIL, "FAKE_ID", 0, "Fake stage identifier", args->fake_id);
  psMetadataAddS64(header, PS_LIST_TAIL, "WARP_ID", 0, "Warp stage identifier", args->warp_id);
  psMetadataAddS64(header, PS_LIST_TAIL, "DIFF_ID", 0, "Diff stage identifier", args->diff_id);
  psMetadataAddBool(header, PS_LIST_TAIL, "DIFF_POS", 0, "Positive subtraction?", args->positive);

  // Get these header words from the first input non null input
  // but for SEEING which is the median of the (non-NULL) detections
  // seeings
  ppMopsDetections *det = NULL;
  for (int d = 0; d < detections->n; d++) {
    if (detections->data[d] != NULL) {
      if (det == NULL) {
        det = detections->data[d];
      }
    }
  }
  //Get the SEEING weighted mean. Update the PLTSCALE value
  float seeing = 0.;
  int totalGood = 0;
  int platescale_has_been_set = 0;
  for (int d = 0; d < detections->n; d++) {
    if ( (detections->data[d] != NULL) && (isfinite(((ppMopsDetections*) detections->data[d])->seeing)) ) {
      seeing += ((ppMopsDetections*) detections->data[d])->seeing;
      totalGood += 1;
      if (isfinite(((ppMopsDetections*) detections->data[d])->platescale)) {
          if (platescale_has_been_set == 0) {
          det->platescale = ((ppMopsDetections*) detections->data[d])->platescale;
              platescale_has_been_set = 1;
          } else {
              if (det->platescale != ((ppMopsDetections*) detections->data[d])->platescale) {
                  printf("Different values for platescale: %g - %g in detection %d\n",
                         det->platescale,
                         ((ppMopsDetections*) detections->data[d])->platescale,
                         d);
      }
    }
  }
    }
  }
  //printf("plate-scale = [%g]\n", det->platescale);
  seeing /= (float) totalGood;
  if (det != NULL) {
    psMetadataAddF64(header, PS_LIST_TAIL, "MJD-OBS", 0, "MJD of exposure midpoint", det->mjd);
    psMetadataAddStr(header, PS_LIST_TAIL, "RA", 0, "Right Ascension of boresight", det->raBoresight);
    psMetadataAddStr(header, PS_LIST_TAIL, "DEC", 0, "Declination of boresight", det->decBoresight);
    psMetadataAddF64(header, PS_LIST_TAIL, "TEL_ALT", 0, "Telescope altitude", det->alt);
    psMetadataAddF64(header, PS_LIST_TAIL, "TEL_AZ", 0, "Telescope azimuth", det->az);
    psMetadataAddF64(header, PS_LIST_TAIL, "EXPTIME", 0, "Exposure time (sec)", det->exptime);
    psMetadataAddF64(header, PS_LIST_TAIL, "ROTANGLE", 0, "Rotator position angle", det->posangle);
    psMetadataAddStr(header, PS_LIST_TAIL, "FILTER", 0, "Filter name", det->filter);
    psMetadataAddF32(header, PS_LIST_TAIL, "AIRMASS", 0, "Airmass of exposure", det->airmass);
    psMetadataAddF32(header, PS_LIST_TAIL, "SEEING", 0, "Mean seeing", seeing);
    //MOPS want the name FWHM for SEEING
    psMetadataAddF32(header, PS_LIST_TAIL, "FWHM", 0, "Mean seeing", seeing);
  } else {
    psWarning("no inputs with surviving detections. output header will be incomplete");
  }
  psMetadataAddStr(header, PS_LIST_TAIL, "OBSCODE", 0, "IAU Observatory code", args->obscode);
  psMetadataAddF32(header, PS_LIST_TAIL, "MAGZP", 0, "Magnitude zero point", args->zp);
  //MOPS want the name ZEROPOINT for MAGZP
  psMetadataAddF32(header, PS_LIST_TAIL, "ZEROPOINT", 0, "Magnitude zero point", args->zp);
  psMetadataAddF32(header, PS_LIST_TAIL, "MAGZPERR", 0, "Error in magnitude zero point", args->zpErr);
  psMetadataAddF32(header, PS_LIST_TAIL, "ASTRORMS", 0, "RMS of astrometric fit", args->rmsAstrom);
  psMetadataAddStr(header, PS_LIST_TAIL, "CMMTOBS", 0, "Exposure comment", args->comment);
  psMetadataAddStr(header, PS_LIST_TAIL, "OBS_MODE", 0, "Exposure observation mode", args->obsMode);
  psMetadataAddStr(header, PS_LIST_TAIL, "DIFFTYPE", 0, "WW: Warp-Warp diff / WS: Warp-Stack diff / SW: Stack-Warp diff", args->difftype);
  psMetadataAddF32(header, PS_LIST_TAIL, "SKY", 0, "Exposure avg sky background", args->sky);
  psMetadataAddStr(header, PS_LIST_TAIL, "SHUTOUTC", 0, "Camera exposure shutter open (UTC, DB arg)", args->shutoutc);
  psMetadataAddF32(header, PS_LIST_TAIL, "PLTSCALE_EXT", 0, "Plate scale at centroid", det->platescale);
  psMetadataAddF32(header, PS_LIST_TAIL, "PLTSCALE", 0, "Plate scale at centroid", det->platescale);
  psMetadataAddStr(header, PS_LIST_TAIL, "FPA.SHUTOUTC", 0, "Time shutter open", det->fpashutoutc);
  psMetadataAddStr(header, PS_LIST_TAIL, "FPA.SHUTCUTC", 0, "Time shutter close", det->fpashutcutc);
  psMetadataAddStr(header, PS_LIST_TAIL, "FPA.SHMDOUTC", 0, "Time shutter open mid-fp", det->fpashmdoutc);
  psMetadataAddStr(header, PS_LIST_TAIL, "FPA.SHMDCUTC", 0, "Time shutter close mid-fp", det->fpashmdcutc);
  psMetadataAddStr(header, PS_LIST_TAIL, "PSREFCAT", 0, "Reference catalog used", det->refcat);

  //field in header that tells about the CMF version
  char cmfVersion[16];
  ps_snprintf_nowarn(cmfVersion, 16, PS1_DV_FORMAT, args->version);
  psMetadataAddStr(header, PS_LIST_TAIL, "CMFVERSION", 0, "CMF version", cmfVersion);

  // Find the total number of detections
  long total = 0;
  for (long i=0; i<detections->n; i++) {
    ppMopsDetections *det = detections->data[i];
    if (!det) {
      continue;
    }
    total += det->num;
  }

  psTrace("ppMops.write", 1, "Writing %ld rows to %s", total, args->output);

  if (total == 0) {
    // Write dummy table
    psMetadata *row = psMetadataAlloc(); // Output row
    psMetadataAddF64(row, PS_LIST_TAIL, "RA", 0, "Right ascension (degrees)", NAN);
    psMetadataAddF64(row, PS_LIST_TAIL, "RA_ERR", 0, "Right ascension error (degrees)", NAN);
    psMetadataAddF64(row, PS_LIST_TAIL, "DEC", 0, "Declination (degrees)", NAN);
    psMetadataAddF64(row, PS_LIST_TAIL, "DEC_ERR", 0, "Declination error (degrees)", NAN);
    psMetadataAddF32(row, PS_LIST_TAIL, "MAG", 0, "Magnitude", NAN);
    psMetadataAddF32(row, PS_LIST_TAIL, "MAG_ERR", 0, "Magnitude error", NAN);
    psMetadataAddF32(row, PS_LIST_TAIL, "PSF_CHI2", 0, "chi^2 of PSF fit", NAN);
    psMetadataAddS32(row, PS_LIST_TAIL, "PSF_DOF", 0, "Degrees of freedom of PSF fit", 0);
    psMetadataAddF32(row, PS_LIST_TAIL, "CR_SIGNIFICANCE", 0, "Significance of CR", NAN);
    psMetadataAddF32(row, PS_LIST_TAIL, "EXT_SIGNIFICANCE", 0, "Significance of extendedness", NAN);
    psMetadataAddF32(row, PS_LIST_TAIL, "PSF_MAJOR", 0, "PSF major axis (pixels)", NAN);
    psMetadataAddF32(row, PS_LIST_TAIL, "PSF_MINOR", 0, "PSF minor axis (pixels)", NAN);
    psMetadataAddF32(row, PS_LIST_TAIL, "PSF_THETA", 0, "PSF position angle (deg on chip)", NAN);
    psMetadataAddF32(row, PS_LIST_TAIL, "PSF_QUALITY", 0, "PSF quality factor", NAN);
    psMetadataAddS32(row, PS_LIST_TAIL, "PSF_NPIX", 0, "Number of pixels in PSF", 0);
    psMetadataAddF32(row, PS_LIST_TAIL, "MOMENTS_XX", 0, "xx moment", NAN);
    psMetadataAddF32(row, PS_LIST_TAIL, "MOMENTS_XY", 0, "xy moment", NAN);
    psMetadataAddF32(row, PS_LIST_TAIL, "MOMENTS_YY", 0, "yy moment", NAN);
    psMetadataAddU32(row, PS_LIST_TAIL, "FLAGS", 0, "Detection bit flags", 0);
    psMetadataAddS64(row, PS_LIST_TAIL, "DIFF_SKYFILE_ID", 0, "Identifier for diff skyfile", 0);

    psMetadataAddS32(row, PS_LIST_TAIL, "N_POS", 0, "Number of positive pixels", 0);
    psMetadataAddF32(row, PS_LIST_TAIL, "F_POS", 0, "Fraction of positive pixels", NAN);
    psMetadataAddF32(row, PS_LIST_TAIL, "RATIO_BAD", 0, "Ratio of positive pixels to negative", NAN);
    psMetadataAddF32(row, PS_LIST_TAIL, "RATIO_MASK", 0, "Ratio of positive pixels to masked", NAN);
    psMetadataAddF32(row, PS_LIST_TAIL, "RATIO_ALL", 0, "Ratio of positive pixels to all", NAN);

    if (args->version == 2) {
      // Write data of version 2 (see ICD)
      psMetadataAdd (row, PS_LIST_TAIL, "IPP_IDET",         PS_DATA_U32, "IPP detection identifier index",
                     NAN);
      psMetadataAdd (row, PS_LIST_TAIL, "PSF_INST_FLUX",    PS_DATA_F32, "PSF fit instrumental magnitude",
                     NAN);
      psMetadataAdd (row, PS_LIST_TAIL, "PSF_INST_FLUX_SIG",PS_DATA_F32, "Sigma of PSF instrumental magnitude",
                     NAN);
      psMetadataAdd (row, PS_LIST_TAIL, "AP_MAG",           PS_DATA_F32, "magnitude in standard aperture",
                     NAN);
      psMetadataAdd (row, PS_LIST_TAIL, "AP_MAG_RAW",       PS_DATA_F32, "magnitude in real aperture",
                     NAN);
      psMetadataAdd (row, PS_LIST_TAIL, "AP_MAG_RADIUS",    PS_DATA_F32, "radius used for aperture mags",
                     NAN);
      psMetadataAdd (row, PS_LIST_TAIL, "AP_FLUX",          PS_DATA_F32, "instrumental flux in standard aperture",
                     NAN);
      psMetadataAdd (row, PS_LIST_TAIL, "AP_FLUX_SIG",      PS_DATA_F32, "aperture flux error",
                     NAN);
      psMetadataAdd (row, PS_LIST_TAIL, "PEAK_FLUX_AS_MAG", PS_DATA_F32, "Peak flux expressed as magnitude",
                     NAN);
      psMetadataAdd (row, PS_LIST_TAIL, "CAL_PSF_MAG",      PS_DATA_F32, "PSF Magnitude using supplied calibration",
                     NAN);
      psMetadataAdd (row, PS_LIST_TAIL, "CAL_PSF_MAG_SIG",  PS_DATA_F32, "measured scatter of zero point calibration",
                     NAN);
      psMetadataAdd (row, PS_LIST_TAIL, "SKY",              PS_DATA_F32, "Sky level",
                     NAN);
      psMetadataAdd (row, PS_LIST_TAIL, "SKY_SIGMA",        PS_DATA_F32, "Sigma of sky level",
                     NAN);
      psMetadataAdd (row, PS_LIST_TAIL, "PSF_QF_PERFECT",   PS_DATA_F32, "PSF coverage/quality factor (poor)",
                     NAN);
      psMetadataAdd (row, PS_LIST_TAIL, "MOMENTS_R1",       PS_DATA_F32, "first radial moment",
                     NAN);
      psMetadataAdd (row, PS_LIST_TAIL, "MOMENTS_RH",       PS_DATA_F32, "half radial moment",
                     NAN);
      psMetadataAdd (row, PS_LIST_TAIL, "KRON_FLUX",        PS_DATA_F32, "Kron Flux (in 2.5 R1)",
                     NAN);
      psMetadataAdd (row, PS_LIST_TAIL, "KRON_FLUX_ERR",    PS_DATA_F32, "Kron Flux Error",
                     NAN);
      psMetadataAdd (row, PS_LIST_TAIL, "KRON_FLUX_INNER",  PS_DATA_F32, "Kron Flux (in 1.0 R1)",
                     NAN);
      psMetadataAdd (row, PS_LIST_TAIL, "KRON_FLUX_OUTER",  PS_DATA_F32, "Kron Flux (in 4.0 R1)",
                     NAN);
      psMetadataAdd (row, PS_LIST_TAIL, "DIFF_R_P",         PS_DATA_F32, "distance to positive match source",
                     NAN);
      psMetadataAdd (row, PS_LIST_TAIL, "DIFF_SN_P",        PS_DATA_F32, "signal-to-noise of pos match src",
                     NAN);
      psMetadataAdd (row, PS_LIST_TAIL, "DIFF_R_M",         PS_DATA_F32, "distance to negative match source",
                     NAN);
      psMetadataAdd (row, PS_LIST_TAIL, "DIFF_SN_M",        PS_DATA_F32, "signal-to-noise of neg match src",
                     NAN);
      psMetadataAdd (row, PS_LIST_TAIL, "FLAGS2",           PS_DATA_U32, "psphot analysis flags (group 2)",
                     0);
      psMetadataAdd (row, PS_LIST_TAIL, "N_FRAMES",         PS_DATA_U16, "Number of frames overlapping source center",
                     0);
      psMetadataAdd (row, PS_LIST_TAIL, "PADDING",          PS_DATA_S16, "padding",
                     0);
      if (args->version == 3) {
// Write data of version 3 (see ICD)
        psMetadataAdd (row, PS_LIST_TAIL, "RA_EXT",         PS_DATA_F32, "Fitted centroid RA",
                       0);
        psMetadataAdd (row, PS_LIST_TAIL, "RA_EXT_SIGMA",   PS_DATA_F32, "Fitted RA sigma",
                       0);
        psMetadataAdd (row, PS_LIST_TAIL, "DEC_EXT",        PS_DATA_F32, "Fitted centroid DEC",
                       0);
        psMetadataAdd (row, PS_LIST_TAIL, "DEC_EXT_SIGMA",  PS_DATA_F32, "Fitted DEC sigma",
                       0);
        psMetadataAdd (row, PS_LIST_TAIL, "POSANG_EXT",     PS_DATA_F32, "Fitted position angle",
                       0);
        psMetadataAdd (row, PS_LIST_TAIL, "EXT_FLUX",       PS_DATA_F32, "Fitted flux",
                       0);
        psMetadataAdd (row, PS_LIST_TAIL, "EXT_CAL_MAG",    PS_DATA_F32, "Calibrated mag",
                       0);
        psMetadataAdd (row, PS_LIST_TAIL, "EXT_MAG_SIG",    PS_DATA_F32, "Mag sigma",
                       0);
        psMetadataAdd (row, PS_LIST_TAIL, "EXT_CHISQ",      PS_DATA_F32, "Chi^2 of fit",
                       0);
        psMetadataAdd (row, PS_LIST_TAIL, "EXT_NDOF",       PS_DATA_F32, "Fit degrees of freedom",
                       0);
      }
    }

    if (!psFitsWriteTableEmpty(fits, header, row, OUT_EXTNAME)) {
      psErrorStackPrint(stderr, "Unable to write empty table.");
      psFree(header);
      psFree(row);
      return false;
    }
    psFree(row);
  } else {

#define addColumn(_outName, _inName, _convertTo32)                      \
    if (!addOutputColumn(table, detections, total, _outName, _inName, _convertTo32)) { \
      psError(PS_ERR_UNKNOWN, false, "Failed to add column %s", _outName); \
      return false;                                                     \
    }

    // Allocate the output table
    psMetadata *table = psMetadataAlloc();
    addColumn("RA", "RA_PSF", 0);
    addColumn("RA_ERR", NULL, 0);      // calculated from various parameters including X_PSF_SIG Y_PSF_SIG and POSANG
    addColumn("DEC", "DEC_PSF", 0);
    addColumn("DEC_ERR", NULL, 0);     // calculated from various parameters including X_PSF_SIG Y_PSF_SIG and POSANG
    addColumn("MAG", "PSF_INST_MAG", 0);
    addColumn("MAG_ERR", "PSF_INST_MAG_SIG", 0);
    addColumn("PSF_CHI2", "PSF_CHISQ", 0);
    addColumn("PSF_DOF", "PSF_NDOF", 1);
    addColumn("CR_SIGNIFICANCE", "CR_NSIGMA", 0);
    addColumn("EXT_SIGNIFICANCE", "EXT_NSIGMA", 0);
    addColumn("PSF_MAJOR", NULL, 0);
    addColumn("PSF_MINOR", NULL, 0);
    addColumn("PSF_THETA", NULL, 0);
    addColumn("PSF_QUALITY", "PSF_QF", 0);
    addColumn("PSF_NPIX", NULL, 1);
    addColumn("MOMENTS_XX", NULL, 0);
    addColumn("MOMENTS_XY", NULL, 0);
    addColumn("MOMENTS_YY", NULL, 0);
    addColumn("N_POS", "DIFF_NPOS", 1);
    addColumn("F_POS", "DIFF_FRATIO", 0);
    addColumn("RATIO_BAD", "DIFF_NRATIO_BAD", 0);
    addColumn("RATIO_MASK", "DIFF_NRATIO_MASK", 0);
    addColumn("RATIO_ALL", "DIFF_NRATIO_ALL", 0);
    addColumn("FLAGS", "FLAGS", 1);
    addSkyfileIDColumn(table, detections, total, "DIFF_SKYFILE_ID");
    if (args->version >= 2) {
      addColumn("IPP_IDET", NULL, 1);
      addColumn("PSF_INST_FLUX", NULL, 0);
      addColumn("PSF_INST_FLUX_SIG", NULL, 0);
      addColumn("AP_MAG", NULL, 0);
      addColumn("AP_MAG_RAW", NULL, 0);
      addColumn("AP_MAG_RADIUS", NULL, 0);
      addColumn("AP_FLUX", NULL, 0);
      addColumn("AP_FLUX_SIG", NULL, 0);
      addColumn("PEAK_FLUX_AS_MAG", NULL, 0);
      addColumn("CAL_PSF_MAG", NULL, 0);
      addColumn("CAL_PSF_MAG_SIG", NULL, 0);
      addColumn("SKY", NULL, 0);
      addColumn("SKY_SIGMA", NULL, 0);
      addColumn("PSF_QF_PERFECT", NULL, 0);
      addColumn("MOMENTS_R1", NULL, 0);
      addColumn("MOMENTS_RH", NULL, 0);
      addColumn("KRON_FLUX", NULL, 0);
      addColumn("KRON_FLUX_ERR", NULL, 0);
      addColumn("KRON_FLUX_INNER", NULL, 0);
      addColumn("KRON_FLUX_OUTER", NULL, 0);
      addColumn("DIFF_R_P", NULL, 0);
      addColumn("DIFF_SN_P", NULL, 0);
      addColumn("DIFF_R_M", NULL, 0);
      addColumn("DIFF_SN_M", NULL, 0);
      addColumn("FLAGS2", NULL, 1);
      addColumn("IPP_IDET", NULL, 0);
      addColumn("N_FRAMES", NULL, 0);
      addColumn("PADDING", NULL, 0);
      addColumn("X_EXT", NULL, 0);
      addColumn("Y_EXT", NULL, 0);
      addColumn("X_EXT_SIG", NULL, 0);
      addColumn("Y_EXT_SIG", NULL, 0);
      addColumn("EXT_INST_MAG", NULL, 0);
      addColumn("EXT_INST_MAG_SIG", NULL, 0);
      addColumn("NPARAMS", NULL, 0);
      addColumn("EXT_WIDTH_MAJ", NULL, 0);
      addColumn("EXT_WIDTH_MIN", NULL, 0);
      addColumn("EXT_THETA", NULL, 0);
      addColumn("EXT_WIDTH_MAJ_ERR", NULL, 0);
      addColumn("EXT_WIDTH_MIN_ERR", NULL, 0);
      addColumn("EXT_THETA_ERR", NULL, 0);
    }
    if (args->version >= 3) {
      addColumn("RA_EXT", NULL, 0);
      addColumn("RA_EXT_SIGMA", NULL, 0);
      addColumn("DEC_EXT", NULL, 0);
      addColumn("DEC_EXT_SIGMA", NULL, 0);
      addColumn("POSANG_EXT", NULL, 0);
      addColumn("EXT_FLUX", NULL, 0);
      addColumn("EXT_CAL_MAG", NULL, 0);
      addColumn("EXT_MAG_SIG", NULL, 0);
      addColumn("EXT_CHISQ", NULL, 0);
      addColumn("EXT_NDOF", NULL, 0);
    }

    if (!psFitsWriteTableAllColumns(fits, header, table, OUT_EXTNAME)) {
      psError(psErrorCodeLast(), false, "Unable to write table");
      return false;
    }
    psFree(table);
  }

  psFree(header);
  psFitsClose(fits);

  psTrace("ppMops.write", 1, "Done writing %ld rows to %s", det->num, args->output);

  return true;
}

// extension parameter values:
// 0: SkyChip.psf
// 1: SkyChip.xfit
// Any other value is ignored
static bool addOutputColumn(psMetadata *table, const psArray *detections, long outputSize, char *outColumnName, char *inColumnName, bool convertTo32)
{
  if (inColumnName == NULL) {
    inColumnName = outColumnName;
  }
  psVector *out = NULL;
  if (convertTo32) {
    // psFitsReadTableAllColumns reads columns of cfitsio type LONG and ULONG into a 64 bit integers
    // We want to write 32 bits to the output.
    int next = 0;
    for (long i=0; i<detections->n; i++) {
      ppMopsDetections *det = detections->data[i];
      if (!det || det->num == 0) {
        // no detections survived for this input
        continue;
      }
      psVector *in = NULL;
      in = psMetadataLookupVector(NULL, det->table, inColumnName);
      if (!in) {
        psError(PS_ERR_PROGRAMMING, true, "failed to find input column: %s (convertTo32 is true)", inColumnName);
        return false;
      }
      if (in->type.type != PS_TYPE_S64 && in->type.type != PS_TYPE_U64) {
        psError(PS_ERR_PROGRAMMING, true, "input column to convert is not S64 or U64: %s %d",
                inColumnName, in->type.type);
        return false;
      }
      if (out == NULL) {
        // First time through set up the output vector and the copy parameters
        if (in->type.type == PS_TYPE_S64) {
          out = psVectorAlloc(outputSize, PS_TYPE_S32);
        } else {
          out = psVectorAlloc(outputSize, PS_TYPE_U32);
        }
      }
      for (long d=0; d < det->num; d++) {
        if (in->type.type == PS_TYPE_S64) {
          out->data.S32[next++] = in->data.S64[d];
        } else {
          out->data.U32[next++] = in->data.U64[d];
        }
      }
    }
  } else {
    void *next = NULL;
    int elementSize = 0;    // size of elements in vector... We are making assumptions here about the organization of primitives in memory so we can use memcopy
    for (long i=0; i<detections->n; i++) {
      ppMopsDetections *det = detections->data[i];
      if (!det || det->num == 0) {
        // no detections survived for this input
        continue;
      }
      psVector *in = NULL;
      in = psMetadataLookupVector(NULL, det->table, inColumnName);
      if (!in) {
        psError(PS_ERR_PROGRAMMING, true, "failed to find input column: %s (convertTo32 is false)", inColumnName);
        out = psVectorAlloc(outputSize, PS_TYPE_F32);
        psVectorInit(out, NAN);
        psMetadataAddVector(table, PS_LIST_TAIL, outColumnName, 0, NULL, out);
        return false;
      }
      if (out == NULL) {
        // First time through set up the output vector and the copy parameters
        out = psVectorAlloc(outputSize, in->type.type);
        next = (void *) out->data.U8;
        switch (in->type.type) {
        case PS_TYPE_S8:
          elementSize = sizeof(psS8);
          break;
        case PS_TYPE_U8:
          elementSize = sizeof(psU8);
          break;
        case PS_TYPE_S16:
          elementSize = sizeof(psS16);
          break;
        case PS_TYPE_U16:
          elementSize = sizeof(psU16);
          break;
        case PS_TYPE_S32:
          elementSize = sizeof(psS32);
          break;
        case PS_TYPE_U32:
          elementSize = sizeof(psU32);
          break;
        case PS_TYPE_S64:
          elementSize = sizeof(psS64);
          break;
        case PS_TYPE_U64:
          elementSize = sizeof(psU64);
          break;
        case PS_TYPE_F32:
          elementSize = sizeof(psF32);
          break;
        case PS_TYPE_F64:
          elementSize = sizeof(psF64);
          break;
        default:
          psError(PS_ERR_BAD_PARAMETER_TYPE, true, "Unknown vector type %d", in->type.type);
          return false;
        }
      }
      // We are doing nasty things here so we can use memcpy.
      // It would be safer to do a proper loop over the elements.
      long toCopy = det->num * elementSize;
      memcpy(next, in->data.U8, toCopy);
      next += toCopy;
    }
  }
  // Finally add the new column to the output table
  psMetadataAddVector(table, PS_LIST_TAIL, outColumnName, 0, NULL, out);
  psFree(out);    // drop reference
  return true;
}

static bool addSkyfileIDColumn(psMetadata *table, const psArray *detections, long total, char *colName)
{
  psVector *out = psVectorAlloc(total, PS_TYPE_S64);
  long next = 0;
  for (long i = 0; i<detections->n; i++) {
    ppMopsDetections *det = detections->data[i];
    if (!det) {
      continue;
    }
    psS64 diffSkyfileId = det->diffSkyfileId;
    for (long j = 0; j < det->num; j++) {
      out->data.S64[next++] = diffSkyfileId;
    }
  }
  psMetadataAddVector(table, PS_LIST_TAIL, colName, 0, NULL, out);
  psFree(out);
  return true;
}
