/** @file pswarpDefineSkycell.c
 *
 *  @brief
 *
 *  @ingroup pswarp
 *
 *  @author IfA
 *  @version $Revision: 1.12 $ $Name: not supported by cvs2svn $
 *  @date $Date: 2009-02-05 20:44:04 $
 *  Copyright 2009 Institute for Astronomy, University of Hawaii
 */

# include "pswarp.h"

/**
 * \brief XXX this function is based on pmFPAfileDefineFromArgs
 * a skycell consists of only one file

 * search for argname on the config->argument list and construct
 * an FPA based on the files in this list (must represent a single FPA)
 * built the association between the FPA elements (CHIP/CELL) and the files
 * define the pmFPAfile filename and bind it to this FPA
 * save the pmFPAfile on config->files
 * @return the pmFPAfile (a view to the one saved on config->files)
 */
bool pswarpDefineSkycell (pmFPAfile **outFile, pmConfig **outConfig, pmConfig *config, const char *filename, const char *argname)
{
    PS_ASSERT_PTR_NON_NULL(config, NULL);
    PS_ASSERT_STRING_NON_EMPTY(filename, NULL);
    PS_ASSERT_STRING_NON_EMPTY(argname, NULL);

    bool status;
    pmFPA *fpa = NULL;
    psFits *fits = NULL;
    pmFPAfile *file = NULL;
    psMetadata *phu = NULL;
    psMetadata *format = NULL;
    pmConfig *skyConfig = NULL;

    *outFile = NULL;
    *outConfig = NULL;

    // we search the argument data for the named fileset (argname)
    psArray *infiles = psMetadataLookupPtr(&status, config->arguments, argname);
    if (!status) {
        return false;
    }
    if (infiles->n != 1) {
        psError(PSWARP_ERR_CONFIG, false, "Found n == %ld files in %s in arguments\n", infiles->n, argname);
        return false;
    }

    // this function is implicitly an INPUT operation: do not create the file
    psString realName = pmConfigConvertFilename (infiles->data[0], config, false, false);
    if (!realName) {
        psError(psErrorCodeLast(), false, "Failed to convert file name %s\n", (char *) infiles->data[0]);
        return false;
    }

    // load the header of the image
    fits = psFitsOpen (realName, "r");
    if (!fits) {
        psError(psErrorCodeLast(), false, "Failed to open file %s\n", realName);
        psFree (realName);
        return false;
    }
    phu = psFitsReadHeader (NULL, fits);
    if (!phu) {
        psError(psErrorCodeLast(), false, "Failed to read file header %s\n", realName);
        psFree (realName);
        return false;
    }
    psFitsClose(fits);

    { // this bit is unique to pswarpDefineSkycell:

      // We need to force the format for the skycell to be equivalent to SIMPLE.  Determine
      // the current format from the header; Determine camera if not specified already
      // XXX EAM : this operation should be defined as a pmConfig function (pmConfigCopy?)
      skyConfig = pmConfigAlloc();
      skyConfig->user = psMemIncrRefCounter(config->user);
      skyConfig->system = psMemIncrRefCounter(config->system);

      psFree (skyConfig->files);
      skyConfig->files = psMemIncrRefCounter(config->files);
      psFree (skyConfig->arguments);
      skyConfig->arguments = psMemIncrRefCounter(config->arguments);
    }

    format = pmConfigCameraFormatFromHeader (NULL, NULL, NULL, skyConfig, phu, false);
    if (!format) {
        psError(psErrorCodeLast(), false, "Failed to read CCD format from %s\n", realName);
        psFree(phu);
        psFree (realName);
        return false;
    }

    // Record the camera name of the skycell, so we can save its configuration
    // XXX this can be put outside of the f
    psMetadataAddStr(config->arguments, PS_LIST_TAIL, "SKYCELL.CAMERA", 0, "Name of camera for skycell", skyConfig->cameraName);

    // build the template fpa, set up the basic view
    fpa = pmFPAConstruct (skyConfig->camera, skyConfig->cameraName);
    if (!fpa) {
        psError(psErrorCodeLast(), false, "Failed to construct FPA from %s", realName);
        psFree (realName);
        return false;
    }
    psFree (realName);

    // load the given filerule (from config->camera) and bind it to the fpa
    // the returned file is just a view to the entry on config->files
    file = pmFPAfileDefineInput (skyConfig, fpa, NULL, filename);
    if (!file) {
        psError(psErrorCodeLast(), false, "file %s not defined\n", filename);
        psFree(phu);
        psFree(fpa);
        psFree(format);
        return false;
    }
    psFree (format);
    file->format = psMemIncrRefCounter(format);

    // adjust the rules to identify these files in the file->names data
    psFree (file->filerule);
    file->filerule = psStringCopy ("@FILES");
    file->filesrc = psStringCopy ("{CHIP.NAME}.{CELL.NAME}");

    file->fileLevel = pmFPAPHULevel(format);
    if (file->fileLevel == PM_FPA_LEVEL_NONE) {
        psError(PSWARP_ERR_CONFIG, true, "Unable to determine file level for %s\n", file->name);
        psFree(phu);
        psFree(fpa);
        psFree(format);
        psFree(file);
        return false;
    }

    // set the view to the corresponding entry for this phu
    pmFPAview *view = pmFPAAddSourceFromHeader (fpa, phu, format);
    if (!view) {
        psError(psErrorCodeLast(), false, "Unable to determine source for %s", file->name);
        psFree(phu);
        psFree (fpa);
        psFree (format);
        return false;
    }

    // associate the filename with the FPA element
    char *name = pmFPAfileNameFromRule (file->filesrc, file, view);

    // save the name association in the pmFPAfile structure
    psMetadataAddStr (file->names, PS_LIST_TAIL, name, 0, "", infiles->data[0]);

    psFree (view);
    psFree (name);
    psFree (phu);
    psFree (fpa);

    *outFile = file;
    *outConfig = skyConfig;

    return true;
}

