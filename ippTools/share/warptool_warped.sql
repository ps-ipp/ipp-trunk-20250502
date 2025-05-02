SELECT
    warpSkyfile.*,
    warpRun.state,
    warpRun.workdir,
    warpRun.label,
    warpRun.data_group,
    warpImfile.warp_skyfile_id,
    rawExp.exp_id,
    rawExp.exp_name,
    rawExp.camera,
    rawExp.filter,
    rawExp.dateobs,
    rawExp.ra,
    rawExp.decl,
    rawExp.exp_time,
    camRun.cam_id,
    magicDSRun.state AS dsRun_state,
    IFNULL(magicDSRun.magic_ds_id, 0) AS magic_ds_id
FROM warpRun
JOIN warpSkyfile
    USING(warp_id)
JOIN warpImfile
    USING(warp_id, skycell_id)
JOIN fakeRun
    ON warpRun.fake_id = fakeRun.fake_id
JOIN camRun
    ON camRun.cam_id   = fakeRun.cam_id
JOIN chipRun
    ON camRun.chip_id  = chipRun.chip_id
JOIN rawExp
    ON chipRun.exp_id  = rawExp.exp_id
LEFT JOIN magicDSRun
    ON stage = 'warp' AND stage_id = warp_id
