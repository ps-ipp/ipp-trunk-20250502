SELECT DISTINCT
    'warp_bg' as stage,
    warpBackgroundRun.warp_bg_id AS stage_id,
    warpBackgroundRun.magicked,
    rawExp.exp_name as run_tag,
    warpBackgroundRun.label,
    warpBackgroundRun.data_group,
    distTarget.dist_group,
    distTarget.target_id,
    distTarget.clean
FROM warpBackgroundRun
JOIN warpRun USING(warp_id)
JOIN fakeRun USING(fake_id)
JOIN camRun ON fakeRun.cam_id = camRun.cam_id
JOIN chipRun USING(chip_id)
JOIN rawExp USING(exp_id)
JOIN distTarget
    ON distTarget.stage = 'warp_bg'
    AND warpBackgroundRun.dist_group = distTarget.dist_group
    AND rawExp.filter = distTarget.filter
JOIN rcInterest USING(target_id)
LEFT JOIN distRun ON (distRun.stage_id = warp_bg_id)
    AND distRun.target_id = distTarget.target_id
WHERE  distTarget.state = 'enabled'
    AND rcInterest.state = 'enabled'
    AND ((warpBackgroundRun.state = 'full') OR (distTarget.clean AND warpBackgroundRun.state = 'cleaned'))



