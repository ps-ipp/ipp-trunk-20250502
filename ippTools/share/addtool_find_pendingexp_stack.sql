SELECT
    addRun.*,
    stackSumSkyfile.path_base as stageroot,
    rawExp.camera,
    rawExp.telescope
FROM addRun
JOIN stackRun
    ON stack_id = stage_id
JOIN stackSumSkyfile
    USING(stack_id)
JOIN stackInputSkyfile 
     USING(stack_id)
JOIN warpRun using(warp_id)
JOIN fakeRun using(fake_id)
JOIN camRun using(cam_id)
JOIN chipRun using(chip_id)
JOIN rawExp using (exp_id)
LEFT JOIN addProcessedExp using (add_id)
LEFT JOIN addMask
    ON addRun.label = addMask.label
WHERE
    stackRun.state = 'full'
    AND stage = 'stack'
    AND ((addRun.state = 'new' AND addProcessedExp.add_id IS NULL) OR addRun.state = 'update')
    AND addRun.dvodb IS NOT NULL
    AND addRun.workdir IS NOT NULL
    AND addMask.label IS NULL

