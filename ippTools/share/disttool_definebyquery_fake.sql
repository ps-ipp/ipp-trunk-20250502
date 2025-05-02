SELECT DISTINCT
    'fake' as stage,
    fakeRun.fake_id AS stage_id,
    CAST(0 AS SIGNED) AS magicked,
    rawExp.exp_name as run_tag,
    fakeRun.label,
    fakeRun.data_group,
    distTarget.dist_group,
    distTarget.target_id,
    distTarget.clean
FROM fakeRun
JOIN camRun USING(cam_id)
JOIN chipRun USING(chip_id)
JOIN rawExp USING(exp_id)
JOIN distTarget ON distTarget.stage = 'fake' AND fakeRun.dist_group = distTarget.dist_group
    AND rawExp.filter = distTarget.filter
JOIN rcInterest USING(target_id)
LEFT JOIN distRun ON (distRun.stage_id = fake_id)
    AND distRun.target_id = distTarget.target_id
WHERE  distTarget.state = 'enabled'
    AND rcInterest.state = 'enabled'
    AND ((fakeRun.state = 'full') OR (distTarget.clean AND fakeRun.state = 'cleaned'))
