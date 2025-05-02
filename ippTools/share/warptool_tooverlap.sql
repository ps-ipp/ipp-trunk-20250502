SELECT
    warpRun.warp_id,
    warpRun.fake_id,
    warpRun.workdir,
    warpRun.tess_id,
    warpRun.label,
    rawExp.camera,
    exp_id,
    exp_tag
FROM warpRun
JOIN fakeRun
    USING(fake_id)
JOIN camRun
    USING(cam_id)
JOIN chipRun
    USING(chip_id)
JOIN rawExp
    USING(exp_id)
LEFT JOIN warpSkyCellMap
    USING(warp_id)
LEFT JOIN warpMask
    ON warpRun.label = warpMask.label
WHERE
    warpRun.state = 'new'
    AND fakeRun.state = 'full'
    AND camRun.state = 'full'
    AND chipRun.state = 'full'
    AND warpSkyCellMap.warp_id IS NULL
    AND warpMask.label IS NULL
