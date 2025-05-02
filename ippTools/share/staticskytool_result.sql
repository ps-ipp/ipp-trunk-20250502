SELECT
    staticskyResult.*,
    staticskyRun.state,
    staticskyRun.workdir,
    staticskyRun.label,
    stackRun.tess_id,
    stackRun.skycell_id,
    count(stackRun.filter) AS num_filters,
    TRUNCATE(skycell.radeg/15., 4) AS rahours,
    skycell.radeg,
    skycell.decdeg,
    skycell.glong,
    skycell.glat
FROM staticskyRun
JOIN staticskyResult USING(sky_id)
JOIN staticskyInput USING(sky_id)
JOIN stackRun USING(stack_id)
LEFT JOIN skycell USING(tess_id, skycell_id)
