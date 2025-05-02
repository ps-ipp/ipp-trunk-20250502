SELECT
    warpBackgroundSkyfile.*,
    warpBackgroundRun.state,
    warpBackgroundRun.workdir,
    warpBackgroundRun.label
FROM warpBackgroundRun
JOIN warpBackgroundSkyfile
    USING(warp_bg_id)
WHERE
   ((warpBackgroundRun.state = 'goto_cleaned' AND (warpBackgroundSkyfile.data_state = 'full'
                                       OR warpBackgroundSkyfile.data_state = 'update'))
OR 
   (warpBackgroundRun.state = 'goto_scrubbed' AND warpBackgroundSkyfile.data_state != 'scrubbed')
OR 
   (warpBackgroundRun.state = 'goto_purged' AND warpBackgroundSkyfile.data_state != 'purged'))
