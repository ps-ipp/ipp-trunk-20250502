#include <pslib.h>
#include <stdlib.h>

int main ()
{
    psDB            *dbh;

    dbh = psDBInit("localhost", "test", NULL, "test", 0);
    if (!dbh) {
        exit(EXIT_FAILURE);
    }

    p_psDBRunQuery(dbh, "DROP TABLE IF EXISTS pzDataStore");
    p_psDBRunQuery(dbh, "DROP TABLE IF EXISTS summitExp");
    p_psDBRunQuery(dbh, "DROP TABLE IF EXISTS summitImfile");
    p_psDBRunQuery(dbh, "DROP TABLE IF EXISTS pzDownloadExp");
    p_psDBRunQuery(dbh, "DROP TABLE IF EXISTS pzDownloadImfile");
    p_psDBRunQuery(dbh, "DROP TABLE IF EXISTS newExp");
    p_psDBRunQuery(dbh, "DROP TABLE IF EXISTS newImfile");
    p_psDBRunQuery(dbh, "DROP TABLE IF EXISTS rawExp");
    p_psDBRunQuery(dbh, "DROP TABLE IF EXISTS rawImfile");
    p_psDBRunQuery(dbh, "DROP TABLE IF EXISTS guidePendingExp");
    p_psDBRunQuery(dbh, "DROP TABLE IF EXISTS chipRun");
    p_psDBRunQuery(dbh, "DROP TABLE IF EXISTS chipProcessedImfile");
    p_psDBRunQuery(dbh, "DROP TABLE IF EXISTS chipMask");
    p_psDBRunQuery(dbh, "DROP TABLE IF EXISTS camRun");
    p_psDBRunQuery(dbh, "DROP TABLE IF EXISTS camProcessedExp");
    p_psDBRunQuery(dbh, "DROP TABLE IF EXISTS camMask");
    p_psDBRunQuery(dbh, "DROP TABLE IF EXISTS fakeRun");
    p_psDBRunQuery(dbh, "DROP TABLE IF EXISTS fakeProcessedImfile");
    p_psDBRunQuery(dbh, "DROP TABLE IF EXISTS fakeMask");
    p_psDBRunQuery(dbh, "DROP TABLE IF EXISTS warpRun");
    p_psDBRunQuery(dbh, "DROP TABLE IF EXISTS warpSkyCellMap");
    p_psDBRunQuery(dbh, "DROP TABLE IF EXISTS warpSkyfile");
    p_psDBRunQuery(dbh, "DROP TABLE IF EXISTS warpMask");
    p_psDBRunQuery(dbh, "DROP TABLE IF EXISTS diffRun");
    p_psDBRunQuery(dbh, "DROP TABLE IF EXISTS diffInputSkyfile");
    p_psDBRunQuery(dbh, "DROP TABLE IF EXISTS diffSkyfile");
    p_psDBRunQuery(dbh, "DROP TABLE IF EXISTS stackRun");
    p_psDBRunQuery(dbh, "DROP TABLE IF EXISTS stackInputSkyfile");
    p_psDBRunQuery(dbh, "DROP TABLE IF EXISTS stackSumSkyfile");
    p_psDBRunQuery(dbh, "DROP TABLE IF EXISTS detRun");
    p_psDBRunQuery(dbh, "DROP TABLE IF EXISTS detInputExp");
    p_psDBRunQuery(dbh, "DROP TABLE IF EXISTS detProcessedImfile");
    p_psDBRunQuery(dbh, "DROP TABLE IF EXISTS detProcessedExp");
    p_psDBRunQuery(dbh, "DROP TABLE IF EXISTS detStackedImfile");
    p_psDBRunQuery(dbh, "DROP TABLE IF EXISTS detNormalizedStatImfile");
    p_psDBRunQuery(dbh, "DROP TABLE IF EXISTS detNormalizedImfile");
    p_psDBRunQuery(dbh, "DROP TABLE IF EXISTS detNormalizedExp");
    p_psDBRunQuery(dbh, "DROP TABLE IF EXISTS detResidImfile");
    p_psDBRunQuery(dbh, "DROP TABLE IF EXISTS detResidExp");
    p_psDBRunQuery(dbh, "DROP TABLE IF EXISTS detRunSummary");
    p_psDBRunQuery(dbh, "DROP TABLE IF EXISTS detRegisteredImfile");
    p_psDBRunQuery(dbh, "DROP TABLE IF EXISTS detCorrectedExp");
    p_psDBRunQuery(dbh, "DROP TABLE IF EXISTS detCorrectedImfile");
    p_psDBRunQuery(dbh, "DROP TABLE IF EXISTS magicRun");
    p_psDBRunQuery(dbh, "DROP TABLE IF EXISTS magicInputSkyfile");
    p_psDBRunQuery(dbh, "DROP TABLE IF EXISTS magicTree");
    p_psDBRunQuery(dbh, "DROP TABLE IF EXISTS magicNodeResult");
    p_psDBRunQuery(dbh, "DROP TABLE IF EXISTS magicMask");
    p_psDBRunQuery(dbh, "DROP TABLE IF EXISTS calDB");
    p_psDBRunQuery(dbh, "DROP TABLE IF EXISTS calRun");
    p_psDBRunQuery(dbh, "DROP TABLE IF EXISTS flatcorrRun");
    p_psDBRunQuery(dbh, "DROP TABLE IF EXISTS flatcorrChipLink");
    p_psDBRunQuery(dbh, "DROP TABLE IF EXISTS flatcorrCamLink");
    p_psDBRunQuery(dbh, "DROP TABLE IF EXISTS pstampDataStore");
    p_psDBRunQuery(dbh, "DROP TABLE IF EXISTS pstampProject");
    p_psDBRunQuery(dbh, "DROP TABLE IF EXISTS pstampRequest");
    p_psDBRunQuery(dbh, "DROP TABLE IF EXISTS pstampJob");

    psDBCleanup(dbh);

    exit(EXIT_SUCCESS);
}
