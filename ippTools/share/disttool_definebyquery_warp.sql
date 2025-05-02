SELECT DISTINCT
    'warp' as stage,
    warpRun.warp_id AS stage_id,
    warpRun.magicked,
    rawExp.exp_name as run_tag,
    warpRun.label,
    warpRun.data_group,
    distTarget.dist_group,
    distTarget.target_id,
    distTarget.clean
FROM warpRun
JOIN fakeRun USING(fake_id)
JOIN camRun ON fakeRun.cam_id = camRun.cam_id
JOIN chipRun USING(chip_id)
JOIN rawExp USING(exp_id)
JOIN distTarget ON distTarget.stage = 'warp' 
    AND warpRun.dist_group = distTarget.dist_group
    AND rawExp.filter = distTarget.filter
JOIN rcInterest USING(target_id)
LEFT JOIN distRun ON (distRun.stage_id = warp_id)
    AND distRun.target_id = distTarget.target_id
WHERE  distTarget.state = 'enabled'
    AND rcInterest.state = 'enabled'
    AND ((warpRun.state = 'full') OR (distTarget.clean AND warpRun.state = 'cleaned'))



