UPDATE rcRun
JOIN rcDestination USING(dest_id)
JOIN rcDSFileset using(dest_id, fs_id)
SET
