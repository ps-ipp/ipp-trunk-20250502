SELECT DISTINCT
    diffRun.*
FROM diffRun
JOIN diffInputSkyfile USING(diff_id)
JOIN warpRun ON warpRun.warp_id = diffInputSkyfile.warp1 -- only JOINing inputs, not templates!
JOIN fakeRun USING(fake_id)
JOIN camRun USING(cam_id)
JOIN chipRun USING(chip_id)
JOIN rawExp USING(exp_id)
WHERE diffRun.state = 'full'
