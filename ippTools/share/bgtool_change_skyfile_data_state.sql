UPDATE warpBackgroundSkyfile
    JOIN warpBackgroundRun USING(warp_bg_id)
SET data_state = '%s',
    warpBackgroundSkyfile.magicked = IF(warpBackgroundSkyfile.magicked != 0, -1, 0)
WHERE warp_bg_id = %lld AND skycell_id = '%s'
