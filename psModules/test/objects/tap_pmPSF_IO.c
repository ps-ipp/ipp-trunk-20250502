#include <stdio.h>
#include <string.h>
#include <pslib.h>
#include <psmodules.h>
#include "tap.h"
#include "pstap.h"
/* STATUS
    Uder construction.  Only lightly tested so far.
*/

#define TEST_FLOATS_EQUAL(X, Y) (abs(X - Y) < 0.0001)
#define NUM_MODELS		5

#define CHIP_ALLOC_NAME        "ChipName"
#define CELL_ALLOC_NAME        "CellName"
#define MISC_NUM                32
#define MISC_NAME              "META00"
#define MISC_NAME2             "META01"
#define NUM_BIAS_DATA           10
#define TEST_NUM_ROWS           4
#define TEST_NUM_COLS           4
#define NUM_READOUTS            3
#define NUM_CELLS               10
#define NUM_CHIPS               8
#define NUM_HDUS                5
#define BASE_IMAGE              10
#define BASE_MASK               40
#define BASE_WEIGHT             70
#define VERBOSE                 0
#define ERR_TRACE_LEVEL         0

psPlaneTransform *PS_CREATE_2D_IDENTITY_PLANE_TRANSFORM()
{
    psPlaneTransform *pt = psPlaneTransformAlloc(1, 1);
    pt->x->coeff[1][0] = 1.0;
    pt->y->coeff[0][1] = 1.0;
    return(pt);
}

psPlaneDistort *PS_CREATE_4D_IDENTITY_PLANE_DISTORT()
{
    psPlaneDistort *pd = psPlaneDistortAlloc(1, 1, 1, 1);
    pd->x->coeff[1][0][0][0] = 1.0;
    pd->y->coeff[0][1][0][0] = 1.0;
    return(pd);
}

/******************************************************************************
generateSimpleReadout(): This function generates a pmReadout data structure and then
populates its members with real data.
 *****************************************************************************/
pmReadout *generateSimpleReadout(pmCell *cell)
{
    pmReadout *readout = pmReadoutAlloc(cell);
    readout->image = psImageAlloc(TEST_NUM_COLS, TEST_NUM_ROWS, PS_TYPE_F32);
    readout->mask = psImageAlloc(TEST_NUM_COLS, TEST_NUM_ROWS, PS_TYPE_U8);
    readout->variance = psImageAlloc(TEST_NUM_COLS, TEST_NUM_ROWS, PS_TYPE_F32);
    for (psS32 i = 0 ; i < NUM_BIAS_DATA ; i++) {
        psImage *tmpImage = psImageAlloc(TEST_NUM_COLS, TEST_NUM_ROWS, PS_TYPE_F32);
        psImageInit(tmpImage, (double) i);
        psListAdd(readout->bias, PS_LIST_HEAD, tmpImage);
        psFree(tmpImage);
    }
    psMetadataAddS32(readout->analysis, PS_LIST_HEAD, MISC_NAME, 0, NULL, MISC_NUM);
    return(readout);
}

/******************************************************************************
generateSimpleCell(): This function generates a pmCell data structure and then
populates its members with real data.
 *****************************************************************************/
pmCell *generateSimpleCell(pmChip *chip)
{
    pmCell *cell = pmCellAlloc(chip, CELL_ALLOC_NAME);

    psMetadataAddS32(cell->analysis, PS_LIST_HEAD, MISC_NAME, 0, NULL, MISC_NUM);
    psMetadataAddS32(cell->concepts, PS_LIST_HEAD, MISC_NAME, 0, NULL, MISC_NUM);
    psArrayRealloc(cell->readouts, NUM_READOUTS);
    cell->hdu = pmHDUAlloc("cellExtName");
    for (int i = 0 ; i < NUM_READOUTS ; i++) {
        cell->readouts->data[i] = psMemDecrRefCounter((psPtr) generateSimpleReadout(cell));
    }

    // First try to read data from ../dataFiles, then try dataFiles.
    bool rc = pmConfigFileRead(&cell->hdu->format, "../dataFiles/camera0/format0.config", "Camera format 0");
    if (!rc) {
        rc = pmConfigFileRead(&cell->hdu->format, "dataFiles/camera0/format0.config", "Camera format 0");
        if (!rc) {
            diag("pmConfigFileRead() was unsuccessful (from generateSimpleCell())");
	}
    }

    cell->hdu->images = psArrayAlloc(NUM_HDUS);
    cell->hdu->masks = psArrayAlloc(NUM_HDUS);
    cell->hdu->variances = psArrayAlloc(NUM_HDUS);
    for (int k = 0 ; k < NUM_HDUS ; k++) {
        cell->hdu->images->data[k]  = psImageAlloc(TEST_NUM_COLS, TEST_NUM_ROWS, PS_TYPE_F32);
        cell->hdu->masks->data[k]   = psImageAlloc(TEST_NUM_COLS, TEST_NUM_ROWS, PS_TYPE_MASK);
        cell->hdu->variances->data[k] = psImageAlloc(TEST_NUM_COLS, TEST_NUM_ROWS, PS_TYPE_F32);
        psImageInit(cell->hdu->images->data[k], (float) (BASE_IMAGE+k));
        psImageInit(cell->hdu->masks->data[k], (psU8) (BASE_MASK+k));
        psImageInit(cell->hdu->variances->data[k], (float) (BASE_WEIGHT+k));
    }

    //XXX: Should the region be set some other way?  Like through the various config files?
//    psRegion *region = psRegionAlloc(0.0, TEST_NUM_COLS-1, 0.0, TEST_NUM_ROWS-1);
    psRegion *region = psRegionAlloc(0.0, 0.0, 0.0, 0.0);
    // You shouldn't have to remove the key from the metadata.  Find out how to simply change the key value.
    psMetadataRemoveKey(cell->concepts, "CELL.TRIMSEC");
    psMetadataAddPtr(cell->concepts, PS_LIST_TAIL|PS_META_REPLACE, "CELL.TRIMSEC", PS_DATA_REGION, "I am a region", region);
    psFree(region);
    return(cell);
}

/******************************************************************************
generateSimpleChip(): This function generates a pmChip data structure and then
populates its members with real data.
 *****************************************************************************/
pmChip *generateSimpleChip(pmFPA *fpa)
{
    pmChip *chip = pmChipAlloc(fpa, CHIP_ALLOC_NAME);
    chip->toFPA = PS_CREATE_2D_IDENTITY_PLANE_TRANSFORM();
    chip->fromFPA = PS_CREATE_2D_IDENTITY_PLANE_TRANSFORM();
    psMetadataAddS32(chip->analysis, PS_LIST_HEAD, MISC_NAME, 0, NULL, MISC_NUM);
    psMetadataAddS32(chip->concepts, PS_LIST_HEAD, MISC_NAME, 0, NULL, MISC_NUM);
    psArrayRealloc(chip->cells, NUM_CELLS);
    for (int i = 0 ; i < NUM_CELLS ; i++) {
        chip->cells->data[i] = psMemDecrRefCounter((psPtr) generateSimpleCell(chip));
    }

    // XXX: Add code to initialize chip pmConcepts


    return(chip);
}

/******************************************************************************
generateSimpleFPA(): This function generates a pmFPA data structure and then
populates its members with real data.
 *****************************************************************************/
pmFPA* generateSimpleFPA(psMetadata *camera)
{
    pmFPA* fpa = pmFPAAlloc(camera, NULL);
    fpa->hdu = pmHDUAlloc("cellExtName");
    fpa->fromTPA = PS_CREATE_2D_IDENTITY_PLANE_TRANSFORM();
    fpa->toTPA = PS_CREATE_2D_IDENTITY_PLANE_TRANSFORM();
    fpa->toSky = psProjectionAlloc(0.0,0.0,10.0,10.0,PS_PROJ_TAN);
    psMetadataAddS32(fpa->analysis, PS_LIST_HEAD, MISC_NAME, 0, NULL, MISC_NUM);
    if (camera != NULL) {
        psMetadataAddS32((psMetadata *) fpa->camera, PS_LIST_HEAD, MISC_NAME, 0, NULL, MISC_NUM);
    }
    psMetadataAddS32(fpa->concepts, PS_LIST_HEAD, MISC_NAME, 0, NULL, MISC_NUM);
    psArrayRealloc(fpa->chips, NUM_CHIPS);
    for (int i = 0 ; i < NUM_CHIPS ; i++) {
        fpa->chips->data[i] = psMemDecrRefCounter((psPtr) generateSimpleChip(fpa));
    }
    pmConceptsBlankFPA(fpa);
    return(fpa);
}


int main(int argc, char* argv[])
{
    psLogSetFormat("HLNM");
    psLogSetLevel(PS_LOG_INFO);
    psTraceSetLevel("err", ERR_TRACE_LEVEL);
    plan_tests(28);


    // ----------------------------------------------------------------------
    // pmPSFmodelCheckDataStatusForView() tests
    // bool pmPSFmodelCheckDataStatusForView (const pmFPAview *view, const pmFPAfile *file)
    // Call pmPSFmodelCheckDataStatusForView() with NULL pmFPAview input parameter
    if (1) {
        psMemId id = psMemGetId();

        pmFPAview *view = pmFPAviewAlloc(32);
        pmFPAfile *file = pmFPAfileAlloc();
        bool rc = pmPSFmodelCheckDataStatusForView(NULL, file);
        ok(rc == false, "pmPSFmodelCheckDataStatusForView() returned FALSE with NULL pmFPAview input parameter");
        psFree(view);
        psFree(file);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Call pmPSFmodelCheckDataStatusForView() with NULL pmFPAfile input parameter
    if (1) {
        psMemId id = psMemGetId();

        pmFPAview *view = pmFPAviewAlloc(32);
        pmFPAfile *file = pmFPAfileAlloc();
        bool rc = pmPSFmodelCheckDataStatusForView(view, NULL);
        ok(rc == false, "pmPSFmodelCheckDataStatusForView() returned FALSE with NULL pmFPAfile input parameter");
        psFree(view);
        psFree(file);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Call pmPSFmodelCheckDataStatusForView() with acceptable input parameters
    if (1) {
        psMemId id = psMemGetId();

        pmFPAview *view = pmFPAviewAlloc(32);
        view->chip = -1;
        view->cell = -1;
        pmFPAfile *file = pmFPAfileAlloc();
        psMetadata *camera = psMetadataAlloc();
        file->fpa = generateSimpleFPA(camera);
        bool rc = pmPSFmodelCheckDataStatusForView(view, file);
        ok(rc == false, "pmPSFmodelCheckDataStatusForView() returned FALSE acceptable input parameters, but no PSPHOT.PSF");

        // Add PSPHOT.PSF to chip->analysis and call pmPSFmodelCheckDataStatusForChip()
        pmChip *chip = file->fpa->chips->data[0];
        psVector *junk = psVectorAlloc(10, PS_TYPE_F32);
        bool rc2 = psMetadataAddPtr(chip->analysis, PS_LIST_HEAD, "PSPHOT.PSF", PS_DATA_VECTOR, NULL, junk);
        ok(rc2 == true, "added PSPHOT.PSF metadata to chip->analysis");
        rc = pmPSFmodelCheckDataStatusForView(view, file);
        ok(rc == true, "pmPSFmodelCheckDataStatusForView() returned TRUE acceptable input parameters");

        psFree(view);
        psFree(file->fpa);
        file->fpa = NULL;
        psFree(file);
        psFree(camera);
        psFree(junk);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // ----------------------------------------------------------------------
    // pmPSFmodelCheckDataStatusForFPA() tests
    // bool pmPSFmodelCheckDataStatusForFPA (const pmFPA *fpa)
    // Call pmPSFmodelCheckDataStatusForFPA() with NULL pmFPA input parameter
    if (1) {
        psMemId id = psMemGetId();
        psMetadata *camera = psMetadataAlloc();
        pmFPA *fpa = generateSimpleFPA(camera);
        bool rc = pmPSFmodelCheckDataStatusForFPA(NULL);
        ok(rc == false, "pmPSFmodelCheckDataStatusForFPA() returned FALSE with NULL pmFPA input parameter");
        psFree(fpa);
        psFree(camera);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Call pmPSFmodelCheckDataStatusForFPA() with NULL pmFPA->chips input parameter
    if (1) {
        psMemId id = psMemGetId();
        psMetadata *camera = psMetadataAlloc();
        pmFPA *fpa = generateSimpleFPA(camera);
        for (int i = 0 ; i < fpa->chips->n ; i++) {
            psFree(fpa->chips->data[i]);
	}
        bool rc = pmPSFmodelCheckDataStatusForFPA(NULL);
        ok(rc == false, "pmPSFmodelCheckDataStatusForFPA() returned FALSE with NULL pmFPA->chips input parameter");
        psFree(fpa);
        psFree(camera);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Call pmPSFmodelCheckDataStatusForFPA() with acceptable input parameters
    if (1) {
        psMemId id = psMemGetId();
        psMetadata *camera = psMetadataAlloc();
        pmFPA *fpa = generateSimpleFPA(camera);
        bool rc = pmPSFmodelCheckDataStatusForFPA(fpa);
        ok(rc == false, "pmPSFmodelCheckDataStatusForFPA() returned FALSE with acceptable input parameters, but no PSPHOT.PSF");

        // Add PSPHOT.PSF to chip->analysis and call pmPSFmodelCheckDataStatusForChip()
        psVector *junk = psVectorAlloc(10, PS_TYPE_F32);
        pmChip *chip = fpa->chips->data[0];
        bool rc2 = psMetadataAddPtr(chip->analysis, PS_LIST_HEAD, "PSPHOT.PSF", PS_DATA_VECTOR, NULL, junk);
        ok(rc2 == true, "added PSPHOT.PSF metadata to chip->analysis");
        rc = pmPSFmodelCheckDataStatusForFPA(fpa);
        ok(rc == true, "pmPSFmodelCheckDataStatusForFPA() returned TRUE with acceptable input parameters");

        psFree(fpa);
        psFree(camera);
        psFree(junk);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // ----------------------------------------------------------------------
    // pmPSFmodelCheckDataStatusForChip() tests
    // bool pmPSFmodelCheckDataStatusForChip (const pmChip *chip)
    // Call pmPSFmodelCheckDataStatusForChip() with NULL pmChip input parameter
    if (1) {
        psMemId id = psMemGetId();
        bool rc = pmPSFmodelCheckDataStatusForChip(NULL);
        ok(rc == false, "pmPSFmodelCheckDataStatusForChip() returned FALSE with NULL pmChip input parameter");
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Call pmPSFmodelCheckDataStatusForChip() with acceptable input parameters
    if (1) {
        psMemId id = psMemGetId();
        psMetadata *camera = psMetadataAlloc();
        pmFPA *fpa = generateSimpleFPA(camera);
        pmChip *chip = fpa->chips->data[0];
        bool rc = pmPSFmodelCheckDataStatusForChip(chip);
        ok(rc == false, "pmPSFmodelCheckDataStatusForChip() returned false with acceptable pmChip, but no PSPHOT.PSF");

        // Add PSPHOT.PSF to chip->analysis and call pmPSFmodelCheckDataStatusForChip()
        psVector *junk = psVectorAlloc(10, PS_TYPE_F32);
        bool rc2 = psMetadataAddPtr(chip->analysis, PS_LIST_HEAD, "PSPHOT.PSF", PS_DATA_VECTOR, NULL, junk);
        ok(rc2 == true, "added PSPHOT.PSF metadata to chip->analysis");
        rc = pmPSFmodelCheckDataStatusForChip(chip);
        ok(rc == true, "pmPSFmodelCheckDataStatusForChip() returned TRUE with acceptable input parameters");
    
        psFree(fpa);
        psFree(camera);
        psFree(junk);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // ----------------------------------------------------------------------
    // pmPSFmodelWrite() tests
    // bool pmPSFmodelWrite (psMetadata *analysis, const pmFPAview *view,
    //                       pmFPAfile *file, const pmConfig *config)
    // Call pmPSFmodelWrite() with NULL pmFPAview input parameter
    if (1) {
        psMemId id = psMemGetId();
        pmFPAview *view = pmFPAviewAlloc(32);
        pmFPAfile *file = pmFPAfileAlloc();
        psMetadata *camera = psMetadataAlloc();
        file->fpa = generateSimpleFPA(camera);
        psMetadata *analysis = psMetadataAlloc();
        pmConfig *config = pmConfigAlloc();

        bool rc = pmPSFmodelWrite(analysis, NULL, file, config);
        ok(rc == false, "pmPSFmodelWrite() returned FALSE with NULL pmFPAview input parameter");

        psFree(view);
        psFree(file->fpa);
        file->fpa = NULL;
        psFree(file);
        psFree(camera);
        psFree(analysis);
        psFree(config);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Call pmPSFmodelWrite() with NULL pmFPAfile input parameter
    if (1) {
        psMemId id = psMemGetId();
        pmFPAview *view = pmFPAviewAlloc(32);
        pmFPAfile *file = pmFPAfileAlloc();
        psMetadata *camera = psMetadataAlloc();
        file->fpa = generateSimpleFPA(camera);
        psMetadata *analysis = psMetadataAlloc();
        pmConfig *config = pmConfigAlloc();

        bool rc = pmPSFmodelWrite(analysis, view, NULL, config);
        ok(rc == false, "pmPSFmodelWrite() returned FALSE with NULL pmFPAfile input parameter");

        psFree(view);
        psFree(file->fpa);
        file->fpa = NULL;
        psFree(file);
        psFree(camera);
        psFree(analysis);
        psFree(config);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Call pmPSFmodelWrite() with NULL pmFPAfile->file input parameter
    if (1) {
        psMemId id = psMemGetId();
        pmFPAview *view = pmFPAviewAlloc(32);
        pmFPAfile *file = pmFPAfileAlloc();
        psMetadata *analysis = psMetadataAlloc();
        pmConfig *config = pmConfigAlloc();

        bool rc = pmPSFmodelWrite(analysis, view, NULL, config);
        ok(rc == false, "pmPSFmodelWrite() returned FALSE with NULL pmFPAfile->fpa input parameter");

        psFree(view);
        psFree(file);
        psFree(analysis);
        psFree(config);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Call pmPSFmodelWrite() with acceptable input parameters
    // XXX: This is currently being coded.  It does not work.
    if (0) {
        psMemId id = psMemGetId();
        pmFPAview *view = pmFPAviewAlloc(32);
        pmFPAfile *file = pmFPAfileAlloc();
        psMetadata *camera = psMetadataAlloc();
        file->fpa = generateSimpleFPA(camera);
        pmConfigFileRead(&file->camera, "../dataFiles/camera0/camera.config", "CAMERA");
        psMetadataPrint(stdout, file->camera, 0);
        psMetadata *menu = psMetadataLookupMetadata(NULL, file->camera, "EXTNAME.RULES");
        if (!menu) {
            printf("NOTE: missing EXTNAME.RULES in camera.config\n");
            exit(1);
        }


        psMetadata *analysis = psMetadataAlloc();
        pmConfig *config = pmConfigAlloc();

        // XXX: I failed here and then moved on.
        /*
        The function reads the PSPHOT recipes and requires that it contains a
	PSPHOT key in the metadata.  I'm not sure how to do that, but I could
	have hacked around it.  Then, however, it required several keys in
	that metadata like PSF.CLUMP.X, PSF.CLUMP.Y, PSF.CLUMP.DX,
	PSF.CLUMP.DY.  I could also put them in there, but I just gave up.
	What should I do here?  I could easily create a metadata file with all
	those metadata keys, but I'd rather use an existing one.
        */

        if (1) {
            psMetadata *junk = psMetadataAlloc();
            bool rc0 = pmConfigFileRead(&junk, "../dataFiles/recipes/psphot.config", "SAVE.PSF");
            if (!rc0) {
                rc0 = pmConfigFileRead(&junk, "dataFiles/recipes/psphot.config", "SAVE.PSF");
	    }
            ok(rc0, "Successfully read the PSPHOT recipe file");
//          psMetadata *recipe = psMetadataLookupPtr(&rc0, junk, "SAVE.OUTPUT");
            bool rc2 = psMetadataLookupBool(&rc0, junk, "SAVE.OUTPUT");
            printf("rc0 is %d\n", (int) rc0);
            printf("rc2 is %d\n", (int) rc2);
//          if (recipe == NULL) printf("ERROR: recipe is NULL\n");
            psMetadataPrint(stdout, junk, 0);
            psFree(junk);
	}
        if (config->recipes == NULL) printf("COOL: config->recipes is NULL");
        bool rc0 = pmConfigFileRead(&config->recipes, "../dataFiles/camera0/recipes.config", "PSPHOT");
        if (!rc0) {
            rc0 = pmConfigFileRead(&config->recipes, "dataFiles/camera0/recipes.config", "PSPHOT");
	}
        ok(rc0, "Successfully read the PSPHOT recipe file");
psMetadataPrint(stdout, config->recipes, 0);

        if (config->recipes == NULL) printf("FUCK: config->recipes is NULL");
        psMetadata *recipe = psMetadataLookupPtr(NULL, config->recipes, "PSPHOT");
        if (!recipe) {
            printf("FUCK: missing recipe %s\n", "PSPHOT");
            exit(1);
        }


        bool rc = pmPSFmodelWrite(analysis, view, file, config);
        ok(rc == true, "pmPSFmodelWrite() returned TRUE with acceptable input parameters");

        psFree(view);
        psFree(file->fpa);
        file->fpa = NULL;
        psFree(file);
        psFree(camera);
        psFree(analysis);
        psFree(config);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }
}
