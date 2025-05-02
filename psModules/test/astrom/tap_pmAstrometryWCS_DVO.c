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

void test1in(); // basic TAN projection,
void test2in(); // small rotation
void test3in(); // 2nd order term

void testA(); // alloc test
void testB(); // alloc test

int main (void)
{
    plan_tests(992);

    note("pmAstrometryWCS tests compared with DVO coords routines");
    note("this file tests pmAstromWCS <-> Header representations");

    test1in();
    test2in();
    test3in();

    test1();
    test2();
    test3();
    return exit_status();
}

void test3in()
{
    note("test pmAstromWCStoHeader");
    psMemId id = psMemGetId();

    // build a DVO-style coordinate system
    Coords coords;
    strcpy (coords.ctype, "RA---TAN");
    coords.crval1 = 0.0;
    coords.crval2 = 0.0;
    coords.crpix1 = 0.0;
    coords.crpix2 = 0.0;
    coords.cdelt1 = 1.0/3600;
    coords.cdelt2 = 1.0/3600;
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

    psMetadata *header1 = WriteCoordsToHeader(&coords);
    pmAstromWCS *wcs1 = pmAstromWCSfromHeader(header1);
    skip_start (wcs1 == NULL, 1, "*** WCS Conversion FAILS *** : skipping related tests");

    psMetadata *header2 = psMetadataAlloc();
    pmAstromWCStoHeader(header2, wcs1);
    pmAstromWCS *wcs2 = pmAstromWCSfromHeader(header2);

    ok (wcs2 != NULL, "converted WCS keywords to WCS astrometry");
    skip_start (wcs2 == NULL, 1, "*** WCS Conversion FAILS *** : skipping related tests");

    psSphere *sky1 = psSphereAlloc();
    psSphere *sky2 = psSphereAlloc();
    psPlane *chip = psPlaneAlloc();

    for (double x = -2000; x <= +2000; x+= 500.0) {
        for (double y = -2000; y <= +2000; y+= 500.0) {
            chip->x = x;
            chip->y = y;
            pmAstromWCStoSky (sky1, wcs1, chip);
            pmAstromWCStoSky (sky2, wcs2, chip);
            while (sky1->r > 2*M_PI)
                sky1->r -= 2*M_PI;
            while (sky1->r <      0)
                sky1->r += 2*M_PI;
            while (sky2->r > 2*M_PI)
                sky2->r -= 2*M_PI;
            while (sky2->r <      0)
                sky2->r += 2*M_PI;

            ok_float(sky1->r*PS_DEG_RAD, sky2->r*PS_DEG_RAD, "coordinate match: %f vs %f (delta = %f)", sky1->r*PS_DEG_RAD, sky2->r*PS_DEG_RAD, sky1->r*PS_DEG_RAD - sky2->r*PS_DEG_RAD);
            ok_float(sky1->d*PS_DEG_RAD, sky2->d*PS_DEG_RAD, "coordinate match: %f vs %f (delta = %f)", sky1->d*PS_DEG_RAD, sky2->d*PS_DEG_RAD, sky1->d*PS_DEG_RAD - sky2->d*PS_DEG_RAD);
        }
    }
    psFree (sky1);
    psFree (sky2);
    psFree (chip);

    skip_end();
    psFree (wcs2);
    psFree (header2);

    skip_end();
    psFree (wcs1);
    psFree (header1);

    ok(!psMemCheckLeaks (id, NULL, stderr, false), "no memory leaks");
}

void test2in()
{
    note("test pmAstromWCStoHeader");
    psMemId id = psMemGetId();

    // build a DVO-style coordinate system
    Coords coords;
    strcpy (coords.ctype, "RA---TAN");
    coords.crval1 = 0.0;
    coords.crval2 = 0.0;
    coords.crpix1 = 0.0;
    coords.crpix2 = 0.0;
    coords.cdelt1 = 1.0/3600;
    coords.cdelt2 = 1.0/3600;
    coords.pc1_1  = 0.9;
    coords.pc1_2  = -0.1;
    coords.pc2_1  = 0.1;
    coords.pc2_2  = 0.9;
    coords.Npolyterms = 0;
    for (int i = 0; i < 7; i++) {
        coords.polyterms[i][0] = 0.0;
        coords.polyterms[i][1] = 0.0;
    }

    psMetadata *header1 = WriteCoordsToHeader(&coords);
    pmAstromWCS *wcs1 = pmAstromWCSfromHeader(header1);
    skip_start (wcs1 == NULL, 1, "*** WCS Conversion FAILS *** : skipping related tests");

    psMetadata *header2 = psMetadataAlloc();
    pmAstromWCStoHeader(header2, wcs1);
    pmAstromWCS *wcs2 = pmAstromWCSfromHeader(header2);

    ok (wcs2 != NULL, "converted WCS keywords to WCS astrometry");
    skip_start (wcs2 == NULL, 1, "*** WCS Conversion FAILS *** : skipping related tests");

    psSphere *sky1 = psSphereAlloc();
    psSphere *sky2 = psSphereAlloc();
    psPlane *chip = psPlaneAlloc();

    for (double x = -2000; x <= +2000; x+= 500.0) {
        for (double y = -2000; y <= +2000; y+= 500.0) {
            chip->x = x;
            chip->y = y;
            pmAstromWCStoSky (sky1, wcs1, chip);
            pmAstromWCStoSky (sky2, wcs2, chip);
            while (sky1->r > 2*M_PI)
                sky1->r -= 2*M_PI;
            while (sky1->r <      0)
                sky1->r += 2*M_PI;
            while (sky2->r > 2*M_PI)
                sky2->r -= 2*M_PI;
            while (sky2->r <      0)
                sky2->r += 2*M_PI;

            ok_float(sky1->r*PS_DEG_RAD, sky2->r*PS_DEG_RAD, "coordinate match: %f vs %f (delta = %f)", sky1->r*PS_DEG_RAD, sky2->r*PS_DEG_RAD, sky1->r*PS_DEG_RAD - sky2->r*PS_DEG_RAD);
            ok_float(sky1->d*PS_DEG_RAD, sky2->d*PS_DEG_RAD, "coordinate match: %f vs %f (delta = %f)", sky1->d*PS_DEG_RAD, sky2->d*PS_DEG_RAD, sky1->d*PS_DEG_RAD - sky2->d*PS_DEG_RAD);
        }
    }
    psFree (sky1);
    psFree (sky2);
    psFree (chip);

    skip_end();
    psFree (wcs2);
    psFree (header2);

    skip_end();
    psFree (wcs1);
    psFree (header1);

    ok(!psMemCheckLeaks (id, NULL, stderr, false), "no memory leaks");
}

void test1in()
{
    note("test pmAstromWCStoHeader");
    psMemId id = psMemGetId();

    // build a DVO-style coordinate system
    Coords coords;
    strcpy (coords.ctype, "RA---TAN");
    coords.crval1 = 0.0;
    coords.crval2 = 0.0;
    coords.crpix1 = 0.0;
    coords.crpix2 = 0.0;
    coords.cdelt1 = 1.0/3600;
    coords.cdelt2 = 1.0/3600;
    coords.pc1_1  = 1.0;
    coords.pc1_2  = 0.0;
    coords.pc2_1  = 0.0;
    coords.pc2_2  = 1.0;
    coords.Npolyterms = 0;
    for (int i = 0; i < 7; i++) {
        coords.polyterms[i][0] = 0.0;
        coords.polyterms[i][1] = 0.0;
    }

    psMetadata *header1 = WriteCoordsToHeader(&coords);
    pmAstromWCS *wcs1 = pmAstromWCSfromHeader(header1);
    skip_start (wcs1 == NULL, 1, "*** WCS Conversion FAILS *** : skipping related tests");

    psMetadata *header2 = psMetadataAlloc();
    pmAstromWCStoHeader(header2, wcs1);
    pmAstromWCS *wcs2 = pmAstromWCSfromHeader(header2);

    ok (wcs2 != NULL, "converted WCS keywords to WCS astrometry");
    skip_start (wcs2 == NULL, 1, "*** WCS Conversion FAILS *** : skipping related tests");

    psSphere *sky1 = psSphereAlloc();
    psSphere *sky2 = psSphereAlloc();
    psPlane *chip = psPlaneAlloc();

    for (double x = -2000; x <= +2000; x+= 500.0) {
        for (double y = -2000; y <= +2000; y+= 500.0) {
            chip->x = x;
            chip->y = y;
            pmAstromWCStoSky (sky1, wcs1, chip);
            pmAstromWCStoSky (sky2, wcs2, chip);
            while (sky1->r > 2*M_PI)
                sky1->r -= 2*M_PI;
            while (sky1->r <      0)
                sky1->r += 2*M_PI;
            while (sky2->r > 2*M_PI)
                sky2->r -= 2*M_PI;
            while (sky2->r <      0)
                sky2->r += 2*M_PI;

            ok_float(sky1->r*PS_DEG_RAD, sky2->r*PS_DEG_RAD, "coordinate match: %f vs %f (delta = %f)", sky1->r*PS_DEG_RAD, sky2->r*PS_DEG_RAD, sky1->r*PS_DEG_RAD - sky2->r*PS_DEG_RAD);
            ok_float(sky1->d*PS_DEG_RAD, sky2->d*PS_DEG_RAD, "coordinate match: %f vs %f (delta = %f)", sky1->d*PS_DEG_RAD, sky2->d*PS_DEG_RAD, sky2->d*PS_DEG_RAD - sky2->d*PS_DEG_RAD);
        }
    }
    psFree (sky1);
    psFree (sky2);
    psFree (chip);

    skip_end();
    psFree (wcs2);
    psFree (header2);

    skip_end();
    psFree (wcs1);
    psFree (header1);

    ok(!psMemCheckLeaks (id, NULL, stderr, false), "no memory leaks");
}

void test3 ()
{
    note("test pmAstromWCSfromHeader ");
    psMemId id = psMemGetId();

    // build a DVO-style coordinate system
    Coords coords;
    strcpy (coords.ctype, "RA---TAN");
    coords.crval1 = 0.0;
    coords.crval2 = 0.0;
    coords.crpix1 = 0.0;
    coords.crpix2 = 0.0;
    coords.cdelt1 = 1.0/3600;
    coords.cdelt2 = 1.0/3600;
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

    psMetadata *header = WriteCoordsToHeader (&coords);
    pmAstromWCS *wcs = pmAstromWCSfromHeader (header);

    ok (wcs != NULL, "converted WCS keywords to WCS astrometry");
    skip_start (wcs == NULL, 1, "*** WCS Conversion FAILS *** : skipping related tests");

    psSphere *sky = psSphereAlloc();
    psPlane *chip = psPlaneAlloc();

    double rDVO, dDVO;
    for (double x = -2000; x <= +2000; x+= 500.0) {
        for (double y = -2000; y <= +2000; y+= 500.0) {
            XY_to_RD (&rDVO, &dDVO, x, y, &coords);
            chip->x = x;
            chip->y = y;
            pmAstromWCStoSky (sky, wcs, chip);
            while (sky->r > 2*M_PI)
                sky->r -= 2*M_PI;
            while (sky->r <      0)
                sky->r += 2*M_PI;

            ok_float(rDVO, sky->r*PS_DEG_RAD, "coordinate match: %f vs %f (delta = %f)", rDVO, sky->r*PS_DEG_RAD, rDVO - sky->r*PS_DEG_RAD);
            ok_float(dDVO, sky->d*PS_DEG_RAD, "coordinate match: %f vs %f (delta = %f)", dDVO, sky->d*PS_DEG_RAD, dDVO - sky->d*PS_DEG_RAD);
        }
    }
    psFree (sky);
    psFree (chip);

    skip_end();
    psFree (wcs);
    psFree (header);

    ok(!psMemCheckLeaks (id, NULL, stderr, false), "no memory leaks");
}

void test2 ()
{
    note("test pmAstromWCSfromHeader");
    psMemId id = psMemGetId();

    // build a DVO-style coordinate system
    Coords coords;
    strcpy (coords.ctype, "RA---TAN");
    coords.crval1 = 0.0;
    coords.crval2 = 0.0;
    coords.crpix1 = 0.0;
    coords.crpix2 = 0.0;
    coords.cdelt1 = 1.0/3600;
    coords.cdelt2 = 1.0/3600;
    coords.pc1_1  = 0.9;
    coords.pc1_2  = -0.1;
    coords.pc2_1  = 0.1;
    coords.pc2_2  = 0.9;
    coords.Npolyterms = 0;
    for (int i = 0; i < 7; i++) {
        coords.polyterms[i][0] = 0.0;
        coords.polyterms[i][1] = 0.0;
    }

    psMetadata *header = WriteCoordsToHeader (&coords);
    pmAstromWCS *wcs = pmAstromWCSfromHeader (header);

    ok (wcs != NULL, "converted WCS keywords to WCS astrometry");
    skip_start (wcs == NULL, 1, "*** WCS Conversion FAILS *** : skipping related tests");

    psSphere *sky = psSphereAlloc();
    psPlane *chip = psPlaneAlloc();

    double rDVO, dDVO;
    for (double x = -2000; x <= +2000; x+= 500.0) {
        for (double y = -2000; y <= +2000; y+= 500.0) {
            XY_to_RD (&rDVO, &dDVO, x, y, &coords);
            chip->x = x;
            chip->y = y;
            pmAstromWCStoSky (sky, wcs, chip);
            while (sky->r > 2*M_PI)
                sky->r -= 2*M_PI;
            while (sky->r <      0)
                sky->r += 2*M_PI;

            ok_float(rDVO, sky->r*PS_DEG_RAD, "coordinate match: %f vs %f (delta = %f)", rDVO, sky->r*PS_DEG_RAD, rDVO - sky->r*PS_DEG_RAD);
            ok_float(dDVO, sky->d*PS_DEG_RAD, "coordinate match: %f vs %f (delta = %f)", dDVO, sky->d*PS_DEG_RAD, dDVO - sky->d*PS_DEG_RAD);
        }
    }
    psFree (sky);
    psFree (chip);

    skip_end();
    psFree (wcs);
    psFree (header);

    ok(!psMemCheckLeaks (id, NULL, stderr, false), "no memory leaks");
}

void test1()
{
    note("test pmAstromWCSfromHeader");
    psMemId id = psMemGetId();

    // build a DVO-style coordinate system
    Coords coords;
    strcpy (coords.ctype, "RA---TAN");
    coords.crval1 = 0.0;
    coords.crval2 = 0.0;
    coords.crpix1 = 0.0;
    coords.crpix2 = 0.0;
    coords.cdelt1 = 1.0/3600;
    coords.cdelt2 = 1.0/3600;
    coords.pc1_1  = 1.0;
    coords.pc1_2  = 0.0;
    coords.pc2_1  = 0.0;
    coords.pc2_2  = 1.0;
    coords.Npolyterms = 0;
    for (int i = 0; i < 7; i++) {
        coords.polyterms[i][0] = 0.0;
        coords.polyterms[i][1] = 0.0;
    }

    psMetadata *header = WriteCoordsToHeader (&coords);
    pmAstromWCS *wcs = pmAstromWCSfromHeader (header);

    ok (wcs != NULL, "converted WCS keywords to WCS astrometry");
    skip_start (wcs == NULL, 1, "*** WCS Conversion FAILS *** : skipping related tests");

    psSphere *sky = psSphereAlloc();
    psPlane *chip = psPlaneAlloc();

    double rDVO, dDVO;
    for (double x = -2000; x <= +2000; x+= 500.0) {
        for (double y = -2000; y <= +2000; y+= 500.0) {
            XY_to_RD (&rDVO, &dDVO, x, y, &coords);
            chip->x = x;
            chip->y = y;
            pmAstromWCStoSky (sky, wcs, chip);
            while (sky->r > 2*M_PI)
                sky->r -= 2*M_PI;
            while (sky->r <      0)
                sky->r += 2*M_PI;

            ok_float(rDVO, sky->r*PS_DEG_RAD, "coordinate match: %f vs %f (delta = %f)", rDVO, sky->r*PS_DEG_RAD, rDVO - sky->r*PS_DEG_RAD);
            ok_float(dDVO, sky->d*PS_DEG_RAD, "coordinate match: %f vs %f (delta = %f)", dDVO, sky->d*PS_DEG_RAD, dDVO - sky->d*PS_DEG_RAD);
        }
    }
    psFree (sky);
    psFree (chip);

    skip_end();
    psFree (wcs);
    psFree (header);

    ok(!psMemCheckLeaks (id, NULL, stderr, false), "no memory leaks");
}

void testA()
{
    note("test coord allocs");
    psMemId id = psMemGetId();

    psPlane *chip = psPlaneAlloc();
    psFree (chip);
    ok(!psMemCheckLeaks (id, NULL, stderr, false), "no memory leaks");
}

void testB()
{
    note("test coord allocs");
    psMemId id = psMemGetId();

    psSphere *sky = psSphereAlloc();
    psFree (sky);
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
    psMetadataAddF32 (header, PS_LIST_TAIL, "CRVAL1", PS_META_REPLACE, "", coords[0].crval1);
    psMetadataAddF32 (header, PS_LIST_TAIL, "CRVAL2", PS_META_REPLACE, "", coords[0].crval2);

    // center coords (X,Y)
    psMetadataAddF32 (header, PS_LIST_TAIL, "CRPIX1", PS_META_REPLACE, "", coords[0].crpix1);
    psMetadataAddF32 (header, PS_LIST_TAIL, "CRPIX2", PS_META_REPLACE, "", coords[0].crpix2);

    // degrees per pixel
    psMetadataAddF32 (header, PS_LIST_TAIL, "CDELT1",  PS_META_REPLACE, "", coords[0].cdelt1);
    psMetadataAddF32 (header, PS_LIST_TAIL, "CDELT2",  PS_META_REPLACE, "", coords[0].cdelt2);

    // rotation matrix
    psMetadataAddF32 (header, PS_LIST_TAIL, "PC001001", PS_META_REPLACE, "", coords[0].pc1_1);
    psMetadataAddF32 (header, PS_LIST_TAIL, "PC001002", PS_META_REPLACE, "", coords[0].pc1_2);
    psMetadataAddF32 (header, PS_LIST_TAIL, "PC002001", PS_META_REPLACE, "", coords[0].pc2_1);
    psMetadataAddF32 (header, PS_LIST_TAIL, "PC002002", PS_META_REPLACE, "", coords[0].pc2_2);

    psMetadataAddF32 (header, PS_LIST_TAIL, "PCA1X2Y0", PS_META_REPLACE, "", coords[0].polyterms[0][0]);
    psMetadataAddF32 (header, PS_LIST_TAIL, "PCA1X1Y1", PS_META_REPLACE, "", coords[0].polyterms[1][0]);
    psMetadataAddF32 (header, PS_LIST_TAIL, "PCA1X0Y2", PS_META_REPLACE, "", coords[0].polyterms[2][0]);
    psMetadataAddF32 (header, PS_LIST_TAIL, "PCA2X2Y0", PS_META_REPLACE, "", coords[0].polyterms[0][1]);
    psMetadataAddF32 (header, PS_LIST_TAIL, "PCA2X1Y1", PS_META_REPLACE, "", coords[0].polyterms[1][1]);
    psMetadataAddF32 (header, PS_LIST_TAIL, "PCA2X0Y2", PS_META_REPLACE, "", coords[0].polyterms[2][1]);
    psMetadataAddF32 (header, PS_LIST_TAIL, "PCA1X3Y0", PS_META_REPLACE, "", coords[0].polyterms[3][0]);
    psMetadataAddF32 (header, PS_LIST_TAIL, "PCA1X2Y1", PS_META_REPLACE, "", coords[0].polyterms[4][0]);
    psMetadataAddF32 (header, PS_LIST_TAIL, "PCA1X1Y2", PS_META_REPLACE, "", coords[0].polyterms[5][0]);
    psMetadataAddF32 (header, PS_LIST_TAIL, "PCA1X0Y3", PS_META_REPLACE, "", coords[0].polyterms[6][0]);
    psMetadataAddF32 (header, PS_LIST_TAIL, "PCA2X3Y0", PS_META_REPLACE, "", coords[0].polyterms[3][1]);
    psMetadataAddF32 (header, PS_LIST_TAIL, "PCA2X2Y1", PS_META_REPLACE, "", coords[0].polyterms[4][1]);
    psMetadataAddF32 (header, PS_LIST_TAIL, "PCA2X1Y2", PS_META_REPLACE, "", coords[0].polyterms[5][1]);
    psMetadataAddF32 (header, PS_LIST_TAIL, "PCA2X0Y3", PS_META_REPLACE, "", coords[0].polyterms[6][1]);

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

