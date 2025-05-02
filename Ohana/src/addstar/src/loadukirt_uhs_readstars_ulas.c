# include "addstar.h"
# include "ukirt_uhs.h"

/* this function reads the values of interest from the UKIRT UGCS CSV files:

  data model description: doc/mapping_UKIDSS_unWISE_DVO.pdf

  N -- field (column) number in CSV file (1 counting)
  | UKIRT field name          : DVO measure field

  1 sourceID                  : extID
  2 frameSetID		      : imageID (assumes we do not try to go backwards for these photcodes)
  3 ra			      : R
  4 dec	  		      : D
  5 sigRa		      : 
  6 sigDec		      : 
  7 epoch 		      : t ???? -- how is there a single epoch?
  8 muRa 		      : 
  9 muDec		      : 
 10 sigMuRa 		      : 
 11 sigMuDec 		      : 
 12 chi2 		      : 
 13 nFrames 		      : 
 14 cx			      : 
 15 cy			      : 
 16 cz			      : 
 17 htmID		      : 
 18 l			      : 
 19 b			      : 
 20 lambda		      : 
 21 eta			      : 
 22 priOrSec		      : primary bit in dbFlags
 23 ymj_1Pnt 	              :
 24 ymj_1PntErr	              :
 25 j_1mhPnt	              :
 26 j_1mhPntErr	              :
 27 hmkPnt	              :
 28 hmkPntErr	              :
 29 ymj_1Ext 	              :
 30 ymj_1ExtErr	              :
 31 j_1mhExt 	              :
 32 j_1mhExtErr	              :
 33 hmkExt                    :
 34 hmkExtErr                 :
 35 mergedClassStat	      : 
 36 mergedClass		      : psfQFperf or photFlags2
 37 pStar		      : psfChisq
 38 pGalaxy		      : extNsigma
 39 pNoise		      : 
 40 pSaturated		      : 
 41 eBV		              : 
 42 aY		              : 
 43 aJ
 44 aH
 45 aK
 46 yHallMag		      : M
 47 yHallMagErr		      : dM
 48 yPetroMag		      : 
 49 yPetroMagErr	      : 
 50 yAperMag3		      : Map
 51 yAperMag3Err	      : dMap
 52 yAperMag4		      : Mkron
 53 yAperMag4Err	      : dMron
 54 yAperMag6		      : 
 55 yAperMag6Err	      : 
 56 yGausig		      : 
 57 yEll		      : 
 58 yPA			      : posangle
 59 yErrBits		      : 
 60 yDeblend		      : 
 61 yClass		      : psfQF
 62 yClassStat		      : 
 63 yppErrBits		      : photFlags
 64 ySeqNum                   : detID
 65 yObjID		      : 
 66 yXi   		      : 
 67 yEta		      : 
 68 j_1HallMag		      : M
 69 j_1HallMagErr	      : dM
 70 j_1PetroMag		      : 
 71 j_1PetroMagErr	      : 
 72 j_1AperMag3		      : Map
 73 j_1AperMag3Err	      : dMap
 74 j_1AperMag4		      : Mkron
 75 j_1AperMag4Err	      : dMron
 76 j_1AperMag6		      : 
 77 j_1AperMag6Err	      : 
 78 j_1Gausig		      : 
 79 j_1Ell		      : 
 80 j_1PA		      : posangle
 81 j_1ErrBits		      : 
 82 j_1Deblend		      : 
 83 j_1Class		      : psfQF
 84 j_1ClassStat	      : 
 85 j_1ppErrBits	      : photFlags
 86 j_1SeqNum                 : detID
 87 j_1ObjID		      : 
 88 j_1Xi   		      : 
 89 j_1Eta		      : 
 90 j_2HallMag		      : M
 91 j_2HallMagErr	      : dM
 92 j_2PetroMag		      : 
 93 j_2PetroMagErr	      : 
 94 j_2AperMag3		      : Map
 95 j_2AperMag3Err	      : dMap
 96 j_2AperMag4		      : Mkron
 97 j_2AperMag4Err	      : dMron
 98 j_2AperMag6		      : 
 99 j_2AperMag6Err	      : 
100 j_2Gausig		      : 
101 j_2Ell		      : 
102 j_2PA		      : posangle
103 j_2ErrBits		      : 
104 j_2Deblend		      : 
105 j_2Class		      : psfQF
106 j_2ClassStat	      : 
107 j_2ppErrBits	      : photFlags
108 j_2SeqNum                 : detID
109 j_2ObjID		      : 
110 j_2Xi   		      : 
111 j_2Eta		      : 
112 hHallMag		      : M
113 hHallMagErr		      : dM
114 hPetroMag		      : 
115 hPetroMagErr	      : 
116 hAperMag3		      : Map
117 hAperMag3Err	      : dMap
118 hAperMag4		      : Mkron
119 hAperMag4Err	      : dMron
120 hAperMag6		      : 
121 hAperMag6Err	      : 
122 hGausig		      : 
123 hEll		      : 
124 hPA			      : posangle
125 hErrBits		      : 
126 hDeblend		      : 
127 hClass		      : psfQF
128 hClassStat		      : 
129 hppErrBits		      : photFlags
130 hSeqNum                   : detID
131 hObjID		      : 
132 hXi   		      : 
133 hEta		      : 
134 kHallMag		      : M
135 kHallMagErr	              : dM
136 kPetroMag		      : 
137 kPetroMagErr	      : 
138 kAperMag3		      : Map
139 kAperMag3Err	      : dMap
140 kAperMag4		      : Mkron
141 kAperMag4Err	      : dMron
142 kAperMag6		      : 
143 kAperMag6Err	      : 
144 kGausig		      : 
145 kEll		      : 
146 kPA		              : posangle
147 kErrBits		      : 
148 kDeblend		      : 
149 kClass		      : psfQF
150 kClassStat	              : 
151 kppErrBits	              : photFlags
152 kSeqNum                   : detID
153 kObjID		      : 
154 kXi   		      : 
155 kEta		      : 
 
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

UKIRT_Stars *loadukirt_uhs_readstars_ulas (FILE *f, char *buffer, int *nstart, AddstarClientOptions *options, int *nstars) {

  int codeY = GetPhotcodeCodebyName ("UKIRT_Y"); if (!codeY) Shutdown ("missing photcode UKIRT_Y");
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

  ALLOCATE_PTR (yHallMag,     double,   NELEM);
  ALLOCATE_PTR (yHallMagErr,  double,   NELEM);
  ALLOCATE_PTR (yAperMag3,    double,   NELEM);
  ALLOCATE_PTR (yAperMag3Err, double,   NELEM);
  ALLOCATE_PTR (yAperMag4,    double,   NELEM);
  ALLOCATE_PTR (yAperMag4Err, double,   NELEM);
  ALLOCATE_PTR (yPA,          double,   NELEM);
  ALLOCATE_PTR (yClass,       int,      NELEM);
  ALLOCATE_PTR (yppErrBits,   int,      NELEM);
  ALLOCATE_PTR (ySeqNum,      int,      NELEM);

  ALLOCATE_PTR (j1HallMag,     double,   NELEM);
  ALLOCATE_PTR (j1HallMagErr,  double,   NELEM);
  ALLOCATE_PTR (j1AperMag3,    double,   NELEM);
  ALLOCATE_PTR (j1AperMag3Err, double,   NELEM);
  ALLOCATE_PTR (j1AperMag4,    double,   NELEM);
  ALLOCATE_PTR (j1AperMag4Err, double,   NELEM);
  ALLOCATE_PTR (j1PA,          double,   NELEM);
  ALLOCATE_PTR (j1Class,       int,      NELEM);
  ALLOCATE_PTR (j1ppErrBits,   int,      NELEM);
  ALLOCATE_PTR (j1SeqNum,      int,      NELEM);

  ALLOCATE_PTR (j2HallMag,     double,   NELEM);
  ALLOCATE_PTR (j2HallMagErr,  double,   NELEM);
  ALLOCATE_PTR (j2AperMag3,    double,   NELEM);
  ALLOCATE_PTR (j2AperMag3Err, double,   NELEM);
  ALLOCATE_PTR (j2AperMag4,    double,   NELEM);
  ALLOCATE_PTR (j2AperMag4Err, double,   NELEM);
  ALLOCATE_PTR (j2PA,          double,   NELEM);
  ALLOCATE_PTR (j2Class,       int,      NELEM);
  ALLOCATE_PTR (j2ppErrBits,   int,      NELEM);
  ALLOCATE_PTR (j2SeqNum,      int,      NELEM);

  ALLOCATE_PTR (hHallMag,     double,   NELEM);
  ALLOCATE_PTR (hHallMagErr,  double,   NELEM);
  ALLOCATE_PTR (hAperMag3,    double,   NELEM);
  ALLOCATE_PTR (hAperMag3Err, double,   NELEM);
  ALLOCATE_PTR (hAperMag4,    double,   NELEM);
  ALLOCATE_PTR (hAperMag4Err, double,   NELEM);
  ALLOCATE_PTR (hPA,          double,   NELEM);
  ALLOCATE_PTR (hClass,       int,      NELEM);
  ALLOCATE_PTR (hppErrBits,   int,      NELEM);
  ALLOCATE_PTR (hSeqNum,      int,      NELEM);

  ALLOCATE_PTR (kHallMag,     double,   NELEM);
  ALLOCATE_PTR (kHallMagErr,  double,   NELEM);
  ALLOCATE_PTR (kAperMag3,    double,   NELEM);
  ALLOCATE_PTR (kAperMag3Err, double,   NELEM);
  ALLOCATE_PTR (kAperMag4,    double,   NELEM);
  ALLOCATE_PTR (kAperMag4Err, double,   NELEM);
  ALLOCATE_PTR (kPA,          double,   NELEM);
  ALLOCATE_PTR (kClass,       int,      NELEM);
  ALLOCATE_PTR (kppErrBits,   int,      NELEM);
  ALLOCATE_PTR (kSeqNum,      int,      NELEM);

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
      jPARSE (  1,   1, sourceID,      "sourceID");
      iPARSE (  2,   1, frameSetID,    "frameSetID");
      dPARSE (  3,   2, ra,            "ra");
      dPARSE (  4,   3, dec,           "dec");
      dPARSE (  7,   4, epoch,         "epoch");
      jPARSE ( 22,   7, priOrSec,      "priOrSec"); // watch for epoch
      iPARSE ( 36,  22, mergedClass,   "mergedClass");
      dPARSE ( 37,  36, pStar,         "pStar");
      dPARSE ( 38,  37, pGalaxy,       "pGalaxy");
      dPARSE ( 46,  38, yHallMag,      "yHallMag");
      dPARSE ( 47,  46, yHallMagErr,   "yHallMagErr");
      dPARSE ( 50,  47, yAperMag3,     "yAperMag3");
      dPARSE ( 51,  50, yAperMag3Err,  "yAperMag3Err");
      dPARSE ( 52,  51, yAperMag4,     "yAperMag4");
      dPARSE ( 53,  52, yAperMag4Err,  "yAperMag4Err");
      dPARSE ( 58,  53, yPA,           "yPA");
      iPARSE ( 61,  58, yClass,        "yClass");
      iPARSE ( 63,  61, yppErrBits,    "yppErrBits");
      iPARSE ( 64,  63, ySeqNum,       "ySeqNum");
      dPARSE ( 68,  64, j1HallMag,     "j1HallMag");
      dPARSE ( 69,  68, j1HallMagErr,  "j1HallMagErr");
      dPARSE ( 72,  69, j1AperMag3,    "j1AperMag3");
      dPARSE ( 73,  72, j1AperMag3Err, "j1AperMag3Err");
      dPARSE ( 74,  73, j1AperMag4,    "j1AperMag4");
      dPARSE ( 75,  74, j1AperMag4Err, "j1AperMag4Err");
      dPARSE ( 80,  75, j1PA,          "j1PA");
      iPARSE ( 83,  80, j1Class,       "j1Class");
      iPARSE ( 85,  83, j1ppErrBits,   "j1ppErrBits");
      iPARSE ( 86,  85, j1SeqNum,      "j1SeqNum");
      dPARSE ( 90,  86, j2HallMag,     "j2HallMag");
      dPARSE ( 91,  90, j2HallMagErr,  "j2HallMagErr");
      dPARSE ( 94,  91, j2AperMag3,    "j2AperMag3");
      dPARSE ( 95,  94, j2AperMag3Err, "j2AperMag3Err");
      dPARSE ( 96,  95, j2AperMag4,    "j2AperMag4");
      dPARSE ( 97,  96, j2AperMag4Err, "j2AperMag4Err");
      dPARSE (102,  97, j2PA,          "j2PA");
      iPARSE (105, 102, j2Class,       "j2Class");
      iPARSE (107, 105, j2ppErrBits,   "j2ppErrBits");
      iPARSE (108, 107, j2SeqNum,      "j2SeqNum");
      dPARSE (112, 108, hHallMag,      "hHallMag");
      dPARSE (113, 112, hHallMagErr,   "hHallMagErr");
      dPARSE (116, 113, hAperMag3,     "hAperMag3");
      dPARSE (117, 116, hAperMag3Err,  "hAperMag3Err");
      dPARSE (118, 117, hAperMag4,     "hAperMag4");
      dPARSE (119, 118, hAperMag4Err,  "hAperMag4Err");
      dPARSE (124, 119, hPA,           "hPA");
      iPARSE (127, 124, hClass,        "hClass");
      iPARSE (129, 127, hppErrBits,    "hppErrBits");
      iPARSE (130, 129, hSeqNum,       "hSeqNum");
      dPARSE (134, 130, kHallMag,      "kHallMag");
      dPARSE (135, 134, kHallMagErr,   "kHallMagErr");
      dPARSE (138, 135, kAperMag3,     "kAperMag3");
      dPARSE (139, 138, kAperMag3Err,  "kAperMag3Err");
      dPARSE (140, 139, kAperMag4,     "kAperMag4");
      dPARSE (141, 140, kAperMag4Err,  "kAperMag4Err");
      dPARSE (146, 141, kPA,           "kPA");
      iPARSE (149, 146, kClass,        "kClass");
      iPARSE (151, 149, kppErrBits,    "kppErrBits");
      iPARSE (152, 151, kSeqNum,       "kSeqNum");

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

	REALLOCATE (yHallMag,     double,   NELEM);
	REALLOCATE (yHallMagErr,  double,   NELEM);
	REALLOCATE (yAperMag3,    double,   NELEM);
	REALLOCATE (yAperMag3Err, double,   NELEM);
	REALLOCATE (yAperMag4,    double,   NELEM);
	REALLOCATE (yAperMag4Err, double,   NELEM);
	REALLOCATE (yPA,          double,   NELEM);
	REALLOCATE (yClass,       int,      NELEM);
	REALLOCATE (yppErrBits,   int,      NELEM);
	REALLOCATE (ySeqNum,      int,      NELEM);

	REALLOCATE (j1HallMag,     double,   NELEM);
	REALLOCATE (j1HallMagErr,  double,   NELEM);
	REALLOCATE (j1AperMag3,    double,   NELEM);
	REALLOCATE (j1AperMag3Err, double,   NELEM);
	REALLOCATE (j1AperMag4,    double,   NELEM);
	REALLOCATE (j1AperMag4Err, double,   NELEM);
	REALLOCATE (j1PA,          double,   NELEM);
	REALLOCATE (j1Class,       int,      NELEM);
	REALLOCATE (j1ppErrBits,   int,      NELEM);
	REALLOCATE (j1SeqNum,      int,      NELEM);

	REALLOCATE (j2HallMag,     double,   NELEM);
	REALLOCATE (j2HallMagErr,  double,   NELEM);
	REALLOCATE (j2AperMag3,    double,   NELEM);
	REALLOCATE (j2AperMag3Err, double,   NELEM);
	REALLOCATE (j2AperMag4,    double,   NELEM);
	REALLOCATE (j2AperMag4Err, double,   NELEM);
	REALLOCATE (j2PA,          double,   NELEM);
	REALLOCATE (j2Class,       int,      NELEM);
	REALLOCATE (j2ppErrBits,   int,      NELEM);
	REALLOCATE (j2SeqNum,      int,      NELEM);

	REALLOCATE (hHallMag,     double,   NELEM);
	REALLOCATE (hHallMagErr,  double,   NELEM);
	REALLOCATE (hAperMag3,    double,   NELEM);
	REALLOCATE (hAperMag3Err, double,   NELEM);
	REALLOCATE (hAperMag4,    double,   NELEM);
	REALLOCATE (hAperMag4Err, double,   NELEM);
	REALLOCATE (hPA,          double,   NELEM);
	REALLOCATE (hClass,       int,      NELEM);
	REALLOCATE (hppErrBits,   int,      NELEM);
	REALLOCATE (hSeqNum,      int,      NELEM);

	REALLOCATE (kHallMag,     double,   NELEM);
	REALLOCATE (kHallMagErr,  double,   NELEM);
	REALLOCATE (kAperMag3,    double,   NELEM);
	REALLOCATE (kAperMag3Err, double,   NELEM);
	REALLOCATE (kAperMag4,    double,   NELEM);
	REALLOCATE (kAperMag4Err, double,   NELEM);
	REALLOCATE (kPA,          double,   NELEM);
	REALLOCATE (kClass,       int,      NELEM);
	REALLOCATE (kppErrBits,   int,      NELEM);
	REALLOCATE (kSeqNum,      int,      NELEM);
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

    // y measure
    stars[Nstars].measure[0].extID      = sourceID[i];
    stars[Nstars].measure[0].imageID    = frameSetID[i]; // XXX check for frameSetID > 31-bit int
    stars[Nstars].measure[0].R          = ra[i];
    stars[Nstars].measure[0].D          = dec[i];
    stars[Nstars].measure[0].psfChisq   = pStar[i];
    stars[Nstars].measure[0].extNsigma  = pGalaxy[i];
    stars[Nstars].measure[0].M          = yHallMag[i];
    stars[Nstars].measure[0].dM         = yHallMagErr[i];
    stars[Nstars].measure[0].Map        = yAperMag3[i];
    stars[Nstars].measure[0].dMap       = yAperMag3Err[i];
    stars[Nstars].measure[0].Mkron      = yAperMag4[i];
    stars[Nstars].measure[0].dMkron     = yAperMag4Err[i];
    stars[Nstars].measure[0].posangle   = ToShortDegrees(yPA[i]); // XXX units
    stars[Nstars].measure[0].psfQF      = psfQFfromXClass(yClass[i]);
    stars[Nstars].measure[0].photFlags  = yppErrBits[i];
    stars[Nstars].measure[0].photFlags2 = photFlags2;
    stars[Nstars].measure[0].detID      = ySeqNum[i];
    stars[Nstars].measure[0].photcode   = codeY;

    // J_1 measure
    stars[Nstars].measure[1].extID      = sourceID[i];
    stars[Nstars].measure[1].imageID    = frameSetID[i]; // XXX check for frameSetID > 31-bit int
    stars[Nstars].measure[1].R          = ra[i];
    stars[Nstars].measure[1].D          = dec[i];
    stars[Nstars].measure[1].psfChisq   = pStar[i];
    stars[Nstars].measure[1].extNsigma  = pGalaxy[i];
    stars[Nstars].measure[1].M          = j1HallMag[i];
    stars[Nstars].measure[1].dM         = j1HallMagErr[i];
    stars[Nstars].measure[1].Map        = j1AperMag3[i];
    stars[Nstars].measure[1].dMap       = j1AperMag3Err[i];
    stars[Nstars].measure[1].Mkron      = j1AperMag4[i];
    stars[Nstars].measure[1].dMkron     = j1AperMag4Err[i];
    stars[Nstars].measure[1].posangle   = ToShortDegrees(j1PA[i]); // XXX units
    stars[Nstars].measure[1].psfQF      = psfQFfromXClass(j1Class[i]);
    stars[Nstars].measure[1].photFlags  = j1ppErrBits[i];
    stars[Nstars].measure[1].photFlags2 = photFlags2;
    stars[Nstars].measure[1].detID      = j1SeqNum[i];
    stars[Nstars].measure[1].photcode   = codeJ;

    // J measure
    stars[Nstars].measure[2].extID      = sourceID[i];
    stars[Nstars].measure[2].imageID    = frameSetID[i]; // XXX check for frameSetID > 31-bit int
    stars[Nstars].measure[2].R          = ra[i];
    stars[Nstars].measure[2].D          = dec[i];
    stars[Nstars].measure[2].psfChisq   = pStar[i];
    stars[Nstars].measure[2].extNsigma  = pGalaxy[i];
    stars[Nstars].measure[2].M          = j2HallMag[i];
    stars[Nstars].measure[2].dM         = j2HallMagErr[i];
    stars[Nstars].measure[2].Map        = j2AperMag3[i];
    stars[Nstars].measure[2].dMap       = j2AperMag3Err[i];
    stars[Nstars].measure[2].Mkron      = j2AperMag4[i];
    stars[Nstars].measure[2].dMkron     = j2AperMag4Err[i];
    stars[Nstars].measure[2].posangle   = ToShortDegrees(j2PA[i]); // XXX units
    stars[Nstars].measure[2].psfQF      = psfQFfromXClass(j2Class[i]);
    stars[Nstars].measure[2].photFlags  = j2ppErrBits[i];
    stars[Nstars].measure[2].photFlags2 = photFlags2;
    stars[Nstars].measure[2].detID      = j2SeqNum[i];
    stars[Nstars].measure[2].photcode   = codeJ;

    // H measure
    stars[Nstars].measure[3].extID      = sourceID[i];
    stars[Nstars].measure[3].imageID    = frameSetID[i]; // XXX check for frameSetID > 31-bit int
    stars[Nstars].measure[3].R          = ra[i];
    stars[Nstars].measure[3].D          = dec[i];
    stars[Nstars].measure[3].psfChisq   = pStar[i];
    stars[Nstars].measure[3].extNsigma  = pGalaxy[i];
    stars[Nstars].measure[3].M          = hHallMag[i];
    stars[Nstars].measure[3].dM         = hHallMagErr[i];
    stars[Nstars].measure[3].Map        = hAperMag3[i];
    stars[Nstars].measure[3].dMap       = hAperMag3Err[i];
    stars[Nstars].measure[3].Mkron      = hAperMag4[i];
    stars[Nstars].measure[3].dMkron     = hAperMag4Err[i];
    stars[Nstars].measure[3].posangle   = ToShortDegrees(hPA[i]); // XXX units
    stars[Nstars].measure[3].psfQF      = psfQFfromXClass(hClass[i]);
    stars[Nstars].measure[3].photFlags  = hppErrBits[i];
    stars[Nstars].measure[3].photFlags2 = photFlags2;
    stars[Nstars].measure[3].detID      = hSeqNum[i];
    stars[Nstars].measure[3].photcode   = codeH;

    // K measure
    stars[Nstars].measure[4].extID      = sourceID[i];
    stars[Nstars].measure[4].imageID    = frameSetID[i]; // XXX check for frameSetID > 31-bit int
    stars[Nstars].measure[4].R          = ra[i];
    stars[Nstars].measure[4].D          = dec[i];
    stars[Nstars].measure[4].psfChisq   = pStar[i];
    stars[Nstars].measure[4].extNsigma  = pGalaxy[i];
    stars[Nstars].measure[4].M          = kHallMag[i];
    stars[Nstars].measure[4].dM         = kHallMagErr[i];
    stars[Nstars].measure[4].Map        = kAperMag3[i];
    stars[Nstars].measure[4].dMap       = kAperMag3Err[i];
    stars[Nstars].measure[4].Mkron      = kAperMag4[i];
    stars[Nstars].measure[4].dMkron     = kAperMag4Err[i];
    stars[Nstars].measure[4].posangle   = ToShortDegrees(kPA[i]); // XXX units
    stars[Nstars].measure[4].psfQF      = psfQFfromXClass(kClass[i]);
    stars[Nstars].measure[4].photFlags  = kppErrBits[i];
    stars[Nstars].measure[4].photFlags2 = photFlags2;
    stars[Nstars].measure[4].detID      = kSeqNum[i];
    stars[Nstars].measure[4].photcode   = codeK;

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

  FREE (yHallMag);
  FREE (yHallMagErr);
  FREE (yAperMag3);
  FREE (yAperMag3Err);
  FREE (yAperMag4);
  FREE (yAperMag4Err);
  FREE (yPA);
  FREE (yClass);
  FREE (yppErrBits);
  FREE (ySeqNum);

  FREE (j1HallMag);
  FREE (j1HallMagErr);
  FREE (j1AperMag3);
  FREE (j1AperMag3Err);
  FREE (j1AperMag4);
  FREE (j1AperMag4Err);
  FREE (j1PA);
  FREE (j1Class);
  FREE (j1ppErrBits);
  FREE (j1SeqNum);

  FREE (j2HallMag);
  FREE (j2HallMagErr);
  FREE (j2AperMag3);
  FREE (j2AperMag3Err);
  FREE (j2AperMag4);
  FREE (j2AperMag4Err);
  FREE (j2PA);
  FREE (j2Class);
  FREE (j2ppErrBits);
  FREE (j2SeqNum);

  FREE (hHallMag);
  FREE (hHallMagErr);
  FREE (hAperMag3);
  FREE (hAperMag3Err);
  FREE (hAperMag4);
  FREE (hAperMag4Err);
  FREE (hPA);
  FREE (hClass);
  FREE (hppErrBits);
  FREE (hSeqNum);

  FREE (kHallMag);
  FREE (kHallMagErr);
  FREE (kAperMag3);
  FREE (kAperMag3Err);
  FREE (kAperMag4);
  FREE (kAperMag4Err);
  FREE (kPA);
  FREE (kClass);
  FREE (kppErrBits);
  FREE (kSeqNum);

  *nstars = Nstars;
  *nstart = Nstart;
  return (stars);
}

















































































































