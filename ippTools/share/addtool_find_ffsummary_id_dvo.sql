SELECT fullForceRun.* FROM fullForceRun 
JOIN fullForceSummary USING (ff_id)
JOIN skycalRun USING (skycal_id) 
JOIN stackRun USING (stack_id) 
JOIN skycell USING (skycell_id, tess_id)

WHERE fullForceRun.state = 'full' 
AND fullForceSummary.quality = 0 
AND fullForceSummary.fault = 0 
AND fullForceRun.ff_id NOT IN (
    SELECT fullForceRun.ff_id FROM addRun
    JOIN fullForceRun ON (fullForceRun.ff_id = addRun.stage_id and addRun.stage = 'fullforce_summary') 
    JOIN fullForceSummary ON (fullForceRun.ff_id = fullForceSummary.ff_id)
       WHERE addRun.stage = 'fullforce_summary' AND %s
    )
