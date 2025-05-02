SELECT DISTINCT
    'ff' as stage,
    fullForceRun.ff_id AS stage_id,
    CAST(0 AS SIGNED) AS magicked,
    -- run tag in the form 'ff.$skycell_id.$ff_id'
    CONCAT_WS('.', 'ff', stackRun.skycell_id, convert(fullForceRun.ff_id, CHAR)) as run_tag,
    fullForceRun.label,
    fullForceRun.data_group,
    distTarget.dist_group,
    distTarget.target_id,
    distTarget.clean
FROM fullForceRun
JOIN skycalRun USING(skycal_id)
JOIN stackRun using(stack_id)
JOIN distTarget ON distTarget.stage = 'ff'
    AND fullForceRun.dist_group = distTarget.dist_group
JOIN rcInterest USING(target_id)
LEFT JOIN distRun ON (distRun.stage_id = ff_id)
    AND distRun.target_id = distTarget.target_id
    -- JOIN hook %s
WHERE  distTarget.state = 'enabled'
    AND rcInterest.state = 'enabled'
    AND fullForceRun.state = 'full'
    AND stackRun.filter = distTarget.filter
    -- we shouldn't need to check fault. If faulted it shouldn't be full
    -- AND (skycalResult.fault = 0 AND skycalResult.quality = 0)
