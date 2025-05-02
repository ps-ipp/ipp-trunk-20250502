#include <pslib.h>
#include <ippdb.h>
#include <stdlib.h>

int main ()
{
    psDB            *dbh;

    dbh = psDBInit("localhost", "test", NULL, "test", 0);
    if (!dbh) {
        exit(EXIT_FAILURE);
    }

    // remove the table if it already exists
    p_psDBRunQuery(dbh, "DROP TABLE IF EXISTS pzDataStore");
    pzDataStoreCreateTable(dbh);

    p_psDBRunQuery(dbh, "DROP TABLE IF EXISTS summitExp");
    summitExpCreateTable(dbh);

    p_psDBRunQuery(dbh, "DROP TABLE IF EXISTS summitImfile");
    summitImfileCreateTable(dbh);

    p_psDBRunQuery(dbh, "DROP TABLE IF EXISTS pzDownloadExp");
    pzDownloadExpCreateTable(dbh);

    p_psDBRunQuery(dbh, "DROP TABLE IF EXISTS pzDownloadImfile");
    pzDownloadImfileCreateTable(dbh);

    p_psDBRunQuery(dbh, "DROP TABLE IF EXISTS newExp");
    newExpCreateTable(dbh);

    p_psDBRunQuery(dbh, "DROP TABLE IF EXISTS newImfile");
    newImfileCreateTable(dbh);

    p_psDBRunQuery(dbh, "DROP TABLE IF EXISTS rawExp");
    rawExpCreateTable(dbh);

    p_psDBRunQuery(dbh, "DROP TABLE IF EXISTS rawImfile");
    rawImfileCreateTable(dbh);

    p_psDBRunQuery(dbh, "DROP TABLE IF EXISTS guidePendingExp");
    guidePendingExpCreateTable(dbh);

    p_psDBRunQuery(dbh, "DROP TABLE IF EXISTS chipRun");
    chipRunCreateTable(dbh);

    p_psDBRunQuery(dbh, "DROP TABLE IF EXISTS chipProcessedImfile");
    chipProcessedImfileCreateTable(dbh);

    p_psDBRunQuery(dbh, "DROP TABLE IF EXISTS chipMask");
    chipMaskCreateTable(dbh);

    p_psDBRunQuery(dbh, "DROP TABLE IF EXISTS camRun");
    camRunCreateTable(dbh);

    p_psDBRunQuery(dbh, "DROP TABLE IF EXISTS camProcessedExp");
    camProcessedExpCreateTable(dbh);

    p_psDBRunQuery(dbh, "DROP TABLE IF EXISTS camMask");
    camMaskCreateTable(dbh);

    p_psDBRunQuery(dbh, "DROP TABLE IF EXISTS fakeRun");
    fakeRunCreateTable(dbh);

    p_psDBRunQuery(dbh, "DROP TABLE IF EXISTS fakeProcessedImfile");
    fakeProcessedImfileCreateTable(dbh);

    p_psDBRunQuery(dbh, "DROP TABLE IF EXISTS fakeMask");
    fakeMaskCreateTable(dbh);

    p_psDBRunQuery(dbh, "DROP TABLE IF EXISTS warpRun");
    warpRunCreateTable(dbh);

    p_psDBRunQuery(dbh, "DROP TABLE IF EXISTS warpSkyCellMap");
    warpSkyCellMapCreateTable(dbh);

    p_psDBRunQuery(dbh, "DROP TABLE IF EXISTS warpSkyfile");
    warpSkyfileCreateTable(dbh);

    p_psDBRunQuery(dbh, "DROP TABLE IF EXISTS warpMask");
    warpMaskCreateTable(dbh);

    p_psDBRunQuery(dbh, "DROP TABLE IF EXISTS diffRun");
    diffRunCreateTable(dbh);

    p_psDBRunQuery(dbh, "DROP TABLE IF EXISTS diffInputSkyfile");
    diffInputSkyfileCreateTable(dbh);

    p_psDBRunQuery(dbh, "DROP TABLE IF EXISTS diffSkyfile");
    diffSkyfileCreateTable(dbh);

    p_psDBRunQuery(dbh, "DROP TABLE IF EXISTS stackRun");
    stackRunCreateTable(dbh);

    p_psDBRunQuery(dbh, "DROP TABLE IF EXISTS stackInputSkyfile");
    stackInputSkyfileCreateTable(dbh);

    p_psDBRunQuery(dbh, "DROP TABLE IF EXISTS stackSumSkyfile");
    stackSumSkyfileCreateTable(dbh);

    p_psDBRunQuery(dbh, "DROP TABLE IF EXISTS detRun");
    detRunCreateTable(dbh);

    p_psDBRunQuery(dbh, "DROP TABLE IF EXISTS detInputExp");
    detInputExpCreateTable(dbh);

    p_psDBRunQuery(dbh, "DROP TABLE IF EXISTS detProcessedImfile");
    detProcessedImfileCreateTable(dbh);

    p_psDBRunQuery(dbh, "DROP TABLE IF EXISTS detProcessedExp");
    detProcessedExpCreateTable(dbh);

    p_psDBRunQuery(dbh, "DROP TABLE IF EXISTS detStackedImfile");
    detStackedImfileCreateTable(dbh);

    p_psDBRunQuery(dbh, "DROP TABLE IF EXISTS detNormalizedStatImfile");
    detNormalizedStatImfileCreateTable(dbh);

    p_psDBRunQuery(dbh, "DROP TABLE IF EXISTS detNormalizedImfile");
    detNormalizedImfileCreateTable(dbh);

    p_psDBRunQuery(dbh, "DROP TABLE IF EXISTS detNormalizedExp");
    detNormalizedExpCreateTable(dbh);

    p_psDBRunQuery(dbh, "DROP TABLE IF EXISTS detResidImfile");
    detResidImfileCreateTable(dbh);

    p_psDBRunQuery(dbh, "DROP TABLE IF EXISTS detResidExp");
    detResidExpCreateTable(dbh);

    p_psDBRunQuery(dbh, "DROP TABLE IF EXISTS detRunSummary");
    detRunSummaryCreateTable(dbh);

    p_psDBRunQuery(dbh, "DROP TABLE IF EXISTS detRegisteredImfile");
    detRegisteredImfileCreateTable(dbh);

    p_psDBRunQuery(dbh, "DROP TABLE IF EXISTS detCorrectedExp");
    detCorrectedExpCreateTable(dbh);

    p_psDBRunQuery(dbh, "DROP TABLE IF EXISTS detCorrectedImfile");
    detCorrectedImfileCreateTable(dbh);

    p_psDBRunQuery(dbh, "DROP TABLE IF EXISTS magicRun");
    magicRunCreateTable(dbh);

    p_psDBRunQuery(dbh, "DROP TABLE IF EXISTS magicInputSkyfile");
    magicInputSkyfileCreateTable(dbh);

    p_psDBRunQuery(dbh, "DROP TABLE IF EXISTS magicTree");
    magicTreeCreateTable(dbh);

    p_psDBRunQuery(dbh, "DROP TABLE IF EXISTS magicNodeResult");
    magicNodeResultCreateTable(dbh);

    p_psDBRunQuery(dbh, "DROP TABLE IF EXISTS magicMask");
    magicMaskCreateTable(dbh);

    p_psDBRunQuery(dbh, "DROP TABLE IF EXISTS calDB");
    calDBCreateTable(dbh);

    p_psDBRunQuery(dbh, "DROP TABLE IF EXISTS calRun");
    calRunCreateTable(dbh);

    p_psDBRunQuery(dbh, "DROP TABLE IF EXISTS flatcorrRun");
    flatcorrRunCreateTable(dbh);

    p_psDBRunQuery(dbh, "DROP TABLE IF EXISTS flatcorrChipLink");
    flatcorrChipLinkCreateTable(dbh);

    p_psDBRunQuery(dbh, "DROP TABLE IF EXISTS flatcorrCamLink");
    flatcorrCamLinkCreateTable(dbh);

    p_psDBRunQuery(dbh, "DROP TABLE IF EXISTS pstampDataStore");
    pstampDataStoreCreateTable(dbh);

    p_psDBRunQuery(dbh, "DROP TABLE IF EXISTS pstampProject");
    pstampProjectCreateTable(dbh);

    p_psDBRunQuery(dbh, "DROP TABLE IF EXISTS pstampRequest");
    pstampRequestCreateTable(dbh);

    p_psDBRunQuery(dbh, "DROP TABLE IF EXISTS pstampJob");
    pstampJobCreateTable(dbh);

    psDBCleanup(dbh);

    exit(EXIT_SUCCESS);
}
