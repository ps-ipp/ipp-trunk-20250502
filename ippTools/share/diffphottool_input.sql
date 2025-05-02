SELECT
    diff_phot_id,
    skycell_id,
    diffSkyfile.path_base,
    diffSkyfile.magicked,
    diffRun.bothways,
    rawExp.camera
FROM diffPhotRun
JOIN diffRun USING(diff_id)
JOIN diffSkyfile USING(diff_id)
JOIN diffInputSkyfile USING(diff_id, skycell_id)
JOIN warpRun ON warp1 = warp_id -- only JOINing on input warps!
JOIN fakeRun USING(fake_id)
JOIN camRun USING(cam_id)
JOIN chipRun USING(chip_id)
JOIN rawExp USING(exp_id)
