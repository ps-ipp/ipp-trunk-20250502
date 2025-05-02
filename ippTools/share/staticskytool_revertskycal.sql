DELETE FROM skycalResult
USING skycalResult, skycalRun, stackRun, skycell
WHERE skycalRun.skycal_id = skycalResult.skycal_id
    AND stackRun.stack_id = skycalRun.stack_id
    AND skycalRun.state = 'new'
    AND skycalResult.fault != 0
    AND stackRun.tess_id = skycell.tess_id
    AND stackRun.skycell_id = skycell.skycell_id
