SELECT
    dist_id,
    target_id,
    distRun.stage,
    distRun.outdir as dist_dir,
    stage_id,
    distRun.data_group,
    distTarget.filter,
    rcDestination.name AS product_name,
    rcDestination.dest_id,
    rcDestination.dbname AS ds_dbname,
    rcDestination.dbhost AS ds_dbhost
FROM rcDestination 
JOIN rcInterest USING(dest_id) 
JOIN distTarget USING(target_id) 
JOIN distRun USING(target_id) 
LEFT JOIN rcDSFileset USING(dest_id, dist_id)
WHERE distRun.state = 'full'
    AND rcDestination.state = 'enabled'
    AND distTarget.state = 'enabled'
    AND rcInterest.state = 'enabled'
    AND rcDSFileset.fs_id IS NULL
