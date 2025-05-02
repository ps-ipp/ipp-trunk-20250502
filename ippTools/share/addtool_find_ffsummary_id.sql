SELECT fullForceRun.* FROM fullForceRun
JOIN fullForceSummary USING (ff_id)
LEFT JOIN (
     SELECT ff_id AS added_ff_id,
            addRun.dvodb AS previous_dvodb
     FROM addRun
     JOIN fullForceRun ON (ff_id = stage_id and addRun.stage = 'fullforce_summary')
     ) as foo
ON ff_id = added_ff_id 
     -- AND stage = 'skycal'
     -- hook for qualifying the join on the previous_dvodb
     AND %s
WHERE
    fullForceRun.state = 'full'
    AND fullForceSummary.quality = 0
    AND fullForceSummary.fault = 0 
    AND added_ff_id IS NULL
    -- addtool adds checks on exposure being added to the dvodb previously
