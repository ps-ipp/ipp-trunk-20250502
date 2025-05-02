UPDATE camProcessedExp
    JOIN camRun USING(cam_id)
    JOIN chipRun USING(chip_id)
    JOIN rawExp USING(exp_id)
SET camProcessedExp.fault = 0
WHERE
    camRun.state = 'update'
    AND camProcessedExp.fault != 0
