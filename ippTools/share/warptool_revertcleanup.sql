UPDATE warpSkyfile
    JOIN warpRun using(warp_id)
SET warpSkyfile.data_state = 'full', 
    warpRun.state = '%s'
WHERE
    warpRun.state = '%s'
    AND warpSkyfile.data_state = '%s'
