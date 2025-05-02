DELETE FROM fullForceSummary
USING fullForceSummary, fullForceRun, skycalRun, stackRun, skycell
WHERE fullForceRun.ff_id = fullForceSummary.ff_id
    AND fullForceSummary.fault != 0
    AND fullForceRun.skycal_id = skycalRun.skycal_id
    AND skycalRun.stack_id = stackRun.stack_id
    AND stackRun.tess_id = skycell.tess_id AND stackRun.skycell_id =  skycell.skycell_id
