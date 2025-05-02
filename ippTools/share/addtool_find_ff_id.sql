SELECT
     fullForceRun.*, fullForceResult.warp_id
FROM 
     fullForceRun
JOIN fullForceResult using (ff_id)

LEFT JOIN (SELECT ff_id       AS added_ff_id,
                  addRun.dvodb AS previous_dvodb
           FROM addRun
JOIN fullForceRun on (ff_id = stage_id and addRun.stage = 'fullforce')
          ) as foo
     ON ff_id = added_ff_id 
     --AND stage = 'skycal'
     -- hook for qualifying the join on the previous_dvodb
     AND %s
WHERE
    fullForceRun.state = 'full'
    AND fullForceResult.quality = 0
    AND fullForceResult.fault = 0 
    AND added_ff_id IS NULL
    -- addtool adds checks on exposure being added to the dvodb previously
