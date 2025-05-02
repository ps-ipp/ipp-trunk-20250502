#include <pslib.h>
#include <ippdb.h>
#include <stdlib.h>

#define TMP_FILENAME "./blargh"

int main ()
{
    {
        psDB            *dbh;
        psFits          *fits;

        dbh = psDBInit("localhost", "test", NULL, "test");
        if (!dbh) {
            exit(EXIT_FAILURE);
        }

        fits = psFitsOpen(TMP_FILENAME, "w");
        if (!fits) {
            exit(EXIT_FAILURE);
        }

        if (!pzDataStoreSelectRowsFits(dbh, fits, NULL, 1)) {
            exit(EXIT_FAILURE);
        }

        psFree(fits);
        psDBCleanup(dbh);
    }

    {
        psDB            *dbh;
        psFits          *fits;

        dbh = psDBInit("localhost", "test", NULL, "test");
        if (!dbh) {
            exit(EXIT_FAILURE);
        }

        fits = psFitsOpen(TMP_FILENAME, "w");
        if (!fits) {
            exit(EXIT_FAILURE);
        }

        if (!summitExpSelectRowsFits(dbh, fits, NULL, 1)) {
            exit(EXIT_FAILURE);
        }

        psFree(fits);
        psDBCleanup(dbh);
    }

    {
        psDB            *dbh;
        psFits          *fits;

        dbh = psDBInit("localhost", "test", NULL, "test");
        if (!dbh) {
            exit(EXIT_FAILURE);
        }

        fits = psFitsOpen(TMP_FILENAME, "w");
        if (!fits) {
            exit(EXIT_FAILURE);
        }

        if (!summitImfileSelectRowsFits(dbh, fits, NULL, 1)) {
            exit(EXIT_FAILURE);
        }

        psFree(fits);
        psDBCleanup(dbh);
    }

    {
        psDB            *dbh;
        psFits          *fits;

        dbh = psDBInit("localhost", "test", NULL, "test");
        if (!dbh) {
            exit(EXIT_FAILURE);
        }

        fits = psFitsOpen(TMP_FILENAME, "w");
        if (!fits) {
            exit(EXIT_FAILURE);
        }

        if (!pzDownloadExpSelectRowsFits(dbh, fits, NULL, 1)) {
            exit(EXIT_FAILURE);
        }

        psFree(fits);
        psDBCleanup(dbh);
    }

    {
        psDB            *dbh;
        psFits          *fits;

        dbh = psDBInit("localhost", "test", NULL, "test");
        if (!dbh) {
            exit(EXIT_FAILURE);
        }

        fits = psFitsOpen(TMP_FILENAME, "w");
        if (!fits) {
            exit(EXIT_FAILURE);
        }

        if (!pzDownloadImfileSelectRowsFits(dbh, fits, NULL, 1)) {
            exit(EXIT_FAILURE);
        }

        psFree(fits);
        psDBCleanup(dbh);
    }

    {
        psDB            *dbh;
        psFits          *fits;

        dbh = psDBInit("localhost", "test", NULL, "test");
        if (!dbh) {
            exit(EXIT_FAILURE);
        }

        fits = psFitsOpen(TMP_FILENAME, "w");
        if (!fits) {
            exit(EXIT_FAILURE);
        }

        if (!newExpSelectRowsFits(dbh, fits, NULL, 1)) {
            exit(EXIT_FAILURE);
        }

        psFree(fits);
        psDBCleanup(dbh);
    }

    {
        psDB            *dbh;
        psFits          *fits;

        dbh = psDBInit("localhost", "test", NULL, "test");
        if (!dbh) {
            exit(EXIT_FAILURE);
        }

        fits = psFitsOpen(TMP_FILENAME, "w");
        if (!fits) {
            exit(EXIT_FAILURE);
        }

        if (!newImfileSelectRowsFits(dbh, fits, NULL, 1)) {
            exit(EXIT_FAILURE);
        }

        psFree(fits);
        psDBCleanup(dbh);
    }

    {
        psDB            *dbh;
        psFits          *fits;

        dbh = psDBInit("localhost", "test", NULL, "test");
        if (!dbh) {
            exit(EXIT_FAILURE);
        }

        fits = psFitsOpen(TMP_FILENAME, "w");
        if (!fits) {
            exit(EXIT_FAILURE);
        }

        if (!rawExpSelectRowsFits(dbh, fits, NULL, 1)) {
            exit(EXIT_FAILURE);
        }

        psFree(fits);
        psDBCleanup(dbh);
    }

    {
        psDB            *dbh;
        psFits          *fits;

        dbh = psDBInit("localhost", "test", NULL, "test");
        if (!dbh) {
            exit(EXIT_FAILURE);
        }

        fits = psFitsOpen(TMP_FILENAME, "w");
        if (!fits) {
            exit(EXIT_FAILURE);
        }

        if (!rawImfileSelectRowsFits(dbh, fits, NULL, 1)) {
            exit(EXIT_FAILURE);
        }

        psFree(fits);
        psDBCleanup(dbh);
    }

    {
        psDB            *dbh;
        psFits          *fits;

        dbh = psDBInit("localhost", "test", NULL, "test");
        if (!dbh) {
            exit(EXIT_FAILURE);
        }

        fits = psFitsOpen(TMP_FILENAME, "w");
        if (!fits) {
            exit(EXIT_FAILURE);
        }

        if (!guidePendingExpSelectRowsFits(dbh, fits, NULL, 1)) {
            exit(EXIT_FAILURE);
        }

        psFree(fits);
        psDBCleanup(dbh);
    }

    {
        psDB            *dbh;
        psFits          *fits;

        dbh = psDBInit("localhost", "test", NULL, "test");
        if (!dbh) {
            exit(EXIT_FAILURE);
        }

        fits = psFitsOpen(TMP_FILENAME, "w");
        if (!fits) {
            exit(EXIT_FAILURE);
        }

        if (!chipRunSelectRowsFits(dbh, fits, NULL, 1)) {
            exit(EXIT_FAILURE);
        }

        psFree(fits);
        psDBCleanup(dbh);
    }

    {
        psDB            *dbh;
        psFits          *fits;

        dbh = psDBInit("localhost", "test", NULL, "test");
        if (!dbh) {
            exit(EXIT_FAILURE);
        }

        fits = psFitsOpen(TMP_FILENAME, "w");
        if (!fits) {
            exit(EXIT_FAILURE);
        }

        if (!chipProcessedImfileSelectRowsFits(dbh, fits, NULL, 1)) {
            exit(EXIT_FAILURE);
        }

        psFree(fits);
        psDBCleanup(dbh);
    }

    {
        psDB            *dbh;
        psFits          *fits;

        dbh = psDBInit("localhost", "test", NULL, "test");
        if (!dbh) {
            exit(EXIT_FAILURE);
        }

        fits = psFitsOpen(TMP_FILENAME, "w");
        if (!fits) {
            exit(EXIT_FAILURE);
        }

        if (!chipMaskSelectRowsFits(dbh, fits, NULL, 1)) {
            exit(EXIT_FAILURE);
        }

        psFree(fits);
        psDBCleanup(dbh);
    }

    {
        psDB            *dbh;
        psFits          *fits;

        dbh = psDBInit("localhost", "test", NULL, "test");
        if (!dbh) {
            exit(EXIT_FAILURE);
        }

        fits = psFitsOpen(TMP_FILENAME, "w");
        if (!fits) {
            exit(EXIT_FAILURE);
        }

        if (!camRunSelectRowsFits(dbh, fits, NULL, 1)) {
            exit(EXIT_FAILURE);
        }

        psFree(fits);
        psDBCleanup(dbh);
    }

    {
        psDB            *dbh;
        psFits          *fits;

        dbh = psDBInit("localhost", "test", NULL, "test");
        if (!dbh) {
            exit(EXIT_FAILURE);
        }

        fits = psFitsOpen(TMP_FILENAME, "w");
        if (!fits) {
            exit(EXIT_FAILURE);
        }

        if (!camProcessedExpSelectRowsFits(dbh, fits, NULL, 1)) {
            exit(EXIT_FAILURE);
        }

        psFree(fits);
        psDBCleanup(dbh);
    }

    {
        psDB            *dbh;
        psFits          *fits;

        dbh = psDBInit("localhost", "test", NULL, "test");
        if (!dbh) {
            exit(EXIT_FAILURE);
        }

        fits = psFitsOpen(TMP_FILENAME, "w");
        if (!fits) {
            exit(EXIT_FAILURE);
        }

        if (!camMaskSelectRowsFits(dbh, fits, NULL, 1)) {
            exit(EXIT_FAILURE);
        }

        psFree(fits);
        psDBCleanup(dbh);
    }

    {
        psDB            *dbh;
        psFits          *fits;

        dbh = psDBInit("localhost", "test", NULL, "test");
        if (!dbh) {
            exit(EXIT_FAILURE);
        }

        fits = psFitsOpen(TMP_FILENAME, "w");
        if (!fits) {
            exit(EXIT_FAILURE);
        }

        if (!fakeRunSelectRowsFits(dbh, fits, NULL, 1)) {
            exit(EXIT_FAILURE);
        }

        psFree(fits);
        psDBCleanup(dbh);
    }

    {
        psDB            *dbh;
        psFits          *fits;

        dbh = psDBInit("localhost", "test", NULL, "test");
        if (!dbh) {
            exit(EXIT_FAILURE);
        }

        fits = psFitsOpen(TMP_FILENAME, "w");
        if (!fits) {
            exit(EXIT_FAILURE);
        }

        if (!fakeProcessedImfileSelectRowsFits(dbh, fits, NULL, 1)) {
            exit(EXIT_FAILURE);
        }

        psFree(fits);
        psDBCleanup(dbh);
    }

    {
        psDB            *dbh;
        psFits          *fits;

        dbh = psDBInit("localhost", "test", NULL, "test");
        if (!dbh) {
            exit(EXIT_FAILURE);
        }

        fits = psFitsOpen(TMP_FILENAME, "w");
        if (!fits) {
            exit(EXIT_FAILURE);
        }

        if (!fakeMaskSelectRowsFits(dbh, fits, NULL, 1)) {
            exit(EXIT_FAILURE);
        }

        psFree(fits);
        psDBCleanup(dbh);
    }

    {
        psDB            *dbh;
        psFits          *fits;

        dbh = psDBInit("localhost", "test", NULL, "test");
        if (!dbh) {
            exit(EXIT_FAILURE);
        }

        fits = psFitsOpen(TMP_FILENAME, "w");
        if (!fits) {
            exit(EXIT_FAILURE);
        }

        if (!warpRunSelectRowsFits(dbh, fits, NULL, 1)) {
            exit(EXIT_FAILURE);
        }

        psFree(fits);
        psDBCleanup(dbh);
    }

    {
        psDB            *dbh;
        psFits          *fits;

        dbh = psDBInit("localhost", "test", NULL, "test");
        if (!dbh) {
            exit(EXIT_FAILURE);
        }

        fits = psFitsOpen(TMP_FILENAME, "w");
        if (!fits) {
            exit(EXIT_FAILURE);
        }

        if (!warpSkyCellMapSelectRowsFits(dbh, fits, NULL, 1)) {
            exit(EXIT_FAILURE);
        }

        psFree(fits);
        psDBCleanup(dbh);
    }

    {
        psDB            *dbh;
        psFits          *fits;

        dbh = psDBInit("localhost", "test", NULL, "test");
        if (!dbh) {
            exit(EXIT_FAILURE);
        }

        fits = psFitsOpen(TMP_FILENAME, "w");
        if (!fits) {
            exit(EXIT_FAILURE);
        }

        if (!warpSkyfileSelectRowsFits(dbh, fits, NULL, 1)) {
            exit(EXIT_FAILURE);
        }

        psFree(fits);
        psDBCleanup(dbh);
    }

    {
        psDB            *dbh;
        psFits          *fits;

        dbh = psDBInit("localhost", "test", NULL, "test");
        if (!dbh) {
            exit(EXIT_FAILURE);
        }

        fits = psFitsOpen(TMP_FILENAME, "w");
        if (!fits) {
            exit(EXIT_FAILURE);
        }

        if (!warpMaskSelectRowsFits(dbh, fits, NULL, 1)) {
            exit(EXIT_FAILURE);
        }

        psFree(fits);
        psDBCleanup(dbh);
    }

    {
        psDB            *dbh;
        psFits          *fits;

        dbh = psDBInit("localhost", "test", NULL, "test");
        if (!dbh) {
            exit(EXIT_FAILURE);
        }

        fits = psFitsOpen(TMP_FILENAME, "w");
        if (!fits) {
            exit(EXIT_FAILURE);
        }

        if (!diffRunSelectRowsFits(dbh, fits, NULL, 1)) {
            exit(EXIT_FAILURE);
        }

        psFree(fits);
        psDBCleanup(dbh);
    }

    {
        psDB            *dbh;
        psFits          *fits;

        dbh = psDBInit("localhost", "test", NULL, "test");
        if (!dbh) {
            exit(EXIT_FAILURE);
        }

        fits = psFitsOpen(TMP_FILENAME, "w");
        if (!fits) {
            exit(EXIT_FAILURE);
        }

        if (!diffInputSkyfileSelectRowsFits(dbh, fits, NULL, 1)) {
            exit(EXIT_FAILURE);
        }

        psFree(fits);
        psDBCleanup(dbh);
    }

    {
        psDB            *dbh;
        psFits          *fits;

        dbh = psDBInit("localhost", "test", NULL, "test");
        if (!dbh) {
            exit(EXIT_FAILURE);
        }

        fits = psFitsOpen(TMP_FILENAME, "w");
        if (!fits) {
            exit(EXIT_FAILURE);
        }

        if (!diffSkyfileSelectRowsFits(dbh, fits, NULL, 1)) {
            exit(EXIT_FAILURE);
        }

        psFree(fits);
        psDBCleanup(dbh);
    }

    {
        psDB            *dbh;
        psFits          *fits;

        dbh = psDBInit("localhost", "test", NULL, "test");
        if (!dbh) {
            exit(EXIT_FAILURE);
        }

        fits = psFitsOpen(TMP_FILENAME, "w");
        if (!fits) {
            exit(EXIT_FAILURE);
        }

        if (!stackRunSelectRowsFits(dbh, fits, NULL, 1)) {
            exit(EXIT_FAILURE);
        }

        psFree(fits);
        psDBCleanup(dbh);
    }

    {
        psDB            *dbh;
        psFits          *fits;

        dbh = psDBInit("localhost", "test", NULL, "test");
        if (!dbh) {
            exit(EXIT_FAILURE);
        }

        fits = psFitsOpen(TMP_FILENAME, "w");
        if (!fits) {
            exit(EXIT_FAILURE);
        }

        if (!stackInputSkyfileSelectRowsFits(dbh, fits, NULL, 1)) {
            exit(EXIT_FAILURE);
        }

        psFree(fits);
        psDBCleanup(dbh);
    }

    {
        psDB            *dbh;
        psFits          *fits;

        dbh = psDBInit("localhost", "test", NULL, "test");
        if (!dbh) {
            exit(EXIT_FAILURE);
        }

        fits = psFitsOpen(TMP_FILENAME, "w");
        if (!fits) {
            exit(EXIT_FAILURE);
        }

        if (!stackSumSkyfileSelectRowsFits(dbh, fits, NULL, 1)) {
            exit(EXIT_FAILURE);
        }

        psFree(fits);
        psDBCleanup(dbh);
    }

    {
        psDB            *dbh;
        psFits          *fits;

        dbh = psDBInit("localhost", "test", NULL, "test");
        if (!dbh) {
            exit(EXIT_FAILURE);
        }

        fits = psFitsOpen(TMP_FILENAME, "w");
        if (!fits) {
            exit(EXIT_FAILURE);
        }

        if (!detRunSelectRowsFits(dbh, fits, NULL, 1)) {
            exit(EXIT_FAILURE);
        }

        psFree(fits);
        psDBCleanup(dbh);
    }

    {
        psDB            *dbh;
        psFits          *fits;

        dbh = psDBInit("localhost", "test", NULL, "test");
        if (!dbh) {
            exit(EXIT_FAILURE);
        }

        fits = psFitsOpen(TMP_FILENAME, "w");
        if (!fits) {
            exit(EXIT_FAILURE);
        }

        if (!detInputExpSelectRowsFits(dbh, fits, NULL, 1)) {
            exit(EXIT_FAILURE);
        }

        psFree(fits);
        psDBCleanup(dbh);
    }

    {
        psDB            *dbh;
        psFits          *fits;

        dbh = psDBInit("localhost", "test", NULL, "test");
        if (!dbh) {
            exit(EXIT_FAILURE);
        }

        fits = psFitsOpen(TMP_FILENAME, "w");
        if (!fits) {
            exit(EXIT_FAILURE);
        }

        if (!detProcessedImfileSelectRowsFits(dbh, fits, NULL, 1)) {
            exit(EXIT_FAILURE);
        }

        psFree(fits);
        psDBCleanup(dbh);
    }

    {
        psDB            *dbh;
        psFits          *fits;

        dbh = psDBInit("localhost", "test", NULL, "test");
        if (!dbh) {
            exit(EXIT_FAILURE);
        }

        fits = psFitsOpen(TMP_FILENAME, "w");
        if (!fits) {
            exit(EXIT_FAILURE);
        }

        if (!detProcessedExpSelectRowsFits(dbh, fits, NULL, 1)) {
            exit(EXIT_FAILURE);
        }

        psFree(fits);
        psDBCleanup(dbh);
    }

    {
        psDB            *dbh;
        psFits          *fits;

        dbh = psDBInit("localhost", "test", NULL, "test");
        if (!dbh) {
            exit(EXIT_FAILURE);
        }

        fits = psFitsOpen(TMP_FILENAME, "w");
        if (!fits) {
            exit(EXIT_FAILURE);
        }

        if (!detStackedImfileSelectRowsFits(dbh, fits, NULL, 1)) {
            exit(EXIT_FAILURE);
        }

        psFree(fits);
        psDBCleanup(dbh);
    }

    {
        psDB            *dbh;
        psFits          *fits;

        dbh = psDBInit("localhost", "test", NULL, "test");
        if (!dbh) {
            exit(EXIT_FAILURE);
        }

        fits = psFitsOpen(TMP_FILENAME, "w");
        if (!fits) {
            exit(EXIT_FAILURE);
        }

        if (!detNormalizedStatImfileSelectRowsFits(dbh, fits, NULL, 1)) {
            exit(EXIT_FAILURE);
        }

        psFree(fits);
        psDBCleanup(dbh);
    }

    {
        psDB            *dbh;
        psFits          *fits;

        dbh = psDBInit("localhost", "test", NULL, "test");
        if (!dbh) {
            exit(EXIT_FAILURE);
        }

        fits = psFitsOpen(TMP_FILENAME, "w");
        if (!fits) {
            exit(EXIT_FAILURE);
        }

        if (!detNormalizedImfileSelectRowsFits(dbh, fits, NULL, 1)) {
            exit(EXIT_FAILURE);
        }

        psFree(fits);
        psDBCleanup(dbh);
    }

    {
        psDB            *dbh;
        psFits          *fits;

        dbh = psDBInit("localhost", "test", NULL, "test");
        if (!dbh) {
            exit(EXIT_FAILURE);
        }

        fits = psFitsOpen(TMP_FILENAME, "w");
        if (!fits) {
            exit(EXIT_FAILURE);
        }

        if (!detNormalizedExpSelectRowsFits(dbh, fits, NULL, 1)) {
            exit(EXIT_FAILURE);
        }

        psFree(fits);
        psDBCleanup(dbh);
    }

    {
        psDB            *dbh;
        psFits          *fits;

        dbh = psDBInit("localhost", "test", NULL, "test");
        if (!dbh) {
            exit(EXIT_FAILURE);
        }

        fits = psFitsOpen(TMP_FILENAME, "w");
        if (!fits) {
            exit(EXIT_FAILURE);
        }

        if (!detResidImfileSelectRowsFits(dbh, fits, NULL, 1)) {
            exit(EXIT_FAILURE);
        }

        psFree(fits);
        psDBCleanup(dbh);
    }

    {
        psDB            *dbh;
        psFits          *fits;

        dbh = psDBInit("localhost", "test", NULL, "test");
        if (!dbh) {
            exit(EXIT_FAILURE);
        }

        fits = psFitsOpen(TMP_FILENAME, "w");
        if (!fits) {
            exit(EXIT_FAILURE);
        }

        if (!detResidExpSelectRowsFits(dbh, fits, NULL, 1)) {
            exit(EXIT_FAILURE);
        }

        psFree(fits);
        psDBCleanup(dbh);
    }

    {
        psDB            *dbh;
        psFits          *fits;

        dbh = psDBInit("localhost", "test", NULL, "test");
        if (!dbh) {
            exit(EXIT_FAILURE);
        }

        fits = psFitsOpen(TMP_FILENAME, "w");
        if (!fits) {
            exit(EXIT_FAILURE);
        }

        if (!detRunSummarySelectRowsFits(dbh, fits, NULL, 1)) {
            exit(EXIT_FAILURE);
        }

        psFree(fits);
        psDBCleanup(dbh);
    }

    {
        psDB            *dbh;
        psFits          *fits;

        dbh = psDBInit("localhost", "test", NULL, "test");
        if (!dbh) {
            exit(EXIT_FAILURE);
        }

        fits = psFitsOpen(TMP_FILENAME, "w");
        if (!fits) {
            exit(EXIT_FAILURE);
        }

        if (!detRegisteredImfileSelectRowsFits(dbh, fits, NULL, 1)) {
            exit(EXIT_FAILURE);
        }

        psFree(fits);
        psDBCleanup(dbh);
    }

    {
        psDB            *dbh;
        psFits          *fits;

        dbh = psDBInit("localhost", "test", NULL, "test");
        if (!dbh) {
            exit(EXIT_FAILURE);
        }

        fits = psFitsOpen(TMP_FILENAME, "w");
        if (!fits) {
            exit(EXIT_FAILURE);
        }

        if (!detCorrectedExpSelectRowsFits(dbh, fits, NULL, 1)) {
            exit(EXIT_FAILURE);
        }

        psFree(fits);
        psDBCleanup(dbh);
    }

    {
        psDB            *dbh;
        psFits          *fits;

        dbh = psDBInit("localhost", "test", NULL, "test");
        if (!dbh) {
            exit(EXIT_FAILURE);
        }

        fits = psFitsOpen(TMP_FILENAME, "w");
        if (!fits) {
            exit(EXIT_FAILURE);
        }

        if (!detCorrectedImfileSelectRowsFits(dbh, fits, NULL, 1)) {
            exit(EXIT_FAILURE);
        }

        psFree(fits);
        psDBCleanup(dbh);
    }

    {
        psDB            *dbh;
        psFits          *fits;

        dbh = psDBInit("localhost", "test", NULL, "test");
        if (!dbh) {
            exit(EXIT_FAILURE);
        }

        fits = psFitsOpen(TMP_FILENAME, "w");
        if (!fits) {
            exit(EXIT_FAILURE);
        }

        if (!magicRunSelectRowsFits(dbh, fits, NULL, 1)) {
            exit(EXIT_FAILURE);
        }

        psFree(fits);
        psDBCleanup(dbh);
    }

    {
        psDB            *dbh;
        psFits          *fits;

        dbh = psDBInit("localhost", "test", NULL, "test");
        if (!dbh) {
            exit(EXIT_FAILURE);
        }

        fits = psFitsOpen(TMP_FILENAME, "w");
        if (!fits) {
            exit(EXIT_FAILURE);
        }

        if (!magicInputSkyfileSelectRowsFits(dbh, fits, NULL, 1)) {
            exit(EXIT_FAILURE);
        }

        psFree(fits);
        psDBCleanup(dbh);
    }

    {
        psDB            *dbh;
        psFits          *fits;

        dbh = psDBInit("localhost", "test", NULL, "test");
        if (!dbh) {
            exit(EXIT_FAILURE);
        }

        fits = psFitsOpen(TMP_FILENAME, "w");
        if (!fits) {
            exit(EXIT_FAILURE);
        }

        if (!magicTreeSelectRowsFits(dbh, fits, NULL, 1)) {
            exit(EXIT_FAILURE);
        }

        psFree(fits);
        psDBCleanup(dbh);
    }

    {
        psDB            *dbh;
        psFits          *fits;

        dbh = psDBInit("localhost", "test", NULL, "test");
        if (!dbh) {
            exit(EXIT_FAILURE);
        }

        fits = psFitsOpen(TMP_FILENAME, "w");
        if (!fits) {
            exit(EXIT_FAILURE);
        }

        if (!magicNodeResultSelectRowsFits(dbh, fits, NULL, 1)) {
            exit(EXIT_FAILURE);
        }

        psFree(fits);
        psDBCleanup(dbh);
    }

    {
        psDB            *dbh;
        psFits          *fits;

        dbh = psDBInit("localhost", "test", NULL, "test");
        if (!dbh) {
            exit(EXIT_FAILURE);
        }

        fits = psFitsOpen(TMP_FILENAME, "w");
        if (!fits) {
            exit(EXIT_FAILURE);
        }

        if (!magicMaskSelectRowsFits(dbh, fits, NULL, 1)) {
            exit(EXIT_FAILURE);
        }

        psFree(fits);
        psDBCleanup(dbh);
    }

    {
        psDB            *dbh;
        psFits          *fits;

        dbh = psDBInit("localhost", "test", NULL, "test");
        if (!dbh) {
            exit(EXIT_FAILURE);
        }

        fits = psFitsOpen(TMP_FILENAME, "w");
        if (!fits) {
            exit(EXIT_FAILURE);
        }

        if (!calDBSelectRowsFits(dbh, fits, NULL, 1)) {
            exit(EXIT_FAILURE);
        }

        psFree(fits);
        psDBCleanup(dbh);
    }

    {
        psDB            *dbh;
        psFits          *fits;

        dbh = psDBInit("localhost", "test", NULL, "test");
        if (!dbh) {
            exit(EXIT_FAILURE);
        }

        fits = psFitsOpen(TMP_FILENAME, "w");
        if (!fits) {
            exit(EXIT_FAILURE);
        }

        if (!calRunSelectRowsFits(dbh, fits, NULL, 1)) {
            exit(EXIT_FAILURE);
        }

        psFree(fits);
        psDBCleanup(dbh);
    }

    {
        psDB            *dbh;
        psFits          *fits;

        dbh = psDBInit("localhost", "test", NULL, "test");
        if (!dbh) {
            exit(EXIT_FAILURE);
        }

        fits = psFitsOpen(TMP_FILENAME, "w");
        if (!fits) {
            exit(EXIT_FAILURE);
        }

        if (!flatcorrRunSelectRowsFits(dbh, fits, NULL, 1)) {
            exit(EXIT_FAILURE);
        }

        psFree(fits);
        psDBCleanup(dbh);
    }

    {
        psDB            *dbh;
        psFits          *fits;

        dbh = psDBInit("localhost", "test", NULL, "test");
        if (!dbh) {
            exit(EXIT_FAILURE);
        }

        fits = psFitsOpen(TMP_FILENAME, "w");
        if (!fits) {
            exit(EXIT_FAILURE);
        }

        if (!flatcorrChipLinkSelectRowsFits(dbh, fits, NULL, 1)) {
            exit(EXIT_FAILURE);
        }

        psFree(fits);
        psDBCleanup(dbh);
    }

    {
        psDB            *dbh;
        psFits          *fits;

        dbh = psDBInit("localhost", "test", NULL, "test");
        if (!dbh) {
            exit(EXIT_FAILURE);
        }

        fits = psFitsOpen(TMP_FILENAME, "w");
        if (!fits) {
            exit(EXIT_FAILURE);
        }

        if (!flatcorrCamLinkSelectRowsFits(dbh, fits, NULL, 1)) {
            exit(EXIT_FAILURE);
        }

        psFree(fits);
        psDBCleanup(dbh);
    }

    {
        psDB            *dbh;
        psFits          *fits;

        dbh = psDBInit("localhost", "test", NULL, "test");
        if (!dbh) {
            exit(EXIT_FAILURE);
        }

        fits = psFitsOpen(TMP_FILENAME, "w");
        if (!fits) {
            exit(EXIT_FAILURE);
        }

        if (!pstampDataStoreSelectRowsFits(dbh, fits, NULL, 1)) {
            exit(EXIT_FAILURE);
        }

        psFree(fits);
        psDBCleanup(dbh);
    }

    {
        psDB            *dbh;
        psFits          *fits;

        dbh = psDBInit("localhost", "test", NULL, "test");
        if (!dbh) {
            exit(EXIT_FAILURE);
        }

        fits = psFitsOpen(TMP_FILENAME, "w");
        if (!fits) {
            exit(EXIT_FAILURE);
        }

        if (!pstampProjectSelectRowsFits(dbh, fits, NULL, 1)) {
            exit(EXIT_FAILURE);
        }

        psFree(fits);
        psDBCleanup(dbh);
    }

    {
        psDB            *dbh;
        psFits          *fits;

        dbh = psDBInit("localhost", "test", NULL, "test");
        if (!dbh) {
            exit(EXIT_FAILURE);
        }

        fits = psFitsOpen(TMP_FILENAME, "w");
        if (!fits) {
            exit(EXIT_FAILURE);
        }

        if (!pstampRequestSelectRowsFits(dbh, fits, NULL, 1)) {
            exit(EXIT_FAILURE);
        }

        psFree(fits);
        psDBCleanup(dbh);
    }

    {
        psDB            *dbh;
        psFits          *fits;

        dbh = psDBInit("localhost", "test", NULL, "test");
        if (!dbh) {
            exit(EXIT_FAILURE);
        }

        fits = psFitsOpen(TMP_FILENAME, "w");
        if (!fits) {
            exit(EXIT_FAILURE);
        }

        if (!pstampJobSelectRowsFits(dbh, fits, NULL, 1)) {
            exit(EXIT_FAILURE);
        }

        psFree(fits);
        psDBCleanup(dbh);
    }

    exit(EXIT_SUCCESS);
}
