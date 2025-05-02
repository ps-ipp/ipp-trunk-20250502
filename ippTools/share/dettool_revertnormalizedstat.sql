DELETE FROM detNormalizedStatImfile
USING detNormalizedStatImfile, detRun
WHERE
    fault != 0
AND
    state = 'run'
AND
    detNormalizedStatImfile.det_id = detRun.det_id
