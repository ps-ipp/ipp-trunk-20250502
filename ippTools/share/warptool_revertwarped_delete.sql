DELETE FROM warpSkyfile
USING warpSkyfile, warpRun, fakeRun, camRun, chipRun, rawExp
WHERE warpSkyfile.warp_id = warpRun.warp_id
    AND warpRun.fake_id = fakeRun.fake_id
    AND fakeRun.cam_id = camRun.cam_id
    AND camRun.chip_id = chipRun.chip_id
    AND chipRun.exp_id = rawExp.exp_id
    AND warpRun.state = 'new'
    AND warpSkyfile.fault != 0

