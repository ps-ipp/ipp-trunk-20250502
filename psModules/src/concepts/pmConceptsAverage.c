#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <pslib.h>
#include <string.h>

#include "pmHDU.h"
#include "pmFPA.h"
#include "pmFPALevel.h"
#include "pmConcepts.h"
#include "pmConceptsAverage.h"

// Update a metadata entry directly
#define MD_UPDATE(MD, NAME, TYPE, VALUE) \
{ \
    psMetadataItem *item = psMetadataLookup(MD, NAME); \
    item->data.TYPE = VALUE; \
}

// Update a metadata string entry directly
#define MD_UPDATE_STR(MD, NAME, VALUE) \
{ \
    psMetadataItem *item = psMetadataLookup(MD, NAME); \
    psFree(item->data.str); \
    item->data.str = psStringCopy(VALUE); \
}


bool pmConceptsAverageFPAs(pmFPA *target, psList *sources)
{
    PS_ASSERT_PTR_NON_NULL(target, false);
    PS_ASSERT_PTR_NON_NULL(sources, false);
    PS_ASSERT_INT_POSITIVE(sources->n, false);

    double time      = 0.0;             // Time of observation
    double zp        = 0.0;             // Zero point
    psTimeType timeSys = 0;             // Time system
    char *filter     = NULL;            // Filter
    char *filterId   = NULL;            // Filter (parsed, abstract name)
    char *telescope  = NULL;            // Telescope of origin
    char *instrument = NULL;            // Instrument name
    char *detector   = NULL;            // Detector name

    int num = 0;                        // Number of FPAs
    psListIterator *sourcesIter = psListIteratorAlloc(sources, PS_LIST_HEAD, false); // Iterator for sources
    pmFPA *fpa = NULL;                  // Source FPA from iteration
    while ((fpa = psListGetAndIncrement(sourcesIter))) {
        if (!fpa) {
            continue;
        }

        num++;

#define COMPARE_STR(NAME, VALUE) \
    if (strcmp(VALUE, psMetadataLookupStr(NULL, fpa->concepts, NAME)) != 0) { \
        psWarning("Differing %s in use: %s vs %s\n", \
                  NAME, VALUE, psMetadataLookupStr(NULL, fpa->concepts, NAME)); \
        VALUE = "VARIOUS"; \
    }

        psTime *fpaTime = psMetadataLookupPtr(NULL, fpa->concepts, "FPA.TIME");
        psTimeConvert(fpaTime, PS_TIME_TAI);
        time       += psTimeToMJD(fpaTime);

        zp += psMetadataLookupF32(NULL, fpa->concepts, "FPA.ZP");

        if (num == 1) {
            timeSys = psMetadataLookupS32(NULL, fpa->concepts, "FPA.TIMESYS");
            filter = psMetadataLookupStr(NULL, fpa->concepts, "FPA.FILTER");
            filterId = psMetadataLookupStr(NULL, fpa->concepts, "FPA.FILTERID");
            telescope = psMetadataLookupStr(NULL, fpa->concepts, "FPA.TELESCOPE");
            instrument = psMetadataLookupStr(NULL, fpa->concepts, "FPA.INSTRUMENT");
            detector = psMetadataLookupStr(NULL, fpa->concepts, "FPA.DETECTOR");
        } else {
            if (timeSys != psMetadataLookupS32(NULL, fpa->concepts, "FPA.TIMESYS")) {
                psWarning("Differing FPA.TIMESYS in use: %d vs %d\n",
                          timeSys, psMetadataLookupS32(NULL, fpa->concepts, "FPA.TIMESYS"));
            }
            COMPARE_STR("FPA.FILTER", filter);
            COMPARE_STR("FPA.FILTERID", filterId);
            COMPARE_STR("FPA.TELESCOPE", telescope);
            COMPARE_STR("FPA.INSTRUMENT", instrument);
            COMPARE_STR("FPA.DETECTOR", detector);
        }
    }
    psFree(sourcesIter);

    time /= (double)num;
    zp /= (double)num;

    MD_UPDATE(target->concepts, "FPA.TIMESYS", S32, timeSys);
    MD_UPDATE_STR(target->concepts, "FPA.FILTER", filter);
    MD_UPDATE_STR(target->concepts, "FPA.FILTERID", filterId);
    MD_UPDATE_STR(target->concepts, "FPA.TELESCOPE", telescope);
    MD_UPDATE_STR(target->concepts, "FPA.INSTRUMENT", instrument);
    MD_UPDATE_STR(target->concepts, "FPA.DETECTOR", detector);
    MD_UPDATE(target->concepts, "FPA.ZP", F32, zp);

    // FPA.TIME needs special care
    {
        psMetadataItem *timeItem = psMetadataLookup(target->concepts, "FPA.TIME");
        psFree(timeItem->data.V);
        psTime *new = psTimeFromMJD(time);
        psTimeConvert(new, timeSys);
        timeItem->data.V = new;
    }

    return true;
}

float averageWithDropouts (psList *sources, char *name) {

    bool status;

    float sum = 0;
    int nCells = 0;                     // Number of cells;
    psListIterator *sourcesIter = psListIteratorAlloc(sources, PS_LIST_HEAD, false); // Iterator for sources
    pmCell *cell = NULL;                // Source cell from iteration
    while ((cell = psListGetAndIncrement(sourcesIter))) {
        if (!cell) {
            continue;
        }

        float value = psMetadataLookupF32(&status, cell->concepts, name);
        if (!status) continue;
        if (!isfinite(value)) continue;

        sum += value;
        nCells++;
    }
    psFree (sourcesIter);

    float average = sum / nCells;
    return average;
}
float medianWithDropouts (psList *sources, char *name) {

    bool status;

    psListIterator *sourcesIter = psListIteratorAlloc(sources, PS_LIST_HEAD, false); // Iterator for sources
    pmCell *cell = NULL;                // Source cell from iteration

    psStats *stats = psStatsAlloc(PS_STAT_SAMPLE_MEDIAN);
    psVector *values = psVectorAlloc(sources->n, PS_TYPE_F32);
    int nvalues = 0;
    while ((cell = psListGetAndIncrement(sourcesIter))) {
        if (!cell) {
            continue;
        }

        float value = psMetadataLookupF32(&status, cell->concepts, name);
        if (!status) continue;
        if (!isfinite(value)) continue;

        values->data.F32[nvalues++] = value;
    }
    psFree (sourcesIter);
    if (!nvalues) {
        psWarning("no valid values found for %s\n", name);
        psFree(values);
        psFree(stats);
        return INFINITY;
    }
    if (!(values = psVectorRealloc(values, nvalues))) {
        psWarning("failed to reallocate values vector for %s\n", name);
        psFree(stats);
        return INFINITY;
    }
    if (!psVectorStats(stats, values, NULL, NULL, 0)) {
        psWarning("psVectorStats failed for %s\n", name);
        psFree(values);
        psFree(stats);
        return INFINITY;
    }

    psF32 median = psStatsGetValue(stats, PS_STAT_SAMPLE_MEDIAN);

    psFree(values);
    psFree(stats);

    return median;
}

// Set a variety of concepts in a cell by averaging over several
// XXX does not properly set XSIZE, YSIZE
bool pmConceptsAverageCells(pmCell *target, psList *sources, psRegion *trimsec, psRegion *biassec, bool same)
{
    PS_ASSERT_PTR_NON_NULL(target, false);
    PS_ASSERT_PTR_NON_NULL(sources, false);
    PS_ASSERT_INT_POSITIVE(sources->n, false);

    float bad        = -INFINITY;       // Bad level
    double time      = 0.0;             // Time of observation
    psTimeType timeSys = 0;             // Time system
    int readdir      = 0;               // Cell read direction
    int xBin = 0, yBin = 0;             // Binning
    int x0 = 0, y0 = 0;                 // Offset
    int xParity = 0, yParity = 0;       // Parity

    float gain      = averageWithDropouts (sources, "CELL.GAIN");
    float readnoise = averageWithDropouts (sources, "CELL.READNOISE");
    float exposure  = averageWithDropouts (sources, "CELL.EXPOSURE");
    float darktime  = averageWithDropouts (sources, "CELL.DARKTIME");
    float saturation = medianWithDropouts(sources, "CELL.SATURATION");

    // other concepts are a bit more "special"
    int nCells = 0;                     // Number of cells;
    psListIterator *sourcesIter = psListIteratorAlloc(sources, PS_LIST_HEAD, false); // Iterator for sources
    pmCell *cell = NULL;                // Source cell from iteration
    while ((cell = psListGetAndIncrement(sourcesIter))) {
        if (!cell) {
            continue;
        }

        nCells++;
        psTime *cellTime = psMetadataLookupPtr(NULL, cell->concepts, "CELL.TIME");
        time       += psTimeToMJD(cellTime);
        if (nCells == 1) {
            timeSys = psMetadataLookupS32(NULL, cell->concepts, "CELL.TIMESYS");
            readdir = psMetadataLookupS32(NULL, cell->concepts, "CELL.READDIR");
            xBin    = psMetadataLookupS32(NULL, cell->concepts, "CELL.XBIN");
            yBin    = psMetadataLookupS32(NULL, cell->concepts, "CELL.YBIN");

            if (same) {
                // Only makes sense to update these if they are the same cell
                x0 = psMetadataLookupS32(NULL, cell->concepts, "CELL.X0");
                y0 = psMetadataLookupS32(NULL, cell->concepts, "CELL.Y0");
                xParity = psMetadataLookupS32(NULL, cell->concepts, "CELL.XPARITY");
                yParity = psMetadataLookupS32(NULL, cell->concepts, "CELL.YPARITY");
            }
        } else {
            if (timeSys != psMetadataLookupS32(NULL, cell->concepts, "CELL.TIMESYS")) {
                psWarning("Differing CELL.TIMESYS in use: %d vs %d\n",
                          timeSys, psMetadataLookupS32(NULL, cell->concepts, "CELL.TIMESYS"));
            }
            if (readdir != psMetadataLookupS32(NULL, cell->concepts, "CELL.READDIR")) {
                psWarning("Differing CELL.READDIR in use: %d vs %d\n",
                          readdir, psMetadataLookupS32(NULL, cell->concepts, "CELL.READDIR"));
            }
            if (xBin != psMetadataLookupS32(NULL, cell->concepts, "CELL.XBIN")) {
                psWarning("Differing CELL.XBIN in use: %d vs %d\n",
                          xBin, psMetadataLookupS32(NULL, cell->concepts, "CELL.XBIN"));
            }
            if (yBin != psMetadataLookupS32(NULL, cell->concepts, "CELL.YBIN")) {
                psWarning("Differing CELL.YBIN in use: %d vs %d\n",
                          yBin, psMetadataLookupS32(NULL, cell->concepts, "CELL.YBIN"));
            }
            if (same) {
                if (x0 != psMetadataLookupS32(NULL, cell->concepts, "CELL.X0")) {
                    psWarning("Differing CELL.X0 in use: %d vs %d\n",
                              x0, psMetadataLookupS32(NULL, cell->concepts, "CELL.X0"));
                }
                if (y0 != psMetadataLookupS32(NULL, cell->concepts, "CELL.Y0")) {
                    psWarning("Differing CELL.Y0 in use: %d vs %d\n",
                              y0, psMetadataLookupS32(NULL, cell->concepts, "CELL.Y0"));
                }
                if (xParity != psMetadataLookupS32(NULL, cell->concepts, "CELL.XPARITY")) {
                    psWarning("Differing CELL.XPARITY in use: %d vs %d\n",
                              xParity, psMetadataLookupS32(NULL, cell->concepts, "CELL.XPARITY"));
                }
                if (yParity != psMetadataLookupS32(NULL, cell->concepts, "CELL.YPARITY")) {
                    psWarning("Differing CELL.YPARITY in use: %d vs %d\n",
                              yParity, psMetadataLookupS32(NULL, cell->concepts, "CELL.YPARITY"));
                }
            }
        }

        float cellBad = psMetadataLookupF32(NULL, cell->concepts, "CELL.BAD");
        if (cellBad > bad) {
            bad = cellBad;
        }
    }
    psFree(sourcesIter);

    time /= (double) nCells;

    MD_UPDATE(target->concepts, "CELL.GAIN", F32, gain);
    MD_UPDATE(target->concepts, "CELL.READNOISE", F32, readnoise);
    MD_UPDATE(target->concepts, "CELL.SATURATION", F32, saturation);
    MD_UPDATE(target->concepts, "CELL.BAD", F32, bad);
    MD_UPDATE(target->concepts, "CELL.EXPOSURE", F32, exposure);
    MD_UPDATE(target->concepts, "CELL.DARKTIME", F32, darktime);
    MD_UPDATE(target->concepts, "CELL.TIMESYS", S32, timeSys);
    MD_UPDATE(target->concepts, "CELL.READDIR", S32, readdir);
    MD_UPDATE(target->concepts, "CELL.XBIN", S32, xBin);
    MD_UPDATE(target->concepts, "CELL.YBIN", S32, yBin);
    if (same) {
        MD_UPDATE(target->concepts, "CELL.X0", S32, x0);
        MD_UPDATE(target->concepts, "CELL.Y0", S32, y0);
        MD_UPDATE(target->concepts, "CELL.XPARITY", S32, xParity);
        MD_UPDATE(target->concepts, "CELL.YPARITY", S32, yParity);
    }

    // CELL.TIME needs special care
    {
        psMetadataItem *timeItem = psMetadataLookup(target->concepts, "CELL.TIME");
        psFree(timeItem->data.V);
        psTime *new = psTimeFromMJD(time);
        psTimeConvert(new, timeSys);
        timeItem->data.V = new;
    }

    // CELL.TRIMSEC needs special care
    if (trimsec) {
        psMetadataItem *trimsecItem = psMetadataLookup(target->concepts, "CELL.TRIMSEC");
        psFree(trimsecItem->data.V);
        trimsecItem->data.V = psMemIncrRefCounter(trimsec);
    }

    // CELL.BIASSEC needs special care
    if (biassec) {
        psMetadataItem *biassecItem = psMetadataLookup(target->concepts, "CELL.BIASSEC");
        psFree(biassecItem->data.V);
        biassecItem->data.V = psMemIncrRefCounter(biassec);
    }

    return true;
}

bool pmConceptsAverageChips(pmChip *target, psList *sources, bool same)
{
    PS_ASSERT_PTR_NON_NULL(target, false);
    PS_ASSERT_PTR_NON_NULL(sources, false);
    PS_ASSERT_INT_POSITIVE(sources->n, false);

    float temp = 0.0;                   // Temperature
    float seeing = 0.0;                 // Seeing FWHM
    int x0 = 0, y0 = 0;                 // Offset
    int xParity = 0, yParity = 0;       // Parity
    int xSize = 0, ySize = 0;           // Size
    psString id = NULL;                 // Identifier

    int nChips = 0;                     // Number of chips;
    psListIterator *sourcesIter = psListIteratorAlloc(sources, PS_LIST_HEAD, false); // Iterator for sources
    pmChip *chip = NULL;                // Source chip from iteration
    while ((chip = psListGetAndIncrement(sourcesIter))) {
        if (!chip) {
            continue;
        }
        temp += psMetadataLookupF32(NULL, chip->concepts, "CHIP.TEMP");
        seeing += psMetadataLookupF32(NULL, chip->concepts, "CHIP.SEEING");
        if (nChips == 0) {
            xSize = psMetadataLookupS32(NULL, chip->concepts, "CHIP.XSIZE");
            ySize = psMetadataLookupS32(NULL, chip->concepts, "CHIP.YSIZE");
            xParity = psMetadataLookupS32(NULL, chip->concepts, "CHIP.XPARITY");
            yParity = psMetadataLookupS32(NULL, chip->concepts, "CHIP.YPARITY");
            x0 = psMetadataLookupS32(NULL, chip->concepts, "CHIP.X0");
            y0 = psMetadataLookupS32(NULL, chip->concepts, "CHIP.Y0");
            id = psMetadataLookupStr(NULL, chip->concepts, "CHIP.ID");
        } else {
            if (xSize != psMetadataLookupS32(NULL, chip->concepts, "CHIP.XSIZE")) {
                psWarning("Differing CHIP.XSIZE in use: %d vs %d\n",
                    xSize, psMetadataLookupS32(NULL, chip->concepts, "CHIP.XSIZE"));
            }
            if (ySize != psMetadataLookupS32(NULL, chip->concepts, "CHIP.YSIZE")) {
                psWarning("Differing CHIP.YSIZE in use: %d vs %d\n",
                    ySize, psMetadataLookupS32(NULL, chip->concepts, "CHIP.YSIZE"));
            }
            if (xParity != psMetadataLookupS32(NULL, chip->concepts, "CHIP.XPARITY")) {
                psWarning("Differing CHIP.XPARITY in use: %d vs %d\n",
                    xParity, psMetadataLookupS32(NULL, chip->concepts, "CHIP.XPARITY"));
            }
            if (yParity != psMetadataLookupS32(NULL, chip->concepts, "CHIP.YPARITY")) {
                psWarning("Differing CHIP.YPARITY in use: %d vs %d\n",
                    yParity, psMetadataLookupS32(NULL, chip->concepts, "CHIP.YPARITY"));
            }
            if (x0 != psMetadataLookupS32(NULL, chip->concepts, "CHIP.X0")) {
                psWarning("Differing CHIP.X0 in use: %d vs %d\n",
                    x0, psMetadataLookupS32(NULL, chip->concepts, "CHIP.X0"));
            }
            if (y0 != psMetadataLookupS32(NULL, chip->concepts, "CHIP.Y0")) {
                psWarning("Differing CHIP.Y0 in use: %d vs %d\n",
                    y0, psMetadataLookupS32(NULL, chip->concepts, "CHIP.Y0"));
            }
            psString newID = psMetadataLookupStr(NULL, chip->concepts, "CHIP.ID");
            if (id && newID && strcmp(id, newID)) {
                psWarning("Differing CHIP.ID in use: %s vs %s\n", id, newID);
            }
        }

        nChips++;
    }
    psFree(sourcesIter);

    temp /= (float)nChips;
    seeing /= (float)nChips;

    MD_UPDATE(target->concepts, "CHIP.TEMP", F32, temp);
    MD_UPDATE(target->concepts, "CHIP.SEEING", F32, seeing);
    if (same) {
        MD_UPDATE(target->concepts, "CHIP.X0", S32, x0);
        MD_UPDATE(target->concepts, "CHIP.Y0", S32, y0);
        MD_UPDATE(target->concepts, "CHIP.XSIZE", S32, xSize);
        MD_UPDATE(target->concepts, "CHIP.YSIZE", S32, ySize);
        MD_UPDATE(target->concepts, "CHIP.XPARITY", S32, xParity);
        MD_UPDATE(target->concepts, "CHIP.YPARITY", S32, yParity);
        MD_UPDATE_STR(target->concepts, "CHIP.ID", id);
    }

    return true;
}

