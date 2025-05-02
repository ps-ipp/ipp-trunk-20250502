/*                                             -*- c-file-style: "Ellemtel" -*-

`fh_registry.h' - A registry of FITS keywords in use at CFHT.

This file is part of version 1 of the FITS Handling Library.
Read the `License' file for terms of use and distribution.
Copyright 2001, Canada-France-Hawaii Telescope, daprog@cfht.hawaii.edu.

___This header automatically generated from `Index'. Do not edit it here!___ */

/*
 * $Id: fh_registry.h,v 1.19 2003/11/25 21:06:28 thomas Exp $
 *
 * Documentation: See http://software.cfht.hawaii.edu/fh_registry/
 */

#ifndef _INCLUDED_fh_registry
#define _INCLUDED_fh_registry 1

#if defined(__GNUC__) && defined(__STDC__)
static const char fh_registry_rcs_id[] __attribute__ ((__unused__)) = "@(#) $Id: fh_registry.h,v 1.19 2003/11/25 21:06:28 thomas Exp $" ;
#else
static const char fh_registry_rcs_id[] = "@(#) $Id: fh_registry.h,v 1.19 2003/11/25 21:06:28 thomas Exp $";
#endif

/*
 * These are environment variables which the CFHT "ccd" process (both
 * script and older C program) make available to instrument handler programs.
 */
#define ENV_FFTEMPLATE	"FFTEMPLATE"
#define ENV_OBSTYPE	"OBSTYPE"
#define ENV_INTTIME	"INTTIME"
#define ENV_EXPNUM	"EXPNUM"
#define ENV_SEQNUM	"SEQNUM"

/*
 * This macro is used inside the other macros (ID_FLT, ID_INT, etc.)
 * so that the keyword names can be referred to as fh_kw_FOO
 * This can be used as an identifier to fh_get() or any other function
 * instead of using the string constant "FOO".
 */

#define ID_FLT(idx, name, keyword, prec, comment) \
static inline void \
fh_set_##name (HeaderUnit hu, double value) \
{ fh_set_flt(hu, idx, keyword, value, prec, comment); } \
static inline void \
fh_setval_##name (HeaderUnit hu, const char* value) \
{ fh_set_val(hu, idx, keyword, value, comment); } \
static inline void \
fh_setcmt_##name (HeaderUnit hu, double value, const char* new_comment) \
{ fh_set_flt(hu, idx, keyword, value, prec, new_comment); } \
static inline fh_result \
fh_get_##name (HeaderUnit hu, double* value) \
{ return fh_get_flt(hu, keyword, value); } \
static inline const char* \
fh_kw_##name(void) { return keyword; } \
static inline const char* \
fh_cmt_##name(void) { return comment; }

#define ID_PFL(idx, name, keyword, prec, comment) \
static inline void \
fh_set_##name (HeaderUnit hu, double value) \
{ fh_set_pfl(hu, idx, keyword, value, prec, comment); } \
static inline void \
fh_setval_##name (HeaderUnit hu, const char* value) \
{ fh_set_val(hu, idx, keyword, value, comment); } \
static inline void \
fh_setcmt_##name (HeaderUnit hu, double value, const char* new_comment) \
{ fh_set_pfl(hu, idx, keyword, value, prec, new_comment); } \
static inline fh_result \
fh_get_##name (HeaderUnit hu, double* value) \
{ return fh_get_flt(hu, keyword, value); } \
static inline const char* \
fh_kw_##name(void) { return keyword; } \
static inline const char* \
fh_cmt_##name(void) { return comment; }

#define ID_INT(idx, name, keyword, comment) \
static inline void \
fh_set_##name (HeaderUnit hu, int value) \
{ fh_set_int(hu, idx, keyword, value, comment); } \
static inline void \
fh_setval_##name (HeaderUnit hu, const char* value) \
{ fh_set_val(hu, idx, keyword, value, comment); } \
static inline void \
fh_setcmt_##name (HeaderUnit hu, int value, const char* new_comment) \
{ fh_set_int(hu, idx, keyword, value, new_comment); } \
static inline fh_result \
fh_get_##name (HeaderUnit hu, int* value) \
{ return fh_get_int(hu, keyword, value); } \
static inline const char* \
fh_kw_##name(void) { return keyword; } \
static inline const char* \
fh_cmt_##name(void) { return comment; }

#define ID_BLN(idx, name, keyword, comment) \
static inline void \
fh_set_##name (HeaderUnit hu, fh_bool value) \
{ fh_set_bool(hu, idx, keyword, value, comment); } \
static inline void \
fh_setval_##name (HeaderUnit hu, const char* value) \
{ fh_set_val(hu, idx, keyword, value, comment); } \
static inline void \
fh_setcmt_##name (HeaderUnit hu, fh_bool value, const char* new_comment) \
{ fh_set_bool(hu, idx, keyword, value, new_comment); } \
static inline fh_result \
fh_get_##name (HeaderUnit hu, fh_bool* value) \
{ return fh_get_bool(hu, keyword, value); } \
static inline const char* \
fh_kw_##name(void) { return keyword; } \
static inline const char* \
fh_cmt_##name(void) { return comment; }

#define ID_STR(idx, name, keyword, comment) \
static inline void \
fh_set_##name (HeaderUnit hu, const char* value) \
{ fh_set_str(hu, idx, keyword, value, comment); } \
static inline void \
fh_setval_##name (HeaderUnit hu, const char* value) \
{ fh_set_val(hu, idx, keyword, value, comment); } \
static inline void \
fh_setcmt_##name (HeaderUnit hu, const char* value, const char* new_comment) \
{ fh_set_str(hu, idx, keyword, value, new_comment); } \
static inline fh_result \
fh_get_##name (HeaderUnit hu, char* value, int maxlen) \
{ return fh_get_str(hu, keyword, value, maxlen); } \
static inline const char* \
fh_kw_##name(void) { return keyword; } \
static inline const char* \
fh_cmt_##name(void) { return comment; }

#define ID_VAL(idx, name, keyword, comment) \
static inline void \
fh_set_##name (HeaderUnit hu, const char* value) \
{ fh_set_val(hu, idx, keyword, value, comment); } \
static inline void \
fh_setcmt_##name (HeaderUnit hu, const char* value, const char* new_comment) \
{ fh_set_val(hu, idx, keyword, value, new_comment); } \
static inline const char* \
fh_kw_##name(void) { return keyword; } \
static inline const char* \
fh_cmt_##name(void) { return comment; }

#define ID_CMT(idx, name, comment) \
static inline void \
fh_set_##name (HeaderUnit hu) \
{ fh_set_com(hu, idx, "COMMENT", comment); }

/*
 * Now the auto-stringified versions of ID_*() ...
 */
#define MK_FLT(idx, name, dig, comment) ID_FLT(idx, name, #name, dig, comment)
#define MK_PFL(idx, name, dec, comment) ID_PFL(idx, name, #name, dec, comment)
#define MK_INT(idx, name, comment) ID_INT(idx, name, #name, comment)
#define MK_STR(idx, name, comment) ID_STR(idx, name, #name, comment)
#define MK_VAL(idx, name, comment) ID_VAL(idx, name, #name, comment)
#define MK_BLN(idx, name, comment) ID_BLN(idx, name, #name, comment)
#define MK_CMT(idx, name, comment) ID_CMT(idx, name, comment)

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
MK_PFL( 10.0,   BZERO   ,1, "Zero factor"                                     )
MK_PFL( 11.0,	BSCALE  ,1, "Scale factor"                                    )

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
MK_STR(   52.0, CMMTSEQ ,   ""                                                )
MK_STR(   53.0, OBJECT  ,   ""                                                )
MK_STR(   54.0, OBSERVER,   ""                                                )
MK_STR(   55.0, PI_NAME ,   ""                                                )
MK_STR(   56.0, RUNID   ,   ""                                                )

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
MK_STR(  74.00, DATE    ,   "UTC Date of file creation"                       )
MK_STR(  74.10, HSTTIME ,   "Local time in Hawaii"                            )
MK_STR(  77.00, IMAGESWV,   "Image creation software version"                 )

/*
 * Summary : 90.0 Key stuff from instrument, detector (and other?) sections
 */
MK_STR(   91.0,	DETECTOR,   "Science Detector"                                )
MK_STR(   92.0, INSTRUME,   "Instrument Name"                                 )
MK_STR(   92.1, INSTMODE,   "Instrument Mode"                                 )

/*
 * Detector : 100.000 to 199.999
 *
 *TYPE --IDX--  KEYWORD- PRC -------------------COMMENT----------------------*/
MK_CMT(  100.1,	CMTDET1	,   ""                                                )
MK_CMT(  100.2,	CMTDET2	,   "Detector"                                        )
MK_CMT(  100.3,	CMTDET3	,   "--------"                                        )
MK_CMT(  100.4,	CMTDET4	,   ""                                                )
MK_STR(  102.0, DETSIZE	,   "Total data pixels in full mosaic"                )
MK_STR(  103.0,	RASTER	,   ""                                                )
MK_STR(  105.0,	CCDSUM	,   "Binning factors"                                 )
MK_INT(  105.1,	CCDBIN1	,   "Binning factor along first axis"                 )
MK_INT(  105.2,	CCDBIN2	,   "Binning factor along second axis"                )
MK_FLT(  110.0,	PIXSIZE ,3, "Pixel size for both axes (microns)"              )
MK_FLT(  111.1,	PIXSIZE1,3, "Pixel size for axis 1 (microns)"                 )
MK_FLT(  111.2,	PIXSIZE2,3, "Pixel size for axis 2 (microns)"                 )
MK_FLT(  112.1,	PIXSCAL1,4, "Pixel scale for axis 1 (arcsec/pixel)"           )
MK_FLT(  112.2,	PIXSCAL2,4, "Pixel scale for axis 2 (arcsec/pixel)"           )
MK_STR(  130.0,	AMPLIST	,   "List of amplifiers for this image"               )
MK_STR(  131.0,	AMPNAME	,   "Amplifier name"                                  )
MK_STR(  140.0,	CCDSIZE ,   "Detector imaging area size"                      )
MK_STR(  140.1,	DETSEC  ,   "Mosaic area of the detector"                     )
MK_STR(  140.11,DETSECA ,   "Mosaic area of the detector from Amp A"          )
MK_STR(  140.12,DETSECB ,   "Mosaic area of the detector from Amp B"          )
MK_STR(  140.13,DETSECC ,   "Mosaic area of the detector from Amp C"          )
MK_STR(  140.14,DETSECD ,   "Mosaic area of the detector from Amp D"          )
MK_STR(  140.2,	DATASEC ,   "Imaging area of the detector"                    )
MK_STR(  140.3,	BIASSEC ,   "Overscan (bias) area of the detector"            )
MK_STR(  141.11,ASECA   ,   "Section from Amp A (non-contig. bias excluded)"  )
MK_STR(  141.12,ASECB   ,   "Section from Amp B (non-contig. bias excluded)"  )
MK_STR(  141.13,ASECC   ,   "Section from Amp C (non-contig. bias excluded)"  )
MK_STR(  141.14,ASECD   ,   "Section from Amp D (non-contig. bias excluded)"  )
MK_STR(  141.21,BSECA   ,   "Overscan (bias) area from Amp A"                 )
MK_STR(  141.22,BSECB   ,   "Overscan (bias) area from Amp B"                 )
MK_STR(  141.23,BSECC   ,   "Overscan (bias) area from Amp C"                 )
MK_STR(  141.24,BSECD   ,   "Overscan (bias) area from Amp D"                 )
MK_STR(  141.31,CSECA   ,   "Section in full CCD for DSECA"                   )
MK_STR(  141.32,CSECB   ,   "Section in full CCD for DSECB"                   )
MK_STR(  141.33,CSECC   ,   "Section in full CCD for DSECC"                   )
MK_STR(  141.34,CSECD   ,   "Section in full CCD for DSECD"                   )
MK_STR(  141.41,DSECA   ,   "Imaging area from Amp A"                         )
MK_STR(  141.42,DSECB   ,   "Imaging area from Amp B"                         )
MK_STR(  141.43,DSECC   ,   "Imaging area from Amp C"                         )
MK_STR(  141.44,DSECD   ,   "Imaging area from Amp D"                         )
MK_STR(  141.51,TSECA   ,   "Trim section for Amp A"                          )
MK_STR(  141.52,TSECB   ,   "Trim section for Amp B"                          )
MK_STR(  141.53,TSECC   ,   "Trim section for Amp C"                          )
MK_STR(  141.54,TSECD   ,   "Trim section for Amp D"                          )
MK_STR(  150.1, CCDNAME,    "Name of the CCD (manufacturer reference)"        )
MK_STR(  150.2, CCDNICK,    "Nickname of the CCD"                             )
MK_INT(  152.0, MAXLIN  ,   "Maximum linearity value (ADU)"                   )
MK_INT(  152.01,MAXLINA ,   "Maximum linearity value for Amp A (ADU)"         )
MK_INT(  152.02,MAXLINB ,   "Maximum linearity value for Amp B (ADU)"         )
MK_INT(  152.03,MAXLINC ,   "Maximum linearity value for Amp C (ADU)"         )
MK_INT(  152.04,MAXLIND ,   "Maximum linearity value for Amp D (ADU)"         )
MK_INT(  152.1, SATURATE,   "Saturation value (ADU)"                          )
MK_FLT(  155.0,	GAIN    ,3, "Amplifier gain (electrons/ADU)"                  )
MK_FLT(  155.01,GAINA   ,3, "Amp A gain (electrons/ADU)"                      )
MK_FLT(  155.02,GAINB   ,3, "Amp B gain (electrons/ADU)"                      )
MK_FLT(  155.03,GAINC   ,3, "Amp C gain (electrons/ADU)"                      )
MK_FLT(  155.04,GAIND   ,3, "Amp D gain (electrons/ADU)"                      )
MK_FLT(  156.0,	RDNOISE ,3, "Read noise (electrons)"                          )
MK_FLT(  156.01,RDNOISEA,3, "Amp A read noise (electrons)"                    )
MK_FLT(  156.02,RDNOISEB,3, "Amp B read noise (electrons)"                    )
MK_FLT(  156.03,RDNOISEC,3, "Amp C read noise (electrons)"                    )
MK_FLT(  156.04,RDNOISED,3, "Amp D read noise (electrons)"                    )
MK_FLT(  157.0, DCURRENT,5, "Dark current (ADU/pixel/second)"                 )
MK_FLT(  157.1, DARKCUR ,5, "Dark current (e-/pixel/hour)"                    )
MK_STR(  158.0,	QEPOINTS,   "QE%@wavelength in nm"                            )
MK_STR(  170.0, CONSWV	,   "Controller software DSPID and SERNO versions"    )
MK_STR(  180.0,	DETSTAT	,   "Detector status"                                 )
MK_FLT(  190.0, DETTEM  ,3, "Detector temperature"                            )

/* ****************************************************
 * *** World Coordinate System : 300.000 to 399.999 ***
 * ****************************************************
 *
 *TYPE --IDX--  KEYWORD- PRC -------------------COMMENT----------------------*/

/*
 * Telescope : 500.000 to 599.999
 *
 * Almost all keywords we define can be generated with the MK_XXX macros.
 * The only exceptions are those keywords which contain a '-', since the
 * '-' is not a legal character in a C identifier.  TCS uses four of them
 * (DATE-OBS, UTC-OBS, MJD-OBS, and LST-OBS.)  The ID_XXX macros are used
 * and the '-' is translated to a '_' (underscore) for use in C programs.
 *
 * NOTE: TELESCOP, TELSTAT, TELCONF, LATITUDE, and LONGITUD are currently
 * inserted by DetCom and _not_ tcsh.  This should probably be changed.
 *
 * Further details on some less obvious keywords:
 *
 *   SKYFLUX  - if the guider(s) can generate a reasonable guess for the
 *              amount of flux currently available for doing flats, that
 *              number is in this keyword - negative means not available
 *
 *   GUIFLUXn - these values are the most recent one minute average of the
 *              total flux seen in whatever guiding box/area is in use
 *
 *   GUIFWH?n - these are the most recent one minute average FWHM values
 *              for the x/y axes of the guider image(s) recorded at the
 *              start of the exposure
 *
 *   ISUSTDV? - these are the most recent one minute standard deviations
 *              in the ISU x and y positions in arc seconds recorded
 *              at the start of the exposure
 *
 *   FSAstats - the FSA statistics are for the most recent 10 minutes
 *              recorded at the start of the exposure
 *
 * If there are multiple guiders in use, the unnumbered GUIEQUIN/GUIRADEC
 * /GUIRA/GUIDEC/GUIRAPM/GUIDECPM values will duplicate the numbered
 * values for the primary, reference guider.  If there is only one guider
 * in use, only the unnumbered values should be present.
 *
 *TYPE --IDX--  SYMBOL--  KEYWORD-  -----------COMMENT----------------------*/
ID_STR( 511.1,	DATE_OBS,"DATE-OBS", "Date at start of observation (UTC)"     )
ID_STR( 512.1,	UTC_OBS ,"UTC-OBS", "Time at start of observation (UTC)"      )
ID_PFL( 513.1,  MJD_OBS ,"MJD-OBS",7, "Modified Julian Date at start of obs." )
ID_STR( 514.1,	LST_OBS ,"LST-OBS", "Sidereal time at start of exposure"      )

/*TYPE --IDX--  KEYWORD- PRC -------------------COMMENT----------------------*/
MK_CMT(  500.1, CMTTCS1 ,   ""                                                )
MK_CMT(  500.2, CMTTCS2 ,   "Telescope"                                       )
MK_CMT(  500.3, CMTTCS3 ,   "---------"                                       )
MK_CMT(  500.4, CMTTCS4 ,   ""                                                )
MK_STR(  501.1, TELESCOP,   ""                                                )
MK_STR(  501.2, ORIGIN  ,   "Canada-France-Hawaii Telescope"                  )
MK_PFL(  502.1, LATITUDE,6, "Latitude (degrees N)"                            )
MK_PFL(  502.2, LONGITUD,6, "Longitude (degrees E)"                           )
MK_STR(  503.0, TELSTAT ,   "Telescope Control System status"                 )
MK_STR(  510.0, TIMESYS ,   "Time System for DATExxxx"                        )
/* note that the following three have xxx.1 versions above */
MK_STR(  512.0,	UTIME   ,   "Time at start of observation (UT)"               )
MK_PFL(  513.0,	MJDATE  ,7 ,"Modified Julian Date at start of observation"    )
MK_STR(  514.0,	SIDTIME ,   "Sidereal time at start of observation"           )
MK_VAL(  521.0,	EPOCH   ,   "Equinox of coordinates"                          )
MK_VAL(  521.1,	EQUINOX ,   "Equinox of coordinates"                          )
MK_STR(  522.0,	RADECSYS,   "Coordinate system for equinox (FK4/FK5/GAPPT)"   )
MK_STR(  522.1,	RA      ,   "Object right ascension"                          )
MK_STR(  522.2,	DEC     ,   "Object declination"                              )
MK_PFL(  523.1,	RA_DEG  ,6, "Object right ascension in degrees"               )
MK_PFL(  523.2,	DEC_DEG ,6, "Object declination in degrees"                   )

MK_PFL(525.101,	CRVAL1	,5, "WCS Ref value (RA in decimal degrees)"           )
MK_PFL(525.102,	CRVAL2	,5, "WCS Ref value (DEC in decimal degrees)"          )
MK_STR(525.211,	CTYPE1	,   "WCS Coordinate type"                             )
MK_STR(525.212,	CTYPE2	,   "WCS Coordinate type"                             )
MK_PFL(525.301,	CRPIX1	,1, "WCS Coordinate reference pixel"                  )
MK_PFL(525.302,	CRPIX2	,1, "WCS Coordinate reference pixel"                  )
MK_FLT(525.411,	CD1_1	,6, "WCS Coordinate scale matrix"                     )
MK_FLT(525.412,	CD1_2	,6, "WCS Coordinate scale matrix"                     )
MK_FLT(525.421,	CD2_1	,6, "WCS Coordinate scale matrix"                     )
MK_FLT(525.422,	CD2_2	,6, "WCS Coordinate scale matrix"                     )

MK_INT(  530.0,	NGUIDER	,   "TCS Number of guiders"                           )
MK_INT( 530.01, NGUISTAR,   "TCS number of guide stars/probes in use"         )
MK_STR(  530.1, GUINAME ,   "TCS guider name"                                 )
MK_VAL( 530.21,	GUIEQUIN,   "TCS guider equinox"                              )
MK_STR( 530.22,	GUIRADEC,   "TCS guider system for equinox"                   )
MK_STR( 530.23,	GUIRA   ,   "TCS guider right ascension"                      )
MK_STR( 530.24,	GUIDEC  ,   "TCS guider declination"                          )
MK_PFL( 530.25, GUIRAPM ,2, "TCS guider right ascension proper motion arcsec" )
MK_PFL( 530.26, GUIDECPM,2, "TCS guider declination proper motion arcsec"     )
MK_STR(  530.3, GUIOBJN ,   "TCS guider object name"                          )
MK_PFL(  530.4, GUIMAGN ,1, "TCS guider object magnitude"                     )
MK_PFL( 530.51,	XPROBE  ,3 ,"Telescope bonnette guide probe X position"       )
MK_PFL( 530.52,	YPROBE  ,3 ,"Telescope bonnette guide probe Y position"       )
MK_PFL( 530.53,	ZPROBE  ,3 ,"Telescope bonnette guide probe Z position"       )
MK_INT(  530.6, GUIFLUX ,   "TCS guider flux"                                 )
MK_PFL( 530.71, GUIFWHX ,2, "TCS guider average x FWHM in pixels"             )
MK_PFL( 530.72, GUIFWHY ,2, "TCS guider average y FWHM in pixels"             )
MK_INT(  530.8, SKYFLUX ,   "TCS total sky flux"                              )

MK_STR(  531.1,	GUINAME1,   "TCS guider #1 identification"                    )
MK_VAL( 531.21,	GUIEQUI1,   "TCS guider #1 equinox"                           )
MK_STR( 531.22,	GUIRADE1,   "TCS guider #1 system for equinox"                )
MK_STR( 531.23,	GUIRA1  ,   "TCS guider #1 right ascension"                   )
MK_STR( 531.24,	GUIDEC1 ,   "TCS guider #1 declination"                       )
MK_PFL( 531.25, GUIRAPM1,2, "TCS guider #1 right ascen proper motion arcsec"  )
MK_PFL( 531.26, GUIDEPM1,2, "TCS guider #1 declination proper motion arcsec"  )
MK_STR(  531.3, GUIOBJN1,   "TCS guider #1 object name"                       )
MK_PFL(	 531.4,	GUIMAGN1,1, "TCS guider #1 object magnitude"                  )
MK_PFL( 531.51, GUIPOSX1,3, "TCS guider #1 probe x position"                  )
MK_PFL( 531.52, GUIPOSY1,3, "TCS guider #1 probe y position"                  )
MK_PFL( 531.53, GUIPOSZ1,3, "TCS guider #1 probe z position"                  )
MK_INT(  531.6, GUIFLUX1,   "TCS guider #1 flux"                              )
MK_PFL( 531.71, GUIFWHX1,2, "TCS guider #1 average x FWHM in pixels"          )
MK_PFL( 531.72, GUIFWHY1,2, "TCS guider #1 average y FWHM in pixels"          )

MK_STR(  532.1,	GUINAME2,   "TCS guider #2 identification"                    )
MK_VAL( 532.21,	GUIEQUI2,   "TCS guider #2 equinox"                           )
MK_STR( 532.22,	GUIRADE2,   "TCS guider #2 system for equinox"                )
MK_STR( 532.23,	GUIRA2  ,   "TCS guider #2 right ascension"                   )
MK_STR( 532.24,	GUIDEC2 ,   "TCS guider #2 declination"                       )
MK_PFL( 532.25, GUIRAPM2,2, "TCS guider #2 right ascen proper motion arcsec"  )
MK_PFL( 532.26, GUIDEPM2,2, "TCS guider #2 declination proper motion arcsec"  )
MK_STR(  532.3, GUIOBJN2,   "TCS guider #2 object name"                       )
MK_PFL(	 532.4,	GUIMAGN2,1, "TCS guider #2 object magnitude"                  )
MK_PFL( 532.51, GUIPOSX2,3, "TCS guider #2 probe x position"                  )
MK_PFL( 532.52, GUIPOSY2,3, "TCS guider #2 probe y position"                  )
MK_PFL( 532.53, GUIPOSZ2,3, "TCS guider #2 probe z position"                  )
MK_INT(  532.6, GUIFLUX2,   "TCS guider #2 flux"                              )
MK_PFL( 532.71, GUIFWHX2,2, "TCS guider #2 average x FWHM in pixels"          )
MK_PFL( 532.72, GUIFWHY2,2, "TCS guider #2 average y FWHM in pixels"          )

MK_STR(  533.1,	GUINAME3,   "TCS guider #3 identification"                    )
MK_VAL( 533.21,	GUIEQUI3,   "TCS guider #3 equinox"                           )
MK_STR( 533.22,	GUIRADE3,   "TCS guider #3 system for equinox"                )
MK_STR( 533.23,	GUIRA3  ,   "TCS guider #3 right ascension"                   )
MK_STR( 533.24,	GUIDEC3 ,   "TCS guider #3 declination"                       )
MK_PFL( 533.25, GUIRAPM3,2, "TCS guider #3 right ascen proper motion arcsec"  )
MK_PFL( 533.26, GUIDEPM3,2, "TCS guider #3 declination proper motion arcsec"  )
MK_STR(  533.3, GUIOBJN3,   "TCS guider #3 object name"                       )
MK_PFL(	 533.4,	GUIMAGN3,1, "TCS guider #3 object magnitude"                  )
MK_PFL( 533.51, GUIPOSX3,3, "TCS guider #3 probe x position"                  )
MK_PFL( 533.52, GUIPOSY3,3, "TCS guider #3 probe y position"                  )
MK_PFL( 533.53, GUIPOSZ3,3, "TCS guider #3 probe z position"                  )
MK_INT(  533.6, GUIFLUX3,   "TCS guider #3 flux"                              )
MK_PFL( 533.71, GUIFWHX3,2, "TCS guider #3 average x FWHM in pixels"          )
MK_PFL( 533.72, GUIFWHY3,2, "TCS guider #3 average y FWHM in pixels"          )

MK_STR(  534.1,	GUINAME4,   "TCS guider #4 identification"                    )
MK_VAL( 534.21,	GUIEQUI4,   "TCS guider #4 equinox"                           )
MK_STR( 534.22,	GUIRADE4,   "TCS guider #4 system for equinox"                )
MK_STR( 534.23,	GUIRA4  ,   "TCS guider #4 right ascension"                   )
MK_STR( 534.24,	GUIDEC4 ,   "TCS guider #4 declination"                       )
MK_PFL( 534.25, GUIRAPM4,2, "TCS guider #4 right ascen proper motion arcsec"  )
MK_PFL( 534.26, GUIDEPM4,2, "TCS guider #4 declination proper motion arcsec"  )
MK_STR(  534.3, GUIOBJN4,   "TCS guider #4 object name"                       )
MK_PFL(	 534.4,	GUIMAGN4,1, "TCS guider #4 object magnitude"                  )
MK_PFL( 534.51, GUIPOSX4,3, "TCS guider #4 probe x position"                  )
MK_PFL( 534.52, GUIPOSY4,3, "TCS guider #4 probe y position"                  )
MK_PFL( 534.53, GUIPOSZ4,3, "TCS guider #4 probe z position"                  )
MK_INT(  534.6, GUIFLUX4,   "TCS guider #4 flux"                              )
MK_PFL( 534.71, GUIFWHX4,2, "TCS guider #4 average x FWHM in pixels"          )
MK_PFL( 534.72, GUIFWHY4,2, "TCS guider #4 average y FWHM in pixels"          )

MK_PFL(  540.1,	AIRMASS ,3, "Airmass at start of observation"                 )
MK_PFL(  540.2, TELALT  ,2, "Telescope altitude at start of observation, deg" )
MK_PFL(  540.3, TELAZ   ,2, "Telescope azimuth at start, deg, 0=N 90=E 270=W" )
MK_PFL(  540.4, MOONANGL,2, "Angle from object to moon at start in degrees"   )
MK_STR(  550.0,	FOCUSID ,   "Telescope focus in use"                          )
MK_STR(  550.1, TELCONF ,   "Telescope focus in use"                          )
MK_INT(  551.0,	FOCUSPOS,   "Telescope focus encoder readout"                 )
MK_INT(  551.1,	TELFOCUS,   "Telescope focus encoder readout"                 )
MK_PFL(  552.0,	BONANGLE,2, "Telescope bonnette rotation angle in degrees"    )
MK_PFL(  552.1,	ROTANGLE,2, "Telescope bonnette rotation angle in degrees"    )

/*
 * The following values perhaps should have their own section, but they
 * are also (with some stretch) a part of TCS so for now they are here.
 * They should only be included in MegaPrime and WIRCam images.
 */

MK_PFL(  560.1, ISUGAIN ,2, "Instrument Stabilization Unit control gain"      )
MK_PFL(  560.2, ISURATE ,2, "ISU control rate in Hz"                          )
MK_PFL(  560.3, ISUTCSOR,2, "ISU offload rate in Hz to telescope position"    )
MK_STR(  560.4, ISUSTATE,   "ISU control state (Off/Only/TCS/Full/Frozen)"    )
MK_PFL(  560.5, ISUSTDVX,2, "ISU most recent minute std dev in x in arcsec"   )
MK_PFL(  560.6, ISUSTDVY,2, "ISU most recent minute std dev in y in arcsec"   )

/*
 * The following values perhaps should have their own section, but they
 * are also (with some stretch) a part of TCS so for now they are here.
 * They should only be included in MegaPrime images.
 */

MK_PFL(  570.1, FSAGAIN ,2, "Focus Stage Assembly movement control gain"      )
MK_PFL(  570.2, FSARATE ,2, "FSA control rate in Hz"                          )
MK_PFL(  570.3, FSATHRES,3, "FSA movement threshold in mm"                    )
MK_STR(  570.4, FSASTATE,   "FSA control state (Off/On/Paused)"               )
MK_INT(  570.5, FSANMOVE,   "FSA number of actual moves in last 10 minutes"   )
MK_PFL( 570.61, FSAMINZ ,3, "FSA minimum position in last 10 minutes"         )
MK_PFL( 570.62, FSAMAXZ ,3, "FSA maximum position in last 10 minutes"         )
MK_PFL( 570.63, FSAMEANZ,3, "FSA mean position in last 10 minutes"            )
MK_PFL( 570.64, FSASTDVZ,3, "FSA position std dev in last 10 minutes"         )

/*
 * The following values are used to collect data for TCS pointing model
 * tuning.  Some of the values duplicate numbers elsewhere in the headers,
 * but where it matters these are all read at the same time in the IOC.
 */

MK_STR( 580.11, TCSGPSBC,   "TCS GPS read out in BCD"                         )
MK_PFL( 580.12, TCSGPSTM,5, "TCS GPS clock time in decimal hours"             )
MK_STR( 580.13, TCSRBUSS,   "TCS RBUSS clock time"                            )
MK_STR( 580.14, TCSEPICS,   "TCS EPICS clock time"                            )
MK_STR( 580.15, TCSAMODE,   "TCS acquisition mode - T/O/G/g for t/o/g coords" )
MK_PFL( 580.21, TCSMJD  ,7, "TCS MJD"                                         )
MK_PFL( 580.22, TCSLST  ,5, "TCS LST in decimal hours"                        )
MK_PFL( 580.31, TCSAPHA ,4, "TCS apparent hour angle in degrees"              )
MK_PFL( 580.32, TCSAPDEC,4, "TCS apparent declination in degrees"             )
MK_PFL( 580.41, TCSOBHA ,4, "TCS observed hour angle in degrees"              )
MK_PFL( 580.42, TCSOBDEC,4, "TCS observed declination in degrees"             )
MK_INT( 580.51, TCSENHA ,   "TCS hour angle encoder bits reading"             )
MK_INT( 580.52, TCSENDEC,   "TCS declination encoder bits reading"            )
MK_PFL( 580.61, TCSEDHA ,4, "TCS hour angle encoder in degrees"               )
MK_PFL( 580.62, TCSEDDEC,4, "TCS declination encoder in degrees"              )
MK_PFL( 580.71, TCSMVRA ,4, "TCS right ascension acquisition move in arcsec"  )
MK_PFL( 580.72, TCSMVDEC,4, "TCS declination acquisition move in arcsec"      )
MK_PFL( 580.81, TCSMVX  ,3, "TCS secondary guider acquisition x move in mm"   )
MK_PFL( 580.82, TCSMVY  ,3, "TCS secondary guider acquisition y move in mm"   )

/*
 * Adaptive Optics : 700.000 to 799.999
 *
 *
 *TYPE --IDX--  KEYWORD- PRC -------------------COMMENT----------------------*/

MK_CMT(  700.1, CMTAOB1 ,   ""                                                )
MK_CMT(  700.2, CMTAOB2 ,   "Adaptive Optics Bonnette"                        )
MK_CMT(  700.3, CMTAOB3 ,   "------------------------"                        )
MK_CMT(  700.4, CMTAOB4 ,   ""                                                )

/* 710 through 719 are input control values to the real time system */

/*
 * state of AO correction (terms come from original description of loop
 * operation which was deemed not PC and changed to "correction")
 */
MK_STR(  711.0, AOBLOOP ,   "AOB closed loop Open/Closed"                     )
/*
 * primary gain in the integrator providing signals to the bimorph
 */
MK_PFL(  712.0, LOOPGAIN,1, "Gain of the integrator"                          )
/*
 * correction can be fully automatic (all modes, bimorph offloading to
 * tip/tilt mirror - nested), manual (manually selected modes, tip/tilt
 * mode if selected only feeds tip/tilt mirror, i.e., not via bimorph
 * - single), or tip/tilt only (bimorph not used at all, kept flat)
 */
MK_STR(  713.0, LOOPNES ,   "Loop type \"Nested\"/\"Tip/Tilt\"/\"Single\""    )
/*
 * in automatic correction this is the gain between the bimorph and the
 * tip/tilt mirror
 */
MK_PFL(  714.0, LOOPNESG,1, "Nested loop gain"                                )
/*
 * the individual mode gains can be fixed or optimized based on signal
 * levels
 */
MK_STR(  715.0, LOOPOPT ,   "Optimized loop control True/False"               )
/*
 * stroke of the membrane mirror (1-256)
 */
MK_INT(  716.0, WFSGOPT ,   "WFS optical gain"                                )
/*
 * integration time for each calculation cycle
 */
MK_PFL(  717.0, WFSSAMP ,3, "WFS sampling period (sec)"                       )

/* 720-729 are measured values calculated by the real time system */

/*
 * calculated value of R0
 */
MK_PFL(  721.0, R0      ,1, "Fried parameter (cm)"                            )
/*
 * sum of the photon counts on the 19 APD's, for the most recent integration
 */
MK_INT(  722.0, WFSCOUNT,   "Total flux count on WFS"                         )

/* 750-769 are input control values to the bench control ProLog(tm) */

/*
 * whether the atmospheric dispersion corrector (ADC) is in or out of the
 * optical path
 */
MK_STR(  751.0, ADCPOS  ,   "AOB ADC position In/Out"                         )
/*
 * angle of the dispersion caused by the ADC
 */
MK_PFL(  752.0, ADCANGLE,2, "AOB ADC angle (degrees)"                         )
/*
 * amount of dispersion caused by the relative angle between the ADC prisms
 */
MK_PFL(  753.0, ADCPOWER,4, "AOB ADC power"                                   )
/*
 * beam splitter id code read from slugs on the splitters
 */
MK_INT(  754.0, BEAMSPID,   "Beam splitter ID"                                )
/*
 * beam splitter description based on the above codes
 */
MK_STR(  755.0, BEAMSP  ,   "Beam splitter description"                       )
/*
 * whether the AOB central mirror is in (AO system in light path)
 * or out (AO system not being used)
 */
MK_STR(  756.0, MIRSLIDE,   "AOB mirror slide \"F/20\"/\"F/8\""               )
/*
 * neutral density filter position (in optical path to WFS)
 *   0 == home
 *   1 == no filter (close but not quite the same physical position as home)
 *   2 == 1.0 density filter
 *   3 == 2.0    "      "
 *   4 == 3.0    "      "
 */
MK_INT(  757.0, WFSNDID ,   "WFS ND filter position"                          )
/*
 * following 3 are the Wave Front Sensor (WFS) position in a coordinate
 * system close to having z parallel to the optical path
 */
MK_PFL(  761.0, WFSX    ,1, "Translated WFS X coord (steps)"                  )
MK_PFL(  762.0, WFSY    ,1, "Translated WFS Y coord (steps)"                  )
MK_PFL(  763.0, WFSZ    ,1, "Translated WFS Z coord (steps)"                  )
/*
 * Calibration Sources : 900.000 to 999.999
 *
 *
 *TYPE --IDX--  KEYWORD- PRC -------------------COMMENT----------------------*/

MK_CMT(  900.1, CMTCAL1 ,   ""                                                )
MK_CMT(  900.2, CMTCAL2 ,   "Calibration sources"                             )
MK_CMT(  900.3, CMTCAL3 ,   "-------------------"                             )
MK_CMT(  900.4, CMTCAL4 ,   ""                                                )

/* 910 - 919 are flat field lamps */

/*
 * Gecko lamps are computer controlled, these give On/Off status and
 * intensity
 */
MK_STR(  910.0, FFLAMPON,   "flatfield lamp status ON/OFF"                    )
MK_INT(  911.0, FFLAMP  ,   "flatfield lamp intensity"                        )
/* add which lamp if get control of dome flat fields */

/* 920-999 are calibration lamp sources */

/*
 * Gecko has 4 lamps, these give On/Off status and descriptions
 */
MK_STR(  920.0, CLAMP0ON,   "comparison lamp 0 status ON/OFF"                 )
MK_STR(  920.1, CLAMP0  ,   "comparison lamp 0 description"                   )
MK_STR(  921.0, CLAMP1ON,   "comparison lamp 1 status ON/OFF"                 )
MK_STR(  921.1, CLAMP1  ,   "comparison lamp 1 description"                   )
MK_STR(  922.0, CLAMP2ON,   "comparison lamp 2 status ON/OFF"                 )
MK_STR(  922.1, CLAMP2  ,   "comparison lamp 2 description"                   )
MK_STR(  923.0, CLAMP3ON,   "comparison lamp 3 status ON/OFF"                 )
MK_STR(  923.1, CLAMP3  ,   "comparison lamp 3 description"                   )

/*
 * GumBall has 10 lamps, though the last two are Fabre-Perot lamps sharing
 * the same light path and cannot be on together, these give On/Off status
 * for each
 */
MK_STR(  930.0, CALIBL0 ,   "lamp 0 ON/OFF"                                   )
MK_STR(  931.0, CALIBL1 ,   "lamp 1 ON/OFF"                                   )
MK_STR(  932.0, CALIBL2 ,   "lamp 2 ON/OFF"                                   )
MK_STR(  933.0, CALIBL3 ,   "lamp 3 ON/OFF"                                   )
MK_STR(  934.0, CALIBL4 ,   "lamp 4 ON/OFF"                                   )
MK_STR(  935.0, CALIBL5 ,   "lamp 5 ON/OFF"                                   )
MK_STR(  936.0, CALIBL6 ,   "lamp 6 ON/OFF"                                   )
MK_STR(  937.0, CALIBL7 ,   "lamp 7 ON/OFF"                                   )
MK_STR(  938.0, CALIBL8 ,   "lamp 8 ON/OFF"                                   )
MK_STR(  939.0, CALIBL9 ,   "lamp 9 ON/OFF"                                   )

/*
 * Instrument : 1000.000 to 1999.999
 *
 *TYPE --IDX--  KEYWORD- PRC -------------------COMMENT----------------------*/

MK_CMT( 1000.1, CMTINST1,   ""                                                )
MK_CMT( 1000.2, CMTINST2,   "Instrument Description"                          )
MK_CMT( 1000.3, CMTINST3,   "----------------------"                          )
MK_CMT( 1000.4, CMTINST4,   ""                                                )

/*
 * There are lots of shared keywords among the instruments and spectrographs.
 * Where the keywords are really general purpose (e.g., FILTER) they are
 * separated from the instrument areas.  Where the keywords are more specific,
 * they are associated with the "first" (arbitrarily defined as starting about
 * 1992 :-) instrument that used them.
 */

/* 1010 through 1099 are filter wheel info */

/*
 * Somewhere in the optical train there be filters.  If there is one filter
 * use the FILTER sequence, whether it is actually in the detector or the
 * instrument.  If there are two filters (usual case for IR), use the WHEEL
 * sequence.  For an IR camera on an instrument with a filter, use both.
 */

/*
 * code descriptions for all wheel identifiers
 *
 *  ID - numeric position number of wheel
 *  <null> or DE - text description of filter mounted in that position
 *  LB - lower bound of filter transmission - units Angstrom or microns?
 *  UB - upper bound of filter transmission - units Angstrom or microns?
 *  BW - bandwidth of filter transmission - units Angstrom or microns?
 *  WL - center wave length of filter transmission - units Angstrom or microns?
 */

/* single filter wheel */
MK_INT( 1010.1, FILTERID,   "wheel position"                                  )
MK_STR( 1010.2, FILTER  ,   "description of filter"                           )
MK_FLT( 1010.3, FILTERBW,3, "filter bandwidth"                                )
MK_FLT( 1010.4, FILTERWL,3, "filter wavelength"                               )
MK_FLT( 1010.5, FILTERLB,5, "lower bound of filter (units/%?)"                )
MK_FLT( 1010.6, FILTERUB,5, "upper bound of filter (units/%?)"                )
/* MOS/SIS have coded filter wheels, this is the code for the installed one */
MK_INT( 1010.7, FILTSLID,   "filter drawer number"                            )

/* first filter wheel of two */
MK_INT( 1020.1, WHEELAID,   "'wheel' A position"                              )
MK_STR( 1020.2, WHEELADE,   "description of filter"                           )
MK_FLT( 1020.5, WHEELALB,4, "lower bound of filter A (units/%?)"              )
MK_FLT( 1020.6, WHEELAUB,4, "upper bound of filter A (units/%?)"              )
/* second filter wheel of two */
MK_INT( 1030.1, WHEELBID,   "'wheel' B position"                              )
MK_STR( 1030.2, WHEELBDE,   "description of filter"                           )
MK_FLT( 1030.5, WHEELBLB,4, "lower bound of filter B (units/%?)"              )
MK_FLT( 1030.6, WHEELBUB,4, "upper bound of filter B (units/%?)"              )

/* 1100 through 1199 are general instrument set up values */

/*
 * For instruments that have an internal focus adjustment, this records
 * the current position
 */
MK_FLT( 1101.0, INSTFOC ,7, "internal instrument focus value"                 )


/*
 * Spectrograph : 4000.000 to 4999.999
 *
 *TYPE --IDX--  KEYWORD- PRC -------------------COMMENT----------------------*/

MK_CMT( 4000.1, CMTSPEC1,   ""                                                )
MK_CMT( 4000.2, CMTSPEC2,   "Spectrograph Description"                        )
MK_CMT( 4000.3, CMTSPEC3,   "------------------------"                        )
MK_CMT( 4000.4, CMTSPEC4,   ""                                                )

/* 4100 through 4199 are MOS/SIS */

/*
 * There are a grism wheel and a mask slide on each side.  By analogy with
 * the filter wheel names:
 *  ID     - numeric position number of wheel or slide
 *  <null> - text description of filter mounted in that position
 *  SLID   - the grism wheels and mask slides have codes, this is the
 *           number of the installed wheel or slide
 */
MK_INT( 4101.0, GRISMID ,   "grism position"                                  )
MK_STR( 4102.0, GRISM   ,   "grism description"                               )
MK_INT( 4103.0, GRISSLID,   "grism drawer number"                             )
MK_INT( 4111.0, MASKID  ,   "mask position"                                   )
MK_STR( 4112.0, MASK    ,   "mask description"                                )
MK_INT( 4113.0, MASKSLID,   "mask slider number"                              )

/* 4200 through 4299 are GECKO */

/*
 * Description of the grating installed
 */
MK_STR( 4201.0, DISPEL  ,   "grating description"                             )
/*
 * Dispersion axis used, always 2 now
 */
MK_INT( 4202.0, DISPAXIS,   "dispersion axis (1 or 2)"                        )
/*
 * Incidence angle
 */
MK_FLT( 4203.0, DISPANG ,6, "grating angle in degrees"                        )
/*
 * Resulting center wavelength
 */
MK_FLT( 4204.0, WAVELENG,10,"central wavelength in angstroms"                 )
/*
 * In this order
 */
MK_INT( 4205.0, ORDER   ,   "spectral order"                                  )
/*
 * Magic number
 */
MK_FLT( 4206.0, GRSETUP ,5, "grating setup constant in degrees"               )
/*
 * Another magic number
 */
MK_FLT( 4207.0, OPENANG2,7, "setup const in degrees (opening angle / 2)"      )
/*
 * Why "grism angle" if it's the angle between 2 prisms ???
 */
MK_FLT( 4208.0, GRISMANG,5, "rel.angle between prisms in degrees"             )

/*
 * unimplemented description of slicer used
 */
MK_STR( 4210.0, SLICER  ,   "image slicer in place"                           )

/*
 * OPEN or CLOSED for each hartman mask
 */
MK_STR( 4221.0, HART1A  ,   "hartman mask 1a position OPEN/CLOSED"            )
MK_STR( 4221.1, HART1B  ,   "hartman mask 1b position OPEN/CLOSED"            )
MK_STR( 4222.0, HART2   ,   "hartman mask 2 position OPEN/CLOSED"             )
MK_STR( 4223.0, HART3   ,   "hartman mask 3 position OPEN/CLOSED"             )
MK_STR( 4224.0, HART4   ,   "hartman mask 4 position OPEN/CLOSED"             )

/*
 * unimplemented description of train used
 * could be UV/RED/CAFE
 */
MK_STR( 4230.0, COUDETRN,   "coude train color UV/RED/CAFE"                   )

/*
 * unimplemented exposure meter info
 */
MK_STR( 4240.0, EMFILTER,   "exposure meter filter description"               )
MK_INT( 4241.0, EMCNTS  ,   "exposure meter counts at end"                    )
MK_INT( 4242.0, MIDEXPTM,   "mid-exposure time (seconds)"                     )

/*
 * CAFE mode added an additional card indicating that the Coude train was
 * not being used - this ought to be added to Gecko files with NOTUSED
 * as the value
 */
MK_STR( 4290.0, CAFE    ,   "CAFE is being used Null/INUSE"                   )


/* 4300 through 4399 are for Fabre-Perot etalons */

/*
 * Description of the etalon in use
 */
MK_STR( 4301.0, AUXINST ,   "fabry perot etalon description"                  )
/*
 * Number used to figure something
 */
MK_PFL( 4302.0, CONST   ,2, "fabry perot constant"                            )
/*
 * Number of scan positions
 */
MK_INT( 4303.0, NUMCHAN ,   "number of channels per scan"                     )
/*
 * Current scan position
 */
MK_INT( 4304.0, CURCHAN ,   "current channel"                                 )
/*
 * Actual control value sent to etalon for this position
 */
MK_INT( 4305.0, BINVAL  ,   "binary control value"                            )

/*
 * GriF needed a bunch more values so some of these are duplicates :-(
 */
/*
 * GriF adds a Fabre-Perot etalon between AOB and KIR.  This value indicates
 * the position of the slide holding the etalon.
 */
MK_STR( 4320.0, GRFPPOS ,   "GriF position of FP carriage - In/Out"           )
/*
 * Etalon calibration values, order and standard wavelength, and BCV at
 * standard wavelength
 */
MK_FLT( 4321.1, GRWAVORD,6, "GriF wavelength of order used to scan"           )
MK_FLT( 4321.2, GRWAVCAL,6, "GriF wavelength of calibration spectral line"    )
MK_FLT( 4321.3, GRBCVCAL,6, "GriF BCV at calibration line"                    )
/*
 * This records what the driver for the scan is
 *   bcv      - specific BCV positions given
 *   wave     - a wavelength range given and BCV's calculated
 *   velocity - a range of delta velocities given and BCV's calculated
 */
MK_STR( 4322.0, GRSCANTY,   "GriF type of scan - bcv, wave, velocity"         )
/*
 * Wavelength values for scan, beginning, delta, and current
 */
MK_FLT( 4323.1, GRWAVBEG,6, "GriF wavelength at beginning of scan"            )
MK_FLT( 4323.2, GRWAVSTP,6, "GriF wavelength step"                            )
MK_FLT( 4323.3, GRWAVCUR,6, "GriF current observed wavelength"                )
/*
 * BCV equivalents (well, these are calculated from the desired wavelengths
 * initially, but since the etalon is integral, the current wavelength value
 * above is a conversion back from the BCV position)
 */
MK_FLT( 4324.1, GRBCVBEG,6, "GriF Binary Contol Value at beginning of scan"   )
MK_FLT( 4324.2, GRBCVSTP,6, "GriF BCV step"                                   )
MK_INT( 4324.3, GRBCVCUR,   "GriF applied BCV"                                )
/*
 * The scan itself is a sequence of etalon positions rounded from the
 * desired wavelength range and step.  These are the count and current
 * position in the scan.  (How is GRCURRCH different from GRBCVCUR ??? )
 */
MK_INT( 4325.1, GRNBCHAN,   "GriF number of channels in scan"                 )
MK_INT( 4325.2, GRCURRCH,   "GriF current channel"                            )
/*
 * GriF also controls the plate parallelism.  These record the x and y
 * commands
 */
MK_INT( 4326.1, GRBCV_X ,   "GriF X Binary Control Value FP"                  )
MK_INT( 4326.2, GRBCV_Y ,   "GriF Y Binary Control Value FP"                  )

/*
 * Environment : 9000.000 to 9999.999
 *
 *TYPE --IDX--  KEYWORD- PRC -------------------COMMENT----------------------*/
/*
 * Data logger probe readings.  NOTE: The first two columns of the comment
 * for these make up the authoritative probe numbers.  "loggerh" MUST BE
 * RECOMPILED to have these changes take effect.  It takes whatever is
 * currently in the first two columns of the COMMENT and adds /p/logger/.
 */
MK_PFL( 9101.0, TESPMIRE,2, "03 temp, surface, primary mirror east deg C"     )
MK_PFL( 9102.0, TESPMIRW,2, "02 temp, surface, primary mirror west deg C"     )
MK_PFL( 9103.0, TESPMIRS,2, "01 temp, surface, primary mirror west side degC" )
MK_PFL( 9104.0, TEAPMCLW,2, "54 temp, air, primary mirror cell west deg C"    )
MK_PFL( 9105.0, TEAMIRCI,2, "58 temp, air, mirror cooling in at unit deg C"   )
MK_PFL( 9106.0, TEAMIRCO,2, "27 temp, air, mirror cooling out at cell deg C"  )
MK_PFL( 9107.0, TEAPMSPN,2, "65 temp, air, mirror spigot north cass deg C"    )
MK_PFL( 9108.0, TEAPMSPS,2, "64 temp, air, mirror spigot nouth M3 deg C"      )
MK_PFL( 9109.0, TEATRNGE,2, "23 temp, air, top ring east deg C"               )
MK_PFL( 9110.0, TEATRNGW,2, "06 temp, air, top ring west deg C"               )
MK_PFL( 9111.0, TEANRLSB,2, "19 temp, air, north rail support beam deg C"     )
MK_PFL( 9112.0, TESHRSET,2, "08 temp, surface, horseshoe east top deg C"      )
MK_PFL( 9113.0, TESTELTL,2, "49 temp, surface, telescope truss low deg C"     )
MK_PFL( 9114.0, TESTELTH,2, "52 temp, surface, telescope truss high deg C"    )
MK_PFL( 9115.0, TEALOWWS,2, "38 temp, air, dome lower weather stat side degC" )
MK_PFL( 9116.0, TEATOPWS,2, "36 temp, air, dome top weather stat side deg C"  )
MK_PFL( 9117.0, TEATOPOP,2, "37 temp, air, dome top opposite weath side degC" )
MK_PFL( 9118.0, TEA2INCH,2, "45 temp, air, two inches above fifth floor degC" )
MK_PFL( 9119.0, TEA2INEB,2, "53 temp, air, two inches up by electronics degC" )
MK_PFL( 9120.0, TEA6FOOT,2, "43 temp, air, six feet above fifth floor deg C"  )
MK_PFL( 9121.0, TESCONRM,2, "61 temp, surface, floor above control room degC" )
MK_PFL( 9122.0, TESPIERN,2, "59 temp, surface, floor by north pier deg C"     )
MK_PFL( 9123.0, TESPIERS,2, "60 temp, surface, floor by south pier deg C"     )
MK_PFL( 9124.0, TEAWTHRT,2, "35 temp, air, weathertron deg C"                 )
MK_PFL( 9125.0, TEMPERAT,2, "86 temp, air, weather tower deg C"               )
MK_PFL( 9126.0, WINDSPED,2, "84 wind speed, weather tower knots"              )
MK_PFL( 9127.0, WINDDIR ,2, "85 wind direction, weather tower deg (N=0 E=90)" )
MK_PFL( 9128.0, RELHUMID,2, "87 relative humidity, weather tower %"           )
MK_PFL( 9129.0, PRESSURE,2, "31 barometric pressure, control room mb"         )

/*
 * Queue Scheduled Observing : 11000.000 to 11999.999
 *
 *TYPE --IDX--  KEYWORD- PRC -------------------COMMENT----------------------*/

/*
 * Processing Pipeline : 15000.000 to 15999.999
 *
 *TYPE --IDX--  KEYWORD- PRC -------------------COMMENT----------------------*/

MK_CMT(15000.1, CMTPROC1,   ""                                                )
MK_CMT(15000.2, CMTPROC2,   "Processing Pipeline"                             )
MK_CMT(15000.3, CMTPROC3,   "-------------------"                             )
MK_CMT(15000.4, CMTPROC4,   ""                                                )
MK_STR(15101.0, CRUNID,     "Elixir camera run ID"                            )
/*
 * Archive System : 90000.000 to 99999.999
 *
 *TYPE --IDX--  KEYWORD- PRC -------------------COMMENT----------------------*/

#endif /* !_INCLUDED_fh_registry */
