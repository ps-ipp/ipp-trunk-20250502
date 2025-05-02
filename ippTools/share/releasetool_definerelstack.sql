SELECT
    ippRelease.rel_id,
    stackRun.stack_id,
    0 as skycal_id,
    stackRun.skycell_id,
    stackRun.tess_id,
    stackRun.filter,
    stackSumSkyfile.mjd_obs,
    0 AS zpt_obs,
    0 AS zpt_stdev

FROM ippRelease
JOIN stackRun
JOIN stackSumSkyfile using(stack_id)
LEFT JOIN relStack AS previousRelStack 
    ON previousRelStack.rel_id = ippRelease.rel_id
    AND previousRelStack.tess_id = stackRun.tess_id 
    AND previousRelStack.skycell_id = stackRun.skycell_id
    AND previousRelStack.filter = stackRun.filter
    -- JOIN hook %s
WHERE previousRelStack.relstack_id IS NULL
    AND stackRun.state ='full'
    AND stackSumSkyfile.quality = 0
    AND stackSumSkyfile.fault = 0
