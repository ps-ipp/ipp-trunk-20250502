DELETE FROM detProcessedImfile
USING detProcessedImfile, detRun
WHERE
    fault != 0
AND
    state = 'run'
AND
    detProcessedImfile.det_id = detRun.det_id
