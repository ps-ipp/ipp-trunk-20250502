SELECT DISTINCT
    'diff' as stage,
    diffRun.diff_id AS stage_id,
    diffRun.magicked,
    rawExp.exp_name as run_tag,
    diffRun.label,
    diffRun.data_group,
    distTarget.dist_group,
    distTarget.target_id,
    distTarget.clean
FROM diffRun
JOIN diffInputSkyfile using(diff_id)
JOIN warpRun on diffInputSkyfile.warp1 = warpRun.warp_id
JOIN fakeRun USING(fake_id)
JOIN camRun USING(cam_id)
JOIN chipRun USING(chip_id)
JOIN rawExp USING(exp_id)
JOIN distTarget ON distTarget.stage = 'diff'
    AND distTarget.filter = rawExp.filter
    AND distTarget.dist_group = diffRun.dist_group
JOIN rcInterest USING(target_id)
LEFT JOIN distRun ON (distRun.stage_id = diff_id)
                  AND distTarget.target_id = distRun.target_id
    -- JOIN hook %s
WHERE  distTarget.state = 'enabled'
    AND rcInterest.state = 'enabled'
    AND ((diffRun.state = 'full') OR (distTarget.clean AND diffRun.state = 'cleaned'))



