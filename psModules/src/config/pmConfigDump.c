#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <string.h>

#include <pslib.h>

#include "pmConfig.h"
#include "pmFPALevel.h"
#include "pmFPA.h"
#include "pmFPAview.h"
#include "pmFPAfile.h"
#include "pmConfigCamera.h"
#include "pmErrorCodes.h"

#include "pmConfigDump.h"

// Cull entries in the metadata, ignoring the ones listed.
static bool configCull(psMetadata *md,  // Configuration metadata from which to cull
                       const psArray *list // List of items NOT to cull
    )
{
    PS_ASSERT_METADATA_NON_NULL(md, false);
    PS_ASSERT_ARRAY_NON_NULL(list, false);

    psHash *keep = psHashAlloc(list->n); // Hash with strings to keep
    for (int i = 0; i < keep->n; i++) {
        psHashAdd(keep, list->data[i], list->data[i]);
    }

    psMetadataIterator *iter = psMetadataIteratorAlloc(md, PS_LIST_HEAD, NULL); // Iterator
    psMetadataItem *item;               // Item from iteration
    while ((item = psMetadataGetAndIncrement(iter))) {
        if (!psHashLookup(keep, item->name)) {
            psMetadataRemoveKey(md, item->name);
        }
    }
    psFree(iter);

    psFree(keep);

    return true;
}

bool pmConfigRecipesCull(pmConfig *config, const char *save)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    if (!save || strlen(save) == 0) {
        return true;
    }

    psArray *keep = psStringSplitArray(save, " ,;", false); // List of items to keep

    if (!configCull(config->recipes, keep)) {
        psError(psErrorCodeLast(), false, "Unable to cull system recipes.");
        psFree(keep);
        return false;
    }
    psFree(keep);

    // Need to cull recipes from all cameras as well
    psMetadata *cameras = psMetadataLookupMetadata(NULL, config->system, "CAMERAS"); // Known cameras
    if (!cameras) {
        psError(PM_ERR_CONFIG, false, "Unable to find CAMERAS in the system configuration.\n");
        return false;
    }

    psMetadataIterator *iter = psMetadataIteratorAlloc(cameras, PS_LIST_HEAD, NULL); // Iterator for cameras
    psMetadataItem *item;               // Item from iteration
    keep = psArrayAllocEmpty(1);
    while ((item = psMetadataGetAndIncrement(iter))) {
        psAssert(item->type == PS_DATA_METADATA, "Should only be metadata types on the camera list.");
        psMetadata *camera = item->data.md; // Camera configuration
        bool mdok;                      // Status of MD lookup
        psMetadata *recipes = psMetadataLookupMetadata(&mdok, camera, "RECIPES"); // Recipe overrides
        if (!recipes) {
            continue;
        }
        if (!configCull(recipes, keep)) {
            psError(psErrorCodeLast(), false, "Unable to cull recipes for camera %s", item->name);
            psFree(iter);
            psFree(keep);
            return false;
        }
    }
    psFree(iter);
    psFree(keep);

    return true;
}

bool pmConfigCamerasCull(pmConfig *config, const char *additional)
{
      PS_ASSERT_PTR_NON_NULL(config, false);

      psMetadata *cameras = psMetadataLookupMetadata(NULL, config->system, "CAMERAS"); // Known cameras
      if (!cameras) {
          psError(PM_ERR_CONFIG, false, "Unable to find CAMERAS in the system configuration.\n");
          return NULL;
      }

      psArray *keep = NULL;             // List of cameras to keep
      if (additional) {
          keep = psStringSplitArray(additional, " ,;", false);
      } else {
          keep = psArrayAllocEmpty(1);
      }
      psArrayAdd(keep, 1, config->cameraName);

      int numKeep = keep->n;            // Number of cameras to keep
      for (int i = 0; i < numKeep; i++) {
          psString orig = keep->data[i];// Original name
          psString root = pmConfigCameraRootName(orig); // Camera root name
          psString chip = pmConfigCameraChipName(config->cameraName); // Chip-mosaicked name
          psString fpa  = pmConfigCameraFPAName(config->cameraName); // FPA-mosaicked name
          psString sky  = pmConfigCameraSkycellName(config->cameraName); // Skycell name

          // Just in case we weren't given the root name
          psFree(keep->data[i]);
          keep->data[i] = root;

          psArrayAdd(keep, 1, chip);
          psArrayAdd(keep, 1, fpa);
          psArrayAdd(keep, 1, sky);

          psFree(chip);
          psFree(fpa);
          psFree(sky);
      }

      bool result = configCull(cameras, keep); // Result of culling
      psFree(keep);

      return result;
}


bool pmConfigDump(const pmConfig *config, const char *filename)
{
    PS_ASSERT_PTR_NON_NULL(config, false);
    PS_ASSERT_STRING_NON_EMPTY(filename, false);

    psString resolved = pmConfigConvertFilename(filename, config, true, false); // Resolved filename
    if (!resolved) {
        psError(psErrorCodeLast(), false, "Unable to create file for configuration dump: %s", filename);
        return false;
    }

    // check for Metadata compression options:
    char *compressMode = NULL;
    bool status = false;
    if (config->camera) {
	compressMode = psMetadataLookupStr(&status, config->camera, "METADATA.COMPRESSION");
    }

    if (!psMetadataConfigWrite(config->user, resolved, compressMode)) {
        psError(psErrorCodeLast(), false, "Unable to dump configuration to %s", filename);
        psFree(resolved);
        return false;
    }

    psFree(resolved);

    return true;
}
