/*                                             -*- c-file-style: "Ellemtel" -*-

`gpc_instrument.h' - Pan-STARRS instrument FITS header keywords

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
 * WCS: 600.0 to 609.99
 *
 * The following keywords give information about the various WCS coordinate
 * systems available for the FITS file.
 *
 *TYPE --IDX--  KEYWORD- PRC -------------------COMMENT----------------------*/
MK_CMT( 600.0,  CMTWCS1 ,   ""                                                )
MK_CMT( 600.1,  CMTWCS2 ,   "WCS Coordinate Systems"                          )
MK_CMT( 600.2,  CMTWCS3 ,   "----------------------"                          )

MK_STR( 601.10, CTYPE1  ,   "R.A. in tangent plane projection"                )
MK_STR( 601.11, CUNIT1  ,   "Units for focal plane coord system axis 1"       )
MK_PFL( 601.12, CRVAL1  ,6, "R.A. at ref. pixel"                              )
MK_PFL( 601.13, CRPIX1  ,1, "Ref. pixel of first axis"                        )
MK_PFL( 601.14, CDELT1  ,10,"R.A. pixel step"                                 )
MK_STR( 601.15, CTYPE2  ,   "Dec. in tangent plane projection"                )
MK_STR( 601.16, CUNIT2  ,   "Units for focal plane coord system axis 1"       )
MK_PFL( 601.17, CRVAL2  ,6, "Dec. at ref. pixel"                              )
MK_PFL( 601.18, CRPIX2  ,1, "Ref. pixel of second axis"                       )
MK_PFL( 601.19, CDELT2  ,10,"Dec. pixel step"                                 )
MK_PFL( 601.20, CD1_1   ,10,"CD transform matrix"                             )
MK_PFL( 601.21, CD1_2   ,10,"CD transform matrix"                             )
MK_PFL( 601.22, CD2_1   ,10,"CD transform matrix"                             )
MK_PFL( 601.23, CD2_2   ,10,"CD transform matrix"                             )

MK_STR( 602.00, WCSNAMEA,   "Redundant reiteration of default WCS system"     )
MK_STR( 602.10, CTYPE1A ,   "R.A. in tangent plane projection"                )
MK_STR( 602.11, CUNIT1A ,   "Units for focal plane coord system axis 1"       )
MK_PFL( 602.12, CRVAL1A ,6, "R.A. at ref. pixel"                              )
MK_PFL( 602.13, CRPIX1A ,1, "Ref. pixel of first axis"                        )
MK_PFL( 602.14, CDELT1A ,10,"R.A. pixel step"                                 )
MK_STR( 602.15, CTYPE2A ,   "Dec. in tangent plane projection"                )
MK_STR( 602.16, CUNIT2A ,   "Units for focal plane coord system axis 1"       )
MK_PFL( 602.17, CRVAL2A ,6, "Dec. at ref. pixel"                              )
MK_PFL( 602.18, CRPIX2A ,1, "Ref. pixel of second axis"                       )
MK_PFL( 602.19, CDELT2A ,10,"Dec. pixel step"                                 )
MK_PFL( 602.20, CD1_1A  ,10,"CD transform matrix"                             )
MK_PFL( 602.21, CD1_2A  ,10,"CD transform matrix"                             )
MK_PFL( 602.22, CD2_1A  ,10,"CD transform matrix"                             )
MK_PFL( 602.23, CD2_2A  ,10,"CD transform matrix"                             )

MK_STR( 603.00, WCSNAMEU,   "Position on instrument focal plane"              )
MK_STR( 603.10, CTYPE1U ,   "Linear instrument focal plane position"          )
MK_STR( 603.11, CUNIT1U ,   "Units for focal plane coord system axis 1"       )
MK_PFL( 603.12, CRVAL1U ,6, "Focal plane position at reference pixel"         )
MK_PFL( 603.13, CRPIX1U ,1, "Ref. pixel of first axis"                        )
MK_PFL( 603.14, CDELT1U ,10,"Pixel step"                                      )
MK_STR( 603.15, CTYPE2U ,   "Linear instrument focal plane position"          )
MK_STR( 603.16, CUNIT2U ,   "Units for focal plane coord system axis 2"       )
MK_PFL( 603.17, CRVAL2U ,6, "Focal plane position at reference pixel"         )
MK_PFL( 603.18, CRPIX2U ,1, "Ref. pixel of second axis"                       )
MK_PFL( 603.19, CDELT2U ,10,"Pixel step"                                      )
MK_PFL( 603.20, CD1_1U  ,10,"CD transform matrix"                             )
MK_PFL( 603.21, CD1_2U  ,10,"CD transform matrix"                             )
MK_PFL( 603.22, CD2_1U  ,10,"CD transform matrix"                             )
MK_PFL( 603.23, CD2_2U  ,10,"CD transform matrix"                             )
