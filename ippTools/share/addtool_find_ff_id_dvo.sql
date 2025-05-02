SELECT fullForceRun.*, fullForceResult.warp_id FROM fullForceRun JOIN fullForceResult using (ff_id)
JOIN skycalRun using (skycal_id) join stackRun using (stack_id) 
JOIN skycell using (skycell_id, tess_id)

WHERE fullForceRun.state = 'full' and fullForceResult.quality = 0
    AND fullForceRun.ff_id NOT IN (SELECT fullForceRun.ff_id
       FROM addRun
       JOIN fullForceRun on (fullForceRun.ff_id = addRun.stage_id and addRun.stage = 'fullforce') JOIN fullForceResult on (fullForceRun.ff_id = fullForceResult.ff_id)
       WHERE addRun.stage = 'fullforce' AND %s
      )
