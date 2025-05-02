#ifndef _PM_CENSOR_H
#define _PM_CENSOR_H

#include <pmConfig.h>

bool pmCensorGetMasks(pmConfig *config, psU32 *pMaskCensor, psU32 *pMaskStreak);
bool pmCensorMasked(pmConfig *config, psImage *image, psImage *mask, psImage *variance, long *pNumCensored);

#endif // _PM_CENSOR_H
