DELETE FROM detResidImfile
USING detResidImfile, detRun
WHERE
    fault != 0
AND
    state = 'run'
AND
    detResidImfile.det_id = detRun.det_id
