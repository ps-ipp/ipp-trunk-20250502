SELECT
    warpBackgroundSkyfile.*,
    warpBackgroundRun.state,
    warpBackgroundRun.warp_id,
    camRun.cam_id,
    warpBackgroundRun.chip_bg_id,
    warpBackgroundRun.workdir,
    warpBackgroundRun.label,
    warpRun.label as warp_label,
    rawExp.exp_id,
    rawExp.exp_name,
    rawExp.camera,
    rawExp.filter,
    rawExp.dateobs,
    rawExp.ra,
    rawExp.decl,
    rawExp.exp_time
FROM warpBackgroundRun
JOIN warpBackgroundSkyfile USING(warp_bg_id)
JOIN warpRun USING(warp_id)
JOIN fakeRun USING(fake_id)
JOIN camRun USING(cam_id)
JOIN chipRun USING(chip_id)
JOIN rawExp USING(exp_id)
