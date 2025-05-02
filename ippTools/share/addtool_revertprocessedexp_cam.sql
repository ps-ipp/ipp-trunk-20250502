DELETE FROM addProcessedExp
USING addProcessedExp, addRun, camRun, chipRun, rawExp
WHERE
    addRun.add_id = addProcessedExp.add_id
    AND addRun.stage_id = camRun.cam_id
    AND camRun.chip_id = chipRun.chip_id
    AND chipRun.exp_id = rawExp.exp_id
    AND addProcessedExp.fault != 0
    AND addRun.state = 'new'
    AND addRun.stage = 'cam'
