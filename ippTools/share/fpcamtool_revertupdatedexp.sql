UPDATE fpcamProcessedExp
    JOIN fpcamRun USING(fpcam_id)
    JOIN camRun USING(cam_id)
    JOIN chipRun ON(fpcamRun.chip_id = chipRun.chip_id)
    JOIN rawExp USING(exp_id)
SET fpcamProcessedExp.fault = 0
WHERE
    fpcamRun.state = 'update'
    AND fpcamProcessedExp.fault != 0
