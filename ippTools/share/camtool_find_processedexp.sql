SELECT
    camProcessedExp.*,
    HEX(IFNULL(camProcessedExp.astrom_chips, 0)) AS good_astrom_chips,
    camRun.workdir,
    camRun.label,
    camRun.data_group,
    camRun.state,
    camRun.magicked,
    chipRun.chip_id,
    chipRun.state as chip_state,
    chipRun.magicked as chip_magicked,
    rawExp.exp_tag,
    rawExp.exp_name,
    rawExp.exp_time,
    rawExp.exp_id,
    rawExp.camera,
    rawExp.telescope,
    rawExp.dateobs,
    rawExp.filter,
    rawExp.filelevel,
    rawExp.magicked as raw_magicked,
    rawExp.comment
FROM camRun
JOIN camProcessedExp
    USING(cam_id)
JOIN chipRun
    USING(chip_id)
JOIN rawExp
    ON chipRun.exp_id = rawExp.exp_id
