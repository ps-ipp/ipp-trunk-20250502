SELECT camRun.* FROM camRun
JOIN camProcessedExp USING(cam_id)
JOIN chipRun USING(chip_id)
JOIN rawExp USING(exp_id)
WHERE camRun.state = 'full' and camProcessedExp.quality = 0
    AND exp_id NOT IN (SELECT exp_id
       FROM addRun
       JOIN camRun on cam_id=stage_id
       JOIN chipRun USING(chip_id)
       WHERE stage = 'cam' AND %s
      )
