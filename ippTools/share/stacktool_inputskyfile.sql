SELECT
    warpSkyfile.*,
    rawExp.camera,
    rawExp.exp_id,
    rawExp.exp_name,
    rawExp.exp_time,
    rawExp.object,
    rawExp.dateobs,
    rawExp.comment
FROM stackRun
JOIN stackInputSkyfile USING(stack_id)
JOIN warpRun USING(warp_id)
JOIN warpSkyfile
    ON warpSkyfile.warp_id = warpRun.warp_id
    AND warpSkyfile.tess_id = stackRun.tess_id
    AND warpSkyfile.skycell_id = stackRun.skycell_id
JOIN fakeRun USING(fake_id)
JOIN camRun USING(cam_id)
JOIN chipRun USING(chip_id)
JOIN rawExp USING(exp_id)
