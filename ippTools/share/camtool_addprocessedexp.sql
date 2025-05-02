SELECT
    camRun.*,
    rawExp.exp_tag,
    rawExp.exp_id,
    rawExp.exp_name,
    rawExp.camera,
    rawExp.telescope,
    rawExp.filelevel,
    chipRun.magicked AS chip_magicked,
    camProcessedExp.path_base
FROM camRun
JOIN chipRun
    USING(chip_id)
JOIN rawExp
    USING(exp_id)
LEFT JOIN camProcessedExp
    USING(cam_id)
LEFT JOIN camMask
    ON camRun.label = camMask.label
WHERE
    chipRun.state = 'full'
    AND ((camRun.state = 'new' AND camProcessedExp.cam_id IS NULL) OR camRun.state = 'update')
    AND camMask.label IS NULL
