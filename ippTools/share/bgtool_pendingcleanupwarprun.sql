SELECT warpBackgroundRun.*,
    CONCAT_WS('.', exp_name, exp_id) AS exp_tag,
    rawExp.camera
FROM warpBackgroundRun
    JOIN warpRun USING(warp_id)
    JOIN fakeRun USING(fake_id)
    JOIN camRun USING(cam_id)
    JOIN chipRun USING(chip_id)
    JOIN rawExp USING(exp_id)
WHERE (warpBackgroundRun.state = 'goto_cleaned'
        OR warpBackgroundRun.state = 'goto_purged' 
        OR warpBackgroundRun.state = 'goto_scrubbed')

