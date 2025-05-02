# ifndef LIBDVO_ASTRO_H
# define LIBDVO_ASTRO_H

typedef enum {
  COORD_NONE, 
  COORD_CELESTIAL, 
  COORD_GALACTIC, // based on Liu et al 2011
  COORD_ECLIPTIC,
  COORD_GALACTIC_REID_2004, // an older definition of the galactic plane
} CoordTransformSystem;

typedef enum {
  PROJ_NONE, // undefined
  PROJ_ZEA, // zenithal
  PROJ_ZPL, // zenithal
  PROJ_ARC, // zenithal
  PROJ_STG, // zenithal
  PROJ_SIN, // zenithal
  PROJ_TAN, // zenithal
  PROJ_TNX, // zenithal
  PROJ_DIS, // zenithal (TAN + polyterms)
  PROJ_LIN, // cartesian
  PROJ_PLY, // cartesian (allow polyterms)
  PROJ_WRP, // cartesian (allow polyterms, require mosaic)
  PROJ_AIT, // pseudocyl
  PROJ_GLS, // pseudocyl
  PROJ_PAR, // pseudocyl
  PROJ_ZPN, // zenithal
  PROJ_MOL, // pseudocyl 
} OhanaProjection;

typedef enum {
  PROJ_MODE_NONE,
  PROJ_MODE_CARTESIAN,
  PROJ_MODE_ZENITHAL,
  PROJ_MODE_PSEUDOCYL,
} OhanaProjectionMode;

typedef struct {
  int    isIdentity;	      // identity transformation 
  double phi;		      // saved in radians
  double Xo;		      // saved in radians
  double xo;		      // saved in degrees
  double sin_phi_cos_Xo;      // pre-computed values
  double sin_phi_sin_Xo;      // pre-computed values
  double cos_phi_cos_Xo;      // pre-computed values
  double cos_phi_sin_Xo;      // pre-computed values
  double cos_phi;	      // pre-computed values
  double sin_phi;	      // pre-computed values
  double cos_Xo;	      // pre-computed values
  double sin_Xo;	      // pre-computed values
} CoordTransform;

typedef struct {
  int Nx;
  int Ny;
  float dX;
  float dY;
  float *dXv;
  float *dYv;
  unsigned int tableID;
  unsigned int imageID;
  int keep;
} AstromOffsetMap;

typedef struct {
  int Nmap;
  int NMAP;
  AstromOffsetMap **map;
  int *imageIDtoTableSeq;
  unsigned int MaxImageID;
  unsigned int MaxTableID;
} AstromOffsetTable;

/* Internal version of Coords (see CoordsDisk in libautocode/def/coords-disk.d) */
typedef struct Coords {
  double           crval1;               // coordinate at reference pixel
  double           crval2;               // coordinate at reference pixel
  float            crpix1;               // coordinate of reference pixel
  float            crpix2;               // coordinate of reference pixel
  float            cdelt1;               // degrees per pixel
  float            cdelt2;               // degrees per pixel
  float            pc1_1;                // rotation matrix
  float            pc1_2;                // rotation matrix
  float            pc2_1;                // rotation matrix
  float            pc2_2;                // rotation matrix
  float            polyterms[7][2];      // higher order warping terms
  char             ctype[15];            // coordinate type
  char             Npolyterms;           // order of polynomial
  struct Coords   *mosaic;               // pointer to parent mosaic
  AstromOffsetMap *offsetMap;		 // pointer to offset map transformation
} Coords;

typedef struct Image {
  Coords           coords;               // astrometric data
  e_time           tzero;                // readout time (row 0)
  unsigned int     nstar;                // number of stars on image
  float            secz;                 // airmass (mag)
  unsigned short   NX;                   // image width
  unsigned short   NY;                   // image height
  float            apmifit;              // aperture correction (mag)
  float            dapmifit;             // apmifit error (mag)
  float            McalPSF;              // calibration mag (mag)
  float            McalAPER;             // calibration mag (mag)
  float            dMcal;                // error on Mcal (mag)
  float            McalChiSq;            // image chisq (10*log(value))
  short            photcode;             // identifier for CCD,
  float            exptime;              // exposure time (seconds)
  float            sidtime;              // sidereal time of exposure
  float            latitude;             // observatory latitude (degrees)
  float            RAo;                  // image center (degrees)
  float            DECo;                 // image center (degrees)
  float            Radius;               // image radius (degrees)
  float            refColorBlue;         // median astrometry ref color
  float            refColorRed;          // median astrometry ref color
  char             name[117];            // name of original image 
  unsigned char    detection_limit;      // detection limit (10*mag)
  unsigned char    saturation_limit;     // saturation limit (10*mag)
  unsigned char    cerror;               // astrometric error (50*arcsec)
  unsigned char    fwhm_x;               // PSF x width (25*arcsec)
  unsigned char    fwhm_y;               // PSF y width (25*arcsec)
  unsigned char    trate;                // scan rate (100 usec/pixel)
  unsigned char    ccdnum;               // CCD ID number
  unsigned int     flags;                // image quality flags
  unsigned int     imageID;              // internal image ID
  unsigned int     parentID;             // associated ref image
  unsigned int     externID;             // external image ID
  unsigned short   sourceID;             // analysis source ID
  short            nLinkAstrom;          // mean number of matched measurements for astrometry
  short            nLinkPhotom;          // mean number of matched measurements for astrometry
  short            ubercalDist;          // distance to nearest ubercal image
  float            dXpixSys;             // systematic astrometry error in X
  float            dYpixSys;             // systematic astrometry error in Y
  float            dMagSys;              // systematic photometry error
  unsigned short   nFitAstrom;           // number of stars used for astrometry cal
  unsigned short   nFitPhotom;           // number of stars used for photometry cal
  unsigned int     photom_map_id;        // reference to 2D zero point map
  unsigned int     astrom_map_id;        // reference to 2D astrometry map
  struct Image    *parent;               // pointer to parent mosaic (not save to disk)
} Image;

CoordTransform *AllocTransform (double phi, double Xo, double xo);
CoordTransform *InitTransform (CoordTransformSystem input, CoordTransformSystem output);
int ApplyTransform (double *x, double *y, double X, double Y, CoordTransform *transform);

/* in coords.c */
void InitCoords (Coords *coords, char *projection);
void CopyCoords (Coords *tgt, Coords *src);

int  XY_to_LM (double *L, double *M, double x,  double y,   Coords *coords);
int  LM_to_XY (double *x,  double *y,   double L, double M, Coords *coords);
int  RD_to_LM (double *L, double *M, double ra,  double dec,   Coords *coords);
int  LM_to_RD (double *ra, double *dec,   double L, double M, Coords *coords);
int  XY_to_RD (double *ra, double *dec, double x,  double y,   Coords *coords);
int  RD_to_XY (double *x,  double *y,   double ra, double dec, Coords *coords);
int  fXY_to_RD (float *ra, float *dec, double x,  double y,   Coords *coords);
int  fRD_to_XY (float *x,  float *y,   double ra, double dec, Coords *coords);
int  GetCoords (Coords *coords, Header *header);
int  PutCoords (Coords *coords, Header *header);
void coords_precess (double *ra, double *dec, double in_epoch, double out_epoch);
OhanaProjection GetProjection (char *ctype);
int SetProjection (char *ctype, OhanaProjection proj);
OhanaProjectionMode GetProjectionMode (OhanaProjection proj);

/* in AstromOffsetMapIO.c */
AstromOffsetTable *AstromOffsetMapLoad (char *filename, int Nrows, int VERBOSE);
int AstromOffsetMapSave (AstromOffsetTable *table, char *filename);
AstromOffsetTable *AstromOffsetMapToTable(AstromOffsetMap_Disk_6x6 *map_disk, off_t Nmap);
AstromOffsetMap_Disk_6x6 *AstromOffsetTableToMap(AstromOffsetTable *table, off_t *Nmap);
int AstromOffsetTableSetIDs (AstromOffsetTable *table);
AstromOffsetTable *AstromOffsetMapAppendToTable(AstromOffsetTable *table, AstromOffsetMap_Disk_6x6 *map_disk, off_t Nmap);

/* in AstromOffsetMapOps.c */
float AstromOffsetMapValue (AstromOffsetMap *map, float x, float y, int xdir);
int AstromOffsetMapFit (AstromOffsetMap *map, float *x, float *y, float *f, float *df, int Npts, int xdir);

/* in AstromOffsetMapUtils.c */
AstromOffsetMap *AstromOffsetMapInit (int Nx, int Ny);
void AstromOffsetMapFree (AstromOffsetMap *map);

int AstromOffsetTableNewMap (AstromOffsetTable *table, int Nx, int Ny, Image *image);
int AstromOffsetTableMatchChips (Image *images, off_t Nimages, AstromOffsetTable *table);
AstromOffsetTable *AstromOffsetTableInit();
void AstromOffsetTableFree(AstromOffsetTable *table);
int AstromOffsetTableAddMapFromImage (AstromOffsetTable *table, Image *image);
void AstromOffsetMapPrint (AstromOffsetMap *map, char *filename);
int AstromOffsetMapRepair (AstromOffsetMap *map, int xdir);
AstromOffsetMap *AstromOffsetMapCopy (AstromOffsetMap *map);
void AstromOffsetMapCopyData (AstromOffsetMap *tgt, AstromOffsetMap *src);
void AstromOffsetMapSetOrder (AstromOffsetMap *map, int Nx, int Ny, Image *image);

# endif
