#include <pslib.h>
#include <ippdb.h>
#include <stdlib.h>

int main ()
{
    {
        psDB            *dbh;

        dbh = psDBInit("localhost", "test", NULL, "test");
        if (!dbh) {
            exit(EXIT_FAILURE);
        }

        if (!pzDataStoreInsert(dbh, "a string", "a string", "a string", "0001-01-01T00:00:00Z")) {
            exit(EXIT_FAILURE);
        }

        psDBCleanup(dbh);
    }

    {
        psDB            *dbh;

        dbh = psDBInit("localhost", "test", NULL, "test");
        if (!dbh) {
            exit(EXIT_FAILURE);
        }

        if (!summitExpInsert(dbh, "a string", "a string", "a string", "0001-01-01T00:00:00Z", "a string", "a string", -32, -16, "0001-01-01T00:00:00Z")) {
            exit(EXIT_FAILURE);
        }

        psDBCleanup(dbh);
    }

    {
        psDB            *dbh;

        dbh = psDBInit("localhost", "test", NULL, "test");
        if (!dbh) {
            exit(EXIT_FAILURE);
        }

        if (!summitImfileInsert(dbh, "a string", "a string", "a string", "a string", -32, "a string", "a string", "a string", "a string", "0001-01-01T00:00:00Z")) {
            exit(EXIT_FAILURE);
        }

        psDBCleanup(dbh);
    }

    {
        psDB            *dbh;

        dbh = psDBInit("localhost", "test", NULL, "test");
        if (!dbh) {
            exit(EXIT_FAILURE);
        }

        if (!pzDownloadExpInsert(dbh, "a string", "a string", "a string", "a string", "0001-01-01T00:00:00Z")) {
            exit(EXIT_FAILURE);
        }

        psDBCleanup(dbh);
    }

    {
        psDB            *dbh;

        dbh = psDBInit("localhost", "test", NULL, "test");
        if (!dbh) {
            exit(EXIT_FAILURE);
        }

        if (!pzDownloadImfileInsert(dbh, "a string", "a string", "a string", "a string", "a string", "a string", -16, "0001-01-01T00:00:00Z")) {
            exit(EXIT_FAILURE);
        }

        psDBCleanup(dbh);
    }

    {
        psDB            *dbh;

        dbh = psDBInit("localhost", "test", NULL, "test");
        if (!dbh) {
            exit(EXIT_FAILURE);
        }

        if (!newExpInsert(dbh, -64, "a string", "a string", "a string", "a string", "a string", "a string", "a string", "a string", "a string", "a string", "a string", "0001-01-01T00:00:00Z")) {
            exit(EXIT_FAILURE);
        }

        psDBCleanup(dbh);
    }

    {
        psDB            *dbh;

        dbh = psDBInit("localhost", "test", NULL, "test");
        if (!dbh) {
            exit(EXIT_FAILURE);
        }

        if (!newImfileInsert(dbh, -64, "a string", "a string", "0001-01-01T00:00:00Z")) {
            exit(EXIT_FAILURE);
        }

        psDBCleanup(dbh);
    }

    {
        psDB            *dbh;

        dbh = psDBInit("localhost", "test", NULL, "test");
        if (!dbh) {
            exit(EXIT_FAILURE);
        }

        if (!rawExpInsert(dbh, -64, "a string", "a string", "a string", "0001-01-01T00:00:00Z", "a string", "a string", "a string", "a string", "a string", "a string", "a string", "a string", "a string", "a string", "a string", "a string", 32.32, 64.64, 64.64, 32.32, 32.32, 64.64, 64.64, 64.64, 64.64, 64.64, 32.32, 64.64, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 64.64, 64.64, 64.64, 64.64, 64.64, "a string", 32.32, 32.32, 32.32, 32.32, 32.32, "a string", -16, "0001-01-01T00:00:00Z")) {
            exit(EXIT_FAILURE);
        }

        psDBCleanup(dbh);
    }

    {
        psDB            *dbh;

        dbh = psDBInit("localhost", "test", NULL, "test");
        if (!dbh) {
            exit(EXIT_FAILURE);
        }

        if (!rawImfileInsert(dbh, -64, "a string", "a string", "a string", "0001-01-01T00:00:00Z", "a string", "a string", "a string", "a string", "a string", "a string", "a string", "a string", "a string", 32.32, 64.64, 64.64, 32.32, 32.32, 64.64, 64.64, 64.64, 64.64, 64.64, 32.32, 64.64, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 64.64, 64.64, 64.64, 64.64, 64.64, "a string", 32.32, 32.32, 32.32, 32.32, 32.32, "a string", -16, "0001-01-01T00:00:00Z")) {
            exit(EXIT_FAILURE);
        }

        psDBCleanup(dbh);
    }

    {
        psDB            *dbh;

        dbh = psDBInit("localhost", "test", NULL, "test");
        if (!dbh) {
            exit(EXIT_FAILURE);
        }

        if (!guidePendingExpInsert(dbh, -64, -64, "a string")) {
            exit(EXIT_FAILURE);
        }

        psDBCleanup(dbh);
    }

    {
        psDB            *dbh;

        dbh = psDBInit("localhost", "test", NULL, "test");
        if (!dbh) {
            exit(EXIT_FAILURE);
        }

        if (!chipRunInsert(dbh, -64, -64, "a string", "a string", "a string", "a string", "a string", "a string", "a string", "a string", "a string")) {
            exit(EXIT_FAILURE);
        }

        psDBCleanup(dbh);
    }

    {
        psDB            *dbh;

        dbh = psDBInit("localhost", "test", NULL, "test");
        if (!dbh) {
            exit(EXIT_FAILURE);
        }

        if (!chipProcessedImfileInsert(dbh, -64, -64, "a string", "a string", "a string", 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, "a string", -32, -32, -32, -32, -32, "a string", -16)) {
            exit(EXIT_FAILURE);
        }

        psDBCleanup(dbh);
    }

    {
        psDB            *dbh;

        dbh = psDBInit("localhost", "test", NULL, "test");
        if (!dbh) {
            exit(EXIT_FAILURE);
        }

        if (!chipMaskInsert(dbh, "a string")) {
            exit(EXIT_FAILURE);
        }

        psDBCleanup(dbh);
    }

    {
        psDB            *dbh;

        dbh = psDBInit("localhost", "test", NULL, "test");
        if (!dbh) {
            exit(EXIT_FAILURE);
        }

        if (!camRunInsert(dbh, -64, -64, "a string", "a string", "a string", "a string", "a string", "a string", "a string", "a string", "a string")) {
            exit(EXIT_FAILURE);
        }

        psDBCleanup(dbh);
    }

    {
        psDB            *dbh;

        dbh = psDBInit("localhost", "test", NULL, "test");
        if (!dbh) {
            exit(EXIT_FAILURE);
        }

        if (!camProcessedExpInsert(dbh, -64, "a string", 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, "a string", -32, -32, -32, -32, -32, -32, "a string", -16)) {
            exit(EXIT_FAILURE);
        }

        psDBCleanup(dbh);
    }

    {
        psDB            *dbh;

        dbh = psDBInit("localhost", "test", NULL, "test");
        if (!dbh) {
            exit(EXIT_FAILURE);
        }

        if (!camMaskInsert(dbh, "a string")) {
            exit(EXIT_FAILURE);
        }

        psDBCleanup(dbh);
    }

    {
        psDB            *dbh;

        dbh = psDBInit("localhost", "test", NULL, "test");
        if (!dbh) {
            exit(EXIT_FAILURE);
        }

        if (!fakeRunInsert(dbh, -64, -64, "a string", "a string", "a string", "a string", "a string", "a string", "a string", "a string", "0001-01-01T00:00:00Z")) {
            exit(EXIT_FAILURE);
        }

        psDBCleanup(dbh);
    }

    {
        psDB            *dbh;

        dbh = psDBInit("localhost", "test", NULL, "test");
        if (!dbh) {
            exit(EXIT_FAILURE);
        }

        if (!fakeProcessedImfileInsert(dbh, -64, -64, "a string", "a string", 32.32, 32.32, "a string", "a string", "a string", -16, "0001-01-01T00:00:00Z")) {
            exit(EXIT_FAILURE);
        }

        psDBCleanup(dbh);
    }

    {
        psDB            *dbh;

        dbh = psDBInit("localhost", "test", NULL, "test");
        if (!dbh) {
            exit(EXIT_FAILURE);
        }

        if (!fakeMaskInsert(dbh, "a string")) {
            exit(EXIT_FAILURE);
        }

        psDBCleanup(dbh);
    }

    {
        psDB            *dbh;

        dbh = psDBInit("localhost", "test", NULL, "test");
        if (!dbh) {
            exit(EXIT_FAILURE);
        }

        if (!warpRunInsert(dbh, -64, -64, "a string", "a string", "a string", "a string", "a string", "a string", "a string", "a string", "0001-01-01T00:00:00Z", true)) {
            exit(EXIT_FAILURE);
        }

        psDBCleanup(dbh);
    }

    {
        psDB            *dbh;

        dbh = psDBInit("localhost", "test", NULL, "test");
        if (!dbh) {
            exit(EXIT_FAILURE);
        }

        if (!warpSkyCellMapInsert(dbh, -64, "a string", "a string", "a string", -16)) {
            exit(EXIT_FAILURE);
        }

        psDBCleanup(dbh);
    }

    {
        psDB            *dbh;

        dbh = psDBInit("localhost", "test", NULL, "test");
        if (!dbh) {
            exit(EXIT_FAILURE);
        }

        if (!warpSkyfileInsert(dbh, -64, "a string", "a string", "a string", "a string", "a string", 64.64, 64.64, 32.32, 32.32, "a string", 32.32, -32, -32, -32, -32, true, -16)) {
            exit(EXIT_FAILURE);
        }

        psDBCleanup(dbh);
    }

    {
        psDB            *dbh;

        dbh = psDBInit("localhost", "test", NULL, "test");
        if (!dbh) {
            exit(EXIT_FAILURE);
        }

        if (!warpMaskInsert(dbh, "a string")) {
            exit(EXIT_FAILURE);
        }

        psDBCleanup(dbh);
    }

    {
        psDB            *dbh;

        dbh = psDBInit("localhost", "test", NULL, "test");
        if (!dbh) {
            exit(EXIT_FAILURE);
        }

        if (!diffRunInsert(dbh, -64, "a string", "a string", "a string", "a string", "a string", "0001-01-01T00:00:00Z", "a string", "a string")) {
            exit(EXIT_FAILURE);
        }

        psDBCleanup(dbh);
    }

    {
        psDB            *dbh;

        dbh = psDBInit("localhost", "test", NULL, "test");
        if (!dbh) {
            exit(EXIT_FAILURE);
        }

        if (!diffInputSkyfileInsert(dbh, -64, true, -64, -64, "a string", "a string", "a string")) {
            exit(EXIT_FAILURE);
        }

        psDBCleanup(dbh);
    }

    {
        psDB            *dbh;

        dbh = psDBInit("localhost", "test", NULL, "test");
        if (!dbh) {
            exit(EXIT_FAILURE);
        }

        if (!diffSkyfileInsert(dbh, -64, "a string", "a string", 64.64, 64.64, -32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, -32, 32.32, 32.32, 32.32, 32.32, "a string", 32.32, -16)) {
            exit(EXIT_FAILURE);
        }

        psDBCleanup(dbh);
    }

    {
        psDB            *dbh;

        dbh = psDBInit("localhost", "test", NULL, "test");
        if (!dbh) {
            exit(EXIT_FAILURE);
        }

        if (!stackRunInsert(dbh, -64, "a string", "a string", "a string", "a string", "a string", "0001-01-01T00:00:00Z", "a string", "a string", "a string")) {
            exit(EXIT_FAILURE);
        }

        psDBCleanup(dbh);
    }

    {
        psDB            *dbh;

        dbh = psDBInit("localhost", "test", NULL, "test");
        if (!dbh) {
            exit(EXIT_FAILURE);
        }

        if (!stackInputSkyfileInsert(dbh, -64, -64)) {
            exit(EXIT_FAILURE);
        }

        psDBCleanup(dbh);
    }

    {
        psDB            *dbh;

        dbh = psDBInit("localhost", "test", NULL, "test");
        if (!dbh) {
            exit(EXIT_FAILURE);
        }

        if (!stackSumSkyfileInsert(dbh, -64, "a string", "a string", 64.64, 64.64, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, -32, -32, 32.32, 32.32, -32, "a string", 32.32, -16)) {
            exit(EXIT_FAILURE);
        }

        psDBCleanup(dbh);
    }

    {
        psDB            *dbh;

        dbh = psDBInit("localhost", "test", NULL, "test");
        if (!dbh) {
            exit(EXIT_FAILURE);
        }

        if (!detRunInsert(dbh, -64, -32, "a string", "a string", "a string", "a string", "a string", "a string", "a string", "a string", "a string", "a string", 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 64.64, 64.64, "0001-01-01T00:00:00Z", "0001-01-01T00:00:00Z", "0001-01-01T00:00:00Z", "0001-01-01T00:00:00Z", "0001-01-01T00:00:00Z", 32.32, 32.32, "a string", -64, -32)) {
            exit(EXIT_FAILURE);
        }

        psDBCleanup(dbh);
    }

    {
        psDB            *dbh;

        dbh = psDBInit("localhost", "test", NULL, "test");
        if (!dbh) {
            exit(EXIT_FAILURE);
        }

        if (!detInputExpInsert(dbh, -64, -32, -64, true)) {
            exit(EXIT_FAILURE);
        }

        psDBCleanup(dbh);
    }

    {
        psDB            *dbh;

        dbh = psDBInit("localhost", "test", NULL, "test");
        if (!dbh) {
            exit(EXIT_FAILURE);
        }

        if (!detProcessedImfileInsert(dbh, -64, -64, "a string", "a string", "a string", 64.64, 64.64, 64.64, 64.64, 64.64, 64.64, 64.64, 64.64, 64.64, 64.64, 64.64, "a string", "a string", -16)) {
            exit(EXIT_FAILURE);
        }

        psDBCleanup(dbh);
    }

    {
        psDB            *dbh;

        dbh = psDBInit("localhost", "test", NULL, "test");
        if (!dbh) {
            exit(EXIT_FAILURE);
        }

        if (!detProcessedExpInsert(dbh, -64, -64, "a string", 64.64, 64.64, 64.64, 64.64, 64.64, 64.64, 64.64, 64.64, 64.64, 64.64, 64.64, "a string", "a string", -16)) {
            exit(EXIT_FAILURE);
        }

        psDBCleanup(dbh);
    }

    {
        psDB            *dbh;

        dbh = psDBInit("localhost", "test", NULL, "test");
        if (!dbh) {
            exit(EXIT_FAILURE);
        }

        if (!detStackedImfileInsert(dbh, -64, -32, "a string", "a string", "a string", 64.64, 64.64, 64.64, 64.64, 64.64, 64.64, 64.64, 64.64, "a string", -16)) {
            exit(EXIT_FAILURE);
        }

        psDBCleanup(dbh);
    }

    {
        psDB            *dbh;

        dbh = psDBInit("localhost", "test", NULL, "test");
        if (!dbh) {
            exit(EXIT_FAILURE);
        }

        if (!detNormalizedStatImfileInsert(dbh, -64, -32, "a string", 32.32, "a string", -16)) {
            exit(EXIT_FAILURE);
        }

        psDBCleanup(dbh);
    }

    {
        psDB            *dbh;

        dbh = psDBInit("localhost", "test", NULL, "test");
        if (!dbh) {
            exit(EXIT_FAILURE);
        }

        if (!detNormalizedImfileInsert(dbh, -64, -32, "a string", "a string", 64.64, 64.64, 64.64, 64.64, 64.64, 64.64, 64.64, 64.64, "a string", "a string", -16)) {
            exit(EXIT_FAILURE);
        }

        psDBCleanup(dbh);
    }

    {
        psDB            *dbh;

        dbh = psDBInit("localhost", "test", NULL, "test");
        if (!dbh) {
            exit(EXIT_FAILURE);
        }

        if (!detNormalizedExpInsert(dbh, -64, -32, "a string", 64.64, 64.64, 64.64, 64.64, 64.64, 64.64, 64.64, 64.64, "a string", "a string", -16)) {
            exit(EXIT_FAILURE);
        }

        psDBCleanup(dbh);
    }

    {
        psDB            *dbh;

        dbh = psDBInit("localhost", "test", NULL, "test");
        if (!dbh) {
            exit(EXIT_FAILURE);
        }

        if (!detResidImfileInsert(dbh, -64, -32, -64, -32, -64, "a string", "a string", "a string", 64.64, 64.64, 64.64, 64.64, 64.64, 64.64, 64.64, 64.64, 64.64, 64.64, 64.64, 64.64, 64.64, 64.64, 64.64, 64.64, 64.64, "a string", "a string", -16)) {
            exit(EXIT_FAILURE);
        }

        psDBCleanup(dbh);
    }

    {
        psDB            *dbh;

        dbh = psDBInit("localhost", "test", NULL, "test");
        if (!dbh) {
            exit(EXIT_FAILURE);
        }

        if (!detResidExpInsert(dbh, -64, -32, -64, "a string", 64.64, 64.64, 64.64, 64.64, 64.64, 64.64, 64.64, 64.64, 64.64, 64.64, 64.64, 64.64, 64.64, 64.64, 64.64, 64.64, 64.64, "a string", "a string", true, -16)) {
            exit(EXIT_FAILURE);
        }

        psDBCleanup(dbh);
    }

    {
        psDB            *dbh;

        dbh = psDBInit("localhost", "test", NULL, "test");
        if (!dbh) {
            exit(EXIT_FAILURE);
        }

        if (!detRunSummaryInsert(dbh, -64, -32, "a string", 64.64, 64.64, 64.64, true, -16)) {
            exit(EXIT_FAILURE);
        }

        psDBCleanup(dbh);
    }

    {
        psDB            *dbh;

        dbh = psDBInit("localhost", "test", NULL, "test");
        if (!dbh) {
            exit(EXIT_FAILURE);
        }

        if (!detRegisteredImfileInsert(dbh, -64, -32, "a string", "a string", 64.64, 64.64, 64.64, 64.64, 64.64, 64.64, 64.64, 64.64, "a string", "a string", -16)) {
            exit(EXIT_FAILURE);
        }

        psDBCleanup(dbh);
    }

    {
        psDB            *dbh;

        dbh = psDBInit("localhost", "test", NULL, "test");
        if (!dbh) {
            exit(EXIT_FAILURE);
        }

        if (!detCorrectedExpInsert(dbh, -64, -64, "a string", -64, "a string", "a string", "a string", -16)) {
            exit(EXIT_FAILURE);
        }

        psDBCleanup(dbh);
    }

    {
        psDB            *dbh;

        dbh = psDBInit("localhost", "test", NULL, "test");
        if (!dbh) {
            exit(EXIT_FAILURE);
        }

        if (!detCorrectedImfileInsert(dbh, -64, -64, "a string", "a string", "a string", -16)) {
            exit(EXIT_FAILURE);
        }

        psDBCleanup(dbh);
    }

    {
        psDB            *dbh;

        dbh = psDBInit("localhost", "test", NULL, "test");
        if (!dbh) {
            exit(EXIT_FAILURE);
        }

        if (!magicRunInsert(dbh, -64, -64, "a string", "a string", "a string", "a string", "a string", "0001-01-01T00:00:00Z", -16)) {
            exit(EXIT_FAILURE);
        }

        psDBCleanup(dbh);
    }

    {
        psDB            *dbh;

        dbh = psDBInit("localhost", "test", NULL, "test");
        if (!dbh) {
            exit(EXIT_FAILURE);
        }

        if (!magicInputSkyfileInsert(dbh, -64, -64, "a string")) {
            exit(EXIT_FAILURE);
        }

        psDBCleanup(dbh);
    }

    {
        psDB            *dbh;

        dbh = psDBInit("localhost", "test", NULL, "test");
        if (!dbh) {
            exit(EXIT_FAILURE);
        }

        if (!magicTreeInsert(dbh, -64, "a string", "a string")) {
            exit(EXIT_FAILURE);
        }

        psDBCleanup(dbh);
    }

    {
        psDB            *dbh;

        dbh = psDBInit("localhost", "test", NULL, "test");
        if (!dbh) {
            exit(EXIT_FAILURE);
        }

        if (!magicNodeResultInsert(dbh, -64, "a string", "a string", -16)) {
            exit(EXIT_FAILURE);
        }

        psDBCleanup(dbh);
    }

    {
        psDB            *dbh;

        dbh = psDBInit("localhost", "test", NULL, "test");
        if (!dbh) {
            exit(EXIT_FAILURE);
        }

        if (!magicMaskInsert(dbh, -64, "a string", -32, -16)) {
            exit(EXIT_FAILURE);
        }

        psDBCleanup(dbh);
    }

    {
        psDB            *dbh;

        dbh = psDBInit("localhost", "test", NULL, "test");
        if (!dbh) {
            exit(EXIT_FAILURE);
        }

        if (!calDBInsert(dbh, -64, "a string", "a string")) {
            exit(EXIT_FAILURE);
        }

        psDBCleanup(dbh);
    }

    {
        psDB            *dbh;

        dbh = psDBInit("localhost", "test", NULL, "test");
        if (!dbh) {
            exit(EXIT_FAILURE);
        }

        if (!calRunInsert(dbh, -64, "a string", "a string", "a string")) {
            exit(EXIT_FAILURE);
        }

        psDBCleanup(dbh);
    }

    {
        psDB            *dbh;

        dbh = psDBInit("localhost", "test", NULL, "test");
        if (!dbh) {
            exit(EXIT_FAILURE);
        }

        if (!flatcorrRunInsert(dbh, -64, "a string", "a string", "a string", "a string", "a string", "a string", "a string", "a string", -16)) {
            exit(EXIT_FAILURE);
        }

        psDBCleanup(dbh);
    }

    {
        psDB            *dbh;

        dbh = psDBInit("localhost", "test", NULL, "test");
        if (!dbh) {
            exit(EXIT_FAILURE);
        }

        if (!flatcorrChipLinkInsert(dbh, -64, -64)) {
            exit(EXIT_FAILURE);
        }

        psDBCleanup(dbh);
    }

    {
        psDB            *dbh;

        dbh = psDBInit("localhost", "test", NULL, "test");
        if (!dbh) {
            exit(EXIT_FAILURE);
        }

        if (!flatcorrCamLinkInsert(dbh, -64, -64, -64)) {
            exit(EXIT_FAILURE);
        }

        psDBCleanup(dbh);
    }

    {
        psDB            *dbh;

        dbh = psDBInit("localhost", "test", NULL, "test");
        if (!dbh) {
            exit(EXIT_FAILURE);
        }

        if (!pstampDataStoreInsert(dbh, -64, "a string", "a string", "a string", "a string")) {
            exit(EXIT_FAILURE);
        }

        psDBCleanup(dbh);
    }

    {
        psDB            *dbh;

        dbh = psDBInit("localhost", "test", NULL, "test");
        if (!dbh) {
            exit(EXIT_FAILURE);
        }

        if (!pstampProjectInsert(dbh, -64, "a string", "a string", "a string", "a string", "a string", "a string", true)) {
            exit(EXIT_FAILURE);
        }

        psDBCleanup(dbh);
    }

    {
        psDB            *dbh;

        dbh = psDBInit("localhost", "test", NULL, "test");
        if (!dbh) {
            exit(EXIT_FAILURE);
        }

        if (!pstampRequestInsert(dbh, -64, -64, "a string", "a string", "a string", "a string", "a string", -32)) {
            exit(EXIT_FAILURE);
        }

        psDBCleanup(dbh);
    }

    {
        psDB            *dbh;

        dbh = psDBInit("localhost", "test", NULL, "test");
        if (!dbh) {
            exit(EXIT_FAILURE);
        }

        if (!pstampJobInsert(dbh, -64, -64, "a string", "a string", "a string", -32, "a string", -64, "a string", "a string")) {
            exit(EXIT_FAILURE);
        }

        psDBCleanup(dbh);
    }

    exit(EXIT_SUCCESS);
}
