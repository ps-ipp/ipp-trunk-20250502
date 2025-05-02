SELECT
    relexp_id,
    relExp.state,
    relExp.fault,
    ippRelease.release_name,
    ippRelease.release_state,
    ippRelease.rel_id,
    ippRelease.priority,
    ippRelease.dvodb,
    ippRelease.ubercal_file,
    survey.surveyName,
    rawExp.exp_name,
    relExp.flags,
    relExp.zpt_obs,
    relExp.zpt_stdev,
    relExp.mcal,
    relExp.ubercal_dist,
    relExp.path_base,
    relExp.registered,
    relExp.time_stamp,
    relExp.exp_id,
    relExp.chip_id,
    chipRun.state as chip_state,
    relExp.cam_id,
    camRun.state as cam_state,
    camProcessedExp.path_base as cam_path_base,
    camProcessedExp.fwhm_major,
    camProcessedExp.fwhm_minor,
    warpRun.warp_id,
    warpRun.state as warp_state,
    warpRun.tess_id,
    rawExp.filter,
    rawExp.dateobs,
    TRUNCATE(DEGREES(rawExp.ra), 4) AS radeg,
    TRUNCATE(DEGREES(rawExp.decl), 4) AS decdeg,
    rawExp.comment
FROM relExp 
JOIN ippRelease USING(rel_id) 
JOIN survey USING(surveyID)
JOIN rawExp using(exp_id)
JOIN chipRun using(chip_id, exp_id)
JOIN camRun using(cam_id, chip_id)
JOIN camProcessedExp using(cam_id)
JOIN fakeRun using(cam_id)
JOIN warpRun using(fake_id)
