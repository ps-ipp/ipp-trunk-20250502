# include "ppMops.h"

int ppMopsGetSkyChipPsfVersion(const psFits* fits) {
  // It is expected that the fits stream cursor was advanced
  // till the "SkyChip.psf"
  psMetadata *headerSkyChip = psFitsReadHeader(NULL, fits); 
  char* version = psMetadataLookupStr(NULL, headerSkyChip, "EXTTYPE");
  psTrace("ppMopsGetSkyChipPsfVersion", 1, 
	  "EXTTYPE value: [%s]\n", version);
  if (strcmp(version, "PS1_DV1") == 0) {
    psFree(headerSkyChip);
    return 1;
  } else if (strcmp(version, "PS1_DV2") == 0) {
    psFree(headerSkyChip);
    return 2;
  } else if (strcmp(version, "PS1_DV3") == 0) {
    psFree(headerSkyChip);
    return 3;
  } else if (strcmp(version, "PS1_DV4") == 0) {
    psFree(headerSkyChip);
    return 4;
  } else if (strcmp(version, "PS1_DV5") == 0) {
    psFree(headerSkyChip);
    return 5;
  }
  psWarning("Unsupported EXTTYPE in SkyChip.psf table: [%s]", version);
  psFree(headerSkyChip);
  return 0;
}
