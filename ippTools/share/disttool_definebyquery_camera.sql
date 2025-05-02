SELECT DISTINCT
    'camera' as stage,
    camRun.cam_id as stage_id,
    rawExp.exp_name as run_tag,
    camRun.magicked,
    camRun.label,
    camRun.data_group,
    distTarget.dist_group,
    distTarget.target_id,
    distTarget.clean
FROM camRun 
JOIN chipRun USING(chip_id)
JOIN rawExp USING(exp_id)
JOIN distTarget ON distTarget.stage = 'camera'
    AND rawExp.filter = distTarget.filter
    AND camRun.dist_group  = distTarget.dist_group
JOIN rcInterest USING(target_id)
LEFT JOIN distRun ON camRun.cam_id = distRun.stage_id
    AND distRun.target_id = distTarget.target_id
    -- JOIN hook %s
WHERE distTarget.state = 'enabled'
    AND rcInterest.state = 'enabled'
    AND ((camRun.state = 'full') OR (distTarget.clean AND camRun.state = 'cleaned'))
