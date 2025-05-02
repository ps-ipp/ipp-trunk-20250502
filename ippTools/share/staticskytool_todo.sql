SELECT
    staticskyRun.sky_id,
    staticskyRun.workdir,
    staticskyRun.reduction,
    staticskyRun.label,
    staticskyRun.state,
    stackRun.tess_id,
    stackRun.skycell_id,
    TRUNCATE(skycell.radeg/15., 4) as rahours,
    skycell.radeg,
    skycell.decdeg,
    skycell.glong,
    skycell.glat,
    staticskyResult.path_base,
    IFNULL(Label.priority, 10000) AS priority
FROM staticskyRun
JOIN staticskyInput USING (sky_id)
JOIN stackRun USING (stack_id)
JOIN skycell USING(tess_id, skycell_id)
LEFT JOIN staticskyResult USING(sky_id)
LEFT JOIN Label ON staticskyRun.label = Label.label
WHERE
   ((staticskyRun.state = 'new' AND staticskyResult.sky_id IS NULL)
    OR (staticskyRun.state = 'update' AND staticskyResult.fault = 0))
    
   AND (Label.active OR Label.active IS NULL)

    -- WHERE hook %s
GROUP BY sky_id
HAVING SUM(IF(stackRun.state = 'full', 1, 0)) = COUNT(staticskyInput.sky_id)
ORDER BY priority DESC, sky_id
