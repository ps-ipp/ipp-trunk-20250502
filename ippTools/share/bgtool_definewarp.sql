SELECT
    warpRun.warp_id,
    chipBackgroundRun.*
    -- the following items are selected for aid in debugging
    ,
    warpsChipRun.chip_id as warpschip_id,
    chipRun.chip_id AS thischip_id,
    warpBackgroundRun.warp_bg_id
FROM warpRun
JOIN fakeRun USING(fake_id)
JOIN camRun USING(cam_id)
JOIN chipRun as warpsChipRun ON camRun.chip_id = warpsChipRun.chip_id
JOIN rawExp USING(exp_id)
JOIN chipRun USING(exp_id)
JOIN chipBackgroundRun ON chipRun.chip_id = chipBackgroundRun.chip_id 
    AND (chipBackgroundRun.cam_id = camRun.cam_id OR chipBackgroundRun.cam_id = 0)
LEFT JOIN warpBackgroundRun ON chipBackgroundRun.chip_bg_id = warpBackgroundRun.chip_bg_id -- label hook %s
WHERE chipBackgroundRun.state = 'full'
    AND warpRun.state IN ('full', 'cleaned', 'goto_cleaned') -- need warp to have completed so warpSkyCellMap is populated
