/** @file  pmSourceIO.c
 *
 *  @author EAM, IfA
 *
 *  @version $Revision: 1.70 $ $Name: not supported by cvs2svn $
 *  @date $Date: 2009-02-16 22:30:50 $
 *
 *  Copyright 2004 Maui High Performance Computing Center, University of Hawaii
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <math.h>
#include <string.h>
#include <strings.h>            /* for strn?casecmp */
#include <pslib.h>

#include "pmConfig.h"
#include "pmDetrendDB.h"

#include "pmHDU.h"
#include "pmFPA.h"
#include "pmFPALevel.h"
#include "pmFPAview.h"
#include "pmFPAfile.h"
#include "pmFPAfileFitsIO.h"
#include "pmConceptsRead.h"

#include "pmTrend2D.h"
#include "pmResiduals.h"
#include "pmGrowthCurve.h"
#include "pmSpan.h"
#include "pmFootprintSpans.h"
#include "pmFootprint.h"
#include "pmPeaks.h"
#include "pmMoments.h"
#include "pmModelFuncs.h"
#include "pmModelClass.h"
#include "pmModel.h"
#include "pmModelUtils.h"
#include "pmSourceMasks.h"
#include "pmSourceExtendedPars.h"
#include "pmSourceDiffStats.h"
#include "pmSourceSatstar.h"
#include "pmSourceLensing.h"
#include "pmSource.h"
#include "pmSourceFitModel.h"
#include "pmPSF.h"
#include "pmPSFtry.h"

#include "pmDetections.h"
#include "pmDetEff.h"
#include "pmSourceIO.h"

#define BLANK_HEADERS "BLANK.HEADERS"   // Name of metadata in camera configuration containing header names
                                        // for putting values into a blank PHU
static bool pmReadoutReadXSRC(pmFPAfile *file, pmReadout *readout, char * exttype, psMetadata *hduHeader, psString xsrcname, psArray *sources, long *sourceIndex);
static bool pmReadoutReadXFIT(pmFPAfile *file, pmReadout *readout, char * exttype, psMetadata *hduHeader, psString xfitname, psArray *sources, long *sourceIndex);
static bool pmReadoutReadXRAD(pmFPAfile *file, pmReadout *readout, char * exttype, psMetadata *hduHeader, psString xfitname, psArray *sources, long *sourceIndex);
static bool pmReadoutReadXGAL(pmFPAfile *file, pmReadout *readout, char * exttype, psMetadata *hduHeader, psString xfitname, psArray *sources, long *sourceIndex);

// lookup the EXTNAME values used for table data and image header segments
bool pmSourceIOextnames(psString *headname,    // Extension name for image header
			psString *dataname,    // Extension name for PSF table data
			psString *deteffname,  // Extension name for detection efficiency
			psString *xsrcname,    // Extension name for extended non-parametric measurements
			psString *xfitname,    // Extension name for extended fitted measurements
			psString *xradname,    // Extension name for radial apertures
			psString *xgalname,    // Extension name for galaxy shapes
			const pmFPAfile *file, // File of interest
			const pmFPAview *view  // View to level of interest
    )
{
    bool status;                        // Status of MD lookup

    // Menu of EXTNAME rules
    psMetadata *menu = psMetadataLookupMetadata(&status, file->camera, "EXTNAME.RULES");
    if (!menu) {
        psError(PS_ERR_UNKNOWN, true, "missing EXTNAME.RULES in camera.config");
        return false;
    }

    // EXTNAME for image header
    if (headname) {
        const char *rule = psMetadataLookupStr(&status, menu, "CMF.HEAD");
        if (!rule) {
            psError(PS_ERR_UNKNOWN, true, "missing entry for CMF.HEAD in EXTNAME.RULES in camera.config");
            return false;
        }
        *headname = pmFPAfileNameFromRule(rule, file, view);
    }

    // EXTNAME for PSF table data
    if (dataname) {
        const char *rule = psMetadataLookupStr(&status, menu, "CMF.DATA");
        if (!rule) {
            psError(PS_ERR_UNKNOWN, true, "missing entry for CMF.DATA in EXTNAME.RULES in camera.config");
            return false;
        }
        *dataname = pmFPAfileNameFromRule(rule, file, view);
    }

    // EXTNAME for detection efficiency
    if (deteffname) {
        const char *rule = psMetadataLookupStr(&status, menu, "CMF.DETEFF");
        if (!rule) {
            psError(PS_ERR_UNKNOWN, true, "missing entry for CMF.DETEFF in EXTNAME.RULES in camera.config");
            return false;
        }
        *deteffname = pmFPAfileNameFromRule(rule, file, view);
    }

    // EXTNAME for extended source non-parametric measurements
    if (xsrcname) {
        const char *rule = psMetadataLookupStr(&status, menu, "CMF.XSRC");
        if (!rule) {
            psError(PS_ERR_UNKNOWN, true, "missing entry for CMF.XSRC in EXTNAME.RULES in camera.config");
            return false;
        }
        *xsrcname = pmFPAfileNameFromRule (rule, file, view);
    }

    // EXTNAME for extended source fitted measurements
    if (xfitname) {
        const char *rule = psMetadataLookupStr(&status, menu, "CMF.XFIT");
        if (!rule) {
            psError(PS_ERR_UNKNOWN, true, "missing entry for CMF.XFIT in EXTNAME.RULES in camera.config");
            return false;
        }
        *xfitname = pmFPAfileNameFromRule (rule, file, view);
    }

    // EXTNAME for radial apertures
    if (xradname) {
        const char *rule = psMetadataLookupStr(&status, menu, "CMF.XRAD");
        if (!rule) {
            psError(PS_ERR_UNKNOWN, true, "missing entry for CMF.XRAD in EXTNAME.RULES in camera.config");
            return false;
        }
        *xradname = pmFPAfileNameFromRule (rule, file, view);
    }

    // EXTNAME for radial apertures
    if (xgalname) {
        const char *rule = psMetadataLookupStr(&status, menu, "CMF.XGAL");
        if (!rule) {
            psError(PS_ERR_UNKNOWN, true, "missing entry for CMF.XGAL in EXTNAME.RULES in camera.config");
            return false;
        }
        *xgalname = pmFPAfileNameFromRule (rule, file, view);
    }

    return true;
}


// translations between psphot object types and dophot object types
int pmSourceGetDophotType (pmSource *source)
{
    PS_ASSERT_PTR_NON_NULL(source, -1);

    switch (source->type) {

      case PM_SOURCE_TYPE_DEFECT:
      case PM_SOURCE_TYPE_SATURATED:
        return (8);

      case PM_SOURCE_TYPE_STAR:
        if (source->mode & PM_SOURCE_MODE_SATSTAR)
            return (10);
        if (source->mode & PM_SOURCE_MODE_POOR)
            return (7);
        if (source->mode & PM_SOURCE_MODE_FAIL)
            return (4);
        return (1);

      case PM_SOURCE_TYPE_EXTENDED:
        return (2);

      default:
        return (0);
    }
    return (0);
}

// translations between psphot object types and dophot object types
bool pmSourceSetDophotType (pmSource *source, int type)
{
    PS_ASSERT_PTR_NON_NULL(source, false);

    if (type == 4) {
        source->mode |= PM_SOURCE_MODE_FAIL;
    }
    if (type == 7) {
        source->mode |= PM_SOURCE_MODE_POOR;
    }
    if (type == 10) {
        source->mode |= PM_SOURCE_MODE_SATSTAR;
    }

    switch (type) {
      case 1:
      case 4:
      case 7:
      case 10:
        source->type = PM_SOURCE_TYPE_STAR;
        return true;
      case 2:
        source->type = PM_SOURCE_TYPE_EXTENDED;
        return true;
      case 8:
        source->type = PM_SOURCE_TYPE_DEFECT;
        return true;
      default:
        return false;
    }
    return false;
}

// Given a FITS file pointer, write the table of object data
bool pmFPAviewWriteObjects (const pmFPAview *view, pmFPAfile *file, pmConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(view, false);
    PS_ASSERT_PTR_NON_NULL(file, false);
    PS_ASSERT_PTR_NON_NULL(file->fpa, false);

    pmFPA *fpa = pmFPAfileSuitableFPA(file, view, config, false); // Suitable FPA for writing

    if (view->chip == -1) {
        if (!pmFPAWriteObjects (fpa, view, file, config)) {
            psError(PS_ERR_IO, false, "Failed to write objects from fpa");
            psFree(fpa);
            return false;
        }
        psFree(fpa);
        return true;
    }

    if (view->chip >= fpa->chips->n) {
        psError(PS_ERR_UNKNOWN, false, "Writing chip == %d (>= chips->n == %ld)", view->chip, fpa->chips->n);
        psFree(fpa);
        return false;
    }
    pmChip *chip = fpa->chips->data[view->chip];

    if (view->cell == -1) {
        if (!pmChipWriteObjects (chip, view, file, config)) {
            psError(PS_ERR_IO, false, "Failed to write objects from chip");
            psFree(fpa);
            return false;
        }
        psFree(fpa);
        return true;
    }

    if (view->cell >= chip->cells->n) {
        psError(PS_ERR_UNKNOWN, false, "Writing cell == %d (>= cells->n == %ld)",
                view->cell, chip->cells->n);
        psFree(fpa);
        return false;
    }
    pmCell *cell = chip->cells->data[view->cell];

    if (view->readout == -1) {
        if (!pmCellWriteObjects (cell, view, file, config)) {
            psError(PS_ERR_IO, false, "Failed to write objects from cell");
            psFree(fpa);
            return false;
        }
        psFree(fpa);
        return true;
    }

    if (view->readout >= cell->readouts->n) {
        psError(PS_ERR_UNKNOWN, false, "Writing readout == %d (>= readouts->n == %ld)",
                view->readout, cell->readouts->n);
        psFree(fpa);
        return false;
    }
    pmReadout *readout = cell->readouts->data[view->readout];

    if (!pmReadoutWriteObjects (readout, view, file, config)) {
        psError(PS_ERR_IO, false, "Failed to write objects from readout %d", view->readout);
        psFree(fpa);
        return false;
    }

    psFree(fpa);
    return true;
}

// read in all chip-level Objects files for this FPA
bool pmFPAWriteObjects (pmFPA *fpa, const pmFPAview *view, pmFPAfile *file, const pmConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(view, false);
    PS_ASSERT_PTR_NON_NULL(fpa, false);
    PS_ASSERT_PTR_NON_NULL(fpa->chips, false);

    pmFPAview *thisView = pmFPAviewAlloc (view->nRows);
    *thisView = *view;

    for (int i = 0; i < fpa->chips->n; i++) {
        pmChip *chip = fpa->chips->data[i];
        thisView->chip = i;
        if (!pmChipWriteObjects (chip, thisView, file, config)) {
            psError(PS_ERR_IO, false, "Failed to write %dth chip", i);
            psFree (thisView);
            return false;
        }
    }
    psFree (thisView);
    return true;
}

// read in all cell-level Objects files for this chip
bool pmChipWriteObjects (pmChip *chip, const pmFPAview *view, pmFPAfile *file, const pmConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(chip, false);
    PS_ASSERT_PTR_NON_NULL(chip->cells, false);
    PS_ASSERT_PTR_NON_NULL(view, false);

    pmFPAview *thisView = pmFPAviewAlloc (view->nRows);
    *thisView = *view;

    for (int i = 0; i < chip->cells->n; i++) {
        pmCell *cell = chip->cells->data[i];
        thisView->cell = i;
        if (!pmCellWriteObjects (cell, thisView, file, config)) {
            psError(PS_ERR_IO, false, "Failed to write %dth cell", i);
            psFree (thisView);
            return false;
        }
    }
    psFree (thisView);
    return true;
}

// read in all readout-level Objects files for this cell
bool pmCellWriteObjects (pmCell *cell, const pmFPAview *view, pmFPAfile *file, const pmConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(cell, false);
    PS_ASSERT_PTR_NON_NULL(cell->readouts, false);
    PS_ASSERT_PTR_NON_NULL(view, false);

    pmFPAview *thisView = pmFPAviewAlloc (view->nRows);
    *thisView = *view;

    for (int i = 0; i < cell->readouts->n; i++) {
        pmReadout *readout = cell->readouts->data[i];
        thisView->readout = i;
        if (!pmReadoutWriteObjects (readout, thisView, file, config)) {
            psError(PS_ERR_IO, false, "Failed to write %dth readout", i);
            psFree (thisView);
            return false;
        }
    }
    psFree (thisView);
    return true;
}

# define PM_SOURCES_WRITE(NAME,TYPE)					\
    if (!strcmp (exttype, NAME)) {					\
	status = pmSourcesWrite_##TYPE(file->fits, readout, sources, file->header, outhead, dataname, recipe); \
	if (xsrcname) {							\
	    status &= pmSourcesWrite_##TYPE##_XSRC(file->fits, readout, sources, file->header, xsrcname, recipe); \
	}								\
	if (xfitname) {							\
	    status &= pmSourcesWrite_##TYPE##_XFIT (file->fits, readout, sources, file->header, xfitname); \
	}								\
	if (xradname) {							\
	    status &= pmSourcesWrite_##TYPE##_XRAD (file->fits, readout, sources, file->header, xradname, recipe); \
	}								\
	if (xgalname) {							\
	    status &= pmSourcesWrite_##TYPE##_XGAL (file->fits, readout, sources, xgalname, recipe); \
	}								\
    }

// write out all readout-level Objects files for this cell
bool pmReadoutWriteObjects (pmReadout *readout, const pmFPAview *view, pmFPAfile *file, const pmConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(readout, false);
    PS_ASSERT_PTR_NON_NULL(view, false);
    PS_ASSERT_PTR_NON_NULL(file, false);

    pmCell *cell = readout->parent;
    PS_ASSERT_PTR_NON_NULL(cell, false);
    pmChip *chip = cell->parent;
    PS_ASSERT_PTR_NON_NULL(chip, false);
    pmFPA *fpa = chip->parent;
    PS_ASSERT_PTR_NON_NULL(fpa, false);

    bool status;
    pmHDU *hdu;
    psMetadata *updates;

    // if sources is NULL, write out an empty table
    // input / output sources are stored on the readout->analysis as "PSPHOT.DETECTIONS"

    psArray *sources = NULL;
    pmDetections *detections = psMetadataLookupPtr (&status, readout->analysis, "PSPHOT.DETECTIONS");
    if (detections) {
	sources = detections->allSources;
    }
    if (!sources) {
	detections = pmDetectionsAlloc();
        sources = psArrayAlloc(0);
	detections->allSources = sources;
        psMetadataAddPtr(readout->analysis, PS_LIST_TAIL, "PSPHOT.DETECTIONS", PS_DATA_UNKNOWN | PS_META_REPLACE, "Blank array of sources", detections);
        psFree(detections); // Held onto by the metadata, so we can continue to use
    }

    // the older types (RAW, OBJ, SX, CMP) are for backwards compatibility -- deprecate eventually?
    switch (file->type) {
      case PM_FPA_FILE_RAW:
        pmSourcesWriteRAW (sources, file->filename);
        break;

      case PM_FPA_FILE_OBJ:
        pmSourcesWriteOBJ (sources, file->filename);
        break;

      case PM_FPA_FILE_SX:
        pmSourcesWriteSX (sources, file->filename);
        break;

      case PM_FPA_FILE_CMP: {
	  // a SPLIT format : only one header and object table per file
	  hdu = pmFPAviewThisHDU (view, fpa);
	  if (!hdu) {
	      psError(PS_ERR_UNEXPECTED_NULL, true, "Unable to find HDU to write sources.");
	      return false;
	  }

	  // copy the header to an output header, add the output header data
	  psMetadata *outhead = psMetadataCopy (NULL, hdu->header);

	  // copy over the entries saved by PSPHOT
	  updates = psMetadataLookupPtr (NULL, readout->analysis, "PSPHOT.HEADER");
	  if (updates) {
	      psMetadataCopy (outhead, updates);
	  }

	  // copy over the entries saved by PSASTRO
	  updates = psMetadataLookupPtr (NULL, readout->analysis, "PSASTRO.HEADER");
	  if (updates) {
	      psMetadataCopy (outhead, updates);
	  }

	  bool status = pmSourcesWriteCMP (sources, file->filename, outhead);
	  psFree (outhead);

	  if (!status) {
	      psError(PS_ERR_IO, false, "Failed to write CMP file\n");
	      return false;
	  }
	  break;
      }

      case PM_FPA_FILE_CMF: 
        // write a header? (only if this is the first readout for cell)
        //   note that the file->header is set to track the last hdu->header written
        // write the data? (always?)

        // get the current header
        hdu = pmFPAviewThisHDU (view, fpa);
        if (!hdu) {
            psError(PS_ERR_UNEXPECTED_NULL, true, "Unable to find HDU to write sources.");
            return false;
        }

        // determine the output table format
        psMetadata *recipe = psMetadataLookupMetadata(&status, config->recipes, "PSPHOT");
        if (!status) {
	    psError(PS_ERR_UNKNOWN, true, "missing recipe PSPHOT in config data");
	    return false;
        }

        // if none of these are TRUE, the output files only contain the psf measurements.
	bool doPetrosian = psMetadataLookupBool (&status, recipe, "EXTENDED_SOURCE_PETROSIAN");
	bool doAnnuli    = psMetadataLookupBool (&status, recipe, "EXTENDED_SOURCE_ANNULI");
        bool XSRC_OUTPUT = doPetrosian || doAnnuli;
        bool XFIT_OUTPUT = psMetadataLookupBool(&status, recipe, "EXTENDED_SOURCE_FITS");
        bool XRAD_OUTPUT = psMetadataLookupBool(&status, recipe, "RADIAL_APERTURES");
        bool XGAL_OUTPUT = psMetadataLookupBool(&status, recipe, "GALAXY_SHAPES");

        // define the EXTNAME values for the different data segments:
        psString headname = NULL;
        psString dataname = NULL;
        psString deteffname = NULL;
        psString xsrcname = NULL;
        psString xfitname = NULL;
        psString xradname = NULL;
        psString xgalname = NULL;
        if (!pmSourceIOextnames(&headname, &dataname, &deteffname, 
				XSRC_OUTPUT ? &xsrcname : NULL,
				XFIT_OUTPUT ? &xfitname : NULL, 
				XRAD_OUTPUT ? &xradname : NULL, 
				XGAL_OUTPUT ? &xgalname : NULL, 
				file, view)) {
            return false;
        }

        // write out the IMAGE header segment (only for the first readout of the cell)
        {
            // this header block is new, write it to disk
            if (hdu->header != file->header) {
                // add EXTNAME, EXTHEAD, EXTTYPE to header
                psMetadataAddStr (hdu->header, PS_LIST_TAIL, "EXTDATA", PS_META_REPLACE, "name of table extension", dataname);
                psMetadataAddStr (hdu->header, PS_LIST_TAIL, "EXTTYPE", PS_META_REPLACE, "extension type", "IMAGE");
                if (!file->wrote_phu) {
                    // this hdu->header acts as the PHU: set EXTEND to be true
                    psMetadataAddBool (hdu->header, PS_LIST_TAIL, "EXTEND", PS_META_REPLACE, "this file has extensions", true);
                    file->wrote_phu = true;
                }

                // save psphot and psastro metadata in the image and table headers
                updates = psMetadataLookupPtr (&status, readout->analysis, "PSPHOT.HEADER");
                if (updates) {
                    psMetadataCopy (hdu->header, updates);
                }
                updates = psMetadataLookupPtr (&status, readout->analysis, "PSASTRO.HEADER");
                if (updates) {
                    psMetadataCopy (hdu->header, updates);
                }

                pmConfigConformHeader(hdu->header, file->format);

                // psFitsWriteBlank strips out the NAXISn keywords, forcing CFITSIO to take care of them
                // save NAXIS1,NAXIS2 as IMNAXIS1,IMNAXIS2
                int numCols = 0, numRows = 0; // Size of image
                if (readout->image) {
                    numCols = readout->image->numCols;
                    numRows = readout->image->numRows;
                } else {
                    numCols = psMetadataLookupS32(&status, hdu->header, "IMNAXIS1");
                    if (!status) {
                        numCols = psMetadataLookupS32(&status, hdu->header, "NAXIS1");
                    }
                    numRows = psMetadataLookupS32(&status, hdu->header, "IMNAXIS2");
                    if (!status) {
                        numRows = psMetadataLookupS32(&status, hdu->header, "NAXIS2");
                    }
                }
                if (numCols == 0 || numRows == 0) {
                    psWarning("Output source file has invalid IMNAXIS1, IMNAXIS2.");
                }
                psMetadataAddS32(hdu->header, PS_LIST_TAIL, "IMNAXIS1", PS_META_REPLACE,
                                 "Number of columns in original image", numCols);
                psMetadataAddS32(hdu->header, PS_LIST_TAIL, "IMNAXIS2", PS_META_REPLACE,
                                 "Number of rows in original image", numRows);

                psFitsWriteBlank (file->fits, hdu->header, headname);
                psTrace ("pmFPAfile", 5, "wrote ext head %s (type: %d)\n", file->filename, file->type);
                file->header = hdu->header;
            }
        }

        // write out the Object TABLE data segment(s)
        {
            // create a header to hold the output data
            psMetadata *outhead = psMetadataAlloc ();
	    
	    char *exttype = psMemIncrRefCounter (psMetadataLookupStr(&status, recipe, "OUTPUT.FORMAT"));
            if (!exttype) {
                exttype = psStringCopy ("SMPDATA");
            }

            // write the links to the image header
            psMetadataAddStr (outhead, PS_LIST_TAIL, "EXTHEAD", PS_META_REPLACE, "name of image extension w/", headname);
            psMetadataAddStr (outhead, PS_LIST_TAIL, "EXTTYPE", PS_META_REPLACE, "extension type", exttype);

            // if we request XSRC output, add the XSRC name to this header
            if (xsrcname) {
		psMetadataAddStr (outhead, PS_LIST_TAIL, "XSRCNAME", PS_META_REPLACE, "name of XSRC table extension", xsrcname);
            }
            if (xfitname) {
		psMetadataAddStr (outhead, PS_LIST_TAIL, "XFITNAME", PS_META_REPLACE, "name of XFIT table extension", xfitname);
            }
            if (xradname) {
		psMetadataAddStr (outhead, PS_LIST_TAIL, "XRADNAME", PS_META_REPLACE, "name of XRAD table extension", xradname);
            }
            if (xgalname) {
		psMetadataAddStr (outhead, PS_LIST_TAIL, "XGALNAME", PS_META_REPLACE, "name of XGAL table extension", xgalname);
            }

            // these are case-sensitive since the EXTYPE is case-sensitive
            status = false;
	    PM_SOURCES_WRITE("SMPDATA",   SMPDATA);
	    PM_SOURCES_WRITE("PS1_DEV_0", PS1_DEV_0);
	    PM_SOURCES_WRITE("PS1_DEV_1", PS1_DEV_1);
	    PM_SOURCES_WRITE("PS1_CAL_0", PS1_CAL_0);
	    PM_SOURCES_WRITE("PS1_V1",    CMF_PS1_V1);
	    PM_SOURCES_WRITE("PS1_V2",    CMF_PS1_V2);
	    PM_SOURCES_WRITE("PS1_V3",    CMF_PS1_V3);
	    PM_SOURCES_WRITE("PS1_V4",    CMF_PS1_V4);
	    PM_SOURCES_WRITE("PS1_V5",    CMF_PS1_V5);
	    PM_SOURCES_WRITE("PS1_SV1",   CMF_PS1_SV1);
	    PM_SOURCES_WRITE("PS1_SV2",   CMF_PS1_SV2);
	    PM_SOURCES_WRITE("PS1_SV3",   CMF_PS1_SV3);
	    PM_SOURCES_WRITE("PS1_SV4",   CMF_PS1_SV4);
	    PM_SOURCES_WRITE("PS1_DV1",   CMF_PS1_DV1);
	    PM_SOURCES_WRITE("PS1_DV2",   CMF_PS1_DV2);
	    PM_SOURCES_WRITE("PS1_DV3",   CMF_PS1_DV3);
	    PM_SOURCES_WRITE("PS1_DV4",   CMF_PS1_DV4);
	    PM_SOURCES_WRITE("PS1_DV5",   CMF_PS1_DV5);

	    psFree (outhead);
	    psFree (exttype);

            if (!status) {
                psError(PS_ERR_IO, false, "writing CMF data to %s with format %s\n", file->filename, exttype);
		goto escape;
            }
        }

	// write out the detection efficiency TABLE segments
	if (deteffname) {
            // create a header to hold the output data
            psMetadata *outhead = psMetadataAlloc ();
            psMetadataAddStr (outhead, PS_LIST_TAIL, "EXTHEAD", PS_META_REPLACE, "name of image extension w/", headname);
	    psMetadataAddStr (outhead, PS_LIST_TAIL, "EXTTYPE", PS_META_REPLACE, "extension type", "DETEFF");

	    status = pmReadoutWriteDetEff(file->fits, readout, outhead, deteffname);
	    psFree (outhead);

            if (!status) {
                psError(PS_ERR_IO, false, "writing DETEFF data to %s\n", file->filename);
		goto escape;
            }
	}
	psFree (headname);
	psFree (dataname);
	psFree (xsrcname);
	psFree (xfitname);
	psFree (xradname);
	psFree (xgalname);
	psFree (deteffname);

        psTrace ("pmFPAfile", 5, "wrote ext data %s (type: %d)\n", file->filename, file->type);
        break;

      escape:
	psFree (headname);
	psFree (dataname);
	psFree (xsrcname);
	psFree (xfitname);
	psFree (xradname);
	psFree (xgalname);
	psFree (deteffname);
	return false;

      case PM_FPA_FILE_CFF: {
        // determine the output table format
        psMetadata *recipe = psMetadataLookupMetadata(&status, config->recipes, "PSPHOT");
        if (!status) {
	    psError(PS_ERR_UNKNOWN, true, "missing recipe PSPHOT in config data");
	    return false;
        }

        hdu = pmFPAviewThisHDU (view, fpa);
        pmConfigConformHeader(hdu->header, file->format);
        psFitsWriteBlank (file->fits, hdu->header, NULL);
        file->header = hdu->header;
        file->wrote_phu = true;
	if (!pmSourcesWrite_CFF(readout, file->fits, sources, hdu->header, recipe)) {
            psError(PS_ERR_UNKNOWN, false, "failed to write CFF");
            return false;
        }
        break;
      }
        
      default:
        fprintf (stderr, "warning: type mismatch\n");
        break;
    }
    return true;

}
// a MEF CMF file has: PHU, CELL-HEAD, TABLE, CELL-HEAD, TABLE, TABLE, TABLE...

// if this file needs to have a PHU written out, write one
bool pmSource_CMF_WritePHU (const pmFPAview *view, pmFPAfile *file, pmConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(view, false);
    PS_ASSERT_PTR_NON_NULL(file, false);

    bool status;

    // not needed if already written
    if (file->wrote_phu) return true;

    // not needed if not FPA
    // XXX this prevents us from defining a SPLIT/MEF CMF file...
    if (file->fileLevel != PM_FPA_LEVEL_FPA) return true;

    // not needed if only one chip
    if (file->fpa->chips->n == 1) {
        pmSourceIO_WriteMatchedRefs (file->fits, file->fpa, config);
        return true;
    }

    // find the FPA phu
    pmFPA *fpa = pmFPAfileSuitableFPA(file, view, config, false); // Suitable FPA for writing
    pmHDU *phu = psMemIncrRefCounter(pmFPAviewThisPHU(view, fpa));

    // if there is no PHU, this is a single header+image (extension-less) file. This could be
    // the case for an input SPLIT set of files being written out as a MEF.  if there is a PHU,
    // write it out as a 'blank'
    psMetadata *outhead = psMetadataAlloc();
    if (phu) {
        psMetadataCopy (outhead, phu->header);
    }
    psFree(phu);

    pmConfigConformHeader (outhead, file->format);

    // We need to get some FPA-level concepts in there
    // This is a hack, not very pretty.  But then, so is writing the FPA in this manner without
    // using the pmFPAMosaic functions....
    // XXX why are these not correctly inserted by pmConfigConformHeader??

    // Because these concepts are not part of the "RULE" in the camera format, pmConfigConformHeader only adds
    // what is required by the "RULE".
    // Though we can configure Ohana to look at particular header keywords, it doesn't like having the
    // important values in "HIERARCH FPA.TIME", etc, as is done for skycells, so we stoop to its level,
    // putting in additional header keywords that it can understand.

    psMetadata *headers = psMetadataLookupMetadata(NULL, fpa->camera, BLANK_HEADERS); // Header names
    if (!headers) {
        psError(PS_ERR_UNEXPECTED_NULL, false,
                "Unable to find %s metadata within camera configuration", BLANK_HEADERS);
        psFree(outhead);
        psFree(fpa);
        return false;
    }

    {
        const char *mjdName = psMetadataLookupStr(NULL, headers, "FPA.TIME"); // Header name
        if (!mjdName || strlen(mjdName) == 0) {
            psError(PS_ERR_UNEXPECTED_NULL, false,
                    "Unable to find FPA.TIME in %s within camera configuration.", BLANK_HEADERS);
            psFree(outhead);
            psFree(fpa);
            return false;
        }
        psTime *time = psMetadataLookupTime(NULL, fpa->concepts, "FPA.TIME"); // Time of observation
        double mjd = psTimeToMJD(time); // The MJD of observation
        psMetadataAddF64(outhead, PS_LIST_TAIL, mjdName, PS_META_REPLACE,
                         "Time of observation", mjd);
    }

    {
        const char *expName = psMetadataLookupStr(NULL, headers, "FPA.EXPOSURE"); // Header name
        if (!expName || strlen(expName) == 0) {
            psError(PS_ERR_UNEXPECTED_NULL, false,
                    "Unable to find FPA.EXPOSURE in %s within camera configuration.",
                    BLANK_HEADERS);
            psFree(outhead);
            psFree(fpa);
            return false;
        }
        float exptime = psMetadataLookupF32(NULL, fpa->concepts, "FPA.EXPOSURE"); // Exposure time
        psMetadataAddF32(outhead, PS_LIST_TAIL, expName, PS_META_REPLACE,
                         "Exposure time (sec)", exptime);
    }

    {
        const char *amName = psMetadataLookupStr(NULL, headers, "FPA.AIRMASS"); // Header name
        if (!amName || strlen(amName) == 0) {
            psError(PS_ERR_UNEXPECTED_NULL, false,
                    "Unable to find FPA.AIRMASS in %s within camera configuration.",
                    BLANK_HEADERS);
            psFree(outhead);
            psFree(fpa);
            return false;
        }
        float airmass = psMetadataLookupF32(NULL, fpa->concepts, "FPA.AIRMASS"); // Airmass
        psMetadataAddF32(outhead, PS_LIST_TAIL, amName, PS_META_REPLACE,
                         "Observation airmass", airmass);
    }

    psMetadata *fileData = psMetadataLookupMetadata(NULL, file->format, "FILE"); // File information
    const char *fpaNameHdr = psMetadataLookupStr(&status, fileData, "FPA.OBS");
    if (fpaNameHdr && strlen(fpaNameHdr) > 0) {
        const char *fpaName = psMetadataLookupStr(&status, fpa->concepts, "FPA.OBS");
	if (fpaName) {
	    psMetadataAddStr(outhead, PS_LIST_TAIL, fpaNameHdr, PS_META_REPLACE, "FPA observation identifier", fpaName);
	}
    }

    // if we have mosaic-level astrometry information, add it here:
    psMetadata *updates = psMetadataLookupPtr (&status, fpa->analysis, "PSASTRO.HEADER");
    if (updates) {
        psMetadataCopy (outhead, updates);
    }
    psFree(fpa);

    psMetadataAddBool (outhead, PS_LIST_TAIL, "EXTEND", PS_META_REPLACE, "this file has extensions", true);
    psFitsWriteBlank (file->fits, outhead, "");
    file->wrote_phu = true;

    psTrace ("pmFPAfile", 5, "wrote phu %s (type: %d)\n", file->filename, file->type);
    psFree (outhead);

    pmSourceIO_WriteMatchedRefs (file->fits, file->fpa, config);
    pmSourceIO_WriteGlints (file->fits, file->fpa, config);
    pmSourceIO_WriteGhosts (file->fits, file->fpa, config);
    return true;
}

// Given a FITS file pointer, read the table of object data
// XXX add in error handling
bool pmFPAviewReadObjects (const pmFPAview *view, pmFPAfile *file, const pmConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(view, false);
    PS_ASSERT_PTR_NON_NULL(file, false);
    PS_ASSERT_PTR_NON_NULL(file->fpa, false);

    pmFPA *fpa = file->fpa;

    pmSourceIO_ReadMatchedRefs (file->fits, fpa, config);

    if (view->chip == -1) {
        pmFPAReadObjects (fpa, view, file, config);
        return true;
    }

    if (view->chip >= fpa->chips->n) {
        return false;
    }
    pmChip *chip = fpa->chips->data[view->chip];

    if (view->cell == -1) {
        pmChipReadObjects (chip, view, file, config);
        return true;
    }

    if (view->cell >= chip->cells->n) {
        return false;
    }
    pmCell *cell = chip->cells->data[view->cell];

    if (view->readout == -1) {
        pmCellReadObjects (cell, view, file, config);
        return true;
    }

    if (view->readout >= cell->readouts->n) {
        return false;
    }
    pmReadout *readout = cell->readouts->data[view->readout];

    pmReadoutReadObjects (readout, view, file, config);
    return true;
}

// read in all chip-level Objects files for this FPA
bool pmFPAReadObjects (pmFPA *fpa, const pmFPAview *view, pmFPAfile *file, const pmConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(view, false);
    PS_ASSERT_PTR_NON_NULL(file, false);
    PS_ASSERT_PTR_NON_NULL(file->fpa, false);
    PS_ASSERT_PTR_NON_NULL(file->fpa->chips, false);

    pmFPAview *thisView = pmFPAviewAlloc (view->nRows);
    *thisView = *view;

    for (int i = 0; i < fpa->chips->n; i++) {
        pmChip *chip = fpa->chips->data[i];
        thisView->chip = i;
        pmChipReadObjects (chip, thisView, file, config);
    }
    psFree (thisView);

    if (!pmConceptsReadFPA(fpa, PM_CONCEPT_SOURCE_HEADER, true, NULL)) {
        psError(PS_ERR_IO, false, "Failed to read concepts for fpa.\n");
        return false;
    }

    return true;
}

// read in all cell-level Objects files for this chip
bool pmChipReadObjects (pmChip *chip, const pmFPAview *view, pmFPAfile *file, const pmConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(view, false);
    PS_ASSERT_PTR_NON_NULL(chip, false);
    PS_ASSERT_PTR_NON_NULL(chip->cells, false);

    pmFPAview *thisView = pmFPAviewAlloc (view->nRows);
    *thisView = *view;

    chip->data_exists = false;
    for (int i = 0; i < chip->cells->n; i++) {
        pmCell *cell = chip->cells->data[i];
        thisView->cell = i;
        pmCellReadObjects (cell, thisView, file, config);
        if (!cell->data_exists) continue;
        chip->data_exists = true;
    }
    psFree (thisView);

    if (!pmConceptsReadChip(chip, PM_CONCEPT_SOURCE_HEADER, true, true, NULL)) {
        psError(PS_ERR_IO, false, "Failed to read concepts for chip.\n");
        return false;
    }

    return true;
}

// read in all readout-level Objects files for this cell
bool pmCellReadObjects (pmCell *cell, const pmFPAview *view, pmFPAfile *file, const pmConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(view, false);
    PS_ASSERT_PTR_NON_NULL(cell, false);
    PS_ASSERT_PTR_NON_NULL(cell->readouts, false);

    pmFPAview *thisView = pmFPAviewAlloc (view->nRows);
    *thisView = *view;

    // multiple readout mode is not yet defined for CMP or CMF files
    // if they have not been allocated, allocate a single readout
    if (!cell->readouts || !cell->readouts->n) {
        pmReadout *readout = pmReadoutAlloc (cell);
        psFree (readout);
    }

    cell->data_exists = false;
    for (int i = 0; i < cell->readouts->n; i++) {
        pmReadout *readout = cell->readouts->data[i];
        thisView->readout = i;
        pmReadoutReadObjects (readout, thisView, file, config);
        if (!readout->data_exists) {
            continue;
        }

        // load in the concept information for this cell
        if (!pmConceptsReadCell(cell, PM_CONCEPT_SOURCE_HEADER, true, NULL)) {
            //psError(PS_ERR_UNKNOWN, false, "Failed to read concepts for cell");
            //return false;
            psWarning("Difficulty reading concepts for cell; attempting to proceed.");
        }
        cell->data_exists = true;
    }
    psFree (thisView);

    if (!pmConceptsReadCell(cell, PM_CONCEPT_SOURCE_HEADER, true, NULL)) {
        //psError(PS_ERR_UNKNOWN, false, "Failed to read concepts for cell");
        //return false;
        psWarning("Difficulty reading concepts for cell; attempting to proceed.");
    }

    return true;
}

// read in all readout-level Objects files for this cell
bool pmReadoutReadObjects (pmReadout *readout, const pmFPAview *view, pmFPAfile *file, const pmConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(view, false);
    PS_ASSERT_PTR_NON_NULL(file, false);

    bool status;
    psArray *sources = NULL;
    pmHDU *hdu;

    // define the EXTNAME values for the different data segments:
    psString headname = NULL;
    psString dataname = NULL;
    psString deteffname = NULL;
    psString xsrcname = NULL;
    psString xfitname = NULL;
    psString xradname = NULL;
    psString xgalname = NULL;

    psMetadata *tableHeader = NULL;
    char *xtension = NULL;

    switch (file->type) {
      case PM_FPA_FILE_OBJ:
        psError(PS_ERR_UNKNOWN, true, "OBJ is not supported as an input object format");
        return false;

      case PM_FPA_FILE_SX:
        psError(PS_ERR_UNKNOWN, true, "SX is not supported as an input object format");
        return false;

      case PM_FPA_FILE_CMP:
        // a SPLIT format : only one header and object table per file

        // read in header, if not yet loaded
        hdu = pmFPAviewThisHDU (view, file->fpa);
#if 0
        if (file->filename)
            psFree (file->filename);
        file->filename = pmFPAfileNameFromRule (file->filerule, file, view);
#endif

        // indirect filenames
        if (!strcasecmp (file->filename, "@FILES")) {
            psFree (file->filename);
            char *filesrc = pmFPAfileNameFromRule (file->filesrc, file, view);
            file->filename = psMetadataLookupStr (&status, file->names, filesrc);
            psFree (filesrc);
            if (file->filename == NULL) return false;
            // psMetadataLookupStr just returns a view, file->filename must be protected
            psMemIncrRefCounter (file->filename);
        }

        // read the PHU from this file
        file->fits = psFitsOpen (file->filename, "r");
        if (hdu->header != NULL) {
            psFree (hdu->header);
        }
        hdu->header = psFitsReadHeader (NULL, file->fits);
        psFitsClose (file->fits);
        file->fits = NULL;

        sources = pmSourcesReadCMP (file->filename, hdu->header);
        break;

      case PM_FPA_FILE_CMF:
      case PM_FPA_FILE_WCS:  // "WCS" is CMF without detected objects
        // read in header, if not yet loaded
        hdu = pmFPAviewThisHDU (view, file->fpa);

        // determine the output table format. Assume if we need to output extendend source
        // parameters that they may exist in the input. 
        // XXX: Perhaps we should use different recipe values.
        // I.E. EXTENDED_SOURCE_ANALYSIS_READ or something like that
        psMetadata *recipe = psMetadataLookupMetadata(&status, config->recipes, "PSPHOT");
        if (!status) {
	    psError(PS_ERR_UNKNOWN, true, "missing recipe PSPHOT in config data");
	    return false;
        }

        // if none of these are TRUE, we only read the psf measurements
        // XXX: shouldn't we look for these extensions and read the regardless of the recipe values?
	bool doPetrosian = psMetadataLookupBool (&status, recipe, "EXTENDED_SOURCE_PETROSIAN");
	bool doAnnuli    = psMetadataLookupBool (&status, recipe, "EXTENDED_SOURCE_ANNULI");
        bool XSRC_OUTPUT = doPetrosian || doAnnuli;
        bool XFIT_OUTPUT = psMetadataLookupBool(&status, recipe, "EXTENDED_SOURCE_FITS");
        bool XRAD_OUTPUT = psMetadataLookupBool(&status, recipe, "RADIAL_APERTURES");
        bool XGAL_OUTPUT = psMetadataLookupBool(&status, recipe, "GALAXY_SHAPES");

        if (!pmSourceIOextnames(&headname, &dataname, &deteffname, 
                XSRC_OUTPUT ? &xsrcname : NULL, 
                XFIT_OUTPUT ? &xfitname : NULL, 
                XRAD_OUTPUT ? &xradname : NULL,
                XGAL_OUTPUT ? &xgalname : NULL,
                file, view)) {
            return false;
        }

        bool saveSourcesHeader = psMetadataLookupBool(&status, recipe, "SAVE.INPUT.SOURCES.HEADER");;
        // advance to the IMAGE HEADER extension
        if (saveSourcesHeader || hdu->header == NULL) {
            // if the IMAGE header does not exist, we have no data for this view
            if (!psFitsMoveExtNameClean (file->fits, headname)) {
                if (hdu->header == NULL) {
                    readout->data_exists = false;
                }
                psFree (headname);
                psFree (dataname);
                psFree (deteffname);
                return true;
            }
            psMetadata *sourcesHeader = psFitsReadHeader (NULL, file->fits);
            // Save the hdu header for the sources on readout->analysis. 
            // psphotStack uses this in updateMode
            psMetadataAddPtr(readout->analysis, PS_LIST_TAIL, "INPUT.SOURCES.HEADER", 
                PS_DATA_UNKNOWN | PS_META_REPLACE, "fits header from input sources file", sourcesHeader);
            if (hdu->header == NULL) {
                hdu->header = sourcesHeader;
            } else {
                psFree(sourcesHeader);
            }
        }

	// "WCS" is CMF without detected objects
	if (file->type == PM_FPA_FILE_WCS) {
	  psTrace("psModules.objects", 6, "read CMF table from %s : %s : %s", file->filename, headname, dataname);
	  psFree (headname);
	  psFree (dataname);
	  psFree (deteffname);
	  break;
	}

	// EXTDATA is the PSF data associated with this image header

        // we need to find the corresponding table EXTNAME.
        // first check the header
        char *extdata = psMetadataLookupStr (&status, hdu->header, "EXTDATA");
        if (extdata) {
            // if EXTDATA is defined in the header, use that value for 'dataname'
            psFree (dataname);
            dataname = psMemIncrRefCounter (extdata);
        }

        // advance to the table data extension
        // since we have read the IMAGE header, the TABLE header should exist
        if (!psFitsMoveExtName (file->fits, dataname)) {
            psAbort("cannot find data extension %s in %s", dataname, file->filename);
        }

        tableHeader = psFitsReadHeader(NULL, file->fits); // The FITS header
        if (!tableHeader) psAbort("cannot read table header");

        xtension = psMetadataLookupStr (NULL, tableHeader, "XTENSION");
        if (!xtension) psAbort("cannot read table type");
	if (strcmp (xtension, "BINTABLE")) {
	    psWarning ("no binary table in extension %s, skipping\n", dataname);
            psFree(tableHeader);
	    return false;
	}

        char *exttype = psMetadataLookupStr (NULL, tableHeader, "EXTTYPE");
        if (!exttype) psAbort("cannot read table type");

# define PM_SOURCES_READ_PSF(NAME,TYPE)				 \
	if (!strcmp (exttype, NAME)) {				 \
	    sources = pmSourcesRead_##TYPE(file->fits, hdu->header);	\
	}									

        // XXX these are case-sensitive since the EXTYPE is case-sensitive
        if (file->type == PM_FPA_FILE_CMF) {
	    PM_SOURCES_READ_PSF("SMPDATA",   SMPDATA);
	    PM_SOURCES_READ_PSF("PS1_DEV_0", PS1_DEV_0);
	    PM_SOURCES_READ_PSF("PS1_DEV_1", PS1_DEV_1);
	    PM_SOURCES_READ_PSF("PS1_CAL_0", PS1_CAL_0);
	    PM_SOURCES_READ_PSF("PS1_V1",    CMF_PS1_V1);
	    PM_SOURCES_READ_PSF("PS1_V2",    CMF_PS1_V2);
	    PM_SOURCES_READ_PSF("PS1_V3",    CMF_PS1_V3);
	    PM_SOURCES_READ_PSF("PS1_V4",    CMF_PS1_V4);
	    PM_SOURCES_READ_PSF("PS1_V5",    CMF_PS1_V5);
	    PM_SOURCES_READ_PSF("PS1_SV1",   CMF_PS1_SV1);
	    PM_SOURCES_READ_PSF("PS1_SV2",   CMF_PS1_SV2);
	    PM_SOURCES_READ_PSF("PS1_SV3",   CMF_PS1_SV3);
	    PM_SOURCES_READ_PSF("PS1_SV4",   CMF_PS1_SV4);
	    PM_SOURCES_READ_PSF("PS1_DV1",   CMF_PS1_DV1);
	    PM_SOURCES_READ_PSF("PS1_DV2",   CMF_PS1_DV2);
	    PM_SOURCES_READ_PSF("PS1_DV3",   CMF_PS1_DV3);
	    PM_SOURCES_READ_PSF("PS1_DV4",   CMF_PS1_DV4);
	    PM_SOURCES_READ_PSF("PS1_DV5",   CMF_PS1_DV5);

            if (!sources) {
                psError(PS_ERR_IO, false, "reading CMF data from %s with format %s\n", file->filename, exttype);
		return false;
            }

            long *sourceIndex = NULL;
            if (XSRC_OUTPUT || XFIT_OUTPUT || XRAD_OUTPUT || XGAL_OUTPUT) {
                // Build sourceIndex. Lookup table from source->seq to index in sources array.
                // Consists of an array of length max(source->seq) + 1.

                // find maximum sequence number
                long seq_max = -1;
                for (long i = sources->n -1; i >= 0; i--) {
                    pmSource *source = sources->data[i];
                    if (source->seq < 0) {
                        // This can happen cmf files that have been corrupted
                        psError(PS_ERR_IO, true, "seq < 0 for source %ld: Suspect %s is corrupt", i, file->origname);
                        return false;
                    }
                    if (source->seq > seq_max) {
                        seq_max = source->seq;
                    }
                }
                // allocate and initialize the index
                sourceIndex = psAlloc((seq_max + 1) * sizeof(long));
                for (long i = 0; i < seq_max; i++) {
                    sourceIndex[i] = -1;
                }
                // populate the index
                for (long i = 0; i < sources->n; i++) {
                    pmSource *source = sources->data[i];
                    sourceIndex[source->seq] = i;
                }
            }
            if (XSRC_OUTPUT && xsrcname) {
		// a cmf file may have an XSRC extension, but it is not required
                if (!pmReadoutReadXSRC(file, readout, exttype, hdu->header, xsrcname, sources, sourceIndex)) {
		    // do anything?
                }
                psFree(xsrcname);
            }
            if (XFIT_OUTPUT && xfitname) {
		// a cmf file may have an XFIT extension, but it is not required
                if (!pmReadoutReadXFIT(file, readout, exttype, hdu->header, xfitname, sources, sourceIndex)) {
		    // do anything?
                }
                psFree(xfitname);
            }
            if (XRAD_OUTPUT && xradname) {
		// a cmf file may have an XRAD extension, but it is not required
                if (!pmReadoutReadXRAD(file, readout, exttype, hdu->header, xradname, sources, sourceIndex)) {
		    // do anything?
                }
                psFree(xradname);
            }
            if (XGAL_OUTPUT && xgalname) {
		// a cmf file may have an XGAL extension, but it is not required
                if (!pmReadoutReadXGAL(file, readout, exttype, hdu->header, xgalname, sources, sourceIndex)) {
		    // do anything?
                }
                psFree(xgalname);
            }
            psFree(sourceIndex);

            if (!pmReadoutReadDetEff(file->fits, readout, deteffname)) {
#if 0
                psError(PS_ERR_IO, false, "Unable to read detection efficiency");
                return false;
#else
                // No great loss
                psErrorClear();
#endif
            }
        }

        psTrace("psModules.objects", 6, "read CMF table from %s : %s : %s", file->filename, headname, dataname);
        psFree (headname);
        psFree (dataname);
	psFree (deteffname);
        psFree (tableHeader);
        break;

      case PM_FPA_FILE_CFF: {
        // determine the output table format
        psMetadata *recipe = psMetadataLookupMetadata(&status, config->recipes, "PSPHOT");
        if (!status) {
	    psError(PS_ERR_UNKNOWN, true, "missing recipe PSPHOT in config data");
	    return false;
        }
        // read in header, if not yet loaded
        hdu = pmFPAviewThisHDU (view, file->fpa);

	// look these up in the camera config?
	// headrule = {CHIP.NAME}.hdr
	// datarule = {CHIP.NAME}.cff

        // define the EXTNAME values for the different data segments:
        headname = pmFPAfileNameFromRule("{CHIP.NAME}.hdr", file, view);
        dataname = pmFPAfileNameFromRule("{CHIP.NAME}.cff", file, view);

        // advance to the IMAGE HEADER extension
        if (hdu->header == NULL) {
            // if the IMAGE header does not exist, we have no data for this view
            if (!psFitsMoveExtNameClean (file->fits, headname)) {
                readout->data_exists = false;
                psFree (headname);
                psFree (dataname);
                return true;
            }
            hdu->header = psFitsReadHeader (NULL, file->fits);
        }

        // advance to the table data extension
        // since we have read the IMAGE header, the TABLE header should exist
        if (!psFitsMoveExtName (file->fits, dataname)) {
            psAbort("cannot find data extension %s in %s", dataname, file->filename);
        }

        tableHeader = psFitsReadHeader(NULL, file->fits); // The FITS header
        if (!tableHeader) psAbort("cannot read table header");

	// verify this is a binary table
        char *xtension = psMetadataLookupStr (NULL, tableHeader, "XTENSION");
        if (!xtension) psAbort("cannot read table type");
	if (strcmp (xtension, "BINTABLE")) {
	    psWarning ("no binary table in extension %s, skipping\n", dataname);
            psFree(tableHeader);
	    return false;
	}

	sources = pmSourcesRead_CFF(file->fits, hdu->header, recipe);

        psTrace("psModules.objects", 6, "read CMF table from %s : %s : %s", file->filename, headname, dataname);
        psFree (headname);
        psFree (dataname);
        psFree (tableHeader);
        }
        break;

      default:
        fprintf (stderr, "warning: type mismatch\n");
        break;
    }
    readout->data_exists = true;

    // if we have a prior set of detections on this readout, we will replace them here
    pmDetections *detections = pmDetectionsAlloc();
    detections->allSources = sources;
    status = psMetadataAdd (readout->analysis, PS_LIST_TAIL, "PSPHOT.DETECTIONS", PS_DATA_ARRAY | PS_META_REPLACE, "input sources", detections);
    psFree (detections);
    return true;
}

bool pmFPAviewCheckDataStatusForSources (const pmFPAview *view, const pmFPAfile *file)
{
    PS_ASSERT_PTR_NON_NULL(view, false);
    PS_ASSERT_PTR_NON_NULL(file, false);
    PS_ASSERT_PTR_NON_NULL(file->fpa, false);

    pmFPA *fpa = file->fpa;

    if (view->chip == -1) {
        bool exists = pmFPACheckDataStatusForSources (fpa);
        return exists;
    }
    if (view->chip >= fpa->chips->n) {
        psError(PS_ERR_IO, true, "Requested chip == %d >= fpa->chips->n == %ld", view->chip, fpa->chips->n);
        return false;
    }
    pmChip *chip = fpa->chips->data[view->chip];

    if (view->cell == -1) {
        bool exists = pmChipCheckDataStatusForSources (chip);
        return exists;
    }
    if (view->cell >= chip->cells->n) {
        psError(PS_ERR_IO, true, "Requested cell == %d >= chip->cells->n == %ld", view->cell, chip->cells->n);
        return false;
    }
    pmCell *cell = chip->cells->data[view->cell];

    if (view->readout == -1) {
        bool exists = pmCellCheckDataStatusForSources (cell);
        return exists;
    }

    if (view->readout >= cell->readouts->n) {
        psError(PS_ERR_IO, true, "Requested readout == %d >= cell->readouds->n == %ld", view->readout, cell->readouts->n);
        return false;
    }
    pmReadout *readout = cell->readouts->data[view->readout];

    bool exists = pmReadoutCheckDataStatusForSources (readout);
    return exists;
}

bool pmFPACheckDataStatusForSources (const pmFPA *fpa)
{
    PS_ASSERT_PTR_NON_NULL(fpa, false);
    PS_ASSERT_PTR_NON_NULL(fpa->chips, false);

    for (int i = 0; i < fpa->chips->n; i++) {
        pmChip *chip = fpa->chips->data[i];
        if (!chip) continue;
        if (pmChipCheckDataStatusForSources (chip)) return true;
    }
    return false;
}

bool pmChipCheckDataStatusForSources (const pmChip *chip)
{
    PS_ASSERT_PTR_NON_NULL(chip, false);
    PS_ASSERT_PTR_NON_NULL(chip->cells, false);

    for (int i = 0; i < chip->cells->n; i++) {
        pmCell *cell = chip->cells->data[i];
        if (!cell) continue;
        if (pmCellCheckDataStatusForSources (cell)) return true;
    }
    return false;
}

bool pmCellCheckDataStatusForSources (const pmCell *cell)
{
    PS_ASSERT_PTR_NON_NULL(cell, false);
    PS_ASSERT_PTR_NON_NULL(cell->readouts, false);

    for (int i = 0; i < cell->readouts->n; i++) {
        pmReadout *readout = cell->readouts->data[i];
        if (!readout) continue;
        if (pmReadoutCheckDataStatusForSources (readout)) return true;
    }
    return false;
}

bool pmReadoutCheckDataStatusForSources (const pmReadout *readout)
{
    PS_ASSERT_PTR_NON_NULL(readout, false);

    bool status;

    // select the detections of interest
    pmDetections *detections = psMetadataLookupPtr (&status, readout->analysis, "PSPHOT.DETECTIONS");
    if (!detections) return false;
    if (!detections->allSources) return false;
    return true;
}

// XXX: We might be able to macroize this and reuse for the other types

static bool pmReadoutReadXSRC(pmFPAfile *file, pmReadout *readout, char *exttype, psMetadata *hduHeader, psString xsrcname, psArray *sources, long *sourceIndex) 
{
    if (!psFitsMoveExtNameClean (file->fits, xsrcname)) {
        psTrace ("pmFPAfile", 1, "cannot find xsrc extension %s in %s, skipping", xsrcname, file->filename);
        return false;
    }

    psMetadata *tableHeader = psFitsReadHeader(NULL, file->fits); // The FITS header
    if (!tableHeader) psAbort("cannot read table header");

    char *xtension = psMetadataLookupStr (NULL, tableHeader, "XTENSION");
    if (!xtension) psAbort("cannot read table type");
    if (strcmp (xtension, "BINTABLE")) {
        psFree(tableHeader);
        psWarning ("no binary table in extension %s, skipping\n", xsrcname);
        return false;
    }

# define PM_SOURCES_READ_XSRC(NAME,TYPE)				\
    if (!strcmp (exttype, NAME)) {					\
	status = pmSourcesRead_##TYPE##_XSRC(file->fits, readout, hduHeader, tableHeader, sources, sourceIndex); \
    }									

    bool status = false;
    if (file->type == PM_FPA_FILE_CMF) {
	PM_SOURCES_READ_XSRC("PS1_V1",    CMF_PS1_V1);
	PM_SOURCES_READ_XSRC("PS1_V2",    CMF_PS1_V2);
	PM_SOURCES_READ_XSRC("PS1_V3",    CMF_PS1_V3);
	PM_SOURCES_READ_XSRC("PS1_V4",    CMF_PS1_V4);
	PM_SOURCES_READ_XSRC("PS1_V5",    CMF_PS1_V5);
	PM_SOURCES_READ_XSRC("PS1_SV1",   CMF_PS1_SV1);
	PM_SOURCES_READ_XSRC("PS1_SV2",   CMF_PS1_SV2);
	PM_SOURCES_READ_XSRC("PS1_SV3",   CMF_PS1_SV3);
	PM_SOURCES_READ_XSRC("PS1_SV4",   CMF_PS1_SV4);
	PM_SOURCES_READ_XSRC("PS1_DV1",   CMF_PS1_DV1);
	PM_SOURCES_READ_XSRC("PS1_DV2",   CMF_PS1_DV2);
	PM_SOURCES_READ_XSRC("PS1_DV3",   CMF_PS1_DV3);
	PM_SOURCES_READ_XSRC("PS1_DV4",   CMF_PS1_DV4);
	PM_SOURCES_READ_XSRC("PS1_DV5",   CMF_PS1_DV5);
    }
    psFree(tableHeader);
    return status;
}

static bool pmReadoutReadXFIT(pmFPAfile *file, pmReadout *readout, char *exttype, psMetadata *hduHeader, psString extname, psArray *sources, long *sourceIndex) 
{
    if (!psFitsMoveExtNameClean (file->fits, extname)) {
        psTrace ("pmFPAfile", 1, "cannot find xfit extension %s in %s, skipping", extname, file->filename);
        return false;
    }

    psMetadata *tableHeader = psFitsReadHeader(NULL, file->fits); // The FITS header
    if (!tableHeader) psAbort("cannot read table header");

    char *xtension = psMetadataLookupStr (NULL, tableHeader, "XTENSION");
    if (!xtension) psAbort("cannot read table type");
    if (strcmp (xtension, "BINTABLE")) {
        psWarning ("no binary table in extension %s, skipping\n", extname);
        psFree(tableHeader);
        return false;
    }

# define PM_SOURCES_READ_XFIT(NAME,TYPE)				\
    if (!strcmp (exttype, NAME)) {					\
	status = pmSourcesRead_##TYPE##_XFIT(file->fits, readout, hduHeader, tableHeader, sources, sourceIndex); \
    }									

    bool status = false;
    if (file->type == PM_FPA_FILE_CMF) {
	PM_SOURCES_READ_XFIT("PS1_V1",    CMF_PS1_V1);
	PM_SOURCES_READ_XFIT("PS1_V2",    CMF_PS1_V2);
	PM_SOURCES_READ_XFIT("PS1_V3",    CMF_PS1_V3);
	PM_SOURCES_READ_XFIT("PS1_V4",    CMF_PS1_V4);
	PM_SOURCES_READ_XFIT("PS1_V5",    CMF_PS1_V5);
	PM_SOURCES_READ_XFIT("PS1_SV1",   CMF_PS1_SV1);
	PM_SOURCES_READ_XFIT("PS1_SV2",   CMF_PS1_SV2);
	PM_SOURCES_READ_XFIT("PS1_SV3",   CMF_PS1_SV3);
	PM_SOURCES_READ_XFIT("PS1_SV4",   CMF_PS1_SV4);
	PM_SOURCES_READ_XFIT("PS1_DV1",   CMF_PS1_DV1);
	PM_SOURCES_READ_XFIT("PS1_DV2",   CMF_PS1_DV2);
	PM_SOURCES_READ_XFIT("PS1_DV3",   CMF_PS1_DV3);
	PM_SOURCES_READ_XFIT("PS1_DV4",   CMF_PS1_DV4);
	PM_SOURCES_READ_XFIT("PS1_DV5",   CMF_PS1_DV5);
    }
    psFree(tableHeader);
    return status;
}
static bool pmReadoutReadXRAD(pmFPAfile *file, pmReadout *readout, char *exttype, psMetadata *hduHeader, psString extname, psArray *sources, long *sourceIndex) 
{
    if (!psFitsMoveExtNameClean (file->fits, extname)) {
        psTrace ("pmFPAfile", 1, "cannot find xrad extension %s in %s, skipping", extname, file->filename);
        return false;
    }

    psMetadata *tableHeader = psFitsReadHeader(NULL, file->fits); // The FITS header
    if (!tableHeader) psAbort("cannot read table header");

    char *xtension = psMetadataLookupStr (NULL, tableHeader, "XTENSION");
    if (!xtension) psAbort("cannot read table type");
    if (strcmp (xtension, "BINTABLE")) {
        psWarning ("no binary table in extension %s, skipping\n", extname);
        psFree(tableHeader);
        return false;
    }

# define PM_SOURCES_READ_XRAD(NAME,TYPE)				\
    if (!strcmp (exttype, NAME)) {					\
	status = pmSourcesRead_##TYPE##_XRAD(file->fits, readout, hduHeader, tableHeader, sources, sourceIndex); \
    }									

    bool status = false;
    if (file->type == PM_FPA_FILE_CMF) {
	PM_SOURCES_READ_XRAD("PS1_V1",    CMF_PS1_V1);
	PM_SOURCES_READ_XRAD("PS1_V2",    CMF_PS1_V2);
	PM_SOURCES_READ_XRAD("PS1_V3",    CMF_PS1_V3);
	PM_SOURCES_READ_XRAD("PS1_V4",    CMF_PS1_V4);
	PM_SOURCES_READ_XRAD("PS1_V5",    CMF_PS1_V5);
	PM_SOURCES_READ_XRAD("PS1_SV1",   CMF_PS1_SV1);
	PM_SOURCES_READ_XRAD("PS1_SV2",   CMF_PS1_SV2);
	PM_SOURCES_READ_XRAD("PS1_SV3",   CMF_PS1_SV3);
	PM_SOURCES_READ_XRAD("PS1_SV4",   CMF_PS1_SV4);
	PM_SOURCES_READ_XRAD("PS1_DV1",   CMF_PS1_DV1);
	PM_SOURCES_READ_XRAD("PS1_DV2",   CMF_PS1_DV2);
	PM_SOURCES_READ_XRAD("PS1_DV3",   CMF_PS1_DV3);
	PM_SOURCES_READ_XRAD("PS1_DV4",   CMF_PS1_DV4);
	PM_SOURCES_READ_XRAD("PS1_DV5",   CMF_PS1_DV5);
    }
    psFree(tableHeader);
    return status;
}
static bool pmReadoutReadXGAL(pmFPAfile *file, pmReadout *readout, char *exttype, psMetadata *hduHeader, psString xgalname, psArray *sources, long *sourceIndex) 
{
    if (!psFitsMoveExtNameClean (file->fits, xgalname)) {
        psTrace ("pmFPAfile", 1, "cannot find xgal extension %s in %s, skipping", xgalname, file->filename);
        return false;
    }

    psMetadata *tableHeader = psFitsReadHeader(NULL, file->fits); // The FITS header
    if (!tableHeader) psAbort("cannot read table header");

    char *xtension = psMetadataLookupStr (NULL, tableHeader, "XTENSION");
    if (!xtension) psAbort("cannot read table type");
    if (strcmp (xtension, "BINTABLE")) {
        psFree(tableHeader);
        psWarning ("no binary table in extension %s, skipping\n", xgalname);
        return false;
    }

# define PM_SOURCES_READ_XGAL(NAME,TYPE)				\
    if (!strcmp (exttype, NAME)) {					\
	status = pmSourcesRead_##TYPE##_XGAL(file->fits, readout, hduHeader, tableHeader, sources, sourceIndex); \
    }									

    bool status = false;
    if (file->type == PM_FPA_FILE_CMF) {
	PM_SOURCES_READ_XGAL("PS1_V1",    CMF_PS1_V1);
	PM_SOURCES_READ_XGAL("PS1_V2",    CMF_PS1_V2);
	PM_SOURCES_READ_XGAL("PS1_V3",    CMF_PS1_V3);
	PM_SOURCES_READ_XGAL("PS1_V4",    CMF_PS1_V4);
	PM_SOURCES_READ_XGAL("PS1_V5",    CMF_PS1_V5);
	PM_SOURCES_READ_XGAL("PS1_SV1",   CMF_PS1_SV1);
	PM_SOURCES_READ_XGAL("PS1_SV2",   CMF_PS1_SV2);
	PM_SOURCES_READ_XGAL("PS1_SV3",   CMF_PS1_SV3);
	PM_SOURCES_READ_XGAL("PS1_SV4",   CMF_PS1_SV4);
	PM_SOURCES_READ_XGAL("PS1_DV1",   CMF_PS1_DV1);
	PM_SOURCES_READ_XGAL("PS1_DV2",   CMF_PS1_DV2);
	PM_SOURCES_READ_XGAL("PS1_DV3",   CMF_PS1_DV3);
	PM_SOURCES_READ_XGAL("PS1_DV4",   CMF_PS1_DV4);
	PM_SOURCES_READ_XGAL("PS1_DV5",   CMF_PS1_DV5);
    }
    psFree(tableHeader);
    return status;
}
