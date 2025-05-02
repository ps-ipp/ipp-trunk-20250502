#include <pslib.h>
#include <ippdb.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

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

        // open a temp
        fits = psFitsOpen(TMP_FILENAME, "r");
        if (!fits) {
            exit(EXIT_FAILURE);
        }

        if (!pzDataStoreInsertFits(dbh, fits)) {
            exit(EXIT_FAILURE);
        }

        if (!psFitsClose(fits)) {
            exit(EXIT_FAILURE);
        }

        psDBCleanup(dbh);
    }

    {
        psDB            *dbh;
        psFits          *fits;

        dbh = psDBInit("localhost", "test", NULL, "test");
        if (!dbh) {
            exit(EXIT_FAILURE);
        }

        // open a temp
        fits = psFitsOpen(TMP_FILENAME, "r");
        if (!fits) {
            exit(EXIT_FAILURE);
        }

        if (!summitExpInsertFits(dbh, fits)) {
            exit(EXIT_FAILURE);
        }

        if (!psFitsClose(fits)) {
            exit(EXIT_FAILURE);
        }

        psDBCleanup(dbh);
    }

    {
        psDB            *dbh;
        psFits          *fits;

        dbh = psDBInit("localhost", "test", NULL, "test");
        if (!dbh) {
            exit(EXIT_FAILURE);
        }

        // open a temp
        fits = psFitsOpen(TMP_FILENAME, "r");
        if (!fits) {
            exit(EXIT_FAILURE);
        }

        if (!summitImfileInsertFits(dbh, fits)) {
            exit(EXIT_FAILURE);
        }

        if (!psFitsClose(fits)) {
            exit(EXIT_FAILURE);
        }

        psDBCleanup(dbh);
    }

    {
        psDB            *dbh;
        psFits          *fits;

        dbh = psDBInit("localhost", "test", NULL, "test");
        if (!dbh) {
            exit(EXIT_FAILURE);
        }

        // open a temp
        fits = psFitsOpen(TMP_FILENAME, "r");
        if (!fits) {
            exit(EXIT_FAILURE);
        }

        if (!pzDownloadExpInsertFits(dbh, fits)) {
            exit(EXIT_FAILURE);
        }

        if (!psFitsClose(fits)) {
            exit(EXIT_FAILURE);
        }

        psDBCleanup(dbh);
    }

    {
        psDB            *dbh;
        psFits          *fits;

        dbh = psDBInit("localhost", "test", NULL, "test");
        if (!dbh) {
            exit(EXIT_FAILURE);
        }

        // open a temp
        fits = psFitsOpen(TMP_FILENAME, "r");
        if (!fits) {
            exit(EXIT_FAILURE);
        }

        if (!pzDownloadImfileInsertFits(dbh, fits)) {
            exit(EXIT_FAILURE);
        }

        if (!psFitsClose(fits)) {
            exit(EXIT_FAILURE);
        }

        psDBCleanup(dbh);
    }

    {
        psDB            *dbh;
        psFits          *fits;

        dbh = psDBInit("localhost", "test", NULL, "test");
        if (!dbh) {
            exit(EXIT_FAILURE);
        }

        // open a temp
        fits = psFitsOpen(TMP_FILENAME, "r");
        if (!fits) {
            exit(EXIT_FAILURE);
        }

        if (!newExpInsertFits(dbh, fits)) {
            exit(EXIT_FAILURE);
        }

        if (!psFitsClose(fits)) {
            exit(EXIT_FAILURE);
        }

        psDBCleanup(dbh);
    }

    {
        psDB            *dbh;
        psFits          *fits;

        dbh = psDBInit("localhost", "test", NULL, "test");
        if (!dbh) {
            exit(EXIT_FAILURE);
        }

        // open a temp
        fits = psFitsOpen(TMP_FILENAME, "r");
        if (!fits) {
            exit(EXIT_FAILURE);
        }

        if (!newImfileInsertFits(dbh, fits)) {
            exit(EXIT_FAILURE);
        }

        if (!psFitsClose(fits)) {
            exit(EXIT_FAILURE);
        }

        psDBCleanup(dbh);
    }

    {
        psDB            *dbh;
        psFits          *fits;

        dbh = psDBInit("localhost", "test", NULL, "test");
        if (!dbh) {
            exit(EXIT_FAILURE);
        }

        // open a temp
        fits = psFitsOpen(TMP_FILENAME, "r");
        if (!fits) {
            exit(EXIT_FAILURE);
        }

        if (!rawExpInsertFits(dbh, fits)) {
            exit(EXIT_FAILURE);
        }

        if (!psFitsClose(fits)) {
            exit(EXIT_FAILURE);
        }

        psDBCleanup(dbh);
    }

    {
        psDB            *dbh;
        psFits          *fits;

        dbh = psDBInit("localhost", "test", NULL, "test");
        if (!dbh) {
            exit(EXIT_FAILURE);
        }

        // open a temp
        fits = psFitsOpen(TMP_FILENAME, "r");
        if (!fits) {
            exit(EXIT_FAILURE);
        }

        if (!rawImfileInsertFits(dbh, fits)) {
            exit(EXIT_FAILURE);
        }

        if (!psFitsClose(fits)) {
            exit(EXIT_FAILURE);
        }

        psDBCleanup(dbh);
    }

    {
        psDB            *dbh;
        psFits          *fits;

        dbh = psDBInit("localhost", "test", NULL, "test");
        if (!dbh) {
            exit(EXIT_FAILURE);
        }

        // open a temp
        fits = psFitsOpen(TMP_FILENAME, "r");
        if (!fits) {
            exit(EXIT_FAILURE);
        }

        if (!guidePendingExpInsertFits(dbh, fits)) {
            exit(EXIT_FAILURE);
        }

        if (!psFitsClose(fits)) {
            exit(EXIT_FAILURE);
        }

        psDBCleanup(dbh);
    }

    {
        psDB            *dbh;
        psFits          *fits;

        dbh = psDBInit("localhost", "test", NULL, "test");
        if (!dbh) {
            exit(EXIT_FAILURE);
        }

        // open a temp
        fits = psFitsOpen(TMP_FILENAME, "r");
        if (!fits) {
            exit(EXIT_FAILURE);
        }

        if (!chipRunInsertFits(dbh, fits)) {
            exit(EXIT_FAILURE);
        }

        if (!psFitsClose(fits)) {
            exit(EXIT_FAILURE);
        }

        psDBCleanup(dbh);
    }

    {
        psDB            *dbh;
        psFits          *fits;

        dbh = psDBInit("localhost", "test", NULL, "test");
        if (!dbh) {
            exit(EXIT_FAILURE);
        }

        // open a temp
        fits = psFitsOpen(TMP_FILENAME, "r");
        if (!fits) {
            exit(EXIT_FAILURE);
        }

        if (!chipProcessedImfileInsertFits(dbh, fits)) {
            exit(EXIT_FAILURE);
        }

        if (!psFitsClose(fits)) {
            exit(EXIT_FAILURE);
        }

        psDBCleanup(dbh);
    }

    {
        psDB            *dbh;
        psFits          *fits;

        dbh = psDBInit("localhost", "test", NULL, "test");
        if (!dbh) {
            exit(EXIT_FAILURE);
        }

        // open a temp
        fits = psFitsOpen(TMP_FILENAME, "r");
        if (!fits) {
            exit(EXIT_FAILURE);
        }

        if (!chipMaskInsertFits(dbh, fits)) {
            exit(EXIT_FAILURE);
        }

        if (!psFitsClose(fits)) {
            exit(EXIT_FAILURE);
        }

        psDBCleanup(dbh);
    }

    {
        psDB            *dbh;
        psFits          *fits;

        dbh = psDBInit("localhost", "test", NULL, "test");
        if (!dbh) {
            exit(EXIT_FAILURE);
        }

        // open a temp
        fits = psFitsOpen(TMP_FILENAME, "r");
        if (!fits) {
            exit(EXIT_FAILURE);
        }

        if (!camRunInsertFits(dbh, fits)) {
            exit(EXIT_FAILURE);
        }

        if (!psFitsClose(fits)) {
            exit(EXIT_FAILURE);
        }

        psDBCleanup(dbh);
    }

    {
        psDB            *dbh;
        psFits          *fits;

        dbh = psDBInit("localhost", "test", NULL, "test");
        if (!dbh) {
            exit(EXIT_FAILURE);
        }

        // open a temp
        fits = psFitsOpen(TMP_FILENAME, "r");
        if (!fits) {
            exit(EXIT_FAILURE);
        }

        if (!camProcessedExpInsertFits(dbh, fits)) {
            exit(EXIT_FAILURE);
        }

        if (!psFitsClose(fits)) {
            exit(EXIT_FAILURE);
        }

        psDBCleanup(dbh);
    }

    {
        psDB            *dbh;
        psFits          *fits;

        dbh = psDBInit("localhost", "test", NULL, "test");
        if (!dbh) {
            exit(EXIT_FAILURE);
        }

        // open a temp
        fits = psFitsOpen(TMP_FILENAME, "r");
        if (!fits) {
            exit(EXIT_FAILURE);
        }

        if (!camMaskInsertFits(dbh, fits)) {
            exit(EXIT_FAILURE);
        }

        if (!psFitsClose(fits)) {
            exit(EXIT_FAILURE);
        }

        psDBCleanup(dbh);
    }

    {
        psDB            *dbh;
        psFits          *fits;

        dbh = psDBInit("localhost", "test", NULL, "test");
        if (!dbh) {
            exit(EXIT_FAILURE);
        }

        // open a temp
        fits = psFitsOpen(TMP_FILENAME, "r");
        if (!fits) {
            exit(EXIT_FAILURE);
        }

        if (!fakeRunInsertFits(dbh, fits)) {
            exit(EXIT_FAILURE);
        }

        if (!psFitsClose(fits)) {
            exit(EXIT_FAILURE);
        }

        psDBCleanup(dbh);
    }

    {
        psDB            *dbh;
        psFits          *fits;

        dbh = psDBInit("localhost", "test", NULL, "test");
        if (!dbh) {
            exit(EXIT_FAILURE);
        }

        // open a temp
        fits = psFitsOpen(TMP_FILENAME, "r");
        if (!fits) {
            exit(EXIT_FAILURE);
        }

        if (!fakeProcessedImfileInsertFits(dbh, fits)) {
            exit(EXIT_FAILURE);
        }

        if (!psFitsClose(fits)) {
            exit(EXIT_FAILURE);
        }

        psDBCleanup(dbh);
    }

    {
        psDB            *dbh;
        psFits          *fits;

        dbh = psDBInit("localhost", "test", NULL, "test");
        if (!dbh) {
            exit(EXIT_FAILURE);
        }

        // open a temp
        fits = psFitsOpen(TMP_FILENAME, "r");
        if (!fits) {
            exit(EXIT_FAILURE);
        }

        if (!fakeMaskInsertFits(dbh, fits)) {
            exit(EXIT_FAILURE);
        }

        if (!psFitsClose(fits)) {
            exit(EXIT_FAILURE);
        }

        psDBCleanup(dbh);
    }

    {
        psDB            *dbh;
        psFits          *fits;

        dbh = psDBInit("localhost", "test", NULL, "test");
        if (!dbh) {
            exit(EXIT_FAILURE);
        }

        // open a temp
        fits = psFitsOpen(TMP_FILENAME, "r");
        if (!fits) {
            exit(EXIT_FAILURE);
        }

        if (!warpRunInsertFits(dbh, fits)) {
            exit(EXIT_FAILURE);
        }

        if (!psFitsClose(fits)) {
            exit(EXIT_FAILURE);
        }

        psDBCleanup(dbh);
    }

    {
        psDB            *dbh;
        psFits          *fits;

        dbh = psDBInit("localhost", "test", NULL, "test");
        if (!dbh) {
            exit(EXIT_FAILURE);
        }

        // open a temp
        fits = psFitsOpen(TMP_FILENAME, "r");
        if (!fits) {
            exit(EXIT_FAILURE);
        }

        if (!warpSkyCellMapInsertFits(dbh, fits)) {
            exit(EXIT_FAILURE);
        }

        if (!psFitsClose(fits)) {
            exit(EXIT_FAILURE);
        }

        psDBCleanup(dbh);
    }

    {
        psDB            *dbh;
        psFits          *fits;

        dbh = psDBInit("localhost", "test", NULL, "test");
        if (!dbh) {
            exit(EXIT_FAILURE);
        }

        // open a temp
        fits = psFitsOpen(TMP_FILENAME, "r");
        if (!fits) {
            exit(EXIT_FAILURE);
        }

        if (!warpSkyfileInsertFits(dbh, fits)) {
            exit(EXIT_FAILURE);
        }

        if (!psFitsClose(fits)) {
            exit(EXIT_FAILURE);
        }

        psDBCleanup(dbh);
    }

    {
        psDB            *dbh;
        psFits          *fits;

        dbh = psDBInit("localhost", "test", NULL, "test");
        if (!dbh) {
            exit(EXIT_FAILURE);
        }

        // open a temp
        fits = psFitsOpen(TMP_FILENAME, "r");
        if (!fits) {
            exit(EXIT_FAILURE);
        }

        if (!warpMaskInsertFits(dbh, fits)) {
            exit(EXIT_FAILURE);
        }

        if (!psFitsClose(fits)) {
            exit(EXIT_FAILURE);
        }

        psDBCleanup(dbh);
    }

    {
        psDB            *dbh;
        psFits          *fits;

        dbh = psDBInit("localhost", "test", NULL, "test");
        if (!dbh) {
            exit(EXIT_FAILURE);
        }

        // open a temp
        fits = psFitsOpen(TMP_FILENAME, "r");
        if (!fits) {
            exit(EXIT_FAILURE);
        }

        if (!diffRunInsertFits(dbh, fits)) {
            exit(EXIT_FAILURE);
        }

        if (!psFitsClose(fits)) {
            exit(EXIT_FAILURE);
        }

        psDBCleanup(dbh);
    }

    {
        psDB            *dbh;
        psFits          *fits;

        dbh = psDBInit("localhost", "test", NULL, "test");
        if (!dbh) {
            exit(EXIT_FAILURE);
        }

        // open a temp
        fits = psFitsOpen(TMP_FILENAME, "r");
        if (!fits) {
            exit(EXIT_FAILURE);
        }

        if (!diffInputSkyfileInsertFits(dbh, fits)) {
            exit(EXIT_FAILURE);
        }

        if (!psFitsClose(fits)) {
            exit(EXIT_FAILURE);
        }

        psDBCleanup(dbh);
    }

    {
        psDB            *dbh;
        psFits          *fits;

        dbh = psDBInit("localhost", "test", NULL, "test");
        if (!dbh) {
            exit(EXIT_FAILURE);
        }

        // open a temp
        fits = psFitsOpen(TMP_FILENAME, "r");
        if (!fits) {
            exit(EXIT_FAILURE);
        }

        if (!diffSkyfileInsertFits(dbh, fits)) {
            exit(EXIT_FAILURE);
        }

        if (!psFitsClose(fits)) {
            exit(EXIT_FAILURE);
        }

        psDBCleanup(dbh);
    }

    {
        psDB            *dbh;
        psFits          *fits;

        dbh = psDBInit("localhost", "test", NULL, "test");
        if (!dbh) {
            exit(EXIT_FAILURE);
        }

        // open a temp
        fits = psFitsOpen(TMP_FILENAME, "r");
        if (!fits) {
            exit(EXIT_FAILURE);
        }

        if (!stackRunInsertFits(dbh, fits)) {
            exit(EXIT_FAILURE);
        }

        if (!psFitsClose(fits)) {
            exit(EXIT_FAILURE);
        }

        psDBCleanup(dbh);
    }

    {
        psDB            *dbh;
        psFits          *fits;

        dbh = psDBInit("localhost", "test", NULL, "test");
        if (!dbh) {
            exit(EXIT_FAILURE);
        }

        // open a temp
        fits = psFitsOpen(TMP_FILENAME, "r");
        if (!fits) {
            exit(EXIT_FAILURE);
        }

        if (!stackInputSkyfileInsertFits(dbh, fits)) {
            exit(EXIT_FAILURE);
        }

        if (!psFitsClose(fits)) {
            exit(EXIT_FAILURE);
        }

        psDBCleanup(dbh);
    }

    {
        psDB            *dbh;
        psFits          *fits;

        dbh = psDBInit("localhost", "test", NULL, "test");
        if (!dbh) {
            exit(EXIT_FAILURE);
        }

        // open a temp
        fits = psFitsOpen(TMP_FILENAME, "r");
        if (!fits) {
            exit(EXIT_FAILURE);
        }

        if (!stackSumSkyfileInsertFits(dbh, fits)) {
            exit(EXIT_FAILURE);
        }

        if (!psFitsClose(fits)) {
            exit(EXIT_FAILURE);
        }

        psDBCleanup(dbh);
    }

    {
        psDB            *dbh;
        psFits          *fits;

        dbh = psDBInit("localhost", "test", NULL, "test");
        if (!dbh) {
            exit(EXIT_FAILURE);
        }

        // open a temp
        fits = psFitsOpen(TMP_FILENAME, "r");
        if (!fits) {
            exit(EXIT_FAILURE);
        }

        if (!detRunInsertFits(dbh, fits)) {
            exit(EXIT_FAILURE);
        }

        if (!psFitsClose(fits)) {
            exit(EXIT_FAILURE);
        }

        psDBCleanup(dbh);
    }

    {
        psDB            *dbh;
        psFits          *fits;

        dbh = psDBInit("localhost", "test", NULL, "test");
        if (!dbh) {
            exit(EXIT_FAILURE);
        }

        // open a temp
        fits = psFitsOpen(TMP_FILENAME, "r");
        if (!fits) {
            exit(EXIT_FAILURE);
        }

        if (!detInputExpInsertFits(dbh, fits)) {
            exit(EXIT_FAILURE);
        }

        if (!psFitsClose(fits)) {
            exit(EXIT_FAILURE);
        }

        psDBCleanup(dbh);
    }

    {
        psDB            *dbh;
        psFits          *fits;

        dbh = psDBInit("localhost", "test", NULL, "test");
        if (!dbh) {
            exit(EXIT_FAILURE);
        }

        // open a temp
        fits = psFitsOpen(TMP_FILENAME, "r");
        if (!fits) {
            exit(EXIT_FAILURE);
        }

        if (!detProcessedImfileInsertFits(dbh, fits)) {
            exit(EXIT_FAILURE);
        }

        if (!psFitsClose(fits)) {
            exit(EXIT_FAILURE);
        }

        psDBCleanup(dbh);
    }

    {
        psDB            *dbh;
        psFits          *fits;

        dbh = psDBInit("localhost", "test", NULL, "test");
        if (!dbh) {
            exit(EXIT_FAILURE);
        }

        // open a temp
        fits = psFitsOpen(TMP_FILENAME, "r");
        if (!fits) {
            exit(EXIT_FAILURE);
        }

        if (!detProcessedExpInsertFits(dbh, fits)) {
            exit(EXIT_FAILURE);
        }

        if (!psFitsClose(fits)) {
            exit(EXIT_FAILURE);
        }

        psDBCleanup(dbh);
    }

    {
        psDB            *dbh;
        psFits          *fits;

        dbh = psDBInit("localhost", "test", NULL, "test");
        if (!dbh) {
            exit(EXIT_FAILURE);
        }

        // open a temp
        fits = psFitsOpen(TMP_FILENAME, "r");
        if (!fits) {
            exit(EXIT_FAILURE);
        }

        if (!detStackedImfileInsertFits(dbh, fits)) {
            exit(EXIT_FAILURE);
        }

        if (!psFitsClose(fits)) {
            exit(EXIT_FAILURE);
        }

        psDBCleanup(dbh);
    }

    {
        psDB            *dbh;
        psFits          *fits;

        dbh = psDBInit("localhost", "test", NULL, "test");
        if (!dbh) {
            exit(EXIT_FAILURE);
        }

        // open a temp
        fits = psFitsOpen(TMP_FILENAME, "r");
        if (!fits) {
            exit(EXIT_FAILURE);
        }

        if (!detNormalizedStatImfileInsertFits(dbh, fits)) {
            exit(EXIT_FAILURE);
        }

        if (!psFitsClose(fits)) {
            exit(EXIT_FAILURE);
        }

        psDBCleanup(dbh);
    }

    {
        psDB            *dbh;
        psFits          *fits;

        dbh = psDBInit("localhost", "test", NULL, "test");
        if (!dbh) {
            exit(EXIT_FAILURE);
        }

        // open a temp
        fits = psFitsOpen(TMP_FILENAME, "r");
        if (!fits) {
            exit(EXIT_FAILURE);
        }

        if (!detNormalizedImfileInsertFits(dbh, fits)) {
            exit(EXIT_FAILURE);
        }

        if (!psFitsClose(fits)) {
            exit(EXIT_FAILURE);
        }

        psDBCleanup(dbh);
    }

    {
        psDB            *dbh;
        psFits          *fits;

        dbh = psDBInit("localhost", "test", NULL, "test");
        if (!dbh) {
            exit(EXIT_FAILURE);
        }

        // open a temp
        fits = psFitsOpen(TMP_FILENAME, "r");
        if (!fits) {
            exit(EXIT_FAILURE);
        }

        if (!detNormalizedExpInsertFits(dbh, fits)) {
            exit(EXIT_FAILURE);
        }

        if (!psFitsClose(fits)) {
            exit(EXIT_FAILURE);
        }

        psDBCleanup(dbh);
    }

    {
        psDB            *dbh;
        psFits          *fits;

        dbh = psDBInit("localhost", "test", NULL, "test");
        if (!dbh) {
            exit(EXIT_FAILURE);
        }

        // open a temp
        fits = psFitsOpen(TMP_FILENAME, "r");
        if (!fits) {
            exit(EXIT_FAILURE);
        }

        if (!detResidImfileInsertFits(dbh, fits)) {
            exit(EXIT_FAILURE);
        }

        if (!psFitsClose(fits)) {
            exit(EXIT_FAILURE);
        }

        psDBCleanup(dbh);
    }

    {
        psDB            *dbh;
        psFits          *fits;

        dbh = psDBInit("localhost", "test", NULL, "test");
        if (!dbh) {
            exit(EXIT_FAILURE);
        }

        // open a temp
        fits = psFitsOpen(TMP_FILENAME, "r");
        if (!fits) {
            exit(EXIT_FAILURE);
        }

        if (!detResidExpInsertFits(dbh, fits)) {
            exit(EXIT_FAILURE);
        }

        if (!psFitsClose(fits)) {
            exit(EXIT_FAILURE);
        }

        psDBCleanup(dbh);
    }

    {
        psDB            *dbh;
        psFits          *fits;

        dbh = psDBInit("localhost", "test", NULL, "test");
        if (!dbh) {
            exit(EXIT_FAILURE);
        }

        // open a temp
        fits = psFitsOpen(TMP_FILENAME, "r");
        if (!fits) {
            exit(EXIT_FAILURE);
        }

        if (!detRunSummaryInsertFits(dbh, fits)) {
            exit(EXIT_FAILURE);
        }

        if (!psFitsClose(fits)) {
            exit(EXIT_FAILURE);
        }

        psDBCleanup(dbh);
    }

    {
        psDB            *dbh;
        psFits          *fits;

        dbh = psDBInit("localhost", "test", NULL, "test");
        if (!dbh) {
            exit(EXIT_FAILURE);
        }

        // open a temp
        fits = psFitsOpen(TMP_FILENAME, "r");
        if (!fits) {
            exit(EXIT_FAILURE);
        }

        if (!detRegisteredImfileInsertFits(dbh, fits)) {
            exit(EXIT_FAILURE);
        }

        if (!psFitsClose(fits)) {
            exit(EXIT_FAILURE);
        }

        psDBCleanup(dbh);
    }

    {
        psDB            *dbh;
        psFits          *fits;

        dbh = psDBInit("localhost", "test", NULL, "test");
        if (!dbh) {
            exit(EXIT_FAILURE);
        }

        // open a temp
        fits = psFitsOpen(TMP_FILENAME, "r");
        if (!fits) {
            exit(EXIT_FAILURE);
        }

        if (!detCorrectedExpInsertFits(dbh, fits)) {
            exit(EXIT_FAILURE);
        }

        if (!psFitsClose(fits)) {
            exit(EXIT_FAILURE);
        }

        psDBCleanup(dbh);
    }

    {
        psDB            *dbh;
        psFits          *fits;

        dbh = psDBInit("localhost", "test", NULL, "test");
        if (!dbh) {
            exit(EXIT_FAILURE);
        }

        // open a temp
        fits = psFitsOpen(TMP_FILENAME, "r");
        if (!fits) {
            exit(EXIT_FAILURE);
        }

        if (!detCorrectedImfileInsertFits(dbh, fits)) {
            exit(EXIT_FAILURE);
        }

        if (!psFitsClose(fits)) {
            exit(EXIT_FAILURE);
        }

        psDBCleanup(dbh);
    }

    {
        psDB            *dbh;
        psFits          *fits;

        dbh = psDBInit("localhost", "test", NULL, "test");
        if (!dbh) {
            exit(EXIT_FAILURE);
        }

        // open a temp
        fits = psFitsOpen(TMP_FILENAME, "r");
        if (!fits) {
            exit(EXIT_FAILURE);
        }

        if (!magicRunInsertFits(dbh, fits)) {
            exit(EXIT_FAILURE);
        }

        if (!psFitsClose(fits)) {
            exit(EXIT_FAILURE);
        }

        psDBCleanup(dbh);
    }

    {
        psDB            *dbh;
        psFits          *fits;

        dbh = psDBInit("localhost", "test", NULL, "test");
        if (!dbh) {
            exit(EXIT_FAILURE);
        }

        // open a temp
        fits = psFitsOpen(TMP_FILENAME, "r");
        if (!fits) {
            exit(EXIT_FAILURE);
        }

        if (!magicInputSkyfileInsertFits(dbh, fits)) {
            exit(EXIT_FAILURE);
        }

        if (!psFitsClose(fits)) {
            exit(EXIT_FAILURE);
        }

        psDBCleanup(dbh);
    }

    {
        psDB            *dbh;
        psFits          *fits;

        dbh = psDBInit("localhost", "test", NULL, "test");
        if (!dbh) {
            exit(EXIT_FAILURE);
        }

        // open a temp
        fits = psFitsOpen(TMP_FILENAME, "r");
        if (!fits) {
            exit(EXIT_FAILURE);
        }

        if (!magicTreeInsertFits(dbh, fits)) {
            exit(EXIT_FAILURE);
        }

        if (!psFitsClose(fits)) {
            exit(EXIT_FAILURE);
        }

        psDBCleanup(dbh);
    }

    {
        psDB            *dbh;
        psFits          *fits;

        dbh = psDBInit("localhost", "test", NULL, "test");
        if (!dbh) {
            exit(EXIT_FAILURE);
        }

        // open a temp
        fits = psFitsOpen(TMP_FILENAME, "r");
        if (!fits) {
            exit(EXIT_FAILURE);
        }

        if (!magicNodeResultInsertFits(dbh, fits)) {
            exit(EXIT_FAILURE);
        }

        if (!psFitsClose(fits)) {
            exit(EXIT_FAILURE);
        }

        psDBCleanup(dbh);
    }

    {
        psDB            *dbh;
        psFits          *fits;

        dbh = psDBInit("localhost", "test", NULL, "test");
        if (!dbh) {
            exit(EXIT_FAILURE);
        }

        // open a temp
        fits = psFitsOpen(TMP_FILENAME, "r");
        if (!fits) {
            exit(EXIT_FAILURE);
        }

        if (!magicMaskInsertFits(dbh, fits)) {
            exit(EXIT_FAILURE);
        }

        if (!psFitsClose(fits)) {
            exit(EXIT_FAILURE);
        }

        psDBCleanup(dbh);
    }

    {
        psDB            *dbh;
        psFits          *fits;

        dbh = psDBInit("localhost", "test", NULL, "test");
        if (!dbh) {
            exit(EXIT_FAILURE);
        }

        // open a temp
        fits = psFitsOpen(TMP_FILENAME, "r");
        if (!fits) {
            exit(EXIT_FAILURE);
        }

        if (!calDBInsertFits(dbh, fits)) {
            exit(EXIT_FAILURE);
        }

        if (!psFitsClose(fits)) {
            exit(EXIT_FAILURE);
        }

        psDBCleanup(dbh);
    }

    {
        psDB            *dbh;
        psFits          *fits;

        dbh = psDBInit("localhost", "test", NULL, "test");
        if (!dbh) {
            exit(EXIT_FAILURE);
        }

        // open a temp
        fits = psFitsOpen(TMP_FILENAME, "r");
        if (!fits) {
            exit(EXIT_FAILURE);
        }

        if (!calRunInsertFits(dbh, fits)) {
            exit(EXIT_FAILURE);
        }

        if (!psFitsClose(fits)) {
            exit(EXIT_FAILURE);
        }

        psDBCleanup(dbh);
    }

    {
        psDB            *dbh;
        psFits          *fits;

        dbh = psDBInit("localhost", "test", NULL, "test");
        if (!dbh) {
            exit(EXIT_FAILURE);
        }

        // open a temp
        fits = psFitsOpen(TMP_FILENAME, "r");
        if (!fits) {
            exit(EXIT_FAILURE);
        }

        if (!flatcorrRunInsertFits(dbh, fits)) {
            exit(EXIT_FAILURE);
        }

        if (!psFitsClose(fits)) {
            exit(EXIT_FAILURE);
        }

        psDBCleanup(dbh);
    }

    {
        psDB            *dbh;
        psFits          *fits;

        dbh = psDBInit("localhost", "test", NULL, "test");
        if (!dbh) {
            exit(EXIT_FAILURE);
        }

        // open a temp
        fits = psFitsOpen(TMP_FILENAME, "r");
        if (!fits) {
            exit(EXIT_FAILURE);
        }

        if (!flatcorrChipLinkInsertFits(dbh, fits)) {
            exit(EXIT_FAILURE);
        }

        if (!psFitsClose(fits)) {
            exit(EXIT_FAILURE);
        }

        psDBCleanup(dbh);
    }

    {
        psDB            *dbh;
        psFits          *fits;

        dbh = psDBInit("localhost", "test", NULL, "test");
        if (!dbh) {
            exit(EXIT_FAILURE);
        }

        // open a temp
        fits = psFitsOpen(TMP_FILENAME, "r");
        if (!fits) {
            exit(EXIT_FAILURE);
        }

        if (!flatcorrCamLinkInsertFits(dbh, fits)) {
            exit(EXIT_FAILURE);
        }

        if (!psFitsClose(fits)) {
            exit(EXIT_FAILURE);
        }

        psDBCleanup(dbh);
    }

    {
        psDB            *dbh;
        psFits          *fits;

        dbh = psDBInit("localhost", "test", NULL, "test");
        if (!dbh) {
            exit(EXIT_FAILURE);
        }

        // open a temp
        fits = psFitsOpen(TMP_FILENAME, "r");
        if (!fits) {
            exit(EXIT_FAILURE);
        }

        if (!pstampDataStoreInsertFits(dbh, fits)) {
            exit(EXIT_FAILURE);
        }

        if (!psFitsClose(fits)) {
            exit(EXIT_FAILURE);
        }

        psDBCleanup(dbh);
    }

    {
        psDB            *dbh;
        psFits          *fits;

        dbh = psDBInit("localhost", "test", NULL, "test");
        if (!dbh) {
            exit(EXIT_FAILURE);
        }

        // open a temp
        fits = psFitsOpen(TMP_FILENAME, "r");
        if (!fits) {
            exit(EXIT_FAILURE);
        }

        if (!pstampProjectInsertFits(dbh, fits)) {
            exit(EXIT_FAILURE);
        }

        if (!psFitsClose(fits)) {
            exit(EXIT_FAILURE);
        }

        psDBCleanup(dbh);
    }

    {
        psDB            *dbh;
        psFits          *fits;

        dbh = psDBInit("localhost", "test", NULL, "test");
        if (!dbh) {
            exit(EXIT_FAILURE);
        }

        // open a temp
        fits = psFitsOpen(TMP_FILENAME, "r");
        if (!fits) {
            exit(EXIT_FAILURE);
        }

        if (!pstampRequestInsertFits(dbh, fits)) {
            exit(EXIT_FAILURE);
        }

        if (!psFitsClose(fits)) {
            exit(EXIT_FAILURE);
        }

        psDBCleanup(dbh);
    }

    {
        psDB            *dbh;
        psFits          *fits;

        dbh = psDBInit("localhost", "test", NULL, "test");
        if (!dbh) {
            exit(EXIT_FAILURE);
        }

        // open a temp
        fits = psFitsOpen(TMP_FILENAME, "r");
        if (!fits) {
            exit(EXIT_FAILURE);
        }

        if (!pstampJobInsertFits(dbh, fits)) {
            exit(EXIT_FAILURE);
        }

        if (!psFitsClose(fits)) {
            exit(EXIT_FAILURE);
        }

        psDBCleanup(dbh);
    }

    exit(EXIT_SUCCESS);
}
