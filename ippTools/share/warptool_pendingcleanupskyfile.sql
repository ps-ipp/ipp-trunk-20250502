SELECT
    warpSkyfile.*,
    warpRun.state,
    warpRun.workdir,
    warpRun.label,
    warpRun.dvodb,
    warpRun.end_stage
FROM warpRun
JOIN warpSkyfile
    USING(warp_id)
WHERE
   ((warpRun.state = 'goto_cleaned'  
       AND (warpSkyfile.data_state = 'full'
          OR  warpSkyfile.data_state = 'error_cleaned'
          OR  warpSkyfile.data_state = 'update')
       AND warpSkyfile.quality = 0)
    OR
    (warpRun.state = 'goto_scrubbed' AND warpSkyfile.data_state != 'scrubbed')
    OR
    (warpRun.state = 'goto_purged'   AND warpSkyfile.data_state != 'purged'))
