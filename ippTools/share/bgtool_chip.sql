SELECT
    chipBackgroundImfile.*,
    chipBackgroundRun.state,
    chipBackgroundRun.workdir,
    chipBackgroundRun.label,
    chipBackgroundRun.data_group,
    chipBackgroundRun.chip_id,
    chipBackgroundRun.cam_id,
    rawExp.exp_id,
    rawExp.exp_name,
    rawExp.camera,
    rawExp.filter,
    rawExp.dateobs,
    rawExp.ra,
    rawExp.decl,
    rawExp.exp_time
FROM chipBackgroundRun
JOIN chipBackgroundImfile USING(chip_bg_id)
JOIN chipRun USING(chip_id)
JOIN rawExp USING(exp_id)
