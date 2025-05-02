#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <pslib.h>

#include "pmConfig.h"
#include "pmConfigMask.h"
#include "pmHDU.h"
#include "pmFPA.h"

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
// File-static (private) functions
//////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Move to the appropriate extension in FITS file for HDU
static bool hduMove(pmHDU *hdu,         // HDU with extname
                    psFits *fits        // FITS file in which to move
                   )
{
    // Deal with the PHU case
    if (hdu->blankPHU || !hdu->extname) {
        if (!psFitsMoveExtNum(fits, 0, false)) {
            psError(PS_ERR_IO, false, "Unable to move to primary header!\n");
            return false;
        }
        return true;
    }

    if (!psFitsMoveExtName(fits, hdu->extname)) {
        psError(PS_ERR_IO, false, "Unable to move to extension %s\n", hdu->extname);
        return false;
    }

    return true;
}

static void hduFree(pmHDU *hdu)
{
    psFree(hdu->extname);
    psFree(hdu->format);
    psFree(hdu->header);
    psFree(hdu->images);
    psFree(hdu->variances);
    psFree(hdu->masks);
}


//////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Public functions
//////////////////////////////////////////////////////////////////////////////////////////////////////////////

pmHDU *pmHDUAlloc(const char *extname)
{
    pmHDU *hdu = psAlloc(sizeof(pmHDU));
    psMemSetDeallocator(hdu, (psFreeFunc)hduFree);

    if (!extname || strlen(extname) == 0) {
        hdu->blankPHU = true;
        hdu->extname = NULL;
    } else {
        hdu->blankPHU = false;
        hdu->extname = psStringCopy(extname);
    }
    hdu->format  = NULL;
    hdu->header  = NULL;
    hdu->images  = NULL;
    hdu->variances = NULL;
    hdu->masks   = NULL;

    return hdu;
}

bool psMemCheckHDU(psPtr ptr)
{
    PS_ASSERT_PTR(ptr, false);
    return ( psMemGetDeallocator(ptr) == (psFreeFunc) hduFree);
}


bool pmHDUReadHeader(pmHDU *hdu, psFits *fits)
{
    PS_ASSERT_PTR_NON_NULL(hdu, false);
    PS_ASSERT_PTR_NON_NULL(fits, false);

    // Move to the appropriate extension
    psTrace("psModules.camera", 5, "Moving to extension %s...\n", hdu->extname);
    if (!hduMove(hdu, fits)) {
        return false;
    }

    psTrace("psModules.camera", 5, "Reading the header...\n");

    // The header may already exist (e.g., from doing concept writing at the PHU level) so we need to be
    // careful.  We read into a separate container and copy that over the top of anything that's already read.
    psMetadata *header = psFitsReadHeader(NULL, fits);
    if (!header) {
        psError(PS_ERR_IO, false, "Unable to read header for extension %s\n", hdu->extname);
        return false;
    }

    if (!hdu->header) {
        hdu->header = header;
        return true;
    }

    psMetadataIterator *iter = psMetadataIteratorAlloc(header, PS_LIST_HEAD, NULL); // Iterator
    psMetadataItem *item;           // Item from iteration
    while ((item = psMetadataGetAndIncrement(iter))) {
        const char *name = item->name; // Name of item
        if (psMetadataLookup(hdu->header, name)) {
            // It exists; clobber
            psMetadataRemoveKey(hdu->header, name);
        }
        psMetadataAddItem(hdu->header, item, PS_LIST_TAIL, 0);
    }
    psFree(iter);
    psFree(header);

    return true;
}

// Read an HDU from a FITS file
// XXX: Add a region specifier?
bool hduRead(pmHDU *hdu,                // HDU to write
             psArray **images,          // Images into which to read
             psFits *fits               // FITS file to read
            )
{
    assert(hdu);
    assert(images);
    assert(fits);

    // Read the header; includes the move
    if (!pmHDUReadHeader(hdu, fits)) {
        return false;
    }

    if (hdu->blankPHU) {
        // Done already!
        return true;
    }

    if (*images) {
        psWarning("HDU %s has already been read --- overwriting.\n", hdu->extname);
        psFree(*images);                // Blow away anything existing
    }
    psTrace("psModules.camera", 5, "Reading the pixels...\n");
    *images = psFitsReadImageCube(fits, psRegionSet(0,0,0,0));
    if (!*images) {
        psError(PS_ERR_IO, false, "Unable to read pixels for extension %s\n", hdu->extname);
        return false;
    }
    return true;
}

bool pmHDURead(pmHDU *hdu, psFits *fits)
{
    PS_ASSERT_PTR_NON_NULL(hdu, false);
    PS_ASSERT_PTR_NON_NULL(fits, false);

    return hduRead(hdu, &hdu->images, fits);
}

bool pmHDUReadMask(pmHDU *hdu, psFits *fits)
{
    PS_ASSERT_PTR_NON_NULL(hdu, false);
    PS_ASSERT_PTR_NON_NULL(fits, false);

    return hduRead(hdu, &hdu->masks, fits);
}

bool pmHDUReadVariance(pmHDU *hdu, psFits *fits)
{
    PS_ASSERT_PTR_NON_NULL(hdu, false);
    PS_ASSERT_PTR_NON_NULL(fits, false);

    return hduRead(hdu, &hdu->variances, fits);
}

// Write an HDU to a FITS file
static bool hduWrite(pmHDU *hdu,        // HDU to write
                     const psArray *images, // Images to write
                     const psArray *masks, // Masks to use when writing
                     psImageMaskType maskVal,// Value to mask
                     psFits *fits       // FITS file to which to write
                    )
{
    assert(hdu);
    assert(fits);

    psTrace("psModules.camera", 7, "Writing HDU %s\n", hdu->extname);

    if (!images && !hdu->header) {
        psWarning("Nothing to write for HDU %s\n", hdu->extname);
        return false;
    }

    // Preserve the extension name, if it's the PHU
    char *extname = hdu->extname;       // The name of the extension
    if (!extname && hdu->header) {
        bool mdok = true;               // Status of MD lookup
        extname = psMetadataLookupStr(&mdok, hdu->header, "EXTNAME");
        if (!mdok || !extname || strlen(extname) == 0) {
            extname = "";
        }
    }

    // Make sure it's recognisable as what it's supposed to be
    if (!hdu->header) {
        hdu->header = psMetadataAlloc();
    }
    if (!pmConfigConformHeader(hdu->header, hdu->format)) {
        psError(PS_ERR_UNKNOWN, false, "Unable to conform header to format.\n");
        return false;
    }

    // Only a header
    if (!images && !psFitsWriteBlank(fits, hdu->header, extname)) {
        psError(PS_ERR_IO, false, "Unable to write header for extension %s\n", extname);
        return false;
    }
    
    if (images) {
	psTrace("psModules.camera", 9, "Writing pixels for %s\n", hdu->extname);
	if (!psFitsWriteImageCubeWithMask(fits, hdu->header, images, masks, maskVal, extname)) {
	    psError(PS_ERR_IO, false, "Unable to write image to extension %s\n", hdu->extname);
            return false;
	}
    }
    return true;
}

// XXX: Add a region specifier?
bool pmHDUWrite(pmHDU *hdu, psFits *fits, const pmConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(hdu, false);
    PS_ASSERT_PTR_NON_NULL(fits, false);

    psImageMaskType maskVal = pmConfigMaskGet("MASK.VALUE", config); // Value to mask
    return hduWrite(hdu, hdu->images, hdu->masks, maskVal, fits);
}

bool pmHDUWriteMask(pmHDU *hdu, psFits *fits, const pmConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(hdu, false);
    PS_ASSERT_PTR_NON_NULL(fits, false);

    // We don't supply a mask because we're writing the mask!
    return hduWrite(hdu, hdu->masks, NULL, 0, fits);
}

bool pmHDUWriteVariance(pmHDU *hdu, psFits *fits, const pmConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(hdu, false);
    PS_ASSERT_PTR_NON_NULL(fits, false);

    psImageMaskType maskVal = pmConfigMaskGet("MASK.VALUE", config); // Value to mask
    return hduWrite(hdu, hdu->variances, hdu->masks, maskVal, fits);
}

bool pmHDUWriteIdentifiers(pmHDU *hdu, psS64 imageId, psS64 sourceId)
{
    PS_ASSERT_PTR_NON_NULL(hdu, false);

    // XXX Get header keyword name from camera configuration

    if (imageId) {
        psMetadataAddS64(hdu->header, PS_LIST_TAIL, "IMAGEID", PS_META_REPLACE, "Image identifier", imageId);
    }
    if (sourceId) {
        psMetadataAddS64(hdu->header, PS_LIST_TAIL, "SOURCEID", PS_META_REPLACE, "Source identifier", sourceId);
    }
    return true;
}

bool pmHDUReadIdentifiers(psS64 *imageId, psS64 *sourceId, const pmHDU *hdu)
{
    PS_ASSERT_PTR_NON_NULL(hdu, false);

    // XXX Get header keyword name from camera configuration

    bool imageOK, sourceOK;             // Status of MD lookups
    *imageId = psMetadataLookupS64(&imageOK, hdu->header, "IMAGEID");
    *sourceId = psMetadataLookupS64(&sourceOK, hdu->header, "SOURCEID");

    return imageOK && sourceOK;
}
