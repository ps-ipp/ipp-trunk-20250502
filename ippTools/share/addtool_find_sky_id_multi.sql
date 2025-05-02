SELECT
     distinct(sky_id) as stage_id,
     staticskyRun.*,
    staticskyResult.num_inputs, stack_id
FROM 
     staticskyRun
join staticskyResult using (sky_id)
join staticskyInput using(sky_id)
JOIN stackRun USING(stack_id)

LEFT JOIN (SELECT stack_id       AS added_stack_id,
                  addRun.dvodb AS previous_dvodb
           FROM addRun
JOIN staticskyRun on sky_id = stage_id
          ) as foo
     ON stack_id = added_stack_id 
     AND stage = 'staticsky'
     -- hook for qualifying the join on the previous_dvodb
     AND %s
WHERE
    staticskyRun.state = 'full'
    AND staticskyResult.quality = 0
    AND added_stack_id IS NULL
    -- addtool adds checks on exposure being added to the dvodb previously
