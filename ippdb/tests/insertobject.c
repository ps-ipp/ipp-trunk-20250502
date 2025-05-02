#include <pslib.h>
#include <ippdb.h>
#include <stdlib.h>

int main ()
{
    {
        psDB            *dbh;
        pzDataStoreRow  *object;

        dbh = psDBInit("localhost", "test", NULL, "test");
        if (!dbh) {
            exit(EXIT_FAILURE);
        }

        object = pzDataStoreRowAlloc("a string", "a string", "a string", "0001-01-01T00:00:00Z");
        if (!object) {
            exit(EXIT_FAILURE);
        }

        if (!pzDataStoreInsertObject(dbh, object)) {
            exit(EXIT_FAILURE);
        }

        psFree(object);
        psDBCleanup(dbh);
    }

    {
        psDB            *dbh;
        summitExpRow    *object;

        dbh = psDBInit("localhost", "test", NULL, "test");
        if (!dbh) {
            exit(EXIT_FAILURE);
        }

        object = summitExpRowAlloc("a string", "a string", "a string", "0001-01-01T00:00:00Z", "a string", "a string", -32, -16, "0001-01-01T00:00:00Z");
        if (!object) {
            exit(EXIT_FAILURE);
        }

        if (!summitExpInsertObject(dbh, object)) {
            exit(EXIT_FAILURE);
        }

        psFree(object);
        psDBCleanup(dbh);
    }

    {
        psDB            *dbh;
        summitImfileRow *object;

        dbh = psDBInit("localhost", "test", NULL, "test");
        if (!dbh) {
            exit(EXIT_FAILURE);
        }

        object = summitImfileRowAlloc("a string", "a string", "a string", "a string", -32, "a string", "a string", "a string", "a string", "0001-01-01T00:00:00Z");
        if (!object) {
            exit(EXIT_FAILURE);
        }

        if (!summitImfileInsertObject(dbh, object)) {
            exit(EXIT_FAILURE);
        }

        psFree(object);
        psDBCleanup(dbh);
    }

    {
        psDB            *dbh;
        pzDownloadExpRow *object;

        dbh = psDBInit("localhost", "test", NULL, "test");
        if (!dbh) {
            exit(EXIT_FAILURE);
        }

        object = pzDownloadExpRowAlloc("a string", "a string", "a string", "a string", "0001-01-01T00:00:00Z");
        if (!object) {
            exit(EXIT_FAILURE);
        }

        if (!pzDownloadExpInsertObject(dbh, object)) {
            exit(EXIT_FAILURE);
        }

        psFree(object);
        psDBCleanup(dbh);
    }

    {
        psDB            *dbh;
        pzDownloadImfileRow *object;

        dbh = psDBInit("localhost", "test", NULL, "test");
        if (!dbh) {
            exit(EXIT_FAILURE);
        }

        object = pzDownloadImfileRowAlloc("a string", "a string", "a string", "a string", "a string", "a string", -16, "0001-01-01T00:00:00Z");
        if (!object) {
            exit(EXIT_FAILURE);
        }

        if (!pzDownloadImfileInsertObject(dbh, object)) {
            exit(EXIT_FAILURE);
        }

        psFree(object);
        psDBCleanup(dbh);
    }

    {
        psDB            *dbh;
        newExpRow       *object;

        dbh = psDBInit("localhost", "test", NULL, "test");
        if (!dbh) {
            exit(EXIT_FAILURE);
        }

        object = newExpRowAlloc(-64, "a string", "a string", "a string", "a string", "a string", "a string", "a string", "a string", "a string", "a string", "a string", "0001-01-01T00:00:00Z");
        if (!object) {
            exit(EXIT_FAILURE);
        }

        if (!newExpInsertObject(dbh, object)) {
            exit(EXIT_FAILURE);
        }

        psFree(object);
        psDBCleanup(dbh);
    }

    {
        psDB            *dbh;
        newImfileRow    *object;

        dbh = psDBInit("localhost", "test", NULL, "test");
        if (!dbh) {
            exit(EXIT_FAILURE);
        }

        object = newImfileRowAlloc(-64, "a string", "a string", "0001-01-01T00:00:00Z");
        if (!object) {
            exit(EXIT_FAILURE);
        }

        if (!newImfileInsertObject(dbh, object)) {
            exit(EXIT_FAILURE);
        }

        psFree(object);
        psDBCleanup(dbh);
    }

    {
        psDB            *dbh;
        rawExpRow       *object;

        dbh = psDBInit("localhost", "test", NULL, "test");
        if (!dbh) {
            exit(EXIT_FAILURE);
        }

        object = rawExpRowAlloc(-64, "a string", "a string", "a string", "0001-01-01T00:00:00Z", "a string", "a string", "a string", "a string", "a string", "a string", "a string", "a string", "a string", "a string", "a string", "a string", 32.32, 64.64, 64.64, 32.32, 32.32, 64.64, 64.64, 64.64, 64.64, 64.64, 32.32, 64.64, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 64.64, 64.64, 64.64, 64.64, 64.64, "a string", 32.32, 32.32, 32.32, 32.32, 32.32, "a string", -16, "0001-01-01T00:00:00Z");
        if (!object) {
            exit(EXIT_FAILURE);
        }

        if (!rawExpInsertObject(dbh, object)) {
            exit(EXIT_FAILURE);
        }

        psFree(object);
        psDBCleanup(dbh);
    }

    {
        psDB            *dbh;
        rawImfileRow    *object;

        dbh = psDBInit("localhost", "test", NULL, "test");
        if (!dbh) {
            exit(EXIT_FAILURE);
        }

        object = rawImfileRowAlloc(-64, "a string", "a string", "a string", "0001-01-01T00:00:00Z", "a string", "a string", "a string", "a string", "a string", "a string", "a string", "a string", "a string", 32.32, 64.64, 64.64, 32.32, 32.32, 64.64, 64.64, 64.64, 64.64, 64.64, 32.32, 64.64, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 64.64, 64.64, 64.64, 64.64, 64.64, "a string", 32.32, 32.32, 32.32, 32.32, 32.32, "a string", -16, "0001-01-01T00:00:00Z");
        if (!object) {
            exit(EXIT_FAILURE);
        }

        if (!rawImfileInsertObject(dbh, object)) {
            exit(EXIT_FAILURE);
        }

        psFree(object);
        psDBCleanup(dbh);
    }

    {
        psDB            *dbh;
        guidePendingExpRow *object;

        dbh = psDBInit("localhost", "test", NULL, "test");
        if (!dbh) {
            exit(EXIT_FAILURE);
        }

        object = guidePendingExpRowAlloc(-64, -64, "a string");
        if (!object) {
            exit(EXIT_FAILURE);
        }

        if (!guidePendingExpInsertObject(dbh, object)) {
            exit(EXIT_FAILURE);
        }

        psFree(object);
        psDBCleanup(dbh);
    }

    {
        psDB            *dbh;
        chipRunRow      *object;

        dbh = psDBInit("localhost", "test", NULL, "test");
        if (!dbh) {
            exit(EXIT_FAILURE);
        }

        object = chipRunRowAlloc(-64, -64, "a string", "a string", "a string", "a string", "a string", "a string", "a string", "a string", "a string");
        if (!object) {
            exit(EXIT_FAILURE);
        }

        if (!chipRunInsertObject(dbh, object)) {
            exit(EXIT_FAILURE);
        }

        psFree(object);
        psDBCleanup(dbh);
    }

    {
        psDB            *dbh;
        chipProcessedImfileRow *object;

        dbh = psDBInit("localhost", "test", NULL, "test");
        if (!dbh) {
            exit(EXIT_FAILURE);
        }

        object = chipProcessedImfileRowAlloc(-64, -64, "a string", "a string", "a string", 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, "a string", -32, -32, -32, -32, -32, "a string", -16);
        if (!object) {
            exit(EXIT_FAILURE);
        }

        if (!chipProcessedImfileInsertObject(dbh, object)) {
            exit(EXIT_FAILURE);
        }

        psFree(object);
        psDBCleanup(dbh);
    }

    {
        psDB            *dbh;
        chipMaskRow     *object;

        dbh = psDBInit("localhost", "test", NULL, "test");
        if (!dbh) {
            exit(EXIT_FAILURE);
        }

        object = chipMaskRowAlloc("a string");
        if (!object) {
            exit(EXIT_FAILURE);
        }

        if (!chipMaskInsertObject(dbh, object)) {
            exit(EXIT_FAILURE);
        }

        psFree(object);
        psDBCleanup(dbh);
    }

    {
        psDB            *dbh;
        camRunRow       *object;

        dbh = psDBInit("localhost", "test", NULL, "test");
        if (!dbh) {
            exit(EXIT_FAILURE);
        }

        object = camRunRowAlloc(-64, -64, "a string", "a string", "a string", "a string", "a string", "a string", "a string", "a string", "a string");
        if (!object) {
            exit(EXIT_FAILURE);
        }

        if (!camRunInsertObject(dbh, object)) {
            exit(EXIT_FAILURE);
        }

        psFree(object);
        psDBCleanup(dbh);
    }

    {
        psDB            *dbh;
        camProcessedExpRow *object;

        dbh = psDBInit("localhost", "test", NULL, "test");
        if (!dbh) {
            exit(EXIT_FAILURE);
        }

        object = camProcessedExpRowAlloc(-64, "a string", 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, "a string", -32, -32, -32, -32, -32, -32, "a string", -16);
        if (!object) {
            exit(EXIT_FAILURE);
        }

        if (!camProcessedExpInsertObject(dbh, object)) {
            exit(EXIT_FAILURE);
        }

        psFree(object);
        psDBCleanup(dbh);
    }

    {
        psDB            *dbh;
        camMaskRow      *object;

        dbh = psDBInit("localhost", "test", NULL, "test");
        if (!dbh) {
            exit(EXIT_FAILURE);
        }

        object = camMaskRowAlloc("a string");
        if (!object) {
            exit(EXIT_FAILURE);
        }

        if (!camMaskInsertObject(dbh, object)) {
            exit(EXIT_FAILURE);
        }

        psFree(object);
        psDBCleanup(dbh);
    }

    {
        psDB            *dbh;
        fakeRunRow      *object;

        dbh = psDBInit("localhost", "test", NULL, "test");
        if (!dbh) {
            exit(EXIT_FAILURE);
        }

        object = fakeRunRowAlloc(-64, -64, "a string", "a string", "a string", "a string", "a string", "a string", "a string", "a string", "0001-01-01T00:00:00Z");
        if (!object) {
            exit(EXIT_FAILURE);
        }

        if (!fakeRunInsertObject(dbh, object)) {
            exit(EXIT_FAILURE);
        }

        psFree(object);
        psDBCleanup(dbh);
    }

    {
        psDB            *dbh;
        fakeProcessedImfileRow *object;

        dbh = psDBInit("localhost", "test", NULL, "test");
        if (!dbh) {
            exit(EXIT_FAILURE);
        }

        object = fakeProcessedImfileRowAlloc(-64, -64, "a string", "a string", 32.32, 32.32, "a string", "a string", "a string", -16, "0001-01-01T00:00:00Z");
        if (!object) {
            exit(EXIT_FAILURE);
        }

        if (!fakeProcessedImfileInsertObject(dbh, object)) {
            exit(EXIT_FAILURE);
        }

        psFree(object);
        psDBCleanup(dbh);
    }

    {
        psDB            *dbh;
        fakeMaskRow     *object;

        dbh = psDBInit("localhost", "test", NULL, "test");
        if (!dbh) {
            exit(EXIT_FAILURE);
        }

        object = fakeMaskRowAlloc("a string");
        if (!object) {
            exit(EXIT_FAILURE);
        }

        if (!fakeMaskInsertObject(dbh, object)) {
            exit(EXIT_FAILURE);
        }

        psFree(object);
        psDBCleanup(dbh);
    }

    {
        psDB            *dbh;
        warpRunRow      *object;

        dbh = psDBInit("localhost", "test", NULL, "test");
        if (!dbh) {
            exit(EXIT_FAILURE);
        }

        object = warpRunRowAlloc(-64, -64, "a string", "a string", "a string", "a string", "a string", "a string", "a string", "a string", "0001-01-01T00:00:00Z", true);
        if (!object) {
            exit(EXIT_FAILURE);
        }

        if (!warpRunInsertObject(dbh, object)) {
            exit(EXIT_FAILURE);
        }

        psFree(object);
        psDBCleanup(dbh);
    }

    {
        psDB            *dbh;
        warpSkyCellMapRow *object;

        dbh = psDBInit("localhost", "test", NULL, "test");
        if (!dbh) {
            exit(EXIT_FAILURE);
        }

        object = warpSkyCellMapRowAlloc(-64, "a string", "a string", "a string", -16);
        if (!object) {
            exit(EXIT_FAILURE);
        }

        if (!warpSkyCellMapInsertObject(dbh, object)) {
            exit(EXIT_FAILURE);
        }

        psFree(object);
        psDBCleanup(dbh);
    }

    {
        psDB            *dbh;
        warpSkyfileRow  *object;

        dbh = psDBInit("localhost", "test", NULL, "test");
        if (!dbh) {
            exit(EXIT_FAILURE);
        }

        object = warpSkyfileRowAlloc(-64, "a string", "a string", "a string", "a string", "a string", 64.64, 64.64, 32.32, 32.32, "a string", 32.32, -32, -32, -32, -32, true, -16);
        if (!object) {
            exit(EXIT_FAILURE);
        }

        if (!warpSkyfileInsertObject(dbh, object)) {
            exit(EXIT_FAILURE);
        }

        psFree(object);
        psDBCleanup(dbh);
    }

    {
        psDB            *dbh;
        warpMaskRow     *object;

        dbh = psDBInit("localhost", "test", NULL, "test");
        if (!dbh) {
            exit(EXIT_FAILURE);
        }

        object = warpMaskRowAlloc("a string");
        if (!object) {
            exit(EXIT_FAILURE);
        }

        if (!warpMaskInsertObject(dbh, object)) {
            exit(EXIT_FAILURE);
        }

        psFree(object);
        psDBCleanup(dbh);
    }

    {
        psDB            *dbh;
        diffRunRow      *object;

        dbh = psDBInit("localhost", "test", NULL, "test");
        if (!dbh) {
            exit(EXIT_FAILURE);
        }

        object = diffRunRowAlloc(-64, "a string", "a string", "a string", "a string", "a string", "0001-01-01T00:00:00Z", "a string", "a string");
        if (!object) {
            exit(EXIT_FAILURE);
        }

        if (!diffRunInsertObject(dbh, object)) {
            exit(EXIT_FAILURE);
        }

        psFree(object);
        psDBCleanup(dbh);
    }

    {
        psDB            *dbh;
        diffInputSkyfileRow *object;

        dbh = psDBInit("localhost", "test", NULL, "test");
        if (!dbh) {
            exit(EXIT_FAILURE);
        }

        object = diffInputSkyfileRowAlloc(-64, true, -64, -64, "a string", "a string", "a string");
        if (!object) {
            exit(EXIT_FAILURE);
        }

        if (!diffInputSkyfileInsertObject(dbh, object)) {
            exit(EXIT_FAILURE);
        }

        psFree(object);
        psDBCleanup(dbh);
    }

    {
        psDB            *dbh;
        diffSkyfileRow  *object;

        dbh = psDBInit("localhost", "test", NULL, "test");
        if (!dbh) {
            exit(EXIT_FAILURE);
        }

        object = diffSkyfileRowAlloc(-64, "a string", "a string", 64.64, 64.64, -32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, -32, 32.32, 32.32, 32.32, 32.32, "a string", 32.32, -16);
        if (!object) {
            exit(EXIT_FAILURE);
        }

        if (!diffSkyfileInsertObject(dbh, object)) {
            exit(EXIT_FAILURE);
        }

        psFree(object);
        psDBCleanup(dbh);
    }

    {
        psDB            *dbh;
        stackRunRow     *object;

        dbh = psDBInit("localhost", "test", NULL, "test");
        if (!dbh) {
            exit(EXIT_FAILURE);
        }

        object = stackRunRowAlloc(-64, "a string", "a string", "a string", "a string", "a string", "0001-01-01T00:00:00Z", "a string", "a string", "a string");
        if (!object) {
            exit(EXIT_FAILURE);
        }

        if (!stackRunInsertObject(dbh, object)) {
            exit(EXIT_FAILURE);
        }

        psFree(object);
        psDBCleanup(dbh);
    }

    {
        psDB            *dbh;
        stackInputSkyfileRow *object;

        dbh = psDBInit("localhost", "test", NULL, "test");
        if (!dbh) {
            exit(EXIT_FAILURE);
        }

        object = stackInputSkyfileRowAlloc(-64, -64);
        if (!object) {
            exit(EXIT_FAILURE);
        }

        if (!stackInputSkyfileInsertObject(dbh, object)) {
            exit(EXIT_FAILURE);
        }

        psFree(object);
        psDBCleanup(dbh);
    }

    {
        psDB            *dbh;
        stackSumSkyfileRow *object;

        dbh = psDBInit("localhost", "test", NULL, "test");
        if (!dbh) {
            exit(EXIT_FAILURE);
        }

        object = stackSumSkyfileRowAlloc(-64, "a string", "a string", 64.64, 64.64, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, -32, -32, 32.32, 32.32, -32, "a string", 32.32, -16);
        if (!object) {
            exit(EXIT_FAILURE);
        }

        if (!stackSumSkyfileInsertObject(dbh, object)) {
            exit(EXIT_FAILURE);
        }

        psFree(object);
        psDBCleanup(dbh);
    }

    {
        psDB            *dbh;
        detRunRow       *object;

        dbh = psDBInit("localhost", "test", NULL, "test");
        if (!dbh) {
            exit(EXIT_FAILURE);
        }

        object = detRunRowAlloc(-64, -32, "a string", "a string", "a string", "a string", "a string", "a string", "a string", "a string", "a string", "a string", 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 64.64, 64.64, "0001-01-01T00:00:00Z", "0001-01-01T00:00:00Z", "0001-01-01T00:00:00Z", "0001-01-01T00:00:00Z", "0001-01-01T00:00:00Z", 32.32, 32.32, "a string", -64, -32);
        if (!object) {
            exit(EXIT_FAILURE);
        }

        if (!detRunInsertObject(dbh, object)) {
            exit(EXIT_FAILURE);
        }

        psFree(object);
        psDBCleanup(dbh);
    }

    {
        psDB            *dbh;
        detInputExpRow  *object;

        dbh = psDBInit("localhost", "test", NULL, "test");
        if (!dbh) {
            exit(EXIT_FAILURE);
        }

        object = detInputExpRowAlloc(-64, -32, -64, true);
        if (!object) {
            exit(EXIT_FAILURE);
        }

        if (!detInputExpInsertObject(dbh, object)) {
            exit(EXIT_FAILURE);
        }

        psFree(object);
        psDBCleanup(dbh);
    }

    {
        psDB            *dbh;
        detProcessedImfileRow *object;

        dbh = psDBInit("localhost", "test", NULL, "test");
        if (!dbh) {
            exit(EXIT_FAILURE);
        }

        object = detProcessedImfileRowAlloc(-64, -64, "a string", "a string", "a string", 64.64, 64.64, 64.64, 64.64, 64.64, 64.64, 64.64, 64.64, 64.64, 64.64, 64.64, "a string", "a string", -16);
        if (!object) {
            exit(EXIT_FAILURE);
        }

        if (!detProcessedImfileInsertObject(dbh, object)) {
            exit(EXIT_FAILURE);
        }

        psFree(object);
        psDBCleanup(dbh);
    }

    {
        psDB            *dbh;
        detProcessedExpRow *object;

        dbh = psDBInit("localhost", "test", NULL, "test");
        if (!dbh) {
            exit(EXIT_FAILURE);
        }

        object = detProcessedExpRowAlloc(-64, -64, "a string", 64.64, 64.64, 64.64, 64.64, 64.64, 64.64, 64.64, 64.64, 64.64, 64.64, 64.64, "a string", "a string", -16);
        if (!object) {
            exit(EXIT_FAILURE);
        }

        if (!detProcessedExpInsertObject(dbh, object)) {
            exit(EXIT_FAILURE);
        }

        psFree(object);
        psDBCleanup(dbh);
    }

    {
        psDB            *dbh;
        detStackedImfileRow *object;

        dbh = psDBInit("localhost", "test", NULL, "test");
        if (!dbh) {
            exit(EXIT_FAILURE);
        }

        object = detStackedImfileRowAlloc(-64, -32, "a string", "a string", "a string", 64.64, 64.64, 64.64, 64.64, 64.64, 64.64, 64.64, 64.64, "a string", -16);
        if (!object) {
            exit(EXIT_FAILURE);
        }

        if (!detStackedImfileInsertObject(dbh, object)) {
            exit(EXIT_FAILURE);
        }

        psFree(object);
        psDBCleanup(dbh);
    }

    {
        psDB            *dbh;
        detNormalizedStatImfileRow *object;

        dbh = psDBInit("localhost", "test", NULL, "test");
        if (!dbh) {
            exit(EXIT_FAILURE);
        }

        object = detNormalizedStatImfileRowAlloc(-64, -32, "a string", 32.32, "a string", -16);
        if (!object) {
            exit(EXIT_FAILURE);
        }

        if (!detNormalizedStatImfileInsertObject(dbh, object)) {
            exit(EXIT_FAILURE);
        }

        psFree(object);
        psDBCleanup(dbh);
    }

    {
        psDB            *dbh;
        detNormalizedImfileRow *object;

        dbh = psDBInit("localhost", "test", NULL, "test");
        if (!dbh) {
            exit(EXIT_FAILURE);
        }

        object = detNormalizedImfileRowAlloc(-64, -32, "a string", "a string", 64.64, 64.64, 64.64, 64.64, 64.64, 64.64, 64.64, 64.64, "a string", "a string", -16);
        if (!object) {
            exit(EXIT_FAILURE);
        }

        if (!detNormalizedImfileInsertObject(dbh, object)) {
            exit(EXIT_FAILURE);
        }

        psFree(object);
        psDBCleanup(dbh);
    }

    {
        psDB            *dbh;
        detNormalizedExpRow *object;

        dbh = psDBInit("localhost", "test", NULL, "test");
        if (!dbh) {
            exit(EXIT_FAILURE);
        }

        object = detNormalizedExpRowAlloc(-64, -32, "a string", 64.64, 64.64, 64.64, 64.64, 64.64, 64.64, 64.64, 64.64, "a string", "a string", -16);
        if (!object) {
            exit(EXIT_FAILURE);
        }

        if (!detNormalizedExpInsertObject(dbh, object)) {
            exit(EXIT_FAILURE);
        }

        psFree(object);
        psDBCleanup(dbh);
    }

    {
        psDB            *dbh;
        detResidImfileRow *object;

        dbh = psDBInit("localhost", "test", NULL, "test");
        if (!dbh) {
            exit(EXIT_FAILURE);
        }

        object = detResidImfileRowAlloc(-64, -32, -64, -32, -64, "a string", "a string", "a string", 64.64, 64.64, 64.64, 64.64, 64.64, 64.64, 64.64, 64.64, 64.64, 64.64, 64.64, 64.64, 64.64, 64.64, 64.64, 64.64, 64.64, "a string", "a string", -16);
        if (!object) {
            exit(EXIT_FAILURE);
        }

        if (!detResidImfileInsertObject(dbh, object)) {
            exit(EXIT_FAILURE);
        }

        psFree(object);
        psDBCleanup(dbh);
    }

    {
        psDB            *dbh;
        detResidExpRow  *object;

        dbh = psDBInit("localhost", "test", NULL, "test");
        if (!dbh) {
            exit(EXIT_FAILURE);
        }

        object = detResidExpRowAlloc(-64, -32, -64, "a string", 64.64, 64.64, 64.64, 64.64, 64.64, 64.64, 64.64, 64.64, 64.64, 64.64, 64.64, 64.64, 64.64, 64.64, 64.64, 64.64, 64.64, "a string", "a string", true, -16);
        if (!object) {
            exit(EXIT_FAILURE);
        }

        if (!detResidExpInsertObject(dbh, object)) {
            exit(EXIT_FAILURE);
        }

        psFree(object);
        psDBCleanup(dbh);
    }

    {
        psDB            *dbh;
        detRunSummaryRow *object;

        dbh = psDBInit("localhost", "test", NULL, "test");
        if (!dbh) {
            exit(EXIT_FAILURE);
        }

        object = detRunSummaryRowAlloc(-64, -32, "a string", 64.64, 64.64, 64.64, true, -16);
        if (!object) {
            exit(EXIT_FAILURE);
        }

        if (!detRunSummaryInsertObject(dbh, object)) {
            exit(EXIT_FAILURE);
        }

        psFree(object);
        psDBCleanup(dbh);
    }

    {
        psDB            *dbh;
        detRegisteredImfileRow *object;

        dbh = psDBInit("localhost", "test", NULL, "test");
        if (!dbh) {
            exit(EXIT_FAILURE);
        }

        object = detRegisteredImfileRowAlloc(-64, -32, "a string", "a string", 64.64, 64.64, 64.64, 64.64, 64.64, 64.64, 64.64, 64.64, "a string", "a string", -16);
        if (!object) {
            exit(EXIT_FAILURE);
        }

        if (!detRegisteredImfileInsertObject(dbh, object)) {
            exit(EXIT_FAILURE);
        }

        psFree(object);
        psDBCleanup(dbh);
    }

    {
        psDB            *dbh;
        detCorrectedExpRow *object;

        dbh = psDBInit("localhost", "test", NULL, "test");
        if (!dbh) {
            exit(EXIT_FAILURE);
        }

        object = detCorrectedExpRowAlloc(-64, -64, "a string", -64, "a string", "a string", "a string", -16);
        if (!object) {
            exit(EXIT_FAILURE);
        }

        if (!detCorrectedExpInsertObject(dbh, object)) {
            exit(EXIT_FAILURE);
        }

        psFree(object);
        psDBCleanup(dbh);
    }

    {
        psDB            *dbh;
        detCorrectedImfileRow *object;

        dbh = psDBInit("localhost", "test", NULL, "test");
        if (!dbh) {
            exit(EXIT_FAILURE);
        }

        object = detCorrectedImfileRowAlloc(-64, -64, "a string", "a string", "a string", -16);
        if (!object) {
            exit(EXIT_FAILURE);
        }

        if (!detCorrectedImfileInsertObject(dbh, object)) {
            exit(EXIT_FAILURE);
        }

        psFree(object);
        psDBCleanup(dbh);
    }

    {
        psDB            *dbh;
        magicRunRow     *object;

        dbh = psDBInit("localhost", "test", NULL, "test");
        if (!dbh) {
            exit(EXIT_FAILURE);
        }

        object = magicRunRowAlloc(-64, -64, "a string", "a string", "a string", "a string", "a string", "0001-01-01T00:00:00Z", -16);
        if (!object) {
            exit(EXIT_FAILURE);
        }

        if (!magicRunInsertObject(dbh, object)) {
            exit(EXIT_FAILURE);
        }

        psFree(object);
        psDBCleanup(dbh);
    }

    {
        psDB            *dbh;
        magicInputSkyfileRow *object;

        dbh = psDBInit("localhost", "test", NULL, "test");
        if (!dbh) {
            exit(EXIT_FAILURE);
        }

        object = magicInputSkyfileRowAlloc(-64, -64, "a string");
        if (!object) {
            exit(EXIT_FAILURE);
        }

        if (!magicInputSkyfileInsertObject(dbh, object)) {
            exit(EXIT_FAILURE);
        }

        psFree(object);
        psDBCleanup(dbh);
    }

    {
        psDB            *dbh;
        magicTreeRow    *object;

        dbh = psDBInit("localhost", "test", NULL, "test");
        if (!dbh) {
            exit(EXIT_FAILURE);
        }

        object = magicTreeRowAlloc(-64, "a string", "a string");
        if (!object) {
            exit(EXIT_FAILURE);
        }

        if (!magicTreeInsertObject(dbh, object)) {
            exit(EXIT_FAILURE);
        }

        psFree(object);
        psDBCleanup(dbh);
    }

    {
        psDB            *dbh;
        magicNodeResultRow *object;

        dbh = psDBInit("localhost", "test", NULL, "test");
        if (!dbh) {
            exit(EXIT_FAILURE);
        }

        object = magicNodeResultRowAlloc(-64, "a string", "a string", -16);
        if (!object) {
            exit(EXIT_FAILURE);
        }

        if (!magicNodeResultInsertObject(dbh, object)) {
            exit(EXIT_FAILURE);
        }

        psFree(object);
        psDBCleanup(dbh);
    }

    {
        psDB            *dbh;
        magicMaskRow    *object;

        dbh = psDBInit("localhost", "test", NULL, "test");
        if (!dbh) {
            exit(EXIT_FAILURE);
        }

        object = magicMaskRowAlloc(-64, "a string", -32, -16);
        if (!object) {
            exit(EXIT_FAILURE);
        }

        if (!magicMaskInsertObject(dbh, object)) {
            exit(EXIT_FAILURE);
        }

        psFree(object);
        psDBCleanup(dbh);
    }

    {
        psDB            *dbh;
        calDBRow        *object;

        dbh = psDBInit("localhost", "test", NULL, "test");
        if (!dbh) {
            exit(EXIT_FAILURE);
        }

        object = calDBRowAlloc(-64, "a string", "a string");
        if (!object) {
            exit(EXIT_FAILURE);
        }

        if (!calDBInsertObject(dbh, object)) {
            exit(EXIT_FAILURE);
        }

        psFree(object);
        psDBCleanup(dbh);
    }

    {
        psDB            *dbh;
        calRunRow       *object;

        dbh = psDBInit("localhost", "test", NULL, "test");
        if (!dbh) {
            exit(EXIT_FAILURE);
        }

        object = calRunRowAlloc(-64, "a string", "a string", "a string");
        if (!object) {
            exit(EXIT_FAILURE);
        }

        if (!calRunInsertObject(dbh, object)) {
            exit(EXIT_FAILURE);
        }

        psFree(object);
        psDBCleanup(dbh);
    }

    {
        psDB            *dbh;
        flatcorrRunRow  *object;

        dbh = psDBInit("localhost", "test", NULL, "test");
        if (!dbh) {
            exit(EXIT_FAILURE);
        }

        object = flatcorrRunRowAlloc(-64, "a string", "a string", "a string", "a string", "a string", "a string", "a string", "a string", -16);
        if (!object) {
            exit(EXIT_FAILURE);
        }

        if (!flatcorrRunInsertObject(dbh, object)) {
            exit(EXIT_FAILURE);
        }

        psFree(object);
        psDBCleanup(dbh);
    }

    {
        psDB            *dbh;
        flatcorrChipLinkRow *object;

        dbh = psDBInit("localhost", "test", NULL, "test");
        if (!dbh) {
            exit(EXIT_FAILURE);
        }

        object = flatcorrChipLinkRowAlloc(-64, -64);
        if (!object) {
            exit(EXIT_FAILURE);
        }

        if (!flatcorrChipLinkInsertObject(dbh, object)) {
            exit(EXIT_FAILURE);
        }

        psFree(object);
        psDBCleanup(dbh);
    }

    {
        psDB            *dbh;
        flatcorrCamLinkRow *object;

        dbh = psDBInit("localhost", "test", NULL, "test");
        if (!dbh) {
            exit(EXIT_FAILURE);
        }

        object = flatcorrCamLinkRowAlloc(-64, -64, -64);
        if (!object) {
            exit(EXIT_FAILURE);
        }

        if (!flatcorrCamLinkInsertObject(dbh, object)) {
            exit(EXIT_FAILURE);
        }

        psFree(object);
        psDBCleanup(dbh);
    }

    {
        psDB            *dbh;
        pstampDataStoreRow *object;

        dbh = psDBInit("localhost", "test", NULL, "test");
        if (!dbh) {
            exit(EXIT_FAILURE);
        }

        object = pstampDataStoreRowAlloc(-64, "a string", "a string", "a string", "a string");
        if (!object) {
            exit(EXIT_FAILURE);
        }

        if (!pstampDataStoreInsertObject(dbh, object)) {
            exit(EXIT_FAILURE);
        }

        psFree(object);
        psDBCleanup(dbh);
    }

    {
        psDB            *dbh;
        pstampProjectRow *object;

        dbh = psDBInit("localhost", "test", NULL, "test");
        if (!dbh) {
            exit(EXIT_FAILURE);
        }

        object = pstampProjectRowAlloc(-64, "a string", "a string", "a string", "a string", "a string", "a string", true);
        if (!object) {
            exit(EXIT_FAILURE);
        }

        if (!pstampProjectInsertObject(dbh, object)) {
            exit(EXIT_FAILURE);
        }

        psFree(object);
        psDBCleanup(dbh);
    }

    {
        psDB            *dbh;
        pstampRequestRow *object;

        dbh = psDBInit("localhost", "test", NULL, "test");
        if (!dbh) {
            exit(EXIT_FAILURE);
        }

        object = pstampRequestRowAlloc(-64, -64, "a string", "a string", "a string", "a string", "a string", -32);
        if (!object) {
            exit(EXIT_FAILURE);
        }

        if (!pstampRequestInsertObject(dbh, object)) {
            exit(EXIT_FAILURE);
        }

        psFree(object);
        psDBCleanup(dbh);
    }

    {
        psDB            *dbh;
        pstampJobRow    *object;

        dbh = psDBInit("localhost", "test", NULL, "test");
        if (!dbh) {
            exit(EXIT_FAILURE);
        }

        object = pstampJobRowAlloc(-64, -64, "a string", "a string", "a string", -32, "a string", -64, "a string", "a string");
        if (!object) {
            exit(EXIT_FAILURE);
        }

        if (!pstampJobInsertObject(dbh, object)) {
            exit(EXIT_FAILURE);
        }

        psFree(object);
        psDBCleanup(dbh);
    }

    exit(EXIT_SUCCESS);
}
