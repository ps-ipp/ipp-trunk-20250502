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
 * FITS Structure : 0.000 to 9.999
 *
 * The following keywords give miscellaneous information about
 * the instrument in general that doesn't really fit anywhere 
 * else.
 * 
 * Instrument : 1000.000 to 1999.999
 *
 *TYPE --IDX--  KEYWORD- PRC -------------------COMMENT----------------------*/
MK_CMT( 1000.00,CMTINST1,   ""                                                )
MK_CMT( 1000.10,CMTINST2,   "Instrument Information"                          )
MK_CMT( 1000.20,CMTINST3,   "----------------------"                          )
MK_CMT( 1000.40,CMTINST4,   ""                                                )
MK_PFL( 1009.0, PRESSURE,6, "[Torr] Micropirani pressure reading"             )
MK_PFL( 1009.10,L3HUMID, 1, "[%] Relative humidity in space above L3"         )
MK_PFL( 1009.11,L3AIRTMP,1, "[C] Air temperature in space above L3"           )
MK_PFL( 1009.12,L3DEWPT, 1, "[C] Calculated dew point in space above L3"      )
MK_PFL( 1009.20,ENHUMID, 1, "[%] Rel. humidity of cam. evironment (ferit)"    )
MK_PFL( 1009.21,ENAIRTMP,1, "[C] Air temperature of camera environment"       )
MK_PFL( 1009.22,ENDEWPT, 1, "[C] Calculated dew point of camera environment"  )
MK_STR( 1009.30,GLYSTAT,    "Camera heat exchanger glycol status"             )
MK_STR( 1009.31,GLYMODE,    "Camera heat exchanger glycol mode"               )
MK_STR( 1010.0, FILTSTAT,   "Filter mechanism status"                         )
MK_STR( 1010.1, FILTERID,   "Filter glass identification"                     )
MK_STR( 1010.2, FILTER  ,   "Filter band name OPEN,u,g,r,i,z,y,w"             )
MK_STR( 1011.0, VIDMODE,    "Video mode used (w pix x h pix x rate Hz)"       )
MK_PFL( 1110.1, DEWTEM1, 1, "[K] Dewar rtd 1 (CTI-Left head)"                 )
MK_PFL( 1110.2, DEWTEM2, 1, "[K] Dewar rtd 2 (Copper plate)"                  )
MK_PFL( 1110.3, DEWTEM3, 1, "[K] Dewar rtd 3 (OTA67 package)"                 )
MK_PFL( 1110.4, DEWTEM4, 1, "[K] Dewar rtd 4 (Triangle post)"                 )
MK_PFL( 1110.5, DEWTEM5, 1, "[K] Dewar rtd 5 (Copper bar)"                    )
MK_PFL( 1110.6, DEWTEM6, 1, "[K] Dewar rtd 6 (CTI-Right head)"                )
MK_PFL( 1210.1, SH_TEM,  1, "[K] Shack Hartmann head temperature"             )
MK_INT( 1220.1, SH_POS    , "[steps] Shack Hartmann arm position"             )
MK_INT( 1221.0, SH_STDLY  , "[2.04us/step] S-H arm step delay"                )
MK_INT( 1222.0, SH_ACCEL  , "[2.04us/step^2] S-H acceleration param"          )
MK_INT( 1223.0, SH_MNDLY  , "[2.04us/step] S-H minimum step delay"            )
MK_INT( 1224.0, SH_ARMPN  , "motion controller input pins"                    )
MK_PFL( 1225.0, SH_DIODE,1, "Vref/4096 (Vref=5V nominal) photodiode volts"    )
MK_STR( 1260.1, CS_LIGHT,   "Cal screen laser status during exposure"         )
MK_STR( 1261.0, CS_LIMIT,   "Cal screen laser exposure limit: time|volt"      )
MK_PFL( 1262.0, CS_WAVE,1,  "Cal screen laser wavelength"                     )
MK_PFL( 1263.0, CS_EXPT,1,  "Cal screen laser exposure time"                  )
MK_PFL( 1264.0, CS_VOLTS,3, "Cal screen laser photodiode limit voltage"       )
MK_PFL( 1265.1, CS_INTV1,3, "Cal screen photodiode integrator pre-voltage"    )
MK_PFL( 1265.2, CS_INTV2,3, "Cal screen photodiode integrator start-voltage"  )
MK_PFL( 1265.3, CS_INTV3,3, "Cal screen photodiode integrator stop-voltage"   )
MK_PFL( 1265.4, CS_INTV4,3, "Cal screen photodiode integrator post-voltage"   )
MK_PFL( 1266.1, CS_INTT1,3, "Cal screen photodiode integrator pre-seconds"    )
MK_PFL( 1266.2, CS_INTT2,3, "Cal screen photodiode integrator exposure sec"   )
MK_PFL( 1266.3, CS_INTT3,3, "Cal screen photodiode integrator post-seconds"   )
MK_PFL( 1267.0, CS_SHOP,3,  "GPC1 shutter open start"                         )
MK_PFL( 1268.0, CS_SHCL,3,  "GPC1 shutter close start"                        )

