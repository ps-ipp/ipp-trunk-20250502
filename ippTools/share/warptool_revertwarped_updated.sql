UPDATE warpSkyfile
    JOIN warpRun USING(warp_id)
    JOIN fakeRun USING(fake_id)
    JOIN camRun USING(cam_id)
    JOIN chipRun USING(chip_id)
    JOIN rawExp USING(exp_id)
SET warpSkyfile.fault = 0
WHERE warpRun.state = 'update'
    AND warpSkyfile.data_state = 'update'
    AND warpSkyfile.fault != 0
