SELECT
    chipBackgroundImfile.path_base AS chip_path_base,
    chipBackgroundImfile.class_id,
    chipBackgroundImfile.magicked,
    camProcessedExp.path_base AS cam_path_base
FROM warpBackgroundRun
JOIN warpRun USING(warp_id)
JOIN warpSkyCellMap USING(warp_id)
JOIN warpSkyfile USING(warp_id, skycell_id)
JOIN chipBackgroundRun USING(chip_bg_id)
JOIN chipBackgroundImfile USING(chip_bg_id, class_id)
JOIN fakeRun USING(fake_id)
JOIN camProcessedExp ON camProcessedExp.cam_id = fakeRun.cam_id
WHERE warpRun.state IN ('full', 'cleaned', 'goto_cleaned') -- only need it to have been completed
    AND warpSkyfile.fault = 0
    AND warpSkyfile.quality = 0
    AND chipBackgroundRun.state = 'full'
    AND chipBackgroundImfile.fault = 0
    AND chipBackgroundImfile.quality = 0
