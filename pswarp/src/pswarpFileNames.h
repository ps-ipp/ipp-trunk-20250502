/** this file contains the lists of file rules used by pswarpLoop at different analysis steps
    we use these lists to turn on or off different I/O files at different stages **/

// Lists must end with a NULL

// Lists of file rules for the detectors and skycells, and an independent list of everything else


// Lists of file rules for the detectors
static char *detectorFiles[] = {
  "PSWARP.INPUT",
  "PSWARP.MASK",
  "PSWARP.VARIANCE",
  "PSWARP.BKGMODEL",
  NULL
};

// Lists of file rules for the skycells
static char *skycellFiles[] = {
  "PSWARP.OUTPUT",
  "PSWARP.OUTPUT.MASK",
  "PSWARP.OUTPUT.VARIANCE",
  "PSWARP.OUTPUT.BKGMODEL",
  NULL
};

// Lists of file rules for photometry
static char *photFiles[] = {
  "PSPHOT.INPUT",
  "PSPHOT.OUTPUT",
#if PSPHOT_FIND_PSF
  "PSPHOT.INPUT.CMF",
#endif
  "PSPHOT.RESID",
  "PSPHOT.BACKMDL",
  "PSPHOT.BACKMDL.STDEV",
  "PSPHOT.BACKGND",
  "PSPHOT.BACKSUB",
  "PSPHOT.PSF.SAVE",
  "SOURCE.PLOT.MOMENTS",
  "SOURCE.PLOT.PSFMODEL",
  "SOURCE.PLOT.APRESID",
  NULL
};

// Lists of file rules for the detectors and skycells, and an independent list of everything else
static char *independentFiles[] = {
  "PSWARP.ASTROM", // Read independently from the input pixels
  "PSWARP.SKYCELL", // Don't care about the skycell once we have its WCS
  "PSWARP.OUTPUT.SOURCES", // Save these independently so we can do the PSF
  NULL
};

