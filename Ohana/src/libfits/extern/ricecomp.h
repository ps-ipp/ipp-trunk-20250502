

// 2008.06.22 EAM : This code was taken from CFITSIO and included in Ohana for rice
// decompression.  In order to include this .c file (or future upgrades), we include our own
// version of ricecomp.h (not the one available in the CFITSIO tree) and include it instead of
// fitsio2.h.  This file defines our own version of ffpmsg.

#define ffpmsg(MSG) fprintf(stderr, "%s\n", MSG)
