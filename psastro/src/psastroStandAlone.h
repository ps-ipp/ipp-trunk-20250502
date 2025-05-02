/** @file psastroStandAlone.h
 *
 *  @brief
 *
 *  @ingroup psastro
 *
 *  @author IfA
 *  @version $Revision: 1.7 $ $Name: not supported by cvs2svn $
 *  @date $Date: 2009-02-07 02:03:34 $
 *  Copyright 2009 Institute for Astronomy, University of Hawaii
 */

# ifdef HAVE_CONFIG_H
# include <config.h>
# endif

#ifndef PSASTRO_STAND_ALONE_H
#define PSASTRO_STAND_ALONE_H

#include <stdio.h>
#include <string.h>
#include <pslib.h>
#include <psmodules.h>
#include "psastro.h"

// Top level functions
pmConfig         *psastroArguments (int argc, char **argv);
void              psastroCleanup (pmConfig *config);
bool              psastroParseCamera (pmConfig *config);
bool              psastroDataLoad (pmConfig *config);

pmConfig         *psastroModelArguments (int argc, char **argv);
bool 		  psastroModelParseCamera (pmConfig *config);
bool 		  psastroModelDataLoad (pmConfig *config);
bool 		  psastroModelAnalysis (pmConfig *config);
bool 		  psastroModelAdjust (pmConfig *config);
bool 		  psastroModelDataSave (pmConfig *config);

psVector         *psastroModelFitBoresite (psVector *Xo, psVector *Yo, psVector *Po, char *outroot);
psF32 		  psastroModelBoresite (psVector *deriv, const psVector *params, const psVector *coord);

// these are used to define the boresite model parameters
# define PAR_X0  0  // Xo = params[0] 
# define PAR_Y0  1  // Yo = params[1] 
# define PAR_RX  2  // RX = params[2] 
# define PAR_RY  3  // RY = params[3] 
# define PAR_P0  4  // P0 = params[4]
# define PAR_T0  5  // phi = params[4]

#endif
