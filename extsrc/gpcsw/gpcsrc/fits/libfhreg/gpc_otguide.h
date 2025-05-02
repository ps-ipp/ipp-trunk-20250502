/*                                             -*- c-file-style: "Ellemtel" -*-

`gpc_otguide.h' - Pan-STARRS GPC OT correction and Guider keywords

This file is part of version 0 of the FITS Keyword Registry.
Read the `License' file for terms of use and distribution.
Copyright 2004, Pan-STARRS, isani@ifa.hawaii.edu.

___This header automatically generated from `Index'. Do not edit it here!___ */

#include "macros.h"

/*
 * Description of columns:
 *
 *      TYPE - MK_BLN, MK_STR, MK_INT, MK_FLT, MK_PFL selects a type for the
 *             keyword (True/False, 'String  ', integer, or floating point.)
 *             Any keyword can still be set to any type with fh_setval_*(),
 *             which takes a pre-formatted character string for the card.
 *
 *      IDX  - Index sorting number, which eventually determines the final
 *             order of keywords in the FITS header.  These are floating point
 *             values so new keywords can always be inserted in between
 *             without shifting all the values.
 *
 *   KEYWORD - The name of the keyword.  Macros will be generated of the form
 *             fh_get_KEYWORD(), fh_set_KEYWORD(), and fh_setval_KEYWORD().
 *
 *      PRC  - For MK_FLT(), this specifies the (scientific) precision,
 *             or number of significant digits in the value.  If necessary,
 *             scientific notation will be used to format the value (see
 *             rules for printf "%.*G").
 *             For MK_PFL(), this specifies mathematical precision, or number
 *             of decimal places in the formatted value (see the rules for
 *             printf "%.*f").
 *
 *             Use MK_PFL() (make "precise float") if that value is always
 *             precise to an exact number digits after the decimal point.
 *             Use MK_FLT() if the value might require scientific notation.
 *             All results are FITS-standard legal.
 *
 *   COMMENT - String to be included after the / in the FITS card, if
 *             there is room within the 80 columns (it may get truncated.)
 *             Note that in the text below, 80 columns are reached when
 *             there is still one space between the " and the ) at the end
 *             of the line.
 *
 * FITS Structure : 0.000 to 9.999
 *
 * The following keywords give basic information about the structure and
 * dimensions of the FITS file, and must appear in the specific order
 * below, usually without any other intervening keywords.
 *
 * Guider : ???
 *
 *TYPE --IDX--  KEYWORD- PRC -------------------COMMENT----------------------*/
MK_CMT(  250.00,CMTOTGD1 ,   ""                                               )
MK_CMT(  250.01,CMTOTGD2 ,   "Guide Star Info"                                )
MK_CMT(  250.02,CMTOTGD3 ,   "---------------"                                )
MK_STR(  250.10,GS_ID,      "Guide star catalogue ID/name"                    )
MK_PFL(  250.20,GS_PRRA,6,  "[degrees] Guide star predicted RA"               )
MK_PFL(  250.30,GS_PRDEC,6, "[degrees] Guide star predicted DEC"              )
MK_PFL(  250.40,GS_PRFP1,2, "[microns] Guide star pred. focal plane pos"      )
MK_PFL(  250.50,GS_PRFP2,2, "[microns] Guide star pred. focal plane pos"      )
MK_INT(  250.60,GS_PREDX,   "[unbinned pix] Guide star predicted X position"  )
MK_INT(  250.70,GS_PREDY,   "[unbinned pix] Guide star predicted Y position"  )
MK_PFL(  250.80,GS_PRMAG,2, "Guide star predicted magnitude with curr filter" )
MK_STR(  251.07,GS_PSF,     "Video PSF algorithm used for centroid"           )
MK_PFL( 251.081,GS_CORX, 6, "[pix] GS calc correction applied in camera X"    )
MK_PFL( 251.082,GS_CORY, 6, "[pix] GS calc correction applied in camera Y"    )
MK_PFL( 251.083,GS_CORPA,6, "[deg] GS calc correction applied to rotation"    )
MK_PFL(  251.10,GS_CPIX1,2, "Guide star centroid from origin of CNAXIS system")
MK_PFL(  251.20,GS_CPIX2,2, "Guide star centroid from origin of CNAXIS system")
MK_PFL(  252.00,GS_FWHM, 2, "[pix] Guide star full width at half maximum"     )
MK_PFL(  252.10,GS_FWHM1,2, "[pix] Guide star full width at half maximum in x")
MK_PFL(  252.20,GS_FWHM2,2, "[pix] Guide star full width at half maximum in y")
MK_PFL(  253.00,GS_PSKY, 2, "[adu] Sky level in guide box"                    )
MK_PFL(  254.00,GS_PFLUX,2, "[adu] Total flux in star"                        )
MK_PFL(  255.00,GS_SN   ,2, "Guide star signal to noise ratio"                )
MK_PFL(  259.10,GS_AVFW ,2, "[pix] Average FWHM to present"                   )
MK_PFL(  259.20,GS_AVX  ,2, "[microns] Average focal plane X pos to present"  )
MK_PFL(  259.30,GS_AVY  ,2, "[microns] Average focal plane Y pos to present"  )
