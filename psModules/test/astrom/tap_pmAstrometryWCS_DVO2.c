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
void test3inv(); // 2nd order term with central offset

int main (void)
{
    plan_tests(1483);

    note("pmAstromReadWCS tests compared with DVO coords routines");

    test1();
    test2();
    test3();
    test1x();
    test2x();
    test3x();
    test3inv();

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
    pmFPA *fpa = pmFPAAlloc (NULL, NULL);
    pmChip *chip = pmChipAlloc (fpa, NULL);

    bool status = pmAstromReadWCS (fpa, chip, header, PM_RAD_DEG*10.0/3600.0);
    ok (status, "converted WCS keywords to WCS astrometry");
    skip_start (!status, 1, "*** WCS Conversion FAILS *** : skipping related tests");

    psPlane *onChip = psPlaneAlloc();
    psPlane *onFPA = psPlaneAlloc();
    psPlane *onTPA = psPlaneAlloc();
    psSphere *onSky = psSphereAlloc();

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

            ok_float(rDVO, onSky->r*PS_DEG_RAD, "coordinate match: %f vs %f (delta = %f)", rDVO, onSky->r*PS_DEG_RAD, rDVO - onSky->r*PS_DEG_RAD);
            ok_float(dDVO, onSky->d*PS_DEG_RAD, "coordinate match: %f vs %f (delta = %f)", dDVO, onSky->d*PS_DEG_RAD, dDVO - onSky->d*PS_DEG_RAD);
        }
    }
    psFree (onSky);
    psFree (onTPA);
    psFree (onFPA);
    psFree (onChip);

    skip_end();
    psFree (fpa);
    psFree (chip);
    psFree (header);

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
    coords.cdelt1 = 1.0/3600;
    coords.cdelt2 = 1.0/3600;
    coords.pc1_1  = 0.9;
    coords.pc1_2  = 0.1;
    coords.pc2_1  =-0.1;
    coords.pc2_2  = 0.9;
    coords.Npolyterms = 0;
    for (int i = 0; i < 7; i++) {
        coords.polyterms[i][0] = 0.0;
        coords.polyterms[i][1] = 0.0;
    }

    psMetadata *header = WriteCoordsToHeader (&coords);
    pmFPA *fpa = pmFPAAlloc (NULL, NULL);
    pmChip *chip = pmChipAlloc (fpa, NULL);

    bool status = pmAstromReadWCS (fpa, chip, header, PM_RAD_DEG*10.0/3600.0);
    ok (status, "converted WCS keywords to WCS astrometry");
    skip_start (!status, 1, "*** WCS Conversion FAILS *** : skipping related tests");

    psPlane *onChip = psPlaneAlloc();
    psPlane *onFPA = psPlaneAlloc();
    psPlane *onTPA = psPlaneAlloc();
    psSphere *onSky = psSphereAlloc();

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

            ok_float(rDVO, onSky->r*PS_DEG_RAD, "coordinate match: %f vs %f (delta = %f)", rDVO, onSky->r*PS_DEG_RAD, rDVO - onSky->r*PS_DEG_RAD);
            ok_float(dDVO, onSky->d*PS_DEG_RAD, "coordinate match: %f vs %f (delta = %f)", dDVO, onSky->d*PS_DEG_RAD, dDVO - onSky->d*PS_DEG_RAD);
        }
    }
    psFree (onSky);
    psFree (onTPA);
    psFree (onFPA);
    psFree (onChip);

    skip_end();
    psFree (fpa);
    psFree (chip);
    psFree (header);

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
    pmFPA *fpa = pmFPAAlloc (NULL, NULL);
    pmChip *chip = pmChipAlloc (fpa, NULL);

    bool status = pmAstromReadWCS (fpa, chip, header, PM_RAD_DEG*10.0/3600.0);
    ok (status, "converted WCS keywords to WCS astrometry");
    skip_start (!status, 1, "*** WCS Conversion FAILS *** : skipping related tests");

    psPlane *onChip = psPlaneAlloc();
    psPlane *onFPA = psPlaneAlloc();
    psPlane *onTPA = psPlaneAlloc();
    psSphere *onSky = psSphereAlloc();

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

            ok_float(rDVO, onSky->r*PS_DEG_RAD, "coordinate match: %f vs %f (delta = %f)", rDVO, onSky->r*PS_DEG_RAD, rDVO - onSky->r*PS_DEG_RAD);
            ok_float(dDVO, onSky->d*PS_DEG_RAD, "coordinate match: %f vs %f (delta = %f)", dDVO, onSky->d*PS_DEG_RAD, dDVO - onSky->d*PS_DEG_RAD);
        }
    }
    psFree (onSky);
    psFree (onTPA);
    psFree (onFPA);
    psFree (onChip);

    skip_end();
    psFree (fpa);
    psFree (chip);
    psFree (header);

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
    coords.crpix1 = +50.0;
    coords.crpix2 = -20.0;
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
    pmFPA *fpa = pmFPAAlloc (NULL, NULL);
    pmChip *chip = pmChipAlloc (fpa, NULL);

    bool status = pmAstromReadWCS (fpa, chip, header, PM_RAD_DEG*10.0/3600.0);
    ok (status, "converted WCS keywords to WCS astrometry");
    skip_start (!status, 1, "*** WCS Conversion FAILS *** : skipping related tests");

    psPlane *onChip = psPlaneAlloc();
    psPlane *onFPA = psPlaneAlloc();
    psPlane *onTPA = psPlaneAlloc();
    psSphere *onSky = psSphereAlloc();

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

            ok_float(rDVO, onSky->r*PS_DEG_RAD, "coordinate match: %f vs %f (delta = %f)", rDVO, onSky->r*PS_DEG_RAD, rDVO - onSky->r*PS_DEG_RAD);
            ok_float(dDVO, onSky->d*PS_DEG_RAD, "coordinate match: %f vs %f (delta = %f)", dDVO, onSky->d*PS_DEG_RAD, dDVO - onSky->d*PS_DEG_RAD);
        }
    }
    psFree (onSky);
    psFree (onTPA);
    psFree (onFPA);
    psFree (onChip);

    skip_end();
    psFree (fpa);
    psFree (chip);
    psFree (header);

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
    coords.crpix1 = +50.0;
    coords.crpix2 = -20.0;
    coords.cdelt1 = 1.0/3600;
    coords.cdelt2 = 1.0/3600;
    coords.pc1_1  = 0.9;
    coords.pc1_2  = 0.1;
    coords.pc2_1  =-0.1;
    coords.pc2_2  = 0.9;
    coords.Npolyterms = 0;
    for (int i = 0; i < 7; i++) {
        coords.polyterms[i][0] = 0.0;
        coords.polyterms[i][1] = 0.0;
    }

    psMetadata *header = WriteCoordsToHeader (&coords);
    pmFPA *fpa = pmFPAAlloc (NULL, NULL);
    pmChip *chip = pmChipAlloc (fpa, NULL);

    bool status = pmAstromReadWCS (fpa, chip, header, PM_RAD_DEG*10.0/3600.0);
    ok (status, "converted WCS keywords to WCS astrometry");
    skip_start (!status, 1, "*** WCS Conversion FAILS *** : skipping related tests");

    psPlane *onChip = psPlaneAlloc();
    psPlane *onFPA = psPlaneAlloc();
    psPlane *onTPA = psPlaneAlloc();
    psSphere *onSky = psSphereAlloc();

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

            ok_float(rDVO, onSky->r*PS_DEG_RAD, "coordinate match: %f vs %f (delta = %f)", rDVO, onSky->r*PS_DEG_RAD, rDVO - onSky->r*PS_DEG_RAD);
            ok_float(dDVO, onSky->d*PS_DEG_RAD, "coordinate match: %f vs %f (delta = %f)", dDVO, onSky->d*PS_DEG_RAD, dDVO - onSky->d*PS_DEG_RAD);
        }
    }
    psFree (onSky);
    psFree (onTPA);
    psFree (onFPA);
    psFree (onChip);

    skip_end();
    psFree (fpa);
    psFree (chip);
    psFree (header);

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
    coords.crpix1 = +50.0;
    coords.crpix2 = -20.0;
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
    pmFPA *fpa = pmFPAAlloc (NULL, NULL);
    pmChip *chip = pmChipAlloc (fpa, NULL);

    bool status = pmAstromReadWCS (fpa, chip, header, PM_RAD_DEG*10.0/3600.0);
    ok (status, "converted WCS keywords to WCS astrometry");
    skip_start (!status, 1, "*** WCS Conversion FAILS *** : skipping related tests");

    psPlane *onChip = psPlaneAlloc();
    psPlane *onFPA = psPlaneAlloc();
    psPlane *onTPA = psPlaneAlloc();
    psSphere *onSky = psSphereAlloc();

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

            ok_float(rDVO, onSky->r*PS_DEG_RAD, "coordinate match: %f vs %f (delta = %f)", rDVO, onSky->r*PS_DEG_RAD, rDVO - onSky->r*PS_DEG_RAD);
            ok_float(dDVO, onSky->d*PS_DEG_RAD, "coordinate match: %f vs %f (delta = %f)", dDVO, onSky->d*PS_DEG_RAD, dDVO - onSky->d*PS_DEG_RAD);
        }
    }
    psFree (onSky);
    psFree (onTPA);
    psFree (onFPA);
    psFree (onChip);

    skip_end();
    psFree (fpa);
    psFree (chip);
    psFree (header);

    ok(!psMemCheckLeaks (id, NULL, stderr, false), "no memory leaks");
}

void test3inv()
{
    note("test the inversion of the non-linear polynomial for toFPA -> fromFPA in pmAstromReadWCS");
    note("note that the tolerance for these tests are rather loose");
    note("a 2nd order polynomial is not a great approximate to 1 over a 2nd order polynomial");
    note("unless the non-linear terms are quite small");
    psMemId id = psMemGetId();

    // build a DVO-style coordinate system
    Coords coords;
    strcpy (coords.ctype, "RA---TAN");
    coords.crval1 = 0.0;
    coords.crval2 = 0.0;
    coords.crpix1 = +50.0;
    coords.crpix2 = -20.0;
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
    pmFPA *fpa = pmFPAAlloc (NULL, NULL);
    pmChip *chip = pmChipAlloc (fpa, NULL);

    bool status = pmAstromReadWCS (fpa, chip, header, PM_RAD_DEG*10.0/3600.0);
    ok (status, "converted WCS keywords to WCS astrometry");
    skip_start (!status, 1, "*** WCS Conversion FAILS *** : skipping related tests");

    psPlane *aChip  = psPlaneAlloc();
    psPlane *aFPA   = psPlaneAlloc();
    psPlane *aTPA   = psPlaneAlloc();
    psPlane *bChip  = psPlaneAlloc();
    psPlane *bFPA   = psPlaneAlloc();
    psPlane *bTPA   = psPlaneAlloc();
    psSphere *onSky = psSphereAlloc();

    for (double x = -2000; x <= +2000; x+= 500.0) {
        for (double y = -2000; y <= +2000; y+= 500.0) {
            // convert up to sky
            aChip->x = x;
            aChip->y = y;

            psPlaneTransformApply (aFPA, chip->toFPA, aChip);
            psPlaneTransformApply (aTPA, fpa->toTPA, aFPA);
            psDeproject (onSky, aTPA, fpa->toSky);

            while (onSky->r > 2*M_PI)
                onSky->r -= 2*M_PI;
            while (onSky->r <      0)
                onSky->r += 2*M_PI;

            psProject (bTPA, onSky, fpa->toSky);
            psPlaneTransformApply (bFPA, fpa->fromTPA, bTPA);
            psPlaneTransformApply (bChip, chip->fromFPA, bFPA);

            // calculate appropriate tol values as f(x,y)
            ok_float_tol(aChip->x, bChip->x, 1.0, "coordinate match: %f vs %f (delta = %f)", aChip->x, bChip->x, aChip->x - bChip->x);
            ok_float_tol(aChip->y, bChip->y, 1.0, "coordinate match: %f vs %f (delta = %f)", aChip->y, bChip->y, aChip->y - bChip->y);

            ok_float(aFPA->x, bFPA->x, "coordinate match: %f vs %f (delta = %f)", aFPA->x, bFPA->x, aFPA->x - bFPA->x);
            ok_float(aFPA->y, bFPA->y, "coordinate match: %f vs %f (delta = %f)", aFPA->y, bFPA->y, aFPA->y - bFPA->y);

            // in this example, TPA coordinates are 10 arcsec/mm; the tol. below represent 1nano-arcsec
            ok_float_tol(aTPA->x, bTPA->x, 1e-10, "coordinate match: %f vs %f (delta = %g)", aTPA->x, bTPA->x, aTPA->x - bTPA->x);
            ok_float_tol(aTPA->y, bTPA->y, 1e-10, "coordinate match: %f vs %f (delta = %g)", aTPA->y, bTPA->y, aTPA->y - bTPA->y);
        }
    }
    psFree (onSky);
    psFree (aTPA);
    psFree (aFPA);
    psFree (aChip);
    psFree (bTPA);
    psFree (bFPA);
    psFree (bChip);

    skip_end();
    psFree (fpa);
    psFree (chip);
    psFree (header);

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

