SELECT DISTINCT
    rcDestination.*,
    count(fs_id) as pending_fs
FROM rcDestination
JOIN rcRun using(dest_id)
WHERE rcRun.state = 'new'
    AND rcDestination.state = 'enabled'
    AND status_uri IS NOT NULL
    -- where hook %s
GROUP BY dest_id
HAVING count(fs_id) > 0

