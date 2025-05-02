/*                                             -*- c-file-style: "Ellemtel" -*-

`gpc_detector.h' - Pan-STARRS Giga Pixel Camera detector keywords

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
 * Detector : 100.000 to 199.999
 *
 *TYPE --IDX--  KEYWORD- PRC -------------------COMMENT----------------------*/
MK_CMT(  100.1,	CMTDET1	,   ""                                                )
MK_CMT(  100.2,	CMTDET2	,   "Detector"                                        )
MK_CMT(  100.3,	CMTDET3	,   "--------"                                        )
MK_CMT(  100.4,	CMTDET4	,   ""                                                )
MK_STR(  103.0,	RASTER	,   ""                                                )
MK_INT(  105.0, NAMPS   ,   "Number of amplifiers in the detector"            )
MK_STR(  105.1,	AMPLIST	,   "List of amplifiers for this image"               )
MK_STR(  105.2,	AMPNAME	,   "Amplifier name"                                  )
MK_STR(  150.1, OTADEV  ,   "Identification of device type"                   )
MK_STR(  150.1, CCDNAME,    "Name of the CCD (manufacturer reference)"        )
MK_STR(  150.2, CCDNICK,    "Nickname of the CCD"                             )
MK_INT(  152.0, MAXLIN  ,   "[ADU] Maximum linearity estimate"                )
MK_INT(  152.1, SATURATE,   "[ADU] Saturation value"                          )
MK_FLT(  155.0,	GAIN    ,3, "[e/ADU] est. gain from xray"                     )
MK_FLT(  155.9, XRNOISE, 3, "[e] RMS noise in overscan on xray"               )
MK_FLT(  156.0, RDNOISE ,3, "[e] Read noise"                                  )
MK_FLT(  157.0, DCURRENT,5, "[ADU/pixel/sec] Dark current"                    )
MK_FLT(  157.1, DARKCUR ,5, "[e-/pixel/hour] Dark current"                    )
MK_STR(  158.0,	QEPOINTS,   "QE%@wavelength in nm"                            )
MK_INT(  159.0, BIASLVL,    "[ADU] Bias level (overscan mean)"                )
MK_INT(  159.2, BACKEST,    "[ADU] Background level estimation"               )
MK_STR(  165.0,	DETSTAT	,   "Detector status"                                 )
MK_FLT(  166.0, DETTEM  ,3, "[C] Detector temperature"                        )
MK_CMT(  166.1, CMTDETT1,   "If DETTEMOV is T, DETTEMP was overridden. Old"   )
MK_CMT(  166.2, CMTDETT2,   "temp. is CDETTEM and new temp. was derived as:"  )
MK_CMT(  166.3, CMTDETT3,   "V = (ADU-B) / (A*32768)"                         )
MK_CMT(  166.4, CMTDETT4,   "T = 3369-sqrt[3369^2+1313^2*(13.1*V-1)/(V-1)]"   )
MK_BLN(  166.5, DETTEMOV,   "Detector temperature overridden"                 )
MK_INT(  166.6, DETTEADU,   "[ADU] Detector temperature A/D measurement"      )
MK_PFL(  166.7, DETTCOFA,3, "Coefficient A used in overriding detector temp." )
MK_INT(  166.8, DETTCOFB,   "Coefficient B used in overriding detector temp." )
MK_FLT(  166.9, CDETTEM ,3, "[C] Detector temperature calc. by controller"    )
MK_PFL(  170.0,	PIXSIZE ,3, "[um] Pixel size for both axes"                   )
MK_PFL(  171.1,	PIXSIZE1,3, "[um] Pixel size for axis 1"                      )
MK_PFL(  171.2,	PIXSIZE2,3, "[um] Pixel size for axis 2"                      )
MK_FLT(  172.1,	PIXSCAL1,4, "[arcsec/pixel] Pixel scale for axis 1"           )
MK_FLT(  172.2,	PIXSCAL2,4, "[arcsec/pixel] Pixel scale for axis 2"           )
MK_STR(  176.0, CELLMODE,   " 64 cells: S)cience D)ead V)ideo F)loat"         )
MK_INT(  180.01,IMNAXIS1,   "OTA image width in unbinned pixels"              )
MK_INT(  180.02,IMNAXIS2,   "OTA image height in unbinned pixels"             )
MK_INT(  180.11,IMNPIX1 ,   "OTA image coordinate of amp. (the cell origin)"  )
MK_INT(  180.12,IMNPIX2 ,   "OTA image coordinate of amp. (the cell origin)"  )
MK_CMT(  181.01,CMTCEL1,
         "The following keywords apply equally to each cell, so can be in PHU")
MK_CMT(  181.02,CMTCEL2,
         "If not, they MUST be included in every image extension."            )
MK_PFL(  181.11,IMTM1_1 ,8, "IMage Transform Matrix; A unit vector in the"    )
MK_PFL(  181.12,IMTM1_2 ,8, " serial (fast) direction of amplifier readout"   )
MK_PFL(  181.21,IMTM2_1 ,8, "And a unit vector in parallel (slow) direction"  )
MK_PFL(  181.22,IMTM2_2 ,8, " of amplifier readout"                           )
MK_INT(  184.01,CELLGAP1,   "Cell gap between columns (in unbinned pixels)"   )
MK_INT(  184.02,CELLGAP2,   "Cell gap between rows (in unbinned pixels)"      )
MK_INT(  184.11,CNAXIS1 ,   "Cell imaging size in unbinned pixel columns"     )
MK_INT(  184.12,CNAXIS2 ,   "Cell imaging size in unbinned pixel rows"        )
MK_INT(  184.21,CNPIX1  ,   "Offset in unbinned pixel cols to readout start"  )
MK_INT(  184.22,CNPIX2  ,   "Offset in unbinned pixel rows to readout start"  )
MK_INT(  185.1,	CCDBIN1	,   "Binning factor along axis 1"                     )
MK_INT(  185.2,	CCDBIN2	,   "Binning factor along axis 2"                     )
MK_INT(  186.1, PRESCAN1,   "Prescan count on axis 1"                         )
MK_INT(  186.11,PRESCANX,   "Post-xtrig-bias portion of prescan1"             )
MK_INT(  186.2, PRESCAN2,   "Prescan count on axis 2"                         )
MK_INT(  187.1, OVRSCAN1,   "Overscan count on axis 1"                        )
MK_INT(  187.2, OVRSCAN2,   "Overscan count on axis 2"                        )
MK_CMT(  195.01,CMTIRAF1,   ""                                                )
MK_CMT(  195.02,CMTIRAF2,   "  Iraf coordinates"                              )
MK_CMT(  195.03,CMTIRAF3,   "  ----------------"                              )
MK_STR(  195.1, CCDSUM  ,   "Binning factors"                                 )
MK_STR(  195.2,	DATASEC ,   "Imaging area of the detector"                    )
MK_STR(  195.3,	BIASSEC ,   "Overscan (bias) area"                            )
MK_STR(  195.4,	CCDSIZE ,   "Detector imaging area size"                      )
MK_STR(  195.5, CCDSEC  ,   "Unbinned mapping of read section to CCD"         )
MK_STR(  195.6, DETSIZE	,   "Iraf total image pixels in full mosaic"          )
MK_STR(  195.7,	DETSEC  ,   "Iraf mosaic area of the detector"                )
MK_PFL(  196.01,LTV1    ,8, "Iraf image transformation vector"                )
MK_PFL(  196.02,LTV2    ,8, "Iraf image transformation vector"                )
MK_PFL(  196.11,LTM1_1  ,8, "Iraf image transformation matrix"                )
MK_PFL(  196.12,LTM1_2  ,8, "Iraf image transformation matrix"                )
MK_PFL(  196.21,LTM2_1  ,8, "Iraf image transformation matrix"                )
MK_PFL(  196.22,LTM2_2  ,8, "Iraf image transformation matrix"                )
MK_PFL(  197.01,ATV1    ,8, "Iraf amplifier transformation vector"            )
MK_PFL(  197.02,ATV2    ,8, "Iraf amplifier transformation vector"            )
MK_PFL(  197.11,ATM1_1  ,8, "Iraf amplifier transformation matrix"            )
MK_PFL(  197.12,ATM1_2  ,8, "Iraf amplifier transformation matrix"            )
MK_PFL(  197.21,ATM2_1  ,8, "Iraf amplifier transformation matrix"            )
MK_PFL(  197.22,ATM2_2  ,8, "Iraf amplifier transformation matrix"            )
MK_CMT(  197.96,CMTCON1,    ""                                                )
MK_CMT(  197.97,CMTCON2,    "  Controller Data"                               )
MK_CMT(  197.98,CMTCON3,    "  ---------------"                               )
MK_STR(  198.00,CONTROLR,   "Detector Controller"                             )
MK_STR(  198.01,CONHWURL,   "Controller hardware SVN location"                )
MK_STR(  198.02,CONHWV,     "Controller hardware SVN revision"                )
MK_STR(  198.01,CONSWURL,   "Controller software SVN location"                )
MK_STR(  198.02,CONSWV,     "Controller software SVN revision"                )
MK_STR(  198.03,CON_MAC,    "Controller MAC (HW Ethernet) Address"            )
MK_STR(  198.04,CON_IP,     "Controller IP Address"                           )
MK_STR(  198.05,CON_NAME,   "Controller IP Hostname"                          )
MK_INT(  198.06,CON_DEV,    "Controller device number (0 or 1)"               )
MK_INT(  198.07,CON_UP,     "[seconds] Controller up-time"                    )
MK_STR(  198.08,CON_VSUM,   "Device operating point md5sum"                   )
MK_INT(  198.10,FPGASER,    "Controller FPGA board serial number"             )
MK_STR(  198.11,FPGAVER,    "Controller FPGA board version"                   )
MK_STR(  198.12,FPGATAR,    "Controller FPGA board target system"             )
MK_STR(  198.13,FPGAEC,     "Controller FPGA board engineering changes"       )
MK_INT(  198.20,DAQ3USER,   "Controller DAQ3U board serial number"            )
MK_STR(  198.21,DAQ3UVER,   "Controller DAQ3U board version"                  )
MK_STR(  198.22,DAQ3UTAR,   "Controller DAQ3U board target system"            )
MK_STR(  198.23,DAQ3UEC,    "Controller DAQ3U board engineering changes"      )
MK_INT(  198.20,PREAMSER,   "Controller Preamp board serial number"           )
MK_STR(  198.21,PREAMVER,   "Controller Preamp board version"                 )
MK_STR(  198.22,PREAMTAR,   "Controller Preamp board target system"           )
MK_STR(  198.23,PREAMEC,    "Controller Preamp board engineering changes"     )
MK_INT(  198.30,DAQLBSER,   "DAQ3U loopback test board serial number"         )
MK_STR(  198.31,DAQLBVER,   "DAQ3U loopback test board version"               )
MK_STR(  198.32,DAQLBTAR,   "DAQ3U loopback test board target system"         )
MK_STR(  198.33,DAQLBEC,    "DAQ3U loopback test board engineering changes"   )
MK_INT(  198.40,TSP_SHOP,   "Milliseconds from last clean to shutter open"    )
MK_INT(  198.41,TSP_SHCL,   "Milliseconds from last clean to shutter close"   )
MK_INT(  198.50,TSP_RD00,   "Milliseconds darktime for buffer (cell row) 0"   )
MK_INT(  198.51,TSP_RD01,   "Milliseconds darktime for buffer (cell row) 1"   )
MK_INT(  198.52,TSP_RD02,   "Milliseconds darktime for buffer (cell row) 2"   )
MK_INT(  198.53,TSP_RD03,   "Milliseconds darktime for buffer (cell row) 3"   )
MK_INT(  198.54,TSP_RD04,   "Milliseconds darktime for buffer (cell row) 4"   )
MK_INT(  198.55,TSP_RD05,   "Milliseconds darktime for buffer (cell row) 5"   )
MK_INT(  198.56,TSP_RD06,   "Milliseconds darktime for buffer (cell row) 6"   )
MK_INT(  198.57,TSP_RD07,   "Milliseconds darktime for buffer (cell row) 7"   )
MK_INT(  198.60,TSP_NOW,    "Milliseconds from last clean until save"         )
MK_STR(  199.01,CLV_PPG4,   "Coded parallel timings"                          )
MK_STR(  199.02,CLV_PG3,    "Coded serial timings"                            )
MK_STR(  199.03,CLV_PG4,    "Coded SW/RST/VCLAMP/SAMP"                        )
MK_STR(  199.04,CLV_ADC,    "Coded ADC parameters"                            )
MK_INT(  199.05,CLV_TRIG,   "Cross-trigger phase delay (x 10nsec)"            )
MK_INT(  199.06,CLV_PRES,   "Serial prescan, (skipped in addition to CNPIX1)" )
MK_INT(  199.07,CLV_PIPE,   "Clocking pattern pipeline (in addition to ADC)"  )
MK_INT(  199.071,CLV_PXTP,  "OT Pixel Type 1 or 2 (or 0 for normal CCD)"      )
MK_STR(  199.072,ADC_CONF,  "ADC configuration"                               )
MK_STR(  199.073,ADC_MUX,   "ADC MUX configuration"                           )
MK_STR(  199.074,ADC_PGAR,  "Red ADC pga (adcgain val)"                       )
MK_STR(  199.075,ADC_PGAG,  "Grn ADC pga (adcgain val)"                       )
MK_STR(  199.076,ADC_PGAB,  "Blu ADC pga (adcgain val)"                       )
MK_STR(  199.077,ADC_OFSR,  "Red ADC offset"                                  )
MK_STR(  199.078,ADC_OFSG,  "Grn ADC offset"                                  )
MK_STR(  199.079,ADC_OFSB,  "Blu ADC offset"                                  )
MK_INT(  199.08,DAC_INIT,   "[mV] DAC internal vref offset voltage"           )
MK_PFL(  199.09,DAC_S_L,3,  "[V] Serial and summing well LO"                  )
MK_PFL(  199.10,DAC_S_H,3,  "[V] Serial and summing well HI"                  )
MK_PFL(  199.11,DAC_SW_L,3, "[V] Summing Well LO           "                  )
MK_PFL(  199.12,DAC_SW_H,3, "[V] Summing Well HI           "                  )
MK_PFL(  199.13,DAC_S1_L,3, "[V] Serial 1 LO               "                  )
MK_PFL(  199.14,DAC_S1_H,3, "[V] Serial 1 HI               "                  )
MK_PFL(  199.15,DAC_S2_L,3, "[V] Serial 2 LO               "                  )
MK_PFL(  199.16,DAC_S2_H,3, "[V] Serial 2 HI               "                  )
MK_PFL(  199.17,DAC_S3_L,3, "[V] Serial 3 LO               "                  )
MK_PFL(  199.18,DAC_S3_H,3, "[V] Serial 3 HI               "                  )
MK_PFL(  199.19,DAC_P_L,3,  "[V] Parallel and P. standby LO"                  )
MK_PFL(  199.20,DAC_P_H,3,  "[V] Parallel and P. standby HI"                  )
MK_PFL(  199.21,DAC_P1_L,3, "[V] Parallel 1 LO             "                  )
MK_PFL(  199.22,DAC_P1_H,3, "[V] Parallel 1 HI             "                  )
MK_PFL(  199.23,DAC_P2_L,3, "[V] Parallel 2 LO             "                  )
MK_PFL(  199.24,DAC_P2_H,3, "[V] Parallel 2 HI             "                  )
MK_PFL(  199.25,DAC_P3_L,3, "[V] Parallel 3 LO             "                  )
MK_PFL(  199.26,DAC_P3_H,3, "[V] Parallel 3 HI             "                  )
MK_PFL(  199.27,DAC_P4_L,3, "[V] Parallel 4 LO             "                  )
MK_PFL(  199.28,DAC_P4_H,3, "[V] Parallel 4 HI             "                  )
MK_PFL(  199.29,DAC_PSL,3,  "[V] Parallel Standby LO       "                  )
MK_PFL(  199.30,DAC_PSH,3,  "[V] Parallel Standby HI       "                  )
MK_PFL(  199.31,DAC_RG_L,3, "[V] Reset Gate LO             "                  )
MK_PFL(  199.32,DAC_RG_H,3, "[V] Reset Gate HI             "                  )
MK_PFL(  199.33,DAC_SO,3,   "[V] 1st Stage Source          "                  )
MK_PFL(  199.34,DAC_DR,3,   "[V] Drain Bias                "                  )
MK_PFL(  199.35,DAC_VSS,3,  "[V] Logic Low Supply          "                  )
MK_PFL(  199.36,DAC_VDDL,3, "[V] Logic Drain Supply LO     "                  )
MK_PFL(  199.37,DAC_VDDH,3, "[V] Logic Drain Supply HI     "                  )
MK_PFL(  199.38,DAC_OG1,3,  "[V] Output Gate Bias          "                  )
MK_PFL(  199.39,DAC_RD,3,   "[V] Reset Drain Bias          "                  )
MK_PFL(  199.40,DAC_SUB,3,  "[V] Substrate                 "                  )
MK_PFL(  199.41,DAC_SCP,3,  "[V] Scupper Bias              "                  )
MK_PFL(  199.42,DAC_LREF,3, "[V] Logic Reference Ground    "                  )
MK_STR(  199.70,DAC_CAL0,   "[mV] Pedestal cal"                               )
MK_STR(  199.71,DAC_CAL1,   "[mV] Pedestal cal"                               )
MK_STR(  199.72,DAC_CAL2,   "[mV] Pedestal cal"                               )
MK_STR(  199.73,DAC_CAL3,   "[mV] Pedestal cal"                               )
MK_STR(  199.74,DAC_CAL4,   "[mV] Pedestal cal"                               )
MK_STR(  199.75,DAC_CAL5,   "[mV] Pedestal cal"                               )
MK_STR(  199.76,DAC_CAL6,   "[mV] Pedestal cal"                               )
MK_STR(  199.77,DAC_CAL7,   "[mV] Pedestal cal"                               )
