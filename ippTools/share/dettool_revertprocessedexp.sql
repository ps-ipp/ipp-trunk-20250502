DELETE FROM detProcessedExp
USING detProcessedExp, detRun
WHERE
    fault != 0
AND
    state = 'run'
AND
    detProcessedExp.det_id = detRun.det_id
