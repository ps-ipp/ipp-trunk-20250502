SELECT
     diffRun.*, diffInputSkyfile.diff_skyfile_id
FROM 
     diffInputSkyfile 
JOIN diffSkyfile using (diff_id, skycell_id) 
JOIN diffRun using (diff_id)

LEFT JOIN (SELECT diff_id       AS added_diff_id,
                  addRun.dvodb AS previous_dvodb
           FROM addRun
JOIN diffRun on (diff_id = stage_id and addRun.stage = 'diff')
          ) as foo
     ON diff_id = added_diff_id 
     -- AND stage = 'diff'
     -- hook for qualifying the join on the previous_dvodb
     AND %s
WHERE
    diffRun.state = 'full'
    AND diffSkyfile.fault = 0
    AND diffSkyfile.quality = 0
    AND added_diff_id IS NULL
    -- addtool adds checks on exposure being added to the dvodb previously
