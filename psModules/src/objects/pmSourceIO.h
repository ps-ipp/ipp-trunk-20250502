/* @file  pmSourceIO.h
 * @brief functions to read and write object files
 *
 * @author EAM, IfA; GLG, MHPCC
 *
 * @version $Revision: 1.20 $ $Name: not supported by cvs2svn $
 * @date $Date: 2009-02-16 22:30:50 $
 * Copyright 2004 Maui High Performance Computing Center, University of Hawaii
 *
 */

# ifndef PM_SOURCE_IO_H
# define PM_SOURCE_IO_H

/// @addtogroup Objects Object Detection / Analysis Functions
/// @{

# define MK_PROTO(TYPE) \
  bool pmSourcesWrite_##TYPE(psFits *fits, pmReadout *readout, psArray *sources, psMetadata *imageHeader, psMetadata *tableHeader, char *extname, psMetadata *recipe); \
  bool pmSourcesWrite_##TYPE##_XSRC(psFits *fits, pmReadout *readout, psArray *sources, psMetadata *imageHeader, char *extname, psMetadata *recipe); \
  bool pmSourcesWrite_##TYPE##_XFIT(psFits *fits, pmReadout *readout, psArray *sources, psMetadata *imageHeader, char *extname); \
  bool pmSourcesWrite_##TYPE##_XRAD(psFits *fits, pmReadout *readout, psArray *sources, psMetadata *imageHeader, char *extname, psMetadata *recipe); \
  bool pmSourcesWrite_##TYPE##_XGAL(psFits *fits, pmReadout *readout, psArray *sources, char *extname, psMetadata *recipe); \
  psArray *pmSourcesRead_##TYPE (psFits *fits, psMetadata *header); \
  bool pmSourcesRead_##TYPE##_XSRC (psFits *fits, pmReadout *readout, psMetadata *header, psMetadata *tableHeader, psArray *sources, long *index); \
  bool pmSourcesRead_##TYPE##_XFIT (psFits *fits, pmReadout *readout, psMetadata *header, psMetadata *tableHeader, psArray *sources, long *index); \
  bool pmSourcesRead_##TYPE##_XRAD (psFits *fits, pmReadout *readout, psMetadata *header, psMetadata *tableHeader, psArray *sources, long *index);\
  bool pmSourcesRead_##TYPE##_XGAL (psFits *fits, pmReadout *readout, psMetadata *header, psMetadata *tableHeader, psArray *sources, long *index);\
  
// All of these functions need to use the same API, even if not all elements are used in a specific case
MK_PROTO(SMPDATA);
MK_PROTO(PS1_DEV_0);
MK_PROTO(PS1_DEV_1);
MK_PROTO(PS1_CAL_0);
MK_PROTO(CMF_PS1_V1);
MK_PROTO(CMF_PS1_V2);
MK_PROTO(CMF_PS1_V3);
MK_PROTO(CMF_PS1_V4);
MK_PROTO(CMF_PS1_V5);
MK_PROTO(CMF_PS1_SV1);
MK_PROTO(CMF_PS1_SV2);
MK_PROTO(CMF_PS1_SV3);
MK_PROTO(CMF_PS1_SV4);
MK_PROTO(CMF_PS1_DV1);
MK_PROTO(CMF_PS1_DV2);
MK_PROTO(CMF_PS1_DV3);
MK_PROTO(CMF_PS1_DV4);
MK_PROTO(CMF_PS1_DV5);

int pmSourceGetDophotType (pmSource *source);
bool pmSourceSetDophotType (pmSource *source, int type);

bool pmSourcesWriteRAW (psArray *sources, char *filename);
bool pmSourcesWriteOBJ (psArray *sources, char *filename);
bool pmSourcesWriteSX (psArray *sources, char *filename);
bool pmSourcesWriteCMP (psArray *sources, char *filename, psMetadata *header);

bool pmSource_CMF_WritePHU (const pmFPAview *view, pmFPAfile *file, pmConfig *config);

psArray *pmSourcesReadCMP (char *filename, psMetadata *header);
psArray *pmSourcesRead_CFF (psFits *fits, psMetadata *header, psMetadata *recipe);
bool pmSourcesWrite_CFF (pmReadout *readout, psFits *fits, psArray *sources, psMetadata *header, psMetadata *recipe);

bool pmSourcesWritePSFs (psArray *sources, char *filename);
bool pmSourcesWriteEXTs (psArray *sources, char *filename, bool require);
bool pmSourcesWriteNULLs (psArray *sources, char *filename);
bool pmMomentsWriteText (psArray *sources, char *filename);
bool pmPeaksWriteText (psArray *peaks, char *filename);

bool pmFPAviewReadObjects (const pmFPAview *view, pmFPAfile *file, const pmConfig *config);
bool pmFPAReadObjects (pmFPA *fpa, const pmFPAview *view, pmFPAfile *file, const pmConfig *config);
bool pmChipReadObjects (pmChip *chip, const pmFPAview *view, pmFPAfile *file, const pmConfig *config);
bool pmCellReadObjects (pmCell *cell, const pmFPAview *view, pmFPAfile *file, const pmConfig *config);
bool pmReadoutReadObjects (pmReadout *readout, const pmFPAview *view, pmFPAfile *file, const pmConfig *config);

bool pmFPAviewWriteObjects (const pmFPAview *view, pmFPAfile *file, pmConfig *config);
bool pmFPAWriteObjects (pmFPA *fpa, const pmFPAview *view, pmFPAfile *file, const pmConfig *config);
bool pmChipWriteObjects (pmChip *chip, const pmFPAview *view, pmFPAfile *file, const pmConfig *config);
bool pmCellWriteObjects (pmCell *cell, const pmFPAview *view, pmFPAfile *file, const pmConfig *config);
bool pmReadoutWriteObjects (pmReadout *readout, const pmFPAview *view, pmFPAfile *file, const pmConfig *config);

bool pmFPAviewCheckDataStatusForSources (const pmFPAview *view, const pmFPAfile *file);
bool pmFPACheckDataStatusForSources (const pmFPA *fpa);
bool pmChipCheckDataStatusForSources (const pmChip *chip);
bool pmCellCheckDataStatusForSources (const pmCell *cell);
bool pmReadoutCheckDataStatusForSources (const pmReadout *readout);

bool pmSourceLocalAstrometry (psSphere *ptSky, float *posAngle, float *pltScale, pmChip *chip, float xPos, float yPos);

bool pmSourceIO_WriteMatchedRefs (psFits *fits, pmFPA *fpa, pmConfig *config);
bool pmSourceIO_ReadMatchedRefs (psFits *fits, pmFPA *fpa, const pmConfig *config);

bool pmSourceZeroPointFromRecipeGlint (float *zeropt, float *exptime, float *ghostMaxMag, double *glintMaxMag, pmFPA *fpa, psMetadata *recipe);
bool pmSourceIO_WriteGlints (psFits *fits, pmFPA *fpa, pmConfig *config);
bool pmSourceIO_WriteGhosts (psFits *fits, pmFPA *fpa, pmConfig *config);

/// @}
# endif /* PM_SOURCE_IO_H */
