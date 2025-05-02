SELECT 
    'raw' AS stage,
    rawExp.exp_id AS stage_id,
    rawExp.exp_name AS run_tag,
    rawExp.magicked,
    camRun.label,
    camRun.data_group,
    distTarget.dist_group,
    distTarget.target_id,
    distTarget.clean
FROM rawExp
JOIN magicDSRun ON stage = 'raw' AND magicDSRun.stage_id = exp_id
JOIN camRun USING(cam_id)
JOIN distTarget ON distTarget.dist_group = camRun.dist_group AND distTarget.stage = 'raw'
    AND rawExp.filter = distTarget.filter
JOIN rcInterest USING(target_id)
LEFT JOIN distRun ON distRun.stage_id = exp_id AND distRun.target_id = distTarget.target_id
WHERE distTarget.state = 'enabled'    -- target and intrest are enabled
    AND rcInterest.state = 'enabled'
    AND magicDSRun.state = 'full'       -- destreaked files are available
