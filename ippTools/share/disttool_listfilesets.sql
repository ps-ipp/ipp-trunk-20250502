SELECT
    fs_id,
    dist_id,
    distRun.label,
    rcDSFileset.state AS fileset_state,
    rcDSFileset.name AS fileset,
    rcDestination.name AS product,
    rcDestination.dbname,
    rcDestination.dbhost
FROM rcDSFileset
JOIN distRun USING(dist_id)
JOIN distTarget USING(target_id, stage, clean)
JOIN rcInterest USING(target_id, dest_id)
JOIN rcDestination USING(dest_id)
