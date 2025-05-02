# include "ppSim.h"

// Print usage information and die
static void usage(const char *program,  // Name of the program
                  psMetadata *arguments, // Command-line arguments
                  pmConfig *config      // Configuration
    )
{
    fprintf(stderr, "\nPan-STARRS data simulator\n\n");
    fprintf(stderr, "Usage: %s -camera CAMERA_NAME (output)\n", program);
    fprintf(stderr, "\n");
    psArgumentHelp(arguments);
    psFree(arguments);
    psFree(config);
    pmConfigDone();
    psLibFinalize();
    exit(PS_EXIT_CONFIG_ERROR);
}

// this function supplements the RECIPE:OPTIONS folder with command-line options
bool ppSimArguments(int argc, char **argv, pmConfig *config)
{
    bool status;

    assert(config);

    // save the following command-line options in the arguments structure.  these will later be
    // parsed and moved to the config->recipes:PPSIM_RECIPE folder

    psMetadata *arguments = psMetadataAlloc(); // Command-line arguments
    psMetadataAddStr(arguments,  PS_LIST_TAIL, "-format", 0, "Camera format name", NULL);
    psMetadataAddStr(arguments,  PS_LIST_TAIL, "-type", 0, "Exposure type (BIAS|DARK|FLAT|OBJECT)", NULL);
    psMetadataAddStr(arguments,  PS_LIST_TAIL, "-obs_mode", 0, "observation mode", NULL);
    psMetadataAddStr(arguments,  PS_LIST_TAIL, "-filter", 0, "Filter name", NULL);
    psMetadataAddF32(arguments,  PS_LIST_TAIL, "-exptime", 0, "Exposure time (s)", NAN);
    psMetadataAddF32(arguments,  PS_LIST_TAIL, "-biaslevel", 0, "Bias level (e)", NAN);
    psMetadataAddF32(arguments,  PS_LIST_TAIL, "-biasrange", 0, "Bias range (e)", NAN);
    psMetadataAddF32(arguments,  PS_LIST_TAIL, "-dettemp", 0, "Detector Temperature (C)", NAN);
    psMetadataAddF32(arguments,  PS_LIST_TAIL, "-darkrate", 0, "Dark rate (e/s)", NAN);
    psMetadataAddF32(arguments,  PS_LIST_TAIL, "-flatsigma", 0, "Flat sigma", NAN);
    psMetadataAddF32(arguments,  PS_LIST_TAIL, "-flatrate", 0, "Flat rate (e/s)", NAN);
    psMetadataAddF32(arguments,  PS_LIST_TAIL, "-shuttertime", 0, "Shutter time (s)", NAN);
    psMetadataAddF32(arguments,  PS_LIST_TAIL, "-skyrate", 0, "Sky rate (e/s)", NAN);
    psMetadataAddF32(arguments,  PS_LIST_TAIL, "-skymags", 0, "Sky brightness in mags / square arcsec", NAN);
    psMetadataAddF32(arguments,  PS_LIST_TAIL, "-ra", 0, "RA (degrees)", NAN);
    psMetadataAddF32(arguments,  PS_LIST_TAIL, "-dec", 0, "Dec (degrees)", NAN);
    psMetadataAddF32(arguments,  PS_LIST_TAIL, "-pa", 0, "Position angle (degrees)", NAN);
    psMetadataAddF32(arguments,  PS_LIST_TAIL, "-scale", 0, "Plate scale (arcsec/pixel)", NAN);
    psMetadataAddF32(arguments,  PS_LIST_TAIL, "-zp", 0, "Photometric zero point", NAN);
    psMetadataAddF32(arguments,  PS_LIST_TAIL, "-seeing", 0, "Seeing FWHM (arcsec)", NAN);
    psMetadataAddF32(arguments,  PS_LIST_TAIL, "-seeing-max", 0, "Seeing FWHM (arcsec) at end of ramp", NAN);
    psMetadataAddBool(arguments,  PS_LIST_TAIL, "-seeing-ramp", 0, "Use a seeing ramp", false);
    psMetadataAddF32(arguments,  PS_LIST_TAIL, "-starslum", 0, "Fake star luminosity function slope", NAN);
    psMetadataAddF32(arguments,  PS_LIST_TAIL, "-starsmag", 0, "Brightest magnitude for fake stars", NAN);
    psMetadataAddF32(arguments,  PS_LIST_TAIL, "-starsdensity", 0, "Density of fake stars at magnitude", NAN);
    psMetadataAddStr(arguments,  PS_LIST_TAIL, "-psfclass", 0, "Type of PSF model", NULL);
    psMetadataAddStr(arguments,  PS_LIST_TAIL, "-galmodel", 0, "Type of Galaxy model", NULL);
    psMetadataAddS32(arguments,  PS_LIST_TAIL, "-bin", 0, "Binning in x and y", 1);
    psMetadataAddS32(arguments,  PS_LIST_TAIL, "-nx", 0, "cell x-size in pixels", 0);
    psMetadataAddS32(arguments,  PS_LIST_TAIL, "-ny", 0, "cell y-size in pixels", 0);
    psMetadataAddBool(arguments, PS_LIST_TAIL, "+photom", 0, "Perform photometry on fake sources", false);

    if (psArgumentGet (argc, argv, "-h")) { usage(argv[0], arguments, config); }
    if (psArgumentGet (argc, argv, "--h")) { usage(argv[0], arguments, config); }
    if (psArgumentGet (argc, argv, "--help")) { usage(argv[0], arguments, config); }

    pmConfigFileSetsMD (config->arguments, &argc, argv, "PSPHOT.PSF", "-psf", "-psflist");
    if (psErrorCodeLast () != PS_ERR_NONE) { psAbort ("problem with -psf or -psflist option"); }

    pmConfigFileSetsMD (config->arguments, &argc, argv, "INPUT.SOURCES", "-cmf", "-cmflist");
    if (psErrorCodeLast () != PS_ERR_NONE) { psAbort ("problem with -cmf or -cmflist option"); }

    // Only one of -camera and -file is needed or allowed.  The -camera option would have been
    // already parsed by pmConfigRead in ppSim.c and resulted in a value for config->camera
    bool loadImage = pmConfigFileSetsMD (config->arguments, &argc, argv, "INPUT", "-input", "-inputlist");
    if (!config->camera && !loadImage) {
	psError(PS_ERR_IO, true, "A camera name (-camera NAME) or an input image (-input NAME) must be specified");
        psFree(arguments);
	return false;
    }
    if (config->camera && loadImage) {
        psError(PS_ERR_IO, true, "Only one of (-camera NAME) and (-file NAME) may be specified");
        psFree(arguments);
	return false;
    }

    if (!psArgumentParse(arguments, &argc, argv)) { 
        psError(PS_ERR_IO, false, "error in command-line arguments");
        psFree(arguments);
	return false;
    }

    if (argc != 2) { 
	psError(PS_ERR_IO, true, "Missing output filename");
        psFree(arguments);
	return false;
    }

    // output filename
    psMetadataAddStr(config->arguments, PS_LIST_TAIL, "OUTPUT", 0, "Name of the output image", argv[1]);

    // save the additional recipe values based on command-line options. These options override
    // the PPSIM recipe values loaded from recipe files
    psMetadata *options = pmConfigRecipeOptions (config, PPSIM_RECIPE);
    if (!options) {
        psError(PS_ERR_IO, false, "Unable to find recipe options for %s", PPSIM_RECIPE);
        psFree(arguments);
	return false;
    }

    // these arguments may be used whether the input image is created or loaded
    ppSimArgToRecipeF32(&status, options, "STARS.LUM",     arguments, "-starslum");
    ppSimArgToRecipeF32(&status, options, "STARS.MAG",     arguments, "-starsmag");
    ppSimArgToRecipeF32(&status, options, "STARS.DENSITY", arguments, "-starsdensity");
    ppSimArgToRecipeBool(&status, options, "PHOTOM",        arguments, "+photom");
    psString obs_mode = psMetadataLookupStr(&status, arguments, "-obs_mode");
    if (obs_mode) {
        psMetadataAddStr(options, PS_LIST_TAIL, "OBS_MODE", 0, "observation mode", obs_mode);
    }


    // if we are loading the input image (not creating it), then we can skip the remaining arguments
    if (loadImage) {
	// if we are supplying an input image, it is so we may supply fake stars; force it to
	// be treated as an OBJECT image. 
	psMetadataAddStr(options, PS_LIST_TAIL, "IMAGE.TYPE", 0, "Exposure type", "OBJECT");

	// check for these options as well
        ppSimArgToRecipeStr(&status, options, "PSF.MODEL",    arguments, "-psfclass"); // PSF model class
        ppSimArgToRecipeStr(&status, options, "GALAXY.MODEL", arguments, "-galmodel"); // Galaxy model name

	// 'seeing' is not required: we can load a psf-model instead; but if not, it is allowed
        ppSimArgToRecipeF32(&status, options, "SEEING", arguments, "-seeing"); // seeing (FWHM in arcsec)
        ppSimArgToRecipeF32(&status, options, "SEEING.MAX", arguments, "-seeing-max"); // seeing (FWHM in arcsec)
        ppSimArgToRecipeBool(&status, options, "SEEING.RAMP", arguments, "-seeing-ramp"); // seeing (FWHM in arcsec)

	// 'scale' is not required: we can use the WCS instead; but if not, it is allowed
        ppSimArgToRecipeF32(&status, options, "PIXEL.SCALE", arguments, "-scale"); // Plate scale

	// XXX we need to be more flexible: get this from the input image header or WCS
        float ra0     = psMetadataLookupF32(NULL, arguments, "-ra"); // Right Ascension of boresight
        float dec0    = psMetadataLookupF32(NULL, arguments, "-dec"); // Declination of boresight
        psMetadataAddF32(options, PS_LIST_TAIL, "RA", 0, "Boresight RA (radians)", ra0 * M_PI / 180.0);
        psMetadataAddF32(options, PS_LIST_TAIL, "DEC", 0, "Boresight Declination (radians)", dec0 * M_PI / 180.0);

	psFree (arguments);
	return true;
    }

    // apply an alternate camera format
    psString formatName = psMetadataLookupStr(NULL, arguments, "-format"); // Name of format
    if (formatName) {
        // XXX delay the config below until ppSimCreate?
        config->formatName = psMemIncrRefCounter(formatName);

        psMetadata *formats = psMetadataLookupMetadata(NULL, config->camera, "FORMATS"); // The camera formats
        if (!formats) {
	    psError(PS_ERR_IO, false, "Unable to find FORMATS in camera configuration.");
            psFree(arguments);
	    return false;
        }
        psMetadata *format = psMetadataLookupMetadata(NULL, formats, formatName); // Format of interest
        if (!format) {
	    psError(PS_ERR_IO, false, "Unable to find format %s in camera FORMATS.", formatName);
            psFree(arguments);
	    return false;
        }
        config->format = psMemIncrRefCounter(format);
    }

    char *typeStr = ppSimArgToRecipeStr (&status, options, "IMAGE.TYPE", arguments, "-type"); // Requested exposure type
    if (typeStr == NULL) {
	psError(PS_ERR_IO, false, "An exposure type must be specified using -type");
        psFree(arguments);
	return false;
    }
    ppSimType type = ppSimTypeFromString (typeStr);
    // XXX handle error

    ppSimArgToRecipeStr(&status, options, "FILTER", arguments, "-filter"); // Filter name

    // set the exposure time 
    if (type == PPSIM_TYPE_BIAS) {
	psMetadataAddF32(options, PS_LIST_TAIL, "EXPTIME", 0, "Exposure time (s)", 0.0);
    } else {
	ppSimArgToRecipeF32(&status, options, "EXPTIME", arguments, "-exptime");
	if (!status) {
	    psError(PS_ERR_IO, false, "The exposure time must be specified using -exptime");
	    psFree(arguments);
	    return false;
        }
    }

    // these values all get moved from arguments to RECIPE:OPTIONS
    ppSimArgToRecipeF32(&status,  options, "BIAS.LEVEL",    arguments, "-biaslevel");
    ppSimArgToRecipeF32(&status,  options, "BIAS.RANGE",    arguments, "-biasrange");
    ppSimArgToRecipeF32(&status,  options, "DARK.RATE",     arguments, "-darkrate");
    ppSimArgToRecipeF32(&status,  options, "DET.TEMP",      arguments, "-dettemp");
    ppSimArgToRecipeF32(&status,  options, "FLAT.SIGMA",    arguments, "-flatsigma");
    ppSimArgToRecipeF32(&status,  options, "FLAT.RATE",     arguments, "-flatrate");
    ppSimArgToRecipeF32(&status,  options, "SHUTTER.TIME",  arguments, "-shuttertime");
    ppSimArgToRecipeF32(&status,  options, "SKY.RATE",      arguments, "-skyrate");
    ppSimArgToRecipeS32(&status,  options, "BINNING",       arguments, "-bin");

    ppSimArgToRecipeS32(&status,  config->arguments, "NX.CELL", arguments, "-nx");
    ppSimArgToRecipeS32(&status,  config->arguments, "NY.CELL", arguments, "-ny");

    if (type == PPSIM_TYPE_OBJECT) {
        // Load values required for adding stars
        float ra0     = psMetadataLookupF32(NULL, arguments, "-ra"); // Right Ascension of boresight
        float dec0    = psMetadataLookupF32(NULL, arguments, "-dec"); // Declination of boresight
        float pa      = psMetadataLookupF32(NULL, arguments, "-pa");  // Position angle
        float seeing  = psMetadataLookupF32(NULL, arguments, "-seeing"); // seeing (FWHM in arcsec)

	// XXX scale and zp should be supplied by the config file (allow override, but this is camera-dependent)
        if (isnan(ra0) || isnan(dec0) || isnan(pa) || isnan(seeing)) {
	    psError(PS_ERR_IO, false, "-ra, -dec, -pa, -scale, -zp, -seeing must be specified for OBJECT type");
	    psFree(arguments);
	    return false;
        }

        ppSimArgToRecipeF32(&status, options, "PIXEL.SCALE", arguments, "-scale"); // Plate scale (arcsec / pixel)
        ppSimArgToRecipeF32(&status, options, "SKY.MAGS", arguments, "-skymags"); // Plate scale
        ppSimArgToRecipeStr(&status, options, "PSF.MODEL",    arguments, "-psfclass"); // PSF model class
        ppSimArgToRecipeStr(&status, options, "GALAXY.MODEL", arguments, "-galmodel"); // Galaxy model name

	// the user supplies FWHM in arcsec; we convert to Sigma in pixels (in ppSimCreate.c:103)
        psMetadataAddF32(options, PS_LIST_TAIL, "SEEING", 0, "Seeing FWHM (arcsec)", seeing);
        psMetadataAddF32(options, PS_LIST_TAIL, "RA", 0, "Boresight RA (radians)", ra0 * M_PI / 180.0);
        psMetadataAddF32(options, PS_LIST_TAIL, "DEC", 0, "Boresight Declination (radians)", dec0 * M_PI / 180.0);
        psMetadataAddF32(options, PS_LIST_TAIL, "PA", 0, "Boresight position angle (radians)",pa * M_PI / 180.0);

        if (psMetadataLookupBool(NULL, arguments, "-seeing-ramp")) {
	    float seeingMax = psMetadataLookupF32(NULL, arguments, "-seeing-max"); // seeing at ramp end
	    psMetadataAddF32(options, PS_LIST_TAIL, "SEEING.MAX", 0, "Seeing FWHM (arcsec)", seeingMax);
	    psMetadataAddBool(options, PS_LIST_TAIL, "SEEING.RAMP", 0, "use seeing ramp", true);
	}

        ppSimArgToRecipeF32(&status, options, "ZEROPOINT", arguments, "-zp"); // Zero point
    }

    psFree(arguments);
    return true;
}

/* the following elements come from the config->arguments:
   
   PSPHOT.PSF
   INPUT
   OUTPUT

   all other values should come from the recipe
*/
