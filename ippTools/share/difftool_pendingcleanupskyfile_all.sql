SELECT
    diffSkyfile.*,
    diffRun.state,
    diffRun.workdir,
    diffRun.dvodb,
    diffRun.tess_id
FROM diffRun
JOIN diffSkyfile
    USING(diff_id)
WHERE ( diffRun.state = 'goto_cleaned' OR diffRun.state = 'goto_scrubbed' OR diffRun.state = 'goto_purged')
