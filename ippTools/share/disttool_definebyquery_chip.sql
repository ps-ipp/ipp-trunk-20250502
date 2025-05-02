SELECT DISTINCT
    'chip' as stage,
    chipRun.chip_id as stage_id,
    chipRun.magicked,
    rawExp.exp_name as run_tag,
    chipRun.label,
    chipRun.data_group,
    chipRun.dist_group,
    distTarget.target_id,
    distTarget.clean
FROM chipRun
JOIN rawExp USING(exp_id)
JOIN distTarget ON distTarget.stage = 'chip'
    AND chipRun.dist_group = distTarget.dist_group 
    AND rawExp.filter = distTarget.filter
JOIN rcInterest USING(target_id)
LEFT JOIN distRun ON distRun.stage_id = chipRun.chip_id
    AND distRun.target_id = distTarget.target_id
    -- JOIN hook %s
WHERE distTarget.state = 'enabled'
    AND rcInterest.state = 'enabled'
    AND ((chipRun.state = 'full') OR (distTarget.clean AND chipRun.state = 'cleaned'))
