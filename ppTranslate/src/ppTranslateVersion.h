#ifndef PP_TRANSLATE_VERSION
#define PP_TRANSLATE_VERSION

#include <pslib.h>

/// Return version
psString ppTranslateVersion(void);

/// Return source
psString ppTranslateSource(void);

/// Return detailed version information
psString ppTranslateVersionLong(void);

/// Put version into header
bool ppTranslateVersionHeader(psMetadata *header);

/// Print version information
void ppTranslateVersionPrint(void);

#endif
