SELECT
   detRun.det_type,
   detRun.mode,
   rawExp.exp_time,
   detResidImfile.*
FROM detResidImfile
JOIN detRun
  USING(det_id, iteration)
JOIN detInputExp
  ON detRun.det_id = detInputExp.det_id
  AND detRun.iteration = detInputExp.iteration
  AND detResidImfile.exp_id = detInputExp.exp_id
JOIN rawExp
  ON rawExp.exp_id = detResidImfile.exp_id
