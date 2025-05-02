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
WHERE (warpRun.state = 'goto_cleaned' OR warpRun.state = 'goto_scrubbed' OR warpRun.state = 'goto_purged')
