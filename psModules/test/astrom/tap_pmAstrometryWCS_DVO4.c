#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <stdio.h>
#include <string.h>
#include <pslib.h>
#include <psmodules.h>

#include "tap.h"
#include "pstap.h"

// HAVA_KAPA checks for Ohana libraries, needed for the Coords structure
# if (HAVE_KAPA)
# include "dvo.h"

psMetadata *WriteCoordsToHeader (Coords *coords);
void test1(); // basic WRP+DIS projections,
void test2(); // small rotation
void test3(); // 2nd order terms in WRP
void test4(); // small rotation in WRP and DIS
void test5(); // 2nd order terms in WRP and DIS
void test1x(); // basic WRP+DIS projections with central offset
void test2x(); // small rotation with central offset
void test3x(); // 2nd order term in WRP with central offset

int main (void)
{
    plan_tests(1329);

    note("pmAstromWriteWCS tests compared with DVO coords routines");

    test1();
    test2();
    test3();
    test4();
    test5();
    test1x();
    test2x();
    test3x();

    return exit_status();
}

// create a fake chip-level mosaic header
void test1()
{
    note("test pmAstrom Read,Write BilevelChip");
    psMemId id = psMemGetId();

    // build a DVO-style mosaic coordinate system
    // chip-level data (chip -> fpa)
    Coords coords;
    strcpy (coords.ctype, "RA---WRP");
    coords.crval1 = 0.0;
    coords.crval2 = 0.0;
    coords.crpix1 = 0.0;
    coords.crpix2 = 0.0;
    coords.cdelt1 = 10.0; // microns per pixel
    coords.cdelt2 = 10.0; // microns per pixel
    coords.pc1_1  = 1.0;
    coords.pc1_2  = 0.0;
    coords.pc2_1  = 0.0;
    coords.pc2_2  = 1.0;
    coords.Npolyterms = 0;
    for (int i = 0; i < 7; i++) {
        coords.polyterms[i][0] = 0.0;
        coords.polyterms[i][1] = 0.0;
    }

    // mosaic-level data (fpa->sky)
    Coords mosaic;
    strcpy (mosaic.ctype, "RA---DIS");
    mosaic.crval1 = 0.0;
    mosaic.crval2 = 0.0;
    mosaic.crpix1 = 0.0;
    mosaic.crpix2 = 0.0;
    mosaic.cdelt1 = 1.0 / 10.0 / 3600.0; // degrees per micron
    mosaic.cdelt2 = 1.0 / 10.0 / 3600.0; // degrees per micron
    mosaic.pc1_1  = 1.0;
    mosaic.pc1_2  = 0.0;
    mosaic.pc2_1  = 0.0;
    mosaic.pc2_2  = 1.0;
    mosaic.Npolyterms = 0;
    for (int i = 0; i < 7; i++) {
        mosaic.polyterms[i][0] = 0.0;
        mosaic.polyterms[i][1] = 0.0;
    }
    RegisterMosaic (&mosaic);

    psMetadata *headerChp = WriteCoordsToHeader (&coords);
    psMetadata *headerMos = WriteCoordsToHeader (&mosaic);

    pmFPA *fpa = pmFPAAlloc (NULL, NULL);
    pmChip *chip = pmChipAlloc (fpa, NULL);

    bool status = pmAstromReadBilevelChip (chip, headerChp);
    ok (status, "read bilevel chip header");
    skip_start (!status, 1, "*** WCS Conversion FAILS *** : skipping related tests");

    bool status = pmAstromReadBilevelMosaic (fpa, headerMos);
    ok (status, "read bilevel fpa header");
    skip_start (!status, 1, "*** WCS Conversion FAILS *** : skipping related tests");

    psPlane  *onChip = psPlaneAlloc();
    psPlane  *onFPA  = psPlaneAlloc();
    psPlane  *onTPA  = psPlaneAlloc();
    psSphere *onSky  = psSphereAlloc();

    double rDVO, dDVO;
    for (double x = -2000; x <= +2000; x+= 500.0) {
        for (double y = -2000; y <= +2000; y+= 500.0) {
            XY_to_RD (&rDVO, &dDVO, x, y, &coords);

            onChip->x = x;
            onChip->y = y;

            psPlaneTransformApply (onFPA, chip->toFPA, onChip);
            psPlaneTransformApply (onTPA, fpa->toTPA, onFPA);
            psDeproject (onSky, onTPA, fpa->toSky);

            while (onSky->r > 2*M_PI)
                onSky->r -= 2*M_PI;
            while (onSky->r <      0)
                onSky->r += 2*M_PI;

            // fprintf (stderr, "fpa x: %f vs %f : %f\n", rDVO, onFPA->x, rDVO - onFPA->x);
            // fprintf (stderr, "fpa y: %f vs %f : %f\n", dDVO, onFPA->y, dDVO - onFPA->y);

            ok_float(onSky->r*PM_DEG_RAD, rDVO, "coordinate match: %f vs %f (delta = %f)", onSky->r*PM_DEG_RAD, rDVO, 3600.0*(onSky->r*PM_DEG_RAD - rDVO));
            ok_float(onSky->d*PM_DEG_RAD, dDVO, "coordinate match: %f vs %f (delta = %f)", onSky->d*PM_DEG_RAD, dDVO, 3600.0*(onSky->d*PM_DEG_RAD - dDVO));
        }
    }
    psFree (onSky);
    psFree (onTPA);
    psFree (onFPA);
    psFree (onChip);

    skip_end();
    skip_end();
    psFree (fpa);
    psFree (chip);

    psFree (headerMos);
    psFree (headerChp);

    ok(!psMemCheckLeaks (id, NULL, stderr, false), "no memory leaks");
}

// create a fake chip-level mosaic header
void test2()
{
    note("test pmAstrom Read,Write BilevelChip");
    psMemId id = psMemGetId();

    // build a DVO-style mosaic coordinate system
    // chip-level data (chip -> fpa)
    Coords coords;
    strcpy (coords.ctype, "RA---WRP");
    coords.crval1 = 0.0;
    coords.crval2 = 0.0;
    coords.crpix1 = 0.0;
    coords.crpix2 = 0.0;
    coords.cdelt1 = 10.0; // microns per pixel
    coords.cdelt2 = 10.0; // microns per pixel
    coords.pc1_1  = 0.9;
    coords.pc1_2  = 0.1;
    coords.pc2_1  =-0.1;
    coords.pc2_2  = 0.9;
    coords.Npolyterms = 0;
    for (int i = 0; i < 7; i++) {
        coords.polyterms[i][0] = 0.0;
        coords.polyterms[i][1] = 0.0;
    }

    // mosaic-level data (fpa->sky)
    Coords mosaic;
    strcpy (mosaic.ctype, "RA---DIS");
    mosaic.crval1 = 0.0;
    mosaic.crval2 = 0.0;
    mosaic.crpix1 = 0.0;
    mosaic.crpix2 = 0.0;
    mosaic.cdelt1 = 1.0 / 10.0 / 3600.0; // degrees per micron
    mosaic.cdelt2 = 1.0 / 10.0 / 3600.0; // degrees per micron
    mosaic.pc1_1  = 1.0;
    mosaic.pc1_2  = 0.0;
    mosaic.pc2_1  = 0.0;
    mosaic.pc2_2  = 1.0;
    mosaic.Npolyterms = 0;
    for (int i = 0; i < 7; i++) {
        mosaic.polyterms[i][0] = 0.0;
        mosaic.polyterms[i][1] = 0.0;
    }
    RegisterMosaic (&mosaic);

    psMetadata *headerChp = WriteCoordsToHeader (&coords);
    psMetadata *headerMos = WriteCoordsToHeader (&mosaic);

    pmFPA *fpa = pmFPAAlloc (NULL, NULL);
    pmChip *chip = pmChipAlloc (fpa, NULL);

    bool status = pmAstromReadBilevelChip (chip, headerChp);
    ok (status, "read bilevel chip header");
    skip_start (!status, 1, "*** WCS Conversion FAILS *** : skipping related tests");

    bool status = pmAstromReadBilevelMosaic (fpa, headerMos);
    ok (status, "read bilevel fpa header");
    skip_start (!status, 1, "*** WCS Conversion FAILS *** : skipping related tests");

    psPlane  *onChip = psPlaneAlloc();
    psPlane  *onFPA  = psPlaneAlloc();
    psPlane  *onTPA  = psPlaneAlloc();
    psSphere *onSky  = psSphereAlloc();

    double rDVO, dDVO;
    for (double x = -2000; x <= +2000; x+= 500.0) {
        for (double y = -2000; y <= +2000; y+= 500.0) {
            XY_to_RD (&rDVO, &dDVO, x, y, &coords);

            onChip->x = x;
            onChip->y = y;

            psPlaneTransformApply (onFPA, chip->toFPA, onChip);
            psPlaneTransformApply (onTPA, fpa->toTPA, onFPA);
            psDeproject (onSky, onTPA, fpa->toSky);

            while (onSky->r > 2*M_PI)
                onSky->r -= 2*M_PI;
            while (onSky->r <      0)
                onSky->r += 2*M_PI;

            // fprintf (stderr, "fpa x: %f vs %f : %f\n", rDVO, onFPA->x, rDVO - onFPA->x);
            // fprintf (stderr, "fpa y: %f vs %f : %f\n", dDVO, onFPA->y, dDVO - onFPA->y);

            ok_float(onSky->r*PS_DEG_RAD, rDVO, "coordinate match: %f vs %f (delta = %f)", onSky->r*PS_DEG_RAD, rDVO, 3600.0*(onSky->r*PS_DEG_RAD - rDVO));
            ok_float(onSky->d*PS_DEG_RAD, dDVO, "coordinate match: %f vs %f (delta = %f)", onSky->d*PS_DEG_RAD, dDVO, 3600.0*(onSky->d*PS_DEG_RAD - dDVO));
        }
    }
    psFree (onSky);
    psFree (onTPA);
    psFree (onFPA);
    psFree (onChip);

    skip_end();
    skip_end();
    psFree (fpa);
    psFree (chip);

    psFree (headerMos);
    psFree (headerChp);

    ok(!psMemCheckLeaks (id, NULL, stderr, false), "no memory leaks");
}

// create a fake chip-level mosaic header
void test3()
{
    note("test pmAstrom Read,Write BilevelChip");
    psMemId id = psMemGetId();

    // build a DVO-style mosaic coordinate system
    // chip-level data (chip -> fpa)
    Coords coords;
    strcpy (coords.ctype, "RA---WRP");
    coords.crval1 = 0.0;
    coords.crval2 = 0.0;
    coords.crpix1 = 0.0;
    coords.crpix2 = 0.0;
    coords.cdelt1 = 10.0; // microns per pixel
    coords.cdelt2 = 10.0; // microns per pixel
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

    // mosaic-level data (fpa->sky)
    Coords mosaic;
    strcpy (mosaic.ctype, "RA---DIS");
    mosaic.crval1 = 0.0;
    mosaic.crval2 = 0.0;
    mosaic.crpix1 = 0.0;
    mosaic.crpix2 = 0.0;
    mosaic.cdelt1 = 1.0 / 10.0 / 3600.0; // degrees per micron
    mosaic.cdelt2 = 1.0 / 10.0 / 3600.0; // degrees per micron
    mosaic.pc1_1  = 1.0;
    mosaic.pc1_2  = 0.0;
    mosaic.pc2_1  = 0.0;
    mosaic.pc2_2  = 1.0;
    mosaic.Npolyterms = 0;
    for (int i = 0; i < 7; i++) {
        mosaic.polyterms[i][0] = 0.0;
        mosaic.polyterms[i][1] = 0.0;
    }
    RegisterMosaic (&mosaic);

    psMetadata *headerChp = WriteCoordsToHeader (&coords);
    psMetadata *headerMos = WriteCoordsToHeader (&mosaic);

    pmFPA *fpa = pmFPAAlloc (NULL, NULL);
    pmChip *chip = pmChipAlloc (fpa, NULL);

    bool status = pmAstromReadBilevelChip (chip, headerChp);
    ok (status, "read bilevel chip header");
    skip_start (!status, 1, "*** WCS Conversion FAILS *** : skipping related tests");

    bool status = pmAstromReadBilevelMosaic (fpa, headerMos);
    ok (status, "read bilevel fpa header");
    skip_start (!status, 1, "*** WCS Conversion FAILS *** : skipping related tests");

    psPlane  *onChip = psPlaneAlloc();
    psPlane  *onFPA  = psPlaneAlloc();
    psPlane  *onTPA  = psPlaneAlloc();
    psSphere *onSky  = psSphereAlloc();

    double rDVO, dDVO;
    for (double x = -2000; x <= +2000; x+= 500.0) {
        for (double y = -2000; y <= +2000; y+= 500.0) {
            XY_to_RD (&rDVO, &dDVO, x, y, &coords);

            onChip->x = x;
            onChip->y = y;

            psPlaneTransformApply (onFPA, chip->toFPA, onChip);
            psPlaneTransformApply (onTPA, fpa->toTPA, onFPA);
            psDeproject (onSky, onTPA, fpa->toSky);

            while (onSky->r > 2*M_PI)
                onSky->r -= 2*M_PI;
            while (onSky->r <      0)
                onSky->r += 2*M_PI;

            // fprintf (stderr, "fpa x: %f vs %f : %f\n", rDVO, onFPA->x, rDVO - onFPA->x);
            // fprintf (stderr, "fpa y: %f vs %f : %f\n", dDVO, onFPA->y, dDVO - onFPA->y);

            ok_float(onSky->r*PM_DEG_RAD, rDVO, "coordinate match: %f vs %f (delta = %f)", onSky->r*PM_DEG_RAD, rDVO, 3600.0*(onSky->r*PM_DEG_RAD - rDVO));
            ok_float(onSky->d*PM_DEG_RAD, dDVO, "coordinate match: %f vs %f (delta = %f)", onSky->d*PM_DEG_RAD, dDVO, 3600.0*(onSky->d*PM_DEG_RAD - dDVO));
        }
    }
    psFree (onSky);
    psFree (onTPA);
    psFree (onFPA);
    psFree (onChip);

    skip_end();
    skip_end();
    psFree (fpa);
    psFree (chip);

    psFree (headerMos);
    psFree (headerChp);

    ok(!psMemCheckLeaks (id, NULL, stderr, false), "no memory leaks");
}

// create a fake chip-level mosaic header
void test4()
{
    note("test pmAstrom Read,Write BilevelChip");
    psMemId id = psMemGetId();

    // build a DVO-style mosaic coordinate system
    // chip-level data (chip -> fpa)
    Coords coords;
    strcpy (coords.ctype, "RA---WRP");
    coords.crval1 = 0.0;
    coords.crval2 = 0.0;
    coords.crpix1 = 0.0;
    coords.crpix2 = 0.0;
    coords.cdelt1 = 10.0; // microns per pixel
    coords.cdelt2 = 10.0; // microns per pixel
    coords.pc1_1  = 0.9;
    coords.pc1_2  = 0.1;
    coords.pc2_1  =-0.1;
    coords.pc2_2  = 0.9;
    coords.Npolyterms = 0;
    for (int i = 0; i < 7; i++) {
        coords.polyterms[i][0] = 0.0;
        coords.polyterms[i][1] = 0.0;
    }

    // mosaic-level data (fpa->sky)
    Coords mosaic;
    strcpy (mosaic.ctype, "RA---DIS");
    mosaic.crval1 = 0.0;
    mosaic.crval2 = 0.0;
    mosaic.crpix1 = 0.0;
    mosaic.crpix2 = 0.0;
    mosaic.cdelt1 = 1.0 / 10.0 / 3600.0; // degrees per micron
    mosaic.cdelt2 = 1.0 / 10.0 / 3600.0; // degrees per micron
    mosaic.pc1_1  = 0.9;
    mosaic.pc1_2  =-0.1;
    mosaic.pc2_1  = 0.1;
    mosaic.pc2_2  = 0.9;
    mosaic.Npolyterms = 0;
    for (int i = 0; i < 7; i++) {
        mosaic.polyterms[i][0] = 0.0;
        mosaic.polyterms[i][1] = 0.0;
    }
    RegisterMosaic (&mosaic);

    psMetadata *headerChp = WriteCoordsToHeader (&coords);
    psMetadata *headerMos = WriteCoordsToHeader (&mosaic);

    pmFPA *fpa = pmFPAAlloc (NULL, NULL);
    pmChip *chip = pmChipAlloc (fpa, NULL);

    bool status = pmAstromReadBilevelChip (chip, headerChp);
    ok (status, "read bilevel chip header");
    skip_start (!status, 1, "*** WCS Conversion FAILS *** : skipping related tests");

    bool status = pmAstromReadBilevelMosaic (fpa, headerMos);
    ok (status, "read bilevel fpa header");
    skip_start (!status, 1, "*** WCS Conversion FAILS *** : skipping related tests");

    psPlane  *onChip = psPlaneAlloc();
    psPlane  *onFPA  = psPlaneAlloc();
    psPlane  *onTPA  = psPlaneAlloc();
    psSphere *onSky  = psSphereAlloc();

    double rDVO, dDVO;
    for (double x = -2000; x <= +2000; x+= 500.0) {
        for (double y = -2000; y <= +2000; y+= 500.0) {
            XY_to_RD (&rDVO, &dDVO, x, y, &coords);

            onChip->x = x;
            onChip->y = y;

            psPlaneTransformApply (onFPA, chip->toFPA, onChip);
            psPlaneTransformApply (onTPA, fpa->toTPA, onFPA);
            psDeproject (onSky, onTPA, fpa->toSky);

            while (onSky->r > 2*M_PI)
                onSky->r -= 2*M_PI;
            while (onSky->r <      0)
                onSky->r += 2*M_PI;

            // fprintf (stderr, "fpa x: %f vs %f : %f\n", rDVO, onFPA->x, rDVO - onFPA->x);
            // fprintf (stderr, "fpa y: %f vs %f : %f\n", dDVO, onFPA->y, dDVO - onFPA->y);

            ok_float(onSky->r*PM_DEG_RAD, rDVO, "coordinate match: %f vs %f (delta = %f)", onSky->r*PM_DEG_RAD, rDVO, 3600.0*(onSky->r*PM_DEG_RAD - rDVO));
            ok_float(onSky->d*PM_DEG_RAD, dDVO, "coordinate match: %f vs %f (delta = %f)", onSky->d*PM_DEG_RAD, dDVO, 3600.0*(onSky->d*PM_DEG_RAD - dDVO));
        }
    }
    psFree (onSky);
    psFree (onTPA);
    psFree (onFPA);
    psFree (onChip);

    skip_end();
    skip_end();
    psFree (fpa);
    psFree (chip);

    psFree (headerMos);
    psFree (headerChp);

    ok(!psMemCheckLeaks (id, NULL, stderr, false), "no memory leaks");
}

// create a fake chip-level mosaic header
void test5()
{
    note("test pmAstrom Read,Write BilevelChip");
    psMemId id = psMemGetId();

    // build a DVO-style mosaic coordinate system
    // chip-level data (chip -> fpa)
    Coords coords;
    strcpy (coords.ctype, "RA---WRP");
    coords.crval1 = 0.0;
    coords.crval2 = 0.0;
    coords.crpix1 = 0.0;
    coords.crpix2 = 0.0;
    coords.cdelt1 = 10.0; // microns per pixel
    coords.cdelt2 = 10.0; // microns per pixel
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

    // mosaic-level data (fpa->sky)
    Coords mosaic;
    strcpy (mosaic.ctype, "RA---DIS");
    mosaic.crval1 = 0.0;
    mosaic.crval2 = 0.0;
    mosaic.crpix1 = 0.0;
    mosaic.crpix2 = 0.0;
    mosaic.cdelt1 = 1.0 / 10.0 / 3600.0; // degrees per micron
    mosaic.cdelt2 = 1.0 / 10.0 / 3600.0; // degrees per micron
    mosaic.pc1_1  = 1.0;
    mosaic.pc1_2  = 0.0;
    mosaic.pc2_1  = 0.0;
    mosaic.pc2_2  = 1.0;
    mosaic.Npolyterms = 2;
    for (int i = 0; i < 7; i++) {
        mosaic.polyterms[i][0] = 0.0;
        mosaic.polyterms[i][1] = 0.0;
    }
    RegisterMosaic (&mosaic);
    mosaic.polyterms[0][0] = 0.01; // L vs X^2
    mosaic.polyterms[2][1] = 0.01; // M vs Y^2

    psMetadata *headerChp = WriteCoordsToHeader (&coords);
    psMetadata *headerMos = WriteCoordsToHeader (&mosaic);

    pmFPA *fpa = pmFPAAlloc (NULL, NULL);
    pmChip *chip = pmChipAlloc (fpa, NULL);

    bool status = pmAstromReadBilevelChip (chip, headerChp);
    ok (status, "read bilevel chip header");
    skip_start (!status, 1, "*** WCS Conversion FAILS *** : skipping related tests");

    bool status = pmAstromReadBilevelMosaic (fpa, headerMos);
    ok (status, "read bilevel fpa header");
    skip_start (!status, 1, "*** WCS Conversion FAILS *** : skipping related tests");

    psPlane  *onChip = psPlaneAlloc();
    psPlane  *onFPA  = psPlaneAlloc();
    psPlane  *onTPA  = psPlaneAlloc();
    psSphere *onSky  = psSphereAlloc();

    double rDVO, dDVO;
    for (double x = -2000; x <= +2000; x+= 500.0) {
        for (double y = -2000; y <= +2000; y+= 500.0) {
            XY_to_RD (&rDVO, &dDVO, x, y, &coords);

            onChip->x = x;
            onChip->y = y;

            psPlaneTransformApply (onFPA, chip->toFPA, onChip);
            psPlaneTransformApply (onTPA, fpa->toTPA, onFPA);
            psDeproject (onSky, onTPA, fpa->toSky);

            while (onSky->r > 2*M_PI)
                onSky->r -= 2*M_PI;
            while (onSky->r <      0)
                onSky->r += 2*M_PI;

            // fprintf (stderr, "fpa x: %f vs %f : %f\n", rDVO, onFPA->x, rDVO - onFPA->x);
            // fprintf (stderr, "fpa y: %f vs %f : %f\n", dDVO, onFPA->y, dDVO - onFPA->y);

            ok_float(onSky->r*PM_DEG_RAD, rDVO, "coordinate match: %f vs %f (delta = %f)", onSky->r*PM_DEG_RAD, rDVO, 3600.0*(onSky->r*PM_DEG_RAD - rDVO));
            ok_float(onSky->d*PM_DEG_RAD, dDVO, "coordinate match: %f vs %f (delta = %f)", onSky->d*PM_DEG_RAD, dDVO, 3600.0*(onSky->d*PM_DEG_RAD - dDVO));
        }
    }
    psFree (onSky);
    psFree (onTPA);
    psFree (onFPA);
    psFree (onChip);

    skip_end();
    skip_end();
    psFree (fpa);
    psFree (chip);

    psFree (headerMos);
    psFree (headerChp);

    ok(!psMemCheckLeaks (id, NULL, stderr, false), "no memory leaks");
}

// create a fake chip-level mosaic header
void test1x()
{
    note("test pmAstrom Read,Write BilevelChip");
    psMemId id = psMemGetId();

    // build a DVO-style mosaic coordinate system
    // chip-level data (chip -> fpa)
    Coords coords;
    strcpy (coords.ctype, "RA---WRP");
    coords.crval1 = 0.0;
    coords.crval2 = 0.0;
    coords.crpix1 = +50.0;
    coords.crpix2 = -20.0;
    coords.cdelt1 = 10.0; // microns per pixel
    coords.cdelt2 = 10.0; // microns per pixel
    coords.pc1_1  = 1.0;
    coords.pc1_2  = 0.0;
    coords.pc2_1  = 0.0;
    coords.pc2_2  = 1.0;
    coords.Npolyterms = 0;
    for (int i = 0; i < 7; i++) {
        coords.polyterms[i][0] = 0.0;
        coords.polyterms[i][1] = 0.0;
    }

    // mosaic-level data (fpa->sky)
    Coords mosaic;
    strcpy (mosaic.ctype, "RA---DIS");
    mosaic.crval1 = 0.0;
    mosaic.crval2 = 0.0;
    mosaic.crpix1 = 0.0;
    mosaic.crpix2 = 0.0;
    mosaic.cdelt1 = 1.0 / 10.0 / 3600.0; // degrees per micron
    mosaic.cdelt2 = 1.0 / 10.0 / 3600.0; // degrees per micron
    mosaic.pc1_1  = 1.0;
    mosaic.pc1_2  = 0.0;
    mosaic.pc2_1  = 0.0;
    mosaic.pc2_2  = 1.0;
    mosaic.Npolyterms = 0;
    for (int i = 0; i < 7; i++) {
        mosaic.polyterms[i][0] = 0.0;
        mosaic.polyterms[i][1] = 0.0;
    }
    RegisterMosaic (&mosaic);

    psMetadata *headerChp = WriteCoordsToHeader (&coords);
    psMetadata *headerMos = WriteCoordsToHeader (&mosaic);

    pmFPA *fpa = pmFPAAlloc (NULL, NULL);
    pmChip *chip = pmChipAlloc (fpa, NULL);

    bool status = pmAstromReadBilevelChip (chip, headerChp);
    ok (status, "read bilevel chip header");
    skip_start (!status, 1, "*** WCS Conversion FAILS *** : skipping related tests");

    bool status = pmAstromReadBilevelMosaic (fpa, headerMos);
    ok (status, "read bilevel fpa header");
    skip_start (!status, 1, "*** WCS Conversion FAILS *** : skipping related tests");

    psPlane  *onChip = psPlaneAlloc();
    psPlane  *onFPA  = psPlaneAlloc();
    psPlane  *onTPA  = psPlaneAlloc();
    psSphere *onSky  = psSphereAlloc();

    double rDVO, dDVO;
    for (double x = -2000; x <= +2000; x+= 500.0) {
        for (double y = -2000; y <= +2000; y+= 500.0) {
            XY_to_RD (&rDVO, &dDVO, x, y, &coords);

            onChip->x = x;
            onChip->y = y;

            psPlaneTransformApply (onFPA, chip->toFPA, onChip);
            psPlaneTransformApply (onTPA, fpa->toTPA, onFPA);
            psDeproject (onSky, onTPA, fpa->toSky);

            while (onSky->r > 2*M_PI)
                onSky->r -= 2*M_PI;
            while (onSky->r <      0)
                onSky->r += 2*M_PI;

            // fprintf (stderr, "fpa x: %f vs %f : %f\n", rDVO, onFPA->x, rDVO - onFPA->x);
            // fprintf (stderr, "fpa y: %f vs %f : %f\n", dDVO, onFPA->y, dDVO - onFPA->y);

            ok_float(onSky->r*PM_DEG_RAD, rDVO, "coordinate match: %f vs %f (delta = %f)", onSky->r*PM_DEG_RAD, rDVO, 3600.0*(onSky->r*PM_DEG_RAD - rDVO));
            ok_float(onSky->d*PM_DEG_RAD, dDVO, "coordinate match: %f vs %f (delta = %f)", onSky->d*PM_DEG_RAD, dDVO, 3600.0*(onSky->d*PM_DEG_RAD - dDVO));
        }
    }
    psFree (onSky);
    psFree (onTPA);
    psFree (onFPA);
    psFree (onChip);

    skip_end();
    skip_end();
    psFree (fpa);
    psFree (chip);

    psFree (headerMos);
    psFree (headerChp);

    ok(!psMemCheckLeaks (id, NULL, stderr, false), "no memory leaks");
}

// create a fake chip-level mosaic header
void test2x()
{
    note("test pmAstrom Read,Write BilevelChip");
    psMemId id = psMemGetId();

    // build a DVO-style mosaic coordinate system
    // chip-level data (chip -> fpa)
    Coords coords;
    strcpy (coords.ctype, "RA---WRP");
    coords.crval1 = 0.0;
    coords.crval2 = 0.0;
    coords.crpix1 = +50.0;
    coords.crpix2 = -20.0;
    coords.cdelt1 = 10.0; // microns per pixel
    coords.cdelt2 = 10.0; // microns per pixel
    coords.pc1_1  = 0.9;
    coords.pc1_2  = 0.1;
    coords.pc2_1  =-0.1;
    coords.pc2_2  = 0.9;
    coords.Npolyterms = 0;
    for (int i = 0; i < 7; i++) {
        coords.polyterms[i][0] = 0.0;
        coords.polyterms[i][1] = 0.0;
    }

    // mosaic-level data (fpa->sky)
    Coords mosaic;
    strcpy (mosaic.ctype, "RA---DIS");
    mosaic.crval1 = 0.0;
    mosaic.crval2 = 0.0;
    mosaic.crpix1 = 0.0;
    mosaic.crpix2 = 0.0;
    mosaic.cdelt1 = 1.0 / 10.0 / 3600.0; // degrees per micron
    mosaic.cdelt2 = 1.0 / 10.0 / 3600.0; // degrees per micron
    mosaic.pc1_1  = 1.0;
    mosaic.pc1_2  = 0.0;
    mosaic.pc2_1  = 0.0;
    mosaic.pc2_2  = 1.0;
    mosaic.Npolyterms = 0;
    for (int i = 0; i < 7; i++) {
        mosaic.polyterms[i][0] = 0.0;
        mosaic.polyterms[i][1] = 0.0;
    }
    RegisterMosaic (&mosaic);

    psMetadata *headerChp = WriteCoordsToHeader (&coords);
    psMetadata *headerMos = WriteCoordsToHeader (&mosaic);

    pmFPA *fpa = pmFPAAlloc (NULL, NULL);
    pmChip *chip = pmChipAlloc (fpa, NULL);

    bool status = pmAstromReadBilevelChip (chip, headerChp);
    ok (status, "read bilevel chip header");
    skip_start (!status, 1, "*** WCS Conversion FAILS *** : skipping related tests");

    bool status = pmAstromReadBilevelMosaic (fpa, headerMos);
    ok (status, "read bilevel fpa header");
    skip_start (!status, 1, "*** WCS Conversion FAILS *** : skipping related tests");

    psPlane  *onChip = psPlaneAlloc();
    psPlane  *onFPA  = psPlaneAlloc();
    psPlane  *onTPA  = psPlaneAlloc();
    psSphere *onSky  = psSphereAlloc();

    double rDVO, dDVO;
    for (double x = -2000; x <= +2000; x+= 500.0) {
        for (double y = -2000; y <= +2000; y+= 500.0) {
            XY_to_RD (&rDVO, &dDVO, x, y, &coords);

            onChip->x = x;
            onChip->y = y;

            psPlaneTransformApply (onFPA, chip->toFPA, onChip);
            psPlaneTransformApply (onTPA, fpa->toTPA, onFPA);
            psDeproject (onSky, onTPA, fpa->toSky);

            while (onSky->r > 2*M_PI)
                onSky->r -= 2*M_PI;
            while (onSky->r <      0)
                onSky->r += 2*M_PI;

            // fprintf (stderr, "fpa x: %f vs %f : %f\n", rDVO, onFPA->x, rDVO - onFPA->x);
            // fprintf (stderr, "fpa y: %f vs %f : %f\n", dDVO, onFPA->y, dDVO - onFPA->y);

            ok_float(onSky->r*PM_DEG_RAD, rDVO, "coordinate match: %f vs %f (delta = %f)", onSky->r*PM_DEG_RAD, rDVO, 3600.0*(onSky->r*PM_DEG_RAD - rDVO));
            ok_float(onSky->d*PM_DEG_RAD, dDVO, "coordinate match: %f vs %f (delta = %f)", onSky->d*PM_DEG_RAD, dDVO, 3600.0*(onSky->d*PM_DEG_RAD - dDVO));
        }
    }
    psFree (onSky);
    psFree (onTPA);
    psFree (onFPA);
    psFree (onChip);

    skip_end();
    skip_end();
    psFree (fpa);
    psFree (chip);

    psFree (headerMos);
    psFree (headerChp);

    ok(!psMemCheckLeaks (id, NULL, stderr, false), "no memory leaks");
}

// create a fake chip-level mosaic header
void test3x()
{
    note("test pmAstrom Read,Write BilevelChip");
    psMemId id = psMemGetId();

    // build a DVO-style mosaic coordinate system
    // chip-level data (chip -> fpa)
    Coords coords;
    strcpy (coords.ctype, "RA---WRP");
    coords.crval1 = 0.0;
    coords.crval2 = 0.0;
    coords.crpix1 = +50.0;
    coords.crpix2 = -20.0;
    coords.cdelt1 = 10.0; // microns per pixel
    coords.cdelt2 = 10.0; // microns per pixel
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

    // mosaic-level data (fpa->sky)
    Coords mosaic;
    strcpy (mosaic.ctype, "RA---DIS");
    mosaic.crval1 = 0.0;
    mosaic.crval2 = 0.0;
    mosaic.crpix1 = 0.0;
    mosaic.crpix2 = 0.0;
    mosaic.cdelt1 = 1.0 / 10.0 / 3600.0; // degrees per micron
    mosaic.cdelt2 = 1.0 / 10.0 / 3600.0; // degrees per micron
    mosaic.pc1_1  = 1.0;
    mosaic.pc1_2  = 0.0;
    mosaic.pc2_1  = 0.0;
    mosaic.pc2_2  = 1.0;
    mosaic.Npolyterms = 0;
    for (int i = 0; i < 7; i++) {
        mosaic.polyterms[i][0] = 0.0;
        mosaic.polyterms[i][1] = 0.0;
    }
    RegisterMosaic (&mosaic);

    psMetadata *headerChp = WriteCoordsToHeader (&coords);
    psMetadata *headerMos = WriteCoordsToHeader (&mosaic);

    pmFPA *fpa = pmFPAAlloc (NULL, NULL);
    pmChip *chip = pmChipAlloc (fpa, NULL);

    bool status = pmAstromReadBilevelChip (chip, headerChp);
    ok (status, "read bilevel chip header");
    skip_start (!status, 1, "*** WCS Conversion FAILS *** : skipping related tests");

    bool status = pmAstromReadBilevelMosaic (fpa, headerMos);
    ok (status, "read bilevel fpa header");
    skip_start (!status, 1, "*** WCS Conversion FAILS *** : skipping related tests");

    psPlane  *onChip = psPlaneAlloc();
    psPlane  *onFPA  = psPlaneAlloc();
    psPlane  *onTPA  = psPlaneAlloc();
    psSphere *onSky  = psSphereAlloc();

    double rDVO, dDVO;
    for (double x = -2000; x <= +2000; x+= 500.0) {
        for (double y = -2000; y <= +2000; y+= 500.0) {
            XY_to_RD (&rDVO, &dDVO, x, y, &coords);

            onChip->x = x;
            onChip->y = y;

            psPlaneTransformApply (onFPA, chip->toFPA, onChip);
            psPlaneTransformApply (onTPA, fpa->toTPA, onFPA);
            psDeproject (onSky, onTPA, fpa->toSky);

            while (onSky->r > 2*M_PI)
                onSky->r -= 2*M_PI;
            while (onSky->r <      0)
                onSky->r += 2*M_PI;

            // fprintf (stderr, "fpa x: %f vs %f : %f\n", rDVO, onFPA->x, rDVO - onFPA->x);
            // fprintf (stderr, "fpa y: %f vs %f : %f\n", dDVO, onFPA->y, dDVO - onFPA->y);

            ok_float(onSky->r*PM_DEG_RAD, rDVO, "coordinate match: %f vs %f (delta = %f)", onSky->r*PM_DEG_RAD, rDVO, 3600.0*(onSky->r*PM_DEG_RAD - rDVO));
            ok_float(onSky->d*PM_DEG_RAD, dDVO, "coordinate match: %f vs %f (delta = %f)", onSky->d*PM_DEG_RAD, dDVO, 3600.0*(onSky->d*PM_DEG_RAD - dDVO));
        }
    }
    psFree (onSky);
    psFree (onTPA);
    psFree (onFPA);
    psFree (onChip);

    skip_end();
    skip_end();
    psFree (fpa);
    psFree (chip);

    psFree (headerMos);
    psFree (headerChp);

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

