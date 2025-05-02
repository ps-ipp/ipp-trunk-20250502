/** @file fpcamera.h
 *
 *  @brief This file defines the library functions available to external
 *  programs.
 *
 *  It must be included by programs which are compiled against
 *  psphot functions.
 *
 *  @ingroup fpcamera
 *
 *  @author IfA
 *  @version $Revision: 1.49 $ $Name: not supported by cvs2svn $
 *  @date $Date: 2009-02-09 21:25:34 $
 *  Copyright 2009 Institute for Astronomy, University of Hawaii
 */

# ifndef FPCAMERA_H
# define FPCAMERA_H

#include <stdio.h>
#include <string.h>
#include <pslib.h>
#include <psmodules.h>
#include <psphot.h>
#include <ppStats.h>
#include "fpcameraErrorCodes.h"

/// @addtogroup fpcamera
/// @{

# define FPCAMERA_RECIPE "FPCAMERA" ///< Name of the recipe to use

# define psMemCopy(A)(psMemIncrRefCounter((A)))
# define DEG_RAD 57.295779513082322
# define RAD_DEG  0.017453292519943
# define SIGN(X)  (((X) == 0) ? 0 : ((fabs((double)(X))) / (X)))

pmConfig         *fpcameraArguments (int argc, char **argv);
bool 		  fpcameraParseCamera (pmConfig *config);
bool 		  fpcameraDataLoad (pmConfig *config);
bool              fpcameraAnalysis (pmConfig *config, psMetadata *stats);
bool              fpcameraDataSave (pmConfig *config, psMetadata *stats);

void              fpcameraCleanup (pmConfig *config, psMetadata *stats);

bool              fpcameraDefineFiles (pmConfig *config, pmFPAfile *input);
bool              fpcameraDefineFile (pmConfig *config, pmFPA *input, char *filerule, char *argname, pmFPAfileType fileType, pmDetrendType detrendType);

bool              fpcameraLoadRefstars (pmFPAfile *input, pmConfig *config);
bool              fpcameraReadAstrometry (pmFPAfile *input, pmConfig *config);
bool              fpcameraChooseRefstars (pmFPAfile *input, pmFPAfile *astrom, pmFPAview *view);

// Return version strings.
psString          fpcameraVersion(void);
psString          fpcameraSource(void);
psString          fpcameraVersionLong(void);
bool              fpcameraVersionHeader(psMetadata *header);
bool              fpcameraVersionHeaderFull(psMetadata *header);
void              fpcameraVersionPrint(void);

psArray          *fpcameraReadGetstarCatalog (psFits *fits);
psArray          *fpcameraReadGetstar_PS1_DEV_0 (psFits *fits);

bool              fpcameraMetadataStats (pmConfig *config, psMetadata *stats);

pmChip           *fpcameraFindChip (double *xChip, double *yChip, pmFPA *fpa, double xFPA, double yFPA);
bool 		  fpcameraChipBounds (pmFPA *fpa);
bool              fpcameraFindChipInXrange (pmFPA *fpa, int nChip, double xFPA, double yFPA);
bool              fpcameraFindChipInYrange (pmFPA *fpa, int nChip, double xFPA, double yFPA);
bool 		  fpcameraFindChipYedges (double *yFPAs, double *yFPAe, pmFPA *fpa, int nChip);
bool 		  fpcameraFindChipXedges (double *yFPAs, double *yFPAe, pmFPA *fpa, int nChip);
bool 		  fpcameraFPAtoChip (double *xChip, double *yChip, pmFPA *fpa, int nChip, double xFPA, double yFPA);

bool              fpcameraAstrometryFPAHeader(pmFPA *fpa, pmFPAfile *astrom, psMetadata *stats);
bool              fpcameraAstrometryChipHeader (pmConfig *config, pmFPAview *view, pmReadout *readout, pmFPAfile *astrom);

///@}
# endif /* FPCAMERA_H */
