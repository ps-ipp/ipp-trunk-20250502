#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <string.h>
#include <strings.h>            /* for strn?casecmp */
#include <pslib.h>

#include "pmErrorCodes.h"
#include "pmConfig.h"
#include "pmConfigMask.h"
#include "pmConfigRun.h"
#include "pmDetrendDB.h"

#include "pmHDU.h"
#include "pmFPA.h"
#include "pmFPALevel.h"
#include "pmFPAview.h"
#include "pmFPAfile.h"
#include "pmFPAfile.h"
#include "pmFPAConstruct.h"

#include "pmConceptsCopy.h"

# define FPA_TEST_ASSERT(A){ \
        assert(A->format == NULL); \
        assert(A->formatName == NULL); \
        assert(A->filerule == NULL); \
        assert(A->filesrc == NULL); }

// Parse an option from a metadata, returning the appropriate integer value
static int parseOptionInt(const psMetadata *md, // Metadata containing the option
                          const char *name, // Option name
                          const char *source, // Description of source, for warning messages
                          int defaultValue // Default value
                          )
{
    psMetadataItem *item = psMetadataLookup(md, name); // Item with the value of interest
    if (!item) {
        psWarning("Unable to find value for %s in %s --- set to %d.", name, source, defaultValue);
        return defaultValue;
    }
    int value = psMetadataItemParseS32(item); // Value of interst
    return value;
}

// Parse an option from a metadata, returning the appropriate float value
static float parseOptionFloat(const psMetadata *md, // Metadata containing the option
                              const char *name, // Option name
                              const char *source // Description of source, for warning messages
                              )
{
    psMetadataItem *item = psMetadataLookup(md, name); // Item with the value of interest
    if (!item) {
        psWarning("Unable to find value for %s in %s", name, source);
        return NAN;
    }
    float value = psMetadataItemParseF32(item); // Value of interst
    return value;
}

// Parse an option from a metadata, returning the appropriate double value
static double parseOptionDouble(const psMetadata *md, // Metadata containing the option
                                const char *name, // Option name
                                const char *source // Description of source, for warning messages
                                )
{
    psMetadataItem *item = psMetadataLookup(md, name); // Item with the value of interest
    if (!item) {
        psWarning("Unable to find value for %s in %s", name, source);
        return NAN;
    }
    double value = psMetadataItemParseF64(item); // Value of interst
    return value;
}


// define an input-type pmFPAfile, bind to the optional fpa if supplied
pmFPAfile *pmFPAfileDefineInput(const pmConfig *config, pmFPA *fpa, char *cameraName, const char *name)
{
    PS_ASSERT_PTR_NON_NULL(config, NULL);
    PS_ASSERT_PTR_NON_NULL(config->files, NULL);
    PS_ASSERT_PTR_NON_NULL(config->camera, NULL);
    PS_ASSERT_STRING_NON_EMPTY(name, NULL);

    bool status;
    char *type;

    const psMetadata *camera = (fpa ? fpa->camera : config->camera); // Camera configuration for this file
    psMetadata *data = pmConfigFileRule(config, camera, name); // File rule
    if (!data) {
        psError(psErrorCodeLast(), false, "Can't find file rule %s!", name);
        return NULL;
    }

    pmFPAfile *file = pmFPAfileAlloc();

    // save the name of this pmFPAfile
    file->name = psStringCopy(name);

    file->filerule = psMemIncrRefCounter(psMetadataLookupStr (&status, data, "FILENAME.RULE"));

    type = psMetadataLookupStr(&status, data, "FILE.TYPE");
    if (!type) {
        psError(PM_ERR_CONFIG, true, "FILE.TYPE is not defined for %s\n", name);
        psFree(file);
        return NULL;
    }

    file->type = pmFPAfileTypeFromString(type);
    if (file->type == PM_FPA_FILE_NONE) {
        psError(PM_ERR_CONFIG, true, "FILE.TYPE %s is not registered in pmFPAfile.c:pmFPAfileTypeFromString\n", type);
        psFree(file);
        return NULL;
    }

    file->mode = PM_FPA_MODE_READ;
    file->fileLevel = PM_FPA_LEVEL_NONE; // the fileLevel depends on the input data

    file->dataLevel = pmFPALevelFromName(psMetadataLookupStr (&status, data, "DATA.LEVEL"));
    if (file->dataLevel == PM_FPA_LEVEL_NONE) {
        psError(PM_ERR_CONFIG, true, "DATA.LEVEL is not set for %s\n", name);
        psFree(file);
        return NULL;
    }
    // default is to free the data after use (after written out)
    // this can be overridden for pmFPAfiles used as carriers as well
    file->freeLevel = file->dataLevel;

    if (fpa) {
        file->fpa = psMemIncrRefCounter(fpa);
        file->camera = psMemIncrRefCounter((psMetadata *)fpa->camera);
        file->cameraName = cameraName ? psMemIncrRefCounter(cameraName) : psMemIncrRefCounter(config->cameraName); // XXX Is this the correct thing to do?
    } else {
        file->camera = psMemIncrRefCounter(config->camera);
        file->cameraName = psMemIncrRefCounter(config->cameraName);
    }

    // XXX ppImage and similar require the added file to be unique
    // XXX ppFocus wants to override the selection with the new selection
    // XXX require programs like ppFocus to remove existing files by hand
    if (!psMetadataAddPtr(config->files, PS_LIST_TAIL, name,
                          PS_DATA_UNKNOWN | PS_META_DUPLICATE_OK, "", file)) {
        psError(PM_ERR_CONFIG, false, "could not add %s to config files", name);
        return NULL;
    }
    psFree(file);
    return file;
}

// Define an output pmFPAfile
pmFPAfile *pmFPAfileDefineOutputForFormat(const pmConfig *config, // Configuration
                                          pmFPA *fpa, // Optional FPA to bind
                                          const char *name, // Name of file rule
                                          psString cameraName, // Name of camera configuration to use
                                          psString formatName // Name of camera format to use
    )
{
    bool status;

    // Use the camera we were told to, the camera of the provided FPA, or default to the default camera
    psMetadata *camera;                 // Camera configuration
    if (!cameraName || strlen(cameraName) == 0) {
        if (fpa && fpa->camera) {
            camera = (psMetadata*)fpa->camera; // Casting away const, so I can put it in the file
        } else {
            camera = config->camera;
            cameraName = config->cameraName;
        }
    } else {
        bool mdok;                      // Status of MD lookup
        psMetadata *cameras = psMetadataLookupMetadata(&mdok, config->system, "CAMERAS"); // Known cameras
        if (!mdok || !cameras) {
            psError(PM_ERR_CONFIG, true, "Unable to find CAMERAS in the system configuration.\n");
            return NULL;
        }
        camera = psMetadataLookupMetadata(&mdok, cameras, cameraName); // Camera configuration of interest
        if (!mdok || !camera) {
            psError(PM_ERR_CONFIG, true,
                    "Unable to find automatically generated camera configuration %s in system configuration.",
                    cameraName);
            return NULL;
        }

        if (fpa && fpa->camera && fpa->camera != camera) {
            psAbort("Camera of bound FPA is not the requested camera --- there is an inconsistency!");
        }
    }

    psMetadata *filerule = pmConfigFileRule(config, camera, name); // File rule
    if (!filerule) {
        psError(psErrorCodeLast(), false, "Can't find file rule %s!", name);
        return NULL;
    }

    pmFPAfile *file = pmFPAfileAlloc();

    // save the name of this pmFPAfile
    file->name = psStringCopy(name);

    // this is the filename rule
    file->filerule = psMemIncrRefCounter(psMetadataLookupStr(&status, filerule, "FILENAME.RULE"));

    const char *type = psMetadataLookupStr(&status, filerule, "FILE.TYPE");
    file->type = pmFPAfileTypeFromString(type);
    if (file->type == PM_FPA_FILE_NONE) {
        psError(PM_ERR_CONFIG, true, "FILE.TYPE is not defined for %s\n", name);
        psFree(file);
        return NULL;
    }

    file->mode = PM_FPA_MODE_WRITE;
    file->save = false;

    file->camera = psMemIncrRefCounter(camera);
    file->cameraName = psMemIncrRefCounter(cameraName);

    // Copy the file id valuves if they have been set in the config
    if (config->sourceId) {
        file->sourceId = config->sourceId;
    }
    if (config->imageId) {
        file->imageId = config->imageId;
    }

    // Use the format we were told to, the format specified in the file rule, or default to the default format
    if (!formatName || strlen(formatName) == 0) {
        // select the format list from the selected camera
        formatName = psMetadataLookupStr(&status, filerule, "FILE.FORMAT");
        if (!formatName || strcmp(formatName, "NONE") == 0) {
            // Try to get by with the default
            formatName = config->formatName;
        }
    }
    psMetadata *formats = psMetadataLookupMetadata(&status, file->camera, "FORMATS"); // List of formats
    psMetadata *format = psMetadataLookupMetadata(&status, formats, formatName); // Camera format to use
    if (!format) {
        psError(PM_ERR_CONFIG, true, "Unable to find format %s for file %s.\n",
                formatName, file->name);
        psFree(file);
        return NULL;
    }
    file->format = psMemIncrRefCounter(format);
    file->formatName = psStringCopy(formatName);

    if (fpa) {
        file->fpa = psMemIncrRefCounter(fpa);
    } else {
        file->fpa = pmFPAConstruct(file->camera, file->cameraName);
    }

    // Get FITS output scheme
    const char *fitsType = psMetadataLookupStr(&status, filerule, "FITS.TYPE"); // Name of FITS scheme to use
    if (fitsType && strcasecmp(fitsType, "NONE") != 0) {

        // load the FITSTYPE scheme for this file
        psMetadata *scheme = pmConfigFitsType(config, camera, fitsType); // File rule
        if (!scheme) {
            // XXX change to a config error?
            psWarning("Unable to find %s in FITS in camera configuration --- will use defaults.", fitsType);
            goto FITS_OPTIONS_DONE;
        }
        psLogMsg ("psModules.camera", PS_LOG_INFO, "using FITS.TYPE %s for %s.\n", fitsType, file->name);

        psString source = NULL;     // Source of options
        psStringAppend(&source, "%s in FITS in camera configuration", fitsType);

        psFitsOptions *options = file->options = psFitsOptionsAlloc(); // FITS I/O options

        // Custom floating-point
        bool mdok;                      // Status of MD lookup
        const char *floatName = psMetadataLookupStr(&mdok, scheme, "FLOAT"); // Name of custom float
        if (mdok && floatName) {
            psString fullName = NULL;   // Full name of custom floating-point
            psStringAppend(&fullName, "FLOAT_%s", floatName);
            options->floatType = psFitsFloatTypeFromString(fullName);
            psFree(fullName);
        }

        options->bitpix = parseOptionInt(scheme, "BITPIX", source, 0); // Bits per pixel

        // Scaling options
        const char *scalingString = psMetadataLookupStr(&mdok, scheme, "SCALING"); // Scaling name
        if (scalingString) {
            options->scaling = psFitsScalingFromString(scalingString); // Scaling method

            switch (options->scaling) {
              case PS_FITS_SCALE_NONE:
              case PS_FITS_SCALE_RANGE:
	      case PS_FITS_SCALE_LOG_RANGE:
	      case PS_FITS_SCALE_ASINH_RANGE:
                // No options required
                break;
              case PS_FITS_SCALE_STDEV_POSITIVE:
              case PS_FITS_SCALE_STDEV_NEGATIVE:
	      case PS_FITS_SCALE_LOG_STDEV_POSITIVE:
  	      case PS_FITS_SCALE_LOG_STDEV_NEGATIVE:
	      case PS_FITS_SCALE_ASINH_STDEV_POSITIVE:
  	      case PS_FITS_SCALE_ASINH_STDEV_NEGATIVE:
                options->stdevNum = parseOptionFloat(scheme, "STDEV.NUM", source); // Padding to edge
                if (!isfinite(options->stdevNum)) {
                    psError(PM_ERR_CONFIG, true, "Bad value for STDEV.NUM for %s", source);
                    psFree(source);
                    psFree(file);
                    return NULL;
                }
                // Flow through
              case PS_FITS_SCALE_STDEV_BOTH:
	      case PS_FITS_SCALE_LOG_STDEV_BOTH:
	      case PS_FITS_SCALE_ASINH_STDEV_BOTH:
                options->stdevBits = parseOptionInt(scheme, "STDEV.BITS", source, 0); // Bits for stdev
                if (options->stdevBits <= 0) {
                    psError(PM_ERR_CONFIG, true, "Bad value for STDEV.BITS (%d) for %s",
                            options->stdevBits, source);
                    psFree(source);
                    psFree(file);
                    return NULL;
                }
                break;
              case PS_FITS_SCALE_MANUAL:
                options->bscale = parseOptionDouble(scheme, "BSCALE", source); // Scaling
                options->bzero = parseOptionDouble(scheme, "BZERO", source); // Zero point
                break;
	      case PS_FITS_SCALE_LOG_MANUAL:
		options->bscale = parseOptionDouble(scheme, "BSCALE", source); // Scaling
		options->bzero = parseOptionDouble(scheme, "BZERO", source); // Zero point
		options->boffset = parseOptionDouble(scheme, "BOFFSET", source); // Log offset
	      case PS_FITS_SCALE_ASINH_MANUAL:
		options->bscale = parseOptionDouble(scheme, "BSCALE", source); // Scaling
		options->bzero = parseOptionDouble(scheme, "BZERO", source); // Zero point
		options->boffset = parseOptionDouble(scheme, "BOFFSET", source); // Log offset
		options->bsoften = parseOptionDouble(scheme, "BSOFTEN", source); // Softening parameter
		break;	      
	      default:
                psAbort("Should never get here.");
            }
        }

        psMetadataItem *fuzz = psMetadataLookup(scheme, "FUZZ"); // Quantisation fuzz?
        if (fuzz) {
            if (fuzz->type != PS_DATA_BOOL) {
                psWarning("FUZZ in compression scheme %s isn't boolean.", fitsType);
                goto FITS_OPTIONS_DONE;
            }
            options->fuzz = fuzz->data.B;
        }

        // Compression options
        const char *compressString = psMetadataLookupStr(&mdok, scheme, "COMPRESSION"); // Compression type
        if (mdok && compressString) {
            psFitsCompressionType type = psFitsCompressionTypeFromString(compressString); // Compression
            psVector *tile = psVectorAlloc(3, PS_TYPE_S32); // Tile sizes
            tile->data.S32[0] = parseOptionInt(scheme, "TILE.X", source, 0); // Tiling in x
            tile->data.S32[1] = parseOptionInt(scheme, "TILE.Y", source, 1); // Tiling in y
            tile->data.S32[2] = parseOptionInt(scheme, "TILE.Z", source, 1); // Tiling in z
            int noise = parseOptionInt(scheme, "NOISE", source, 16); // Noise bits
            int hscale = 0, hsmooth = 0;// Scaling and smoothing for HCOMPRESS
            if (type == PS_FITS_COMPRESS_HCOMPRESS) {
                hscale = parseOptionInt(scheme, "HSCALE", source, 0);
                hsmooth = parseOptionInt(scheme, "HSMOOTH", source, 0);
            }

            file->compression = psFitsCompressionAlloc(type, tile, noise, hscale, hsmooth);
            psFree(tile);
        }

        psFree(source);
    }
 FITS_OPTIONS_DONE:

    file->fileLevel = pmFPAPHULevel(format);
    if (file->fileLevel == PM_FPA_LEVEL_NONE) {
        psError(PM_ERR_CONFIG, true, "Unable to determine file level for %s\n", name);
        psFree(file);
        return NULL;
    }

    file->dataLevel = pmFPALevelFromName(psMetadataLookupStr(&status, filerule, "DATA.LEVEL"));
    if (file->dataLevel == PM_FPA_LEVEL_NONE) {
        psError(PM_ERR_CONFIG, true, "DATA.LEVEL is not set for %s\n", name);
        psFree(file);
        return NULL;
    }
    // default is to free the data after use (after written out)
    // this can be overridden for pmFPAfiles used as carriers as well
    file->freeLevel = file->dataLevel;
    file->fileLevel = PS_MIN (file->fileLevel, file->dataLevel);

    // XXX the file/data/free level must be consistent with the reference fpa (but since we
    // don't have access to its pmFPAfile, we cannot enforce this here...

    pmFPALevel extLevel = pmFPAExtensionsLevel(format); // Level for extensions
    if (extLevel != PM_FPA_LEVEL_NONE) {
        if (extLevel < file->dataLevel) {
            psWarning("Level for extensions is higher than desired data level --- adjusting.\n");
            file->dataLevel = extLevel;
        }
        if (extLevel < file->freeLevel) {
            psWarning("Level for extensions is higher than desired free level --- adjusting.\n");
            file->freeLevel = extLevel;
        }
    } else {
        // if we do not have extensions in the file, we are forced to write out at the file level
        file->dataLevel = file->fileLevel;
        file->freeLevel = file->fileLevel;
    }

    psTrace("psModules.camera", 5,
            "file: %s, format: %s, fileLevel: %s, extLevel: %s, dataLevel: %s, freeLevel: %s\n",
            file->name, file->formatName, pmFPALevelToName(file->fileLevel), pmFPALevelToName(extLevel),
            pmFPALevelToName(file->dataLevel), pmFPALevelToName(file->freeLevel));

    // add argument-supplied OUTPUT name to this file
    char *outname = psMetadataLookupStr(&status, config->arguments, "OUTPUT");
    psMetadataAddStr(file->names, PS_LIST_TAIL, "OUTPUT", PS_META_NO_REPLACE, "Output file name", outname);

    // place the resulting file in the config system
    psMetadataAddPtr(config->files, PS_LIST_TAIL, name, PS_DATA_UNKNOWN | PS_META_DUPLICATE_OK,
                     "Output file", file);
    psFree(file);                       // we free this copy of file, but 'files' still has a copy
    return file;                        // the returned value is a view into the version on 'files'
}

// define a pmFPAfile, bind to the optional fpa if supplied
pmFPAfile *pmFPAfileDefineOutput(const pmConfig *config, pmFPA *fpa, const char *name)
{
    PS_ASSERT_PTR_NON_NULL(config, NULL);
    PS_ASSERT_PTR_NON_NULL(config->files, NULL);
    PS_ASSERT_PTR_NON_NULL(config->camera, NULL);
    PS_ASSERT_STRING_NON_EMPTY(name, NULL);

    return pmFPAfileDefineOutputForFormat(config, fpa, name, NULL, NULL);
}

// define a pmFPAfile, bind to the optional file if supplied
pmFPAfile *pmFPAfileDefineOutputFromFile(const pmConfig *config, pmFPAfile *file, const char *name)
{
    PS_ASSERT_PTR_NON_NULL(config, NULL);
    PS_ASSERT_PTR_NON_NULL(config->files, NULL);
    PS_ASSERT_PTR_NON_NULL(config->camera, NULL);
    PS_ASSERT_STRING_NON_EMPTY(name, NULL);

    char *cameraName = NULL, *formatName = NULL; // Name of camera and format
    pmFPA *fpa = NULL;                  // FPA for file
    if (file) {
        cameraName = file->cameraName;
        formatName = file->formatName;
        fpa = file->fpa;
    }

    return pmFPAfileDefineOutputForFormat(config, fpa, name, cameraName, formatName);
}

// given a filename, convert to UNIX namespace and read the PHU 
psMetadata *readPHUfromFilename (char *filename, pmConfig *config) {

    // Need to generate an FPA
    psString realName = pmConfigConvertFilename(filename, config, false, false);
    if (!realName) {
	psError(psErrorCodeLast(), false, "Failed to convert file name %s", filename);
	return NULL;
    }

    // load the header of the first image
    // EXTWORD (fits->extword) is not relevant to the PHU
    psFits *fits = psFitsOpen(realName, "r"); // FITS file
    if (!fits) {
	psError(psErrorCodeLast(), false, "Failed to open file %s", realName);
	psFree(realName);
	return NULL;
    }

    psMetadata *phu = psFitsReadHeader (NULL, fits); // Primary header
    if (!phu) {
	psError(psErrorCodeLast(), false, "Failed to read file header %s", realName);
	psFree(realName);
	return NULL;
    }

    if (!psFitsClose(fits)) {
	psError(psErrorCodeLast(), false, "Failed to close file %s", realName);
	psFree(realName);
	psFree(phu);
	return NULL;
    }

    psFree(realName);
    return phu;
}

// this this function wants to return:
// pmFPA, PHU, fileLevel, outConfig
// camera, cameraName, formatName
typedef struct {
    pmFPA *fpa;
    psMetadata *phu;
    psMetadata *format;
    pmFPALevel fileLevel;
    psString cameraName;
    psString formatName;
} pmFPAfromFilenameOutput;

// for the given filename, read PHU and determine camera format; build an FPA for the file
bool pmFPAfromFilename (pmFPAfromFilenameOutput *output, pmConfig **outConfig, pmConfig *sysConfig, char *filename){

    // Need to generate an FPA
    psMetadata *phu = readPHUfromFilename (filename, sysConfig);
    if (!phu) {
	psError(psErrorCodeLast(), false, "Failed to read PHU for %s", filename);
	return false;
    }

    // if we expect the loaded FPA to differ in configuration from the current system configuration
    // generate an output config for this FPA
    pmConfig *config = NULL;
    if (outConfig) {
	config = pmConfigAlloc();
	config->user = psMemIncrRefCounter(sysConfig->user);
	config->system = psMemIncrRefCounter(sysConfig->system);

	psFree (config->files);
	config->files = psMemIncrRefCounter(sysConfig->files);
	psFree (config->arguments);
	config->arguments = psMemIncrRefCounter(sysConfig->arguments);

	*outConfig = config;
    } else {
	config = sysConfig;
    }

    // values which are returned to calling function
    psString formatName = NULL;	// Name of camera format
    psString cameraName = NULL;	// Name of camera
    psMetadata *camera = NULL;	// Camera configuration

    // Determine the current format from the header; determine camera if not specified already.
    psMetadata *format = pmConfigCameraFormatFromHeader(&camera, &cameraName, &formatName, config, phu, true);
    if (!format) {
	psError(psErrorCodeLast(), false, "Failed to determine camera format for %s", filename);
	psFree(camera);
	psFree(formatName);
	psFree(phu);
	return false;
    }

    pmFPALevel fileLevel = pmFPAPHULevel(format);
    if (fileLevel == PM_FPA_LEVEL_NONE) {
	psError(PM_ERR_CONFIG, true, "Unable to determine file level for %s", filename);
	psFree(camera);
	psFree(formatName);
	psFree(phu);
	return false;
    }

    // build the template fpa, set up the basic view
    // we supply the metaCamera name (if NULL, baseCamera name is used)
    pmFPA *fpa = pmFPAConstruct(camera, cameraName);
    psFree(camera);

    if (!fpa) {
	psError(psErrorCodeLast(), false, "Failed to construct FPA from %s", filename);
	psFree(formatName);
	psFree(format);
	psFree(phu);
	return NULL;
    }

    output->fpa = fpa;
    output->phu = phu;
    output->format = format;
    output->fileLevel = fileLevel;
    output->cameraName = cameraName;
    output->formatName = formatName;

    return true;

}

/// Define a file from an array of filenames
static pmFPAfile *fpaFileDefineFromArray(pmConfig **outConfig, // output configuration
					 pmConfig *sysConfig, // global configuration
                                         pmFPAfile *bind, // File to bind to, or NULL
                                         const char *name, // Name of file
                                         const psArray *filenames // Array of file names
    )
{
    PS_ASSERT_PTR_NON_NULL(sysConfig, NULL);
    PS_ASSERT_STRING_NON_EMPTY(name, NULL);

    pmFPA *fpa = NULL;                  // FPA for file
    psMetadata *format = NULL;          // Camera format configuration
    psString formatName = NULL;         // Name of camera format
    psString cameraName = NULL;         // Name of camera
    pmFPALevel fileLevel = PM_FPA_LEVEL_NONE; // Level for files
    psMetadata *phu = NULL;             // Primary header

    if (bind) {
        // Use the FPA we're binding to
        fpa = psMemIncrRefCounter(bind->fpa);
        fileLevel = bind->fileLevel;
    } else {
	pmFPAfromFilenameOutput output;
	if (!pmFPAfromFilename (&output, outConfig, sysConfig, filenames->data[0])) {
	    return NULL;
	}
	fpa = output.fpa;
	phu = output.phu;
	format = output.format;
	fileLevel = output.fileLevel;
	cameraName = output.cameraName;
	formatName = output.formatName;
    }

    pmConfig *config = outConfig ? *outConfig : sysConfig;

    // load the given filerule (from config->camera) and bind it to the fpa
    // the returned file is just a view to the entry on config->files
    pmFPAfile *file = pmFPAfileDefineInput(config, fpa, cameraName, name); // File, to return
    if (!file) {
        psError(psErrorCodeLast(), false, "File %s not defined", name);
        psFree(formatName);
        psFree(format);
        psFree(fpa);
        psFree(phu);
        return NULL;
    }
    psFree(cameraName);
    psFree(fpa);                        // Drop reference

    file->format = format;
    file->formatName = formatName;
    file->fileLevel = fileLevel;

    // We use the filerule and filesrc to identify the files in the file->names data
    psFree(file->filerule); // this is set in pmFPAfileDefineInput
    file->filerule = psStringCopy("@FILES");
    file->filesrc = psStringCopy("{CHIP.NAME}.{CELL.NAME}");

    // Examine the list of input files and validate their cameras
    // Associate each filename with an element of the FPA
    // Save the association on file->names
    for (int i = 0; i < filenames->n; i++) {
        // Check that the file corresponds to the same camera and format
        if (!phu) {
	    phu = readPHUfromFilename (filenames->data[i], config);
            if (!phu) {
                psError(psErrorCodeLast(), false, "Failed to read PHU for %s", (char *)filenames->data[i]);
                return NULL;
            }
        }

        if (i == 0 && file->type == PM_FPA_FILE_MASK) {
            if (!pmConfigMaskReadHeader(config, phu)) {
                psError(psErrorCodeLast(), false, "Error reading mask bits");
                psFree(phu);
                return NULL;
            }
        }

        if (bind || i > 0) {
            if (format) {
                bool valid = false;
                if (!pmConfigValidateCameraFormat(&valid, format, phu)) {
                    psError(psErrorCodeLast(), false, "Error in config scripts\n");
                    psFree(phu);
                    return NULL;
                }
                if (!valid) {
                    psError(psErrorCodeLast(), false, "File %s is not from the required camera",
                            (char*)filenames->data[i]);
                    psFree(phu);
                    return NULL;
                }
            } else {
                format = pmConfigCameraFormatFromHeader(NULL, NULL, NULL, config, phu, true);
                if (!format) {
                    psError(psErrorCodeLast(), false, "Failed to determine camera format from %s",
                            (char*)filenames->data[i]);
                    psFree(phu);
                    return NULL;
                }
            }
        }

        // Ensure the format is set
        if (!file->format) {
            file->format = format;
        }
        if (!file->formatName) {
            file->formatName = formatName;
        }

        // Set the view to the corresponding entry for this phu
        pmFPAview *view = NULL;         // View to PHU
        if (bind) {
            view = pmFPAIdentifySourceFromHeader(bind->fpa, phu, format);
        } else {
            view = pmFPAAddSourceFromHeader(fpa, phu, format);
        }
        psFree(phu);
        phu = NULL;
        if (!view) {
            psError(PM_ERR_CONFIG, true, "Unable to determine source for %s", name);
            return NULL;
        }

        // Associate the filename with the FPA element
        psString location = pmFPAfileNameFromRule(file->filesrc, file, view);
        psFree(view);
        psMetadataAddStr(file->names, PS_LIST_TAIL, location, 0, "Location of file", filenames->data[i]);
        psFree(location);
    }

    return file;
}

// find the file associated with the argname & generate a pmFPAfile for it based on the filerule
pmFPAfile *pmFPAfileDefineFromArgs(bool *success, pmConfig *config, const char *filename, const char *argname)
{
    PS_ASSERT_PTR_NON_NULL(config, NULL);
    PS_ASSERT_STRING_NON_EMPTY(filename, NULL);
    PS_ASSERT_STRING_NON_EMPTY(argname, NULL);

    // Search the argument data for the named fileset (argname)
    bool status;                        // Status of MD lookup
    psArray *filenames = psMetadataLookupPtr(&status, config->arguments, argname); // Filenames for file
    if (!status) {
        if (success) {
            *success = true;
        }
        return NULL;
    }
    if (filenames->n == 0) {
        psError(PM_ERR_CONFIG, true, "No files in array in %s in arguments", argname);
        if (success) {
            *success = false;
        }
        return NULL;
    }

    pmFPAfile *file = fpaFileDefineFromArray(NULL, config, NULL, filename, filenames); // File of interest

    if (success) {
        *success = file ? true : false;
    }

    return file;
}

// find the file associated with the argname & bind it to the given pmFPAfile for it based on the filerule
pmFPAfile *pmFPAfileBindFromArgs(bool *success, pmFPAfile *input, pmConfig *config, const char *filename, const char *argname)
{
    PS_ASSERT_PTR_NON_NULL(input, NULL);
    PS_ASSERT_PTR_NON_NULL(config, NULL);
    PS_ASSERT_STRING_NON_EMPTY(filename, NULL);
    PS_ASSERT_STRING_NON_EMPTY(argname, NULL);

    // Search the argument data for the named fileset (argname)
    bool status;                        // Status of MD lookup
    psArray *filenames = psMetadataLookupPtr(&status, config->arguments, argname); // Filenames for file
    if (!status) {
        if (success) {
            *success = true;
        }
        return NULL;
    }
    if (filenames->n == 0) {
        psError(PM_ERR_CONFIG, true, "No files in array in %s in arguments", argname);
        if (success) {
            *success = false;
        }
        return NULL;
    }

    pmFPAfile *file = fpaFileDefineFromArray(NULL, config, input, filename, filenames); // File of interest

    if (success) {
        *success = file ? true : false;
    }

    return file;
}

// find the specific file associated with the argname & generate a pmFPAfile for it based on the filerule
pmFPAfile *pmFPAfileDefineSingleFromArgs(bool *success, pmConfig *config, const char *filename, const char *argname, int entry)
{
    PS_ASSERT_PTR_NON_NULL(config, NULL);
    PS_ASSERT_STRING_NON_EMPTY(filename, NULL);
    PS_ASSERT_STRING_NON_EMPTY(argname, NULL);

    // Search the argument data for the named fileset (argname)
    bool status;                        // Status from MD lookup
    psArray *filenames = psMetadataLookupPtr(&status, config->arguments, argname); // Filenames for file
    if (!status) {
        if (success) {
            *success = true;
        }
        return NULL;
    }
    if (filenames->n <= entry) {
        psError(PM_ERR_CONFIG, true, "Insufficient files (%ld) in array in %s in arguments",
                filenames->n, argname);
        if (success) {
            *success = false;
        }
        return NULL;
    }

    psArray *single = psArrayAlloc(1);  // Array of single filename of interest
    single->data[0] = psMemIncrRefCounter(filenames->data[entry]);
    pmFPAfile *file = fpaFileDefineFromArray(NULL, config, NULL, filename, single); // File of interest
    psFree(single);

    if (success) {
        *success = file ? true : false;
    }

    return file;
}

// find the file in the config list & generate a pmFPAfile for it based on the filerule
// return values (return, status):
// (not NULL, true) : file defined on RUN and can be loaded
// (NULL, true) : file not defined on RUN
// (NULL, false) : file defined on RUN, cannot be loaded
pmFPAfile *pmFPAfileDefineFromRun(bool *success, pmFPAfile *bind, pmConfig *config, const char *filename)
{
    PS_ASSERT_PTR_NON_NULL(config, NULL);
    PS_ASSERT_STRING_NON_EMPTY(filename, NULL);

    psArray *filenames = pmConfigRunFileGet(config, filename); // Filenames used, or NULL
    if (!filenames) {
        if (success) {
            *success = true;
        }
        return NULL;
    }

    pmFPAfile *file = fpaFileDefineFromArray(NULL, config, bind, filename, filenames); // File of interest
    psFree(filenames);

    if (success) {
        *success = file ? true : false;
    }

    return file;
}

// find the files in the config list & generate an array of pmFPAfiles for them based on the filerule
psArray *pmFPAfileDefineMultipleFromRun(bool *success, psArray *bind, pmConfig *config, const char *filename)
{
    PS_ASSERT_PTR_NON_NULL(config, NULL);
    PS_ASSERT_STRING_NON_EMPTY(filename, NULL);

    if (success) {
        *success = false;
    }

    psArray *files = pmConfigRunFileGet(config, filename); // Filenames used, to return
    if (!files || files->n == 0) {
        if (success) {
            *success = true;
        }
        return NULL;
    }
    if (bind && files->n != bind->n) {
        psError(PM_ERR_CONFIG, true,
                "Length of filenames (%ld) and bind files (%ld) does not match.",
                files->n, bind->n);
        psFree(files);
        return NULL;
    }

    psArray *dummy = psArrayAlloc(1);   // Dummy array of single filename
    for (int i = 0; i < files->n; i++) {
        psFree(dummy->data[0]);
        dummy->data[0] = files->data[i];
        pmFPAfile *bindFile = bind ? bind->data[i] : NULL; // File to which to bind
        files->data[i] = psMemIncrRefCounter(fpaFileDefineFromArray(NULL, config, bindFile, filename, dummy));
        if (!files->data[i]) {
            psError(psErrorCodeLast(), false, "Unable to define file %s %d", filename, i);
            psFree(dummy);
            psFree(files);
            return NULL;
        }
    }
    psFree(dummy);

    if (success) {
        *success = true;
    }

    return files;
}

// find the file associated with the argname & generate a pmFPAfile for it based on the filerule
pmFPAfile *pmFPAfileDefineNewConfig(bool *success, pmConfig **outConfig, pmConfig *sysConfig, const char *filename, const char *argname)
{
    PS_ASSERT_PTR_NON_NULL(outConfig, NULL);
    PS_ASSERT_PTR_NON_NULL(sysConfig, NULL);
    PS_ASSERT_STRING_NON_EMPTY(filename, NULL);
    PS_ASSERT_STRING_NON_EMPTY(argname, NULL);

    // Search the argument data for the named fileset (argname)
    bool status;                        // Status of MD lookup
    psArray *filenames = psMetadataLookupPtr(&status, sysConfig->arguments, argname); // Filenames for file
    if (!status) {
        if (success) {
            *success = true;
        }
        return NULL;
    }
    if (filenames->n == 0) {
        psError(PM_ERR_CONFIG, true, "No files in array in %s in arguments", argname);
        if (success) {
            *success = false;
        }
        return NULL;
    }

    pmFPAfile *file = fpaFileDefineFromArray(outConfig, sysConfig, NULL, filename, filenames); // File of interest

    if (success) {
        *success = file ? true : false;
    }

    return file;
}

// define the named pmFPAfile from the camera->config
// only valid for pmFPAfile->mode = READ
pmFPAfile *pmFPAfileDefineFromConf(bool *success, const pmConfig *config, const char *filename)
{
    PS_ASSERT_PTR_NON_NULL(config, false);
    PS_ASSERT_STRING_NON_EMPTY(filename, NULL);

    if (success) {
        *success = false;
    }

    // a camera config is needed (as source of file rule)
    if (config->camera == NULL) {
        psError(PM_ERR_PROG, true, "camera is not defined");
        return NULL;
    }

    // build the template fpa, set up the basic view
    pmFPA *fpa = pmFPAConstruct(config->camera, config->cameraName);
    if (!fpa) {
        psError(psErrorCodeLast(), false, "Failed to construct FPA for %s", filename);
        return NULL;
    }

    // load the given filerule (from config->camera) and bind it to the fpa
    // the returned file is just a view to the entry on config->files
    pmFPAfile *file = pmFPAfileDefineInput(config, fpa, NULL, filename);
    psFree (fpa);
    if (!file) {
        psError(psErrorCodeLast(), false, "file %s not defined\n", filename);
        return NULL;
    }

    // image names may not come from file->names
    if (!strcasecmp(file->filerule, "@FILES")) {
        psError(PM_ERR_CONFIG, true, "supplied filerule uses illegal value @FILES");
        // XXX remove the file from config->files
        return NULL;
    }

    // image names may come from the detrend database
    if (!strcasecmp(file->filerule, "@DETDB")) {
        psTrace ("pmFPAfile", 5, "requiring use of detrend database source\n");
        // don't free the file here: it is left on config->files
        // to be used optionally by pmFPAfileDefineFromDetDB (or others) XXX potentially free the fpa...
        if (success) {
            *success = true;
        }
        return NULL;
    }

    // Prepend the global path to the file rule
    // this function is implicitly an INPUT operation: do not create the file
    psString tmpName = pmConfigConvertFilename(file->filerule, config, false, false);
    psFree (file->filerule);
    file->filerule = tmpName;

    if (success) {
        *success = true;
    }

    return file;
}

// construct an FPA based on the supplied config->camera
// built the association between the FPA elements (CHIP/CELL) and the files
// define the pmFPAfile filename and bind it to this FPA
// save the pmFPAfile on config->files
// return the pmFPAfile (a view to the one saved on config->files)
pmFPAfile *pmFPAfileDefineFromDetDB (bool *success, const pmConfig *config, const char *filename,
                                     pmFPA *input, pmDetrendType type)
{
    PS_ASSERT_PTR_NON_NULL(input, NULL);
    PS_ASSERT_PTR_NON_NULL(config, NULL);
    PS_ASSERT_PTR_NON_NULL(config->camera, NULL);
    PS_ASSERT_PTR_NON_NULL(config->files, NULL);
    PS_ASSERT_STRING_NON_EMPTY(filename, NULL);

    bool status;
    pmFPA *fpa = NULL;
    pmFPAfile *file = NULL;

    if (type == PM_DETREND_TYPE_NONE) {
        return NULL;
    }

    if (success) {
        *success = false;
    }

    // a camera config is needed (as source of file rule)
    if (config->camera == NULL) {
        psError(PM_ERR_PROG, true, "camera is not defined");
        return NULL;
    }
    // a camera config is needed (as source of file rule)
    if (config->cameraName == NULL) {
        psAbort("camera defined but not cameraName!");
    }

    // find or define a pmFPAfile with this name
    file = psMetadataLookupPtr (NULL, config->files, filename);
    if (!file) {
        // build the template fpa, set up the basic view
        fpa = pmFPAConstruct(config->camera, config->cameraName);
        if (!fpa) {
            psError(psErrorCodeLast(), false, "Failed to construct FPA for %s", filename);
            return NULL;
        }
        // load the given filerule (from config->camera) and bind it to the fpa
        // the returned file is just a view to the entry on config->files
        file = pmFPAfileDefineInput (config, fpa, NULL, filename);
        if (!file) {
            psError(psErrorCodeLast(), false, "file %s not defined\n", filename);
            psFree(fpa);
            return NULL;
        }

        // XXX A TEST: this is a provisional fpa until we read the first header for this pmFPAfile
        // we are going to replace it when we determine the true file.  blow this away here...
        psFree (file->fpa);
        file->fpa = NULL;
    }

    // we are constructing a detselect command of the form:
    //   detselect -search -inst (camera) -type (type) -time (time) [others]
    // camera, type, and time are derived from pmFPA *input, other options are
    // added if specified for the particular detrend type by the DETREND.CONSTRAINTS
    // note that the filter-dependent choices are set for ppImage in ppImageParseCamera
    // XXX make all of the detrend constraints explicit in DETREND.CONSTRAINTS?

    // Get the time from FPA.TIME
    psTime *time = psMetadataLookupPtr(NULL, input->concepts, "FPA.TIME");
    if (time->sec == 0 && time->nsec == 0) {
        psLogMsg ("psModules.camera", PS_LOG_WARN, "FPA.TIME has not been set.\n");
    }

    // XXX careful about this: is this set correctly in the camera.config files?
    char *cameraName = psMetadataLookupStr(NULL, input->concepts, "FPA.CAMERA");
    pmDetrendSelectOptions *options = pmDetrendSelectOptionsAlloc(cameraName, *time, type);

    // add additional constraints based on the type defined in the PPIMAGE recipe
    // XXX use PPIMAGE or DETREND for the recipe name?
    psMetadata *recipe  = psMetadataLookupPtr (&status, config->recipes, "PPIMAGE");
    if (!status) {
        psError(PM_ERR_CONFIG, true, "PPIMAGE recipe not found.");
        psFree(options);
        psFree(fpa);
        return false;
    }
    psMetadata *detConstraints = psMetadataLookupPtr (&status, recipe, "DETREND.CONSTRAINTS");
    if (!status) {
        psWarning("DETREND.CONSTRAINTS not found --- no constraints will be applied.");
        goto DETREND_SELECT;
    }

    psString typeName = pmDetrendTypeToString (type);
    psMetadata *constraints = psMetadataLookupPtr (&status, detConstraints, typeName);
    if (!status) {
        psWarning("DETREND.CONSTRAINTS for type %s not found --- no contraints will be applied.", typeName);
        psFree(typeName);
        goto DETREND_SELECT;
    }
    psFree(typeName);

    // loop over the constraints and include in the detselect options
    psMetadataIterator *iter = psMetadataIteratorAlloc (constraints, PS_LIST_HEAD, NULL);
    psMetadataItem *item = NULL;
    while ((item = psMetadataGetAndIncrement (iter)) != NULL) {
        if (item->type != PS_DATA_STRING) {
            psWarning("Invalid type for DETREND.CONSTRAINT element %s --- ignoring constraint", item->name);
            continue;
        }
        char *option  = item->name;     // item->name must correspond to a valid detselect option
        char *concept = item->data.V;

        // these items refer to the corresponding values for the input image
        // (ie, -filter input:filter or -exptime input:exptime)
        if (!strcasecmp (option, "filter")) {
            options->filter = psMetadataLookupPtr (&status, input->concepts, concept);
            psMemIncrRefCounter (options->filter);
            if (!status)
                psAbort("failed to find filter (concept %s)", concept);
        } else if (!strcasecmp (option, "exptime")) {
            options->exptime = psMetadataLookupF32 (&status, input->concepts, concept);
            options->exptimeSet = true;
            if (!status)
                psAbort("exptime not found (concept %s)", concept);
        } else if (!strcasecmp (option, "airmass")) {
            options->airmass = psMetadataLookupF32 (&status, input->concepts, concept);
            options->airmassSet = true;
            if (!status)
                psAbort("airmass not found (concept %s)", concept);
        } else if (!strcasecmp (option, "dettemp")) {
            options->dettemp = psMetadataLookupF32 (&status, input->concepts, concept);
            options->dettempSet = true;
            if (!status)
                psAbort("dettemp not found (concept %s)", concept);
        } else if (!strcasecmp (option, "twilight")) {
            options->twilight = psMetadataLookupF32 (&status, input->concepts, concept);
            options->twilightSet = true;
            if (!status)
                psAbort("twilight not found (concept %s)", concept);
        }

        // the version is applied literally
        if (!strcasecmp (option, "version")) {
            options->version = psMemIncrRefCounter (concept);
        }
        // we can override the detrend database dettype if desired
        // ie, use DOMEFLAT for type FLAT
        // the dettype string is applied literally
        if (!strcasecmp (option, "dettype")) {
            options->dettype = psMemIncrRefCounter (concept);
        }
    }
    psFree(iter);

DETREND_SELECT:
    {
        // search for existing detrend data (detID)
        pmDetrendSelectResults *results = pmDetrendSelect (options, config);
        if (!results) {
            psError (psErrorCodeLast(), false, "no matching detrend data");
            return NULL;
        }
        file->detrend = results;
        file->fileLevel = pmFPALevelFromName(results->level);
        if (file->fileLevel == PM_FPA_LEVEL_NONE) {
            psError (PM_ERR_CONFIG, false, "invalid file level for selected detrend data");
            return NULL;
        }
    }

    psFree (options);

    if (success) {
        *success = true;
    }
    return file;
}

// create a new output pmFPAfile based on an existing FPA
// only valid for pmFPAfile->mode == WRITE (or internal?)
pmFPAfile *pmFPAfileDefineFromFPA (const pmConfig *config, pmFPA *src, int xBin, int yBin, const char *filename)
{
    PS_ASSERT_PTR_NON_NULL(config, false);
    PS_ASSERT_PTR_NON_NULL(src, false);
    PS_ASSERT_STRING_NON_EMPTY(filename, NULL);

    pmFPA *fpa = pmFPAConstruct(src->camera, psMetadataLookupStr(NULL, src->concepts, "FPA.CAMERA"));
    // XXX should this use DefineOutputForFormat?
    pmFPAfile *file = pmFPAfileDefineOutput (config, fpa, filename);
    if (!file) {
        psError(psErrorCodeLast(), false, "file %s not defined\n", filename);
        return NULL;
    }
    file->src = psMemIncrRefCounter(src); // inherit output elements from this source pmFPA
    file->xBin = xBin;
    file->yBin = yBin;
    psFree (fpa);
    return file;
}

pmFPAfile *pmFPAfileDefineFromFile(const pmConfig *config, pmFPAfile *src, int xBin, int yBin,
                                   const char *filename)
{
    PS_ASSERT_PTR_NON_NULL(config, false);
    PS_ASSERT_PTR_NON_NULL(src, false);
    PS_ASSERT_STRING_NON_EMPTY(filename, NULL);

    pmFPAfile *file = pmFPAfileDefineOutputForFormat(config, NULL, filename, src->cameraName, src->formatName);
    if (!file) {
        psError(psErrorCodeLast(), false, "file %s not defined\n", filename);
        return NULL;
    }
    file->src = psMemIncrRefCounter(src->fpa); // inherit output elements from this source pmFPA
    file->xBin = xBin;
    file->yBin = yBin;

    // inherit the concepts from the src fpa:
    pmConceptsCopyFPA(file->fpa, file->src, true, true);

    return file;
}

// create a new output pmFPAfile based on an existing FPA
// only valid for pmFPAfile->mode == WRITE (or internal?)
pmFPAfile *pmFPAfileDefineNewCamera (const pmConfig *config, const char *filename)
{
    PS_ASSERT_PTR_NON_NULL(config, false);
    PS_ASSERT_STRING_NON_EMPTY(filename, NULL);

    pmFPAfile *file = pmFPAfileDefineOutput (config, NULL, filename);
    if (!file) {
        psError(psErrorCodeLast(), false, "file %s not defined\n", filename);
        return NULL;
    }
    if (!file->camera) {
        psError(PM_ERR_CONFIG, false, "file %s does not define a new camera\n", filename);
        return NULL;
    }
    file->fpa = pmFPAConstruct(file->camera, file->cameraName);

    return file;
}

pmFPAfile *pmFPAfileDefineSkycell(const pmConfig *config, pmFPA *fpa, const char *filename)
{
    PS_ASSERT_PTR_NON_NULL(config, NULL);
    PS_ASSERT_STRING_NON_EMPTY(filename, NULL);
    PS_ASSERT_STRING_NON_EMPTY(config->cameraName, NULL);
    PS_ASSERT_STRING_NON_EMPTY(config->formatName, NULL);

    pmFPAfile *file;                    // The new file

    if (config->cameraName[0] == '_' &&
        strcmp(config->cameraName + strlen(config->cameraName) - 8, "-SKYCELL") == 0) {
        // The input camera is already a skycell
        file = pmFPAfileDefineOutputForFormat(config, fpa, filename, config->cameraName, "SKYCELL");
    } else {
        psString cameraName = NULL;         // Name of the old camera configuration
        if (config->cameraName[0] == '_' &&
            strcmp(config->cameraName + strlen(config->cameraName) - 5, "-CHIP") == 0) {
            cameraName = psStringNCopy(config->cameraName + 1, strlen(config->cameraName) - 6);
        } else if (config->cameraName[0] == '_' &&
                   strcmp(config->cameraName + strlen(config->cameraName) - 4 , "-FPA") == 0) {
            cameraName = psStringNCopy(config->cameraName + 1, strlen(config->cameraName) - 5);
        } else {
            cameraName = psMemIncrRefCounter(config->cameraName);
        }
        psString newCameraName = NULL;  // Name of the new (automatically-generated) camera configuration
        psStringAppend(&newCameraName, "_%s-SKYCELL", cameraName);
        file = pmFPAfileDefineOutputForFormat(config, fpa, filename, newCameraName, "SKYCELL");
        psFree(cameraName);
        psFree(newCameraName);
    }
    if (!file) {
        psError(PM_ERR_CONFIG, true, "file %s not defined\n", filename);
        return NULL;
    }

    // Ensure everything is written out at the appropriate level
    file->fileLevel = PM_FPA_LEVEL_FPA;
    file->dataLevel = PM_FPA_LEVEL_FPA;
    file->freeLevel = PM_FPA_LEVEL_FPA;

    return file;
}

pmFPAfile *pmFPAfileDefineChipMosaic(const pmConfig *config, pmFPA *src, const char *filename)
{
    PS_ASSERT_PTR_NON_NULL(config, NULL);
    PS_ASSERT_PTR_NON_NULL(src, NULL);
    PS_ASSERT_STRING_NON_EMPTY(filename, NULL);
    PS_ASSERT_STRING_NON_EMPTY(config->cameraName, NULL);
    PS_ASSERT_STRING_NON_EMPTY(config->formatName, NULL);

    pmFPAfile *file;                    // The new file
    if (config->cameraName[0] == '_' &&
        (strcmp(config->cameraName + strlen(config->cameraName) - 5, "-CHIP") == 0 ||
         strcmp(config->cameraName + strlen(config->cameraName) - 8, "-SKYCELL") == 0)) {
        // The input camera has already been mosaicked to this level
        file = pmFPAfileDefineOutputForFormat(config, NULL, filename, config->cameraName, config->formatName);
    } else {
        psString cameraName = NULL; // Name of the new (automatically-generated) camera configuration
        if (config->cameraName[0] == '_' &&
            strcmp(config->cameraName + strlen(config->cameraName) - 4 , "-FPA") == 0) {
            cameraName = psStringNCopy(config->cameraName + 1, strlen(config->cameraName) - 5);
        } else {
            cameraName = psMemIncrRefCounter(config->cameraName);
        }
        psString newCameraName = NULL;  // Name of the new (automatically-generated) camera configuration
        psStringAppend(&newCameraName, "_%s-CHIP", cameraName);

        // Find the correct camera configuration
        file = pmFPAfileDefineOutputForFormat(config, NULL, filename, newCameraName, config->formatName);
        psFree(newCameraName);
        psFree(cameraName);
    }
    if (!file) {
        psError(PM_ERR_CONFIG, true, "file %s not defined\n", filename);
        return NULL;
    }

    file->src = psMemIncrRefCounter(src); // inherit output elements from this source pmFPA
    if (src) {
        if (!pmConceptsCopyFPA(file->fpa, src, true, false)) {
            psError(psErrorCodeLast(), false, "Unable to copy concepts from source to new FPA");
            return NULL;
        }
    }

    file->mosaicLevel = PM_FPA_LEVEL_CHIP; // don't do any I/O on this at a lower level

    return file;
}

pmFPAfile *pmFPAfileDefineFPAMosaic(const pmConfig *config, pmFPA *src, const char *filename)
{
    PS_ASSERT_PTR_NON_NULL(config, NULL);
    PS_ASSERT_PTR_NON_NULL(src, NULL);
    PS_ASSERT_STRING_NON_EMPTY(filename, NULL);
    PS_ASSERT_STRING_NON_EMPTY(config->cameraName, NULL);

    pmFPAfile *file;                    // The new file
    if (config->cameraName[0] == '_' &&
        (strcmp(config->cameraName + strlen(config->cameraName) - 4 , "-FPA") == 0 ||
         strcmp(config->cameraName + strlen(config->cameraName) - 8, "-SKYCELL") == 0)) {
        // The input camera has already been mosaicked to this level
        file = pmFPAfileDefineOutputForFormat(config, NULL, filename, config->cameraName, config->formatName);
    } else {

        psString original = NULL;       // Name of the original camera configuration
        if (config->cameraName[0] == '_' &&
            strcmp(config->cameraName + strlen(config->cameraName) - 5 , "-CHIP") == 0) {
            // It's a chip mosaic; we need to get the original name
            original = psStringNCopy(config->cameraName + 1, strlen(config->cameraName) - 6);
        } else if (config->cameraName[0] == '_' &&
            strcmp(config->cameraName + strlen(config->cameraName) - 8, "-SKYCELL") == 0) {
            original = psStringNCopy(config->cameraName + 1, strlen(config->cameraName) - 9);
        } else {
            original = psMemIncrRefCounter(config->cameraName);
        }
        psString cameraName = NULL;
        psStringAppend(&cameraName, "_%s-FPA", original);
        psFree(original);

        file = pmFPAfileDefineOutputForFormat(config, NULL, filename, cameraName, config->formatName);
        psFree(cameraName);
    }
    if (!file) {
        psError(PM_ERR_CONFIG, true, "file %s not defined\n", filename);
        return NULL;
    }

    file->src = psMemIncrRefCounter(src); // inherit output elements from this source pmFPA
    if (src) {
        if (!pmConceptsCopyFPA(file->fpa, src, false, false)) {
            psError(psErrorCodeLast(), false, "Unable to copy concepts from source to new FPA");
            return NULL;
        }
    }

    file->mosaicLevel = PM_FPA_LEVEL_FPA; // don't do any I/O on this at a lower level

    return file;
}

// create a file with the given name, assign it type "INTERNAL", and supply it with an image
// of the requested dimensions. (image only, mask and weight are ignored)
pmReadout *pmFPAfileDefineInternal (psMetadata *files, const char *name, int Nx, int Ny, int type)
{
    PS_ASSERT_PTR_NON_NULL(files, false);
    PS_ASSERT_STRING_NON_EMPTY(name, NULL);

    pmReadout *readout = pmReadoutAlloc(NULL);
    readout->image = psImageAlloc(Nx, Ny, type);

    // I want an image from the
    pmFPAfile *file = pmFPAfileAlloc();
    file->mode = PM_FPA_MODE_INTERNAL;
    file->name = psStringCopy (name);

    // free a previously existing readout
    psFree(file->readout);
    file->readout = readout;

    // allow for multiple entries
    // XXX handle replace vs multiple?
    psMetadataAddPtr(files, PS_LIST_TAIL, name, PS_DATA_UNKNOWN | PS_META_DUPLICATE_OK, "", file);
    psFree(file);
    // we free this copy of file, but 'files' still has a copy

    return readout;
}

bool pmFPAfileDropInternal(psMetadata *files, const char *name)
{
    PS_ASSERT_PTR_NON_NULL(files, false);
    PS_ASSERT_STRING_NON_EMPTY(name, NULL);

    bool status = false;

    pmFPAfile *file = psMetadataLookupPtr(&status, files, name);
    if (!status) {
        psTrace("psModules.camera", 6, "Internal File %s not in file list", name);
        return true;
    }
    if (file == NULL) {
        psError(PM_ERR_CONFIG, true, "file %s is NULL", name);
        return false;
    }
    if (file->mode != PM_FPA_MODE_INTERNAL) {
        psTrace("psModules.camera", 6, "FPA File %s not Internal, not dropping", name);
        return true;
    }

    psTrace("psModules.camera", 6, "dropping Internal FPA File %s", name);
    psMetadataRemoveKey(files, name);
    return true;
}

// Select or construct the requested readout.  If the named entry does not exist, generate it based
// on the specified fpa and binning.  We have 4 possibilities: (INTERNAL or I/O file) and (exists or
// not).  This call is used after all user-requested pmFPAfiles have been generated.  A missing
// pmFPAfile is being used internally.
pmReadout *pmFPAGenerateReadout(const pmConfig *config, // configuration information
                                const pmFPAview *view, // select background for this entry
                                const char *name, // name of internal/external file
                                const pmFPA *fpa, // use this fpa to generate
                                const psImageBinning *binning,
                                int index) {
  pmReadout *readout = NULL;

  pmFPAfile *file = pmFPAfileSelectSingle(config->files, name, index);

  // if the file does not exist, it is not being used as an I/O file: define an internal version
  if (file == NULL) {
      // XXX currently, we do not guarantee that the defined file lands on entry 'index'
      psAssert (binning, "internal files must be supplied a psImageBinning for the output images size"); 
      readout = pmFPAfileDefineInternal (config->files, name, binning->nXruff, binning->nYruff, PS_TYPE_F32);
      return readout;
  }

  // if the mode is INTERNAL, it has been defined in a previous call.  XXX This seems to require
  // that the readout have the same dimensions for all entries.
  if (file->mode == PM_FPA_MODE_INTERNAL) {
    readout = file->readout;
    return readout;
  }

  // we are using this pmFPAfile as an I/O file: select readout or create
  readout = pmFPAviewThisReadout (view, file->fpa);
  if (readout == NULL) {
    // readout does not yet exist: create from input
    // XXX we have an inconsistency in this calculation here and in pmFPACopy
    // XXX use the psImageBinning functions to set the output image size
    if (binning == NULL) {
      pmFPAfileCopyStructureView (file->fpa, fpa, 1, 1, view);
      readout = pmFPAviewThisReadout (view, file->fpa);
    } else {
      pmFPAfileCopyStructureView (file->fpa, fpa, binning->nXbin, binning->nYbin, view);
      readout = pmFPAviewThisReadout (view, file->fpa);
      PS_ASSERT (binning->nXruff == readout->image->numCols, false);
      PS_ASSERT (binning->nYruff == readout->image->numRows, false);
    }
  }

  return readout;
}

