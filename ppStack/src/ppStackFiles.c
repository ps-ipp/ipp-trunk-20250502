#include "ppStack.h"
# define DEBUG 0

// Here follows lists of files for activation/deactivation at various stages.  Each must be NULL-terminated.

/// NOP list
static char *filesNOP[] = { NULL };

/// Files required in preparation for convolution
static char *filesPrepare[] = { "PPSTACK.INPUT.PSF", "PPSTACK.INPUT.SOURCES", "PPSTACK.TARGET.PSF", NULL };

/// Files required for generating convolution target
static char *filesTarget[] = { "PPSTACK.INPUT.VARIANCE", "PPSTACK.INPUT.MASK", NULL };

/// Files required for the convolution
static char *filesConvolve[] = { "PPSTACK.INPUT", "PPSTACK.INPUT.MASK", "PPSTACK.INPUT.VARIANCE", NULL };

/// Files required for the background
static char *filesBkg[] = { "PPSTACK.INPUT.BKGMODEL", NULL };

/// Files required for median only stacking
static char *filesMedianIn[] =  { "PPSTACK.INPUT",                              			     
                              NULL };
/// Files required for median only stacking
static char *filesMedianOut[] =  { "PPSTACK.OUTPUT",                              			     
                              NULL };

/// Regular (convolved) stack files
static char *filesStack[] = { "PPSTACK.OUTPUT", "PPSTACK.OUTPUT.MASK", "PPSTACK.OUTPUT.VARIANCE",
                              "PPSTACK.OUTPUT.EXP", "PPSTACK.OUTPUT.EXPNUM", "PPSTACK.OUTPUT.EXPWT",
			      "PPSTACK.OUTPUT.BKGMODEL",
                              NULL };
/// Unconvolved stack files
static char *filesUnconv[] = { "PPSTACK.UNCONV", "PPSTACK.UNCONV.MASK", "PPSTACK.UNCONV.VARIANCE",
                               "PPSTACK.UNCONV.EXP", "PPSTACK.UNCONV.EXPNUM", "PPSTACK.UNCONV.EXPWT",
                               NULL };

/// Files for photometry
static char *filesPhot[] = { "PSPHOT.INPUT", "PSPHOT.OUTPUT", "PSPHOT.RESID", "PSPHOT.BACKMDL",
                             "PSPHOT.BACKMDL.STDEV", "PSPHOT.BACKGND", "PSPHOT.BACKSUB",
                             "SOURCE.PLOT.MOMENTS", "SOURCE.PLOT.PSFMODEL", "SOURCE.PLOT.APRESID",
                             "PSPHOT.INPUT.CMF", NULL };

static char **stackFiles(ppStackFileList list)
{
    switch (list) {
      case PPSTACK_FILES_NONE:       return filesNOP;
      case PPSTACK_FILES_PREPARE:    return filesPrepare;
      case PPSTACK_FILES_TARGET:     return filesTarget;
      case PPSTACK_FILES_CONVOLVE:   return filesConvolve;
      case PPSTACK_FILES_STACK:      return filesStack;
      case PPSTACK_FILES_UNCONV:     return filesUnconv;
      case PPSTACK_FILES_PHOT:       return filesPhot;
      case PPSTACK_FILES_BKG:        return filesBkg;
      case PPSTACK_FILES_MEDIAN_IN:  return filesMedianIn;
      case PPSTACK_FILES_MEDIAN_OUT: return filesMedianOut;
      default:
        psAbort("Unrecognised file list: %x", list);
    }
    return NULL;
}


void ppStackMemDump(const char *name)
{
    return;

    static int num = 0;                 // Counter, to make files unique and give an idea of sequence

    psString filename = NULL;           // Name of file
    psStringAppend(&filename, "memdump_%s_%03d.txt", name, num);
    FILE *memFile = fopen(filename, "w");
    psFree(filename);

    psMemBlock **leaks = NULL;
    int numLeaks = psMemCheckLeaks(0, &leaks, NULL, true);
    fprintf(memFile, "# MemBlock Size Source\n");
    unsigned long total = 0;            // Total memory used
    for (int i = 0; i < numLeaks; i++) {
        psMemBlock *mb = leaks[i];
        fprintf(memFile, "%12lu\t%12zd\t%s:%d\n", mb->id, mb->userMemorySize, mb->file, mb->lineno);
        total += mb->userMemorySize;
    }
    fclose(memFile);
    psFree(leaks);

    fprintf(stderr, "Memdump %s %d: Memory use: %ld, sbrk: %p\n", name, num, total, sbrk(0));
    num++;
}

// Activate/deactivate a list of files
void ppStackFileActivation(pmConfig *config, // Configuration
                           ppStackFileList list, // Files to turn on/off
                           bool state   // Activation state
    )
{
    assert(config);

    char **files = stackFiles(list);    // Files to turn on/off
    for (int i = 0; files[i] != NULL; i++) {
	if (DEBUG) {
	    if (state) {
		fprintf (stderr, "activate %s\n", files[i]);
	    } else {
		fprintf (stderr, "deativate %s\n", files[i]);
	    }
	}
        pmFPAfileActivate(config->files, state, files[i]);
    }
    return;
}

// Activate/deactivate a single element for a list
void ppStackFileActivationSingle(pmConfig *config, // Configuration
                                 ppStackFileList list, // Files to turn on/off
                                 bool state,   // Activation state
                                 int num // Number of file in sequence
                                 )
{
    assert(config);

    char **files = stackFiles(list);    // Files to turn on/off
    for (int i = 0; files[i] != NULL; i++) {
        pmFPAfileActivateSingle(config->files, state, files[i], num); // Activated file
    }
    return;
}

// Iterate down the hierarchy, loading files; we can get away with this because we're working on skycells
pmFPAview *ppStackFilesIterateDown(pmConfig *config // Configuration
    )
{
    assert(config);

    pmFPAview *view = pmFPAviewAlloc(0);// Pointer into FPA hierarchy
    if (!pmFPAfileIOChecks(config, view, PM_FPA_BEFORE)) {
        psError(PPSTACK_ERR_IO, false, "File checks failed.");
        return NULL;
    }
    view->chip = 0;
    if (!pmFPAfileIOChecks(config, view, PM_FPA_BEFORE)) {
        psError(PPSTACK_ERR_IO, false, "File checks failed.");
        return NULL;
    }
    view->cell = 0;
    if (!pmFPAfileIOChecks(config, view, PM_FPA_BEFORE)) {
        psError(PPSTACK_ERR_IO, false, "File checks failed.");
        return NULL;
    }
    view->readout = 0;
    if (!pmFPAfileIOChecks(config, view, PM_FPA_BEFORE)) {
        psError(PPSTACK_ERR_IO, false, "File checks failed.");
        return NULL;
    }
    return view;
}

// Iterate up the hierarchy, writing files; we can get away with this because we're working on skycells
bool ppStackFilesIterateUp(pmConfig *config // Configuration
                           )
{
    assert(config);

    pmFPAview *view = pmFPAviewAlloc(0);// Pointer into FPA hierarchy
    view->chip = view->cell = view->readout = 0;
    if (!pmFPAfileIOChecks(config, view, PM_FPA_AFTER)) {
        psError(PPSTACK_ERR_IO, false, "File checks failed.");
        return false;
    }
    view->readout = -1;
    if (!pmFPAfileIOChecks(config, view, PM_FPA_AFTER)) {
        psError(PPSTACK_ERR_IO, false, "File checks failed.");
        return false;
    }
    view->cell = -1;
    if (!pmFPAfileIOChecks(config, view, PM_FPA_AFTER)) {
        psError(PPSTACK_ERR_IO, false, "File checks failed.");
        return false;
    }
    view->chip = -1;
    if (!pmFPAfileIOChecks(config, view, PM_FPA_AFTER)) {
        psError(PPSTACK_ERR_IO, false, "File checks failed.");
        return false;
    }
    psFree(view);
    return true;
}

// Write an image to a FITS file
bool ppStackWriteImage(const char *name, // Name of image
                       psMetadata *header, // Header
                       const psImage *image, // Image
                       pmConfig *config // Configuration
    )
{
    assert(name);
    assert(image);

    psString resolved = pmConfigConvertFilename(name, config, true, true); // Resolved file name
    psFits *fits = psFitsOpen(resolved, "w");
    if (!fits) {
        psError(PPSTACK_ERR_IO, false, "Unable to open FITS file %s to write image.", resolved);
        psFree(resolved);
        return false;
    }
    if (!psFitsWriteImage(fits, header, image, 0, NULL)) {
        psError(PPSTACK_ERR_IO, false, "Unable to write FITS image %s.", resolved);
        psFitsClose(fits);
        psFree(resolved);
        return false;
    }
    if (!psFitsClose(fits)) {
        psError(PPSTACK_ERR_IO, false, "Unable to close FITS image %s.", resolved);
        psFree(resolved);
        return false;
    }
    psFree(resolved);
    return true;
}

// Write an image to a FITS file
bool ppStackWriteVariance(const char *name, // Name of image
			  psMetadata *header, // Header
			  const psImage *variance, // Variance
			  const psImage *covariance, // Variance
			  pmConfig *config // Configuration
    )
{
    assert(name);
    assert(variance);

    psString resolved = pmConfigConvertFilename(name, config, true, true); // Resolved file name
    psFits *fits = psFitsOpen(resolved, "w");
    if (!fits) {
        psError(PPSTACK_ERR_IO, false, "Unable to open FITS file %s to write image.", resolved);
        psFree(resolved);
        return false;
    }
    if (!psFitsWriteImage(fits, header, variance, 0, NULL)) {
        psError(PPSTACK_ERR_IO, false, "Unable to write FITS image %s.", resolved);
        psFitsClose(fits);
        psFree(resolved);
        return false;
    }
    if (covariance) {
	psMetadata *tmphead = psMetadataAlloc();
	psMetadataAddS32(tmphead, PS_LIST_TAIL, "COVARIANCE.CENTRE.X", PS_META_REPLACE, "center", (int)(covariance->numCols / 2));
	psMetadataAddS32(tmphead, PS_LIST_TAIL, "COVARIANCE.CENTRE.Y", PS_META_REPLACE, "center", (int)(covariance->numRows / 2));
	if (!psFitsWriteImage(fits, tmphead, covariance, 0, "COVAR_SkyChip_SkyCell")) {
	    psError(PPSTACK_ERR_IO, false, "Unable to write FITS image %s.", resolved);
	    psFitsClose(fits);
	    psFree(resolved);
	    return false;
	}
    }
    if (!psFitsClose(fits)) {
        psError(PPSTACK_ERR_IO, false, "Unable to close FITS image %s.", resolved);
        psFree(resolved);
        return false;
    }
    psFree(resolved);
    return true;
}
