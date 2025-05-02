-- is this DISTINCT needed?
SELECT DISTINCT
  detRun.det_type,
  rawExp.exp_time,
  detProcessedImfile.*
FROM detProcessedImfile
JOIN detRun
  USING(det_id)
JOIN detInputExp
  ON detRun.det_id = detInputExp.det_id
  AND detRun.iteration = detInputExp.iteration
  AND detProcessedImfile.exp_id = detInputExp.exp_id
JOIN rawExp
  ON rawExp.exp_id = detProcessedImfile.exp_id
