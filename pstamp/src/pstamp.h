#ifndef PSTAMP_H
#define PSTAMP_H

// error codes returned to users in results flie
// PS-IPP-PStamp::RequestFile
// These must match the values in the perl module PS-IPP-PStamp::RequestFile
// i.e. PS-IPP-PStamp/lib/PS/IPP/PStamp/RequestFile.pm

typedef enum {
        PSTAMP_SUCCESS          = 0,
	PSTAMP_FIRST_ERROR_CODE = 10,
	PSTAMP_SYSTEM_ERROR     = 10,
	PSTAMP_NOT_IMPLEMENTED  = 11,
	PSTAMP_UNKNOWN_ERROR    = 12,
	PSTAMP_DUP_REQUEST      = 20,
	PSTAMP_INVALID_REQUEST  = 21,
	PSTAMP_UNKNOWN_PROJECT  = 22,
	PSTAMP_NO_IMAGE_MATCH   = 23,
	PSTAMP_NOT_DESTREAKED   = 24,
	PSTAMP_NOT_AVAILABLE    = 25,
	PSTAMP_GONE             = 26,
	PSTAMP_NO_JOBS_QUEUED   = 27,
        PSTAMP_NO_OVERLAP       = 28,
        PSTAMP_NOT_AUTHORIZED   = 29,
        PSTAMP_NO_VALID_PIXELS  = 30,
        PSTAMP_BG_RESTORE_NOT_AVAILABLE = 31,
} pstampJobErrors;


// values for options mask.
#define PSTAMP_SELECT_IMAGE         1
#define PSTAMP_SELECT_MASK          2
#define PSTAMP_SELECT_WEIGHT        4
#define PSTAMP_SELECT_SOURCES       8
#define PSTAMP_SELECT_CMF           8
#define PSTAMP_SELECT_PSF           16
#define PSTAMP_SELECT_BACKMDL       32
#define PSTAMP_SELECT_JPEG          64
#define PSTAMP_SELECT_EXP           128
#define PSTAMP_SELECT_NUM           256
#define PSTAMP_SELECT_UNCOMPRESSED  512
#define PSTAMP_SELECT_INVERSE       1024
#define PSTAMP_SELECT_UNCONV        2048
#define PSTAMP_RESTORE_BACKGROUND   4096
// MEH -- previously unused                           8192
#define PSTAMP_MULTI_OVERLAP_IMAGE  8192
#define PSTAMP_USE_IMFILE_ID        16384

#define PSTAMP_NO_WAIT_FOR_UPDATE   32768
#ifdef notdef
#define PSTAMP_REQUEST_UNCENSORED  0x10000
#define PSTAMP_REQUIRE_UNCENSORED  0x20000
#endif

#define PSTAMP_CENTER_IN_PIXELS 1
#define PSTAMP_RANGE_IN_PIXELS  2

#define STAMP_REQUEST_EXTNAME "PS1_PS_REQUEST"
#define STAMP_REQUEST_VERSION "1"

#define STAMP_RESULTS_EXTNAME "PS1_PS_RESULTS"
#define STAMP_RESULTS_VERSION "1"

// end of values tha must match PS-IPP-PStamp::RequestFile

#endif
