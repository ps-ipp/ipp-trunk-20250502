# include "addstar.h"
# include "ukirt_uhs.h"

/* this function reads the values of interest from the UKIRT UGCS CSV files:

  data model description: doc/mapping_UKIDSS_unWISE_DVO.pdf

  N -- field (column) number in CSV file (1 counting)
  | UKIRT field name          : DVO measure field
  1 sourceID                  : extID
  2 cuEventID		      : 
  3 frameSetID		      : imageID (assumes we do not try to go backwards for these photcodes)
  4 ra			      : R
  5 dec	  		      : D

  6 sigRa		      : 
  7 sigDec		      : 
  8 epoch 		      : t ???? -- how is there a single epoch?
  9 muRa 		      : 
 10 muDec		      : 
 11 sigMuRa 		      : 
 12 sigMuDec 		      : 
 13 chi2 		      : 
 14 nFrames 		      : 

 15 cx			      : 
 16 cy			      : 
 17 cz			      : 
 18 htmID		      : 
 19 l			      : 
 20 b			      : 
 21 lambda		      : 
 22 eta			      : 
 23 priOrSec		      : primary bit in dbFlags

 24 jmhPnt                    :
 25 jmhPntErr	              :
 26 hmk1Pnt 	              :
 27 hmk1PntErr	              :
 28 h2mk1Pnt	              :
 29 h2mk1PntErr	              :

 30 mergedClassStat	      : 
 31 mergedClass		      : psfQFperf or photFlags2
 32 pStar		      : psfChisq
 33 pGalaxy		      : extNsigma
 34 pNoise		      : 
 35 pSaturated		      : 

 36 jAperMag1		      : M
 37 jAperMag1Err	      : dM
 38 jAperMag3		      : Map
 39 jAperMag3Err	      : dMap
 40 jAperMag4		      : Mkron
 41 jAperMag4Err	      : dMron
 42 jGausig		      : 
 43 jEll		      : 
 44 jPA			      : posangle
 45 jErrBits		      : 
 46 jDeblend		      : 
 47 jClass		      : psfQF
 48 jClassStat		      : 
 49 jppErrBits		      : photFlags
 50 jSeqNum                   : detID
 51 jObjID		      : 
 52 jXi   		      : 
 53 jEta		      : 
 54 hAperMag1		      : M
 55 hAperMag1Err	      : dM
 56 hAperMag3		      : Map
 57 hAperMag3Err	      : dMap
 58 hAperMag4		      : Mkron
 59 hAperMag4Err	      : dMron
 60 hGausig		      : 
 61 hEll		      : 
 62 hPA			      : posangle
 63 hErrBits		      : 
 64 hDeblend		      : 
 65 hClass		      : psfQF
 66 hClassStat		      : 
 67 hppErrBits		      : photFlags
 68 hSeqNum                   : detID
 69 hObjID		      : 
 70 hXi   		      : 
 71 hEta		      : 
 72 k_1AperMag1		      : M
 73 k_1AperMag1Err	      : dM
 74 k_1AperMag3		      : Map
 75 k_1AperMag3Err	      : dMap
 76 k_1AperMag4		      : Mkron
 77 k_1AperMag4Err	      : dMron
 78 k_1Gausig		      : 
 79 k_1Ell		      : 
 80 k_1PA		      : posangle
 81 k_1ErrBits		      : 
 82 k_1Deblend		      : 
 83 k_1Class		      : psfQF
 84 k_1ClassStat	      : 
 85 k_1ppErrBits	      : photFlags
 86 k_1SeqNum                 : detID
 87 k_1ObjID		      : 
 88 k_1Xi   		      : 
 89 k_1Eta		      : 
 90 k_2AperMag1		      : M
 91 k_2AperMag1Err	      : dM
 92 k_2AperMag3		      : Map
 93 k_2AperMag3Err	      : dMap
 94 k_2AperMag4		      : Mkron
 95 k_2AperMag4Err	      : dMron
 96 k_2Gausig		      : 
 97 k_2Ell		      : 
 98 k_2PA		      : posangle
 99 k_2ErrBits		      : 
100 k_2Deblend		      : 
101 k_2Class		      : psfQF
102 k_2ClassStat	      : 
103 k_2ppErrBits	      : photFlags
104 k_2SeqNum                 : detID
105 k_2ObjID		      : 
106 k_2Xi   		      : 
107 k_2Eta		      : 
108 h_2AperMag1		      : M
109 h_2AperMag1Err	      : dM
110 h_2AperMag3		      : Map
111 h_2AperMag3Err	      : dMap
112 h_2AperMag4		      : Mkron
113 h_2AperMag4Err	      : dMron
114 h_2Gausig		      : 
115 h_2Ell		      : 
116 h_2PA		      : posangle
117 h_2ErrBits		      : 
118 h_2Deblend		      : 
119 h_2Class		      : psfQF
120 h_2ClassStat	      : 
121 h_2ppErrBits	      : photFlags
122 h_2SeqNum                 : detID
123 h_2ObjID		      : 
124 h_2Xi   		      : 
125 h_2Eta		      : 

*/

# define iPARSE(NS,NE,FIELD,NAME) {					\
    cA = iparse_csv_rpt (&ivalue, (NS), (NE), cA, &readStatus);		\
    if (!readStatus && VERBOSE) {					\
      gprint (GP_ERR, "suspect field: %d (%s) in %s\n",   (NS), NAME, c0); \
    }									\
    lineStatus &= readStatus; FIELD[Nelem] = ivalue; }

# define jPARSE(NS,NE,FIELD,NAME) {					\
    cA = jparse_csv_rpt (&jvalue, (NS), (NE), cA, &readStatus);		\
    if (!readStatus && VERBOSE) {					\
      gprint (GP_ERR, "suspect field: %d (%s) in %s\n",   (NS), NAME, c0); \
    }									\
    lineStatus &= readStatus; FIELD[Nelem] = jvalue; }

# define dPARSE(NS,NE,FIELD,NAME) {					\
    cA = dparse_csv_rpt (&dvalue, (NS), (NE), cA, &readStatus);		\
    if (!readStatus && VERBOSE) {					\
      gprint (GP_ERR, "suspect field: %d (%s) in %s\n",   (NS), NAME, c0); \
    }									\
    lineStatus &= readStatus; FIELD[Nelem] = dvalue; }

static int Nline_read = 0; // track number of lines read so far (use to skip lines as well)
static int Nskip = 0; // UKIRT UHS CSV files have a single header row (and no special character to mark)

UKIRT_Stars *loadukirt_uhs_readstars_ugps (FILE *f, char *buffer, int *nstart, AddstarClientOptions *options, int *nstars) {

  int codeJ = GetPhotcodeCodebyName ("UKIRT_J"); if (!codeJ) Shutdown ("missing photcode UKIRT_J");
  int codeH = GetPhotcodeCodebyName ("UKIRT_H"); if (!codeH) Shutdown ("missing photcode UKIRT_H");
  int codeK = GetPhotcodeCodebyName ("UKIRT_K"); if (!codeK) Shutdown ("missing photcode UKIRT_K");

  // XXX I need a UKIRT UHS Mean Epoch
  // time_t UKIRT_UHS_EPOCH = ohana_date_to_sec ("2016/01/01,00:00:00");
  // fprintf (stderr, "WARNING: using an invalid UKIRT_UHS_EPOCH (see loadukirt_uhs_readstars.c:60)\n");

  int Nelem = 0;      // number of valid rows read (vector elements)
  int NELEM = 10000;  // currently-allocated number of output rows

  // vectors to hold the data loaded from the csv file (names are UKIRT native names)
  ALLOCATE_PTR (sourceID,     uint64_t, NELEM);
  ALLOCATE_PTR (frameSetID,   uint64_t, NELEM); // XXX doc says 8-bytes for this value!
  ALLOCATE_PTR (ra,           double,   NELEM);
  ALLOCATE_PTR (dec,          double,   NELEM);
  ALLOCATE_PTR (epoch,        double,   NELEM);
  ALLOCATE_PTR (priOrSec,     uint64_t, NELEM);
  ALLOCATE_PTR (mergedClass,  int,      NELEM); // XXX doc says 2-bytes (could use a short)
  ALLOCATE_PTR (pStar,        double,   NELEM);
  ALLOCATE_PTR (pGalaxy,      double,   NELEM);

  ALLOCATE_PTR (jAperMag1,    double,   NELEM);
  ALLOCATE_PTR (jAperMag1Err, double,   NELEM);
  ALLOCATE_PTR (jAperMag3,    double,   NELEM);
  ALLOCATE_PTR (jAperMag3Err, double,   NELEM);
  ALLOCATE_PTR (jAperMag4,    double,   NELEM);
  ALLOCATE_PTR (jAperMag4Err, double,   NELEM);
  ALLOCATE_PTR (jPA,          double,   NELEM);
  ALLOCATE_PTR (jClass,       int,      NELEM);
  ALLOCATE_PTR (jppErrBits,   int,      NELEM);
  ALLOCATE_PTR (jSeqNum,      int,      NELEM);

  ALLOCATE_PTR (hAperMag1,    double,   NELEM);
  ALLOCATE_PTR (hAperMag1Err, double,   NELEM);
  ALLOCATE_PTR (hAperMag3,    double,   NELEM);
  ALLOCATE_PTR (hAperMag3Err, double,   NELEM);
  ALLOCATE_PTR (hAperMag4,    double,   NELEM);
  ALLOCATE_PTR (hAperMag4Err, double,   NELEM);
  ALLOCATE_PTR (hPA,          double,   NELEM);
  ALLOCATE_PTR (hClass,       int,      NELEM);
  ALLOCATE_PTR (hppErrBits,   int,      NELEM);
  ALLOCATE_PTR (hSeqNum,      int,      NELEM);

  ALLOCATE_PTR (k1AperMag1,    double,   NELEM);
  ALLOCATE_PTR (k1AperMag1Err, double,   NELEM);
  ALLOCATE_PTR (k1AperMag3,    double,   NELEM);
  ALLOCATE_PTR (k1AperMag3Err, double,   NELEM);
  ALLOCATE_PTR (k1AperMag4,    double,   NELEM);
  ALLOCATE_PTR (k1AperMag4Err, double,   NELEM);
  ALLOCATE_PTR (k1PA,          double,   NELEM);
  ALLOCATE_PTR (k1Class,       int,      NELEM);
  ALLOCATE_PTR (k1ppErrBits,   int,      NELEM);
  ALLOCATE_PTR (k1SeqNum,      int,      NELEM);

  ALLOCATE_PTR (k2AperMag1,    double,   NELEM);
  ALLOCATE_PTR (k2AperMag1Err, double,   NELEM);
  ALLOCATE_PTR (k2AperMag3,    double,   NELEM);
  ALLOCATE_PTR (k2AperMag3Err, double,   NELEM);
  ALLOCATE_PTR (k2AperMag4,    double,   NELEM);
  ALLOCATE_PTR (k2AperMag4Err, double,   NELEM);
  ALLOCATE_PTR (k2PA,          double,   NELEM);
  ALLOCATE_PTR (k2Class,       int,      NELEM);
  ALLOCATE_PTR (k2ppErrBits,   int,      NELEM);
  ALLOCATE_PTR (k2SeqNum,      int,      NELEM);

  ALLOCATE_PTR (h2AperMag1,    double,   NELEM);
  ALLOCATE_PTR (h2AperMag1Err, double,   NELEM);
  ALLOCATE_PTR (h2AperMag3,    double,   NELEM);
  ALLOCATE_PTR (h2AperMag3Err, double,   NELEM);
  ALLOCATE_PTR (h2AperMag4,    double,   NELEM);
  ALLOCATE_PTR (h2AperMag4Err, double,   NELEM);
  ALLOCATE_PTR (h2PA,          double,   NELEM);
  ALLOCATE_PTR (h2Class,       int,      NELEM);
  ALLOCATE_PTR (h2ppErrBits,   int,      NELEM);
  ALLOCATE_PTR (h2SeqNum,      int,      NELEM);

  // we have a working buffer read from the file. we parse the lines in the working buffer
  // until we reach the last chunk without an EOL char.  at that point, we shift the start
  // of the last (partial) line to the start of the buffer and re-fill.

  // we treat \n\r pair as a single EOL char to handle mac files:

  int Nstart = *nstart; // location of the last valid byte in the buffer (start filling here)
  int EndOfFile = FALSE;
  while (!EndOfFile && (Nelem < NSTARS_MAX)) {
    int Nbytes = BUFFER_SIZE - Nstart;
    // we have allocated one extra byte into which we never read so there will always be a NULL terminating the string
    bzero (&buffer[Nstart], Nbytes + 1);
    int Nread = fread (&buffer[Nstart], 1, Nbytes, f);
    if (ferror (f)) {
      perror ("error reading data file");
      break;
    }

    // we still need to parse the rest of the buffer, but there might not be an EOL on the last line
    if (Nread == 0) {
      EndOfFile = TRUE;
    }
    
    int bufferStatus = TRUE; 
    char *c0 = buffer; // c0 always marks the start of a line
    char *cA = NULL; // cA will carry the curr point within the line
    while (bufferStatus) {

      // find the end of this current line (\n or \r).  if we hit the end of the buffer (NULL),
      // attempt to read more data.  finish up when we hit the end of the file
      char *c1 = strchr (c0, '\n'); // find the end of this current line (also valid for a Mac: \r\n)
      if (!c1) {
	c1 = strchr (c0, '\r'); // try \r for Windows files
      }
      if (!c1) {
	Nstart = strlen (c0);
	if (EndOfFile) {
	  // if we have reached EOF, we need to do one last pass in case there is a line without a return
	  c1 = c0 + Nstart;
	  bufferStatus = FALSE;
	  if (Nstart == 0) continue; // if we have reached EOF and c0 points at the last valid character, we are done
	} else {
	  // if we have not reached EOF, we need to shift the buffer to the start of this line and read more data
	  memmove (buffer, c0, Nstart);
	  bufferStatus = FALSE;
	  continue;
	}
      }
      *c1 = 0; // mark the end of the line 
      Nline_read ++;

      // skip to the next line (but if EOF, do not overrun buffer)
      if (Nline_read <= Nskip) { if (!EndOfFile) { c0 = c1 + 1; } continue; }

      // these are not needed: Gaia CSV files do not have any commented-out lines
      if (*c0 == '#')          { if (!EndOfFile) { c0 = c1 + 1; } continue; }
      if (*c0 == '!')          { if (!EndOfFile) { c0 = c1 + 1; } continue; }

      // for UKIRT UHS, we know which columns we want in advance

      int lineStatus = TRUE;
      int readStatus;
      double dvalue;
      int ivalue;
      uint64_t jvalue;

      // cA will follow the currently extracted field, c0 points to the start of the line
      cA = c0;

      // Tref : TBD for UHS

      // the start of the line is the 1st element (fields are 1-counting)
      jPARSE (  1,   1, sourceID,       "sourceID");
      iPARSE (  3,   1, frameSetID,     "frameSetID");
      dPARSE (  4,   3, ra,             "ra");
      dPARSE (  5,   4, dec,            "dec");
      dPARSE (  8,   5, epoch,          "epoch");
      jPARSE ( 23,   8, priOrSec,       "priOrSec"); // watch for epoch
      iPARSE ( 31,  23, mergedClass,    "mergedClass");
      dPARSE ( 32,  31, pStar,          "pStar");
      dPARSE ( 33,  32, pGalaxy,        "pGalaxy");
      dPARSE ( 36,  33, jAperMag1,      "jAperMag1");
      dPARSE ( 37,  36, jAperMag1Err,   "jAperMag1Err");
      dPARSE ( 38,  37, jAperMag3,      "jAperMag3");
      dPARSE ( 39,  38, jAperMag3Err,   "jAperMag3Err");
      dPARSE ( 40,  39, jAperMag4,      "jAperMag4");
      dPARSE ( 41,  40, jAperMag4Err,   "jAperMag4Err");
      dPARSE ( 44,  41, jPA,            "jPA");
      iPARSE ( 47,  44, jClass,         "jClass");
      iPARSE ( 49,  47, jppErrBits,     "jppErrBits");
      iPARSE ( 50,  49, jSeqNum,        "jSeqNum");
      dPARSE ( 54,  50, hAperMag1,      "hAperMag1");
      dPARSE ( 55,  54, hAperMag1Err,   "hAperMag1Err");
      dPARSE ( 56,  55, hAperMag3,      "hAperMag3");
      dPARSE ( 57,  56, hAperMag3Err,   "hAperMag3Err");
      dPARSE ( 58,  57, hAperMag4,      "hAperMag4");
      dPARSE ( 59,  58, hAperMag4Err,   "hAperMag4Err");
      dPARSE ( 62,  59, hPA,            "hPA");
      iPARSE ( 65,  62, hClass,         "hClass");
      iPARSE ( 67,  65, hppErrBits,     "hppErrBits");
      iPARSE ( 68,  67, hSeqNum,        "hSeqNum");
      dPARSE ( 72,  68, k1AperMag1,     "k1AperMag1");
      dPARSE ( 73,  72, k1AperMag1Err,  "k1AperMag1Err");
      dPARSE ( 74,  73, k1AperMag3,     "k1AperMag3");
      dPARSE ( 75,  74, k1AperMag3Err,  "k1AperMag3Err");
      dPARSE ( 76,  75, k1AperMag4,     "k1AperMag4");
      dPARSE ( 77,  76, k1AperMag4Err,  "k1AperMag4Err");
      dPARSE ( 80,  77, k1PA,           "k1PA");
      iPARSE ( 83,  80, k1Class,        "k1Class");
      iPARSE ( 85,  83, k1ppErrBits,    "k1ppErrBits");
      iPARSE ( 86,  85, k1SeqNum,       "k1SeqNum");
      dPARSE ( 90,  86, k2AperMag1,     "k2AperMag1");
      dPARSE ( 91,  90, k2AperMag1Err,  "k2AperMag1Err");
      dPARSE ( 92,  91, k2AperMag3,     "k2AperMag3");
      dPARSE ( 93,  92, k2AperMag3Err,  "k2AperMag3Err");
      dPARSE ( 94,  93, k2AperMag4,     "k2AperMag4");
      dPARSE ( 95,  94, k2AperMag4Err,  "k2AperMag4Err");
      dPARSE ( 98,  95, k2PA,           "k2PA");
      iPARSE (101,  98, k2Class,        "k2Class");
      iPARSE (103, 101, k2ppErrBits,    "k2ppErrBits");
      iPARSE (104, 103, k2SeqNum,       "k2SeqNum");
      dPARSE (108, 104, h2AperMag1,     "hAperMag1");
      dPARSE (109, 108, h2AperMag1Err,  "hAperMag1Err");
      dPARSE (110, 109, h2AperMag3,     "hAperMag3");
      dPARSE (111, 110, h2AperMag3Err,  "hAperMag3Err");
      dPARSE (112, 111, h2AperMag4,     "hAperMag4");
      dPARSE (113, 112, h2AperMag4Err,  "hAperMag4Err");
      dPARSE (116, 113, h2PA,           "hPA");
      iPARSE (119, 116, h2Class,        "hClass");
      iPARSE (121, 119, h2ppErrBits,    "hppErrBits");
      iPARSE (122, 121, h2SeqNum,       "hSeqNum");

      if (!lineStatus && VERBOSE) {
	// why do I need to copy temp here, does gprint modify the value of temp?
	char temp[32];
	strncpy_nowarn (temp, c0, 31);
	gprint (GP_ERR, "skip line %s\n\n", temp);
      }

      Nelem ++;
      if (Nelem == NELEM) {
	NELEM += 1000;

	REALLOCATE (sourceID,     uint64_t, NELEM);    
	REALLOCATE (frameSetID,   uint64_t, NELEM);
	REALLOCATE (ra,           double,   NELEM);
	REALLOCATE (dec,          double,   NELEM);
	REALLOCATE (epoch,        double,   NELEM);
	REALLOCATE (priOrSec,     uint64_t, NELEM);
	REALLOCATE (mergedClass,  int,      NELEM);
	REALLOCATE (pStar,        double,   NELEM);
	REALLOCATE (pGalaxy,      double,   NELEM);

	REALLOCATE (jAperMag1,     double,   NELEM);
	REALLOCATE (jAperMag1Err,  double,   NELEM);
	REALLOCATE (jAperMag3,    double,   NELEM);
	REALLOCATE (jAperMag3Err, double,   NELEM);
	REALLOCATE (jAperMag4,    double,   NELEM);
	REALLOCATE (jAperMag4Err, double,   NELEM);
	REALLOCATE (jPA,          double,   NELEM);
	REALLOCATE (jClass,       int,      NELEM);
	REALLOCATE (jppErrBits,   int,      NELEM);
	REALLOCATE (jSeqNum,      int,      NELEM);

	REALLOCATE (hAperMag1,     double,   NELEM);
	REALLOCATE (hAperMag1Err,  double,   NELEM);
	REALLOCATE (hAperMag3,    double,   NELEM);
	REALLOCATE (hAperMag3Err, double,   NELEM);
	REALLOCATE (hAperMag4,    double,   NELEM);
	REALLOCATE (hAperMag4Err, double,   NELEM);
	REALLOCATE (hPA,          double,   NELEM);
	REALLOCATE (hClass,       int,      NELEM);
	REALLOCATE (hppErrBits,   int,      NELEM);
	REALLOCATE (hSeqNum,      int,      NELEM);

	REALLOCATE (k1AperMag1,     double,   NELEM);
	REALLOCATE (k1AperMag1Err,  double,   NELEM);
	REALLOCATE (k1AperMag3,    double,   NELEM);
	REALLOCATE (k1AperMag3Err, double,   NELEM);
	REALLOCATE (k1AperMag4,    double,   NELEM);
	REALLOCATE (k1AperMag4Err, double,   NELEM);
	REALLOCATE (k1PA,          double,   NELEM);
	REALLOCATE (k1Class,       int,      NELEM);
	REALLOCATE (k1ppErrBits,   int,      NELEM);
	REALLOCATE (k1SeqNum,      int,      NELEM);

	REALLOCATE (k2AperMag1,     double,   NELEM);
	REALLOCATE (k2AperMag1Err,  double,   NELEM);
	REALLOCATE (k2AperMag3,    double,   NELEM);
	REALLOCATE (k2AperMag3Err, double,   NELEM);
	REALLOCATE (k2AperMag4,    double,   NELEM);
	REALLOCATE (k2AperMag4Err, double,   NELEM);
	REALLOCATE (k2PA,          double,   NELEM);
	REALLOCATE (k2Class,       int,      NELEM);
	REALLOCATE (k2ppErrBits,   int,      NELEM);
	REALLOCATE (k2SeqNum,      int,      NELEM);

	REALLOCATE (h2AperMag1,     double,   NELEM);
	REALLOCATE (h2AperMag1Err,  double,   NELEM);
	REALLOCATE (h2AperMag3,    double,   NELEM);
	REALLOCATE (h2AperMag3Err, double,   NELEM);
	REALLOCATE (h2AperMag4,    double,   NELEM);
	REALLOCATE (h2AperMag4Err, double,   NELEM);
	REALLOCATE (h2PA,          double,   NELEM);
	REALLOCATE (h2Class,       int,      NELEM);
	REALLOCATE (h2ppErrBits,   int,      NELEM);
	REALLOCATE (h2SeqNum,      int,      NELEM);
      }
      if (!EndOfFile) {
	c0 = c1 + 1;
      }
    }
  }
  fprintf (stderr, " DONE: Nelem: %d, *nstars: %d\n", Nelem, *nstars);

  // Nelem is now the number of items (objects,stars) read from the Gaia CSV file
  int NstarsIn = Nelem;

  double Rmin = +360.0;
  double Rmax = -360.0;
  double Dmin = +360.0;
  double Dmax = -360.0;

  // start off where we finished on a previous read
  int Nstars = *nstars;
  int NSTARS = Nstars + 0.1*NstarsIn;

  ALLOCATE_PTR (stars, UKIRT_Stars, NSTARS);

  for (int i = 0; i < NstarsIn; i++) {

    Rmin = MIN (Rmin, ra[i]);
    Rmax = MAX (Rmax, ra[i]);
    Dmin = MIN (Dmin, dec[i]);
    Dmax = MAX (Dmax, dec[i]);

    // only allocate the measures for stars as we define them
    ALLOCATE (stars[Nstars].measure, Measure, UKIRT_NFILTER);

    // we have NFILTER (== 6) UKIRT measurements per object
    dvo_average_init (&stars[Nstars].average);
    for (int j = 0; j < UKIRT_NFILTER; j++) {
      dvo_measure_init (&stars[Nstars].measure[j]);
    }

    stars[Nstars].average.R = ra[i];
    stars[Nstars].average.D = dec[i];
    stars[Nstars].average.dR = NAN;
    stars[Nstars].average.dD = NAN;

    stars[Nstars].flag  = FALSE;
    stars[Nstars].found = FALSE;

    int isPrimary   = (!priOrSec[i] || (priOrSec[i] == frameSetID[i])) ? ID_MEAS_STACK_PRIMARY : 0x00;
    int photFlags2 = isPrimary;
    switch (mergedClass[i]) {
      case  1: photFlags2 |= 0x01; break; // Galaxy
      case  0: photFlags2 |= 0x02; break; // Noise
      case -1: photFlags2 |= 0x04; break; // Star
      case -2: photFlags2 |= 0x08; break; // probably star
      case -3: photFlags2 |= 0x10; break; // probably galaxy
      case -9: photFlags2 |= 0x20; break; // saturated
      default: break;
    }

    // J measure
    stars[Nstars].measure[0].extID      = sourceID[i];
    stars[Nstars].measure[0].imageID    = frameSetID[i]; // XXX check for frameSetID > 31-bit int
    stars[Nstars].measure[0].R          = ra[i];
    stars[Nstars].measure[0].D          = dec[i];
    stars[Nstars].measure[0].psfChisq   = pStar[i];
    stars[Nstars].measure[0].extNsigma  = pGalaxy[i];
    stars[Nstars].measure[0].M          = jAperMag1[i];
    stars[Nstars].measure[0].dM         = jAperMag1Err[i];
    stars[Nstars].measure[0].Map        = jAperMag3[i];
    stars[Nstars].measure[0].dMap       = jAperMag3Err[i];
    stars[Nstars].measure[0].Mkron      = jAperMag4[i];
    stars[Nstars].measure[0].dMkron     = jAperMag4Err[i];
    stars[Nstars].measure[0].posangle   = ToShortDegrees(jPA[i]); // XXX units
    stars[Nstars].measure[0].psfQF      = psfQFfromXClass(jClass[i]);
    stars[Nstars].measure[0].photFlags  = jppErrBits[i];
    stars[Nstars].measure[0].photFlags2 = photFlags2;
    stars[Nstars].measure[0].detID      = jSeqNum[i];
    stars[Nstars].measure[0].photcode   = codeJ;

    // H measure
    stars[Nstars].measure[1].extID      = sourceID[i];
    stars[Nstars].measure[1].imageID    = frameSetID[i]; // XXX check for frameSetID > 31-bit int
    stars[Nstars].measure[1].R          = ra[i];
    stars[Nstars].measure[1].D          = dec[i];
    stars[Nstars].measure[1].psfChisq   = pStar[i];
    stars[Nstars].measure[1].extNsigma  = pGalaxy[i];
    stars[Nstars].measure[1].M          = hAperMag1[i];
    stars[Nstars].measure[1].dM         = hAperMag1Err[i];
    stars[Nstars].measure[1].Map        = hAperMag3[i];
    stars[Nstars].measure[1].dMap       = hAperMag3Err[i];
    stars[Nstars].measure[1].Mkron      = hAperMag4[i];
    stars[Nstars].measure[1].dMkron     = hAperMag4Err[i];
    stars[Nstars].measure[1].posangle   = ToShortDegrees(hPA[i]); // XXX units
    stars[Nstars].measure[1].psfQF      = psfQFfromXClass(hClass[i]);
    stars[Nstars].measure[1].photFlags  = hppErrBits[i];
    stars[Nstars].measure[1].photFlags2 = photFlags2;
    stars[Nstars].measure[1].detID      = hSeqNum[i];
    stars[Nstars].measure[1].photcode   = codeH;

    // K1 measure
    stars[Nstars].measure[2].extID      = sourceID[i];
    stars[Nstars].measure[2].imageID    = frameSetID[i]; // XXX check for frameSetID > 31-bit int
    stars[Nstars].measure[2].R          = ra[i];
    stars[Nstars].measure[2].D          = dec[i];
    stars[Nstars].measure[2].psfChisq   = pStar[i];
    stars[Nstars].measure[2].extNsigma  = pGalaxy[i];
    stars[Nstars].measure[2].M          = k1AperMag1[i];
    stars[Nstars].measure[2].dM         = k1AperMag1Err[i];
    stars[Nstars].measure[2].Map        = k1AperMag3[i];
    stars[Nstars].measure[2].dMap       = k1AperMag3Err[i];
    stars[Nstars].measure[2].Mkron      = k1AperMag4[i];
    stars[Nstars].measure[2].dMkron     = k1AperMag4Err[i];
    stars[Nstars].measure[2].posangle   = ToShortDegrees(k1PA[i]); // XXX units
    stars[Nstars].measure[2].psfQF      = psfQFfromXClass(k1Class[i]);
    stars[Nstars].measure[2].photFlags  = k1ppErrBits[i];
    stars[Nstars].measure[2].photFlags2 = photFlags2;
    stars[Nstars].measure[2].detID      = k1SeqNum[i];
    stars[Nstars].measure[2].photcode   = codeK;

    // K1 measure
    stars[Nstars].measure[3].extID      = sourceID[i];
    stars[Nstars].measure[3].imageID    = frameSetID[i]; // XXX check for frameSetID > 31-bit int
    stars[Nstars].measure[3].R          = ra[i];
    stars[Nstars].measure[3].D          = dec[i];
    stars[Nstars].measure[3].psfChisq   = pStar[i];
    stars[Nstars].measure[3].extNsigma  = pGalaxy[i];
    stars[Nstars].measure[3].M          = k2AperMag1[i];
    stars[Nstars].measure[3].dM         = k2AperMag1Err[i];
    stars[Nstars].measure[3].Map        = k2AperMag3[i];
    stars[Nstars].measure[3].dMap       = k2AperMag3Err[i];
    stars[Nstars].measure[3].Mkron      = k2AperMag4[i];
    stars[Nstars].measure[3].dMkron     = k2AperMag4Err[i];
    stars[Nstars].measure[3].posangle   = ToShortDegrees(k2PA[i]); // XXX units
    stars[Nstars].measure[3].psfQF      = psfQFfromXClass(k2Class[i]);
    stars[Nstars].measure[3].photFlags  = k2ppErrBits[i];
    stars[Nstars].measure[3].photFlags2 = photFlags2;
    stars[Nstars].measure[3].detID      = k2SeqNum[i];
    stars[Nstars].measure[3].photcode   = codeK;

    // H2 measure
    stars[Nstars].measure[4].extID      = sourceID[i];
    stars[Nstars].measure[4].imageID    = frameSetID[i]; // XXX check for frameSetID > 31-bit int
    stars[Nstars].measure[4].R          = ra[i];
    stars[Nstars].measure[4].D          = dec[i];
    stars[Nstars].measure[4].psfChisq   = pStar[i];
    stars[Nstars].measure[4].extNsigma  = pGalaxy[i];
    stars[Nstars].measure[4].M          = h2AperMag1[i];
    stars[Nstars].measure[4].dM         = h2AperMag1Err[i];
    stars[Nstars].measure[4].Map        = h2AperMag3[i];
    stars[Nstars].measure[4].dMap       = h2AperMag3Err[i];
    stars[Nstars].measure[4].Mkron      = h2AperMag4[i];
    stars[Nstars].measure[4].dMkron     = h2AperMag4Err[i];
    stars[Nstars].measure[4].posangle   = ToShortDegrees(h2PA[i]); // XXX units
    stars[Nstars].measure[4].psfQF      = psfQFfromXClass(h2Class[i]);
    stars[Nstars].measure[4].photFlags  = h2ppErrBits[i];
    stars[Nstars].measure[4].photFlags2 = photFlags2;
    stars[Nstars].measure[4].detID      = h2SeqNum[i];
    stars[Nstars].measure[4].photcode   = codeH;

    stars[Nstars].average.Nmeasure = UKIRT_NFILTER;
    Nstars ++;

    if (Nstars >= NSTARS) {
      NSTARS += 10000;
      REALLOCATE (stars, UKIRT_Stars, NSTARS);
    }
  }

  FREE (sourceID);
  FREE (frameSetID);
  FREE (ra);
  FREE (dec);
  FREE (epoch);
  FREE (priOrSec);
  FREE (mergedClass);
  FREE (pStar);
  FREE (pGalaxy);

  FREE (jAperMag1);
  FREE (jAperMag1Err);
  FREE (jAperMag3);
  FREE (jAperMag3Err);
  FREE (jAperMag4);
  FREE (jAperMag4Err);
  FREE (jPA);
  FREE (jClass);
  FREE (jppErrBits);
  FREE (jSeqNum);

  FREE (hAperMag1);
  FREE (hAperMag1Err);
  FREE (hAperMag3);
  FREE (hAperMag3Err);
  FREE (hAperMag4);
  FREE (hAperMag4Err);
  FREE (hPA);
  FREE (hClass);
  FREE (hppErrBits);
  FREE (hSeqNum);

  FREE (k1AperMag1);
  FREE (k1AperMag1Err);
  FREE (k1AperMag3);
  FREE (k1AperMag3Err);
  FREE (k1AperMag4);
  FREE (k1AperMag4Err);
  FREE (k1PA);
  FREE (k1Class);
  FREE (k1ppErrBits);
  FREE (k1SeqNum);

  FREE (k2AperMag1);
  FREE (k2AperMag1Err);
  FREE (k2AperMag3);
  FREE (k2AperMag3Err);
  FREE (k2AperMag4);
  FREE (k2AperMag4Err);
  FREE (k2PA);
  FREE (k2Class);
  FREE (k2ppErrBits);
  FREE (k2SeqNum);

  FREE (h2AperMag1);
  FREE (h2AperMag1Err);
  FREE (h2AperMag3);
  FREE (h2AperMag3Err);
  FREE (h2AperMag4);
  FREE (h2AperMag4Err);
  FREE (h2PA);
  FREE (h2Class);
  FREE (h2ppErrBits);
  FREE (h2SeqNum);

  *nstars = Nstars;
  *nstart = Nstart;
  return (stars);
}
