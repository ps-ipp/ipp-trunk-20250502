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

        if(!pzDataStoreCreateTable(dbh)) {
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

        if(!summitExpCreateTable(dbh)) {
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

        if(!summitImfileCreateTable(dbh)) {
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

        if(!pzDownloadExpCreateTable(dbh)) {
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

        if(!pzDownloadImfileCreateTable(dbh)) {
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

        if(!newExpCreateTable(dbh)) {
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

        if(!newImfileCreateTable(dbh)) {
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

        if(!rawExpCreateTable(dbh)) {
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

        if(!rawImfileCreateTable(dbh)) {
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

        if(!guidePendingExpCreateTable(dbh)) {
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

        if(!chipRunCreateTable(dbh)) {
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

        if(!chipProcessedImfileCreateTable(dbh)) {
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

        if(!chipMaskCreateTable(dbh)) {
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

        if(!camRunCreateTable(dbh)) {
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

        if(!camProcessedExpCreateTable(dbh)) {
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

        if(!camMaskCreateTable(dbh)) {
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

        if(!fakeRunCreateTable(dbh)) {
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

        if(!fakeProcessedImfileCreateTable(dbh)) {
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

        if(!fakeMaskCreateTable(dbh)) {
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

        if(!warpRunCreateTable(dbh)) {
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

        if(!warpSkyCellMapCreateTable(dbh)) {
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

        if(!warpSkyfileCreateTable(dbh)) {
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

        if(!warpMaskCreateTable(dbh)) {
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

        if(!diffRunCreateTable(dbh)) {
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

        if(!diffInputSkyfileCreateTable(dbh)) {
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

        if(!diffSkyfileCreateTable(dbh)) {
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

        if(!stackRunCreateTable(dbh)) {
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

        if(!stackInputSkyfileCreateTable(dbh)) {
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

        if(!stackSumSkyfileCreateTable(dbh)) {
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

        if(!detRunCreateTable(dbh)) {
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

        if(!detInputExpCreateTable(dbh)) {
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

        if(!detProcessedImfileCreateTable(dbh)) {
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

        if(!detProcessedExpCreateTable(dbh)) {
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

        if(!detStackedImfileCreateTable(dbh)) {
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

        if(!detNormalizedStatImfileCreateTable(dbh)) {
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

        if(!detNormalizedImfileCreateTable(dbh)) {
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

        if(!detNormalizedExpCreateTable(dbh)) {
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

        if(!detResidImfileCreateTable(dbh)) {
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

        if(!detResidExpCreateTable(dbh)) {
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

        if(!detRunSummaryCreateTable(dbh)) {
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

        if(!detRegisteredImfileCreateTable(dbh)) {
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

        if(!detCorrectedExpCreateTable(dbh)) {
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

        if(!detCorrectedImfileCreateTable(dbh)) {
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

        if(!magicRunCreateTable(dbh)) {
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

        if(!magicInputSkyfileCreateTable(dbh)) {
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

        if(!magicTreeCreateTable(dbh)) {
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

        if(!magicNodeResultCreateTable(dbh)) {
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

        if(!magicMaskCreateTable(dbh)) {
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

        if(!calDBCreateTable(dbh)) {
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

        if(!calRunCreateTable(dbh)) {
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

        if(!flatcorrRunCreateTable(dbh)) {
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

        if(!flatcorrChipLinkCreateTable(dbh)) {
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

        if(!flatcorrCamLinkCreateTable(dbh)) {
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

        if(!pstampDataStoreCreateTable(dbh)) {
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

        if(!pstampProjectCreateTable(dbh)) {
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

        if(!pstampRequestCreateTable(dbh)) {
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

        if(!pstampJobCreateTable(dbh)) {
            exit(EXIT_FAILURE);
        }

        psDBCleanup(dbh);
    }

    exit(EXIT_SUCCESS);
}
