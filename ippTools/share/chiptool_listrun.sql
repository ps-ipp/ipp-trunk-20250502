SELECT DISTINCT
    chipRun.chip_id,
    chipRun.state,
    chipRun.workdir,
    chipRun.label,
    chipRun.data_group,
    chipRun.dist_group,
    chipRun.note,
    chipRun.magicked,
    chipRun.reduction,
    camRun.cam_id,
    camRun.state AS camRun_state,
    camProcessedExp.path_base AS cam_path_base,
    rawExp.exp_id,
    rawExp.exp_name,
    rawExp.camera,
    rawExp.filter,
    rawExp.dateobs,
    rawExp.ra,
    rawExp.decl,
    rawExp.exp_time,
    rawExp.magicked AS raw_magicked,
    magicDSRun.state AS dsRun_state,
    IFNULL(magicDSRun.magic_ds_id, 0) AS magic_ds_id
FROM chipRun
JOIN rawExp
    USING(exp_id)
LEFT JOIN camRun USING(chip_id)
LEFT JOIN camProcessedExp USING(cam_id)
LEFT JOIN magicDSRun
    ON stage_id = chip_id AND stage = 'chip' AND magicDSRun.re_place AND magicDSRun.state != 'drop'
