SELECT DISTINCT
    'skycal' as stage,
    skycalRun.skycal_id AS stage_id,
    CAST(0 AS SIGNED) AS magicked,
    -- run tag in the form 'skycal.$skycell_id.$skycal_id'
    CONCAT_WS('.', 'skycal', stackRun.skycell_id, convert(skycalRun.skycal_id, CHAR)) as run_tag,
    skycalRun.label,
    skycalRun.data_group,
    distTarget.dist_group,
    distTarget.target_id,
    distTarget.clean
FROM skycalRun
JOIN skycalResult USING(skycal_id)
JOIN stackRun using(stack_id)
JOIN distTarget ON distTarget.stage = 'skycal'
    AND skycalRun.dist_group = distTarget.dist_group
JOIN rcInterest USING(target_id)
LEFT JOIN distRun ON (distRun.stage_id = skycal_id)
    AND distRun.target_id = distTarget.target_id
    -- JOIN hook %s
WHERE  distTarget.state = 'enabled'
    AND rcInterest.state = 'enabled'
    AND ((skycalRun.state = 'full') OR (distTarget.clean AND skycalRun.state = 'cleaned'))
    AND stackRun.filter = distTarget.filter
    -- we shouldn't need to check fault. If faulted it shouldn't be full
    AND (skycalResult.fault = 0 AND skycalResult.quality = 0)
