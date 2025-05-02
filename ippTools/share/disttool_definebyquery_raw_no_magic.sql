SELECT 
    'raw' AS stage,
    rawExp.exp_id AS stage_id,
    rawExp.exp_name AS run_tag,
    rawExp.magicked,
    CAST(NULL AS CHAR(255)) AS label,
    CAST(NULL AS CHAR(255)) AS data_group,
    distTarget.dist_group,
    distTarget.target_id,
    distTarget.clean
FROM rawExp
JOIN distTarget ON distTarget.stage = 'raw'
    AND rawExp.filter = distTarget.filter
JOIN rcInterest USING(target_id)
LEFT JOIN distRun ON distRun.stage_id = exp_id AND distRun.target_id = distTarget.target_id
WHERE distTarget.state = 'enabled'    -- target and intrest are enabled
    AND rcInterest.state = 'enabled'
