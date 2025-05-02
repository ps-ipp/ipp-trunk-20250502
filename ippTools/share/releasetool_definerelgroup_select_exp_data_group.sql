SELECT relExp.relexp_id
FROM relExp
    JOIN camRun USING(cam_id)
WHERE 
    relExp.group_id = 0
    AND relExp.state != 'drop'
