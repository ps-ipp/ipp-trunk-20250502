DELETE FROM camProcessedExp
USING camProcessedExp, camRun, chipRun, rawExp
WHERE
    camRun.cam_id = camProcessedExp.cam_id
    AND camRun.chip_id = chipRun.chip_id
    AND chipRun.exp_id = rawExp.exp_id
    AND camProcessedExp.fault != 0
    AND camRun.state = 'new'
