/** @file ppSubDefineOutput.c
 *
 *  @brief
 *
 *  @ingroup ppSub
 *
 *  @author IfA
 *  @version $Revision: 1.3 $ $Name: not supported by cvs2svn $
 *  @date $Date: 2009-02-18 00:31:20 $
 *  Copyright 2009 Institute for Astronomy, University of Hawaii
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <libgen.h>
#include <pslib.h>
#include <psmodules.h>

#include "ppSub.h"

bool ppSubCopyWarpToChip (psMetadata *tgtHeader, psMetadata *srcHeader);

bool ppSubDefineOutput(const char *name, pmConfig *config)
{
    psAssert(name, "Require name");
    psAssert(config, "Require configuration");

    bool status = false;

    pmFPAview *view = ppSubViewReadout(); // View to readout
    pmCell *outCell = pmFPAfileThisCell(config->files, view, name); // Output cell
    pmFPA *outFPA = outCell->parent->parent; // Output FPA
    pmHDU *outHDU = outFPA->hdu;        // Output HDU
    if (!outHDU->header) {
        outHDU->header = psMetadataAlloc();
    }

    // The output readout may already be present if we read in the convolution kernel
    pmReadout *outRO = NULL;            // Output readout
    if (outCell->readouts && outCell->readouts->n > 0 && outCell->readouts->data[0]) {
        outRO = psMemIncrRefCounter(outCell->readouts->data[0]);
    } else {
        outRO = pmReadoutAlloc(outCell);
    }

    // Convolved input images
    psMetadata *recipe = psMetadataLookupPtr(&status, config->recipes, PPSUB_RECIPE);
    bool noConvolve = psMetadataLookupBool(&status, recipe, "NOCONVOLVE"); // Do not use convolved images.
    bool reverse = psMetadataLookupBool(&status, config->arguments, "REVERSE"); // Reverse sense of subtraction?
    bool mapFromPositive = psMetadataLookupBool(&status, config->arguments, "CHIP.MAP.FROM.POSITIVE"); 
    // mapFromPositive = true means "always grab the warp->chip map from the positive image (eg, A for A - B, B for B - A)
    // mapFromPositive = false means "always grab the warp->chip map from the first image (eg, PPSUB.INPUT if not 'reverse')

    pmReadout *inConv;
    if (noConvolve) {
      inConv = pmFPAfileThisReadout(config->files, view, "PPSUB.INPUT"); // Input readout
    } else {
      inConv = pmFPAfileThisReadout(config->files, view, "PPSUB.INPUT.CONV"); // Input readout
    }
    pmReadout *refConv;
    if (noConvolve) {
      refConv = pmFPAfileThisReadout(config->files, view, "PPSUB.REF"); // Reference readout
    } else {
      refConv = pmFPAfileThisReadout(config->files, view, "PPSUB.REF.CONV"); // Reference readout
    }

    // Add kernel descrption to header.
    // We don't know which readout has the kernels because it depends on which PSF is larger
    bool mdok;                          // Status of MD lookup
    psMetadata *analysis = inConv->analysis; // Analysis metadata with kernel information
    pmHDU *hdu = pmHDUFromCell(inConv->parent);
    pmSubtractionKernels *kernels = psMetadataLookupPtr(&mdok, analysis, PM_SUBTRACTION_ANALYSIS_KERNEL); // Subtraction kernel

    if (!kernels) {
        hdu = pmHDUFromCell(refConv->parent);
        analysis = refConv->analysis;
        kernels = psMetadataLookupPtr(&mdok, analysis, PM_SUBTRACTION_ANALYSIS_KERNEL);
    }
    if (!kernels && !noConvolve) {
        psError(PPSUB_ERR_UNKNOWN, true, "Unable to find SUBTRACTION.KERNEL");
        psFree(outRO);
	psFree(view);
        return false;
    }
    psAssert (hdu, "unable to find HDU");
    if (!noConvolve) {
      psMetadataAddStr(outHDU->header, PS_LIST_TAIL, "PPSUB.KERNEL", 0, "Subtraction kernel", kernels->description);
    }
    outHDU->header = psMetadataCopy(outHDU->header, hdu->header);

    // in warp-stack mode, we should always use the warp 
    // in warp-warp mode, we should use the positive warp for the positive subtraction

    bool normalDiff = true;
    if (!strcmp (name, "PPSUB.INVERSE"))  {
      normalDiff = false;
    }

    if (reverse) {
      // normal  = PPSUB.REF - PPSUB.INPUT
      // inverse = PPSUB.INPUT - PPSUB.REF
      if (mapFromPositive) {
	pmCell *cell_Pos = normalDiff ? pmFPAfileThisCell(config->files, view, "PPSUB.REF") : pmFPAfileThisCell(config->files, view, "PPSUB.INPUT");
	pmHDU *hdu_Pos = pmHDUFromCell(cell_Pos);
	ppSubCopyWarpToChip (outHDU->header, hdu_Pos->header);
      } else {
	pmCell *cell_Pos = pmFPAfileThisCell(config->files, view, "PPSUB.REF");
	pmHDU *hdu_Pos = pmHDUFromCell(cell_Pos);
	ppSubCopyWarpToChip (outHDU->header, hdu_Pos->header);
      }
    } else {
      // normal  = PPSUB.INPUT - PPSUB.REF
      // inverse = PPSUB.REF - PPSUB.INPUT
      if (mapFromPositive) {
	pmCell *cell_Pos = normalDiff ? pmFPAfileThisCell(config->files, view, "PPSUB.INPUT") : pmFPAfileThisCell(config->files, view, "PPSUB.REF");
	pmHDU *hdu_Pos = pmHDUFromCell(cell_Pos);
	ppSubCopyWarpToChip (outHDU->header, hdu_Pos->header);
      } else {
	pmCell *cell_Pos = pmFPAfileThisCell(config->files, view, "PPSUB.INPUT");
	pmHDU *hdu_Pos = pmHDUFromCell(cell_Pos);
	ppSubCopyWarpToChip (outHDU->header, hdu_Pos->header);
      }
    }

    // Add additional data to the header
    pmFPAfile *refFile = psMetadataLookupPtr(NULL, config->files, "PPSUB.REF"); // Reference file
    pmFPAfile *inFile = psMetadataLookupPtr(NULL, config->files, "PPSUB.INPUT"); // Input file

    // save the names of the input and reference image in the header
    psString refBase = psStringFileBasename(refFile->origname);            // Basename of reference
    psMetadataAddStr(outHDU->header, PS_LIST_TAIL, "PPSUB.REFERENCE", 0, "Subtraction reference", refBase);
    psFree(refBase);

    psString inBase = psStringFileBasename(inFile->origname);              // Basename of input
    psMetadataAddStr(outHDU->header, PS_LIST_TAIL, "PPSUB.INPUT", 0, "Subtraction input", inBase);
    psFree(inBase);

    ppSubVersionHeader(outHDU->header);

    outRO->analysis = psMetadataCopy(outRO->analysis, analysis);

    psFree(outRO);
    psFree(view);

    return true;
}

// we have 4 sets of header keywords to copy:
// SRC_nnnn, SEC_nnnn, MPX_nnnn, MPY_nnnn

bool ppSubCopyWarpToChip (psMetadata *tgtHeader, psMetadata *srcHeader) {

  char keyword[80];

  bool status = false;

  int Nchip = 0;
  while (true) {
    snprintf (keyword, 80, "SRC_%04d", Nchip);
    char *string = psMetadataLookupStr (&status, srcHeader, keyword);
    if (!status) {
      break;
    }
    psMetadataAddStr(tgtHeader, PS_LIST_TAIL, keyword, PS_META_REPLACE, "input image", string);
    Nchip ++;
  }
    
  for (int i = 0; i < Nchip; i++) {
    snprintf (keyword, 80, "SEC_%04d", i);
    char *string = psMetadataLookupStr (&status, srcHeader, keyword);
    if (!status) {
      psWarning ("cannot find keyword %s\n", keyword);
      continue;
    }
    psMetadataAddStr(tgtHeader, PS_LIST_TAIL, keyword, PS_META_REPLACE, "input image", string);
  }

  for (int i = 0; i < Nchip; i++) {
    snprintf (keyword, 80, "MPX_%04d", i);
    char *string = psMetadataLookupStr (&status, srcHeader, keyword);
    if (!status) {
      psWarning ("cannot find keyword %s\n", keyword);
      continue;
    }
    psMetadataAddStr(tgtHeader, PS_LIST_TAIL, keyword, PS_META_REPLACE, "input image", string);
  }

  for (int i = 0; i < Nchip; i++) {
    snprintf (keyword, 80, "MPY_%04d", i);
    char *string = psMetadataLookupStr (&status, srcHeader, keyword);
    if (!status) {
      psWarning ("cannot find keyword %s\n", keyword);
      continue;
    }
    psMetadataAddStr(tgtHeader, PS_LIST_TAIL, keyword, PS_META_REPLACE, "input image", string);
  }

  // Copy PSREFCAT from the source to the target as well
  snprintf(keyword, 80, "PSREFCAT");
  char *string = psMetadataLookupStr(&status, srcHeader, keyword);
  if (status) {
    psMetadataAddStr(tgtHeader, PS_LIST_TAIL, keyword, PS_META_REPLACE, "input image reference catalog", string);
  }
  
  return true;
}

bool ppSubCopyWarpToChip_Alt (psMetadata *tgtHeader, psMetadata *srcHeader, bool isPositive) {

  char srcKeyword[80], tgtKeyword[80];

  bool status = false;

  int Nchip = 0;
  while (true) {
    snprintf (srcKeyword, 80, "SRC_%04d", Nchip);
    if (isPositive) {
      snprintf (tgtKeyword, 80, "SRCP_%03d", Nchip);
    } else {
      snprintf (tgtKeyword, 80, "SRCM_%03d", Nchip);
    }

    char *string = psMetadataLookupStr (&status, srcHeader, srcKeyword);
    if (!status) {
      break;
    }
    psMetadataAddStr(tgtHeader, PS_LIST_TAIL, srcKeyword, PS_META_REPLACE, "input image", string);
    Nchip ++;
  }
    
  for (int i = 0; i < Nchip; i++) {
    snprintf (srcKeyword, 80, "SEC_%04d", i);
    if (isPositive) {
      snprintf (tgtKeyword, 80, "SECP_%03d", Nchip);
    } else {
      snprintf (tgtKeyword, 80, "SECM_%03d", Nchip);
    }

    char *string = psMetadataLookupStr (&status, srcHeader, srcKeyword);
    if (!status) {
      psWarning ("cannot find keyword %s\n", srcKeyword);
      continue;
    }
    psMetadataAddStr(tgtHeader, PS_LIST_TAIL, srcKeyword, PS_META_REPLACE, "input image", string);
  }

  for (int i = 0; i < Nchip; i++) {
    snprintf (srcKeyword, 80, "MPX_%04d", i);
    if (isPositive) {
      snprintf (tgtKeyword, 80, "MPXP_%03d", Nchip);
    } else {
      snprintf (tgtKeyword, 80, "MPXM_%03d", Nchip);
    }

    char *string = psMetadataLookupStr (&status, srcHeader, srcKeyword);
    if (!status) {
      psWarning ("cannot find keyword %s\n", srcKeyword);
      continue;
    }
    psMetadataAddStr(tgtHeader, PS_LIST_TAIL, srcKeyword, PS_META_REPLACE, "input image", string);
  }

  for (int i = 0; i < Nchip; i++) {
    snprintf (srcKeyword, 80, "MPY_%04d", i);
    if (isPositive) {
      snprintf (tgtKeyword, 80, "MPYP_%03d", Nchip);
    } else {
      snprintf (tgtKeyword, 80, "MPYM_%03d", Nchip);
    }

    char *string = psMetadataLookupStr (&status, srcHeader, srcKeyword);
    if (!status) {
      psWarning ("cannot find keyword %s\n", srcKeyword);
      continue;
    }
    psMetadataAddStr(tgtHeader, PS_LIST_TAIL, srcKeyword, PS_META_REPLACE, "input image", string);
  }
  return true;
}

