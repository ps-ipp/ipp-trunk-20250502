# include "addstar.h"

// this function evaluates the collection of image headers for keywords of the form ZPT_OBS.
// Downstream, we will set the zero point offset for the image (image[0].Mcal) based on the
// value calculated here and the nominal zero point for the chip photcode. Four possible modes
// are defined: 

// NOMINAL: in this case, the database value for the nominal zero point is used and
// image[0].Mcal is set to zero.

// CHIP_HEADER: in this case, the zero point offset measured for each chip is applied
// independently to image[0].Mcal

// CHIP_AVERAGE: in this case, the per-chip measured zero points are converted to offsets from
// the nominal zero points, and the median zero point offset is calculate, and later applied to
// image[0].Mcal

// PHU_HEADER: in this case, the zero point measured for the entire exposure and reported in
// the PHU header is used to set the zero point offset for each chip.  
// Here set ZPT_OBS_PHU and ZPT_ERR_PHU to the observed values from the header.
// ZERO_POINT_OFFSET is calculated in ReadImageHeader based on the per-chip zero points in the
// photcode database.

int GetZeroPointExposure (Header **headers, HeaderSet *headerSets, off_t Nimages) {

    // the zero point correction is not applied
    if (!strcasecmp(ZERO_POINT_OPTION, "NOMINAL")) {
	ZERO_POINT_OFFSET = 0.0;
	ZERO_POINT_ERROR = NAN;
	return (TRUE);
    }

    // the zero point correction is applied in ReadImageHeader
    if (!strcasecmp(ZERO_POINT_OPTION, "CHIP_HEADER")) {
	ZERO_POINT_OFFSET = 0.0;
	ZERO_POINT_ERROR = NAN;
	return (TRUE);
    }

    // the zero point correction is measured here and applied in ReadImageHeader
    if (!strcasecmp(ZERO_POINT_OPTION, "CHIP_AVERAGE")) {

	int i, Nzpt, Nmid, Nhead;
	float *zpt, ZPT_OBS, ZPT_ERR;
	PhotCode *photcode;
	char photname[80];

	Nzpt = 0;
	ALLOCATE (zpt, float, Nimages);

	for (i = 0; i < Nimages; i++) {
	    if (!strcmp(headerSets[i].exthead, "PHU")) continue;
	    Nhead = headerSets[i].extnum_head;

	    if (!gfits_scan (headers[Nhead], ZERO_POINT_KEYWORD, "%f", 1, &ZPT_OBS)) {
		fprintf (stderr, "zero point not supplied in header\n");
		continue;
	    }
	    
	    if (!gfits_scan (headers[Nhead], "ZPT_ERR", "%f", 1, &ZPT_ERR)) {
//		XXX should we emit this message? We currently aren't using this value so no
//		fprintf (stderr, "zero point not supplied in header\n");
//		continue;
	        ZPT_ERR = NAN;
	    }
	    
	    /* get photcode from header */
	    if (!gfits_scan (headers[Nhead], "PHOTCODE", "%s", 1, photname)) {
		fprintf (stderr, "photcode not supplied in header\n");
		continue;
	    }
	    photcode = GetPhotcodebyName (photname);
	    if (photcode == NULL) {
		fprintf (stderr, "photcode %s not found in photcode table\n", photname);
		continue;
	    }
	    // photcode table stores zero point in milli-mags :
	    // Mrel = measure[0].M - ZERO_POINT + code[0].K*(measure[0].airmass - 1.000) + SCALE*code[0].C - measure[0].Mcal;
	    // ZERO_POINT above is the standard 25.0
	    // measure[0].M = Mobs + 2.5log(exptime)
	    // Mrel should be equivalent to Mref.  thus Mref - Mobs - 2.5log(exptime) = -Mcal

	    zpt[Nzpt] = 0.001*photcode[0].C - ZPT_OBS;
	    Nzpt ++;
	}
	    
	if (Nzpt == 0) {
	    fprintf (stderr, "WARNING: zero point is not measured, no valid entries in headers\n");
	    ZERO_POINT_OFFSET = 0.0;
	    ZERO_POINT_ERROR = NAN;
	    free (zpt);
	    return (FALSE);
	} 

	fsort (zpt, Nzpt);
	Nmid = Nzpt / 2;  // Nzpt = 1,2,3,4 -> Nmid = 0,1,1,2  2: (0 1), 4: 0 (1 2) 3 
	ZERO_POINT_OFFSET = (Nzpt % 2) ? zpt[Nmid] : 0.5*(zpt[Nmid] + zpt[Nmid-1]);
	free (zpt);
        // XXX: TODO calculate someting for ZERO_POINT_ERROR
	ZERO_POINT_ERROR = NAN;
	return (TRUE);
    }
    
    // the zero point (NOT the correction) is measured here and applied in ReadImageHeader
    if (!strcasecmp(ZERO_POINT_OPTION, "PHU_HEADER")) {
	int i, Nhead;

	ZERO_POINT_OFFSET = 0.0;
	ZERO_POINT_ERROR = NAN;
	for (i = 0; i < Nimages; i++) {
	    if (strcmp(headerSets[i].exthead, "PHU")) continue;
	    Nhead = headerSets[i].extnum_head;
	    
            float ZPT_OBS, ZPT_ERR;
	    if (!gfits_scan (headers[Nhead], ZERO_POINT_KEYWORD, "%f", 1, &ZPT_OBS)) {
		fprintf (stderr, "WARNING: zero point is not measured, no valid entries in headers\n");
		return (FALSE);
	    }
	    if (!gfits_scan (headers[Nhead], "ZPT_ERR", "%f", 1, &ZPT_ERR)) {
		fprintf (stderr, "WARNING: zero point error is not measured\n");
//		XXX: Do we want to require ZPT_ERR? for now just set it to zero and proceed.
//		return (FALSE);
		ZPT_ERR = NAN;
	    }
            ZPT_OBS_PHU = ZPT_OBS;
            ZPT_ERR_PHU = ZPT_ERR;
	    return (TRUE);
	}
	fprintf (stderr, "WARNING: zero point is not measured, no valid entries in headers\n");
	return (FALSE);
    }

    if (!strcasecmp(ZERO_POINT_OPTION, "MATCHED_REFS")) {
	fprintf (stderr, "ERROR: MATCHED_REFS zero point analysis is not implemented\n");
	exit (1);
    }

    fprintf (stderr, "ERROR: invalid value for ZERO_POINT_OPTION: %s\n", ZERO_POINT_OPTION);
    exit (1);
}
