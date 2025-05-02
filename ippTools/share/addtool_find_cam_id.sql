SELECT
    camRun.*
FROM camRun
JOIN camProcessedExp
    USING(cam_id)
JOIN chipRun
    USING(chip_id)
JOIN rawExp
    USING(exp_id)
LEFT JOIN (SELECT exp_id       AS added_exp_id,
                  addRun.dvodb AS previous_dvodb
           FROM addRun
           JOIN camRun on cam_id=stage_id
	   JOIN chipRun USING(chip_id)
          ) as foo
     ON exp_id = added_exp_id
     AND stage = 'cam'
     -- hook for qualifying the join on the previous_dvodb
     AND %s
WHERE
    camRun.state = 'full'
    AND camProcessedExp.quality = 0
    AND added_exp_id IS NULL
    -- addtool adds checks on exposure being added to the dvodb previously
