SELECT 
    lapRun.seq_id,
    lapRun.lap_id,
    ippRelease.rel_id
FROM lapRun
    JOIN ippRelease
    LEFT JOIN relGroup USING(lap_id)
WHERE (lapRun.state = 'done' or lapRun.state = 'full')
    AND relGroup.group_id IS NULL
-- AND lapRun.seq_id = xxx

