# include "addstar.h"
# include "ukirt_uhs.h"

/* this function reads the values of interest from the UKIRT UHS CSV files:

  data model description: doc/mapping_UKIDSS_unWISE_DVO.pdf

  N -- field (column) number in CSV file (1 counting)
  | UKIRT field name          : DVO measure field
  1 sourceID                  : extID
  2 cuEventID		      : 
  3 frameSetID		      : imageID (assumes we do not try to go backwards for these photcodes)
  4 ra			      : R
  5 dec			      : D
  6 cx			      : 
  7 cy			      : 
  8 cz			      : 
  9 htmID		      : 
 10 l			      : 
 11 b			      : 
 12 lambda		      : 
 13 eta			      : 
 14 priOrSec		      : primary bit in dbFlags
 15 mergedClassStat	      : 
 16 mergedClass		      : psfQFperf or photFlags2
 17 pStar		      : psfChisq
 18 pGalaxy		      : extNsigma
 19 pNoise		      : 
 20 pSaturated		      : 
 21 eBV			      : 
 22 aJ			      : 
 23 jHallMag		      : M
 24 jHallMagErr		      : dM
 25 jPetroMag		      : 
 26 jPetroMagErr	      : 
 27 jAperMag3		      : Map
 28 jAperMag3Err	      : dMap
 29 jAperMag4		      : Mkron
 30 jAperMag4Err	      : dMron
 31 jAperMag6		      : 
 32 jAperMag6Err	      : 
 33 jGausig		      : 
 34 jEll		      : 
 35 jPA			      : posangle
 36 jErrBits		      : 
 37 jDeblend		      : 
 38 jClass		      : psfQF
 39 jClassStat		      : 
 40 jppErrBits		      : photFlags
 41 jSeqNum                   : detID

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
static int Nskip = 1; // UKIRT UHS CSV files have a single header row (and no special character to mark)

UKIRT_Stars *loadukirt_uhs_readstars_uhs (FILE *f, char *buffer, int *nstart, AddstarClientOptions *options, int *nstars) {

  int codeJ = GetPhotcodeCodebyName ("UKIRT_J"); if (!codeJ) Shutdown ("missing photcode UKIRT_UHS_J");

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
  ALLOCATE_PTR (priOrSec,     uint64_t, NELEM);
  ALLOCATE_PTR (mergedClass,  int,      NELEM); // XXX doc says 2-bytes (could use a short)
  ALLOCATE_PTR (pStar,        double,   NELEM);
  ALLOCATE_PTR (pGalaxy,      double,   NELEM);
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
      jPARSE ( 1,  1, sourceID,     "sourceID");
      iPARSE ( 3,  1, frameSetID,   "frameSetID");
      dPARSE ( 4,  3, ra,           "ra");
      dPARSE ( 5,  4, dec,          "dec");
      jPARSE (14,  5, priOrSec,     "priOrSec");
      iPARSE (16, 14, mergedClass,  "mergedClass");
      dPARSE (17, 16, pStar,        "pStar");
      dPARSE (18, 17, pGalaxy,      "pGalaxy");
      dPARSE (23, 18, jHallMag,     "jHallMag");
      dPARSE (24, 23, jHallMagErr,  "jHallMagErr");
      dPARSE (27, 24, jAperMag3,    "jAperMag3");
      dPARSE (28, 27, jAperMag3Err, "jAperMag3Err");
      dPARSE (29, 28, jAperMag4,    "jAperMag4");
      dPARSE (30, 29, jAperMag4Err, "jAperMag4Err");
      dPARSE (35, 30, jPA,          "jPA");
      iPARSE (38, 35, jClass,       "jClass");
      iPARSE (40, 38, jppErrBits,   "jppErrBits");
      iPARSE (41, 40, jSeqNum,      "jSeqNum");

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
	REALLOCATE (priOrSec,     uint64_t, NELEM);
	REALLOCATE (mergedClass,  int,      NELEM);
	REALLOCATE (pStar,        double,   NELEM);
	REALLOCATE (pGalaxy,      double,   NELEM);
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

    // we have one UKIRT UHS (J-band) measurement per object
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

    stars[Nstars].measure[0].extID      = sourceID[i];
    stars[Nstars].measure[0].imageID    = frameSetID[i]; // XXX check for frameSetID > 31-bit int
    stars[Nstars].measure[0].R          = ra[i];
    stars[Nstars].measure[0].D          = dec[i];
    stars[Nstars].measure[0].psfChisq   = pStar[i];
    stars[Nstars].measure[0].extNsigma  = pGalaxy[i];
    stars[Nstars].measure[0].M          = jHallMag[i];
    stars[Nstars].measure[0].dM         = jHallMagErr[i];
    stars[Nstars].measure[0].Map        = jAperMag3[i];
    stars[Nstars].measure[0].dMap       = jAperMag3Err[i];
    stars[Nstars].measure[0].Mkron      = jAperMag4[i];
    stars[Nstars].measure[0].dMkron     = jAperMag4Err[i];
    stars[Nstars].measure[0].posangle   = ToShortDegrees(jPA[i]); // jPA is in degrees, posangle is short 
    stars[Nstars].measure[0].psfQF      = psfQFfromXClass(jClass[i]);
    stars[Nstars].measure[0].photFlags  = jppErrBits[i];
    stars[Nstars].measure[0].photFlags2 = photFlags2;
    stars[Nstars].measure[0].detID      = jSeqNum[i];
    stars[Nstars].measure[0].photcode   = codeJ;

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
  FREE (priOrSec);
  FREE (mergedClass);
  FREE (pStar);
  FREE (pGalaxy);
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

  *nstars = Nstars;
  *nstart = Nstart;
  return (stars);
}

