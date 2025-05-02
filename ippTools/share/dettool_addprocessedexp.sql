SELECT DISTINCT
   detProcessedImfile.det_id,
   detRun.iteration,
   detRun.det_type,
   detProcessedImfile.exp_id
FROM detRun
JOIN detInputExp
   ON detRun.det_id = detInputExp.det_id
   AND detRun.iteration = detInputExp.iteration
JOIN rawExp
   ON detInputExp.exp_id = rawExp.exp_id
JOIN detProcessedImfile
   ON detInputExp.det_id = detProcessedImfile.det_id
   AND detInputExp.exp_id = detProcessedImfile.exp_id
LEFT JOIN detProcessedExp
   ON detInputExp.det_id = detProcessedExp.det_id
   AND detProcessedImfile.exp_id= detProcessedExp.exp_id
LEFT JOIN rawImfile
   ON detInputExp.exp_id = rawImfile.exp_id
   AND detProcessedImfile.class_id = rawImfile.class_id
WHERE
  detRun.state = 'run'
  AND (detRun.mode = 'master' or detRun.mode = 'verify')
  AND detProcessedExp.det_id IS NULL
  AND detProcessedExp.exp_id IS NULL
  AND detInputExp.include = 1
  AND detRun.det_id = %lld
  AND detProcessedImfile.exp_id = %lld
GROUP BY
   detProcessedImfile.class_id,
   rawImfile.class_id,
   detRun.det_id
HAVING
   COUNT(detProcessedImfile.class_id) = COUNT(rawImfile.class_id)
