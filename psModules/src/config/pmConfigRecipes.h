/*  @file pmConfigRecipes.h
 *  @brief Configuration Recipe functions
 *
 *  @author ?, MHPCC
 *  @author Paul Price, IfA
 *  @author Eugene Magnier, IfA
 *
 *  @version $Revision: 1.4 $ $Name: not supported by cvs2svn $
 *  @date $Date: 2007-04-19 02:10:12 $
 *  Copyright 2005-2006 Institute for Astronomy, University of Hawaii
 */

#ifndef PM_CONFIG_RECIPES_H
#define PM_CONFIG_RECIPES_H

#include <pmConfig.h>

/// @addtogroup Config Configuration System
/// @{

/// Read recipes
///
/// Attempt to read recipes from the sources that are available but have not already been read.  Having read a
/// recipe, attempt to resolve symbolic links that were specified on the command line.
bool pmConfigReadRecipes(pmConfig *config, ///< Configuration
                         pmRecipeSource source ///< desired sources for recipes
                        );


bool pmConfigLoadRecipeArguments(int *argc, char **argv, pmConfig *config);
bool pmConfigLoadRecipeOptions(int *argc, char **argv, pmConfig *config, char *flag);
psMetadata *pmConfigRecipeOptions(pmConfig *config, char *recipe);

/// @}
#endif
