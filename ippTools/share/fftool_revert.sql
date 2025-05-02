DELETE FROM fullForceResult
USING fullForceResult, fullForceRun, skycalRun, stackRun, skycell
WHERE fullForceRun.ff_id = fullForceResult.ff_id
    AND fullForceResult.fault != 0
    AND fullForceRun.skycal_id = skycalRun.skycal_id
    AND skycalRun.stack_id = stackRun.stack_id
    AND stackRun.tess_id = skycell.tess_id AND stackRun.skycell_id =  skycell.skycell_id
