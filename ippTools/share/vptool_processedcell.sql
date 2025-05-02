SELECT
    vpProcessedCell.*,
    vpRun.state,
    vpRun.label,
    vpRun.data_group,
    vpRun.outroot,
    rawExp.exp_id,
    rawExp.exp_tag,
    rawExp.exp_name,
    rawExp.filter,
    rawExp.camera,
    rawExp.telescope,
    rawExp.filelevel,
    rawExp.dateobs,
    rawExp.obs_mode,
    rawExp.comment,
    rawExp.ra,
    rawExp.decl,
    rawExp.exp_time
FROM vpRun 
    JOIN vpProcessedCell USING(vp_id)
    JOIN rawExp USING(exp_id)
