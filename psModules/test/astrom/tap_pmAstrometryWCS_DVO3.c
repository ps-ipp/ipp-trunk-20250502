#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <stdio.h>
#include <string.h>
#include <pslib.h>
#include <psmodules.h>

#include "tap.h"
#include "pstap.h"

# if (HAVE_KAPA)
    # include "dvo.h"

    psMetadata *WriteCoordsToHeader (Coords *coords);
void test1(); // basic TAN projection,
void test2(); // small rotation
void test3(); // 2nd order term
void test1x(); // basic TAN projection with central offset
void test2x(); // small rotation with central offset
void test3x(); // 2nd order term with central offset

int main (void)
{
    plan_tests(991);

    note("pmAstromWriteWCS tests compared with DVO coords routines");

    test1();
    test2();
    test3();
    test1x();
    test2x();
    test3x();

    return exit_status();
}

void test1()
{
    note("test pmAstromReadWCS");
    psMemId id = psMemGetId();

    // build a DVO-style coordinate system
    Coords coords;
    strcpy (coords.ctype, "RA---TAN");
    coords.crval1 = 0.0;
    coords.crval2 = 0.0;
    coords.crpix1 = 0.0;
    coords.crpix2 = 0.0;
    coords.cdelt1 = 1.0/3600.0;
    coords.cdelt2 = 1.0/3600.0;
    coords.pc1_1  = 1.0;
    coords.pc1_2  = 0.0;
    coords.pc2_1  = 0.0;
    coords.pc2_2  = 1.0;
    coords.Npolyterms = 0;
    for (int i = 0; i < 7; i++) {
        coords.polyterms[i][0] = 0.0;
        coords.polyterms[i][1] = 0.0;
    }

    psMetadata *header1 = WriteCoordsToHeader (&coords);
    pmFPA *fpa = pmFPAAlloc (NULL, NULL);
    pmChip *chip = pmChipAlloc (fpa, NULL);

    bool status = pmAstromReadWCS (fpa, chip, header1, 10.0);

    ok (status, "converted WCS keywords to WCS astrometry");
    skip_start (!status, 1, "*** WCS Conversion FAILS *** : skipping related tests");

    psMetadata *header2 = psMetadataAlloc();
    status = pmAstromWriteWCS (header2, fpa, chip, 0.001);
    pmAstromWCS *wcs = pmAstromWCSfromHeader(header2);

    psPlane  *aChip = psPlaneAlloc();
    psPlane  *aFPA = psPlaneAlloc();
    psPlane  *aTPA = psPlaneAlloc();
    psSphere *aSky = psSphereAlloc();

    psPlane  *bChip = psPlaneAlloc();
    psSphere *bSky = psSphereAlloc();

    for (double x = -2000; x <= +2000; x+= 500.0) {
        for (double y = -2000; y <= +2000; y+= 500.0) {
            aChip->x = x;
            aChip->y = y;
            bChip->x = x;
            bChip->y = y;

            psPlaneTransformApply (aFPA, chip->toFPA, aChip);
            psPlaneTransformApply (aTPA, fpa->toTPA, aFPA);
            psDeproject (aSky, aTPA, fpa->toSky);

            pmAstromWCStoSky (bSky, wcs, bChip);

            while (aSky->r > 2*M_PI)
                aSky->r -= 2*M_PI;
            while (aSky->r <      0)
                aSky->r += 2*M_PI;
            while (bSky->r > 2*M_PI)
                bSky->r -= 2*M_PI;
            while (bSky->r <      0)
                bSky->r += 2*M_PI;

            ok_float(aSky->r*PS_DEG_RAD, bSky->r*PS_DEG_RAD, "coordinate match: %f vs %f (delta = %f)", aSky->r*PS_DEG_RAD, bSky->r*PS_DEG_RAD, aSky->r*PS_DEG_RAD - bSky->r*PS_DEG_RAD);
            ok_float(aSky->d*PS_DEG_RAD, bSky->d*PS_DEG_RAD, "coordinate match: %f vs %f (delta = %f)", aSky->d*PS_DEG_RAD, bSky->d*PS_DEG_RAD, aSky->d*PS_DEG_RAD - bSky->d*PS_DEG_RAD);
        }
    }
    psFree (aSky);
    psFree (aTPA);
    psFree (aFPA);
    psFree (aChip);

    psFree (bSky);
    psFree (bChip);

    psFree (wcs);
    psFree (header2);

    skip_end();
    psFree (fpa);
    psFree (chip);
    psFree (header1);

    ok(!psMemCheckLeaks (id, NULL, stderr, false), "no memory leaks");
}

void test2()
{
    note("test pmAstromReadWCS");
    psMemId id = psMemGetId();

    // build a DVO-style coordinate system
    Coords coords;
    strcpy (coords.ctype, "RA---TAN");
    coords.crval1 = 0.0;
    coords.crval2 = 0.0;
    coords.crpix1 = 0.0;
    coords.crpix2 = 0.0;
    coords.cdelt1 = 1.0/3600.0;
    coords.cdelt2 = 1.0/3600.0;
    coords.pc1_1  = 0.9;
    coords.pc1_2  = 0.1;
    coords.pc2_1  = -0.1;
    coords.pc2_2  = 0.9;
    coords.Npolyterms = 0;
    for (int i = 0; i < 7; i++) {
        coords.polyterms[i][0] = 0.0;
        coords.polyterms[i][1] = 0.0;
    }

    psMetadata *header1 = WriteCoordsToHeader (&coords);

    pmFPA *fpa = pmFPAAlloc (NULL, NULL);
    pmChip *chip = pmChipAlloc (fpa, NULL);

    bool status = pmAstromReadWCS (fpa, chip, header1, 10.0);

    ok (status, "converted WCS keywords to WCS astrometry");
    skip_start (!status, 1, "*** WCS Conversion FAILS *** : skipping related tests");

    psMetadata *header2 = psMetadataAlloc();
    status = pmAstromWriteWCS (header2, fpa, chip, 0.001);

    pmAstromWCS *wcs = pmAstromWCSfromHeader(header2);

    psPlane  *aChip = psPlaneAlloc();
    psPlane  *aFPA = psPlaneAlloc();
    psPlane  *aTPA = psPlaneAlloc();
    psSphere *aSky = psSphereAlloc();

    psPlane  *bChip = psPlaneAlloc();
    psSphere *bSky = psSphereAlloc();

    for (double x = -2000; x <= +2000; x+= 500.0) {
        for (double y = -2000; y <= +2000; y+= 500.0) {
            aChip->x = x;
            aChip->y = y;
            bChip->x = x;
            bChip->y = y;

            psPlaneTransformApply (aFPA, chip->toFPA, aChip);
            psPlaneTransformApply (aTPA, fpa->toTPA, aFPA);
            psDeproject (aSky, aTPA, fpa->toSky);

            pmAstromWCStoSky (bSky, wcs, bChip);

            while (aSky->r > 2*M_PI)
                aSky->r -= 2*M_PI;
            while (aSky->r <      0)
                aSky->r += 2*M_PI;
            while (bSky->r > 2*M_PI)
                bSky->r -= 2*M_PI;
            while (bSky->r <      0)
                bSky->r += 2*M_PI;

            ok_float(aSky->r*PS_DEG_RAD, bSky->r*PS_DEG_RAD, "coordinate match: %f vs %f (delta = %f)", aSky->r*PS_DEG_RAD, bSky->r*PS_DEG_RAD, aSky->r*PS_DEG_RAD - bSky->r*PS_DEG_RAD);
            ok_float(aSky->d*PS_DEG_RAD, bSky->d*PS_DEG_RAD, "coordinate match: %f vs %f (delta = %f)", aSky->d*PS_DEG_RAD, bSky->d*PS_DEG_RAD, aSky->d*PS_DEG_RAD - bSky->d*PS_DEG_RAD);
        }
    }
    psFree (aSky);
    psFree (aTPA);
    psFree (aFPA);
    psFree (aChip);

    psFree (bSky);
    psFree (bChip);

    psFree (wcs);
    psFree (header2);

    skip_end();
    psFree (fpa);
    psFree (chip);
    psFree (header1);

    ok(!psMemCheckLeaks (id, NULL, stderr, false), "no memory leaks");
}

void test3()
{
    note("test pmAstromReadWCS");
    psMemId id = psMemGetId();

    // build a DVO-style coordinate system
    Coords coords;
    strcpy (coords.ctype, "RA---TAN");
    coords.crval1 = 0.0;
    coords.crval2 = 0.0;
    coords.crpix1 = 0.0;
    coords.crpix2 = 0.0;
    coords.cdelt1 = 1.0/3600.0;
    coords.cdelt2 = 1.0/3600.0;
    coords.pc1_1  = 1.0;
    coords.pc1_2  = 0.0;
    coords.pc2_1  = 0.0;
    coords.pc2_2  = 1.0;
    coords.Npolyterms = 2;
    for (int i = 0; i < 7; i++) {
        coords.polyterms[i][0] = 0.0;
        coords.polyterms[i][1] = 0.0;
    }
    coords.polyterms[0][0] = 0.01; // L vs X^2
    coords.polyterms[2][1] = 0.01; // M vs Y^2

    psMetadata *header1 = WriteCoordsToHeader (&coords);
    pmFPA *fpa = pmFPAAlloc (NULL, NULL);
    pmChip *chip = pmChipAlloc (fpa, NULL);

    bool status = pmAstromReadWCS (fpa, chip, header1, 10.0);

    ok (status, "converted WCS keywords to WCS astrometry");
    skip_start (!status, 1, "*** WCS Conversion FAILS *** : skipping related tests");

    psMetadata *header2 = psMetadataAlloc();
    status = pmAstromWriteWCS (header2, fpa, chip, 0.001);

    pmAstromWCS *wcs = pmAstromWCSfromHeader(header2);

    psPlane  *aChip = psPlaneAlloc();
    psPlane  *aFPA = psPlaneAlloc();
    psPlane  *aTPA = psPlaneAlloc();
    psSphere *aSky = psSphereAlloc();

    psPlane  *bChip = psPlaneAlloc();
    psSphere *bSky = psSphereAlloc();

    for (double x = -2000; x <= +2000; x+= 500.0) {
        for (double y = -2000; y <= +2000; y+= 500.0) {
            aChip->x = x;
            aChip->y = y;
            bChip->x = x;
            bChip->y = y;

            psPlaneTransformApply (aFPA, chip->toFPA, aChip);
            psPlaneTransformApply (aTPA, fpa->toTPA, aFPA);
            psDeproject (aSky, aTPA, fpa->toSky);

            pmAstromWCStoSky (bSky, wcs, bChip);

            while (aSky->r > 2*M_PI)
                aSky->r -= 2*M_PI;
            while (aSky->r <      0)
                aSky->r += 2*M_PI;
            while (bSky->r > 2*M_PI)
                bSky->r -= 2*M_PI;
            while (bSky->r <      0)
                bSky->r += 2*M_PI;

            ok_float(aSky->r*PS_DEG_RAD, bSky->r*PS_DEG_RAD, "coordinate match: %f vs %f (delta = %f)", aSky->r*PS_DEG_RAD, bSky->r*PS_DEG_RAD, aSky->r*PS_DEG_RAD - bSky->r*PS_DEG_RAD);
            ok_float(aSky->d*PS_DEG_RAD, bSky->d*PS_DEG_RAD, "coordinate match: %f vs %f (delta = %f)", aSky->d*PS_DEG_RAD, bSky->d*PS_DEG_RAD, aSky->d*PS_DEG_RAD - bSky->d*PS_DEG_RAD);
        }
    }
    psFree (aSky);
    psFree (aTPA);
    psFree (aFPA);
    psFree (aChip);

    psFree (bSky);
    psFree (bChip);

    psFree (wcs);
    psFree (header2);

    skip_end();
    psFree (fpa);
    psFree (chip);
    psFree (header1);

    ok(!psMemCheckLeaks (id, NULL, stderr, false), "no memory leaks");
}

void test1x()
{
    note("test pmAstromReadWCS");
    psMemId id = psMemGetId();

    // build a DVO-style coordinate system
    Coords coords;
    strcpy (coords.ctype, "RA---TAN");
    coords.crval1 = 0.0;
    coords.crval2 = 0.0;
    coords.crpix1 = 20.0;
    coords.crpix2 = 50.0;
    coords.cdelt1 = 1.0/3600.0;
    coords.cdelt2 = 1.0/3600.0;
    coords.pc1_1  = 1.0;
    coords.pc1_2  = 0.0;
    coords.pc2_1  = 0.0;
    coords.pc2_2  = 1.0;
    coords.Npolyterms = 0;
    for (int i = 0; i < 7; i++) {
        coords.polyterms[i][0] = 0.0;
        coords.polyterms[i][1] = 0.0;
    }

    psMetadata *header1 = WriteCoordsToHeader (&coords);
    pmFPA *fpa = pmFPAAlloc (NULL, NULL);
    pmChip *chip = pmChipAlloc (fpa, NULL);

    bool status = pmAstromReadWCS (fpa, chip, header1, 10.0);

    ok (status, "converted WCS keywords to WCS astrometry");
    skip_start (!status, 1, "*** WCS Conversion FAILS *** : skipping related tests");

    psMetadata *header2 = psMetadataAlloc();
    status = pmAstromWriteWCS (header2, fpa, chip, 0.001);
    pmAstromWCS *wcs = pmAstromWCSfromHeader(header2);

    psPlane  *aChip = psPlaneAlloc();
    psPlane  *aFPA = psPlaneAlloc();
    psPlane  *aTPA = psPlaneAlloc();
    psSphere *aSky = psSphereAlloc();

    psPlane  *bChip = psPlaneAlloc();
    psSphere *bSky = psSphereAlloc();

    for (double x = -2000; x <= +2000; x+= 500.0) {
        for (double y = -2000; y <= +2000; y+= 500.0) {
            aChip->x = x;
            aChip->y = y;
            bChip->x = x;
            bChip->y = y;

            psPlaneTransformApply (aFPA, chip->toFPA, aChip);
            psPlaneTransformApply (aTPA, fpa->toTPA, aFPA);
            psDeproject (aSky, aTPA, fpa->toSky);

            pmAstromWCStoSky (bSky, wcs, bChip);

            while (aSky->r > 2*M_PI)
                aSky->r -= 2*M_PI;
            while (aSky->r <      0)
                aSky->r += 2*M_PI;
            while (bSky->r > 2*M_PI)
                bSky->r -= 2*M_PI;
            while (bSky->r <      0)
                bSky->r += 2*M_PI;

            ok_float(aSky->r*PS_DEG_RAD, bSky->r*PS_DEG_RAD, "coordinate match: %f vs %f (delta = %f)", aSky->r*PS_DEG_RAD, bSky->r*PS_DEG_RAD, aSky->r*PS_DEG_RAD - bSky->r*PS_DEG_RAD);
            ok_float(aSky->d*PS_DEG_RAD, bSky->d*PS_DEG_RAD, "coordinate match: %f vs %f (delta = %f)", aSky->d*PS_DEG_RAD, bSky->d*PS_DEG_RAD, aSky->d*PS_DEG_RAD - bSky->d*PS_DEG_RAD);
        }
    }
    psFree (aSky);
    psFree (aTPA);
    psFree (aFPA);
    psFree (aChip);

    psFree (bSky);
    psFree (bChip);

    psFree (wcs);
    psFree (header2);

    skip_end();
    psFree (fpa);
    psFree (chip);
    psFree (header1);

    ok(!psMemCheckLeaks (id, NULL, stderr, false), "no memory leaks");
}

void test2x()
{
    note("test pmAstromReadWCS");
    psMemId id = psMemGetId();

    // build a DVO-style coordinate system
    Coords coords;
    strcpy (coords.ctype, "RA---TAN");
    coords.crval1 = 0.0;
    coords.crval2 = 0.0;
    coords.crpix1 = 50.0;
    coords.crpix2 = -20.0;
    coords.cdelt1 = 1.0/3600.0;
    coords.cdelt2 = 1.0/3600.0;
    coords.pc1_1  = 0.9;
    coords.pc1_2  = 0.1;
    coords.pc2_1  = -0.1;
    coords.pc2_2  = 0.9;
    coords.Npolyterms = 0;
    for (int i = 0; i < 7; i++) {
        coords.polyterms[i][0] = 0.0;
        coords.polyterms[i][1] = 0.0;
    }

    psMetadata *header1 = WriteCoordsToHeader (&coords);

    pmFPA *fpa = pmFPAAlloc (NULL, NULL);
    pmChip *chip = pmChipAlloc (fpa, NULL);

    bool status = pmAstromReadWCS (fpa, chip, header1, 10.0);

    ok (status, "converted WCS keywords to WCS astrometry");
    skip_start (!status, 1, "*** WCS Conversion FAILS *** : skipping related tests");

    psMetadata *header2 = psMetadataAlloc();
    status = pmAstromWriteWCS (header2, fpa, chip, 0.001);

    pmAstromWCS *wcs = pmAstromWCSfromHeader(header2);

    psPlane  *aChip = psPlaneAlloc();
    psPlane  *aFPA = psPlaneAlloc();
    psPlane  *aTPA = psPlaneAlloc();
    psSphere *aSky = psSphereAlloc();

    psPlane  *bChip = psPlaneAlloc();
    psSphere *bSky = psSphereAlloc();

    for (double x = -2000; x <= +2000; x+= 500.0) {
        for (double y = -2000; y <= +2000; y+= 500.0) {
            aChip->x = x;
            aChip->y = y;
            bChip->x = x;
            bChip->y = y;

            psPlaneTransformApply (aFPA, chip->toFPA, aChip);
            psPlaneTransformApply (aTPA, fpa->toTPA, aFPA);
            psDeproject (aSky, aTPA, fpa->toSky);

            pmAstromWCStoSky (bSky, wcs, bChip);

            while (aSky->r > 2*M_PI)
                aSky->r -= 2*M_PI;
            while (aSky->r <      0)
                aSky->r += 2*M_PI;
            while (bSky->r > 2*M_PI)
                bSky->r -= 2*M_PI;
            while (bSky->r <      0)
                bSky->r += 2*M_PI;

            ok_float(aSky->r*PS_DEG_RAD, bSky->r*PS_DEG_RAD, "coordinate match: %f vs %f (delta = %f)", aSky->r*PS_DEG_RAD, bSky->r*PS_DEG_RAD, aSky->r*PS_DEG_RAD - bSky->r*PS_DEG_RAD);
            ok_float(aSky->d*PS_DEG_RAD, bSky->d*PS_DEG_RAD, "coordinate match: %f vs %f (delta = %f)", aSky->d*PS_DEG_RAD, bSky->d*PS_DEG_RAD, aSky->d*PS_DEG_RAD - bSky->d*PS_DEG_RAD);
        }
    }
    psFree (aSky);
    psFree (aTPA);
    psFree (aFPA);
    psFree (aChip);

    psFree (bSky);
    psFree (bChip);

    psFree (wcs);
    psFree (header2);

    skip_end();
    psFree (fpa);
    psFree (chip);
    psFree (header1);

    ok(!psMemCheckLeaks (id, NULL, stderr, false), "no memory leaks");
}

void test3x()
{
    note("test pmAstromReadWCS");
    psMemId id = psMemGetId();

    // build a DVO-style coordinate system
    Coords coords;
    strcpy (coords.ctype, "RA---TAN");
    coords.crval1 = 0.0;
    coords.crval2 = 0.0;
    coords.crpix1 = 20.0;
    coords.crpix2 = 50.0;
    coords.cdelt1 = 1.0/3600.0;
    coords.cdelt2 = 1.0/3600.0;
    coords.pc1_1  = 1.0;
    coords.pc1_2  = 0.0;
    coords.pc2_1  = 0.0;
    coords.pc2_2  = 1.0;
    coords.Npolyterms = 2;
    for (int i = 0; i < 7; i++) {
        coords.polyterms[i][0] = 0.0;
        coords.polyterms[i][1] = 0.0;
    }
    coords.polyterms[0][0] = 0.01; // L vs X^2
    coords.polyterms[2][1] = 0.01; // M vs Y^2

    psMetadata *header1 = WriteCoordsToHeader (&coords);
    pmFPA *fpa = pmFPAAlloc (NULL, NULL);
    pmChip *chip = pmChipAlloc (fpa, NULL);

    bool status = pmAstromReadWCS (fpa, chip, header1, 10.0);

    ok (status, "converted WCS keywords to WCS astrometry");
    skip_start (!status, 1, "*** WCS Conversion FAILS *** : skipping related tests");

    psMetadata *header2 = psMetadataAlloc();
    status = pmAstromWriteWCS (header2, fpa, chip, 0.00001);

    pmAstromWCS *wcs = pmAstromWCSfromHeader(header2);

    psPlane  *aChip = psPlaneAlloc();
    psPlane  *aFPA = psPlaneAlloc();
    psPlane  *aTPA = psPlaneAlloc();
    psSphere *aSky = psSphereAlloc();

    psPlane  *bChip = psPlaneAlloc();
    psSphere *bSky = psSphereAlloc();

    for (double x = -2000; x <= +2000; x+= 500.0) {
        for (double y = -2000; y <= +2000; y+= 500.0) {
            aChip->x = x;
            aChip->y = y;
            bChip->x = x;
            bChip->y = y;

            psPlaneTransformApply (aFPA, chip->toFPA, aChip);
            psPlaneTransformApply (aTPA, fpa->toTPA, aFPA);
            psDeproject (aSky, aTPA, fpa->toSky);

            pmAstromWCStoSky (bSky, wcs, bChip);

            while (aSky->r > 2*M_PI)
                aSky->r -= 2*M_PI;
            while (aSky->r <      0)
                aSky->r += 2*M_PI;
            while (bSky->r > 2*M_PI)
                bSky->r -= 2*M_PI;
            while (bSky->r <      0)
                bSky->r += 2*M_PI;

            // XXX we are getting round-off errors as a result of the wcs transformation
            // having terms in units of pix/degree. for now require 10mas on this
            ok_float_tol(aSky->r*PS_DEG_RAD, bSky->r*PS_DEG_RAD, 0.01/3600.0, "coordinate match: %f vs %f (delta = %f)", aSky->r*PS_DEG_RAD, bSky->r*PS_DEG_RAD, aSky->r*PS_DEG_RAD - bSky->r*PS_DEG_RAD);
            ok_float_tol(aSky->d*PS_DEG_RAD, bSky->d*PS_DEG_RAD, 0.01/3600.0, "coordinate match: %f vs %f (delta = %f)", aSky->d*PS_DEG_RAD, bSky->d*PS_DEG_RAD, aSky->d*PS_DEG_RAD - bSky->d*PS_DEG_RAD);
        }
    }
    psFree (aSky);
    psFree (aTPA);
    psFree (aFPA);
    psFree (aChip);

    psFree (bSky);
    psFree (bChip);

    psFree (wcs);
    psFree (header2);

    skip_end();
    psFree (fpa);
    psFree (chip);
    psFree (header1);

    ok(!psMemCheckLeaks (id, NULL, stderr, false), "no memory leaks");
}

psMetadata *WriteCoordsToHeader (Coords *coords)
{

    char name[16];

    // construct a header using coords as the input
    psMetadata *header = psMetadataAlloc();

    sprintf (name, "RA--%s", &coords[0].ctype[4]);
    psMetadataAddStr (header, PS_LIST_TAIL, "CTYPE1", PS_META_REPLACE, "", name);
    sprintf (name, "DEC-%s", &coords[0].ctype[4]);
    psMetadataAddStr (header, PS_LIST_TAIL, "CTYPE2", PS_META_REPLACE, "", name);

    // center coords (R,D)
    psMetadataAddF64 (header, PS_LIST_TAIL, "CRVAL1", PS_META_REPLACE, "", coords[0].crval1);
    psMetadataAddF64 (header, PS_LIST_TAIL, "CRVAL2", PS_META_REPLACE, "", coords[0].crval2);

    // center coords (X,Y)
    psMetadataAddF64 (header, PS_LIST_TAIL, "CRPIX1", PS_META_REPLACE, "", coords[0].crpix1);
    psMetadataAddF64 (header, PS_LIST_TAIL, "CRPIX2", PS_META_REPLACE, "", coords[0].crpix2);

    // degrees per pixel
    psMetadataAddF64 (header, PS_LIST_TAIL, "CDELT1",  PS_META_REPLACE, "", coords[0].cdelt1);
    psMetadataAddF64 (header, PS_LIST_TAIL, "CDELT2",  PS_META_REPLACE, "", coords[0].cdelt2);

    // rotation matrix
    psMetadataAddF64 (header, PS_LIST_TAIL, "PC001001", PS_META_REPLACE, "", coords[0].pc1_1);
    psMetadataAddF64 (header, PS_LIST_TAIL, "PC001002", PS_META_REPLACE, "", coords[0].pc1_2);
    psMetadataAddF64 (header, PS_LIST_TAIL, "PC002001", PS_META_REPLACE, "", coords[0].pc2_1);
    psMetadataAddF64 (header, PS_LIST_TAIL, "PC002002", PS_META_REPLACE, "", coords[0].pc2_2);

    psMetadataAddF64 (header, PS_LIST_TAIL, "PCA1X2Y0", PS_META_REPLACE, "", coords[0].polyterms[0][0]);
    psMetadataAddF64 (header, PS_LIST_TAIL, "PCA1X1Y1", PS_META_REPLACE, "", coords[0].polyterms[1][0]);
    psMetadataAddF64 (header, PS_LIST_TAIL, "PCA1X0Y2", PS_META_REPLACE, "", coords[0].polyterms[2][0]);
    psMetadataAddF64 (header, PS_LIST_TAIL, "PCA2X2Y0", PS_META_REPLACE, "", coords[0].polyterms[0][1]);
    psMetadataAddF64 (header, PS_LIST_TAIL, "PCA2X1Y1", PS_META_REPLACE, "", coords[0].polyterms[1][1]);
    psMetadataAddF64 (header, PS_LIST_TAIL, "PCA2X0Y2", PS_META_REPLACE, "", coords[0].polyterms[2][1]);
    psMetadataAddF64 (header, PS_LIST_TAIL, "PCA1X3Y0", PS_META_REPLACE, "", coords[0].polyterms[3][0]);
    psMetadataAddF64 (header, PS_LIST_TAIL, "PCA1X2Y1", PS_META_REPLACE, "", coords[0].polyterms[4][0]);
    psMetadataAddF64 (header, PS_LIST_TAIL, "PCA1X1Y2", PS_META_REPLACE, "", coords[0].polyterms[5][0]);
    psMetadataAddF64 (header, PS_LIST_TAIL, "PCA1X0Y3", PS_META_REPLACE, "", coords[0].polyterms[6][0]);
    psMetadataAddF64 (header, PS_LIST_TAIL, "PCA2X3Y0", PS_META_REPLACE, "", coords[0].polyterms[3][1]);
    psMetadataAddF64 (header, PS_LIST_TAIL, "PCA2X2Y1", PS_META_REPLACE, "", coords[0].polyterms[4][1]);
    psMetadataAddF64 (header, PS_LIST_TAIL, "PCA2X1Y2", PS_META_REPLACE, "", coords[0].polyterms[5][1]);
    psMetadataAddF64 (header, PS_LIST_TAIL, "PCA2X0Y3", PS_META_REPLACE, "", coords[0].polyterms[6][1]);

    psMetadataAddS32 (header, PS_LIST_TAIL, "NPLYTERM", PS_META_REPLACE, "", coords[0].Npolyterms);

    return header;
}

# else

    int main (void)
{
    plan_tests(2);

    ok(true, "Skipping tests: (libdvo not available)");
    note("pmAstrometryWCS tests compared with DVO coords routines : SKIPPED (libdvo not available)");

    return exit_status();
}

# endif

