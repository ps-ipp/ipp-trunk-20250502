/** @file psastroInternal.h
 *
 *  @brief
 *
 *  @ingroup psastro
 *
 *  @author IfA
 *  @version $Revision: 1.3 $ $Name: not supported by cvs2svn $
 *  @date $Date: 2009-02-07 02:03:34 $
 *  Copyright 2009 Institute for Astronomy, University of Hawaii
 */

# ifdef HAVE_CONFIG_H
# include <config.h>
# endif

# ifndef PSASTRO_INTERNAL_H
# define PSASTRO_INTERNAL_H

# include <stdio.h>
# include <string.h>
# include <strings.h>  // for strcasecmp
# include <unistd.h>   // for unlink
# include <pslib.h>
# include <psmodules.h>

# define PSASTRO_RECIPE "PSASTRO" ///< Name of the recipe to use

# define psMemCopy(A)(psMemIncrRefCounter((A)))
# define DEG_RAD 57.295779513082322
# define RAD_DEG  0.017453292519943
# define SIGN(X)  (((X) == 0) ? 0 : ((fabs((double)(X))) / (X)))

/**
 * this structure defines the parameters to describe a ghost
 */
typedef struct {
    psPlane *srcFP;			///< location in FPA coords of the source star
    psPlane *FP;			///< location in FPA coords of the ghost center
    psPlane *chip;			///< location in chip coords of the ghost center
    double Mag;				///< instrumental magnitude of source star
    psEllipseAxes inner;		///< inner elliptical annulus boundary
    psEllipseAxes outer;		///< outer elliptical annulus boundary
} pmSourceGhost;

pmSourceGhost     *pmSourceGhostAlloc (void);

pmChip           *pmSourceFindChip (double *xChip, double *yChip, pmFPA *fpa, double xFPA, double yFPA);
bool 		  pmSourceChipBounds (pmFPA *fpa);
bool              pmSourceFindChipInXrange (pmFPA *fpa, int nChip, double xFPA, double yFPA);
bool              pmSourceFindChipInYrange (pmFPA *fpa, int nChip, double xFPA, double yFPA);
bool 		  pmSourceFindChipYedges (double *yFPAs, double *yFPAe, pmFPA *fpa, int nChip);
bool 		  pmSourceFindChipXedges (double *yFPAs, double *yFPAe, pmFPA *fpa, int nChip);
bool 		  pmSourceFPAtoChip (double *xChip, double *yChip, pmFPA *fpa, int nChip, double xFPA, double yFPA);
bool              pmSourceExtractFreeChipBounds(void);


#endif
