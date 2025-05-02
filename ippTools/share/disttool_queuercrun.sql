-- create rcRun's for all destinations that have filesets that they are interested in available
INSERT INTO rcRun
SELECT                  -- rows in this select must match rcRun
    0,                  -- rc_id (auto-increment)
    fs_id,
    dest_id,
    'new',
    NULL,                -- status_fs
    NULL,                -- registered (database sets this to CURRENT_TIMESTAMP)
    0                    -- fault

FROM rcDestination 
JOIN rcInterest USING(dest_id) 
JOIN distTarget USING(target_id) 
JOIN distRun USING(target_id, stage) 
JOIN rcDSFileset USING(dest_id, dist_id)
LEFT JOIN rcRun using(fs_id, dest_id)
WHERE rcRun.rc_id IS NULL
    AND rcDSFileset.state = 'full'
    AND rcDSFileset.fault = 0
