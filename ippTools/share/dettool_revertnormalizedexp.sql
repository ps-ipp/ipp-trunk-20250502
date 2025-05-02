DELETE FROM detNormalizedExp
USING detNormalizedExp, detRun
WHERE
    fault != 0
AND
    state = 'run'
AND
    detNormalizedExp.det_id = detRun.det_id
