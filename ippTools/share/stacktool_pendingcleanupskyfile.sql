SELECT
    stackSumSkyfile.*,
    stackRun.state,
    stackRun.workdir,
    stackRun.dvodb,
    stackRun.tess_id,
    stackRun.skycell_id
FROM stackRun
JOIN stackSumSkyfile
    USING(stack_id)
WHERE
    (stackRun.state = 'goto_cleaned' OR stackRun.state = 'goto_scrubbed' OR stackRun.state = 'goto_purged')
