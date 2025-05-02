SELECT
    diffSkyfile.*,
    diffRun.state,
    diffRun.workdir,
    diffRun.dvodb,
    diffRun.tess_id
FROM diffRun
JOIN diffSkyfile
    USING(diff_id)
WHERE
   ((diffRun.state = 'goto_cleaned'  AND (diffSkyfile.data_state = 'full' OR diffSkyfile.data_state = 'update') AND diffSkyfile.quality = 0)
    OR
    (diffRun.state = 'goto_scrubbed' AND diffSkyfile.data_state != 'scrubbed')
    OR
    (diffRun.state = 'goto_purged'   AND diffSkyfile.data_state != 'purged'))
