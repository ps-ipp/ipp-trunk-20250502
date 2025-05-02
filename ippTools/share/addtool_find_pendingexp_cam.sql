SELECT
    addRun.*,
    camProcessedExp.path_base as stageroot,
    rawExp.exp_tag,
    rawExp.exp_id,
    rawExp.exp_name,
    rawExp.camera,
    rawExp.telescope,
    rawExp.filelevel,
    minidvodbRun.minidvodb_host as addrun_host
FROM addRun
JOIN minidvodbRun
    on addRun.minidvodb_group = minidvodbRun.minidvodb_group
JOIN camRun
    on cam_id = stage_id
JOIN camProcessedExp
    USING(cam_id)
JOIN chipRun
    USING(chip_id)
JOIN rawExp
    USING(exp_id)
LEFT JOIN addProcessedExp
    USING(add_id)
LEFT JOIN addMask
    ON addRun.label = addMask.label
WHERE
    camRun.state = 'full' 
    AND addRun.stage = 'cam' 
    AND ((addRun.state = 'new' AND addProcessedExp.add_id IS NULL) OR addRun.state = 'update')
    AND addRun.dvodb IS NOT NULL
    AND addRun.workdir IS NOT NULL
    AND addMask.label IS NULL
    AND minidvodbRun.state = 'active'

