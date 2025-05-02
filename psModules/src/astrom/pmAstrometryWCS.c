/** @file  pmAstrometryWCS.c
 *
 *  @brief functions to convert FITS WCS keywords to / from pmFPA structures
 *
 *  @ingroup Astrometry
 *
 *  @author EAM, IfA
 *
 *  @version $Revision: 1.35 $ $Name: not supported by cvs2svn $
 *  @date $Date: 2009-02-10 20:55:47 $
 *
 *  Copyright 2006 Institute for Astronomy, University of Hawaii
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <strings.h>
#include <string.h>
#include <pslib.h>

#include "pmHDU.h"
#include "pmFPA.h"
#include "pmFPAExtent.h"
#include "pmAstrometryWCS.h"
#include "pmAstrometryUtils.h"
#include "pmAstrometryRegions.h"

// the following functions support coordinate transformations direcly related to the FITS WCS
// keywords.  The FITS WCS allows for only a single level of transformation, thus it is not
// appropriate for mosaic astrometry consisting of telescope distortion plus chip terms.
// Below, we support the Elixir convention of using two connected FITS headers to define two
// levels of coordinate transformation.  In the pmFPA structure, the projection, distortion,
// and FPA-to-Chip transformations are carried independently.  NOTE: The FITS WCS keywords do
// not represent a simple polynomial.  Instead, they have no constant term, and the coordinates
// are corrected to a reference pixel before the polynomial transformation is applied.

// interpret header WCS (only handles traditional WCS for the moment)
// pixelScale is microns per pixel
bool pmAstromReadWCS (pmFPA *fpa, pmChip *chip, const psMetadata *header, double pixelScale)
{
    pmAstromWCS *wcs = pmAstromWCSfromHeader (header);
    if (!wcs) {
        return false;
    }

    bool status = pmAstromWCStoFPA (fpa, chip, wcs, pixelScale);

    psFree (wcs);
    return status;
}

// convert toFPA / toSky components to pmAstromWCS
// tolerance is convergence for inversion of non-linear terms in pixels
bool pmAstromWriteWCS (psMetadata *header, const pmFPA *fpa, const pmChip *chip, double tol)
{
    pmAstromWCS *wcs = pmAstromWCSfromFPA(fpa, chip, tol);
    if (!wcs) return false;

    pmAstromWCStoHeader (header, wcs);

    psFree (wcs);
    return true;
}

// interpret chip header WCS as bilevel chip components
bool pmAstromReadBilevelChip (pmChip *chip, const psMetadata *header)
{
    pmAstromWCS *wcs = pmAstromWCSfromHeader (header);
    if (!wcs) {
        return false;
    }

    bool status = pmAstromWCSBileveltoChip (chip, wcs);

    psFree (wcs);
    return status;
}

// convert toFPA / toSky components to traditional WCS
// we require the header to have NAXIS1,NAXIS2, the field of the FPA
// the center of the TPA/Sky projection is 0.5*(NAXIS1,NAXIS2)
bool pmAstromReadBilevelMosaic (pmFPA *fpa, const psMetadata *header)
{
    pmAstromWCS *wcs = pmAstromWCSfromHeader (header);
    if (!wcs) {
        psError(PS_ERR_UNKNOWN, false, "failure to determine WCS terms from header");
        return false;
    }

    bool status1 = false;
    bool status2 = false;
    int Nx = psMetadataLookupS32 (&status1, header, "NAXIS1");
    int Ny = psMetadataLookupS32 (&status2, header, "NAXIS2");

    if (!status1 || !status2) {
        Nx = psMetadataLookupS32 (&status1, header, "IMNAXIS1");
        Ny = psMetadataLookupS32 (&status2, header, "IMNAXIS2");
    }

    if (!status1 || !status2) {
        Nx = psMetadataLookupS32 (&status1, header, "ZNAXIS1");
        Ny = psMetadataLookupS32 (&status2, header, "ZNAXIS2");
    }

    if (!status1 || !status2) {
        psFree (wcs);
        psError(PS_ERR_UNKNOWN, false, "missing required FPA size in header");
        return false;
    }

    psRegion region = psRegionSet (-0.5*Nx, +0.5*Nx, -0.5*Ny, +0.5*Ny);
    bool status = pmAstromWCSBileveltoFPA (fpa, wcs, region);

    psFree (wcs);
    return status;
}

// convert chip->toFPA components to bilevel WCS
bool pmAstromWriteBilevelChip (psMetadata *header, const pmChip *chip, double tol)
{
    pmAstromWCS *wcs = pmAstromWCSBilevelChipFromFPA (chip, tol);
    if (!wcs) {
        psError(PS_ERR_UNKNOWN, false, "failure to determine WCS terms from fpa");
        return false;
    }

    pmAstromWCStoHeader (header, wcs);

    psFree (wcs);
    return true;
}


// convert fpa->toTPA, fpa->toSky components to bilevel WCS
bool pmAstromWriteBilevelMosaic (psMetadata *header, const pmFPA *fpa, double tol)
{
    pmAstromWCS *wcs = pmAstromWCSBilevelMosaicFromFPA (fpa, tol);
    if (!wcs) {
        psError(PS_ERR_UNKNOWN, false, "failure to determine WCS terms from fpa");
        return false;
    }

    // we need to specify the dimensions of the FPA
    // if we have chips defined, we can do
    psRegion *region = pmAstromFPAExtent (fpa);
    int Nx = region->x1 - region->x0;
    int Ny = region->y1 - region->y0;
    psMetadataAddS32 (header, PS_LIST_TAIL, "IMNAXIS1", PS_META_REPLACE, "Mosaic Dimensions", Nx);
    psMetadataAddS32 (header, PS_LIST_TAIL, "IMNAXIS2", PS_META_REPLACE, "Mosaic Dimensions", Ny);

    pmAstromWCStoHeader (header, wcs);

    psFree (region);
    psFree (wcs);
    return true;
}

// convert coordinates from chip to sky using a pmAstromWCS structure
bool pmAstromWCStoSky (psSphere *sky, pmAstromWCS *wcs, psPlane *chip)
{

    if (chip == NULL)
        return false;
    if (sky == NULL)
        return false;
    if (wcs == NULL)
        return false;

    psPlane *Chip = psPlaneAlloc();
    psPlane *FP = psPlaneAlloc();

    Chip->x = chip->x - wcs->crpix1;
    Chip->y = chip->y - wcs->crpix2;

    psPlaneTransformApply (FP, wcs->trans, Chip);
    psDeproject (sky, FP, wcs->toSky); // find the RA,DEC coord of the focal-plane coordinate

    psFree (Chip);
    psFree (FP);
    return true;
}

// convert coordinates from sky to chip using a pmAstromWCS structure
bool pmAstromWCStoChip (psPlane *chip, pmAstromWCS *wcs, psSphere *sky)
{

    if (chip == NULL)
        return false;
    if (sky == NULL)
        return false;
    if (wcs == NULL)
        return false;

    psError(PS_ERR_UNKNOWN, true, "not yet implemented: needs to invert the transformation");
    return false;

    psPlane *Chip = psPlaneAlloc();
    psPlane *FP = psPlaneAlloc();

    psProject (FP, sky, wcs->toSky); // find the RA,DEC coord of the focal-plane coordinate

    // XXX I actually need the inverse of wcs->transform at this point
    psPlaneTransformApply (Chip, wcs->trans, FP);

    chip->x = Chip->x + wcs->crpix1;
    chip->y = Chip->y + wcs->crpix2;

    psFree (Chip);
    psFree (FP);
    return true;
}

// interpret header WCS keywords (valid for bilevel and traditional WCS)
pmAstromWCS *pmAstromWCSfromHeader (const psMetadata *header)
{
    psProjectionType type;
    bool status, pcKeys, cdKeys, isPoly;
    char name[PS_SMALLWORD]; // used to store FITS keyword below (always < 8, so 16 should be safe!)

    // interpret header data, convert to crval(i), etc
    char *ctype = psMetadataLookupPtr (&status, header, "CTYPE2");
    if (!status) {
        psLogMsg ("psastro", 5, "warning: no WCS metadata in header\n");
        return NULL;
    }

    // determine projection type
    // XXX there are two indications for higher-order terms: the type (DIS,WRP,PLY,ZPL) and
    // the value of NPLYTERM.
    type = psProjectTypeFromString (ctype);
    if (type == PS_PROJ_NONE) {
        psLogMsg ("psastro", 2, "warning: unknown projection type %s\n", ctype);
        return NULL;
    }

    // what type of WCS keywords are available?
    // XXX add check for CROTA2
    int fitOrder = psMetadataLookupS32 (&isPoly, header, "NPLYTERM");
    psMetadataLookupF64 (&pcKeys, header, "PC001001");
    psMetadataLookupF64 (&cdKeys, header, "CD1_1");

    if (cdKeys && pcKeys) {
        // XXX make this an option
        psLogMsg ("psastro", 5, "warning: both CDi_j and PC00i00j defined in headers, using PC00i00j terms\n");
    }
    if (!cdKeys && !pcKeys) {
        psError(PS_ERR_UNKNOWN, true, "missing both CDi_j and PC00i00j WCS terms");
        // XXX we could default here to RA, DEC, ROTANGLE
        return NULL;
    }
    if (isPoly) {
        if (!pcKeys) {
            psError(PS_ERR_UNKNOWN, true, "polynomial terms defined, but missing PC00i00j WCS terms");
            return NULL;
        }
        if (fitOrder == 0)
            fitOrder = 1;
        if ((fitOrder > 3) || (fitOrder < 1)) {
            psError(PS_ERR_UNKNOWN, true, "NPLYTERM value undefined: %d", fitOrder);
            return NULL;
        }
    } else {
        fitOrder = 1;
    }

    pmAstromWCS *wcs = pmAstromWCSAlloc (fitOrder, fitOrder);

    // construct a transformation from X,Y in pixels to L,M in pixels
    // NOTE that the WCS keywords convert X,Y to degrees first (using cdelt1,2)
    // and then define a transformation from degrees to degrees

    wcs->crval1 = psMetadataLookupF64 (&status, header, "CRVAL1");
    wcs->crval2 = psMetadataLookupF64 (&status, header, "CRVAL2");
    wcs->crpix1 = psMetadataLookupF64 (&status, header, "CRPIX1");
    wcs->crpix2 = psMetadataLookupF64 (&status, header, "CRPIX2");
    wcs->toSky = psProjectionAlloc (wcs->crval1*PM_RAD_DEG, wcs->crval2*PM_RAD_DEG, PM_RAD_DEG, PM_RAD_DEG, type);

    // XXX if type == ZPN, look for PV2_%d elements:
    if (type == PS_PROJ_ZPN) {
        psVector *maxRadial = psVectorAlloc (21, PS_TYPE_F64);
        for (int i = 0; i <= 20; i++) {
            char name[PS_BIGWORD];
            ps_snprintf_nowarn (name, PS_BIGWORD, "PV2_%d", i);

            maxRadial->data.F64[i] = 0.0;
            double value = psMetadataLookupF64 (&status, header, name);

            if (status) {
                maxRadial->data.F64[i] = value;
                maxRadial->n = i;
            }

            // PV2_1 is implicit if not present
            if ((i == 1) && !status) {
                maxRadial->data.F64[i] = 1.0;
                continue;
            }
        }
        maxRadial->n ++;
        wcs->toSky->radial = maxRadial;
    }

    // These aren't needed but having them empty is disconcerting
    strncpy(wcs->ctype2, ctype, PM_ASTROM_WCS_TYPE_SIZE-1);
    ctype = psMetadataLookupStr (&status, header, "CTYPE1");
    strncpy(wcs->ctype1, ctype, PM_ASTROM_WCS_TYPE_SIZE-1);
    wcs->ctype1[PM_ASTROM_WCS_TYPE_SIZE-1] = 0;
    wcs->ctype2[PM_ASTROM_WCS_TYPE_SIZE-1] = 0;

    // XXX I think this is wrong for linear proj

    // test the CDELTi varient
    if (pcKeys) {
        wcs->wcsCDkeys = 0;
        wcs->cdelt1 = psMetadataLookupF64 (&status, header, "CDELT1");
        wcs->cdelt2 = psMetadataLookupF64 (&status, header, "CDELT2");

        // test the CROTAi varient:
        // XXX double check lambda..
        double rotate = psMetadataLookupF64 (&status, header, "CROTA2");
        if (status) {
            wcs->trans->x->coeff[1][0] = +wcs->cdelt1 * cos(rotate*PM_RAD_DEG); // == PC1_1
            wcs->trans->x->coeff[0][1] = -wcs->cdelt2 * sin(rotate*PM_RAD_DEG); // == PC1_2
            wcs->trans->y->coeff[1][0] = +wcs->cdelt1 * sin(rotate*PM_RAD_DEG); // == PC2_1
            wcs->trans->y->coeff[0][1] = +wcs->cdelt2 * cos(rotate*PM_RAD_DEG); // == PC2_2
            return wcs;
        }

        // FITS WCS PCi,j has units of unity
        // wcs->trans has units of degrees/pixel
        wcs->trans->x->coeff[1][0] = wcs->cdelt1 * psMetadataLookupF64 (&status, header, "PC001001"); // == PC1_1
        wcs->trans->x->coeff[0][1] = wcs->cdelt2 * psMetadataLookupF64 (&status, header, "PC001002"); // == PC1_2
        wcs->trans->y->coeff[1][0] = wcs->cdelt1 * psMetadataLookupF64 (&status, header, "PC002001"); // == PC2_1
        wcs->trans->y->coeff[0][1] = wcs->cdelt2 * psMetadataLookupF64 (&status, header, "PC002002"); // == PC2_2

        if (isPoly) {
            // Elixir-style polynomial terms
            // XXX currently, Elixir/DVO cannot accept mixed orders
            for (int i = 0; i <= fitOrder; i++) {
                for (int j = 0; j <= fitOrder; j++) {
                    if (i + j < 2)
                        continue;
                    if (i + j > fitOrder) {
                        wcs->trans->x->coeffMask[i][j] = PS_POLY_MASK_SET;
                        wcs->trans->y->coeffMask[i][j] = PS_POLY_MASK_SET;
                        continue;
                    }
                    ps_snprintf_nowarn (name, PS_SMALLWORD, "PCA1X%1dY%1d", i, j);
                    wcs->trans->x->coeff[i][j] = pow(wcs->cdelt1, i) * pow(wcs->cdelt2, j) * psMetadataLookupF64 (&status, header, name);
                    ps_snprintf_nowarn (name, PS_SMALLWORD, "PCA2X%1dY%1d", i, j);
                    wcs->trans->y->coeff[i][j] = pow(wcs->cdelt1, i) * pow(wcs->cdelt2, j) * psMetadataLookupF64 (&status, header, name);
                }
            }
        }
        return wcs;
    }

    // test the CDi_j varient
    if (cdKeys) {
        wcs->wcsCDkeys = 1;

        wcs->trans->x->coeff[1][0] = psMetadataLookupF64 (&status, header, "CD1_1"); // == PC1_1
        wcs->trans->x->coeff[0][1] = psMetadataLookupF64 (&status, header, "CD1_2"); // == PC1_2
        wcs->trans->y->coeff[1][0] = psMetadataLookupF64 (&status, header, "CD2_1"); // == PC2_1
        wcs->trans->y->coeff[0][1] = psMetadataLookupF64 (&status, header, "CD2_2"); // == PC2_2
        wcs->cdelt1 = hypot (wcs->trans->x->coeff[1][0], wcs->trans->x->coeff[0][1]);
        wcs->cdelt2 = hypot (wcs->trans->y->coeff[1][0], wcs->trans->y->coeff[0][1]);
        return wcs;
    }
    psLogMsg ("psastro", 2, "warning: missing rotation matrix?\n");
    psFree (wcs);
    return NULL;
}

// convert wcs transformations into header WCS keywords (only handles traditional WCS for the moment)
// wcs->trans defines the transformation from pixels to degrees.
// wcs->cdelt1,2 carries the original pixels scale.
// XXX force PC00i00j to be normalized, or use cdelt1,2 to set the scale?
// here I've chosen to force the rotation matrix to be normalized
bool pmAstromWCStoHeader (psMetadata *header, const pmAstromWCS *wcs)
{
    char name[PS_SMALLWORD]; // used to store FITS keyword below (always < 8, so PS_SMALLWORD should be safe!)
    char *type;

    if (!wcs) return false;

    type = psProjectTypeToString (wcs->toSky->type, "RA--");
    psMetadataAddStr (header, PS_LIST_TAIL, "CTYPE1", PS_META_REPLACE, "", type);
    psFree (type);

    type = psProjectTypeToString (wcs->toSky->type, "DEC-");
    psMetadataAddStr (header, PS_LIST_TAIL, "CTYPE2", PS_META_REPLACE, "", type);
    psFree (type);

    psMetadataAddF64 (header, PS_LIST_TAIL, "CRVAL1", PS_META_REPLACE, "", wcs->toSky->R*PS_DEG_RAD);
    psMetadataAddF64 (header, PS_LIST_TAIL, "CRVAL2", PS_META_REPLACE, "", wcs->toSky->D*PS_DEG_RAD);

    psMetadataAddF64 (header, PS_LIST_TAIL, "CRPIX1", PS_META_REPLACE, "", wcs->crpix1);
    psMetadataAddF64 (header, PS_LIST_TAIL, "CRPIX2", PS_META_REPLACE, "", wcs->crpix2);

    if (wcs->toSky->type == PS_PROJ_ZPN) {
        psAssert (wcs->toSky->radial, "missing radial vector");
        for (int i = 0; i < wcs->toSky->radial->n; i++) {
            if (wcs->toSky->radial->data.F64[i] == 0.0) continue;
            ps_snprintf_nowarn (name, PS_SMALLWORD, "PV2_%d", i);
            psMetadataAddF64 (header, PS_LIST_TAIL, name, PS_META_REPLACE, "", wcs->toSky->radial->data.F64[i]);
        }
    }

    // XXX make it optional to write out CDi_j terms, or other versions
    // apply CDELT1,2 (degrees / pixel) to yield PCi,j terms of order unity
    if (!wcs->wcsCDkeys) {

        double cdelt1 = wcs->cdelt1;
        double cdelt2 = wcs->cdelt2;
        psMetadataAddF64 (header, PS_LIST_TAIL, "CDELT1", PS_META_REPLACE, "", cdelt1);
        psMetadataAddF64 (header, PS_LIST_TAIL, "CDELT2", PS_META_REPLACE, "", cdelt2);

        // test the PC00i00j varient:
        psMetadataAddF64 (header, PS_LIST_TAIL, "PC001001", PS_META_REPLACE, "", wcs->trans->x->coeff[1][0] / cdelt1); // == PC1_1
        psMetadataAddF64 (header, PS_LIST_TAIL, "PC001002", PS_META_REPLACE, "", wcs->trans->x->coeff[0][1] / cdelt2); // == PC1_2
        psMetadataAddF64 (header, PS_LIST_TAIL, "PC002001", PS_META_REPLACE, "", wcs->trans->y->coeff[1][0] / cdelt1); // == PC2_1
        psMetadataAddF64 (header, PS_LIST_TAIL, "PC002002", PS_META_REPLACE, "", wcs->trans->y->coeff[0][1] / cdelt2); // == PC2_2

        // Elixir-style polynomial terms
        // XXX currently, Elixir/DVO cannot accept mixed orders
        // XXX need to respect the masks
        // XXX is wcs->cdelt1,2 always consistent?
        int fitOrder = wcs->trans->x->nX;
        if (fitOrder > 1) {
            for (int i = 0; i <= fitOrder; i++) {
                for (int j = 0; j <= fitOrder; j++) {
                    if (i + j < 2)
                        continue;
                    if (i + j > fitOrder)
                        continue;
                    ps_snprintf_nowarn (name, PS_SMALLWORD, "PCA1X%1dY%1d", i, j);
                    psMetadataAddF64 (header, PS_LIST_TAIL, name, PS_META_REPLACE, "", wcs->trans->x->coeff[i][j] / pow(cdelt1, i) / pow(cdelt2, j));
                    ps_snprintf_nowarn (name, PS_SMALLWORD, "PCA2X%1dY%1d", i, j);
                    psMetadataAddF64 (header, PS_LIST_TAIL, name, PS_META_REPLACE, "", wcs->trans->y->coeff[i][j] / pow(cdelt1, i) / pow(cdelt2, j));
                }
            }
            psMetadataAddS32 (header, PS_LIST_TAIL, "NPLYTERM", PS_META_REPLACE, "", fitOrder);
        }

        // remove any existing 'CDi_j style' wcs keywords
        if (psMetadataLookup(header, "CD1_1")) {
            psMetadataRemoveKey(header, "CD1_1");
            psMetadataRemoveKey(header, "CD1_2");
            psMetadataRemoveKey(header, "CD2_1");
            psMetadataRemoveKey(header, "CD2_2");
        }

        // Remove 'CDi_jX' WCS keywords
        psString cd11 = psStringCopy("CD1_1 ");
        psString cd12 = psStringCopy("CD1_2 ");
        psString cd21 = psStringCopy("CD2_1 ");
        psString cd22 = psStringCopy("CD2_2 ");
        for (char extra = 'A'; extra <= 'Z'; extra++) {
            cd11[strlen(cd11)-1] = extra;
            if (psMetadataLookup(header, cd11)) {
                cd12[strlen(cd12)-1] = extra;
                cd21[strlen(cd21)-1] = extra;
                cd22[strlen(cd22)-1] = extra;
                psMetadataRemoveKey(header, cd11);
                psMetadataRemoveKey(header, cd12);
                psMetadataRemoveKey(header, cd21);
                psMetadataRemoveKey(header, cd22);
            }
        }
        psFree(cd11);
        psFree(cd12);
        psFree(cd21);
        psFree(cd22);


    } else {

        psMetadataAddF64 (header, PS_LIST_TAIL, "CD1_1", PS_META_REPLACE, "", wcs->trans->x->coeff[1][0]);
        psMetadataAddF64 (header, PS_LIST_TAIL, "CD1_2", PS_META_REPLACE, "", wcs->trans->x->coeff[0][1]);
        psMetadataAddF64 (header, PS_LIST_TAIL, "CD2_1", PS_META_REPLACE, "", wcs->trans->y->coeff[1][0]);
        psMetadataAddF64 (header, PS_LIST_TAIL, "CD2_2", PS_META_REPLACE, "", wcs->trans->y->coeff[0][1]);

        if (psMetadataLookup(header, "PC001001")) {
            psMetadataRemoveKey(header, "PC001001");
            psMetadataRemoveKey(header, "PC001002");
            psMetadataRemoveKey(header, "PC002001");
            psMetadataRemoveKey(header, "PC002002");
        }
    }

    return true;
}

// interpret header WCS (only handles traditional WCS for the moment)
// pixelScale is the pixel size in microns/pixel
bool pmAstromWCStoFPA (pmFPA *fpa, pmChip *chip, const pmAstromWCS *wcs, double pixelScale)
{
    psPlaneTransform *toFPA;

    int ExtraOrders = pmAstrometryGetExtraOrders();

    // create transformation with 0,0 reference pixel and units of degrees/pixel
    toFPA = psPlaneTransformSetCenter (NULL, wcs->trans, -wcs->crpix1, -wcs->crpix2);

    // modify scale of toFPA to have units of microns/pixel
    // cdelt1,2 has units of degree/pixel
    for (int i = 0; i <= toFPA->x->nX; i++) {
        for (int j = 0; j <= toFPA->x->nX; j++) {
            toFPA->x->coeff[i][j] *= pixelScale/wcs->cdelt1;
            toFPA->y->coeff[i][j] *= pixelScale/wcs->cdelt2;
        }
    }

    // pdelt1,2 has units of degree/micron
    double pdelt1 = wcs->cdelt1 / pixelScale;
    double pdelt2 = wcs->cdelt2 / pixelScale;

    // projection from TPA (linear microns) to SKY (radians)
    psProjection *toSky = psProjectionAlloc (wcs->toSky->R, wcs->toSky->D, PM_RAD_DEG*pdelt1, PM_RAD_DEG*pdelt2, wcs->toSky->type);
    toSky->radial = psMemIncrRefCounter (wcs->toSky->radial);

    if (fpa->toSky == NULL) {
        psFree(fpa->toTPA);
        psFree(fpa->fromTPA);
        fpa->toTPA = psPlaneTransformIdentity (1);
        fpa->fromTPA = psPlaneTransformIdentity (1);
        fpa->toSky = toSky;
    } else {

        // this section allows the loaded chip to be included in an fpa structure in which
        // other chips have already been loaded (ie, the fpa->toTPA, fpa->toSky components have
        // already been defined).  we have to adjust to match the existing transformation.

        if (fpa->toTPA == NULL)
            psAbort("projection defined, tangent-plane not defined");
        if (fpa->fromTPA == NULL)
            psAbort("projection defined, tangent-plane not defined");

        // convert from pixels on this chip to pixels on reference chip
        // rX has units of refpixels / pixel
        double rX = toSky->Xs / fpa->toSky->Xs;
        double rY = toSky->Ys / fpa->toSky->Ys;

        for (int i = 0; i <= toFPA->x->nX; i++) {
            for (int j = 0; j <= toFPA->x->nY; j++) {
                toFPA->x->coeff[i][j] *= rX;
                toFPA->y->coeff[i][j] *= rY;
            }
        }

        // apply the exiting fromTPA transformation to make the new toFPA consistent with the toTPA layter
        // XXX this only works if toTPA is at most a linear transformation
        psPlaneTransform *toFPAnew = psPlaneTransformAlloc(toFPA->x->nX, toFPA->x->nY, PS_POLYNOMIAL_ORD);
        for (int i = 0; i <= toFPA->x->nX; i++) {
            for (int j = 0; j <= toFPA->x->nY; j++) {
                double f1 = toFPA->x->coeffMask[i][j] ? 0.0 : fpa->fromTPA->x->coeff[1][0]*toFPA->x->coeff[i][j];
                double f2 = toFPA->y->coeffMask[i][j] ? 0.0 : fpa->fromTPA->x->coeff[0][1]*toFPA->y->coeff[i][j];
                toFPAnew->x->coeff[i][j] = f1 + f2;

                double g1 = toFPA->x->coeffMask[i][j] ? 0.0 : fpa->fromTPA->y->coeff[1][0]*toFPA->x->coeff[i][j];
                double g2 = toFPA->y->coeffMask[i][j] ? 0.0 : fpa->fromTPA->y->coeff[0][1]*toFPA->y->coeff[i][j];
                toFPAnew->y->coeff[i][j] = g1 + g2;
            }
        }
        toFPAnew->x->coeff[0][0] += fpa->fromTPA->x->coeff[0][0];
        toFPAnew->y->coeff[0][0] += fpa->fromTPA->y->coeff[0][0];

        psFree (toFPA);
        toFPA = toFPAnew;

        // adjust reference pixel for new toSky reference coordinate
        // find the FPA coordinate of 0,0 for this chip.
        psPlane *fpOld = psPlaneAlloc();
        psPlane *fpNew = psPlaneAlloc();
        psPlane *tp = psPlaneAlloc();
        psSphere *sky = psSphereAlloc();

        sky->r = toSky->R;
        sky->d = toSky->D;
        psProject (tp, sky, fpa->toSky); // find the focal-plane coord of this RA,DEC coord using the ref chip projection
        psPlaneTransformApply (fpOld, fpa->fromTPA, tp);

        sky->r = fpa->toSky->R;
        sky->d = fpa->toSky->D;
        psProject (tp, sky, fpa->toSky); // find the focal-plane coord of this RA,DEC coord using the ref chip projection
        psPlaneTransformApply (fpNew, fpa->fromTPA, tp);

        toFPA->x->coeff[0][0] -= fpNew->x - fpOld->x;
        toFPA->y->coeff[0][0] -= fpNew->y - fpOld->y;

        psFree (sky);
        psFree (tp);
        psFree (fpOld);
        psFree (fpNew);

        psFree (toSky);
    }

    // free an existing toFPA structure
    psFree (chip->toFPA);
    assert (chip->toFPA == NULL);
    chip->toFPA = toFPA;

    // determine the inverse transformation: we need the chip pixels covered by this transform
    psRegion *region = pmChipPixels (chip);

    // as of r40806, psPlaneTransformInvert supplies the extra order (if non-linear)
    psFree (chip->fromFPA);
    chip->fromFPA = psPlaneTransformInvert(NULL, chip->toFPA, *region, 50, ExtraOrders);
    psFree (region);

    // XXX if the inversion fails, we probably do not have a valid transform anyway
    if (!chip->fromFPA) {
        psWarning ("failed to find a valid transformation");
        psFree (chip->toFPA);
        return false;
    }

    // this can take a very long time...
    while (fpa->toSky->R < 0)
        fpa->toSky->R += 2.0*M_PI;
    while (fpa->toSky->R > 2.0*M_PI)
        fpa->toSky->R -= 2.0*M_PI;

    fpa->wcsCDkeys = wcs->wcsCDkeys;

    psTrace ("psastro", 5, "toFPA: %f %f  (%f,%f),(%f,%f)\n",
             chip->toFPA->x->coeff[0][0], chip->toFPA->y->coeff[0][0],
             chip->toFPA->x->coeff[1][0], chip->toFPA->x->coeff[0][1],
             chip->toFPA->y->coeff[1][0], chip->toFPA->y->coeff[0][1]);

    psTrace ("psastro", 5, "frFPA: %f %f  (%f,%f),(%f,%f)\n",
             chip->fromFPA->x->coeff[0][0], chip->fromFPA->y->coeff[0][0],
             chip->fromFPA->x->coeff[1][0], chip->fromFPA->x->coeff[0][1],
             chip->fromFPA->y->coeff[1][0], chip->fromFPA->y->coeff[0][1]);

    return true;
}

// convert a pmAstromWCS structure representing a bilevel chip into corresponding chip elements
bool pmAstromWCSBileveltoChip (pmChip *chip, const pmAstromWCS *wcs)
{
    /* we convert wcs->trans to toFPA, which is different from wcs->trans in 3 important ways:
     * 1) the output is in pixel (not degrees): divide by cdelt1,2 raised to an appropriate power
     * 2) X,Y are applied directly, without an applied Xo,Yo offset
     * 3) there is an allowed Lo,Mo term ([0][0] coefficients)
     */

    int ExtraOrders = pmAstrometryGetExtraOrders();

    psFree (chip->toFPA);
    if ((fabs(wcs->crpix1) > 0.01) || (fabs(wcs->crpix2) > 0.01)) {
      chip->toFPA = psPlaneTransformSetCenter (NULL, wcs->trans, -wcs->crpix1, -wcs->crpix2);
    } else {
      chip->toFPA = psPlaneTransformAlloc(wcs->trans->x->nX, wcs->trans->x->nY, PS_POLYNOMIAL_ORD);

      // copy the toFPA x,y, transformations to the wcs version
      chip->toFPA->x = psPolynomial2DCopy (chip->toFPA->x, wcs->trans->x);
      chip->toFPA->y = psPolynomial2DCopy (chip->toFPA->y, wcs->trans->y);

      // these need to be set based on crval1,2
      chip->toFPA->x->coeff[0][0] = wcs->crval1;
      chip->toFPA->y->coeff[0][0] = wcs->crval2;
    }

    // determine the inverse transformation: we need the chip pixels covered by this transform
    psRegion *region = pmChipPixels (chip);

    // as of r40806, psPlaneTransformInvert supplies the extra order (if non-linear)
    psFree (chip->fromFPA);
    chip->fromFPA = psPlaneTransformInvert(NULL, chip->toFPA, *region, 50, ExtraOrders);
    psFree (region);

    return true;
}

// convert a pmAstromWCS structure representing a bilevel mosaic into corresponding fpa elements
bool pmAstromWCSBileveltoFPA (pmFPA *fpa, const pmAstromWCS *wcs, psRegion region)
{
    // projection from TPA (microns) to SKY (radians)
    // cdelt1,2 has units of degrees/micron
    fpa->toSky = psProjectionAlloc (wcs->toSky->R, wcs->toSky->D, wcs->cdelt1*PM_RAD_DEG, wcs->cdelt2*PM_RAD_DEG, wcs->toSky->type);
    fpa->toSky->radial = psMemIncrRefCounter (wcs->toSky->radial);

    // create transformation with 0,0 reference pixel
    fpa->toTPA = psPlaneTransformSetCenter (NULL, wcs->trans, -wcs->crpix1, -wcs->crpix2);

    // convert fpa->toTPA to units of unity (microns/micron)
    for (int i = 0; i <= fpa->toTPA->x->nX; i++) {
        for (int j = 0; j <= fpa->toTPA->x->nY; j++) {
            fpa->toTPA->x->coeff[i][j] /= wcs->cdelt1;
            fpa->toTPA->y->coeff[i][j] /= wcs->cdelt2;
        }
    }

    // the transformation used the region to define the inversion grid
    // the region defines the FPA pixels covered by the tranformation
    psFree (fpa->fromTPA);
    int ExtraOrders = pmAstrometryGetExtraOrders();  // This is the number of orders that should be added.
    fpa->fromTPA = psPlaneTransformInvert(NULL, fpa->toTPA, region, 50, ExtraOrders);
    return true;
}

// convert toFPA / toSky components to pmAstromWCS
// tolerance is allowed error in center solution in pixels
pmAstromWCS *pmAstromWCSfromFPA (const pmFPA *fpa, const pmChip *chip, double tol)
{
    PS_ASSERT_PTR_NON_NULL(fpa, NULL);
    PS_ASSERT_PTR_NON_NULL(chip, NULL);
    PS_ASSERT_PTR_NON_NULL(chip->toFPA, NULL);
    PS_ASSERT_PTR_NON_NULL(fpa->toTPA, NULL);

    // XXX require chip->toFPA->x->nX == chip->toFPA->x->nY
    // XXX require chip->toFPA->y->nX == chip->toFPA->y->nY
    // XXX require chip->toFPA->x->nX == chip->toFPA->y->nX
    // XXX require chip->toFPA->nX == 1,2,3

    // technically, we can have a plate scale here (fpa->toTPA:dx,dy != 1)
    // XXX not really: toTPA needs to have unity scale for distortion fitting function
    // if (!psPlaneTransformIsDiagonal (fpa->toTPA))
    // psAbort("invalid TPA transformation");

    // create a temporary transform which combines toTPA and toFPA.  allow toTPA to have 0th
    // and 1st order terms

    // XXX require fpa->toTPA->x->nX == fpa->toTPA->x->nY
    // XXX require fpa->toTPA->y->nX == fpa->toTPA->y->nY
    // XXX require fpa->toTPA->x->nX == fpa->toTPA->y->nX
    // XXX require fpa->toTPA->x->coeffMask[1][1]
    // XXX require fpa->toTPA->y->coeffMask[1][1]
    // XXX require fpa->toTPA->nX == 1

    psPlaneTransform *toTPA = psPlaneTransformAlloc(chip->toFPA->x->nX, chip->toFPA->x->nY, PS_POLYNOMIAL_ORD);

    for (int i = 0; i <= toTPA->x->nX; i++) {
        for (int j = 0; j <= toTPA->x->nY; j++) {
            double f1 = chip->toFPA->x->coeffMask[i][j] ? 0.0 : fpa->toTPA->x->coeff[1][0]*chip->toFPA->x->coeff[i][j];
            double f2 = chip->toFPA->y->coeffMask[i][j] ? 0.0 : fpa->toTPA->x->coeff[0][1]*chip->toFPA->y->coeff[i][j];
            toTPA->x->coeff[i][j] = f1 + f2;

            double g1 = chip->toFPA->x->coeffMask[i][j] ? 0.0 : fpa->toTPA->y->coeff[1][0]*chip->toFPA->x->coeff[i][j];
            double g2 = chip->toFPA->y->coeffMask[i][j] ? 0.0 : fpa->toTPA->y->coeff[0][1]*chip->toFPA->y->coeff[i][j];
            toTPA->y->coeff[i][j] = g1 + g2;
        }
    }
    toTPA->x->coeff[0][0] += fpa->toTPA->x->coeff[0][0];
    toTPA->y->coeff[0][0] += fpa->toTPA->y->coeff[0][0];

    pmAstromWCS *wcs = pmAstromWCSAlloc(toTPA->x->nX, toTPA->x->nY);

    // convert projection from FPA to SKY into wcs projection (degrees to radians)
    wcs->toSky = psProjectionAlloc (fpa->toSky->R, fpa->toSky->D, PM_RAD_DEG, PM_RAD_DEG, fpa->toSky->type);
    wcs->toSky->radial = psMemIncrRefCounter (fpa->toSky->radial);

    wcs->crval1 = fpa->toSky->R*PS_DEG_RAD;
    wcs->crval2 = fpa->toSky->D*PS_DEG_RAD;

    // generate a transform that has 0.0 rotation:
    // get the current posangle of the ref chip
    // XXX average angles for x and y...
    float angle = atan2 (toTPA->y->coeff[1][0], toTPA->x->coeff[1][0]);
    // fprintf (stderr, "angle: %f\n", angle*PS_DEG_RAD);
    psPlaneTransform *tpa1 = psPlaneTransformRotate (NULL, toTPA, angle);

    // given transformation, solve for coordinates which yields output coordinates of 0,0
    psPlane *center = psPlaneTransformGetCenter (tpa1, tol);
    if (!center) {
        psError(PS_ERR_UNKNOWN, false, "Unable to solve for TPA center.");
        psFree (toTPA);
        psFree (tpa1);
        psFree (wcs);
        return NULL;
    }

    // generate transform with the original orientation (does this rotate about 'center'?)
    psPlaneTransform *tpa2 = psPlaneTransformRotate (NULL, tpa1, -1.0*angle);

    // prove that the center coordinates give 0,0:
    // float Xo = psPolynomial2DEval (tpa1->x, center->x, center->y);
    // float Yo = psPolynomial2DEval (tpa1->x, center->x, center->y);
    // fprintf (stderr, "tpa1: Xo, Yo: %f, %f\n", Xo, Yo);

    // prove that the center coordinates give 0,0:
    // Xo = psPolynomial2DEval (tpa2->x, center->x, center->y);
    // Yo = psPolynomial2DEval (tpa2->x, center->x, center->y);
    // fprintf (stderr, "tpa2: Xo, Yo: %f, %f\n", Xo, Yo);

    // create wcs transform from toFPA, resulting transformation has units of microns/pixel
    // adjust wcs transform to use center as reference coordinate
    psPlaneTransformSetCenter (wcs->trans, tpa2, center->x, center->y);

    // calculated center is crpix1,2
    wcs->crpix1 = center->x;
    wcs->crpix2 = center->y;
    psFree (center);
    psFree (tpa1);
    psFree (tpa2);

    // pdelt1,2 has units of degrees/micron
    double pdelt1 = fpa->toSky->Xs * PS_DEG_RAD;
    double pdelt2 = fpa->toSky->Ys * PS_DEG_RAD;

    // convert wcs->trans to a matrix with units of degrees/pixel
    for (int i = 0; i <= wcs->trans->x->nX; i++) {
        for (int j = 0; j <= wcs->trans->x->nY; j++) {
            wcs->trans->x->coeff[i][j] *= pdelt1;
            wcs->trans->y->coeff[i][j] *= pdelt2;
        }
    }

    // cdelt1,2 has units of degrees/pixel
    wcs->cdelt1 = hypot (wcs->trans->x->coeff[1][0], wcs->trans->x->coeff[0][1]);
    wcs->cdelt2 = hypot (wcs->trans->y->coeff[1][0], wcs->trans->y->coeff[0][1]);

    wcs->wcsCDkeys = fpa->wcsCDkeys;
    psFree (toTPA);

    return wcs;
}

/* the bilevel astrometry description consists of a polynomial warping from
   chip coordinates to FPA coordinates (coords->ctype = LIN---WRP), followed
   by a polynomial representation of the telescope distortion + the projection
   (coords->ctype = RA---DIS).
*/

// convert the chip-level toFPA to a wcs polynomial transformation.  the pmAstromWCS
// structure represents a single layer transformation (e.g., RA-TAN, RA-WRP).  Here we are
// converting the chip-level to a WRP projection in the structure.  Later, this will be
// converted to the WCS keywords

pmAstromWCS *pmAstromWCSBilevelChipFromFPA (const pmChip *chip, double tol)
{
    // XXX require chip->toFPA->x->nX == chip->toFPA->x->nY
    // XXX require chip->toFPA->y->nX == chip->toFPA->y->nY
    // XXX require chip->toFPA->x->nX == chip->toFPA->y->nX
    // XXX require chip->toFPA->nX == 1,2,3

    // convert chip->toFPA to wcs format (WRP)
    pmAstromWCS *wcs = pmAstromWCSAlloc(chip->toFPA->x->nX, chip->toFPA->x->nY);

    // copy the toFPA x,y, transformations to the wcs version
    wcs->trans->x = psPolynomial2DCopy (wcs->trans->x, chip->toFPA->x);
    wcs->trans->y = psPolynomial2DCopy (wcs->trans->y, chip->toFPA->y);

    // Chip to FPA transformation is a Cartesian 'projection'
    // reference pixel for FPA is 0.0, 0.0
    wcs->toSky = psProjectionAlloc (0.0, 0.0, 1.0, 1.0, PS_PROJ_WRP);

    // reference pixel (CRPIX1,2) is (0.0, 0.0):
    wcs->crpix1 = 0.0;
    wcs->crpix2 = 0.0;

    // we need to set CRVAL1,2 for the 0,0 pixel:
    wcs->crval1 = psPolynomial2DEval (chip->toFPA->x, 0.0, 0.0);
    wcs->crval2 = psPolynomial2DEval (chip->toFPA->y, 0.0, 0.0);

    wcs->toSky->R = wcs->crval1*PM_RAD_DEG;
    wcs->toSky->D = wcs->crval2*PM_RAD_DEG;

    // these need to be set to 0.0 since they have been moved to crpix1,crpix2
    wcs->trans->x->coeff[0][0] = 0.0;
    wcs->trans->y->coeff[0][0] = 0.0;
    
    // output coordinates are in microns : CDELT1,2 has units of microns/pixel
    wcs->cdelt1 = hypot (wcs->trans->x->coeff[1][0], wcs->trans->x->coeff[0][1]);
    wcs->cdelt2 = hypot (wcs->trans->y->coeff[1][0], wcs->trans->y->coeff[0][1]);

    return wcs;
}

// convert the fpa-level toTPA, toSky to a wcs polynomial transformation
pmAstromWCS *pmAstromWCSBilevelMosaicFromFPA (const pmFPA *fpa, double tol)
{
    // XXX require fpa->toTPA->x->nX == fpa->toTPA->x->nY
    // XXX require fpa->toTPA->y->nX == fpa->toTPA->y->nY
    // XXX require fpa->toTPA->x->nX == fpa->toTPA->y->nX
    // XXX require fpa->toTPA->nX == 1,2,3
    // XXX require fpa->toSky->type == PS_PROJ_TAN

    // convert fpa->toTPA + fpa->toSky to wcs format (DIS)
    pmAstromWCS *wcs = pmAstromWCSAlloc(fpa->toTPA->x->nX, fpa->toTPA->x->nY);

    // convert projection from TPA to SKY into wcs projection (degrees to radians)
    wcs->toSky = psProjectionAlloc (fpa->toSky->R, fpa->toSky->D, PM_RAD_DEG, PM_RAD_DEG, PS_PROJ_DIS);
    wcs->crval1 = fpa->toSky->R*PS_DEG_RAD;
    wcs->crval2 = fpa->toSky->D*PS_DEG_RAD;

    // given transformation, solve for coordinates which yields output coordinates of 0,0
    psPlane *center = psPlaneTransformGetCenter (fpa->toTPA, tol);
    if (!center) {
        psError(PS_ERR_UNKNOWN, false, "Unable to solve for TPA center.");
        psFree (wcs);
        return NULL;
    }

    // adjust wcs transform to use center as reference coordinate
    // resulting transformation has units of unity (microns/micron)
    psPlaneTransformSetCenter (wcs->trans, fpa->toTPA, center->x, center->y);

    // calculated center is crpix1,2
    wcs->crpix1 = center->x;
    wcs->crpix2 = center->y;
    psFree (center);

    // pdelt1,2 has units of degrees/micron
    double pdelt1 = fpa->toSky->Xs * PS_DEG_RAD;
    double pdelt2 = fpa->toSky->Ys * PS_DEG_RAD;

    // convert wcs->trans to units of degree/micron
    for (int i = 0; i <= wcs->trans->x->nX; i++) {
        for (int j = 0; j <= wcs->trans->x->nY; j++) {
            wcs->trans->x->coeff[i][j] *= pdelt1;
            wcs->trans->y->coeff[i][j] *= pdelt2;
        }
    }

    // cdelt1,2 has units of degrees/micron
    wcs->cdelt1 = hypot (wcs->trans->x->coeff[1][0], wcs->trans->x->coeff[0][1]);
    wcs->cdelt2 = hypot (wcs->trans->y->coeff[1][0], wcs->trans->y->coeff[0][1]);

    return wcs;
}

static psPlaneTransform *
linearFitToTransform(psPlaneTransform *trans, psRegion *bounds)
{
    int     nSamples = 10;  // 10 samples in each dimension

    double   deltaX = (bounds->x1 - bounds->x0);
    double   deltaY = (bounds->y1 - bounds->y0);

    psArray *src = psArrayAlloc(nSamples * nSamples);
    psArray *dst = psArrayAlloc(nSamples * nSamples);

    int k=0;
    for (int j=0; j<nSamples; j++) {
        double y = bounds->y0 + (j * deltaY / nSamples);
        for (int i=0; i<nSamples; i++) {
            psPlane *s = psPlaneAlloc();
            s->x = bounds->x0 + (i * deltaX / nSamples);
            s->y = y;
            psArraySet(src, k, s);
            psPlane *d = psPlaneTransformApply(NULL, trans, s);
            psArraySet(dst, k, d);
            psFree(s);  // drop our refs to s and d
            psFree(d);
            ++k;
        }
    }

    psPlaneTransform *newTrans = psPlaneTransformAlloc(1, 1, PS_POLYNOMIAL_ORD);

    if (!psPlaneTransformFit(newTrans, src, dst, 0, 0)) {
        psError(PS_ERR_UNKNOWN, false, "linear fit to transform failed");
        return NULL;
    }

#define noCOMPARE_TRANS
#ifdef COMPARE_TRANS
    // compare the computed coordintes from this transform with the original
    psPlane *new = psPlaneAlloc();
    printf("   i     chip_x  tpa_x     tpa_x_fit     dx         chip_y    tpa_y     tpa_y_fit     dy     dx > 0.5 || dy > 0.5\n");
    for (int i=0; i<psArrayLength(dst); i++) {
        psPlane *d = (psPlane *) psArrayGet(dst, i);
        psPlane *s = (psPlane *) psArrayGet(src, i);

        new = psPlaneTransformApply(new, newTrans, s);

        double xerr = new->x - d->x;
        double yerr = new->y - d->y;
        bool bigerr = (fabs(xerr) > .5) || (fabs(yerr) > .5);
        printf("%4d %9.2f %9.2f %9.2f %9.4f     %9.2f %9.2f %9.2f %9.4f   %s\n"
               , i, s->x, new->x, d->x, xerr, s->y, new->y, d->y, yerr, bigerr ? "BIGERR" : "");
    }
    psFree(new);
#endif
    psArrayElementsFree(src);
    psFree(src);
    psArrayElementsFree(dst);
    psFree(dst);

    return newTrans;
}

bool pmAstromLinearizeTransforms(pmFPA *inFPA, pmChip *inChip, pmFPA *outFPA, pmChip *outChip, psRegion *outputBounds, double offset_x, double offset_y)
{
    PS_ASSERT_PTR_NON_NULL(inFPA, NULL);
    PS_ASSERT_PTR_NON_NULL(inChip, NULL);

    int ExtraOrders = pmAstrometryGetExtraOrders();

    if (outFPA == NULL) {
        outFPA = inFPA;
    }
    if (outChip == NULL) {
        outChip = inChip;
    }
    if (outputBounds == NULL) {
        outputBounds = pmChipPixels(outChip);
    }

    // First combine the "chip to FPA" and "FPA to TPA" into a single transformation
    psPlaneTransform *chipToTPA = psPlaneTransformCombine(NULL, inChip->toFPA, inFPA->toTPA, *outputBounds, 50);
    if (!chipToTPA) {
        psError(PS_ERR_UNKNOWN, false, "failed to create chipToTPA");
        return false;
    }

    // Next do the linear fit within the output boundary pixels
    psPlaneTransform *chipToFPA = linearFitToTransform(chipToTPA, outputBounds);
    psFree(chipToTPA);
    if (!chipToFPA) {
        psError(PS_ERR_UNKNOWN, false, "linear fit of chip to TPA transform failed");
        return false;
    }

    // if requested,  change the center
    psPlaneTransform *outToFPA;
    if (offset_x != 0. && offset_y != 0.) {
        outToFPA = psPlaneTransformSetCenter(NULL, chipToFPA, offset_x, offset_y);
        psFree(chipToFPA);
    } else {
        outToFPA = chipToFPA;
    }

    // NOTE: the extraOrders value (4) should be ignored since outToFPA is specified to be linear
    psPlaneTransform *outFromFPA = psPlaneTransformInvert(NULL, outToFPA, *outputBounds, 50, ExtraOrders);
    if (!outFromFPA) {
        psFree(outToFPA);
        psError(PS_ERR_UNKNOWN, false, "inversion of fit of output chip toFPA failed");
        return false;
    }

    // Success. Now set the fpa's toTPA and fromTPA to identity and replace the chip's transforms.

    psFree(outFPA->toTPA);
    outFPA->toTPA =  psPlaneTransformIdentity(1);

    psFree(outFPA->fromTPA);
    outFPA->fromTPA = psPlaneTransformIdentity(1);

    psFree(outChip->toFPA);
    outChip->toFPA = outToFPA;

    psFree(outChip->fromFPA);
    outChip->fromFPA = outFromFPA;

    // Finally, change the type for the projection.
    outFPA->toSky->type = PS_PROJ_TAN;

    return true;
}

bool pmAstromLinearizeToSky(pmFPA *inFPA, pmChip *inChip, pmFPA *outFPA, pmChip *outChip, psRegion *bounds)
{
    PS_ASSERT_PTR_NON_NULL(inFPA, NULL);
    PS_ASSERT_PTR_NON_NULL(inChip, NULL);
    PS_ASSERT_PTR_NON_NULL(outFPA, NULL);
    PS_ASSERT_PTR_NON_NULL(outChip, NULL);
    PS_ASSERT_PTR_NON_NULL(bounds, NULL);

    // outFPA projection must be defined as the goal

    // the output transformations are:
    // chip -> FPA : standard linear trans with needed rotation, etc
    // FPA  -> TPA : identidy

    // NOTE: streaksremove's linearizeTransform function passes pointers to skeleton outFPA and outChip structs
    // Only outFPA->toSky is valid. No other memebers in these structs should be read. The resulting output transforms
    // are copied to inFPA and inChip by the caller.

    int nSamples = 10;  // 10 samples in each dimension

    double deltaX = (bounds->x1 - bounds->x0);
    double deltaY = (bounds->y1 - bounds->y0);

    psArray *src = psArrayAllocEmpty(nSamples * nSamples);
    psArray *dst = psArrayAllocEmpty(nSamples * nSamples);

    psPlane srcFP, srcTP;

    for (int j = 0; j < nSamples; j++) {
        double y = bounds->y0 + (j * deltaY / nSamples);
        for (int i =  0; i < nSamples; i++) {

            psSphere srcSky;
            psPlane *srcChip = psPlaneAlloc();
            psPlane *dstTP = psPlaneAlloc();

            srcChip->x = bounds->x0 + (i * deltaX / nSamples);
            srcChip->y = y;

            psPlaneTransformApply (&srcFP, inChip->toFPA, srcChip);
            psPlaneTransformApply (&srcTP, inFPA->toTPA, &srcFP);
            psDeproject (&srcSky, &srcTP, inFPA->toSky);

            // fprintf (stderr, "%f %f | %f %f | %f %f | %f %f\n", srcChip->x, srcChip->y, srcFP.x, srcFP.y, srcTP.x, srcTP.y, srcSky.r*PS_DEG_RAD, srcSky.d*PS_DEG_RAD);

            psProject (dstTP, &srcSky, outFPA->toSky);

            srcChip->x -= bounds->x0;
            srcChip->y -= bounds->y0;
            psArrayAdd (src, 100, srcChip);
            psArrayAdd (dst, 100, dstTP);

            psFree(srcChip);  // drop our refs to s and d
            psFree(dstTP);
        }
    }

    psPlaneTransform *newToFPA = psPlaneTransformAlloc(1, 1, PS_POLYNOMIAL_ORD);
    newToFPA->x->coeffMask[1][1] = 1;
    newToFPA->y->coeffMask[1][1] = 1;

    if (!psPlaneTransformFit(newToFPA, src, dst, 0, 0)) {
        psError(PS_ERR_UNKNOWN, false, "linear fit to transform failed");
        psFree(src);
        psFree(dst);
        return NULL;
    }

# if (0)
    for (int i = 0; i < src->n; i++) {

        psSphere srcSky, dstSky;
        psPlane *srcChip = src->data[i];
        psPlane *dstTP   = dst->data[i];

        psPlaneTransformApply (&srcFP, newToFPA, srcChip);
        psDeproject (&srcSky, &srcFP, outFPA->toSky);
        psDeproject (&dstSky, dstTP, outFPA->toSky);

        double dX = (srcSky.r*PS_DEG_RAD - dstSky.r*PS_DEG_RAD)*3600.0;
        double dY = (srcSky.d*PS_DEG_RAD - dstSky.d*PS_DEG_RAD)*3600.0;
        fprintf (stderr, "%f %f | %f %f | %f %f | %f %f | %f %f | %f %f\n", dX, dY, srcChip->x, srcChip->y, srcFP.x, srcFP.y, dstTP->x, dstTP->y, srcSky.r*PS_DEG_RAD, srcSky.d*PS_DEG_RAD, dstSky.r*PS_DEG_RAD, dstSky.d*PS_DEG_RAD);

    }
# endif

    psFree(src);
    psFree(dst);

    // this is a linear transformation, no extra orders are needed
    psPlaneTransform *newFromFPA = psPlaneTransformInvert(NULL, newToFPA, *bounds, 1, 0);
    if (!newFromFPA) {
        psFree(newToFPA);
        psError(PS_ERR_UNKNOWN, false, "inversion of fit of output chip toFPA failed");
        return false;
    }

    // Success. Now set the fpa's toTPA and fromTPA to identity and replace the chip's transforms.
    psFree(outChip->toFPA);
    outChip->toFPA = newToFPA;

    psFree(outChip->fromFPA);
    outChip->fromFPA = newFromFPA;

    psFree(outFPA->toTPA);
    outFPA->toTPA =  psPlaneTransformIdentity(1);

    psFree(outFPA->fromTPA);
    outFPA->fromTPA = psPlaneTransformIdentity(1);

    return true;
}

static void pmAstromWCSFree (pmAstromWCS *wcs)
{

    if (!wcs)
        return;
    psFree (wcs->trans);
    psFree (wcs->toSky);
}

pmAstromWCS *pmAstromWCSAlloc (int nXorder, int nYorder)
{

    pmAstromWCS *wcs = (pmAstromWCS *) psAlloc(sizeof(pmAstromWCS));
    psMemSetDeallocator(wcs, (psFreeFunc) pmAstromWCSFree);

    // note: WCS transforms are always defined as chip to sky
    wcs->trans = psPlaneTransformAlloc (nXorder, nYorder, PS_POLYNOMIAL_ORD);
    wcs->toSky = NULL;
    wcs->wcsCDkeys = 0;

    memset (wcs->ctype1, 0, PM_ASTROM_WCS_TYPE_SIZE);
    memset (wcs->ctype2, 0, PM_ASTROM_WCS_TYPE_SIZE);
    return wcs;
}

/*****

      For mosaic astrometry, we need to have a starting set of projection terms in which the
      chip-to-FPA terms result in a fixed physical unit on the focal plane (eg, pixels or
      microns).  This set of projections, coupled with an identity toTPA (ie, no distortion) will
      result in substantial errors between the observed and predicted star positions on the focal
      plane: this is the measurement of the optical distortion in the camera.  At the same time,
      we need to carry around the transformations which allow us to make an accurate calculation
      of the position of the stars based on the input (per-chip) astrometry.  These
      transformations will allow us to match the raw and ref stars robustly.  To convert the
      per-chip astrometry (which may have been calculated with a different plate scale for each
      chip) to a collection of astrometry terms for chips in a single mosaic, we need to adjust
      the chip-to-FPA scaling (eg, pc11) to match the variations in the effective plate scale for
      each chip (eg, cdelt1).  Thus, we need to carry around both the

*****/

/* discussion of the coord transformations:
   X,Y: coord on a chip in pixels
   L,M: coord on the focal plane (pixels)
   P,Q: coord in the tangent plane (microns or mm?)
   R,D: coord on the sky

   this function creates WCS terms which convert directly from chip to sky.
   this function requires a linear, unrotated toTPA distortion term
   toTPA->x,y->coeff[1][0],[0][1] defines the detector scale (microns / pixel)
   tpSky->Xs,Ys defines the plate scale (radians / micron)
*/

/* at this point, we have extracted from the header the WCS terms in the form of a polynomial,
 * wcs->trans, which will convert X,Y in pixels to L,M in degrees.  we also have the following
 * elements defined:
 * type (projection type)
 * crval1,2 (in RA,DEC degrees)
 * crpix1,2
 * cdelt1,2 (in degrees / pixel)
 * pixelScale (microns / pixel)
 *
 * now we convert wcs->trans to toFPA, which is different from wcs->trans in 3 important ways:
 * 1) the output is in microns (not degrees): divide by cdelt1,2
 * 2) X,Y are applied directly, without an applied Xo,Yo offset
 * 3) there is an allowed Lo,Mo term ([0][0] coefficients)
 */

