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

        if (!pzDataStoreDropTable(dbh)) {
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

        if (!summitExpDropTable(dbh)) {
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

        if (!summitImfileDropTable(dbh)) {
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

        if (!pzDownloadExpDropTable(dbh)) {
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

        if (!pzDownloadImfileDropTable(dbh)) {
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

        if (!newExpDropTable(dbh)) {
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

        if (!newImfileDropTable(dbh)) {
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

        if (!rawExpDropTable(dbh)) {
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

        if (!rawImfileDropTable(dbh)) {
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

        if (!guidePendingExpDropTable(dbh)) {
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

        if (!chipRunDropTable(dbh)) {
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

        if (!chipProcessedImfileDropTable(dbh)) {
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

        if (!chipMaskDropTable(dbh)) {
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

        if (!camRunDropTable(dbh)) {
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

        if (!camProcessedExpDropTable(dbh)) {
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

        if (!camMaskDropTable(dbh)) {
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

        if (!fakeRunDropTable(dbh)) {
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

        if (!fakeProcessedImfileDropTable(dbh)) {
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

        if (!fakeMaskDropTable(dbh)) {
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

        if (!warpRunDropTable(dbh)) {
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

        if (!warpSkyCellMapDropTable(dbh)) {
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

        if (!warpSkyfileDropTable(dbh)) {
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

        if (!warpMaskDropTable(dbh)) {
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

        if (!diffRunDropTable(dbh)) {
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

        if (!diffInputSkyfileDropTable(dbh)) {
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

        if (!diffSkyfileDropTable(dbh)) {
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

        if (!stackRunDropTable(dbh)) {
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

        if (!stackInputSkyfileDropTable(dbh)) {
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

        if (!stackSumSkyfileDropTable(dbh)) {
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

        if (!detRunDropTable(dbh)) {
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

        if (!detInputExpDropTable(dbh)) {
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

        if (!detProcessedImfileDropTable(dbh)) {
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

        if (!detProcessedExpDropTable(dbh)) {
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

        if (!detStackedImfileDropTable(dbh)) {
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

        if (!detNormalizedStatImfileDropTable(dbh)) {
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

        if (!detNormalizedImfileDropTable(dbh)) {
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

        if (!detNormalizedExpDropTable(dbh)) {
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

        if (!detResidImfileDropTable(dbh)) {
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

        if (!detResidExpDropTable(dbh)) {
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

        if (!detRunSummaryDropTable(dbh)) {
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

        if (!detRegisteredImfileDropTable(dbh)) {
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

        if (!detCorrectedExpDropTable(dbh)) {
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

        if (!detCorrectedImfileDropTable(dbh)) {
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

        if (!magicRunDropTable(dbh)) {
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

        if (!magicInputSkyfileDropTable(dbh)) {
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

        if (!magicTreeDropTable(dbh)) {
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

        if (!magicNodeResultDropTable(dbh)) {
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

        if (!magicMaskDropTable(dbh)) {
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

        if (!calDBDropTable(dbh)) {
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

        if (!calRunDropTable(dbh)) {
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

        if (!flatcorrRunDropTable(dbh)) {
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

        if (!flatcorrChipLinkDropTable(dbh)) {
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

        if (!flatcorrCamLinkDropTable(dbh)) {
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

        if (!pstampDataStoreDropTable(dbh)) {
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

        if (!pstampProjectDropTable(dbh)) {
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

        if (!pstampRequestDropTable(dbh)) {
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

        if (!pstampJobDropTable(dbh)) {
            exit(EXIT_FAILURE);
        }

        psDBCleanup(dbh);
    }

    exit(EXIT_SUCCESS);
}
