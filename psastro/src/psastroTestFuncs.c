/** @file psastroTestFuncs.c
 *
 *  @brief 
 *
 *  @ingroup libpsastro
 *
 *  @author IfA
 *  @version $Revision: 1.7 $ $Name: not supported by cvs2svn $
 *  @date $Date: 2009-02-07 02:03:34 $
 *  Copyright 2009 Institute for Astronomy, University of Hawaii
 */

# include "psastroInternal.h"

/**
 * write out objects
 */
bool psastroWriteStars (char *filename, psArray *sources) {

    // re-open, add data to end of file
    FILE *f = fopen (filename, "w");
    if (f == NULL) {
	psLogMsg ("psastroWriteStars", 3, "can't open output file for output %s\n", filename);
	return false;
    }

    for (int i = 0; i < sources->n; i++) {
	
	pmAstromObj *star = sources->data[i];

	fprintf (f, "%8.2f %8.2f   %8.2f %8.2f   %8.2f %8.2f   %10.6f %10.6f   %8.2f %8.2f\n", 
		 star->chip->x, star->chip->y, 
		 star->FP->x, star->FP->y, 
		 star->TP->x, star->TP->y, 
		 star->sky->r*DEG_RAD, star->sky->d*DEG_RAD, 
		 star->Mag, star->dMag);
    }
    fclose (f);
    return true;
}

bool psastroWriteTransform (psPlaneTransform *map) {

    // dump initial values:
    for (int i = 0; i < map->x->nX + 1; i++) {
	for (int j = 0; j < map->x->nY + 1; j++) {
	    if (map->x->coeffMask[i][j] & PS_POLY_MASK_SET) continue;
	    psLogMsg ("psastro", 4, "x term %d,%d: %f +/- %f\n", i, j, map->x->coeff[i][j], map->x->coeffErr[i][j]);
	}
    }

    for (int i = 0; i < map->y->nX + 1; i++) {
	for (int j = 0; j < map->y->nY + 1; j++) {
	    if (map->y->coeffMask[i][j] & PS_POLY_MASK_SET) continue;
	    psLogMsg ("psastro", 4, "y term %d,%d: %f +/- %f\n", i, j, map->y->coeff[i][j], map->y->coeffErr[i][j]);
	}
    }
    return true;
}
