DELETE FROM fpcamProcessedExp
USING fpcamProcessedExp, fpcamRun, camRun, chipRun, rawExp
WHERE
    fpcamRun.fpcam_id      = fpcamProcessedExp.fpcam_id
    AND fpcamRun.cam_id  = camRun.cam_id
    AND fpcamRun.chip_id = chipRun.chip_id
    AND chipRun.exp_id   = rawExp.exp_id
    AND fpcamProcessedExp.fault != 0
    AND fpcamRun.state = 'new'
