SELECT DISTINCT
    'stack' as stage,
    stackRun.stack_id AS stage_id,
    CAST(0 AS SIGNED) AS magicked,
    -- run tag in the form 'stack.$skycell_id.$stack_id'
    CONCAT_WS('.', 'stack', stackRun.skycell_id, convert(stackRun.stack_id, CHAR)) as run_tag,
    stackRun.label,
    stackRun.data_group,
    distTarget.dist_group,
    distTarget.target_id,
    distTarget.clean
FROM stackRun
JOIN stackSumSkyfile USING(stack_id)
JOIN distTarget ON distTarget.stage = 'stack'
    AND stackRun.dist_group = distTarget.dist_group
    AND stackRun.filter = distTarget.filter
JOIN rcInterest USING(target_id)
LEFT JOIN distRun ON (distRun.stage_id = stack_id)
    AND distRun.target_id = distTarget.target_id
    -- JOIN hook %s
WHERE  distTarget.state = 'enabled'
    AND rcInterest.state = 'enabled'
    AND ((stackRun.state = 'full') OR (distTarget.clean AND stackRun.state = 'cleaned'))
    -- we shouldn't need to check fault. If faulted it shouldn't be full
    AND (stackSumSkyfile.fault = 0 AND stackSumSkyfile.quality = 0)
