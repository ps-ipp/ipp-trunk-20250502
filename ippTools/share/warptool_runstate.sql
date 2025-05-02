SELECT 
    warp_id,
    warpRun.label,
    warpRun.state,
    warpRun.magicked,
    chip_id,
    chipRun.state AS chip_state,
    exp_id,
    magic_ds_id,
    magicDSRun.state AS magic_ds_state
FROM warpRun 
    JOIN fakeRun USING(fake_id) 
    JOIN camRun USING(cam_id) 
    JOIN chipRun USING(chip_id) 
    JOIN rawExp USING(exp_id)
    LEFT JOIN magicDSRun ON stage = 'warp' AND stage_id = warp_id AND re_place
