SELECT
     skycalRun.*
FROM 
     skycalRun
JOIN skycalResult using (skycal_id)
JOIN staticskyInput using (sky_id, stack_id)
JOIN staticskyRun using (sky_id)

LEFT JOIN (SELECT skycal_id       AS added_skycal_id,
                  addRun.dvodb AS previous_dvodb
           FROM addRun
JOIN skycalRun on skycal_id = stage_id
          ) as foo
     ON skycal_id = added_skycal_id 
     AND stage = 'skycal'
     -- hook for qualifying the join on the previous_dvodb
     AND %s
WHERE
    skycalRun.state = 'full'
    AND skycalResult.quality = 0
    AND added_skycal_id IS NULL
    -- addtool adds checks on exposure being added to the dvodb previously
