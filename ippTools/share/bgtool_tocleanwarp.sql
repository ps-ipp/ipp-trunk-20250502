SELECT
    warpBackgroundRun.warp_bg_id,
    warpBackgroundRun.state,
    rawExp.camera
FROM warpBackgroundRun
JOIN warpRun USING(warp_id)
JOIN fakeRun USING(fake_id)
JOIN camRun USING(cam_id)
JOIN chipRun USING(chip_id)
JOIN rawExp USING(exp_id)
WHERE warpBackgroundRun.state IN ('goto_cleaned', 'goto_scrubbed', 'goto_purged')

