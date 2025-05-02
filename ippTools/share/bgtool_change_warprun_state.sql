UPDATE warpBackgroundRun
SET state = '%s',
    warpBackgroundRun.magicked = IF(warpBackgroundRun.magicked != 0, -1, 0)
WHERE warp_bg_id = %lld
AND (SELECT COUNT(skycell_id)
    FROM warpBackgroundSkyfile 
    WHERE 
        warpBackgroundSkyfile.warp_bg_id = warpBackgroundRun.warp_bg_id
        AND data_state != '%s'
    ) = 0
