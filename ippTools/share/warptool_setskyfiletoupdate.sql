UPDATE warpRun
    JOIN warpSkyfile USING(warp_id)
SET warpRun.state = 'update', 
    warpSkyfile.data_state = 'update',
    warpSkyfile.fault = 0
    -- set hook %s
WHERE warp_id = %lld
    AND (warpRun.state = 'cleaned' OR warpRun.state = 'update')
    AND (warpSkyfile.data_state = 'cleaned' OR warpSkyfile.data_state = 'update')
    AND (warpSkyfile.quality = 0)
