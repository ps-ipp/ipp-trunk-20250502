SELECT
    addProcessedExp.*,
    addRun.label,
    addRun.workdir,
    camRun.cam_id,
    camRun.label as cam_label,
    camRun.data_group as cam_data_group,
    rawExp.exp_id,
    exp_name
FROM addProcessedExp
JOIN addRun
    USING(add_id)
JOIN camRun
    on cam_id = stage_id
JOIN chipRun
    USING(chip_id)
JOIN rawExp
    ON chipRun.exp_id = rawExp.exp_id
