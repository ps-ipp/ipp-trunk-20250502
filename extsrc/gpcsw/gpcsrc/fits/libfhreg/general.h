/*                                             -*- c-file-style: "Ellemtel" -*-

`general.h' - General FITS structure keyword registry entries

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
 *TYPE --IDX--  KEYWORD- PRC -------------------COMMENT----------------------*/
MK_BLN( 0.0,	SIMPLE  ,   "Standard FITS"                                   )
MK_STR( 0.0,	XTENSION,   ""                                                )
MK_INT( 1.0,	BITPIX  ,   "Bits per pixel"                                  )
MK_INT( 2.0,	NAXIS   ,   "Number of axes"                                  )
MK_INT( 2.1,	NAXIS1  ,   "Number of pixel columns"                         )
MK_INT( 2.2,	NAXIS2  ,   "Number of pixel rows"                            )
MK_INT( 2.3,	NAXIS3  ,   "Number of stacked frames (cube)"                 )
MK_BLN( 3.0,	EXTEND  ,   "File contains extensions"                        )
MK_INT( 3.1,	NEXTEND ,   "Number of extensions"                            )
MK_BLN( 4.0,	GROUPS  ,   "File contains random groups records"             )
MK_INT( 5.0,	PCOUNT  ,   "Random parameters before each array in a group"  )
MK_INT( 6.0,	GCOUNT  ,   "Number of random groups"                         )
MK_INT( 7.0000,	TFIELDS ,   "Number of fields in a row"                       )
MK_STR( 7.0011,	TFORM1  ,   "Table format for field 1"                        )
MK_INT( 7.0012,	TBCOL1  ,   "Start Column for field 1"                        )
MK_STR( 7.0021,	TFORM2  ,   "Table format for field 2"                        )
MK_INT( 7.0022,	TBCOL2  ,   "Start Column for field 2"                        )
MK_STR( 7.0031,	TFORM3  ,   "Table format for field 3"                        )
MK_INT( 7.0032,	TBCOL3  ,   "Start Column for field 3"                        )
MK_STR( 7.0041,	TFORM4  ,   "Table format for field 4"                        )
MK_INT( 7.0042,	TBCOL4  ,   "Start Column for field 4"                        )
MK_STR( 7.0051,	TFORM5  ,   "Table format for field 5"                        )
MK_INT( 7.0052,	TBCOL5  ,   "Start Column for field 5"                        )
MK_BLN( 9.0,	INHERIT	,   "Inherit global keywords from primary header"     )
MK_PFL( 10.0,   BZERO   ,1, "Zero factor"                                     )
MK_PFL( 11.0,	BSCALE  ,1, "Scale factor"                                    )
MK_STR( 12.0,	BUNIT   ,   "fits-data * BSCALE + BZERO = these units"        )

/*
 * Summary : 50.000 to 59.999
 *
 *TYPE --IDX--  KEYWORD- PRC -------------------COMMENT----------------------*/
MK_CMT(   50.1, CMTSUM1 ,   ""                                                )
MK_CMT(   50.2, CMTSUM2 ,   "Observation Summary"                             )
MK_CMT(   50.3, CMTSUM3 ,   "-------------------"                             )
MK_CMT(   50.4, CMTSUM4 ,   ""                                                )
MK_CMT(   50.5, CMTSUM5 ,   ""                                                )
MK_STR(   51.0, CMMTOBS ,   ""                                                )
MK_STR(   51.1, CMMTOBS1,   ""                                                )
MK_STR(   51.2, CMMTOBS2,   ""                                                )
MK_STR(   51.3, CMMTOBS3,   ""                                                )
MK_STR(   51.4, CMMTOBS4,   ""                                                )
MK_STR(   51.5, CMMTOBS5,   ""                                                )
MK_STR(   51.6, CMMTOBS6,   ""                                                )
MK_STR(   51.7, CMMTOBS7,   ""                                                )
MK_STR(   51.8, CMMTOBS8,   ""                                                )
MK_STR(   51.9, CMMTOBS9,   ""                                                )
MK_STR(   52.0, CMMTSEQ ,   ""                                                )
MK_STR(   53.0, OBJECT  ,   ""                                                )
MK_STR(   54.0, OBSERVER,   ""                                                )
MK_STR(   55.0, PI_NAME ,   ""                                                )
MK_STR(   56.0, RUNID   ,   ""                                                )
MK_STR(   57.0, QUEUEID ,   ""                                                )
MK_STR(   58.0, OBS_MODE,   ""                                                )
MK_INT(   59.0, TESSEL  ,   ""                                                )

/*
 * General : 70.000 to 79.999
 *
 *TYPE --IDX--  KEYWORD- PRC -------------------COMMENT----------------------*/
MK_CMT(  70.10, CMTGEN1 ,   ""                                                )
MK_CMT(  70.20, CMTGEN2 ,   "General"                                         )
MK_CMT(  70.30, CMTGEN3 ,   "-------"                                         )
MK_CMT(  70.40, CMTGEN4 ,   ""                                                )
MK_STR(  71.00, FILENAME,   "Base filename at acquisition"                    )
MK_STR(  71.01, PATHNAME,   "Original directory name at acquisition"          )
MK_STR(  71.10, EXTNAME ,   "Extension name"                                  )
MK_INT(  71.20, EXTVER  ,   "Extension version"                               )
MK_INT(  71.21, IMAGEID ,   ""                                                )
MK_STR(  74.00, DATE    ,   "UTC Date of file creation"                       )
MK_STR(  74.10, HSTTIME ,   "Local time in Hawaii"                            )
MK_STR(  76.99, IMGSWPRG,   "Image creation software program"                 )
MK_STR(  77.00, IMAGESWV,   "Image creation software version"                 )
MK_STR(  79.00, ORIGIN  ,   ""                                                )

/*
 * Summary : 90.0 Key stuff from instrument, detector (and other?) sections
 */
MK_STR(   91.0, INSTRUME,   "Instrument Name"                                 )
MK_STR(   91.1, INSTMODE,   "Instrument Mode"                                 )
MK_STR(   92.0,	DETECTOR,   "Science Detector"                                )
MK_STR(   92.1, FPPOS,      "Position of detector in focal plane mosaic"      )

MK_STR(   95.0, OBSTYPE,    "Observation/Exposure type"                       )
MK_PFL(   96.0, EXPTIME,3,  "[sec] Exposure time"                             )
MK_PFL(   96.1, EXPREQ ,3,  "[sec] Exposure time requested"                   )
MK_PFL(   97.0, DARKTIME,3, "[sec] Dark current time"                         )
MK_INT(   97.5,  PONTIME,   "[sec] Time since last detector \"power on\""     )
MK_INT(   97.6,  SATTIME,   "[sec] Time since last detected saturation event" )
MK_INT(   97.7,  TRKTIME,   "[sec] Time since last detected tracking error"   )
MK_INT(   97.8,  VDOTIME,   "[sec] Time since \"video\" command used"         )
