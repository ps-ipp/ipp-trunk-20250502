#ifndef DVO_UTIL_H
#define DVO_UTIL_H

// dvo_util.h
// Declares a set of structures and functions useful for doing read only access
// to a dvo database

#include <dvo.h>
#define PHOTCODE_FILE_SIZE 280

typedef struct {
  char        gscfile[256];
  char        catdir[256];
  char        catmode[256];
  char        catformat[256];
  char        photcodeFile[PHOTCODE_FILE_SIZE];
  char        skyTableFile[256];
  int         skyDepth;
  FITS_DB     imageDB;
  Image      *images;
  off_t       nImages;
  SkyTable   *skyTable;
#if (DVO_UTIL_READ_CAMERA_CONFIG)
  char        cameraConfig[256];
  int        *ccdNum;
  int         nCCD;
#endif
} dvoConfig;

// This structure needs to be fleshed out
typedef struct {
  int         valid;
  Average     ave;
  Measure     meas;
#ifdef notdef
  int         objID;
  int         catID;
  int         detID;
  uint64_t    pspsObjID;
  uint64_t    pspsDetID;
#endif
} dvoDetection;

#define PSPS_OBJID(_d) (_d->ave.extID)
#define PSPS_DETID(_d) (_d->meas.extID)

// dvoConfigRead
// Prepares a program for reading from a dvo database
// 1. reads configuration files and program's argument list to configure dvo parameters are removed.
//       e.g. -D CATDIR mycatdir
// 2. Loads photcode file
// 3. Saves the resulting data in a structure that is used for calls to other utility functions
dvoConfig *dvoConfigRead(int *argc, char **argv);

// frees memory associated with the dvoConfig structure
void dvoConfigFree(dvoConfig *dvoConfig);

// Loads the images database. Saves the results in dvoConfig structure.
int dvoLoadImages(dvoConfig *dvoConfig);

// finds the dvo Image corresponding to the given external id (and sourceid)
// There is no need to release memory pointed to by return value. 
// It will be freed when dvoConfigFree() is invoked.
Image *dvoImageByExternID(dvoConfig *dvoConfig, unsigned short sourceID, unsigned int externID);

// Loads the sky table for the database.
SkyTable *dvoLoadSkyTable(dvoConfig *dvoConfig);

// find a list of catalogs touched by the given external id
// Use SkyListFree() to free memory pointed to by return value
SkyList *dvoSkyListByExternID(dvoConfig *dvoConfig, int sourceID, int externID, Image **ppImage);

// return a list of detections from a particular image id
// Use dvoFree() to free the memory pointed to by results
off_t dvoGetDetections(SkyList *skylist, unsigned int imageID, dvoDetection **results, unsigned int *pMaxDetID);

// free memory returned by various dvo util functions
void dvoFree(void *ptr);

#endif // DVO_UTIL_H
