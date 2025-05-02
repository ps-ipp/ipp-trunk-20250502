-- relGroups to process
SELECT relGroup.*,
    ippRelease.release_id,
    ippRelease.release_name
FROM relGroup 
    JOIN ippRelease USING(rel_id)
WHERE relGroup.state ='new'
    AND relGroup.fault = 0
