/*                                             -*- c-file-style: "Ellemtel" -*-

`gpc_telescope.h' - Pan-STARRS TCS FITS header keywords

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
 *  * Telescope : 500.000 to 599.999
 *
 * Almost all keywords we define can be generated with the MK_XXX macros.
 * The only exceptions are those keywords which contain a '-', since the
 * '-' is not a legal character in a C identifier.  TCS uses four of them
 * (DATE-OBS, UTC-OBS, MJD-OBS, and LST-OBS.)  The ID_XXX macros are used
 * and the '-' is translated to a '_' (underscore) for use in C programs.
 *
 *TYPE --IDX--  KEYWORD- PRC -------------------COMMENT----------------------*/
MK_CMT(  500.00,CMTPNT1 ,   ""                                                )
MK_CMT(  500.10,CMTPNT2 ,   "Telescope and Pointing Information"              )
MK_CMT(  500.20,CMTPNT3 ,   "----------------------------------"              )
MK_CMT(  500.40,CMTPNT4 ,   ""                                                )
MK_STR(  501.00,TELESCOP,   "Telescope"                                       )
ID_PFL( 513.1,  MJD_OBS ,"MJD-OBS",7, "Modified Julian Date at start of obs." )
ID_STR( 514.1,  LST_OBS ,"LST-OBS", "Local Sidereal time at start of obs."    )
MK_STR(  503.0, TELSTAT ,   "Telescope control system status"                 )
MK_STR(  503.1, DOMSTAT ,   "Telescope enclosure status"                      )
MK_VAL(  521.1, EQUINOX ,   "Telescope equinox of coordinates"                )
MK_VAL(  521.2, EPOCH   ,   "Telescope equinox of coordinates"                )
MK_PFL(  522.1, RA      ,6, "[deg] Telescope FOV center Right Ascension"      )
MK_PFL(  522.2, DEC     ,6, "[deg] Telescope FOV center Declination"          )
MK_STR(  522.21,RASTRNG ,   "[deg:mm:ss] Telescope sexagesimal Right Ascen."  )
MK_STR(  522.22,DECSTRNG,   "[deg:mm:ss] Telescope sexagesimal Declination"   )
MK_PFL(  522.3, AZ      ,6, "[deg] Telescope azimuth"                         )
MK_PFL(  522.4, ALT     ,6, "[deg] Telescope pointing altitude"               )
MK_PFL(  522.5, ROT     ,6, "[deg] Telescope rotator angle"                   )
MK_PFL(  522.6, POSANGLE,6, "[deg] Telescope position angle"                  )
MK_PFL(  523.0, COMRA   ,6, "[deg] Commanded telescope Right Ascension"       )
MK_PFL(  523.1, COMDEC  ,6, "[deg] Commanded telescope Declination"           )
MK_PFL(  523.2, COMAZ   ,6, "[deg] Commanded telescope azimuth"               )
MK_PFL(  523.3, COMALT  ,6, "[deg] Commanded telescope pointing altitude"     )
MK_PFL(  523.4, COMROT  ,6, "[deg] Commanded telescope rotator angle"         )
MK_PFL(  523.5, MOONANG ,6, "[deg] Angular distance to moon"                  )
MK_PFL(  523.6, AIRMASS ,3, "Airmass at start of observation"                 )
MK_CMT(  530.00,CMTTELO1,   "NOTE: Telescope RA DEC or ALT AZ already include")
MK_CMT(  530.10,CMTTELO2,   "      the following offsets.  Do not re-apply!"  )
MK_PFL(  532.1, TELOFRA ,6, "[deg] Telescope offset in RA"                    )
MK_PFL(  532.2, TELOFDEC,6, "[deg] Telescope offset in DEC"                   )
MK_PFL(  532.3, TELOFAZ ,6, "[deg] Telescope offset in AZ"                    )
MK_PFL(  532.4, TELOFALT,6, "[deg] Telescope offset in ALT"                   )
MK_PFL(  532.5, TELOFROT,6, "[deg] Telescope offset in ROT"                   )
MK_PFL(  532.7, TELOFX  ,6, "[deg] Telescope offset in camera X"              )
MK_PFL(  532.8, TELOFY  ,6, "[deg] Telescope offset in camera Y"              )

/*
 * Guiding
 */
MK_STR(  540.00,GUISTAT ,   "Telescope guiding system status"                 )
MK_STR(  540.01,GUICONF ,   "Telescope guiding configuration"                 )
MK_STR(  540.02,GUISTAR ,   "Telescope guiding (single or multi-star)"        )
MK_BLN(  540.03,GUIROTEN,   "Telescope guiding rotation enabled"              )
MK_PFL(  540.10,GUIGAIX ,2, "Telescope guiding gain factor (x offset)"        )
MK_PFL(  540.11,GUIGAIY ,2, "Telescope guiding gain factor (y offset)"        )
MK_PFL(  540.12,GUIRATE ,1, "[Hz] Telescope guiding rate"                     )
MK_STR(  540.20,GUIKERN ,   "Filter kernel used for guiding"                  )
MK_STR(  540.21,GUIKERN1,   "Filter kernel used for guiding (cont)"           )
MK_STR(  540.22,GUIKERN2,   "Filter kernel used for guiding (cont)"           )
MK_STR(  540.23,GUIKERN3,   "Filter kernel used for guiding (cont)"           )
MK_STR(  540.24,GUIKERN4,   "Filter kernel used for guiding (cont)"           )
MK_STR(  540.25,GUIKERN5,   "Filter kernel used for guiding (cont)"           )
MK_STR(  540.26,GUIKERN6,   "Filter kernel used for guiding (cont)"           )
MK_STR(  540.27,GUIKERN7,   "Filter kernel used for guiding (cont)"           )
MK_STR(  540.28,GUIKERN8,   "Filter kernel used for guiding (cont)"           )
MK_STR(  540.29,GUIKERN9,   "Filter kernel used for guiding (cont)"           )
MK_INT(  540.30,GUIKLEN ,   "Number of taps in filter kernel"                 )
MK_INT(  540.31,GUIKSUM ,   "Sum of all filter kernel taps"                   )
MK_INT(  540.40,GUINSTAR,   "Number of guide stars sought"                    )
MK_INT(  540.41,GUINOK,     "Number of guide stars used (median)"             )
MK_INT(  540.42,GUIERR,     "90 percentile of guide fit error codes"          )
MK_PFL(  540.43,GUIDR,   2, "[arcsec] median RMS of guide star offsets"       )
MK_PFL(  540.50,GUIXMED, 2, "[arcsec] median x offset of guide stars"         )
MK_PFL(  540.51,GUIX50,  2, "[arcsec] 50% range of guide star x"              )
MK_PFL(  540.52,GUIX90,  2, "[arcsec] 90% range of guide star x"              )
MK_PFL(  540.60,GUIYMED, 2, "[arcsec] median y offset of guide stars"         )
MK_PFL(  540.61,GUIY50,  2, "[arcsec] 50% range of guide star y"              )
MK_PFL(  540.62,GUIY90,  2, "[arcsec] 90% range of guide star y"              )
MK_PFL(  540.70,GUIROT,  4, "[deg] median rotation offset of guide stars"     )
MK_PFL(  540.71,GUIROT50,4, "[deg] 50% range of rotation offset"              )
MK_PFL(  540.72,GUIROT90,4, "[deg] 90% range of rotation offset"              )
MK_PFL(  540.80,GUIFWMED,2, "[arcsec] median guide star FWHM"                 )
MK_PFL(  540.81,GUIFW50, 2, "[arcsec] 50% range of guide star FWHM"           )
MK_PFL(  540.82,GUIFW90, 2, "[arcsec] 90% range of guide star FWHM"           )
MK_PFL(  540.90,GUIM1MED,2, "[mag] median zeropoint"                          )
MK_PFL(  540.91,GUIM150, 2, "[mag] 50% range of median zeropoint"             )
MK_PFL(  540.92,GUIM190, 2, "[mag] 90% range of median zeropoint"             )
MK_PFL(  540.93,GUIXTNCT,2, "[mag] total extinction during exposure"          )
MK_PFL(  540.94,GUIZPRMS,2, "[mag] median RMS scatter of zeropoints"          )

/*
 * Mirror info
 */
MK_PFL(  541.1, M1X     ,6, "[um] Primary mirror x position"                  )
MK_PFL(  541.2, M1Y     ,6, "[um] Primary mirror y position"                  )
MK_PFL(  541.3, M1Z     ,6, "[um] Primary mirror z position"                  )
MK_PFL(  541.4, M1TIP   ,6, "[arcsec] Primary mirror tip"                     )
MK_PFL(  541.5, M1TILT  ,6, "[arcsec] Primary mirror tilt"                    )
MK_PFL(  542.1, M2X     ,6, "[um] Secondary mirror x position"                )
MK_PFL(  542.2, M2Y     ,6, "[um] Secondary mirror y position"                )
MK_PFL(  542.3, M2Z     ,6, "[um] Secondary mirror z position"                )
MK_PFL(  542.4, M2TIP   ,6, "[arcsec] Secondary mirror tip"                   )
MK_PFL(  542.5, M2TILT  ,6, "[arcsec] Secondary mirror tilt"                  )
MK_STR(  543.0, M1M2MODV,   "Telescope mirror position model version"         )
MK_PFL(  543.1, M1NOMX  ,6, "[um] Modeled Primary mirror x position"          )
MK_PFL(  543.2, M1NOMY  ,6, "[um] Modeled Primary mirror y position"          )
MK_PFL(  543.3, M1NOMZ  ,6, "[um] Modeled Primary mirror z position"          )
MK_PFL(  543.4, M1NOMTIP,6, "[arcsec] Modeled Primary mirror tip"             )
MK_PFL(  543.5, M1NOMTIL,6, "[arcsec] Modeled Primary mirror tilt"            )
MK_PFL(  544.1, M2NOMX  ,6, "[um] Modeled Secondary mirror x position"        )
MK_PFL(  544.2, M2NOMY  ,6, "[um] Modeled Secondary mirror y position"        )
MK_PFL(  544.3, M2NOMZ  ,6, "[um] Modeled Secondary mirror z position"        )
MK_PFL(  544.4, M2NOMTIP,6, "[arcsec] Modeled Secondary mirror tip"           )
MK_PFL(  544.5, M2NOMTIL,6, "[arcsec] Modeled Secondary mirror tilt"          )
MK_STR(  550.1, TELTEMTR,   "[C] MTL 11,12,2,3,5,6,8,9"                       )
MK_STR(  550.2, TELTEMSP,   "[C] IS 10,1,4,7 O 10,1,4,7"                      )
MK_STR(  550.3, TELTEMMS,   "[C] M1 Perim. 12,3,6,9,Air"                      )
MK_STR(  550.4, TELTEMM1,   "[C] M1I 12,3,6,9 O12,3,6,9"                      )
MK_STR(  550.5, TELTEMM2,   "[C] M2 I 12,O 12,3,3,6,6,9,9"                    )
MK_STR(  550.6, TELTEMEX,   "[C] M2 Can Air,Surf,CentSec 12,3,6,9"            )

/*
 * Wavefront
 */
MK_PFL(  551.01,WVFA    ,14,"[N] Wavefront A actuator force"                  )
MK_PFL(  551.02,WVFB    ,14,"[N] Wavefront B actuator force"                  )
MK_PFL(  551.03,WVFC    ,14,"[N] Wavefront C actuator force"                  )
MK_PFL(  551.04,WVFD    ,14,"[N] Wavefront D actuator force"                  )
MK_PFL(  551.05,WVFE    ,14,"[N] Wavefront E actuator force"                  )
MK_PFL(  551.06,WVFF    ,14,"[N] Wavefront F actuator force"                  )
MK_PFL(  551.07,WVFG    ,14,"[N] Wavefront G actuator force"                  )
MK_PFL(  551.08,WVFH    ,14,"[N] Wavefront H actuator force"                  )
MK_PFL(  551.09,WVFI    ,14,"[N] Wavefront I actuator force"                  )
MK_PFL(  551.10,WVFJ    ,14,"[N] Wavefront J actuator force"                  )
MK_PFL(  551.11,WVFK    ,14,"[N] Wavefront K actuator force"                  )
MK_PFL(  551.12,WVFL    ,14,"[N] Wavefront L actuator force"                  )

/*
 * These probably want to be in a different section... %%%
 */
MK_PFL(  560.1, ENVTEM,  1, "[C]           Weather, outside temperature"      )
MK_PFL(  560.2, ENVHUM,  1, "[%]           Weather, relative humidity"        )
MK_PFL(  560.3, ENVWIN,  1, "[m/s]         Weather, wind speed"               )
MK_PFL(  560.4, ENVDIR,  1, "[deg E. of N] Weather, wind direction"           )
MK_PFL(  561.0, SEEING,  3, "[arcsec]      Weather, seeing from DIMM, FWHM"   )

/* 
 * Other stuff Craig had put in general.h:
 *
 *TYPE --IDX--  KEYWORD- PRC -------------------COMMENT----------------------*/
MK_PFL( 586.02, HA      ,16,"[deg] Hour angle at start"                       )
MK_PFL( 586.03,	ST      ,16,"[hours] Sidereal time at start"                  )
MK_PFL( 586.04,	ZD      ,16,"[deg] Zenith distance"                           )
MK_STR( 587.02,	HASTRNG ,   "Hour angle at start"                             )
MK_STR( 587.03,	STSTRNG ,   "Sidereal time at start"                          )     
MK_STR( 587.04,	ZDSTRNG ,   "[deg:mm:ss] Zenith distance"                     )                   
