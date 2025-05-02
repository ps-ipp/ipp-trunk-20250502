    /** @file tst_pmConcepts.c
 *
 *  @brief Contains the tests for pmConcepts.c:
 *
 *  @version $Revision: 1.3 $ $Name: not supported by cvs2svn $
 *  @date $Date: 2009-01-29 20:45:54 $
*   pmConceptSpecAlloc()
    pmConceptsList()
*   pmConceptGetRequired()
*   pmConceptSetRequired()
    pmConceptRegister()
    pmConceptsRead()
*   pmConceptsBlankFPA()
    pmConceptsReadFPA()
    pmConceptsWriteFPA()
*   pmConceptsBlankChip()
    pmConceptsReadChip()
    pmConceptsWriteChip()
*   pmConceptsBlankCell()
    pmConceptsReadCell()
    pmConceptsWriteCell()
*   pmConceptsInit()
*   pmConceptsDone()
    pmFPACopyConcepts()
 *
 *  Copyright 2004 Maui High Performance Computing Center, University of Hawaii
 */
#include <stdio.h>
#include <string.h>
#include <pslib.h>
#include <psmodules.h>
#include "tap.h"
#include "pstap.h"

#define VERBOSE                 0
#define ERR_TRACE_LEVEL         0

psMetadataItem *dummyConceptParser(
    const psMetadataItem *concept,
    const psMetadataItem *pattern,
    pmConceptSource source,
    const psMetadata *cameraFormat,
    const pmFPA *fpa,
    const pmChip *chip,
    const pmCell *cell)
{
    if (concept == NULL ||
        pattern == NULL ||
        source == PM_CONCEPT_SOURCE_NONE ||
        cameraFormat == NULL ||
        fpa == NULL ||
        chip == NULL ||
        cell == NULL) {
        printf("dummyConceptParser() args are NULL\n");
    }
    return(NULL);
}

// FPA.RA and FPA.DEC
psMetadataItem *dummyConceptFormatter(
    const psMetadataItem *concept,
    pmConceptSource source,
    const psMetadata *cameraFormat,
    const pmFPA *fpa,
    const pmChip *chip,
    const pmCell *cell)
{
    if (concept == NULL ||
        source == PM_CONCEPT_SOURCE_NONE ||
        cameraFormat == NULL ||
        fpa == NULL ||
        chip == NULL ||
        cell == NULL) {
        printf("dummyConceptFormatter() args are NULL\n");
    }
    return(NULL);
}

psMetadataItem *dummyConceptCopier(
    const psMetadataItem *source,
    const psMetadataItem *target,
    const psMetadata *cameraFormat,
    const pmFPA *fpa,
    const pmChip *chip,
    const pmCell *cell)
{
    if (target == NULL ||
        source == PM_CONCEPT_SOURCE_NONE ||
        cameraFormat == NULL ||
        fpa == NULL ||
        chip == NULL ||
        cell == NULL) {
        printf("dummyConceptCopier() args are NULL\n");
    }
    return(NULL);
}

int main(int argc, char* argv[])
{
    psLogSetFormat("HLNM");
    psLogSetLevel(PS_LOG_INFO);
    psTraceSetLevel(".", ERR_TRACE_LEVEL);
    plan_tests(126);
    
    // --------------------------------------------------------------------
    // Tests for pmConceptSpecAlloc()
    // Acceptable input parameters.
    {
        psMemId id = psMemGetId();
        psMetadataItem *blank = psMetadataItemAlloc("myItem1", PS_DATA_BOOL, "I am a boolean", true);
        pmConceptSpec *tmp = pmConceptSpecAlloc(blank, dummyConceptParser,
            dummyConceptFormatter, dummyConceptCopier, true);
        ok(tmp != NULL, "pmConceptSpecAlloc() returned non-NULL");
        skip_start(tmp == NULL, 4, "Skipping tests because pmConceptSpecAlloc() returned NULL");
        ok(tmp->blank == blank, "pmConceptSpecAlloc() set the ->blank member correctly");
        ok(tmp->parse == dummyConceptParser, "pmConceptSpecAlloc() set the ->parse member correctly");
        ok(tmp->format == dummyConceptFormatter, "pmConceptSpecAlloc() set the ->format member correctly");
        ok(tmp->copy == dummyConceptCopier, "pmConceptSpecAlloc() set the ->copy member correctly");
        ok(tmp->required == true, "pmConceptSpecAlloc() set the ->required member correctly");
        skip_end();
        psFree(tmp);
        psFree(blank);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }

    // NULL input parameters.
    {
        psMemId id = psMemGetId();
        pmConceptSpec *tmp = pmConceptSpecAlloc(NULL, NULL, NULL, NULL, false);
        ok(tmp != NULL, "pmConceptSpecAlloc() returned non-NULL with NULL inputs");
        psFree(tmp);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // --------------------------------------------------------------------
    // Tests for pmConceptGetRequired() and pmConceptSetRequired()
    // Acceptable input parameters.
    // We get the "required" for FPA.TELESCOPE, ensure that it is false
    // then set it to true, then ensure that it is true.
    {
        psMemId id = psMemGetId();
        bool tmpBool = pmConceptGetRequired("FPA.TELESCOPE", PM_FPA_LEVEL_FPA);
        ok (tmpBool == false, "pmConceptGetRequired() returned true for FPA.TELESCOPE");
        tmpBool = pmConceptSetRequired("FPA.TELESCOPE", PM_FPA_LEVEL_FPA, true);
        ok (tmpBool == true, "pmConceptSetRequired() returned true for FPA.TELESCOPE");
        tmpBool = pmConceptGetRequired("FPA.TELESCOPE", PM_FPA_LEVEL_FPA);
        ok (tmpBool == true, "pmConceptGetRequired() returned true for FPA.TELESCOPE");
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }

    // Test pmConceptGetRequired() with a few incorrect concept names
    // and/or levels.
    {
        psMemId id = psMemGetId();
        bool tmpBool = pmConceptGetRequired("FPA.TELESCOPE", PM_FPA_LEVEL_CHIP);
        ok (tmpBool == false, "pmConceptGetRequired() returned false for FPA.TELESCOPE with wrong level (CHIP)");
        tmpBool = pmConceptGetRequired("FPA.TELESCOPE", PM_FPA_LEVEL_CELL);
        ok (tmpBool == false, "pmConceptGetRequired() returned false for FPA.TELESCOPE with wrong level (CELL)");
        tmpBool = pmConceptGetRequired("FPA.BADCONCEPTNAME", PM_FPA_LEVEL_FPA);
        ok (tmpBool == false, "pmConceptGetRequired() returned false for FPA.BADCONCEPTNAME");
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // --------------------------------------------------------------------
    // Tests for pmConceptsInit(), pmConceptsDone()
    // We determine if pmConceptsInit() was successful by looking at the
    // "required" for the CHIP.XPARITY concept at level PM_FPA_LEVEL_CHIP
    {
        psMemId id = psMemGetId();
        pmConceptsDone();
        // pmConceptsInit() should return TRUE after pmConceptsDone() is called
        ok(true == pmConceptsInit(), "pmConceptsInit() returned TRUE");

        // FPA concepts
        ok(false == pmConceptGetRequired("FPA.TELESCOPE", PM_FPA_LEVEL_FPA),
          "pmConceptGetRequired() returned false for FPA.TELESCOPE at level: FPA");
        ok(false == pmConceptGetRequired("FPA.INSTRUMENT", PM_FPA_LEVEL_FPA),
          "pmConceptGetRequired() returned false for FPA.INSTRUMENT at level: FPA");
        ok(false == pmConceptGetRequired("FPA.DETECTOR", PM_FPA_LEVEL_FPA),
          "pmConceptGetRequired() returned false for FPA.DETECTOR at level: FPA");
        ok(false == pmConceptGetRequired("FPA.CAMERA", PM_FPA_LEVEL_FPA),
          "pmConceptGetRequired() returned false for FPA.CAMERA at level: FPA");
        ok(false == pmConceptGetRequired("FPA.FOCUS", PM_FPA_LEVEL_FPA),
          "pmConceptGetRequired() returned false for FPA.FOCUS at level: FPA");
        ok(false == pmConceptGetRequired("FPA.AIRMASS", PM_FPA_LEVEL_FPA),
          "pmConceptGetRequired() returned false for FPA.AIRMASS at level: FPA");
        ok(false == pmConceptGetRequired("FPA.FILTERID", PM_FPA_LEVEL_FPA),
          "pmConceptGetRequired() returned false for FPA.FILTERID at level: FPA");
        ok(false == pmConceptGetRequired("FPA.FILTER", PM_FPA_LEVEL_FPA),
          "pmConceptGetRequired() returned false for FPA.FILTER at level: FPA");
        ok(false == pmConceptGetRequired("FPA.POSANGLE", PM_FPA_LEVEL_FPA),
          "pmConceptGetRequired() returned false for FPA.POSANGLE at level: FPA");
        ok(false == pmConceptGetRequired("FPA.RADECSYS", PM_FPA_LEVEL_FPA),
          "pmConceptGetRequired() returned false for FPA.RADECSYS at level: FPA");
        ok(false == pmConceptGetRequired("FPA.RA", PM_FPA_LEVEL_FPA),
          "pmConceptGetRequired() returned false for FPA.RA at level: FPA");
        ok(false == pmConceptGetRequired("FPA.DEC", PM_FPA_LEVEL_FPA),
          "pmConceptGetRequired() returned false for FPA.DEC at level: FPA");
        ok(false == pmConceptGetRequired("FPA.OBSTYPE", PM_FPA_LEVEL_FPA),
          "pmConceptGetRequired() returned false for FPA.OBSTYPE at level: FPA");
        ok(false == pmConceptGetRequired("FPA.OBJECT", PM_FPA_LEVEL_FPA),
          "pmConceptGetRequired() returned false for FPA.OBJECT at level: FPA");
        ok(false == pmConceptGetRequired("FPA.ALT", PM_FPA_LEVEL_FPA),
          "pmConceptGetRequired() returned false for FPA.ALT at level: FPA");
        ok(false == pmConceptGetRequired("FPA.AZ", PM_FPA_LEVEL_FPA),
          "pmConceptGetRequired() returned false for FPA.AZ at level: FPA");
        ok(false == pmConceptGetRequired("FPA.TIMESYS", PM_FPA_LEVEL_FPA),
          "pmConceptGetRequired() returned false for FPA.TIMESYS at level: FPA");
        ok(false == pmConceptGetRequired("FPA.TIME", PM_FPA_LEVEL_FPA),
          "pmConceptGetRequired() returned false for FPA.TIME at level: FPA");
        ok(false == pmConceptGetRequired("FPA.TEMP", PM_FPA_LEVEL_FPA),
          "pmConceptGetRequired() returned false for FPA.TEMP at level: FPA");
        ok(false == pmConceptGetRequired("FPA.EXPOSURE", PM_FPA_LEVEL_FPA),
          "pmConceptGetRequired() returned false for FPA.EXPOSURE at level: FPA");

        // Chip concepts
        ok(true == pmConceptGetRequired("CHIP.XPARITY", PM_FPA_LEVEL_CHIP),
          "pmConceptGetRequired() returned true for CHIP.XPARITY at level: CHIP");
        ok(true == pmConceptGetRequired("CHIP.YPARITY", PM_FPA_LEVEL_CHIP),
          "pmConceptGetRequired() returned true for CHIP.YPARITY at level: CHIP");
        ok(true == pmConceptGetRequired("CHIP.X0", PM_FPA_LEVEL_CHIP),
          "pmConceptGetRequired() returned true for CHIP.X0 at level: CHIP");
        ok(true == pmConceptGetRequired("CHIP.Y0", PM_FPA_LEVEL_CHIP),
          "pmConceptGetRequired() returned true for CHIP.Y0 at level: CHIP");
        ok(true == pmConceptGetRequired("CHIP.XSIZE", PM_FPA_LEVEL_CHIP),
          "pmConceptGetRequired() returned true for CHIP.XSIZE at level: CHIP");
        ok(true == pmConceptGetRequired("CHIP.YSIZE", PM_FPA_LEVEL_CHIP),
          "pmConceptGetRequired() returned true for CHIP.YSIZE at level: CHIP");
        ok(false == pmConceptGetRequired("CHIP.TEMP", PM_FPA_LEVEL_CHIP),
          "pmConceptGetRequired() returned true for CHIP.TEMP at level: CHIP");

        // Cell concepts
        ok(true == pmConceptGetRequired("CELL.GAIN", PM_FPA_LEVEL_CELL),
          "pmConceptGetRequired() returned true for CELL.GAIN at level: CELL");
        ok(true == pmConceptGetRequired("CELL.READNOISE", PM_FPA_LEVEL_CELL),
          "pmConceptGetRequired() returned true for CELL.READNOISE at level: CELL");
        ok(true == pmConceptGetRequired("CELL.SATURATION", PM_FPA_LEVEL_CELL),
          "pmConceptGetRequired() returned true for CELL.SATURATION at level: CELL");
        ok(true == pmConceptGetRequired("CELL.BAD", PM_FPA_LEVEL_CELL),
          "pmConceptGetRequired() returned true for CELL.BAD at level: CELL");
        ok(true == pmConceptGetRequired("CELL.XPARITY", PM_FPA_LEVEL_CELL),
          "pmConceptGetRequired() returned true for CELL.XPARITY at level: CELL");
        ok(true == pmConceptGetRequired("CELL.YPARITY", PM_FPA_LEVEL_CELL),
          "pmConceptGetRequired() returned true for CELL.YPARITY at level: CELL");
        ok(true == pmConceptGetRequired("CELL.READDIR", PM_FPA_LEVEL_CELL),
          "pmConceptGetRequired() returned true for CELL.READDIR at level: CELL");
        ok(false == pmConceptGetRequired("CELL.EXPOSURE", PM_FPA_LEVEL_CELL),
          "pmConceptGetRequired() returned false for CELL.EXPOSURE at level: CELL");
        ok(false == pmConceptGetRequired("CELL.DARKTIME", PM_FPA_LEVEL_CELL),
          "pmConceptGetRequired() returned false for CELL.DARKTIME at level: CELL");
        ok(true == pmConceptGetRequired("CELL.TRIMSEC", PM_FPA_LEVEL_CELL),
          "pmConceptGetRequired() returned true for CELL.TRIMSEC at level: CELL");
        ok(true == pmConceptGetRequired("CELL.BIASSEC", PM_FPA_LEVEL_CELL),
          "pmConceptGetRequired() returned true for CELL.BIASSEC at level: CELL");
        ok(true == pmConceptGetRequired("CELL.XBIN", PM_FPA_LEVEL_CELL),
          "pmConceptGetRequired() returned true for CELL.XBIN at level: CELL");
        ok(true == pmConceptGetRequired("CELL.YBIN", PM_FPA_LEVEL_CELL),
          "pmConceptGetRequired() returned true for CELL.YBIN at level: CELL");
        ok(false == pmConceptGetRequired("CELL.TIMESYS", PM_FPA_LEVEL_CELL),
          "pmConceptGetRequired() returned false for CELL.TIMESYS at level: CELL");
        ok(false == pmConceptGetRequired("CELL.TIME", PM_FPA_LEVEL_CELL),
          "pmConceptGetRequired() returned false for CELL.TIME at level: CELL");
        ok(true == pmConceptGetRequired("CELL.X0", PM_FPA_LEVEL_CELL),
          "pmConceptGetRequired() returned true for CELL.X0 at level: CELL");
        ok(true == pmConceptGetRequired("CELL.Y0", PM_FPA_LEVEL_CELL),
          "pmConceptGetRequired() returned true for CELL.Y0 at level: CELL");
        ok(true == pmConceptGetRequired("CELL.XSIZE", PM_FPA_LEVEL_CELL),
          "pmConceptGetRequired() returned true for CELL.XSIZE at level: CELL");
        ok(true == pmConceptGetRequired("CELL.YSIZE", PM_FPA_LEVEL_CELL),
          "pmConceptGetRequired() returned true for CELL.YSIZE at level: CELL");
        ok(true == pmConceptGetRequired("CELL.XWINDOW", PM_FPA_LEVEL_CELL),
          "pmConceptGetRequired() returned true for CELL.XWINDOW at level: CELL");
        ok(true == pmConceptGetRequired("CELL.YWINDOW", PM_FPA_LEVEL_CELL),
          "pmConceptGetRequired() returned true for CELL.YWINDOW at level: CELL");

        // The 2nd pmConceptsInit() should return FALSE after pmConceptsInit() is called
        ok(false == pmConceptsInit(), "pmConceptsInit() returned FALSE");

        pmConceptsDone();
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }



    // --------------------------------------------------------------------
    // Tests for pmConceptsBlankFPA()
    // Verify error with NULL input.
    {
        psMemId id = psMemGetId();
        bool rc = pmConceptsBlankFPA(NULL);
        ok(rc == false, "pmConceptsBlankFPA() returned FALSE with NULL input");
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // -----------------------------------------------------------------------------
    // Tests for pmConceptsBlankFPA()
    // Call with valid data.  We test by ensuring the first 5 metadata items were
    // added to fpa->concepts.
    {
        psMemId id = psMemGetId();
        bool mdok;
        char *tmpStr;
        psF32 tmpF32;
        psF64 tmpF64;
        psS32 tmpS32;
        pmFPA *fpa = pmFPAAlloc(NULL, NULL);
        bool rc = false;

        // First junk items to fpa->concepts so that we know they are later blanked.
        rc|= psMetadataAddStr(fpa->concepts, PS_LIST_TAIL, "FPA.TELESCOPE", PS_META_REPLACE, "", "JUNK");
        rc|= psMetadataAddStr(fpa->concepts, PS_LIST_TAIL, "FPA.INSTRUMENT", PS_META_REPLACE, "", "JUNK");
        rc|= psMetadataAddStr(fpa->concepts, PS_LIST_TAIL, "FPA.DETECTOR", PS_META_REPLACE, "", "JUNK");
        rc|= psMetadataAddStr(fpa->concepts, PS_LIST_TAIL, "FPA.CAMERA", PS_META_REPLACE, "", "JUNK");
        rc|= psMetadataAddF32(fpa->concepts, PS_LIST_TAIL, "FPA.FOCUS", PS_META_REPLACE, "", 22.0);
        rc|= psMetadataAddF32(fpa->concepts, PS_LIST_TAIL, "FPA.AIRMASS", PS_META_REPLACE, "", 22.0);
        rc|= psMetadataAddStr(fpa->concepts, PS_LIST_TAIL, "FPA.FILTERID", PS_META_REPLACE, "", "JUNK");
        rc|= psMetadataAddStr(fpa->concepts, PS_LIST_TAIL, "FPA.FILTER", PS_META_REPLACE, "", "JUNK");
        rc|= psMetadataAddF32(fpa->concepts, PS_LIST_TAIL, "FPA.POSANGLE", PS_META_REPLACE, "", 22.0);
        rc|= psMetadataAddStr(fpa->concepts, PS_LIST_TAIL, "FPA.RADECSYS", PS_META_REPLACE, "", "JUNK");
        rc|= psMetadataAddF64(fpa->concepts, PS_LIST_TAIL, "FPA.RA", PS_META_REPLACE, "", 22.0);
        rc|= psMetadataAddF64(fpa->concepts, PS_LIST_TAIL, "FPA.DEC", PS_META_REPLACE, "", 22.0);
        rc|= psMetadataAddStr(fpa->concepts, PS_LIST_TAIL, "FPA.OBSTYPE", PS_META_REPLACE, "", "JUNK");
        rc|= psMetadataAddStr(fpa->concepts, PS_LIST_TAIL, "FPA.OBJECT", PS_META_REPLACE, "", "JUNK");
        rc|= psMetadataAddF64(fpa->concepts, PS_LIST_TAIL, "FPA.ALT", PS_META_REPLACE, "", 22.0);
        rc|= psMetadataAddF64(fpa->concepts, PS_LIST_TAIL, "FPA.AZ", PS_META_REPLACE, "", 22.0);
        rc|= psMetadataAddS32(fpa->concepts, PS_LIST_TAIL, "FPA.TIMESYS", PS_META_REPLACE, "", 22);
        rc|= psMetadataAddF32(fpa->concepts, PS_LIST_TAIL, "FPA.TEMP", PS_META_REPLACE, "", 22.0);
        rc|= psMetadataAddF32(fpa->concepts, PS_LIST_TAIL, "FPA.EXPOSURE", PS_META_REPLACE, "", 22.0);
        ok(rc, "Set dummy data in fpa->concepts");

        rc = pmConceptsBlankFPA(fpa);
        ok(rc == true, "pmConceptsBlankFPA() returned TRUE with valid input data");
        tmpStr = psMetadataLookupStr(&mdok, fpa->concepts, "FPA.TELESCOPE");
        ok(mdok && !strcmp(tmpStr, ""), "FPA.TELESCOPE was cleared (%s)", tmpStr);
        tmpStr = psMetadataLookupStr(&mdok, fpa->concepts, "FPA.INSTRUMENT");
        ok(mdok && !strcmp(tmpStr, ""), "FPA.INSTRUMENT was cleared (%s)", tmpStr);
        tmpStr = psMetadataLookupStr(&mdok, fpa->concepts, "FPA.DETECTOR");
        ok(mdok && !strcmp(tmpStr, ""), "FPA.DETECTOR was cleared (%s)", tmpStr);
        tmpStr = psMetadataLookupStr(&mdok, fpa->concepts, "FPA.CAMERA");
        ok(mdok && !strcmp(tmpStr, ""), "FPA.CAMERA was cleared (%s)", tmpStr);
        tmpF32 = psMetadataLookupF32(&mdok, fpa->concepts, "FPA.FOCUS");
        ok(mdok && isnan(tmpF32), "FPA.FOCUS was cleared (%f)", tmpF32);
        tmpF32 = psMetadataLookupF32(&mdok, fpa->concepts, "FPA.AIRMASS");
        ok(mdok && isnan(tmpF32), "FPA.AIRMASS was cleared (%f)", tmpF32);
        tmpStr = psMetadataLookupStr(&mdok, fpa->concepts, "FPA.FILTERID");
        ok(mdok && !strcmp(tmpStr, ""), "FPA.FILTERID was cleared (%s)", tmpStr);
        tmpStr = psMetadataLookupStr(&mdok, fpa->concepts, "FPA.FILTER");
        ok(mdok && !strcmp(tmpStr, ""), "FPA.FILTER was cleared (%s)", tmpStr);
        tmpF32 = psMetadataLookupF32(&mdok, fpa->concepts, "FPA.POSANGLE");
        ok(mdok && isnan(tmpF32), "FPA.POSANGLE was cleared (%f)", tmpF32);
        tmpStr = psMetadataLookupStr(&mdok, fpa->concepts, "FPA.RADECSYS");
        ok(mdok && !strcmp(tmpStr, ""), "FPA.RADECSYS was cleared (%s)", tmpStr);
        tmpF64 = psMetadataLookupF64(&mdok, fpa->concepts, "FPA.RA");
        ok(mdok && isnan(tmpF64), "FPA.RA was cleared (%f)", tmpF64);
        tmpF64 = psMetadataLookupF64(&mdok, fpa->concepts, "FPA.DEC");
        ok(mdok && isnan(tmpF64), "FPA.DEC was cleared (%f)", tmpF64);
        tmpStr = psMetadataLookupStr(&mdok, fpa->concepts, "FPA.OBSTYPE");
        ok(mdok && !strcmp(tmpStr, ""), "FPA.OBSTYPE was cleared (%s)", tmpStr);
        tmpStr = psMetadataLookupStr(&mdok, fpa->concepts, "FPA.OBJECT");
        ok(mdok && !strcmp(tmpStr, ""), "FPA.OBJECT was cleared (%s)", tmpStr);
        tmpF64 = psMetadataLookupF64(&mdok, fpa->concepts, "FPA.ALT");
        ok(mdok && isnan(tmpF64), "FPA.ALT was cleared (%f)", tmpF64);
        tmpF64 = psMetadataLookupF64(&mdok, fpa->concepts, "FPA.AZ");
        ok(mdok && isnan(tmpF64), "FPA.AZ was cleared (%f)", tmpF64);
        tmpS32 = psMetadataLookupS32(&mdok, fpa->concepts, "FPA.TIMESYS");
        ok(mdok && -1 == tmpS32, "FPA.TIMESYS was cleared (%d)", tmpS32);
        // XXX: Add code to make sure it was cleared.
        psMetadataItem *tmpMI = psMetadataLookup(fpa->concepts, "FPA.TIME");
        ok(tmpMI != NULL, "FPA.TIME was cleared");
        tmpF32 = psMetadataLookupF32(&mdok, fpa->concepts, "FPA.TEMP");
        ok(mdok && isnan(tmpF32), "FPA.TEMP was cleared (%f)", tmpF32);
        tmpF32 = psMetadataLookupF32(&mdok, fpa->concepts, "FPA.TEMP");
        ok(mdok && isnan(tmpF32), "FPA.TEMP was cleared (%f)", tmpF32);

        psFree(fpa);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }

    // -----------------------------------------------------------------------------
    // Tests for pmConceptsBlankChip()
    // Call with valid data.  We test by ensuring the first 5 metadata items were
    // added to chip->concepts.
    {
        psMemId id = psMemGetId();
        bool mdok;
        psF32 tmpF32;
        psS32 tmpS32;
        pmChip *chip = pmChipAlloc(NULL, NULL);
        bool rc = false;

        // First junk items to chip->concepts so that we know they are later blanked.
        rc|= psMetadataAddS32(chip->concepts, PS_LIST_TAIL, "CHIP.XPARITY", PS_META_REPLACE, "", 22);
        rc|= psMetadataAddS32(chip->concepts, PS_LIST_TAIL, "CHIP.YPARITY", PS_META_REPLACE, "", 22);
        rc|= psMetadataAddS32(chip->concepts, PS_LIST_TAIL, "CHIP.X0", PS_META_REPLACE, "", 22);
        rc|= psMetadataAddS32(chip->concepts, PS_LIST_TAIL, "CHIP.Y0", PS_META_REPLACE, "", 22);
        rc|= psMetadataAddS32(chip->concepts, PS_LIST_TAIL, "CHIP.XSIZE", PS_META_REPLACE, "", 22);
        rc|= psMetadataAddS32(chip->concepts, PS_LIST_TAIL, "CHIP.YSIZE", PS_META_REPLACE, "", 22);
        rc|= psMetadataAddF32(chip->concepts, PS_LIST_TAIL, "CHIP.TEMP", PS_META_REPLACE, "", 22.0);
        ok(rc, "Set dummy data in chip->concepts");

        rc = pmConceptsBlankChip(chip);
        tmpS32 = psMetadataLookupS32(&mdok, chip->concepts, "CHIP.XPARITY");
        ok(mdok && 0 == tmpS32, "CHIP.XPARITY was cleared (%d)", tmpS32);
        tmpS32 = psMetadataLookupS32(&mdok, chip->concepts, "CHIP.YPARITY");
        ok(mdok && 0 == tmpS32, "CHIP.YPARITY was cleared (%d)", tmpS32);
        tmpS32 = psMetadataLookupS32(&mdok, chip->concepts, "CHIP.X0");
        ok(mdok && 0 == tmpS32, "CHIP.X0 was cleared (%d)", tmpS32);
        tmpS32 = psMetadataLookupS32(&mdok, chip->concepts, "CHIP.Y0");
        ok(mdok && 0 == tmpS32, "CHIP.Y0 was cleared (%d)", tmpS32);
        tmpS32 = psMetadataLookupS32(&mdok, chip->concepts, "CHIP.XSIZE");
        ok(mdok && 0 == tmpS32, "CHIP.XSIZE was cleared (%d)", tmpS32);
        tmpS32 = psMetadataLookupS32(&mdok, chip->concepts, "CHIP.YSIZE");
        ok(mdok && 0 == tmpS32, "CHIP.YSIZE was cleared (%d)", tmpS32);
        tmpF32 = psMetadataLookupF32(&mdok, chip->concepts, "CHIP.TEMP");
        ok(mdok && isnan(tmpF32), "CHIP.TEMP was cleared (%d)", tmpF32);

        psFree(chip);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }



    // -----------------------------------------------------------------------------
    // Tests for pmConceptsBlankCell()
    // Call with valid data.  We test by ensuring the first 5 metadata items were
    // added to cell->concepts.
    {
        psMemId id = psMemGetId();
        bool mdok;
        psF32 tmpF32;
        psS32 tmpS32;
        pmCell *cell = pmCellAlloc(NULL, NULL);
        bool rc = false;

        // First junk items to cell->concepts so that we know they are later blanked.
        rc|= psMetadataAddF32(cell->concepts, PS_LIST_TAIL, "CELL.GAIN", PS_META_REPLACE, "", 22.0);
        rc|= psMetadataAddF32(cell->concepts, PS_LIST_TAIL, "CELL.READNOISE", PS_META_REPLACE, "", 22.0);
        rc|= psMetadataAddS32(cell->concepts, PS_LIST_TAIL, "CELL.XPARITY", PS_META_REPLACE, "", 22);
        rc|= psMetadataAddS32(cell->concepts, PS_LIST_TAIL, "CELL.YPARITY", PS_META_REPLACE, "", 22);
        rc|= psMetadataAddS32(cell->concepts, PS_LIST_TAIL, "CELL.READDIR", PS_META_REPLACE, "", 22);
        rc|= psMetadataAddF32(cell->concepts, PS_LIST_TAIL, "CELL.SATURATION", PS_META_REPLACE, "", 22.0);
        rc|= psMetadataAddF32(cell->concepts, PS_LIST_TAIL, "CELL.BAD", PS_META_REPLACE, "", 22.0);
        rc|= psMetadataAddF32(cell->concepts, PS_LIST_TAIL, "CELL.EXPOSURE", PS_META_REPLACE, "", 22.0);
        rc|= psMetadataAddF32(cell->concepts, PS_LIST_TAIL, "CELL.DARKTIME", PS_META_REPLACE, "", 22.0);
        rc|= psMetadataAddS32(cell->concepts, PS_LIST_TAIL, "CELL.XBIN", PS_META_REPLACE, "", 22);
        rc|= psMetadataAddS32(cell->concepts, PS_LIST_TAIL, "CELL.YBIN", PS_META_REPLACE, "", 22);
        rc|= psMetadataAddS32(cell->concepts, PS_LIST_TAIL, "CELL.TIMESYS", PS_META_REPLACE, "", 22);
        rc|= psMetadataAddS32(cell->concepts, PS_LIST_TAIL, "CELL.X0", PS_META_REPLACE, "", 22);
        rc|= psMetadataAddS32(cell->concepts, PS_LIST_TAIL, "CELL.Y0", PS_META_REPLACE, "", 22);
        rc|= psMetadataAddS32(cell->concepts, PS_LIST_TAIL, "CELL.XSIZE", PS_META_REPLACE, "", 22);
        rc|= psMetadataAddS32(cell->concepts, PS_LIST_TAIL, "CELL.YSIZE", PS_META_REPLACE, "", 22);
        rc|= psMetadataAddS32(cell->concepts, PS_LIST_TAIL, "CELL.XWINDOW", PS_META_REPLACE, "", 22);
        rc|= psMetadataAddS32(cell->concepts, PS_LIST_TAIL, "CELL.YWINDOW", PS_META_REPLACE, "", 22);
        rc|= psMetadataAddS32(cell->concepts, PS_LIST_TAIL, "CELL.", PS_META_REPLACE, "", 22);


        ok(rc, "Set dummy data in cell->concepts");

        rc = pmConceptsBlankCell(cell);
        ok(rc == true, "pmConceptsBlankCELL() returned TRUE with valid input data");

        tmpF32 = psMetadataLookupF32(&mdok, cell->concepts, "CELL.GAIN");
        ok(mdok && isnan(tmpF32), "CELL.GAIN was cleared (%f)", tmpF32);
        tmpF32 = psMetadataLookupF32(&mdok, cell->concepts, "CELL.READNOISE");
        ok(mdok && isnan(tmpF32), "CELL.READNOISE was cleared (%f)", tmpF32);
        tmpF32 = psMetadataLookupF32(&mdok, cell->concepts, "CELL.SATURATION");
        ok(mdok && isnan(tmpF32), "CELL.SATURATION was cleared (%f)", tmpF32);
        tmpF32 = psMetadataLookupF32(&mdok, cell->concepts, "CELL.BAD");
        ok(mdok && isnan(tmpF32), "CELL.BAD was cleared (%f)", tmpF32);

        tmpS32 = psMetadataLookupS32(&mdok, cell->concepts, "CELL.XPARITY");
        ok(mdok && 0 == tmpS32, "CELL.XPARITY was cleared (%d)", tmpS32);
        tmpS32 = psMetadataLookupS32(&mdok, cell->concepts, "CELL.YPARITY");
        ok(mdok && 0 == tmpS32, "CELL.YPARITY was cleared (%d)", tmpS32);
        tmpS32 = psMetadataLookupS32(&mdok, cell->concepts, "CELL.READDIR");
        ok(mdok && 0 == tmpS32, "CELL.READDIR was cleared (%d)", tmpS32);
        tmpF32 = psMetadataLookupF32(&mdok, cell->concepts, "CELL.EXPOSURE");
        ok(mdok && isnan(tmpF32), "CELL.EXPOSURE was cleared (%f)", tmpF32);
        tmpF32 = psMetadataLookupF32(&mdok, cell->concepts, "CELL.DARKTIME");
        ok(mdok && isnan(tmpF32), "CELL.DARKTIME was cleared (%f)", tmpF32);
        // XXX: Add code to make sure it was cleared.
        psMetadataItem *tmpMI = psMetadataLookup(cell->concepts, "CELL.TRIMSEC");
        ok(tmpMI != NULL, "CELL.TRIMSEC was cleared");
        tmpMI = psMetadataLookup(cell->concepts, "CELL.BIASSEC");
        ok(tmpMI != NULL, "CELL.BIASSEC was cleared");
        tmpS32 = psMetadataLookupS32(&mdok, cell->concepts, "CELL.XBIN");
        ok(mdok && 0 == tmpS32, "CELL.XBIN was cleared (%d)", tmpS32);
        tmpS32 = psMetadataLookupS32(&mdok, cell->concepts, "CELL.YBIN");
        ok(mdok && 0 == tmpS32, "CELL.YBIN was cleared (%d)", tmpS32);
        tmpS32 = psMetadataLookupS32(&mdok, cell->concepts, "CELL.TIMESYS");
        ok(mdok && -1 == tmpS32, "CELL.TIMESYS was cleared (%d)", tmpS32);
        // XXX: Add code to make sure it was cleared.
        tmpMI = psMetadataLookup(cell->concepts, "CELL.TIME");
        ok(tmpMI != NULL, "CELL.TIME was cleared");
        tmpS32 = psMetadataLookupS32(&mdok, cell->concepts, "CELL.X0");
        ok(mdok && 0 == tmpS32, "CELL.X0 was cleared (%d)", tmpS32);
        tmpS32 = psMetadataLookupS32(&mdok, cell->concepts, "CELL.Y0");
        ok(mdok && 0 == tmpS32, "CELL.Y0 was cleared (%d)", tmpS32);
        tmpS32 = psMetadataLookupS32(&mdok, cell->concepts, "CELL.XSIZE");
        ok(mdok && 0 == tmpS32, "CELL.XSIZE was cleared (%d)", tmpS32);
        tmpS32 = psMetadataLookupS32(&mdok, cell->concepts, "CELL.YSIZE");
        ok(mdok && 0 == tmpS32, "CELL.YSIZE was cleared (%d)", tmpS32);
        tmpS32 = psMetadataLookupS32(&mdok, cell->concepts, "CELL.XWINDOW");
        ok(mdok && 0 == tmpS32, "CELL.XWINDOW was cleared (%d)", tmpS32);
        tmpS32 = psMetadataLookupS32(&mdok, cell->concepts, "CELL.YWINDOW");
        ok(mdok && 0 == tmpS32, "CELL.YWINDOW was cleared (%d)", tmpS32);

        psFree(cell);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }



}
