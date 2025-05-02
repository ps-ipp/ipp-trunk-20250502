UPDATE relStack
    JOIN ippRelease USING(rel_id)
    JOIN skycell USING(tess_id, skycell_id)
    JOIN stackRun USING(stack_id, tess_id, skycell_id)  
    JOIN skycalRun USING(stack_id)  -- join to skycal using stack_id not skycal_id ...
    JOIN skycalResult ON skycalRun.skycal_id = skycalResult.skycal_id
SET relStack.skycal_id = skycalRun.skycal_id,    -- ... because we set skycal_id here
    relStack.zpt_obs = skycalResult.zpt_obs,
    relStack.zpt_stdev = skycalResult.zpt_stdev,
    relStack.state = 'calibrated'
