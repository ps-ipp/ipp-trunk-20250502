DELETE FROM detRunSummary
USING detRunSummary, detRun
WHERE
    fault != 0
AND
    state = 'run'
AND
    detRunSummary.det_id = detRun.det_id
