DELETE FROM warpSkyCellMap
USING warpSkyCellMap, warpRun, fakeRun, camRun, chipRun, rawExp
WHERE warpSkyCellMap.warp_id = warpRun.warp_id
    AND warpRun.fake_id = fakeRun.fake_id
    AND fakeRun.cam_id = camRun.cam_id
    AND camRun.chip_id = chipRun.chip_id
    AND chipRun.exp_id = rawExp.exp_id
    AND warpRun.state = 'new'
    AND warpSkyCellMap.fault != 0

