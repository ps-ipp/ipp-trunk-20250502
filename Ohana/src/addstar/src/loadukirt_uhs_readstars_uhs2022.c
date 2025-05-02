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

  6 cx			      : 		      
  7 cy			      : 		      
  8 cz			      : 		      
  9 htmID		      : 		      
 10 l			      : 		      
 11 b			      : 		      
 12 lambda		      : 		      
 13 eta			      : 		      
 14 priOrSec		      : primary bit in dbFlags

 15 jmkPnt		      :
 16 jmkPntErr		      :
 17 jmkExt		      :
 18 jmkExtErr		      :

 19 mergedClassStat	      :                         
 20 mergedClass		      : psfQFperf or photFlags2	
 21 pStar		      : psfChisq		
 22 pGalaxy		      : extNsigma		
 23 pNoise		      : 			
 24 pSaturated		      : 			

 25 eBV			    :
 26 aJ			    :
 27 aK			    :

 28 jHallMag		      : M	  
 29 jHallMagErr		      : dM	  
 30 jPetroMag		      : 	  
 31 jPetroMagErr	      : 	  
 32 jAperMag3		      : Map	  
 33 jAperMag3Err	      : dMap	  
 34 jAperMag4		      : Mkron	  
 35 jAperMag4Err	      : dMron	  
 36 jAperMag6		      : 	  
 37 jAperMag6Err	      : 	  
 38 jGausig		      : 	  
 39 jEll		      : 	  
 40 jPA			      : posangle  
 41 jErrBits		      : 	  
 42 jDeblend		      : 	  
 43 jClass		      : psfQF	  
 44 jClassStat		      : 	  
 45 jppErrBits		      : photFlags 
 46 jSeqNum                   : detID     
 47 jXi   		      : 
 48 jEta		      : 
 
 49 kHallMag		      : M                
 50 kHallMagErr		      : dM		 
 51 kPetroMag		      : 		 
 52 kPetroMagErr	      : 		 
 53 kAperMag3		      : Map		 
 54 kAperMag3Err	      : dMap		 
 55 kAperMag4		      : Mkron		 
 56 kAperMag4Err	      : dMron		 
 57 kAperMag6		      : 		 
 58 kAperMag6Err	      : 		 
 59 kGausig		      : 		 
 60 kEll		      : 		 
 61 kPA			      : posangle	 
 62 kErrBits		      : 		 
 63 kDeblend		      : 		 
 64 kClass		      : psfQF		 
 65 kClassStat		      : 		 
 66 kppErrBits		      : photFlags	 
 67 kSeqNum                   : detID		 
 68 kObjID		      : 		 
 69 kXi   		      : 		 
 70 kEta		      : 		 

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

UKIRT_Stars *loadukirt_uhs_readstars_uhs2022 (FILE *f, char *buffer, int *nstart, AddstarClientOptions *options, int *nstars) {

  int codeJ = GetPhotcodeCodebyName ("UKIRT_J"); if (!codeJ) Shutdown ("missing photcode UKIRT_J");
  int codeK = GetPhotcodeCodebyName ("UKIRT_K"); if (!codeK) Shutdown ("missing photcode UKIRT_K");

  // XXX I need a UKIRT UHS Mean Epoch
  time_t UKIRT_UHS_J_EPOCH = ohana_date_to_sec ("2016/01/01,00:00:00");
  time_t UKIRT_UHS_K_EPOCH = ohana_date_to_sec ("2019/01/01,00:00:00");
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
      iPARSE (  3,   1, frameSetID,    "frameSetID");
      dPARSE (  4,   3, ra,            "ra");
      dPARSE (  5,   4, dec,           "dec");
      jPARSE ( 14,   5, priOrSec,      "priOrSec"); // watch for epoch
      iPARSE ( 20,  14, mergedClass,   "mergedClass");
      dPARSE ( 21,  20, pStar,         "pStar");
      dPARSE ( 22,  21, pGalaxy,       "pGalaxy");
      dPARSE ( 28,  22, jHallMag,      "jHallMag");
      dPARSE ( 29,  28, jHallMagErr,   "jHallMagErr");
      dPARSE ( 32,  29, jAperMag3,     "jAperMag3");
      dPARSE ( 33,  32, jAperMag3Err,  "jAperMag3Err");
      dPARSE ( 34,  33, jAperMag4,     "jAperMag4");
      dPARSE ( 35,  34, jAperMag4Err,  "jAperMag4Err");
      dPARSE ( 40,  35, jPA,           "jPA");
      iPARSE ( 43,  40, jClass,        "jClass");
      iPARSE ( 45,  43, jppErrBits,    "jppErrBits");
      iPARSE ( 46,  45, jSeqNum,       "jSeqNum");
      dPARSE ( 49,  46, kHallMag,     "k1HallMag");
      dPARSE ( 50,  49, kHallMagErr,  "k1HallMagErr");
      dPARSE ( 53,  50, kAperMag3,    "k1AperMag3");
      dPARSE ( 54,  53, kAperMag3Err, "k1AperMag3Err");
      dPARSE ( 55,  54, kAperMag4,    "k1AperMag4");
      dPARSE ( 56,  55, kAperMag4Err, "k1AperMag4Err");
      dPARSE ( 61,  56, kPA,          "k1PA");
      iPARSE ( 64,  61, kClass,       "k1Class");
      iPARSE ( 66,  64, kppErrBits,   "k1ppErrBits");
      iPARSE ( 67,  66, kSeqNum,      "k1SeqNum");

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

    // J measure
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
    stars[Nstars].measure[0].posangle   = ToShortDegrees(jPA[i]); // XXX units
    stars[Nstars].measure[0].psfQF      = psfQFfromXClass(jClass[i]);
    stars[Nstars].measure[0].photFlags  = jppErrBits[i];
    stars[Nstars].measure[0].photFlags2 = photFlags2;
    stars[Nstars].measure[0].detID      = jSeqNum[i];
    stars[Nstars].measure[0].photcode   = codeJ;
    stars[Nstars].measure[0].t          = UKIRT_UHS_J_EPOCH;

    // K measure
    stars[Nstars].measure[1].extID      = sourceID[i];
    stars[Nstars].measure[1].imageID    = frameSetID[i]; // XXX check for frameSetID > 31-bit int
    stars[Nstars].measure[1].R          = ra[i];
    stars[Nstars].measure[1].D          = dec[i];
    stars[Nstars].measure[1].psfChisq   = pStar[i];
    stars[Nstars].measure[1].extNsigma  = pGalaxy[i];
    stars[Nstars].measure[1].M          = kHallMag[i];
    stars[Nstars].measure[1].dM         = kHallMagErr[i];
    stars[Nstars].measure[1].Map        = kAperMag3[i];
    stars[Nstars].measure[1].dMap       = kAperMag3Err[i];
    stars[Nstars].measure[1].Mkron      = kAperMag4[i];
    stars[Nstars].measure[1].dMkron     = kAperMag4Err[i];
    stars[Nstars].measure[1].posangle   = ToShortDegrees(kPA[i]); // XXX units
    stars[Nstars].measure[1].psfQF      = psfQFfromXClass(kClass[i]);
    stars[Nstars].measure[1].photFlags  = kppErrBits[i];
    stars[Nstars].measure[1].photFlags2 = photFlags2;
    stars[Nstars].measure[1].detID      = kSeqNum[i];
    stars[Nstars].measure[1].photcode   = codeK;
    stars[Nstars].measure[1].t          = UKIRT_UHS_K_EPOCH;

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
