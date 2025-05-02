SELECT DISTINCT
    'sky' as stage,
    staticskyRun.sky_id AS stage_id,
    CAST(0 AS SIGNED) AS magicked,
    -- run tag in the form 'sky.$skycell_id.$sky_id'
    CONCAT_WS('.', 'sky', stackRun.skycell_id, convert(staticskyRun.sky_id, CHAR)) as run_tag,
    staticskyRun.label,
    staticskyRun.data_group,
    distTarget.dist_group,
    distTarget.target_id,
    distTarget.clean
FROM staticskyRun
JOIN staticskyResult USING(sky_id)
JOIN staticskyInput USING(sky_id)
JOIN stackRun using(stack_id)
JOIN distTarget ON distTarget.stage = 'sky'
    AND staticskyRun.dist_group = distTarget.dist_group
JOIN rcInterest USING(target_id)
LEFT JOIN distRun ON (distRun.stage_id = sky_id)
    AND distRun.target_id = distTarget.target_id
    -- JOIN hook %s
WHERE  distTarget.state = 'enabled'
    AND rcInterest.state = 'enabled'
    AND distTarget.filter = 'multi'
    AND ((staticskyRun.state = 'full') OR (distTarget.clean AND staticskyRun.state = 'cleaned'))
    -- we shouldn't need to check fault. If faulted it shouldn't be full
    AND (staticskyResult.fault = 0 AND staticskyResult.quality = 0)
