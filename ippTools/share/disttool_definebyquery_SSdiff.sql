SELECT DISTINCT
       'SSdiff' as stage,
       diffRun.diff_id AS stage_id,
       diffRun.magicked,           
       CONCAT_WS('.','SSdiff',convert(diffRun.diff_id,CHAR)) as run_tag,
       diffRun.label,
       diffRun.data_group,
       distTarget.dist_group,
       distTarget.target_id,
       distTarget.clean
FROM diffRun
JOIN diffInputSkyfile using(diff_id)
JOIN stackRun ON diffInputSkyfile.stack1 = stackRun.stack_id
JOIN distTarget ON distTarget.stage = 'SSdiff'
     AND distTarget.filter = stackRun.filter
     AND distTarget.dist_group = diffRun.dist_group
JOIN rcInterest USING(target_id)
LEFT JOIN distRun ON (distRun.stage_id = diff_id)
     	  	  AND distTarget.target_id = distRun.target_id
    -- JOIN hook %s
WHERE diffInputSkyfile.stack1 IS NOT NULL AND diffInputSkyfile.stack2 IS NOT NULL
      AND distTarget.state = 'enabled'
      AND rcInterest.state = 'enabled'
      AND ((diffRun.state = 'full') OR (distTarget.clean AND diffRun.state = 'cleaned'))
