DELETE FROM detNormalizedImfile
USING detNormalizedImfile, detRun
WHERE
    fault != 0
AND
    state = 'run'
AND
    detNormalizedImfile.det_id = detRun.det_id
