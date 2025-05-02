SELECT
    staticskyRun.sky_id,
    stackRun.stack_id,
    skycell_id,
    filter,
    staticskyRun.workdir,
    staticskyRun.label,
    staticskyRun.data_group,
    staticskyRun.dist_group
FROM staticskyRun 
    JOIN staticskyResult USING(sky_id)
    JOIN staticskyInput USING(sky_id)
    JOIN stackRun USING(stack_id)
    JOIN stackSumSkyfile USING(stack_id)
    JOIN skycell USING(tess_id, skycell_id)
    LEFT JOIN skycalRun ON staticskyRun.sky_id = skycalRun.sky_id AND stackRun.stack_id = skycalRun.stack_id -- join hook %s
WHERE staticskyRun.state = 'full'
    AND staticskyResult.quality = 0

