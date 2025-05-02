# include "data.h"
# include "basic.h"
# include "astro.h"
# include "dvo.h"

# ifndef DVOSHELL_H
# define DVOSHELL_H

typedef struct {
  char name[64];
  double RA0, RA1, DEC0, DEC1;
} RegionFile;

typedef struct {
  double X;
  double Y;
  double R;
  double D;
  double M, dM;
  char   dophot;
  double sky;
  double fx, fy, df;
  double Mgal, Map;
  int found;
  short int code;
  e_time t;
} CMPstars;

// a structure to define a sequence lookup
typedef struct {
  int minID;
  int maxID;
  int *value;
  int *sequence;
  int Nsequence;
  int NSEQUENCE;
} mySequenceType;

/** some globals used particularly by DVO_CLIENT **/
int   HOST_ID;
char *HOSTDIR;
char *RESULT_FILE;

typedef struct {
  int Nmeasure;
  int Nperiods;
  opihi_flt *period;
  opihi_flt *power;
  double P50;
  double R50;
  double R90;
} PeriodogramResult;

typedef struct {
  off_t Ncat;
  off_t Nseq;
  float Roff;
} AvselectResult;

/*** Periodogram functions (avperiodogram and avperiodomatch) ***/
void PeriodogramResultFree (PeriodogramResult *result);
void PeriodogramResultSave (PeriodogramResult *result, char *extname, FILE *foutput, Average *average);
void PeriodogramSetOptions (float minimumPeriod, float maximumPeriod);
PeriodogramResult *PeriodogramRawFloatingMean (opihi_flt *time, opihi_flt *flux, opihi_flt *dflux, int Npts);

/*** dvo prototypes ***/
int           DetermineTypeCode     PROTO((Average *average, Measure *measure, int code));
double        DetermineTypefrac     PROTO((Average *average, Measure *measure, PhotCode *code));
double        ExtractAverages       PROTO((PhotCode *code, int mode, Average *average, SecFilt *secfilt, Measure *measure, int param));
double       *ExtractByDMag         PROTO((PhotCode **code, int *mode, Average *average, SecFilt *secfilt, Measure *measure, off_t *nlist, int param));
double       *ExtractDMag           PROTO((PhotCode **code, int *mode, Average *average, SecFilt *secfilt, Measure *measure, off_t *nlist));
double       *ExtractMagnitudes     PROTO((PhotCode *code, int mode, Average *average, SecFilt *secfilt, Measure *measure, off_t *n));
double       *ExtractMeasures       PROTO((PhotCode *code, int mode, Average *average, SecFilt *secfilt, Measure *measure, off_t *nlist, int param));
double       *ExtractMeasuresByDMag PROTO((PhotCode **code, int *mode, int use_first, Average *average, SecFilt *secfilt, Measure *measure, off_t *nlist, int param));
double       *ExtractMeasuresDMag   PROTO((PhotCode **code, int *mode, Average *average, SecFilt *secfilt, Measure *measure, off_t *nlist));
void          FreeImageSelection    PROTO((void));
void          FreeImageSelection    PROTO((void));
int           GetAverageParam       PROTO((char *parname));
void          GetAverageParamHelp   PROTO((void));
int           GetMagMode            PROTO((char *string));
double        GetMeasure            PROTO((int param, Average *average, Measure *measure, double mag));
int           GetMeasureParam       PROTO((char *parname));
int           GetMeasureTypeCode    PROTO((Measure *measure));
int           GetPhotcodeInfo       PROTO((char *string, PhotCode **Code, int *Mode));
int           GetSelectionParam     PROTO((void));
int           GetTimeSelection      PROTO((time_t *tz, time_t *te));
void          InitDVO               PROTO((void));
void          FreeDVO               PROTO((void));
int           InitPhotcodes         PROTO((void));
Coords       *MatchMosaic           PROTO((unsigned int time, short int source));
int           Quality               PROTO((Measure *measure, int IsDophot));
int           SelectMags            PROTO((int Nphot, int Tphot, int Ns, Average *average, Measure *measure, SecFilt *secfilt, int UL));

int           SetSkyRegions         PROTO((SkyRegionSelection *selection));
SkyList      *SelectRegions         PROTO((SkyRegionSelection *selection));
SkyList      *SkyListLoadFile       PROTO((char *filename));
int           SetCATDIR             PROTO((char *path, int verbose));
char *        GetCATDIR             PROTO((void));
SkyTable     *GetSkyTable           PROTO((void));
SkyList      *SkyListFromFile       PROTO((char *filename));
SkyRegionSelection *SetRegionSelection    PROTO((int *argc, char **argv));

int           SkyRegionByPoint_r    PROTO((SkyTable *table, SkyList *list, int depth, double ra, double dec));
SkyList      *SelectRegionsByCoordVectors PROTO((Vector *RA, Vector *DEC));
SkyList      *SelectRegionsByCoordVectorsAndRadius (Vector *RA, Vector *DEC, float Radius);

int             find_matches_by_vectors_closest PROTO((SkyRegion *region, Catalog *catalog, Vector *RAvec, Vector *DECvec, float RADIUS, off_t *index));
AvselectResult *find_matches_by_vectors_allmatch PROTO((SkyRegion *region, Catalog *catalog, Vector *RAvec, Vector *DECvec, float RADIUS, off_t *Nresult));

int           SetImageSelection     PROTO((int mode, SkyRegionSelection *selection));
int           SetPhotSelections     PROTO((int *argc, char **argv, int Nparams));
int           SetSelectionParam     PROTO((int param));
int           TestAverage           PROTO((PhotCode *code, Average *average, SecFilt *secfilt, Measure *measure));
int           TestPhotSelections    PROTO((PhotCode **code, int *mode, int param));
void          compare               PROTO((Catalog *catlog1, Catalog *catlog2, Vector *rvec,  Vector *dvec,  Vector *mvec, Vector *drvec, Vector *ddvec, Vector *dmvec, double radius));
void          cprecess              PROTO((Average *average, off_t Naverage, double in_epoch, double out_epoch));
off_t         match_image           PROTO((Image *image, off_t Nimage, unsigned int T, short int S));
void          print_value           PROTO((double value, short int ival));
CMPstars     *cmpReadFits           PROTO((FILE *f, off_t *nstars));
CMPstars     *cmpReadText           PROTO((FILE *f, off_t *nstars));
int           RD_to_XYpic           PROTO((double *x, double *y, double r, double d, Coords *coords, double Rmin, double Rmax, double Rmid, int *leftside));
int           wordhash              PROTO((char *word));

int          HostTableLaunchJobs    PROTO((SkyList *sky, HostTable *table, char *basecmd, char *options, int VERBOSE));
int          HostTableParallelOps   PROTO((SkyList *sky, int argc, char **argv, char *ResultFile, int ReadVectors, int Nelements, int VERBOSE));
int          HostTableReloadResults PROTO((char *uniquer, int VERBOSE));
int          HostTableGetResults    PROTO((char *uniquer, int VERBOSE));

mySequenceType *mySequenceAlloc ();
int mySequenceFree (mySequenceType *mySequence);
int mySequenceSetSize (mySequenceType *mySequence, int Nmax);
int mySequenceSetValue (mySequenceType *mySequence, int value, int entry);
int mySequenceSort (mySequenceType *mySequence);
int mySequenceGetEntry (mySequenceType *mySequence, int value);

# endif // DVOSHELL_H
