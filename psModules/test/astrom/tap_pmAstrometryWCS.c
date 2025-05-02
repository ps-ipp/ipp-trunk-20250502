#include <stdio.h>
#include <string.h>
#include <pslib.h>
#include <psmodules.h>

#include "tap.h"
#include "pstap.h"

int main (void)
{
    plan_tests(33);

    note("pmAstrometryWCS tests");

    {
        note("test pmAstromReadWCS");
        psMemId id = psMemGetId();

        // construct a header with a simple set of WCS values
        // convert to pmFPA components and check

        psMetadata *header = psMetadataAlloc();

        psMetadataAddStr (header, PS_LIST_TAIL, "CTYPE1", PS_META_REPLACE, "", "RA---TAN");
        psMetadataAddStr (header, PS_LIST_TAIL, "CTYPE2", PS_META_REPLACE, "", "DEC--TAN");

        // center coords (R,D)
        psMetadataAddF32 (header, PS_LIST_TAIL, "CRVAL1", PS_META_REPLACE, "", 0.0);
        psMetadataAddF32 (header, PS_LIST_TAIL, "CRVAL2", PS_META_REPLACE, "", 0.0);

        // center coords (X,Y)
        psMetadataAddF32 (header, PS_LIST_TAIL, "CRPIX1", PS_META_REPLACE, "", 0.0);
        psMetadataAddF32 (header, PS_LIST_TAIL, "CRPIX2", PS_META_REPLACE, "", 0.0);

        // degrees per pixel
        psMetadataAddF32 (header, PS_LIST_TAIL, "CDELT1",  PS_META_REPLACE, "", 1.0/3600.0);
        psMetadataAddF32 (header, PS_LIST_TAIL, "CDELT2",  PS_META_REPLACE, "", 1.0/3600.0);

        // rotation matrix
        psMetadataAddF32 (header, PS_LIST_TAIL, "PC001001", PS_META_REPLACE, "", 1.0);
        psMetadataAddF32 (header, PS_LIST_TAIL, "PC001002", PS_META_REPLACE, "", 0.0);
        psMetadataAddF32 (header, PS_LIST_TAIL, "PC002001", PS_META_REPLACE, "", 0.0);
        psMetadataAddF32 (header, PS_LIST_TAIL, "PC002002", PS_META_REPLACE, "", 1.0);

        pmFPA *fpa = pmFPAAlloc (NULL, NULL);
        pmChip *chip = pmChipAlloc (NULL, "test");

        // toFPA carries pixel scale (microns per pixel)
        // toSky carries plate scale (radians per micron)
        bool status = pmAstromReadWCS (fpa, chip, header, 10.0);
        ok (status, "converted WCS keywords to FPA astrometry");
        skip_start (!status, 1, "*** WCS Conversion FAILS *** : skipping related tests");

        ok(fpa->toSky->type == PS_PROJ_TAN, "correct projection (TAN)");

        // make these tests double
        ok_float(fpa->toSky->R*PS_DEG_RAD, 0.0, "projection center RA %f", fpa->toSky->R*PS_DEG_RAD);
        ok_float(fpa->toSky->R*PS_DEG_RAD, 0.0, "projection center DEC %f", fpa->toSky->R*PS_DEG_RAD);

        ok_float(fpa->toSky->Xs, PM_RAD_DEG/3600.0/10.0, "projection X scale %f", fpa->toSky->Xs);
        ok_float(fpa->toSky->Ys, PM_RAD_DEG/3600.0/10.0, "projection X scale %f", fpa->toSky->Ys);

        ok_float(fpa->toTPA->x->coeff[1][0], 1.0, "TP scale (unity): %f", fpa->toTPA->x->coeff[1][0]);
        ok_float(fpa->toTPA->y->coeff[0][1], 1.0, "TP scale (unity): %f", fpa->toTPA->x->coeff[1][0]);

        ok_float(chip->toFPA->x->coeff[0][0], 0.0, "ref pixel X: %f", chip->toFPA->x->coeff[0][0]);
        ok_float(chip->toFPA->y->coeff[0][0], 0.0, "ref pixel Y: %f", chip->toFPA->y->coeff[0][0]);

        ok_float(chip->toFPA->x->coeff[1][0], 10.0, "CD1_1: %f", chip->toFPA->x->coeff[1][0]);
        ok_float(chip->toFPA->x->coeff[0][1], 0.0, "CD1_2: %f", chip->toFPA->x->coeff[0][1]);
        ok_float(chip->toFPA->y->coeff[1][0], 0.0, "CD2_1: %f", chip->toFPA->y->coeff[1][0]);
        ok_float(chip->toFPA->y->coeff[0][1], 10.0, "CD2_2: %f", chip->toFPA->y->coeff[0][1]);

        psFree (fpa);
        psFree (chip);
        psFree (header);

        skip_end();
        ok(!psMemCheckLeaks (id, NULL, stderr, false), "no memory leaks");
    }

    {
        note("test pmAstromReadWCS");
        psMemId id = psMemGetId();

        // construct a header with a simple set of WCS values
        // convert to pmFPA components and check

        psMetadata *header = psMetadataAlloc();

        psMetadataAddStr (header, PS_LIST_TAIL, "CTYPE1", PS_META_REPLACE, "", "RA---TAN");
        psMetadataAddStr (header, PS_LIST_TAIL, "CTYPE2", PS_META_REPLACE, "", "DEC--TAN");

        // center coords (R,D)
        psMetadataAddF32 (header, PS_LIST_TAIL, "CRVAL1", PS_META_REPLACE, "", 0.0);
        psMetadataAddF32 (header, PS_LIST_TAIL, "CRVAL2", PS_META_REPLACE, "", 0.0);

        // center coords (X,Y)
        psMetadataAddF32 (header, PS_LIST_TAIL, "CRPIX1", PS_META_REPLACE, "", 10.0);
        psMetadataAddF32 (header, PS_LIST_TAIL, "CRPIX2", PS_META_REPLACE, "", 10.0);

        // degrees per pixel
        psMetadataAddF32 (header, PS_LIST_TAIL, "CDELT1",  PS_META_REPLACE, "", 1.0/3600.0);
        psMetadataAddF32 (header, PS_LIST_TAIL, "CDELT2",  PS_META_REPLACE, "", 1.0/3600.0);

        // rotation matrix
        psMetadataAddF32 (header, PS_LIST_TAIL, "PC001001", PS_META_REPLACE, "", 1.0);
        psMetadataAddF32 (header, PS_LIST_TAIL, "PC001002", PS_META_REPLACE, "", 0.0);
        psMetadataAddF32 (header, PS_LIST_TAIL, "PC002001", PS_META_REPLACE, "", 0.0);
        psMetadataAddF32 (header, PS_LIST_TAIL, "PC002002", PS_META_REPLACE, "", 1.0);

        pmFPA *fpa = pmFPAAlloc (NULL, NULL);
        pmChip *chip = pmChipAlloc (NULL, "test");

        // toFPA carries pixel scale (pixels per micron)
        // toTPA carries plate scale (microns per arcsecond)
        bool status = pmAstromReadWCS (fpa, chip, header, 10.0);
        ok (status, "converted WCS keywords to FPA astrometry");
        skip_start (!status, 1, "*** WCS Conversion FAILS *** : skipping related tests");

        ok(fpa->toSky->type == PS_PROJ_TAN, "correct projection (TAN)");

        // make these tests double
        ok_float(fpa->toSky->R*PS_DEG_RAD, 0.0, "projection center RA %f", fpa->toSky->R*PS_DEG_RAD);
        ok_float(fpa->toSky->R*PS_DEG_RAD, 0.0, "projection center DEC %f", fpa->toSky->R*PS_DEG_RAD);

        ok_float(fpa->toSky->Xs, PM_RAD_DEG/3600.0/10.0, "projection X scale %f", fpa->toSky->Xs);
        ok_float(fpa->toSky->Ys, PM_RAD_DEG/3600.0/10.0, "projection X scale %f", fpa->toSky->Ys);

        ok_float(fpa->toTPA->x->coeff[1][0], 1.0, "TP scale (unity): %f", fpa->toTPA->x->coeff[1][0]);
        ok_float(fpa->toTPA->y->coeff[0][1], 1.0, "TP scale (unity): %f", fpa->toTPA->x->coeff[1][0]);

        ok_float(chip->toFPA->x->coeff[0][0], -100.0, "ref pixel X: %f", chip->toFPA->x->coeff[0][0]);
        ok_float(chip->toFPA->y->coeff[0][0], -100.0, "ref pixel Y: %f", chip->toFPA->y->coeff[0][0]);

        ok_float(chip->toFPA->x->coeff[1][0], 10.0, "CD1_1: %f", chip->toFPA->x->coeff[1][0]);
        ok_float(chip->toFPA->x->coeff[0][1], 0.0, "CD1_2: %f", chip->toFPA->x->coeff[0][1]);
        ok_float(chip->toFPA->y->coeff[1][0], 0.0, "CD2_1: %f", chip->toFPA->y->coeff[1][0]);
        ok_float(chip->toFPA->y->coeff[0][1], 10.0, "CD2_2: %f", chip->toFPA->y->coeff[0][1]);

        psFree (fpa);
        psFree (chip);
        psFree (header);

        skip_end();
        ok(!psMemCheckLeaks (id, NULL, stderr, false), "no memory leaks");
    }

    return exit_status();
}

