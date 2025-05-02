/** @file ppSubCamera.c
 *
 *  @brief
 *
 *  @ingroup ppSub
 *
 *  @author IfA
 *  @version $Revision: 1.34 $ $Name: not supported by cvs2svn $
 *  @date $Date: 2009-02-18 00:31:20 $
 *  Copyright 2009 Institute for Astronomy, University of Hawaii
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <pslib.h>
#include <psmodules.h>
#include <psphot.h>

#include "ppSub.h"

bool checkFileruleFileSave (pmFPAfile *file, pmConfig *config) {

  bool status;

  psMetadata *filerule = pmConfigFileRule(config, file->camera, file->name); // File rule
  if (!filerule) return false;

  char *myString = psMetadataLookupStr(&status, filerule, "FILE.SAVE");
  if (!myString) return false;

  // do not change the value from the default unless TRUE or FALSE are found
  if (!strcasecmp(myString, "TRUE")) {
    file->save = true;
    return true;
  } 
  if (!strcasecmp(myString, "FALSE")) {
    file->save = false;
    return true;
  } 
 
  return false;
}

// Define an input file
static pmFPAfile *defineInputFile(bool *success,
                                  pmConfig *config,// Configuration
                                  pmFPAfile *bind,    // File to which to bind, or NULL
                                  char *filerule,     // Name of file rule
                                  char *argname,      // Argument name
                                  pmFPAfileType fileType // Type of file
    )
{
    bool status;

    *success = false;

    pmFPAfile *file = NULL;

    // look for the file on the argument list
    if (bind) {
        file = pmFPAfileBindFromArgs(&status, bind, config, filerule, argname);
    } else {
        file = pmFPAfileDefineFromArgs(&status, config, filerule, argname);
    }

    if (!status) {
        psError(psErrorCodeLast(), false, "Failed to load file definition for %s", filerule);
        return false;
    }
    if (!file) {
        // look for the file on the RUN metadata
        file = pmFPAfileDefineFromRun(&status, bind, config, filerule); // File to return
        if (!status) {
            psError(psErrorCodeLast(), false, "Failed to load file definition for %s", filerule);
            return NULL;
        }
    }

    if (!file) {
        // no file defined
        *success = true;
        return NULL;
    }

    if (file->type != fileType) {
        psError(PPSUB_ERR_CONFIG, true, "%s is not of type %s", filerule, pmFPAfileStringFromType(fileType));
        return NULL;
    }

    *success = true;
    return file;
}

// Define an output file
static pmFPAfile *defineOutputFile(pmConfig *config, // Configuration
                                   pmFPAfile *template,    // File to use as basis for definition
                                   bool source, // Is template a source (T), or for binding (F)?
                                   char *filerule,     // Name of file rule
                                   pmFPAfileType fileType // Type of file
    )
{

    pmFPAfile *file = source ? pmFPAfileDefineFromFile(config, template, 1, 1, filerule) :
        pmFPAfileDefineOutput(config, template->fpa, filerule);
    if (!file) {
        psError(psErrorCodeLast(), false, _("Unable to generate output file from %s"), filerule);
        return NULL;
    }
    if (file->type != fileType) {
        psError(PPSUB_ERR_CONFIG, true, "%s is not of type %s", filerule, pmFPAfileStringFromType(fileType));
        return NULL;
    }

    return file;
}


// Define an output file that will be used in a calculation. This means it might already be
// available in the RUN metadata

// EAM 2022.10.14 : If the file is not found, treat it as if it did not exist in the input
// RUN metadata.  Currently, this is only used in this file to select
// PPSUB.OUTPUT.KERNELS.
static pmFPAfile *defineCalcFile(pmConfig *config, // Configuration
                                 pmFPAfile *bind,    // File to which to bind, or NULL
                                 char *filerule,     // Name of file rule
                                 const char *argname,   // Argument name for file
                                 pmFPAfileType fileType // Type of file
    )
{
    bool status;

    // look for the file on the RUN metadata
    pmFPAfile *file = pmFPAfileDefineFromRun(&status, NULL, config, filerule); // File to return
    if (!status) {
      bool requireSubkernel = psMetadataLookupBool(NULL, config->arguments, "-require-subkernel");
      if (requireSubkernel) {
	psError(psErrorCodeLast(), false, "Failed to load file definition for %s", filerule);
	return NULL;
      } else {
        psWarning("Failed to load file definition for %s, regenerate", filerule);
      }
    }
    file = pmFPAfileBindFromArgs(&status, bind, config, filerule, argname);
    if (!status) {
        psError(psErrorCodeLast(), false, "Failed to load file definition for %s", filerule);
        return NULL;
    }
    if (file) {
        // It's an input
        file->save = false;
    }

    // define new version of file
    if (!file) {
        file = pmFPAfileDefineOutput(config, bind ? bind->fpa : NULL, filerule);
        if (!status) {
            psError(psErrorCodeLast(), false, "Failed to load file definition for %s", filerule);
            return false;
        }
        if (file) {
            // It's an output (respect filerules)
	  checkFileruleFileSave(file, config); 
        }
    }

    if (!file) {
        return NULL;
    }

    if (file->type != fileType) {
        psError(PPSUB_ERR_CONFIG, true, "%s is not of type %s", filerule, pmFPAfileStringFromType(fileType));
        return NULL;
    }

    return file;
}

bool pmConfigFixSkycellCamera (pmConfig *config) {

    // We have some static MDC files used for update for which we are missing PSCAMERA.
    // (This was an attempt to address cross-camera diffs, but was only a partial solution)
    // Here we need to check for -SKYCELL entries and re-instate the PSCAMERA entries

    bool mdok = false;
    psMetadata *cameras = psMetadataLookupMetadata(&mdok, config->system, "CAMERAS");
    psAssert(cameras, "missing cameras in system config info");
    
    // iterate over the cameras and find ones with names like _*-SKYCELLS
    // psMetadataIterator *camerasIter = psMetadataIteratorAlloc(cameras, PS_LIST_HEAD, "^_.+-SKYCELL$");
    psMetadataIterator *camerasIter = psMetadataIteratorAlloc(cameras, PS_LIST_HEAD, NULL);
    psAssert(camerasIter, "unable to generate iterator?");

    psMetadataItem *camerasItem = NULL; // Item from the metadata
    while ((camerasItem = psMetadataGetAndIncrement(camerasIter))) {
      psAssert(camerasItem->type == PS_DATA_METADATA, "camerasItem has invalid type");
      
      char *name = camerasItem->name;
      psAssert (name, "NULL name for item");

      int nameLen = strlen(name);
      if (name[0] != '_') continue;
      if (nameLen <= 9) continue;
      char *p = &name[nameLen - 8];
      if (strcmp(p, "-SKYCELL")) continue;

      // we have found a -SKYCELL camera.  does it already have PSCAMERA?

      // remove PSCAMERA entries from *-SKYCELL cameras
      psMetadata *formats = psMetadataLookupMetadata(&mdok, camerasItem->data.md, "FORMATS"); // List of formats
      psAssert (formats, "missing FORMATS in camera.config ");

      psMetadata *format = psMetadataLookupMetadata(&mdok, formats, "SKYCELL"); // one true format for skycells
      psAssert (format, "missing SKYCELL metaformat for -SKYCELL entry ");

      psMetadata *rule = psMetadataLookupMetadata(&mdok, format, "RULE"); // one true format for skycells
      psAssert (rule, "missing RULE in SKYCELL metaformat for -SKYCELL entry ");

      psString pscamera = psMetadataLookupStr(&mdok, rule, "PSCAMERA"); // one true format for skycells
      if (pscamera) continue; // PSCAMERA exists, we are good

      // the name of this camera is encoded in the portion of the word skipped above
      // name[1] to name[nameLen - 8] : number of bytes to copy is : nameLen - 8 - 1:

      // _GPC2-SKYCELL
      // 0123456789012
      // nameLen = 13
      // nameLen - 8 - 1 = 4
      char cameraName[64];

      int nByte = nameLen - 8 - 1;
      psAssert (nByte < 64 - 1, "camera name is too long");
      
      strncpy (cameraName, &name[1], nByte); cameraName[nByte] = 0;

      psMetadataAddStr (rule, PS_LIST_TAIL, "PSCAMERA", PS_META_REPLACE, "", cameraName);
    }
    psFree (camerasIter);

    return true;
}

pmConfig *pmConfigMakeTemp (pmConfig *config) {
    pmConfig *altconfig = pmConfigAlloc();

    // these are NULL on pmConfigAlloc
    altconfig->user   = psMemIncrRefCounter(config->user);   // inherit from primary camera
    altconfig->site   = psMemIncrRefCounter(config->site);   // inherit from primary camera

    psFree (altconfig->system);
    altconfig->system = psMetadataCopy(NULL, config->system); // container for camera-specific recipe values (to be dropped)

    psFree (altconfig->files);
    altconfig->files  = psMemIncrRefCounter(config->files); // inherit from primary camera

    psFree (altconfig->arguments);
    altconfig->arguments = psMemIncrRefCounter(config->arguments); // inherit from primary camera

    psFree (altconfig->recipes);
    altconfig->recipes = psMetadataCopy(NULL, config->recipes); // container for camera-specific recipe values (to be dropped)

    // remove PSCAMERA entries from *-SKYCELL cameras.  This allows skycell images from
    // one camera to match those of another camera.  XXX Make this optional to force
    // matching cameras if desired?
    bool mdok = false;
    psMetadata *cameras = psMetadataLookupMetadata(&mdok, altconfig->system, "CAMERAS");
    psAssert(cameras, "missing cameras in system config info");
    
    // iterate over the cameras and find ones with names like _*-SKYCELLS
    // psMetadataIterator *camerasIter = psMetadataIteratorAlloc(cameras, PS_LIST_HEAD, "^_.+-SKYCELL$");
    psMetadataIterator *camerasIter = psMetadataIteratorAlloc(cameras, PS_LIST_HEAD, NULL);
    psAssert(camerasIter, "unable to generate iterator?");

    psMetadataItem *camerasItem = NULL; // Item from the metadata
    while ((camerasItem = psMetadataGetAndIncrement(camerasIter))) {
      psAssert(camerasItem->type == PS_DATA_METADATA, "camerasItem has invalid type");
      
      char *name = camerasItem->name;
      psAssert (name, "NULL name for item");

      int nameLen = strlen(name);
      if (name[0] != '_') continue;
      if (nameLen <= 9) continue;
      char *p = &name[nameLen - 8];
      if (strcmp(p, "-SKYCELL")) continue;

      // remove PSCAMERA entries from *-SKYCELL cameras
      psMetadata *formats = psMetadataLookupMetadata(&mdok, camerasItem->data.md, "FORMATS"); // List of formats
      psAssert (formats, "missing FORMATS in camera.config ");

      psMetadata *format = psMetadataLookupMetadata(&mdok, formats, "SKYCELL"); // one true format for skycells
      psAssert (format, "missing SKYCELL metaformat for -SKYCELL entry ");

      psMetadata *rule = psMetadataLookupMetadata(&mdok, format, "RULE"); // one true format for skycells
      psAssert (rule, "missing RULE in SKYCELL metaformat for -SKYCELL entry ");

      psString pscamera = psMetadataLookupStr(&mdok, rule, "PSCAMERA"); // one true format for skycells
      if (!pscamera) continue; // already removed, or never supplied

      psMetadataRemoveKey (rule, "PSCAMERA"); // allow any camera skycell to match
    }
    psFree (camerasIter);

    return (altconfig);
}

bool ppSubCopyPSCamera (pmFPAfile *tgt, pmFPAfile *src) {

    bool success;
    psMetadata *ruleSrc = psMetadataLookupMetadata(&success, src->format, "RULE"); // one true format for skycells
    psAssert (ruleSrc, "missing RULE in src->format");

    psString pscameraSrc = psMetadataLookupStr(&success, ruleSrc, "PSCAMERA"); // one true format for skycells
    psAssert (pscameraSrc, "missing PSCAMERA in src file"); 

    psMetadata *ruleTgt = psMetadataLookupMetadata(&success, tgt->format, "RULE"); // one true format for skycells
    psAssert (ruleTgt, "missing RULE in tgt->format");

    psMetadataAddStr (ruleTgt, PS_LIST_TAIL, "PSCAMERA", PS_META_REPLACE, "", pscameraSrc);
    return true;
}

bool ppSubCamera(ppSubData *data)
{
    bool success = true;

    psAssert(data, "Require processing data");
    pmConfig *config = data->config;
    psAssert(config, "Require configuration");

    pmConfigFixSkycellCamera (config);

    // Input image
    pmFPAfile *input = defineInputFile(&success, config, NULL, "PPSUB.INPUT", "INPUT", PM_FPA_FILE_IMAGE);
    if (!success) {
        psError(psErrorCodeLast(), false, "Failed to build FPA from PPSUB.INPUT");
        return false;
    }

    defineInputFile(&success, config, input, "PPSUB.INPUT.MASK", "INPUT.MASK", PM_FPA_FILE_MASK);
    if (!success) {
        psError(psErrorCodeLast(), false, "Failed to build FPA from PPSUB.INPUT.MASK");
        return false;
    }

    pmFPAfile *inVar = defineInputFile(&success, config, input, "PPSUB.INPUT.VARIANCE", "INPUT.VARIANCE", PM_FPA_FILE_VARIANCE);
    if (!success) {
        psError(psErrorCodeLast(), false, "Failed to build FPA from PPSUB.INPUT.VARIANCE");
        return false;
    }

    defineInputFile(&success, config, NULL, "PPSUB.INPUT.SOURCES", "INPUT.SOURCES", PM_FPA_FILE_CMF);
    if (!success) {
        psError(psErrorCodeLast(), false, "Failed to build FPA from PPSUB.INPUT.SOURCES");
        return false;
    }

    // Use a temporary config for the reference image, keeping the main user, site,
    // system, files, arguments entries.  This allows the reference image to be from a
    // different camera than the input image.  NOTE: there is no check that these two
    // images match in terms of size, pixel scale, etc. That is up to the user.
    // The temporary config structure has PSCAMERA entries removed from the SKYCELL rules
    // to allow subtraction between cameras.
    
    pmConfig *refconfig = pmConfigMakeTemp(config);

    // Reference image
    pmFPAfile *ref = defineInputFile(&success, refconfig, NULL, "PPSUB.REF", "REF", PM_FPA_FILE_IMAGE);
    if (!success) {
	psError(psErrorCodeLast(), false, "Failed to build FPA from PPSUB.REF");
	return false;
    }
    // Reference mask
    defineInputFile(&success, refconfig, ref, "PPSUB.REF.MASK", "REF.MASK", PM_FPA_FILE_MASK);
    if (!success) {
        psError(psErrorCodeLast(), false, "Failed to build FPA from PPSUB.REF.MASK");
        return false;
    }
    // Reference variance
    pmFPAfile *refVar = defineInputFile(&success, refconfig, ref, "PPSUB.REF.VARIANCE", "REF.VARIANCE", PM_FPA_FILE_VARIANCE);
    if (!success) {
        psError(psErrorCodeLast(), false, "Failed to build FPA from PPSUB.REF.VARIANCE");
        return false;
    }
    // copy ref->format(RULE) PSCAMERA from input->format(RULE)
    ppSubCopyPSCamera (ref, input);
    psFree (refconfig);

    // The reference sources may not be from the same origin as the reference image, so
    // use another temporary camera.
    pmConfig *srcconfig = pmConfigMakeTemp(config);

    // Sources input file (CMF)
    pmFPAfile *src = defineInputFile(&success, srcconfig, NULL, "PPSUB.REF.SOURCES", "REF.SOURCES", PM_FPA_FILE_CMF);
    if (!success) {
	psError(psErrorCodeLast(), false, "Failed to build FPA from PPSUB.REF.SOURCES");
	return false;
    }
    // copy src->format(RULE) PSCAMERA from input->format(RULE)
    ppSubCopyPSCamera (src, input);
    psFree (srcconfig);
    
    // Now that the camera has been determined, we can read the recipe
    psMetadata *recipe = psMetadataLookupMetadata(NULL, config->recipes, PPSUB_RECIPE); // Recipe for ppSim

    if (!recipe) {
        psError(PPSUB_ERR_CONFIG, false, "Unable to find recipe %s", PPSUB_RECIPE);
        return false;
    }
    if (psMetadataLookupBool(NULL, config->arguments, "-photometry")) {
        psMetadataAddBool(recipe, PS_LIST_TAIL, "PHOTOMETRY", PS_META_REPLACE,
                          "Perform photometry?", true);
    }
    if (psMetadataLookupBool(NULL, config->arguments, "-inverse")) {
        psMetadataAddBool(recipe, PS_LIST_TAIL, "INVERSE", PS_META_REPLACE,
                          "Generate inverse subtractions?", true);
    }
    if (psMetadataLookupBool(NULL, config->arguments, "-forced-phot")) {
        psMetadataAddBool(recipe, PS_LIST_TAIL, "FORCED.PHOTOMETRY.BOTH", PS_META_REPLACE, "Perform forced photometry?", true);
    }
    if (psMetadataLookupBool(NULL, config->arguments, "-forced-input1")) {
        psMetadataAddBool(recipe, PS_LIST_TAIL, "FORCED.PHOTOMETRY.INPUT1", PS_META_REPLACE, "Perform forced photometry?", true);
    }
    if (psMetadataLookupBool(NULL, config->arguments, "-forced-input2")) {
        psMetadataAddBool(recipe, PS_LIST_TAIL, "FORCED.PHOTOMETRY.INPUT2", PS_META_REPLACE, "Perform forced photometry?", true);
    }

    data->inverse     = psMetadataLookupBool(NULL, recipe, "INVERSE");
    data->photometry  = psMetadataLookupBool(NULL, recipe, "PHOTOMETRY");
    data->forcedPhot1 = psMetadataLookupBool(NULL, recipe, "FORCED.PHOTOMETRY.BOTH") || psMetadataLookupBool(NULL, recipe, "FORCED.PHOTOMETRY.INPUT1");
    data->forcedPhot2 = psMetadataLookupBool(NULL, recipe, "FORCED.PHOTOMETRY.BOTH") || psMetadataLookupBool(NULL, recipe, "FORCED.PHOTOMETRY.INPUT2");

    // Convolved input image
    pmFPAfile *inConvImage = defineOutputFile(config, input, true, "PPSUB.INPUT.CONV", PM_FPA_FILE_IMAGE);
    pmFPAfile *inConvMask = defineOutputFile(config, inConvImage, false, "PPSUB.INPUT.CONV.MASK",
                                             PM_FPA_FILE_MASK);
    if (!inConvImage || !inConvMask) {
        psError(psErrorCodeLast(), false, "Unable to define output files");
        return false;
    }
    // these ->save values below are set by command-line arguments (in ppSubArguments.c) : -save-inconv, -save-refconv
    checkFileruleFileSave (inConvImage, config);
    inConvImage->save = inConvImage->save || data->saveInConv;

    checkFileruleFileSave (inConvMask, config);
    inConvMask->save = inConvMask->save || data->saveInConv;
    if (inVar) {
        pmFPAfile *inConvVar = defineOutputFile(config, inConvImage, false, "PPSUB.INPUT.CONV.VARIANCE",
                                                PM_FPA_FILE_VARIANCE);
        if (!inConvVar) {
            psError(psErrorCodeLast(), false, "Unable to define output files");
            return false;
        }
	checkFileruleFileSave (inConvVar, config);
        inConvVar->save = inConvVar->save || data->saveInConv;
    }

    // Convolved ref image
    pmFPAfile *refConvImage = defineOutputFile(config, input, true, "PPSUB.REF.CONV", PM_FPA_FILE_IMAGE);
    pmFPAfile *refConvMask = defineOutputFile(config, refConvImage, false, "PPSUB.REF.CONV.MASK",
                                              PM_FPA_FILE_MASK);
    if (!refConvImage || !refConvMask) {
        psError(psErrorCodeLast(), false, "Unable to define output files");
        return false;
    }
    checkFileruleFileSave (refConvImage, config);
    refConvImage->save = refConvImage->save || data->saveRefConv;

    checkFileruleFileSave (refConvMask, config);
    refConvMask->save = refConvMask->save || data->saveRefConv;
    if (refVar) {
        pmFPAfile *refConvVar = defineOutputFile(config, refConvImage, false, "PPSUB.REF.CONV.VARIANCE",
                                                 PM_FPA_FILE_VARIANCE);
        if (!refConvVar) {
            psError(psErrorCodeLast(), false, "Unable to define output files");
            return false;
        }
	checkFileruleFileSave (refConvVar, config);
        refConvVar->save = refConvVar->save || data->saveRefConv;
    }

    // Output image
    pmFPAfile *output = defineOutputFile(config, inConvImage, true, "PPSUB.OUTPUT", PM_FPA_FILE_IMAGE);
    pmFPAfile *outMask = defineOutputFile(config, output, false, "PPSUB.OUTPUT.MASK", PM_FPA_FILE_MASK);
    if (!output || !outMask) {
        psError(psErrorCodeLast(), false, "Unable to define output files");
        return false;
    }
    checkFileruleFileSave (output, config);
    checkFileruleFileSave (outMask, config);
    pmFPAfile *outVar = NULL;
    if (inVar && refVar) {
        outVar = defineOutputFile(config, output, false, "PPSUB.OUTPUT.VARIANCE",
                                             PM_FPA_FILE_VARIANCE);
        if (!outVar) {
            psError(psErrorCodeLast(), false, "Unable to define output files");
            return false;
        }
	checkFileruleFileSave (outVar, config);
    }
    // If we are in update mode unconditionally save the output files
    bool updateMode = psMetadataLookupBool(NULL, config->arguments, "-updatemode");
    if (updateMode) {
        output->save = true;
        outMask->save = true;
        if (outVar) {
            outVar->save = true;
        }
    }

    pmFPAfile *inverse = NULL;          // Inverse output image
    if (data->inverse) {
        // Inverse output image
        inverse = defineOutputFile(config, output, true, "PPSUB.INVERSE", PM_FPA_FILE_IMAGE);
        pmFPAfile *invMask = defineOutputFile(config, inverse, false, "PPSUB.INVERSE.MASK",
                                              PM_FPA_FILE_MASK);
        if (!inverse || !invMask) {
            psError(psErrorCodeLast(), false, "Unable to define output files");
            return false;
        }
        checkFileruleFileSave (inverse, config);
        checkFileruleFileSave (invMask, config);
        if (inVar && refVar) {
            pmFPAfile *invVar = defineOutputFile(config, inverse, false, "PPSUB.INVERSE.VARIANCE",
                                                 PM_FPA_FILE_VARIANCE);
            if (!invVar) {
                psError(psErrorCodeLast(), false, "Unable to define output files");
                return false;
            }
            checkFileruleFileSave(invVar, config);
        }
    }


    // Output JPEGs
    pmFPAfile *jpeg1 = pmFPAfileDefineOutput(config, NULL, "PPSUB.OUTPUT.JPEG1");
    if (!jpeg1) {
        psError(psErrorCodeLast(), false, _("Unable to generate output file from PPSUB.OUTPUT.JPEG1"));
        return false;
    }
    if (jpeg1->type != PM_FPA_FILE_JPEG) {
        psError(psErrorCodeLast(), true, "PPSUB.OUTPUT.JPEG1 is not of type JPEG");
        return false;
    }
    checkFileruleFileSave(jpeg1, config);
    pmFPAfile *jpeg2 = pmFPAfileDefineOutput(config, NULL, "PPSUB.OUTPUT.JPEG2");
    if (!jpeg2) {
        psError(psErrorCodeLast(), false, _("Unable to generate output file from PPSUB.OUTPUT.JPEG2"));
        return false;
    }
    if (jpeg2->type != PM_FPA_FILE_JPEG) {
        psError(psErrorCodeLast(), true, "PPSUB.OUTPUT.JPEG2 is not of type JPEG");
        return false;
    }
    checkFileruleFileSave(jpeg2, config);

    // Output residual JPEG
    pmFPAfile *jpeg3 = pmFPAfileDefineOutput(config, NULL, "PPSUB.OUTPUT.RESID.JPEG");
    if (!jpeg3) {
        psError(psErrorCodeLast(), false, _("Unable to generate output file from PPSUB.OUTPUT.RESID.JPEG"));
        return false;
    }
    if (jpeg3->type != PM_FPA_FILE_JPEG) {
        psError(psErrorCodeLast(), true, "PPSUB.OUTPUT.RESID.JPEG is not of type JPEG");
        return false;
    }
    checkFileruleFileSave(jpeg3, config);

# if (0)
    // XXX NOTE EAM 20240209: This block of code attempts to correct the problem of missing
    // PSCAMERA entries, in this case in the output kernel definition.  This seems to work, but
    // there was a crash related to running psphot in update mode.  This block is probably not
    // responsible for the crash, but is commented out for the moment

    // The reference sources may not be from the same origin as the reference image, so
    // use another temporary camera.
    pmConfig *kernel_config = pmConfigMakeTemp(config);
    
    // Output subtraction kernel
    pmFPAfile *kernel = defineCalcFile(kernel_config, output, "PPSUB.OUTPUT.KERNELS", "KERNEL", PM_FPA_FILE_SUBKERNEL);
    if (!kernel) {
	psError(PS_ERR_UNEXPECTED_NULL, false, "Unable to define file PPSUB.OUTPUT.KERNELS");
	return false;
    }

    // copy kernel->format(RULE) PSCAMERA from input->format(RULE)
    ppSubCopyPSCamera (kernel, input);
    psFree (kernel_config);
# else
    // XXX NOTE EAM 20240209: This block of code matches the older version used in ipp-ps1-20220906-gentoo

    // Output subtraction kernel
    pmFPAfile *kernel = defineCalcFile(config, output, "PPSUB.OUTPUT.KERNELS", "KERNEL", PM_FPA_FILE_SUBKERNEL);
    if (!kernel) {
	psError(PS_ERR_UNEXPECTED_NULL, false, "Unable to define file PPSUB.OUTPUT.KERNELS");
	return false;
    }
# endif

    // psPhot input
    if (data->photometry || 1) {
	psphotModelClassInit();        // load implementation-specific models

        pmFPAfile *psphot = pmFPAfileDefineFromFPA(config, output->fpa, 1, 1, "PSPHOT.INPUT");
        if (!psphot) {
            psError(psErrorCodeLast(), false, "Failed to build FPA from PSPHOT.INPUT");
            return false;
        }
        if (psphot->type != PM_FPA_FILE_IMAGE) {
            psError(psErrorCodeLast(), true, "PSPHOT.INPUT is not of type IMAGE");
            return false;
        }
        pmFPAfileActivate(config->files, false, "PSPHOT.INPUT");

        // Internal file for getting the PSF from the minuend
        pmFPAfile *psf = pmFPAfileDefineFromFPA(config, output->fpa, 1, 1, "PSPHOT.PSF.LOAD");
        if (!psf) {
            psError(psErrorCodeLast(), false, "Failed to build FPA from PSPHOT.PSF.LOAD");
            return false;
        }
        if (psf->type != PM_FPA_FILE_PSF) {
            psError(psErrorCodeLast(), true, "PSPHOT.PSF.LOAD is not of type PSF");
            return false;
        }
        pmFPAfileActivate(config->files, false, "PSPHOT.PSF.LOAD");

        if (!psphotDefineFiles(config, psphot)) {
            psError(psErrorCodeLast(), false, "Unable to set up psphot files.");
            return false;
        }

        // Deactivate psphot output sources --- we want to define output source files of our own
        pmFPAfile *psphotOutput = pmFPAfileSelectSingle(config->files, "PSPHOT.OUTPUT", 0);
        psphotOutput->save = false;  // this one should NOT be set

        pmFPAfile *outSources = defineOutputFile(config, output, false, "PPSUB.OUTPUT.SOURCES",
                                                 PM_FPA_FILE_CMF);
        if (!outSources) {
            psError(psErrorCodeLast(), false, "Unable to set up output source file.");
            return false;
        }
        checkFileruleFileSave(outSources, config);

        if (data->inverse) {
            pmFPAfile *invSources = defineOutputFile(config, inverse, false, "PPSUB.INVERSE.SOURCES",
                                                     PM_FPA_FILE_CMF);
            if (!invSources) {
                psError(psErrorCodeLast(), false, "Unable to set up inverse source file.");
                return false;
            }
            checkFileruleFileSave(invSources, config);
        }

	// files need to do the forced photometry on the positions of sources in the positive images
        if (data->forcedPhot1) {
	    // this pmFPAfile is used to carry sources detected in the positive image #1
            pmFPAfile *posSources1 = defineOutputFile(config, input, true, "PPSUB.POS1.SOURCES", PM_FPA_FILE_CMF);
            if (!posSources1) {
                psError(psErrorCodeLast(), false, "Unable to set up forced source file.");
                return false;
            }
            checkFileruleFileSave(posSources1, config);

	    // this pmFPAfile is used to carry sources detected in the diff image @ the positions from positive image #1
            pmFPAfile *frcSources1 = defineOutputFile(config, input, true, "PPSUB.FORCED1.SOURCES", PM_FPA_FILE_CMF);
            if (!frcSources1) {
                psError(psErrorCodeLast(), false, "Unable to set up forced source file.");
                return false;
            }
            checkFileruleFileSave(frcSources1, config);
	}

        if (data->forcedPhot2) {
	    // this pmFPAfile is used to carry sources detected in the positive image #2
            pmFPAfile *posSources2 = defineOutputFile(config, ref, true, "PPSUB.POS2.SOURCES", PM_FPA_FILE_CMF);
            if (!posSources2) {
                psError(psErrorCodeLast(), false, "Unable to set up forced source file.");
                return false;
            }
            checkFileruleFileSave(posSources2, config);

	    // this pmFPAfile is used to carry sources detected in the diff image @ the positions from positive image #2
            pmFPAfile *frcSources2 = defineOutputFile(config, ref, true, "PPSUB.FORCED2.SOURCES", PM_FPA_FILE_CMF);
            if (!frcSources2) {
                psError(psErrorCodeLast(), false, "Unable to set up forced source file.");
                return false;
            }
	    checkFileruleFileSave(frcSources2, config);
        }
    }

    return true;
}

// Test function if needed:
bool pmConfigDumpSub (pmConfig *config, char *filename) {

    psMetadata *recipe = psMetadataLookupMetadata(NULL, config->recipes, PPSUB_RECIPE); // Recipe for ppSim
    psAssert (recipe, "oops");

    psMetadataConfigWrite (recipe, filename, NULL);

    return true;
}

