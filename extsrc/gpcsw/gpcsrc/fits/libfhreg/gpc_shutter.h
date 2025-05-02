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
 * Shutter: 200.000 to 209.999
 *
 * The following keywords give information about the shutter system
 * for a given exposure.
 *
 *TYPE --IDX--  KEYWORD- PRC -------------------COMMENT----------------------*/
MK_CMT(  200.0, CMTSHU1 ,   ""                                                )
MK_CMT(  200.1, CMTSHU2 ,   "Shutter"                                         )
MK_CMT(  200.2, CMTSHU3 ,   "-------"                                         )
MK_STR(  201.0,	SHUTSTAT,   "Shutter status"                                  )
MK_STR(  201.1,	SHUTERR,    "Shutter error code"                              )
MK_STR(  202.00,SHUTOPEN,   "[TAI date time] Shutter open time"               )
MK_STR(  202.01,SHUTCLOS,   "[TAI date time] Shutter close time"              )
MK_PFL(  202.02,SHUTOPWN,6, "[sec] Shutter open uncertainty before SHUTOPEN"  )
MK_PFL(  202.03,SHUTCLWN,6, "[sec] Shutter close uncertainty before SHUTCLOS" )
MK_STR(  202.04,SHUTOUTC,   "[UTC date time] Shutter open time"               )
MK_STR(  202.05,SHUTCUTC,   "[UTC date time] Shutter close time"              )
MK_INT(  202.06,SHUTLEAP,   "[sec] Leap seconds used in TAI-UTC conversion"   )
MK_PFL(  202.07,SHUTREQ, 6, "[sec] Requested exposure duration"               )
MK_PFL(  202.08,SHUTTIME,6, "[sec] Actual exposure duration"                  )
MK_STR(  202.09,SHUTOPBL,   "Shutter blade in aperture at start of exposure"  )
MK_STR(  202.10,SHUTCLBL,   "Shutter blade in aperture at end of exposure"    )
MK_STR(  209.0, SHUTDAVR,   "Shutter daemon version"                          )
MK_STR(  209.1, SHUTDRVR,   "Shutter driver version"                          )
