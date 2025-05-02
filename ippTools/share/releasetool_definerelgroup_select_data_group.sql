SELECT
    ippRelease.rel_id,
    count(exp_id) AS num_exposures
FROM ippRelease
    LEFT JOIN relExp using(rel_id)
    LEFT JOIN camRun using(cam_id)
    WHERE relExp.group_id = 0
    AND relExp.state != 'drop'

