// XXX *REALLY* need generic "concept update" and "concept read" functions that handles the type transparently

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <assert.h>
#include <pslib.h>
#include <string.h>

#include "pmConfig.h"
#include "pmHDU.h"
#include "pmFPA.h"
#include "pmFPALevel.h"
#include "pmHDUUtils.h"
#include "pmConcepts.h"
#include "pmConceptsRead.h"
#include "pmConceptsWrite.h"
#include "pmConceptsStandard.h"

static bool conceptsInitialised = false;// Have concepts been read?
static psMetadata *conceptsFPA = NULL;  // Known concepts for FPA
static psMetadata *conceptsChip = NULL; // Known concepts for chip
static psMetadata *conceptsCell = NULL; // Known concepts for cell

// Return the appropriate concepts metadata, given the level
static psMetadata *conceptsFromLevel(pmFPALevel level)
{
    switch (level) {
    case PM_FPA_LEVEL_FPA:
        return conceptsFPA;
    case PM_FPA_LEVEL_CHIP:
        return conceptsChip;
    case PM_FPA_LEVEL_CELL:
        return conceptsCell;
    default:
        psError(PS_ERR_BAD_PARAMETER_VALUE, true, "Invalid concept level provided: %d\n", level);
        return NULL;
    }
}

// Free a concept
static void conceptSpecFree(pmConceptSpec *spec)
{
    psFree(spec->blank);
}

pmConceptSpec *pmConceptSpecAlloc(psMetadataItem *blank, pmConceptParseFunc parse,
                                  pmConceptFormatFunc format, pmConceptCopyFunc copy,
                                  bool required)
{
    pmConceptSpec *spec = psAlloc(sizeof(pmConceptSpec));
    psMemSetDeallocator(spec, (psFreeFunc)conceptSpecFree);

    spec->blank = psMemIncrRefCounter(blank);
    spec->parse = parse;
    spec->format = format;
    spec->copy = copy;
    spec->required = required;

    return spec;
}

psList *pmConceptsList(pmFPALevel level)
{
    if (!conceptsInitialised) {
        pmConceptsInit();
    }

    // Get the appropriate concepts
    psMetadata *concepts = conceptsFromLevel(level); // Metadata of concepts specs
    if (!concepts) {
        psError(PS_ERR_BAD_PARAMETER_VALUE, true, "Invalid concept level provided: %d\n", level);
        return NULL;
    }

    // Pull out the names
    psList *list = psListAlloc(NULL);   // List of concepts' names
    psMetadataIterator *iter = psMetadataIteratorAlloc(concepts, PS_LIST_HEAD, NULL); // Iterator
    psMetadataItem *item;               // Item from iteration
    while ((item = psMetadataGetAndIncrement(iter))) {
        psListAdd(list, PS_LIST_TAIL, item->name);
    }
    psFree(iter);
    return list;
}

psMetadata *pmConceptsSpecs(pmFPALevel level)
{
    if (!conceptsInitialised) {
        pmConceptsInit();
    }

    // Get the appropriate concepts
    psMetadata *concepts = conceptsFromLevel(level); // Metadata of concepts specs
    if (!concepts) {
        psError(PS_ERR_BAD_PARAMETER_VALUE, true, "Invalid concept level provided: %d\n", level);
        return NULL;
    }

    return concepts;
}

bool pmConceptGetRequired(const char *name, pmFPALevel level)
{
    PS_ASSERT_STRING_NON_EMPTY(name, false);
    if (!conceptsInitialised) {
        pmConceptsInit();
    }

    psMetadata *concepts = conceptsFromLevel(level); // The metadata of known concepts

    bool mdok;                          // Status of MD lookup
    pmConceptSpec *spec = psMetadataLookupPtr(&mdok, concepts, name); // The specification
    if (!spec) {
        // Won't throw an error, because we can't distinguish an error from the desired result.
        // However, that doesn't really matter, because if we can't find it, then it can't be required!
        return false;
    }

    return spec->required;
}

bool pmConceptSetRequired(const char *name, pmFPALevel level, bool required)
{
    PS_ASSERT_STRING_NON_EMPTY(name, false);

    if (!conceptsInitialised) {
        pmConceptsInit();
    }

    psMetadata *concepts = conceptsFromLevel(level); // The metadata of known concepts

    bool mdok;                          // Status of MD lookup
    pmConceptSpec *spec = psMetadataLookupPtr(&mdok, concepts, name); // The specification
    if (!spec) {
        psError(PS_ERR_BAD_PARAMETER_VALUE, true, "Unable to find defined concept %s in level %d.",
                name, level);
        return false;
    }
    spec->required = required;

    return true;
}

bool pmConceptRegister(psMetadataItem *blank, pmConceptParseFunc parse,
                       pmConceptFormatFunc format, pmConceptCopyFunc copy,
                       bool required, pmFPALevel level)
{
    PS_ASSERT_PTR_NON_NULL(blank, false);

    if (!conceptsInitialised) {
        pmConceptsInit();
    }

    psMetadata *target = conceptsFromLevel(level); // The metadata of known concepts to write to
    if (!target) {
        psError(PS_ERR_BAD_PARAMETER_VALUE, false,
                "Unable to register concept at invalid concept level.");
        return false;
    }

    pmConceptSpec *spec = pmConceptSpecAlloc(blank, parse, format, copy, required); // Concept specification
    psMetadataAdd(target, PS_LIST_TAIL, blank->name, PS_DATA_UNKNOWN | PS_META_REPLACE,
                  "Concepts specification", spec);
    psFree(spec);                       // Drop reference

    return true;
}


// Set all registered concepts to blank value for the specified level
static bool conceptsBlank(psMetadata **specs, // One of the concepts specifications
                          psMetadata *target // Place to install the concepts
                         )
{
    assert(specs);
    assert(target);

    if (!conceptsInitialised) {
        pmConceptsInit();
    }
    psMetadataIterator *specsIter = psMetadataIteratorAlloc(*specs, PS_LIST_HEAD, NULL); // Iterator on specs
    psMetadataItem *specItem = NULL;    // Item from the specs metadata
    while ((specItem = psMetadataGetAndIncrement(specsIter))) {
        psTrace("psModules.concepts", 9, "Blanking %s...\n", specItem->name);
        pmConceptSpec *spec = specItem->data.V; // The specification
        psMetadataItem *blank = spec->blank; // The concept
        psMetadataItem *copy = NULL;    // Copy of the blank concept
        // Trap the lists, which can't be copied in the ordinary way without a warning
        if (blank->type == PS_DATA_LIST) {
            copy = psMetadataItemAllocPtr(blank->name, PS_DATA_LIST, blank->comment, blank->data.V);
        } else {
            copy = psMetadataItemCopy(blank);
        }
        if (!psMetadataAddItem(target, copy, PS_LIST_TAIL, PS_META_REPLACE)) {
            psLogMsg(__func__, PS_LOG_WARN, "Unable to add blank version of concept %s\n", blank->name);
        }
        psFree(copy);                   // Drop reference
    }
    psFree(specsIter);

    return true;
}


bool pmConceptsBlankFPA(pmFPA *fpa)
{
    PS_ASSERT_PTR_NON_NULL(fpa, false);
    psTrace("psModules.concepts", 5, "Blanking FPA concepts: %p %p\n", conceptsFPA, fpa->concepts);
    return conceptsBlank(&conceptsFPA, fpa->concepts);
}

bool pmConceptsBlankChip(pmChip *chip)
{
    PS_ASSERT_PTR_NON_NULL(chip, false);
    psTrace("psModules.concepts", 5, "Blanking chip concepts: %p %p\n", conceptsChip, chip->concepts);
    return conceptsBlank(&conceptsChip, chip->concepts);
}

bool pmConceptsBlankCell(pmCell *cell)
{
    PS_ASSERT_PTR_NON_NULL(cell, false);
    psTrace("psModules.concepts", 5, "Blanking cell concepts: %p %p\n", conceptsCell, cell->concepts);
    return conceptsBlank(&conceptsCell, cell->concepts);
}

// Register a concept
#define CONCEPT_REGISTER_FUNCTION(TYPENAME, SUFFIX, DEFAULT) \
static void conceptRegister##SUFFIX(const char *name, /* Name of concept */ \
                                    const char *comment, /* Comment for concept */ \
                                    pmConceptParseFunc parse, /* Parsing function, or NULL */ \
                                    pmConceptFormatFunc format, /* Formatting function, or NULL */ \
                                    pmConceptCopyFunc copy, /* Copying function, or NULL */ \
                                    bool required, /* Required concept? */ \
                                    pmFPALevel level /* Level at which concept applies */ \
    ) \
{ \
    psMetadataItem *item = psMetadataItemAlloc##TYPENAME(name, comment, DEFAULT); /* Item to add */ \
    pmConceptRegister(item, parse, format, copy, required, level); \
    psFree(item); \
    return; \
}

CONCEPT_REGISTER_FUNCTION(Str, Str, "");
CONCEPT_REGISTER_FUNCTION(F32, F32, NAN);
CONCEPT_REGISTER_FUNCTION(F64, F64, NAN);
CONCEPT_REGISTER_FUNCTION(S32, Enum, -1); // For enums: set default to -1
CONCEPT_REGISTER_FUNCTION(S32, S32, 0); // For values: set default to 0
CONCEPT_REGISTER_FUNCTION(Bool, Bool, NULL); // For values: set default to 0

static void conceptRegisterTime(const char *name, /* Name of concept */ \
                                const char *comment, /* Comment for concept */ \
                                bool required, /* Required concept? */ \
                                pmFPALevel level /* Level at which concept applies */ \
    )
{
    psTime *time = psTimeAlloc(PS_TIME_TAI); // Blank time
    // Not particularly distinguishing, but should be good enough
    time->sec = 0;
    time->nsec = 0;
    psMetadataItem *item = psMetadataItemAlloc(name, PS_DATA_TIME, comment, time);
    psFree(time);
    pmConceptRegister(item, p_pmConceptParse_TIME, p_pmConceptFormat_TIME, NULL, required, level);
    psFree(item);
}

bool pmConceptsInit(void)
{
    if (conceptsInitialised) {
        return true;
    }

    conceptsInitialised = true;

    p_psMemAllocatePersistent(true);

    bool init = false;                  // Did we initialise anything?

    if (!conceptsFPA) {
        conceptsFPA = psMetadataAlloc();
        init = true;

        // Install the standard concepts
        conceptRegisterStr("FPA.TELESCOPE", "Telescope of origin", NULL, NULL, NULL, false, PM_FPA_LEVEL_FPA);
        conceptRegisterStr("FPA.INSTRUMENT", "Instrument name (according to the instrument)", NULL, NULL, NULL, false, PM_FPA_LEVEL_FPA);
        conceptRegisterStr("FPA.DETECTOR", "Detector name", NULL, NULL, NULL, false, PM_FPA_LEVEL_FPA);
        conceptRegisterStr("FPA.COMMENT", "Observation comment", NULL, NULL, NULL, false, PM_FPA_LEVEL_FPA);
        conceptRegisterStr("FPA.OBS.MODE", "Observation mode (eg, survey id)", NULL, NULL, NULL, false, PM_FPA_LEVEL_FPA);
        conceptRegisterStr("FPA.OBS.GROUP", "Observation group (eg, associated images)", NULL, NULL, NULL, false, PM_FPA_LEVEL_FPA);
        conceptRegisterF32("FPA.FOCUS", "Telescope focus", NULL, NULL, NULL, false, PM_FPA_LEVEL_FPA);
        conceptRegisterF32("FPA.AIRMASS", "Airmass at boresight", NULL, NULL, NULL, false, PM_FPA_LEVEL_FPA);
        // XXX p_pmConceptParse_FPA_FILTER -> p_pmConceptParse_FPA_FILTERID (and Format as well)?
        conceptRegisterStr("FPA.FILTERID", "Filter used (parsed, abstract name)", p_pmConceptParse_FPA_FILTER, p_pmConceptFormat_FPA_FILTER, NULL, false, PM_FPA_LEVEL_FPA);
        conceptRegisterStr("FPA.FILTER", "Filter used (instrument name)", NULL, NULL, NULL, false, PM_FPA_LEVEL_FPA);
        conceptRegisterF32("FPA.POSANGLE", "Position angle of instrument", NULL, NULL, NULL, false, PM_FPA_LEVEL_FPA);
        conceptRegisterF32("FPA.ROTANGLE", "Rotator angle of instrument", NULL, NULL, NULL, false, PM_FPA_LEVEL_FPA);
        conceptRegisterStr("FPA.RADECSYS", "Celestial coordinate system", NULL, NULL, NULL, false, PM_FPA_LEVEL_FPA);
        conceptRegisterF64("FPA.RA", "Right Ascension of boresight", p_pmConceptParse_FPA_Coords, p_pmConceptFormat_FPA_Coords, NULL, false, PM_FPA_LEVEL_FPA);
        conceptRegisterF64("FPA.DEC", "Declination of boresight", p_pmConceptParse_FPA_Coords, p_pmConceptFormat_FPA_Coords, NULL, false, PM_FPA_LEVEL_FPA);
        conceptRegisterF64("FPA.LONGITUDE", "West longitude of observatory", p_pmConceptParse_FPA_Coords, p_pmConceptFormat_FPA_Coords, NULL, false, PM_FPA_LEVEL_FPA);
        conceptRegisterF64("FPA.LATITUDE", "Latitude of observatory", p_pmConceptParse_FPA_Coords, p_pmConceptFormat_FPA_Coords, NULL, false, PM_FPA_LEVEL_FPA);
        conceptRegisterF32("FPA.ELEVATION", "Elevation of observatory (meters)", NULL, NULL, NULL, false, PM_FPA_LEVEL_FPA);

        // conceptRegisterStr("FPA.OBSTYPE", "Type of observation", NULL, NULL, NULL, false, PM_FPA_LEVEL_FPA);
        conceptRegisterStr("FPA.OBSTYPE", "Type of observation", p_pmConceptParse_FPA_OBSTYPE, p_pmConceptFormat_FPA_OBSTYPE, NULL, false, PM_FPA_LEVEL_FPA);

        conceptRegisterStr("FPA.OBJECT", "Object of observation", NULL, NULL, NULL, false, PM_FPA_LEVEL_FPA);
        conceptRegisterF64("FPA.ALT", "Altitude of boresight", NULL, NULL, NULL, false, PM_FPA_LEVEL_FPA);
        conceptRegisterF64("FPA.AZ", "Azimuth of boresight", NULL, NULL, NULL, false, PM_FPA_LEVEL_FPA);
        conceptRegisterEnum("FPA.TIMESYS", "Time system", p_pmConceptParse_TIMESYS, p_pmConceptFormat_TIMESYS, p_pmConceptCopy_TIMESYS, false, PM_FPA_LEVEL_FPA);
        conceptRegisterTime("FPA.TIME", "Time of exposure", false, PM_FPA_LEVEL_FPA);

        conceptRegisterStr("FPA.SHUTOUTC", "Time of exposure open", NULL,NULL,NULL,false, PM_FPA_LEVEL_FPA);
        conceptRegisterStr("FPA.SHUTCUTC", "Time of exposure close", NULL,NULL,NULL,false, PM_FPA_LEVEL_FPA);
        conceptRegisterStr("FPA.SHMDOUTC", "Time of exposure open mid-focalplane", NULL,NULL,NULL,false, PM_FPA_LEVEL_FPA);
        conceptRegisterStr("FPA.SHMDCUTC", "Time of exposure close mid-focalplane", NULL,NULL,NULL,false, PM_FPA_LEVEL_FPA);

        conceptRegisterF32("FPA.TEMP", "Temperature of focal plane", NULL, NULL, NULL, false, PM_FPA_LEVEL_FPA);
        conceptRegisterF32("FPA.M1X", "Primary Mirror X Position", NULL, NULL, NULL, false, PM_FPA_LEVEL_FPA);
        conceptRegisterF32("FPA.M1Y", "Primary Mirror Y Position", NULL, NULL, NULL, false, PM_FPA_LEVEL_FPA);
        conceptRegisterF32("FPA.M1Z", "Primary Mirror Z Position", NULL, NULL, NULL, false, PM_FPA_LEVEL_FPA);
        conceptRegisterF32("FPA.M1TIP", "Primary Mirror TIP", NULL, NULL, NULL, false, PM_FPA_LEVEL_FPA);
        conceptRegisterF32("FPA.M1TILT", "Primary Mirror TILT", NULL, NULL, NULL, false, PM_FPA_LEVEL_FPA);
        conceptRegisterF32("FPA.M2X", "Primary Mirror X Position", NULL, NULL, NULL, false, PM_FPA_LEVEL_FPA);
        conceptRegisterF32("FPA.M2Y", "Primary Mirror Y Position", NULL, NULL, NULL, false, PM_FPA_LEVEL_FPA);
        conceptRegisterF32("FPA.M2Z", "Primary Mirror Z Position", NULL, NULL, NULL, false, PM_FPA_LEVEL_FPA);
        conceptRegisterF32("FPA.M2TIP", "Primary Mirror TIP", NULL, NULL, NULL, false, PM_FPA_LEVEL_FPA);
        conceptRegisterF32("FPA.M2TILT", "Primary Mirror TILT", NULL, NULL, NULL, false, PM_FPA_LEVEL_FPA);
        conceptRegisterF32("FPA.ENV.TEMP", "Environment: Temperature", NULL, NULL, NULL, false, PM_FPA_LEVEL_FPA);
        conceptRegisterF32("FPA.ENV.HUMID", "Environment: Humidity", NULL, NULL, NULL, false, PM_FPA_LEVEL_FPA);
        conceptRegisterF32("FPA.ENV.WIND", "Environment: Wind speed", NULL, NULL, NULL, false, PM_FPA_LEVEL_FPA);
        conceptRegisterF32("FPA.ENV.DIR", "Environment: Wind direction", NULL, NULL, NULL, false, PM_FPA_LEVEL_FPA);
        conceptRegisterF32("FPA.TELTEMP.M1", "Telescope Temperatures: M1", p_pmConceptParse_TELTEMPS, NULL, NULL, false, PM_FPA_LEVEL_FPA);
        conceptRegisterF32("FPA.TELTEMP.M1CELL", "Telescope Temperatures: M1 cell", p_pmConceptParse_TELTEMPS, NULL, NULL, false, PM_FPA_LEVEL_FPA);
        conceptRegisterF32("FPA.TELTEMP.M2", "Telescope Temperatures: M2", p_pmConceptParse_TELTEMPS, NULL, NULL, false, PM_FPA_LEVEL_FPA);
        conceptRegisterF32("FPA.TELTEMP.SPIDER", "Telescope Temperatures: spider", p_pmConceptParse_TELTEMPS, NULL, NULL, false, PM_FPA_LEVEL_FPA);
        conceptRegisterF32("FPA.TELTEMP.TRUSS", "Telescope Temperatures: truss", p_pmConceptParse_TELTEMPS, NULL, NULL, false, PM_FPA_LEVEL_FPA);
        conceptRegisterF32("FPA.TELTEMP.EXTRA", "Telescope Temperatures: extra", p_pmConceptParse_TELTEMPS, NULL, NULL, false, PM_FPA_LEVEL_FPA);
        conceptRegisterF32("FPA.PON.TIME", "Power On Time", NULL, NULL, NULL, false, PM_FPA_LEVEL_FPA);
        conceptRegisterS32("FPA.BURNTOOL.APPLIED", "[T=applied] Burn streaks applied to image data", p_pmConceptParse_BTOOLAPP,p_pmConceptFormat_BTOOLAPP,NULL,false,PM_FPA_LEVEL_FPA);
        conceptRegisterF32("FPA.EXPOSURE", "Exposure time (sec)", NULL, NULL, NULL, false, PM_FPA_LEVEL_FPA);
        conceptRegisterF32("FPA.ZP", "Magnitude zero point", NULL, NULL, NULL, false, PM_FPA_LEVEL_FPA);
    }
    if (!conceptsChip) {
        conceptsChip = psMetadataAlloc();
        init = true;

        // Install the standard concepts
        conceptRegisterS32("CHIP.XPARITY", "Orientation in x compared to the rest of the FPA", NULL, NULL, NULL, true, PM_FPA_LEVEL_CHIP);
        conceptRegisterS32("CHIP.YPARITY", "Orientation in y compared to the rest of the FPA", NULL, NULL, NULL, true, PM_FPA_LEVEL_CHIP);
        conceptRegisterS32("CHIP.X0", "Position of (0,0) on the FPA",p_pmConceptParse_Positions,p_pmConceptFormat_Positions, NULL, true, PM_FPA_LEVEL_CHIP);
        conceptRegisterS32("CHIP.Y0", "Position of (0,0) on the FPA",p_pmConceptParse_Positions,p_pmConceptFormat_Positions, NULL, true, PM_FPA_LEVEL_CHIP);
        conceptRegisterS32("CHIP.XSIZE", "Size of chip (pixels)", NULL, NULL, NULL, true, PM_FPA_LEVEL_CHIP);
        conceptRegisterS32("CHIP.YSIZE", "Size of chip (pixels)", NULL, NULL, NULL, true, PM_FPA_LEVEL_CHIP);
        conceptRegisterF32("CHIP.TEMP", "Temperature of chip", NULL, NULL, NULL, false, PM_FPA_LEVEL_CHIP);
        conceptRegisterF32("CHIP.TEMPERATURE", "Temperature of chip", NULL, NULL, NULL, false, PM_FPA_LEVEL_CHIP);
        conceptRegisterStr("CHIP.ID", "Chip identifier", NULL, NULL, NULL, false, PM_FPA_LEVEL_CHIP);
        conceptRegisterF32("CHIP.SEEING", "Seeing FWHM (pixels)", NULL, NULL, NULL, false, PM_FPA_LEVEL_CHIP);
	conceptRegisterBool("CHIP.VIDEOCELL", "Does this OTA have any video cells", p_pmConceptParse_VideoCell,NULL,NULL,false,PM_FPA_LEVEL_CHIP);
    }

    if (!conceptsCell) {
        conceptsCell = psMetadataAlloc();
        init = true;

        // Install the standard concepts
        conceptRegisterF32("CELL.GAIN", "CCD gain (e/count)", NULL, NULL, NULL, true, PM_FPA_LEVEL_CELL);
        conceptRegisterF32("CELL.READNOISE", "CCD read noise (e)", p_pmConceptParse_CELL_READNOISE, p_pmConceptFormat_CELL_READNOISE, NULL, true, PM_FPA_LEVEL_CELL);
        conceptRegisterF32("CELL.SATURATION", "Saturation level (counts)", NULL, NULL, NULL, true, PM_FPA_LEVEL_CELL);
        conceptRegisterF32("CELL.BAD", "Bad level (counts)", NULL, NULL, NULL, true, PM_FPA_LEVEL_CELL);
        conceptRegisterS32("CELL.XPARITY", "Orientation in x compared to the rest of the chip", NULL, NULL, NULL, true, PM_FPA_LEVEL_CELL);
        conceptRegisterS32("CELL.YPARITY", "Orientation in y compared to the rest of the chip", NULL, NULL, NULL, true, PM_FPA_LEVEL_CELL);
        conceptRegisterS32("CELL.READDIR", "Read direction, rows=1, cols=2", NULL, NULL, NULL, true, PM_FPA_LEVEL_CELL);

        // These (CELL.EXPOSURE and CELL.DARKTIME) used to be READOUT.EXPOSURE and READOUT.DARKTIME, but that
        // doesn't really make sense at the moment.  Maybe we need to add a "parent" link to the readouts.
        // But then how are the exposure times REALLY derived?  They're not in the FITS headers, because a
        // readout is a plane in a 3D image.  We'll have to dream up some additional suffix to specify these,
        // but for now....
        conceptRegisterF32("CELL.EXPOSURE", "Exposure time (sec)", NULL, NULL, NULL, false, PM_FPA_LEVEL_CELL);
        conceptRegisterF32("CELL.DARKTIME", "Time since flush (sec)", NULL, NULL, NULL, false, PM_FPA_LEVEL_CELL);

        conceptRegisterS32("CELL.XBIN", "Binning in x", p_pmConceptParse_CELL_Binning,p_pmConceptFormat_CELL_XBIN, NULL, true, PM_FPA_LEVEL_CELL);
        conceptRegisterS32("CELL.YBIN", "Binning in y",p_pmConceptParse_CELL_Binning,p_pmConceptFormat_CELL_YBIN, NULL, true, PM_FPA_LEVEL_CELL);
        conceptRegisterEnum("CELL.TIMESYS", "Time system", p_pmConceptParse_TIMESYS,p_pmConceptFormat_TIMESYS, p_pmConceptCopy_TIMESYS, false, PM_FPA_LEVEL_CELL);
        conceptRegisterTime("CELL.TIME", "Time of exposure", false, PM_FPA_LEVEL_CELL);
        conceptRegisterS32("CELL.X0", "Position of (0,0) on the chip",p_pmConceptParse_Positions,p_pmConceptFormat_Positions, NULL, true, PM_FPA_LEVEL_CELL);
        conceptRegisterS32("CELL.Y0", "Position of (0,0) on the chip",p_pmConceptParse_Positions,p_pmConceptFormat_Positions, NULL, true, PM_FPA_LEVEL_CELL);
        conceptRegisterS32("CELL.XSIZE", "Size of cell (pixels)", NULL, NULL, NULL, true, PM_FPA_LEVEL_CELL);
        conceptRegisterS32("CELL.YSIZE", "Size of cell (pixels)", NULL, NULL, NULL, true, PM_FPA_LEVEL_CELL);
        conceptRegisterS32("CELL.XWINDOW", "Start of cell window (pixels)",p_pmConceptParse_Positions,p_pmConceptFormat_Positions, NULL, true, PM_FPA_LEVEL_CELL);
        conceptRegisterS32("CELL.YWINDOW", "Start of cell window (pixels)",p_pmConceptParse_Positions,p_pmConceptFormat_Positions, NULL, true, PM_FPA_LEVEL_CELL);

        // CELL.TRIMSEC
        {
            psRegion *trimsec = psAlloc(sizeof(psRegion)); // Blank trimsec
            trimsec->x0 = trimsec->y0 = trimsec->x1 = trimsec->y1 = NAN;
            psMetadataItem *cellTrimsec = psMetadataItemAllocPtr("CELL.TRIMSEC", PS_DATA_REGION,
                                          "Trim section", trimsec);
            psFree(trimsec);
            pmConceptRegister(cellTrimsec, p_pmConceptParse_CELL_TRIMSEC,p_pmConceptFormat_CELL_TRIMSEC, NULL, true, PM_FPA_LEVEL_CELL);
            psFree(cellTrimsec);
        }

        // CELL.BIASSEC
        {
            psList *biassecs = psListAlloc(NULL); // Blank biassecs
            psMetadataItem *cellBiassec = psMetadataItemAllocPtr("CELL.BIASSEC", PS_DATA_LIST,
                                          "Bias sections", biassecs);
            psFree(biassecs);
            pmConceptRegister(cellBiassec, p_pmConceptParse_CELL_BIASSEC, p_pmConceptFormat_CELL_BIASSEC, NULL, true, PM_FPA_LEVEL_CELL);
            psFree(cellBiassec);
        }

    }

    p_psMemAllocatePersistent(false);

    return init;
}

void pmConceptsDone(void)
{
    psFree(conceptsFPA);
    conceptsFPA = NULL;
    psFree(conceptsChip);
    conceptsChip = NULL;
    psFree(conceptsCell);
    conceptsCell = NULL;

    conceptsInitialised = false;
}


// Interpolate the concept.  Generalises the FPA/Chip/Cell
#define CONCEPT_INTERPOLATE(SOURCE, NAME, DEFAULT) \
    if (strncmp(concept, NAME, strlen(NAME)) == 0) { \
        psString value = NULL; /* Value of concept */ \
        if (SOURCE) { \
            psMetadataItem *item = psMetadataLookup((SOURCE)->concepts, concept); /* Item with concept */ \
            if (!item) { \
                psError(PS_ERR_BAD_PARAMETER_VALUE, true, "Can't find concept %s in %s", concept, NAME); \
                psFree(string); \
                return NULL; \
            } \
            \
            value = psMetadataItemParseString(item); /* Value of concept */ \
            if (!value) { \
                psError(PS_ERR_UNEXPECTED_NULL, false, "Unable to parse concept %s", concept); \
                psFree(string); \
                return NULL; \
            } \
        } else { \
            if (!(DEFAULT)) { \
                psError(PS_ERR_UNEXPECTED_NULL, false, "Unable to determine concept %s", concept); \
                psFree(string); \
                return NULL; \
            } \
            value = psStringCopy(DEFAULT); \
        } \
        \
        char replace[length + 2];       /* String to replace with value */ \
        replace[0] = '{'; \
        strcpy(replace + 1, concept); \
        strcpy(replace + length, "}"); \
        \
        psTrace("psModules.concepts", 10, "Interpolating concept %s for %s", replace, value); \
        \
        if (!psStringSubstitute(&string, value, replace)) { \
            psError(PS_ERR_UNKNOWN, false, "Unable to replace concept %s", concept); \
            psFree(string); \
            psFree(value); \
            return NULL; \
        } \
        psFree(value); \
        \
        continue; \
    }


// XXX Could make the concept delimiters, currently '{' and '}', configurable
psString pmConceptsInterpolate(const char *input,
                               const pmFPA *fpa,
                               const pmChip *chip,
                               const pmCell *cell
    )
{
    PS_ASSERT_STRING_NON_EMPTY(input, NULL);

    psString string = psStringCopy(input); // Interpolated string, to return

    char *start;                        // Start of a concept
    while ((start = strchr(string, '{'))) {
        char *stop = strchr(start, '}'); // End of a concept
        int length = stop - start;      // Length of the concept name, including terminating \0
        char concept[length];  // Name of concept
        strncpy(concept, start + 1, length - 1);
        concept[length - 1] = '\0';

        // special variants:
        if (!strcmp(concept, "FPA.DATE")) {
          psTime *fpaTime = psMetadataLookupPtr(NULL, fpa->concepts, "FPA.TIME");
          if (!fpaTime) {
            psError(PS_ERR_BAD_PARAMETER_VALUE, true, "Missing concept FPA.TIME needed for FPA.DATE");
            psFree(string);
            return NULL;
          }
          psString dateTimeString = psTimeToISO(fpaTime); // String representation
          psList *dateTime = psStringSplit(dateTimeString, "T", true);
          psFree(dateTimeString);
          psString dateString = psMemIncrRefCounter(psListGet(dateTime, PS_LIST_HEAD)); // The date string
          psFree (dateTime);

          if (!psStringSubstitute(&string, dateString, "{FPA.DATE}")) {
              psError(PS_ERR_UNKNOWN, false, "Unable to replace concept %s", concept);
              psFree(string);
              psFree(dateString);
              return NULL;
          }
          psFree (dateString);
          continue;
        }

        psTrace("psModules.concepts", 7, "Interpolating concept %s", concept);

        CONCEPT_INTERPOLATE(fpa,  "FPA", NULL);
        CONCEPT_INTERPOLATE(chip, "CHIP", "fpa");
        CONCEPT_INTERPOLATE(cell, "CELL", "chip");

        psError(PS_ERR_BAD_PARAMETER_VALUE, true, "Unrecognised concept: %s", concept);
        psFree(string);
        return NULL;
    }

    return string;
}


psMetadataItem *p_pmConceptsDepend(const char *name, const psMetadata *menu, const psMetadata *source,
                                   const pmFPA *fpa, const pmChip *chip, const pmCell *cell)
{
    psAssert(name && strlen(name) > 0, "Concept name is empty");
    psAssert(menu, "Must have menu");
    psAssert(source, "Must have source");

    // Check for DEPEND
    psString depend = NULL; // The CONCEPT.DEPEND
    psStringAppend(&depend, "%s.DEPEND", name);
    bool mdok;                          // Status of MD lookup
    const char *dependConcept = psMetadataLookupStr(&mdok, source, depend); // The concept name
    if (!mdok || !dependConcept || strlen(dependConcept) == 0) {
        psError(PS_ERR_IO, true, "Unable to parse %s: couldn't find %s in DEFAULTS.\n", name, depend);
        psFree(depend);
        return NULL;
    }
    psFree(depend);
    // Now look up the depend value
    psMetadataItem *dependValue = NULL; // The value of the concept we're looking up
    if (cell) {
        dependValue = psMetadataLookup(cell->concepts, dependConcept);
    }
    if (chip && !dependValue) {
        dependValue = psMetadataLookup(chip->concepts, dependConcept);
    }
    if (fpa && !dependValue) {
        dependValue = psMetadataLookup(chip->concepts, dependConcept);
    }
    if (!dependValue) {
        // Not an error --- it may be specified some other way
        psTrace("psModules.concepts", 7, "Couldn't find DEPEND for %s", name);
        return NULL;
    }
    if (dependValue->type != PS_DATA_STRING) {
        psError(PS_ERR_BAD_PARAMETER_TYPE, true, "%s is required to resolve %s in DEFAULTS, "
                "but it is not of type STRING.\n", dependConcept, name);
        return NULL;
    }
    const char *key = dependValue->data.V; // The key to the DEPEND menu
    psTrace("psModules.concepts", 7, "%s.DEPEND resolves to %s....\n", name, key);

    return psMetadataLookup(menu, key);
}

int pmConceptsChipNumberFromName (pmFPA *fpa, char *name) {

    for (int i = 0; i < fpa->chips->n; i++) {
        pmChip *chip = fpa->chips->data[i];
        if (!chip) continue;
        char *thisone = psMetadataLookupStr (NULL, chip->concepts, "CHIP.NAME");
        if (!thisone) continue;
        if (!strcmp (name, thisone)) return (i);
    }
    return -1;
}

pmChip *pmConceptsChipFromName (pmFPA *fpa, char *name) {

    for (int i = 0; i < fpa->chips->n; i++) {
        pmChip *chip = fpa->chips->data[i];
        if (!chip) continue;
        char *thisone = psMetadataLookupStr (NULL, chip->concepts, "CHIP.NAME");
        if (!thisone) continue;
        if (!strcmp (name, thisone)) return (chip);
    }
    return NULL;
}

