SELECT 
    dist_id,
    stage,
    stage_id,
    label,
    outdir
FROM distRun
WHERE state = 'goto_cleaned'
    AND distRun.fault = 0
