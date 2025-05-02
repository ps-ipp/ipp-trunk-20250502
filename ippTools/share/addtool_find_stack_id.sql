SELECT
    stackRun.*
FROM stackSumSkyfile
JOIN stackRun USING(stack_id)

LEFT JOIN (SELECT stack_id       AS added_stack_id,
                  addRun.dvodb AS previous_dvodb
           FROM addRun
JOIN stackRun on stack_id = stage_id
          ) as foo
     ON stack_id = added_stack_id 
     AND stage = 'stack'
     -- hook for qualifying the join on the previous_dvodb
     AND %s
WHERE
    stackRun.state = 'full'
    AND stackSumSkyfile.quality = 0
    AND added_exp_id IS NULL
    -- addtool adds checks on exposure being added to the dvodb previously
