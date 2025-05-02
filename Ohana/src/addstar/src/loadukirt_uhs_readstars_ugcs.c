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
 24 zmyPnt                    :
 25 zmyPntErr	              :
 26 ymjPnt 	              :
 27 ymjPntErr	              :
 28 jmhPnt	              :
 29 jmhPntErr	              :
 30 hmk_1Pnt	              :
 31 hmk_1PntErr	              :
 32 zmyExt 	              :
 33 zmyExtErr	              :
 34 ymjExt 	              :
 35 ymjExtErr	              :
 36 jmhExt 	              :
 37 jmhExtErr	              :
 38 hmk_1Ext                  :
 39 hmk_1ExtErr               :

 40 mergedClassStat	      : 
 41 mergedClass		      : psfQFperf or photFlags2
 42 pStar		      : psfChisq
 43 pGalaxy		      : extNsigma
 44 pNoise		      : 
 45 pSaturated		      : 

 46 zHallMag		      : M
 47 zHallMagErr		      : dM
 48 zPetroMag		      : 
 49 zPetroMagErr	      : 
 50 zAperMag3		      : Map
 51 zAperMag3Err	      : dMap
 52 zAperMag4		      : Mkron
 53 zAperMag4Err	      : dMron
 54 zAperMag6		      : 
 55 zAperMag6Err	      : 
 56 zGausig		      : 
 57 zEll		      : 
 58 zPA			      : posangle
 59 zErrBits		      : 
 60 zDeblend		      : 
 61 zClass		      : psfQF
 62 zClassStat		      : 
 63 zppErrBits		      : photFlags
 64 zSeqNum                   : detID
 65 zObjID		      : 
 66 zXi   		      : 
 67 zEta		      : 

 68 yHallMag		      : M
 69 yHallMagErr		      : dM
 70 yPetroMag		      : 
 71 yPetroMagErr	      : 
 72 yAperMag3		      : Map
 73 yAperMag3Err	      : dMap
 74 yAperMag4		      : Mkron
 75 yAperMag4Err	      : dMron
 76 yAperMag6		      : 
 77 yAperMag6Err	      : 
 78 yGausig		      : 
 79 yEll		      : 
 80 yPA			      : posangle
 81 yErrBits		      : 
 82 yDeblend		      : 
 83 yClass		      : psfQF
 84 yClassStat		      : 
 85 yppErrBits		      : photFlags
 86 ySeqNum                   : detID
 87 yObjID		      : 
 88 yXi   		      : 
 89 yEta		      : 

 90 jHallMag		      : M
 91 jHallMagErr		      : dM
 92 jPetroMag		      : 
 93 jPetroMagErr	      : 
 94 jAperMag3		      : Map
 95 jAperMag3Err	      : dMap
 96 jAperMag4		      : Mkron
 97 jAperMag4Err	      : dMron
 98 jAperMag6		      : 
 99 jAperMag6Err	      : 
100 jGausig		      : 
101 jEll		      : 
102 jPA			      : posangle
103 jErrBits		      : 
104 jDeblend		      : 
105 jClass		      : psfQF
106 jClassStat		      : 
107 jppErrBits		      : photFlags
108 jSeqNum                   : detID
109 jObjID		      : 
110 jXi   		      : 
111 jEta		      : 

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

134 k_1HallMag		      : M
135 k_1HallMagErr	      : dM
136 k_1PetroMag		      : 
137 k_1PetroMagErr	      : 
138 k_1AperMag3		      : Map
139 k_1AperMag3Err	      : dMap
140 k_1AperMag4		      : Mkron
141 k_1AperMag4Err	      : dMron
142 k_1AperMag6		      : 
143 k_1AperMag6Err	      : 
144 k_1Gausig		      : 
145 k_1Ell		      : 
146 k_1PA		      : posangle
147 k_1ErrBits		      : 
148 k_1Deblend		      : 
149 k_1Class		      : psfQF
150 k_1ClassStat	      : 
151 k_1ppErrBits	      : photFlags
152 k_1SeqNum                 : detID
153 k_1ObjID		      : 
154 k_1Xi   		      : 
155 k_1Eta		      : 

156 k_2HallMag		      : M
157 k_2HallMagErr	      : dM
158 k_2PetroMag		      : 
159 k_2PetroMagErr	      : 
160 k_2AperMag3		      : Map
161 k_2AperMag3Err	      : dMap
162 k_2AperMag4		      : Mkron
163 k_2AperMag4Err	      : dMron
164 k_2AperMag6		      : 
165 k_2AperMag6Err	      : 
166 k_2Gausig		      : 
167 k_2Ell		      : 
168 k_2PA		      : posangle
169 k_2ErrBits		      : 
170 k_2Deblend		      : 
171 k_2Class		      : psfQF
172 k_2ClassStat	      : 
173 k_2ppErrBits	      : photFlags
174 k_2SeqNum                 : detID
175 k_2ObjID		      : 
176 k_2Xi   		      : 
177 k_2Eta		      : 

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

UKIRT_Stars *loadukirt_uhs_readstars_ugcs (FILE *f, char *buffer, int *nstart, AddstarClientOptions *options, int *nstars) {

  int codeZ = GetPhotcodeCodebyName ("UKIRT_Z"); if (!codeZ) Shutdown ("missing photcode UKIRT_Z");
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

  ALLOCATE_PTR (zHallMag,     double,   NELEM);
  ALLOCATE_PTR (zHallMagErr,  double,   NELEM);
  ALLOCATE_PTR (zAperMag3,    double,   NELEM);
  ALLOCATE_PTR (zAperMag3Err, double,   NELEM);
  ALLOCATE_PTR (zAperMag4,    double,   NELEM);
  ALLOCATE_PTR (zAperMag4Err, double,   NELEM);
  ALLOCATE_PTR (zPA,          double,   NELEM);
  ALLOCATE_PTR (zClass,       int,      NELEM);
  ALLOCATE_PTR (zppErrBits,   int,      NELEM);
  ALLOCATE_PTR (zSeqNum,      int,      NELEM);

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

  ALLOCATE_PTR (jHallMag,     double,   NELEM);
  ALLOCATE_PTR (jHallMagErr,  double,   NELEM);
  ALLOCATE_PTR (jAperMag3,    double,   NELEM);
  ALLOCATE_PTR (jAperMag3Err, double,   NELEM);
  ALLOCATE_PTR (jAperMag4,    double,   NELEM);
  ALLOCATE_PTR (jAperMag4Err, double,   NELEM);
  ALLOCATE_PTR (jPA,          double,   NELEM);
  ALLOCATE_PTR (jClass,       int,      NELEM);
  ALLOCATE_PTR (jppErrBits,   int,      NELEM);
  ALLOCATE_PTR (jSeqNum,      int,      NELEM);

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

  ALLOCATE_PTR (k1HallMag,     double,   NELEM);
  ALLOCATE_PTR (k1HallMagErr,  double,   NELEM);
  ALLOCATE_PTR (k1AperMag3,    double,   NELEM);
  ALLOCATE_PTR (k1AperMag3Err, double,   NELEM);
  ALLOCATE_PTR (k1AperMag4,    double,   NELEM);
  ALLOCATE_PTR (k1AperMag4Err, double,   NELEM);
  ALLOCATE_PTR (k1PA,          double,   NELEM);
  ALLOCATE_PTR (k1Class,       int,      NELEM);
  ALLOCATE_PTR (k1ppErrBits,   int,      NELEM);
  ALLOCATE_PTR (k1SeqNum,      int,      NELEM);

  ALLOCATE_PTR (k2HallMag,     double,   NELEM);
  ALLOCATE_PTR (k2HallMagErr,  double,   NELEM);
  ALLOCATE_PTR (k2AperMag3,    double,   NELEM);
  ALLOCATE_PTR (k2AperMag3Err, double,   NELEM);
  ALLOCATE_PTR (k2AperMag4,    double,   NELEM);
  ALLOCATE_PTR (k2AperMag4Err, double,   NELEM);
  ALLOCATE_PTR (k2PA,          double,   NELEM);
  ALLOCATE_PTR (k2Class,       int,      NELEM);
  ALLOCATE_PTR (k2ppErrBits,   int,      NELEM);
  ALLOCATE_PTR (k2SeqNum,      int,      NELEM);

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
      iPARSE (  3,   1, frameSetID,    "frameSetID");
      dPARSE (  4,   3, ra,            "ra");
      dPARSE (  5,   4, dec,           "dec");
      dPARSE (  8,   5, epoch,         "epoch");
      jPARSE ( 23,   8, priOrSec,      "priOrSec"); // watch for epoch
      iPARSE ( 41,  23, mergedClass,   "mergedClass");
      dPARSE ( 42,  41, pStar,         "pStar");
      dPARSE ( 43,  42, pGalaxy,       "pGalaxy");
      dPARSE ( 46,  43, zHallMag,      "zHallMag");
      dPARSE ( 47,  46, zHallMagErr,   "zHallMagErr");
      dPARSE ( 50,  47, zAperMag3,     "zAperMag3");
      dPARSE ( 51,  50, zAperMag3Err,  "zAperMag3Err");
      dPARSE ( 52,  51, zAperMag4,     "zAperMag4");
      dPARSE ( 53,  52, zAperMag4Err,  "zAperMag4Err");
      dPARSE ( 58,  53, zPA,           "zPA");
      iPARSE ( 61,  58, zClass,        "zClass");
      iPARSE ( 63,  61, zppErrBits,    "zppErrBits");
      iPARSE ( 64,  63, zSeqNum,       "zSeqNum");
      dPARSE ( 68,  64, yHallMag,      "yHallMag");
      dPARSE ( 69,  68, yHallMagErr,   "yHallMagErr");
      dPARSE ( 72,  69, yAperMag3,     "yAperMag3");
      dPARSE ( 73,  72, yAperMag3Err,  "yAperMag3Err");
      dPARSE ( 74,  73, yAperMag4,     "yAperMag4");
      dPARSE ( 75,  74, yAperMag4Err,  "yAperMag4Err");
      dPARSE ( 80,  75, yPA,           "yPA");
      iPARSE ( 83,  80, yClass,        "yClass");
      iPARSE ( 85,  83, yppErrBits,    "yppErrBits");
      iPARSE ( 86,  85, ySeqNum,       "ySeqNum");
      dPARSE ( 90,  86, jHallMag,      "jHallMag");
      dPARSE ( 91,  90, jHallMagErr,   "jHallMagErr");
      dPARSE ( 94,  91, jAperMag3,     "jAperMag3");
      dPARSE ( 95,  94, jAperMag3Err,  "jAperMag3Err");
      dPARSE ( 96,  95, jAperMag4,     "jAperMag4");
      dPARSE ( 97,  96, jAperMag4Err,  "jAperMag4Err");
      dPARSE (102,  97, jPA,           "jPA");
      iPARSE (105, 102, jClass,        "jClass");
      iPARSE (107, 105, jppErrBits,    "jppErrBits");
      iPARSE (108, 107, jSeqNum,       "jSeqNum");
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
      dPARSE (134, 130, k1HallMag,     "k1HallMag");
      dPARSE (135, 134, k1HallMagErr,  "k1HallMagErr");
      dPARSE (138, 135, k1AperMag3,    "k1AperMag3");
      dPARSE (139, 138, k1AperMag3Err, "k1AperMag3Err");
      dPARSE (140, 139, k1AperMag4,    "k1AperMag4");
      dPARSE (141, 140, k1AperMag4Err, "k1AperMag4Err");
      dPARSE (146, 141, k1PA,          "k1PA");
      iPARSE (149, 146, k1Class,       "k1Class");
      iPARSE (151, 149, k1ppErrBits,   "k1ppErrBits");
      iPARSE (152, 151, k1SeqNum,      "k1SeqNum");
      dPARSE (156, 152, k2HallMag,     "k2HallMag");
      dPARSE (157, 156, k2HallMagErr,  "k2HallMagErr");
      dPARSE (160, 157, k2AperMag3,    "k2AperMag3");
      dPARSE (161, 160, k2AperMag3Err, "k2AperMag3Err");
      dPARSE (162, 161, k2AperMag4,    "k2AperMag4");
      dPARSE (163, 162, k2AperMag4Err, "k2AperMag4Err");
      dPARSE (168, 163, k2PA,          "k2PA");
      iPARSE (171, 168, k2Class,       "k2Class");
      iPARSE (173, 171, k2ppErrBits,   "k2ppErrBits");
      iPARSE (174, 173, k2SeqNum,      "k2SeqNum");

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

	REALLOCATE (zHallMag,     double,   NELEM);
	REALLOCATE (zHallMagErr,  double,   NELEM);
	REALLOCATE (zAperMag3,    double,   NELEM);
	REALLOCATE (zAperMag3Err, double,   NELEM);
	REALLOCATE (zAperMag4,    double,   NELEM);
	REALLOCATE (zAperMag4Err, double,   NELEM);
	REALLOCATE (zPA,          double,   NELEM);
	REALLOCATE (zClass,       int,      NELEM);
	REALLOCATE (zppErrBits,   int,      NELEM);
	REALLOCATE (zSeqNum,      int,      NELEM);

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

	REALLOCATE (jHallMag,     double,   NELEM);
	REALLOCATE (jHallMagErr,  double,   NELEM);
	REALLOCATE (jAperMag3,    double,   NELEM);
	REALLOCATE (jAperMag3Err, double,   NELEM);
	REALLOCATE (jAperMag4,    double,   NELEM);
	REALLOCATE (jAperMag4Err, double,   NELEM);
	REALLOCATE (jPA,          double,   NELEM);
	REALLOCATE (jClass,       int,      NELEM);
	REALLOCATE (jppErrBits,   int,      NELEM);
	REALLOCATE (jSeqNum,      int,      NELEM);

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

	REALLOCATE (k1HallMag,     double,   NELEM);
	REALLOCATE (k1HallMagErr,  double,   NELEM);
	REALLOCATE (k1AperMag3,    double,   NELEM);
	REALLOCATE (k1AperMag3Err, double,   NELEM);
	REALLOCATE (k1AperMag4,    double,   NELEM);
	REALLOCATE (k1AperMag4Err, double,   NELEM);
	REALLOCATE (k1PA,          double,   NELEM);
	REALLOCATE (k1Class,       int,      NELEM);
	REALLOCATE (k1ppErrBits,   int,      NELEM);
	REALLOCATE (k1SeqNum,      int,      NELEM);

	REALLOCATE (k2HallMag,     double,   NELEM);
	REALLOCATE (k2HallMagErr,  double,   NELEM);
	REALLOCATE (k2AperMag3,    double,   NELEM);
	REALLOCATE (k2AperMag3Err, double,   NELEM);
	REALLOCATE (k2AperMag4,    double,   NELEM);
	REALLOCATE (k2AperMag4Err, double,   NELEM);
	REALLOCATE (k2PA,          double,   NELEM);
	REALLOCATE (k2Class,       int,      NELEM);
	REALLOCATE (k2ppErrBits,   int,      NELEM);
	REALLOCATE (k2SeqNum,      int,      NELEM);
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

    // z measure
    stars[Nstars].measure[1].extID      = sourceID[i];
    stars[Nstars].measure[1].imageID    = frameSetID[i]; // XXX check for frameSetID > 31-bit int
    stars[Nstars].measure[1].R          = ra[i];
    stars[Nstars].measure[1].D          = dec[i];
    stars[Nstars].measure[1].psfChisq   = pStar[i];
    stars[Nstars].measure[1].extNsigma  = pGalaxy[i];
    stars[Nstars].measure[1].M          = zHallMag[i];
    stars[Nstars].measure[1].dM         = zHallMagErr[i];
    stars[Nstars].measure[1].Map        = zAperMag3[i];
    stars[Nstars].measure[1].dMap       = zAperMag3Err[i];
    stars[Nstars].measure[1].Mkron      = zAperMag4[i];
    stars[Nstars].measure[1].dMkron     = zAperMag4Err[i];
    stars[Nstars].measure[1].posangle   = ToShortDegrees(zPA[i]); // XXX units
    stars[Nstars].measure[1].psfQF      = psfQFfromXClass(zClass[i]);
    stars[Nstars].measure[1].photFlags  = zppErrBits[i];
    stars[Nstars].measure[1].photFlags2 = photFlags2;
    stars[Nstars].measure[1].detID      = zSeqNum[i];
    stars[Nstars].measure[1].photcode   = codeZ;

    // J measure
    stars[Nstars].measure[2].extID      = sourceID[i];
    stars[Nstars].measure[2].imageID    = frameSetID[i]; // XXX check for frameSetID > 31-bit int
    stars[Nstars].measure[2].R          = ra[i];
    stars[Nstars].measure[2].D          = dec[i];
    stars[Nstars].measure[2].psfChisq   = pStar[i];
    stars[Nstars].measure[2].extNsigma  = pGalaxy[i];
    stars[Nstars].measure[2].M          = jHallMag[i];
    stars[Nstars].measure[2].dM         = jHallMagErr[i];
    stars[Nstars].measure[2].Map        = jAperMag3[i];
    stars[Nstars].measure[2].dMap       = jAperMag3Err[i];
    stars[Nstars].measure[2].Mkron      = jAperMag4[i];
    stars[Nstars].measure[2].dMkron     = jAperMag4Err[i];
    stars[Nstars].measure[2].posangle   = ToShortDegrees(jPA[i]); // XXX units
    stars[Nstars].measure[2].psfQF      = psfQFfromXClass(jClass[i]);
    stars[Nstars].measure[2].photFlags  = jppErrBits[i];
    stars[Nstars].measure[2].photFlags2 = photFlags2;
    stars[Nstars].measure[2].detID      = jSeqNum[i];
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

    // K1 measure
    stars[Nstars].measure[4].extID      = sourceID[i];
    stars[Nstars].measure[4].imageID    = frameSetID[i]; // XXX check for frameSetID > 31-bit int
    stars[Nstars].measure[4].R          = ra[i];
    stars[Nstars].measure[4].D          = dec[i];
    stars[Nstars].measure[4].psfChisq   = pStar[i];
    stars[Nstars].measure[4].extNsigma  = pGalaxy[i];
    stars[Nstars].measure[4].M          = k1HallMag[i];
    stars[Nstars].measure[4].dM         = k1HallMagErr[i];
    stars[Nstars].measure[4].Map        = k1AperMag3[i];
    stars[Nstars].measure[4].dMap       = k1AperMag3Err[i];
    stars[Nstars].measure[4].Mkron      = k1AperMag4[i];
    stars[Nstars].measure[4].dMkron     = k1AperMag4Err[i];
    stars[Nstars].measure[4].posangle   = ToShortDegrees(k1PA[i]); // XXX units
    stars[Nstars].measure[4].psfQF      = psfQFfromXClass(k1Class[i]);
    stars[Nstars].measure[4].photFlags  = k1ppErrBits[i];
    stars[Nstars].measure[4].photFlags2 = photFlags2;
    stars[Nstars].measure[4].detID      = k1SeqNum[i];
    stars[Nstars].measure[4].photcode   = codeK;

    // K1 measure
    stars[Nstars].measure[5].extID      = sourceID[i];
    stars[Nstars].measure[5].imageID    = frameSetID[i]; // XXX check for frameSetID > 31-bit int
    stars[Nstars].measure[5].R          = ra[i];
    stars[Nstars].measure[5].D          = dec[i];
    stars[Nstars].measure[5].psfChisq   = pStar[i];
    stars[Nstars].measure[5].extNsigma  = pGalaxy[i];
    stars[Nstars].measure[5].M          = k2HallMag[i];
    stars[Nstars].measure[5].dM         = k2HallMagErr[i];
    stars[Nstars].measure[5].Map        = k2AperMag3[i];
    stars[Nstars].measure[5].dMap       = k2AperMag3Err[i];
    stars[Nstars].measure[5].Mkron      = k2AperMag4[i];
    stars[Nstars].measure[5].dMkron     = k2AperMag4Err[i];
    stars[Nstars].measure[5].posangle   = ToShortDegrees(k2PA[i]); // XXX units
    stars[Nstars].measure[5].psfQF      = psfQFfromXClass(k2Class[i]);
    stars[Nstars].measure[5].photFlags  = k2ppErrBits[i];
    stars[Nstars].measure[5].photFlags2 = photFlags2;
    stars[Nstars].measure[5].detID      = k2SeqNum[i];
    stars[Nstars].measure[5].photcode   = codeK;

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

  FREE (zHallMag);
  FREE (zHallMagErr);
  FREE (zAperMag3);
  FREE (zAperMag3Err);
  FREE (zAperMag4);
  FREE (zAperMag4Err);
  FREE (zPA);
  FREE (zClass);
  FREE (zppErrBits);
  FREE (zSeqNum);

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

  FREE (jHallMag);
  FREE (jHallMagErr);
  FREE (jAperMag3);
  FREE (jAperMag3Err);
  FREE (jAperMag4);
  FREE (jAperMag4Err);
  FREE (jPA);
  FREE (jClass);
  FREE (jppErrBits);
  FREE (jSeqNum);

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

  FREE (k1HallMag);
  FREE (k1HallMagErr);
  FREE (k1AperMag3);
  FREE (k1AperMag3Err);
  FREE (k1AperMag4);
  FREE (k1AperMag4Err);
  FREE (k1PA);
  FREE (k1Class);
  FREE (k1ppErrBits);
  FREE (k1SeqNum);

  FREE (k2HallMag);
  FREE (k2HallMagErr);
  FREE (k2AperMag3);
  FREE (k2AperMag3Err);
  FREE (k2AperMag4);
  FREE (k2AperMag4Err);
  FREE (k2PA);
  FREE (k2Class);
  FREE (k2ppErrBits);
  FREE (k2SeqNum);

  *nstars = Nstars;
  *nstart = Nstart;
  return (stars);
}
