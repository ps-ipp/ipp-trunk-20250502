/** @file gpcModel.c
 *
 *  @brief
 *
 *  @ingroup gpcModel
 *
 *  @author IfA
 *  @version $Revision: 1.2 $ $Name: not supported by cvs2svn $
 *  @date $Date: 2009-02-07 02:03:34 $
 *  Copyright 2009 Institute for Astronomy, University of Hawaii
 */

# include "psastroStandAlone.h"

/**
 * generate model for gpc based on input model for all but chips
 * USAGE: gpcModel (input) (output)
 */
int main (int argc, char **argv) {

    if (argc != 3) {
	fprintf (stderr, "USAGE: gpcModel (input) (output)\n");
	exit (2);
    }

    psFits *input = psFitsOpen (argv[1], "r");
    psFits *output = psFitsOpen (argv[2], "w");

    { // PHU
	psMetadata *header = psFitsReadHeader (NULL, input);
	psFitsWriteBlank (output, header, "");
    }

    { // CHIPS

	// read initial chips
	if (!psFitsMoveExtName (input, "CHIPS")) {
	    psError(PS_ERR_IO, false, "missing CHIPS extension in astrometry table\n");
	    return false;
	}
	psArray *chips = psFitsReadTable (input);
	if (!chips) psAbort("cannot read chips");
	fprintf (stderr, "read %ld rows from CHIPS\n", chips->n);

	psMetadata *header = psMetadataAlloc();
	psMetadataAddStr(header, PS_LIST_TAIL, "COORD",    PS_META_REPLACE, "name of this layer",	"CHIPS");
	psMetadataAddStr(header, PS_LIST_TAIL, "PARENT",   PS_META_REPLACE, "next layer up",     	"FOCAL_PLANE");
	psMetadataAddStr(header, PS_LIST_TAIL, "BOUNDARY", PS_META_REPLACE, "validity region",   	"RECTANGLE");
	psMetadataAddStr(header, PS_LIST_TAIL, "TRANSFRM", PS_META_REPLACE, "mapping to parent", 	"POLYNOMIAL");

	float coeff[2][2];
	char coeffMask[2][2];
	coeff[0][0] = 0.0;
	coeff[1][0] = 1.0;
	coeff[0][1] = 0.0;
	coeff[1][1] = 0.0;

	coeffMask[0][0] = 0;
	coeffMask[1][0] = 0;
	coeffMask[0][1] = 0;
	coeffMask[1][1] = 1;

	psArray *table = psArrayAllocEmpty (1);
	for (int i = 0; i < 8; i++) {
	    for (int j = 0; j < 8; j++) {
		if ((i == 0) && (j == 0)) continue;
		if ((i == 0) && (j == 7)) continue;
		if ((i == 7) && (j == 0)) continue;
		if ((i == 7) && (j == 7)) continue;

		float Xo;
		float Yo;
		if (i < 4) {
		    Xo = (3 - i)*4970.0 +  60.0;
		    Yo = (3 - j)*5133.0 + 125.0;
		    coeff[1][0] = +1.0;
		} else {
		    Xo = (4 - i)*4970.0 -  60.0;
		    Yo = (4 - j)*5133.0 - 150.0;
		    coeff[1][0] = -1.0;
		}	    

		for (int ix = 0; ix < 2; ix++) {
		    for (int iy = 0; iy < 2; iy++) {

			psMetadata *row = psMetadataAlloc ();
		
			char chipname[80];
			sprintf (chipname, "XY%d%d", i, j);
			psMetadataAddStr(row,    PS_LIST_TAIL, "SEGMENT",  PS_META_REPLACE, "name of this segment", chipname);
			psMetadataAddStr(row,    PS_LIST_TAIL, "PARENT",   PS_META_REPLACE, "next layer up",        "FOCAL_PLANE");
			psMetadataAddF32(row,    PS_LIST_TAIL, "MINX",     PS_META_REPLACE, "range", 0.0);
			psMetadataAddF32(row,    PS_LIST_TAIL, "MAXX",     PS_META_REPLACE, "range", 4846.0);
			psMetadataAddF32(row,    PS_LIST_TAIL, "MINY",     PS_META_REPLACE, "range", 0.0);
			psMetadataAddF32(row,    PS_LIST_TAIL, "MAXY",     PS_META_REPLACE, "range", 4868.0);

			psMetadataAddS32(row,    PS_LIST_TAIL, "XORDER",   PS_META_REPLACE, "", ix);
			psMetadataAddS32(row,    PS_LIST_TAIL, "YORDER",   PS_META_REPLACE, "", iy);
			psMetadataAddS32(row,    PS_LIST_TAIL, "NXORDER",  PS_META_REPLACE, "", 1);
			psMetadataAddS32(row,    PS_LIST_TAIL, "NYORDER",  PS_META_REPLACE, "", 1);
			if ((ix == 0) && (iy == 0)) {
			    psMetadataAddF32(row,    PS_LIST_TAIL, "POLY_X",   PS_META_REPLACE, "", Xo);
			    psMetadataAddF32(row,    PS_LIST_TAIL, "POLY_Y",   PS_META_REPLACE, "", Yo);
			} else {
			    psMetadataAddF32(row,    PS_LIST_TAIL, "POLY_X",   PS_META_REPLACE, "", coeff[ix][iy]);
			    psMetadataAddF32(row,    PS_LIST_TAIL, "POLY_Y",   PS_META_REPLACE, "", coeff[iy][ix]);
			}
			psMetadataAddF32(row,    PS_LIST_TAIL, "ERROR_X",  PS_META_REPLACE, "", 0.0);
			psMetadataAddF32(row,    PS_LIST_TAIL, "ERROR_Y",  PS_META_REPLACE, "", 0.0);
			psMetadataAddU8 (row,    PS_LIST_TAIL, "MASK_X",   PS_META_REPLACE, "", coeffMask[ix][iy]);
			psMetadataAddU8 (row,    PS_LIST_TAIL, "MASK_Y",   PS_META_REPLACE, "", coeffMask[ix][iy]);
			psArrayAdd (table, 100, row);
			psFree (row);
		    }
		}
	    }
	}
    
	if (!psFitsWriteTable (output, header, table, "CHIPS")) {
	    psError(PS_ERR_IO, false, "writing sky data\n");
	    psFree(table);
	    return false;
	}
    }

    { // FP
	if (!psFitsMoveExtName (input, "FP")) {
	    psError(PS_ERR_IO, false, "missing FP extension in astrometry table\n");
	    return false;
	}
	psMetadata *header = psFitsReadHeader (NULL, input);
	psArray *table = psFitsReadTable (input);
	if (!table) psAbort("cannot read fp");
	fprintf (stderr, "read %ld rows from FP\n", table->n);

	if (!psFitsWriteTable (output, header, table, "FP")) {
	    psError(PS_ERR_IO, false, "writing sky data\n");
	    psFree(table);
	    return false;
	}
    }

    { // TP
	if (!psFitsMoveExtName (input, "TP")) {
	    psError(PS_ERR_IO, false, "missing TP extension in astrometry table\n");
	    return false;
	}
	psMetadata *header = psFitsReadHeader (NULL, input);
	psArray *table = psFitsReadTable (input);
	if (!table) psAbort("cannot read tp");
	fprintf (stderr, "read %ld rows from TP\n", table->n);
	if (!psFitsWriteTable (output, header, table, "TP")) {
	    psError(PS_ERR_IO, false, "writing sky data\n");
	    psFree(table);
	    return false;
	}
    }

    { // SKY
	if (!psFitsMoveExtName (input, "SKY")) {
	    psError(PS_ERR_IO, false, "missing SKY extension in astrometry table\n");
	    return false;
	}
	psMetadata *header = psFitsReadHeader (NULL, input);
	psArray *sky = psFitsReadTable (input);
	if (!sky) psAbort("cannot read sky");
	fprintf (stderr, "read %ld rows from SKY\n", sky->n);
	if (!psFitsWriteTable (output, header, sky, "SKY")) {
	    psError(PS_ERR_IO, false, "writing sky data\n");
	    psFree(sky);
	    return false;
	}
    }

    psFitsClose (input);
    psFitsClose (output);
    exit (0);
}
