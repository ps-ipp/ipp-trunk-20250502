#ifndef PM_MASK_STATS_H
#define PM_MASK_STATS_H

bool pmFPAMaskStats(pmFPA *fpa, pmConfig *config);
bool pmSingleImageMaskStats(psImage *mask,
                            psS32 *Npix_valid, psS32 *Npix_static, psS32 *Npix_magic,
                            psS32 *Npix_dynamic, psS32 *Npix_advisory,
                            psImageMaskType staticMaskVal, psImageMaskType magicMaskVal,
                            psImageMaskType dynamicMaskVal, psImageMaskType advisoryMaskVal);


//bool pmMaskStats(pmFPA *fpa, pmConfig *config, psMetadata *results);

#endif
