#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <string.h>
#include <strings.h>            /* for strn?casecmp */
#include <pslib.h>

#include "pmConfig.h"
#include "pmDetrendDB.h"

#include "pmHDU.h"
#include "pmFPA.h"
#include "pmHDUUtils.h"
#include "pmFPALevel.h"
#include "pmFPAview.h"
#include "pmFPAfile.h"
#include "pmFPACopy.h"
#include "pmConceptsCopy.h"

static int fileNum = 0;                 // Number of file

static bool fpaFileFreeStrict = true;   // Strict checking when file has been freed?

bool pmFPAfileFreeSetStrict(bool new)
{
    bool old = fpaFileFreeStrict;       // Old value, to return
    fpaFileFreeStrict = new;
    return old;
}

static void pmFPAfileFree(pmFPAfile *file)
{
    if (!file) {
        return;
    }
    psTrace ("pmFPAfileFree", 5, "freeing %s %p\n", file->name, file->fits);
    psAssert(!fpaFileFreeStrict || file->fits == NULL, "File %s wasn't closed.", file->name);

    int testFile = FALSE;							
    testFile = testFile || !strcmp(file->name, "GDIFF.OUTPUT.SOURCES");	
    testFile = testFile || !strcmp(file->name, "GDIFF.POS1.SOURCES");	
    testFile = testFile || !strcmp(file->name, "GDIFF.POS2.SOURCES");	
    if (FALSE && testFile) {							
      fprintf (stderr, "%s : %d : %d", file->name, file->state, file->mode);
      fprintf (stderr, "\n");
    }

    psTrace ("pmFPAfileFree", 5, "freeing %s\n", file->name);
    psFree (file->fpa);
    psFree (file->src);
    psFree (file->readout);
    psFree (file->names);

    psFree (file->camera);
    psFree (file->cameraName);
    psFree (file->format);
    psFree (file->formatName);
    psFree (file->name);

    psFree(file->compression);
    psFree(file->options);

    psFree (file->filerule);

    psFree (file->filesrc);
    psFree (file->detrend);

    psFree (file->filename);
    psFree (file->origname);
    psFree (file->extname);

    return;
}

pmFPAfile *pmFPAfileAlloc(void)
{
    pmFPAfile *file = psAlloc(sizeof(pmFPAfile));
    psMemSetDeallocator(file, (psFreeFunc) pmFPAfileFree);

    file->wrote_phu = false;
    file->readout = NULL;
    file->header = NULL;

    file->fileLevel = PM_FPA_LEVEL_NONE;
    file->dataLevel = PM_FPA_LEVEL_NONE;
    file->freeLevel = PM_FPA_LEVEL_NONE;
    file->mosaicLevel = PM_FPA_LEVEL_NONE;

    file->type = PM_FPA_FILE_NONE;
    file->mode = PM_FPA_MODE_NONE;
    file->state = PM_FPA_STATE_CLOSED;

    file->fpa = NULL;
    file->fits = NULL;
    file->compression = NULL;
    file->options = NULL;
    file->names = psMetadataAlloc();

    file->camera = NULL;
    file->cameraName = NULL;
    file->format = NULL;
    file->formatName = NULL;
    file->name = NULL;

    file->filerule = NULL;

    file->filename = NULL;
    file->origname = NULL;
    file->extname  = NULL;

    file->filesrc = NULL;
    file->detrend = NULL;

    file->xBin = 1;
    file->yBin = 1;
    file->src = NULL;

    file->save = false;

    file->fileIndex = fileNum++;
    file->fileID = 0;

    file->imageId = 0;
    file->sourceId = 0;

    return file;
}

// select the readout from the named pmFPAfile; if the named file does not exist,
pmReadout *pmFPAfileThisReadout (psMetadata *files, const pmFPAview *view, const char *name)
{
    PS_ASSERT_PTR_NON_NULL(files, false);
    PS_ASSERT_PTR_NON_NULL(view, false);
    PS_ASSERT_PTR_NON_NULL(name, false);
    PS_ASSERT_INT_POSITIVE(strlen(name), false);

    bool status;

    pmFPAfile *file = psMetadataLookupPtr (&status, files, name);
    if (file == NULL) {
        return NULL;
    }

    // internal files have the readout as a separate element:
    if (file->mode == PM_FPA_MODE_INTERNAL) {
        return file->readout;
    }

    pmReadout *readout = pmFPAviewThisReadout (view, file->fpa);
    return readout;
}

// select the cell from the named pmFPAfile; if the named file does not exist,
pmCell *pmFPAfileThisCell (psMetadata *files, const pmFPAview *view, const char *name)
{
    PS_ASSERT_PTR_NON_NULL(files, false);
    PS_ASSERT_PTR_NON_NULL(view, false);
    PS_ASSERT_PTR_NON_NULL(name, false);
    PS_ASSERT_INT_POSITIVE(strlen(name), false);

    bool status;

    pmFPAfile *file = psMetadataLookupPtr (&status, files, name);
    if (file == NULL) {
        return NULL;
    }

    // internal files have the readout as a separate element:
    if (file->mode == PM_FPA_MODE_INTERNAL) {
        return NULL;
    }

    pmCell *cell = pmFPAviewThisCell(view, file->fpa);
    return cell;
}

// select the readout from the named pmFPAfile; if the named file does not exist,
pmChip *pmFPAfileThisChip (psMetadata *files, const pmFPAview *view, const char *name)
{
    PS_ASSERT_PTR_NON_NULL(files, false);
    PS_ASSERT_PTR_NON_NULL(view, false);
    PS_ASSERT_PTR_NON_NULL(name, false);
    PS_ASSERT_INT_POSITIVE(strlen(name), false);

    bool status;

    pmFPAfile *file = psMetadataLookupPtr (&status, files, name);
    if (file == NULL) {
        return NULL;
    }

    // internal files have the readout as a separate element:
    if (file->mode == PM_FPA_MODE_INTERNAL) {
        return NULL;
    }

    pmChip *chip = pmFPAviewThisChip (view, file->fpa);
    return chip;
}

psString pmFPANameFromRule(const char *rule, const pmFPA *fpa, const pmFPAview *view)
{
    PS_ASSERT_STRING_NON_EMPTY(rule, NULL);
    PS_ASSERT_PTR_NON_NULL(view, NULL);
    PS_ASSERT_PTR_NON_NULL(fpa, NULL);

    psString newName = NULL;            // New name, to be returned
    newName = psStringCopy(rule);

    bool status = false;
    if (strstr(newName, "{FPA.OBS}")) {
        char *name = psMetadataLookupStr(&status, fpa->concepts, "FPA.OBS");
        if (name) {
            psStringSubstitute(&newName, name, "{FPA.OBS}");
        } 
    }
    if (strstr(newName, "{FPA.NAME}")) {
        char *name = psMetadataLookupStr(NULL, fpa->concepts, "FPA.NAME");
        if (name) {
            psStringSubstitute(&newName, name, "{FPA.NAME}");
        }
    }
    if (strstr(newName, "{CHIP.NAME}")) {
        const char *name = NULL;        // Name of chip
        pmChip *chip = pmFPAviewThisChip(view, fpa);
        if (chip) {
            name = psMetadataLookupStr(NULL, chip->concepts, "CHIP.NAME");
            psAssert(name, "All chips should have a name");
        } else {
            name = "fpa";
        }
        psStringSubstitute(&newName, name, "{CHIP.NAME}");

    }
    if (strstr(newName, "{CHIP.ID}")) {
        pmChip *chip = pmFPAviewThisChip(view, fpa);
        if (chip) {
            char *name = psMetadataLookupStr(NULL, chip->concepts, "CHIP.ID");
            if (name) {
                psStringSubstitute(&newName, name, "{CHIP.ID}");
            }
        }
    }
    if (strstr(newName, "{CHIP.N}")) {
        char *name = NULL;
        if (view->chip < 0) {
            psStringAppend(&name, "XX");
        } else {
            psStringAppend(&name, "%02d", view->chip);
        }
        psStringSubstitute(&newName, name, "{CHIP.N}");
        psFree(name);
    }
    if (strstr(newName, "{CHIP.NUM}")) {
        char *name = NULL;
        if (view->chip < 0) {
            psStringAppend(&name, "XX");
        } else {
            int chipNum = view->chip;   // Number of chip
            // Potential correction for fortran (unit-indexed) numbering
            pmChip *chip = pmFPAviewThisChip(view, fpa); // Chip of interest
            pmHDU *hdu = pmHDUFromChip(chip); // Corresponding HDU
            bool mdok;                  // Status of MD lookup
            psMetadata *formats = psMetadataLookupMetadata(&mdok, hdu->format, "FORMATS"); // Special formats
            if (mdok && formats) {
                const char *format = psMetadataLookupStr(&mdok, formats, "CHIP.NUM"); // Format for CHIP.NUM
                if (mdok && format && strcmp(format, "FORTRAN") == 0) {
                    chipNum++;
                }
            }
            psStringAppend(&name, "%d", chipNum);
        }
        psStringSubstitute(&newName, name, "{CHIP.NUM}");
        psFree(name);
    }
    if (strstr(newName, "{CELL.NAME}")) {
        const char *name = NULL;        // Name of cell
        pmCell *cell = pmFPAviewThisCell(view, fpa);
        if (cell) {
            name = psMetadataLookupStr(NULL, cell->concepts, "CELL.NAME");
            psAssert(name, "All cells should have a name");
        } else {
            name = "chip";
        }
        psStringSubstitute(&newName, name, "{CELL.NAME}");
    }
    if (strstr(newName, "{CELL.N}")) {
        char *name = NULL;
        if (view->cell < 0) {
            psStringAppend(&name, "XX");
        } else {
            psStringAppend(&name, "%02d", view->cell);
        }
        psStringSubstitute(&newName, name, "{CELL.N}");
    }
    if (strstr(newName, "{CELL.NUM}")) {
        char *name = NULL;
        if (view->cell < 0) {
            psStringAppend(&name, "XX");
        } else {
            int cellNum = view->cell;   // Number of cell
            // Potential correction for fortran (unit-indexed) numbering
            pmCell *cell = pmFPAviewThisCell(view, fpa); // Cell of interest
            pmHDU *hdu = pmHDUFromCell(cell); // Corresponding HDU
            bool mdok;                  // Status of MD lookup
            psMetadata *formats = psMetadataLookupMetadata(&mdok, hdu->format, "FORMATS"); // Special formats
            if (mdok && formats) {
                const char *format = psMetadataLookupStr(&mdok, formats, "CELL.NUM"); // Format for CELL.NUM
                if (mdok && format && strcmp(format, "FORTRAN") == 0) {
                    cellNum++;
                }
            }
            psStringAppend(&name, "%d", cellNum);
        }
        psStringSubstitute(&newName, name, "{CELL.NUM}");
    }
    if (strstr(newName, "{EXTNAME}")) {
        pmHDU *hdu = pmFPAviewThisHDU(view, fpa);
        if (hdu->extname && *hdu->extname) {
            psStringSubstitute(&newName, hdu->extname, "{EXTNAME}");
        }
    }
    if (strstr(newName, "{FILTER}") && fpa) {
        char *name = psMetadataLookupStr(NULL, fpa->concepts, "FPA.FILTER");
        if (name && *name) {
            psStringSubstitute(&newName, name, "{FILTER}");
        }
    }
    if (strstr(newName, "{FILTER.ID}") && fpa) {
        char *name = psMetadataLookupStr(NULL, fpa->concepts, "FPA.FILTERID");
        if (name && *name) {
            psStringSubstitute(&newName, name, "{FILTER.ID}");
        }
    }
    if (strstr(newName, "{CAMERA}") && fpa) {
        char *name = psMetadataLookupStr(NULL, fpa->concepts, "FPA.INSTRUMENT");
        if (name && *name) {
            psStringSubstitute(&newName, name, "{CAMERA}");
        }
    }
    if (strstr(newName, "{INSTRUMENT}") && fpa) {
        char *name = psMetadataLookupStr(NULL, fpa->concepts, "FPA.INSTRUMENT");
        if (name && *name) {
            psStringSubstitute(&newName, name, "{INSTRUMENT}");
        }
    }
    if (strstr(newName, "{DETECTOR}") && fpa) {
        char *name = psMetadataLookupStr(NULL, fpa->concepts, "FPA.DETECTOR");
        if (name && *name) {
            psStringSubstitute(&newName, name, "{DETECTOR}");
        }
    }
    if (strstr(newName, "{TELESCOPE}") && fpa) {
        char *name = psMetadataLookupStr(NULL, fpa->concepts, "FPA.TELESCOPE");
        if (name && *name) {
            psStringSubstitute(&newName, name, "{TELESCOPE}");
        }
    }
    return newName;
}

// select the rule from the camera configuration, perform substitutions as needed
psString pmFPAfileNameFromRule(const char *rule, const pmFPAfile *file, const pmFPAview *view)
{
    PS_ASSERT_PTR_NON_NULL(rule, NULL);
    PS_ASSERT_INT_POSITIVE(strlen(rule), NULL);
    PS_ASSERT_PTR_NON_NULL(file, NULL);
    PS_ASSERT_PTR_NON_NULL(view, NULL);

    psString newRule = NULL;            // Rule to pass on to pmFPANameFromRule
    newRule = psStringCopy(rule);

    if (strstr(newRule, "{OUTPUT}") != NULL) {
        char *name = psMetadataLookupStr(NULL, file->names, "OUTPUT");
        if (name) {
            psStringSubstitute(&newRule, name, "{OUTPUT}");
        }
    }

    if (strstr(newRule, "{FILE.INDEX}")) {
        // Number of the file in list
        psString num = NULL;            // Number to use
        psStringAppend(&num, "%d", file->fileIndex);
        psStringSubstitute(&newRule, num, "{FILE.INDEX}");
        psFree(num);
    }

    if (strstr(newRule, "{FILE.ID}")) {
        // Number of the file in list
        psString num = NULL;            // Number to use
        psStringAppend(&num, "%" PRId64, file->fileID);
        psStringSubstitute(&newRule, num, "{FILE.ID}");
        psFree(num);
    }

    psString newName = pmFPANameFromRule(newRule, file->fpa, view); // New name, to be returned
    psFree(newRule);

    return newName;
}

// given an already-opened fits file, write the components corresponding
// to the specified view
bool pmFPAfileCopyView (pmFPA *out, pmFPA *in, const pmFPAview *view)
{
    PS_ASSERT_PTR_NON_NULL(out, false);
    PS_ASSERT_PTR_NON_NULL(in, false);
    PS_ASSERT_PTR_NON_NULL(view, false);

    // pmFPAWrite takes care of all PHUs as needed
    if (view->chip == -1) {
        pmFPACopy (out, in);
        return true;
    }
    if (view->chip >= in->chips->n) {
        psError(PS_ERR_IO, true, "Requested chip == %d >= in->chips->n == %ld", view->chip, in->chips->n);
        return false;
    }
    pmChip *inChip = in->chips->data[view->chip];
    pmChip *outChip = out->chips->data[view->chip];

    if (view->cell == -1) {
        pmChipCopy (outChip, inChip);
        return true;
    }
    if (view->cell >= inChip->cells->n) {
        psError(PS_ERR_IO, true, "Requested cell == %d>= inChip->cells->n == %ld",
                view->cell, inChip->cells->n);
        return false;
    }
    pmCell *inCell = inChip->cells->data[view->cell];
    pmCell *outCell = outChip->cells->data[view->cell];

    if (view->readout == -1) {
        pmCellCopy (outCell, inCell);
        return true;
    }
    psError(PS_ERR_UNKNOWN, true, "Returning false");
    return false;

    // XXX add readout / segment equivalents
}

// given an already-opened fits file, write the components corresponding
// to the specified view
bool pmFPAfileCopyStructureView (pmFPA *out, const pmFPA *in, int xBin, int yBin, const pmFPAview *view)
{
    bool status;
    PS_ASSERT_PTR_NON_NULL(out, false);
    PS_ASSERT_PTR_NON_NULL(in, false);
    PS_ASSERT_PTR_NON_NULL(view, false);

    // XXX this should be smarter (ie, only copy concepts from the current chips)
    // but such a call is needed, so re-copy stuff rather than no copy
    pmConceptsCopyFPA(out, in, true, true);

    // pmFPAWrite takes care of all PHUs as needed
    if (view->chip == -1) {
        status = pmFPACopyStructure (out, in, xBin, yBin);
        return status;
    }
    if (view->chip >= in->chips->n) {
        psError(PS_ERR_IO, true, "Requested chip == %d >= in->chips->n == %ld", view->chip, in->chips->n);
        return false;
    }
    pmChip *inChip = in->chips->data[view->chip];
    pmChip *outChip = out->chips->data[view->chip];

    if (view->cell == -1) {
        status = pmChipCopyStructure (outChip, inChip, xBin, yBin);
        return status;
    }
    if (view->cell >= inChip->cells->n) {
        psError(PS_ERR_IO, true, "Requested cell == %d>= inChip->cells->n == %ld",
                view->cell, inChip->cells->n);
        return false;
    }
    pmCell *inCell = inChip->cells->data[view->cell];
    pmCell *outCell = outChip->cells->data[view->cell];

    status = pmCellCopyStructure (outCell, inCell, xBin, yBin);
    return status;
}

pmFPAfileType pmFPAfileTypeFromString(const char *type)
{
    PS_ASSERT_STRING_NON_EMPTY(type, PM_FPA_FILE_NONE);

    if (!strcasecmp(type, "SX"))     {
        return PM_FPA_FILE_SX;
    }
    if (!strcasecmp(type, "OBJ"))     {
        return PM_FPA_FILE_OBJ;
    }
    if (!strcasecmp(type, "CMP"))     {
        return PM_FPA_FILE_CMP;
    }
    if (!strcasecmp(type, "CMF"))     {
        return PM_FPA_FILE_CMF;
    }
    if (!strcasecmp(type, "CFF"))     {
        return PM_FPA_FILE_CFF;
    }
    if (!strcasecmp(type, "WCS"))     {
        return PM_FPA_FILE_WCS;
    }
    if (!strcasecmp(type, "RAW"))     {
        return PM_FPA_FILE_RAW;
    }
    if (!strcasecmp(type, "IMAGE"))     {
        return PM_FPA_FILE_IMAGE;
    }
    if (!strcasecmp(type, "PSF"))     {
        return PM_FPA_FILE_PSF;
    }
    if (!strcasecmp(type, "JPEG"))     {
        return PM_FPA_FILE_JPEG;
    }
    if (!strcasecmp(type, "KAPA"))     {
        return PM_FPA_FILE_KAPA;
    }
    if (!strcasecmp(type, "MASK"))     {
        return PM_FPA_FILE_MASK;
    }
    if (!strcasecmp(type, "VARIANCE"))     {
        return PM_FPA_FILE_VARIANCE;
    }
    if (!strcasecmp(type, "FRINGE")) {
        return PM_FPA_FILE_FRINGE;
    }
    if (!strcasecmp(type, "DARK"))     {
        return PM_FPA_FILE_DARK;
    }
    if (!strcasecmp(type, "HEADER"))     {
        return PM_FPA_FILE_HEADER;
    }
    if (!strcasecmp(type, "LINEARITY"))  {
      return PM_FPA_FILE_LINEARITY;
    }
    if (!strcasecmp(type, "NEWNONLIN"))  {
      return PM_FPA_FILE_NEWNONLIN;
    }
    if (!strcasecmp(type, "ASTROM"))     {
      return PM_FPA_FILE_ASTROM_MODEL;
    }
    if (!strcasecmp(type, "ASTROM.MODEL"))     {
        return PM_FPA_FILE_ASTROM_MODEL;
    }
    if (!strcasecmp(type, "ASTROM.REFSTARS"))     {
        return PM_FPA_FILE_ASTROM_REFSTARS;
    }
    if (!strcasecmp(type, "KH.CORRECT"))     {
        return PM_FPA_FILE_KH_CORRECT;
    }
    if (!strcasecmp(type, "PATTERN.ROW.AMP"))     {
        return PM_FPA_FILE_PATTERN_ROW_AMP;
    }
    if (!strcasecmp(type, "PATTERN.DEAD.CELLS"))     {
        return PM_FPA_FILE_PATTERN_DEAD_CELLS;
    }
    if (!strcasecmp(type, "SUBKERNEL"))     {
        return PM_FPA_FILE_SUBKERNEL;
    }
    if (!strcasecmp(type, "PATTERN")) {
        return PM_FPA_FILE_PATTERN;
    }
    if (!strcasecmp(type, "EXPNUM")) {
        return PM_FPA_FILE_EXPNUM;
    }

    return PM_FPA_FILE_NONE;
}

const char *pmFPAfileStringFromType(pmFPAfileType type)
{
    switch (type) {
      case PM_FPA_FILE_SX:
        return ("SX");
      case PM_FPA_FILE_OBJ:
        return ("OBJ");
      case PM_FPA_FILE_CMP:
        return ("CMP");
      case PM_FPA_FILE_CMF:
        return ("CMF");
      case PM_FPA_FILE_CFF:
        return ("CFF");
      case PM_FPA_FILE_WCS:
        return ("WCS");
      case PM_FPA_FILE_RAW:
        return ("RAW");
      case PM_FPA_FILE_IMAGE:
        return ("IMAGE");
      case PM_FPA_FILE_PSF:
        return ("PSF");
      case PM_FPA_FILE_JPEG:
        return ("JPEG");
      case PM_FPA_FILE_KAPA:
        return ("KAPA");
      case PM_FPA_FILE_MASK:
        return ("MASK");
      case PM_FPA_FILE_VARIANCE:
        return ("VARIANCE");
      case PM_FPA_FILE_FRINGE:
        return ("FRINGE");
      case PM_FPA_FILE_DARK:
        return("DARK");
      case PM_FPA_FILE_HEADER:
        return ("HEADER");
      case PM_FPA_FILE_ASTROM_MODEL:
        return ("ASTROM.MODEL");
      case PM_FPA_FILE_ASTROM_REFSTARS:
        return ("ASTROM.REFSTARS");
      case PM_FPA_FILE_KH_CORRECT:
        return ("KH.CORRECT");
      case PM_FPA_FILE_PATTERN_ROW_AMP:
        return ("PATTERN.ROW.AMP");
      case PM_FPA_FILE_PATTERN_DEAD_CELLS:
        return ("PATTERN.DEAD.CELLS");
      case PM_FPA_FILE_SUBKERNEL:
        return ("SUBKERNEL");
      case PM_FPA_FILE_PATTERN:
        return "PATTERN";
      case PM_FPA_FILE_EXPNUM:
        return "EXPNUM";
      default:
        return ("NONE");
    }
    return ("NONE");
}


psArray *pmFPAfileSelect(psMetadata *files, const char *name)
{
    PS_ASSERT_PTR_NON_NULL(files, NULL);

    psList *list = psListAlloc(NULL);   // List of files selected

    psString regex = NULL;              // Regular expression
    if (name) {
        if (!psMetadataLookup(files, name)) {
            psFree (list);
            return NULL;
        }
        psStringAppend(&regex, "^%s$", name);
    }
    psMetadataIterator *iter = psMetadataIteratorAlloc(files, PS_LIST_HEAD, regex); // Iterator
    psFree(regex);
    psMetadataItem *item;               // Item from iteration
    while ((item = psMetadataGetAndIncrement(iter))) {
        pmFPAfile *file = item->data.V; // File of iterest
        psListAdd(list, PS_LIST_TAIL, file);
    }
    psFree(iter);

    psArray *array = psListToArray(list); // Array generated from list
    psFree(list);

    return array;
}

pmFPAfile *pmFPAfileSelectSingle(psMetadata *files, const char *name, int num)
{
    PS_ASSERT_PTR_NON_NULL(files, NULL);
    PS_ASSERT_INT_NONNEGATIVE(num, NULL);

    psString regex = NULL;              // Regular expression
    if (name) {
        if (!psMetadataLookup(files, name)) {
            // No files
            return NULL;
        }
        psStringAppend(&regex, "^%s$", name);
    }

    psMetadataIterator *iter = psMetadataIteratorAlloc(files, PS_LIST_HEAD, regex); // Iterator
    psFree(regex);
    psMetadataItem *item;               // Item from iteration
    int i = 0;                          // Counter
    while ((item = psMetadataGetAndIncrement(iter))) {
        if (i++ == num) {
            psFree(iter);
            return item->data.V;
        }
    }
    psFree(iter);

    psLogMsg("psModules.camera", PS_LOG_MINUTIA, "Unable to find instance %d of file %s", num, name);
    return NULL;
}
