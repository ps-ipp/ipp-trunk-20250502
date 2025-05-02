SELECT
    ippRelease.rel_id,
    stackRun.stack_id,
    IFNULL(skycalRun.skycal_id, 0) as skycal_id,
    stackRun.skycell_id,
    stackRun.tess_id,
    stackRun.filter,
    stackSumSkyfile.mjd_obs,
    skycalResult.zpt_obs,
    skycalResult.zpt_stdev

FROM ippRelease
JOIN stackRun
JOIN stackSumSkyfile USING(stack_id)
JOIN skycalRun ON skycalRun.stack_id = stackRun.stack_id
JOIN skycalResult ON skycalRun.skycal_id = skycalResult.skycal_id
-- LEFT JOIN relStack AS previousRelStack USING(rel_id, tess_id, skycell_id, filter)
LEFT JOIN relStack AS previousRelStack 
    ON ippRelease.rel_id = previousRelStack.rel_id 
    AND stackRun.tess_id = previousRelStack.tess_id 
    AND stackRun.skycell_id = previousRelStack.skycell_id 
    AND stackRun.filter = previousRelStack.filter
-- JOIN Hook %s

WHERE previousRelStack.relstack_id IS NULL
    AND skycalRun.state = 'full'
