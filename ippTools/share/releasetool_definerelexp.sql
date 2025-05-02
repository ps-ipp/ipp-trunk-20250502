SELECT
    ippRelease.rel_id,
    ippRelease.release_name,
    rawExp.exp_name,
    rawExp.filter,
    rawExp.dateobs,
    rawExp.exp_id,
    camRun.chip_id,
    camRun.cam_id,
    camProcessedExp.zpt_obs,
    camProcessedExp.zpt_stdev
FROM ippRelease
JOIN camRun
JOIN camProcessedExp USING(cam_id)
JOIN chipRun USING(chip_id)
JOIN rawExp USING(exp_id)
LEFT JOIN relExp AS previousrelexp USING(exp_id, rel_id) 

WHERE previousrelexp.relexp_id IS NULL
    AND camRun.state = 'full'
