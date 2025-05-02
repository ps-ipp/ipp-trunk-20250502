SELECT
    fpcamProcessedExp.*,
    fpcamRun.workdir,
    fpcamRun.label,
    fpcamRun.data_group,
    fpcamRun.state,
    camRun.state as cam_state,
    chipRun.state as chip_state,
    rawExp.exp_tag,
    rawExp.exp_name,
    rawExp.exp_time,
    rawExp.exp_id,
    rawExp.camera,
    rawExp.telescope,
    rawExp.dateobs,
    rawExp.filter,
    rawExp.filelevel,
    rawExp.comment
FROM fpcamRun
JOIN fpcamProcessedExp
    USING(fpcam_id)
JOIN camRun
    USING(cam_id)
JOIN chipRun
    ON(fpcamRun.chip_id = chipRun.chip_id)
JOIN rawExp
    ON chipRun.exp_id = rawExp.exp_id
