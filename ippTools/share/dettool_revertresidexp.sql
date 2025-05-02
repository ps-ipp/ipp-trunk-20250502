DELETE FROM detResidExp
USING detResidExp, detRun
WHERE
    fault != 0
AND
    state = 'run'
AND
    detResidExp.det_id = detRun.det_id
