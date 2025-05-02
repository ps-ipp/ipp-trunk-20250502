DELETE FROM detStackedImfile
USING detStackedImfile, detRun
WHERE
    fault != 0
AND
    state = 'run'
AND
    detStackedImfile.det_id = detRun.det_id
